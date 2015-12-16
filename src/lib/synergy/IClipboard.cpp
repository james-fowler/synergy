/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2004 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synergy/IClipboard.h"
#include "common/stdvector.h"
#include "synergy/IClipboardAccess.h"
#include "base/Log.h"

#include "mt/Mutex.h"
#include "mt/Lock.h"
#include <typeinfo>
#include <iostream>

//
// IClipboard
//

#ifdef SYN_ENABLE_CLIPBOARD_DEBUGGING

int IClipboard::next_instance_id = 0;
struct IClipboardTracker {
	Mutex _add_clipboard_mutex;
	std::vector< IClipboard *> _active_clipboards;
};
static IClipboardTracker * _tracker;
static IClipboardTracker &tracker() {
	if( _tracker == 0 ) {
		_tracker = new IClipboardTracker();
		_tracker->_active_clipboards.push_back(0);
	}
	return *_tracker;
}


IClipboard::IClipboard() :
			_instance_id( -1 )
{
	Lock l(&tracker()._add_clipboard_mutex);

	_instance_id = tracker()._active_clipboards.size();
	tracker()._active_clipboards.push_back( this );
}

IClipboard::~IClipboard() {
	if( _instance_id >= 0 ) {
		if( tracker()._active_clipboards[_instance_id] == this ) {
			tracker()._active_clipboards[_instance_id] = 0;
		}
		_instance_id = -_instance_id;
	}
}

void IClipboardAccess::dump_clipboards() {
	LOG((CLOG_NOTE "dumping clipboards : %d active", (int) tracker()._active_clipboards.size() ));

	for( size_t i = 0; i < tracker()._active_clipboards.size(); i++ ) {
		IClipboard *cp = tracker()._active_clipboards[i];
		if( cp ) {
			//std::cout << " " << i << " : " << typeid(*cp).name() << " : ";
			IClipboardDumper d;
			cp->v_dump_internals( d );
			std::string info = d._out.str();
			LOG((CLOG_NOTE " %d (%s) : %s ", (int)i, (const char *)(typeid(*cp).name()), info.c_str() ) );
		}
	}
};


#else

IClipboard::IClipboard() {}
IClipboard::~IClipboard() {}
void IClipboardAccess::dump_clipboards() {
	std::cerr << "dump_clipboards() : synergy not built with SYN_ENABLE_CLIPBOARD_DEBUGGING set" << std::endl;
};

#endif

static const char *format_desc( IClipboard::EFormat f ) {
	switch( f ) {
	case IClipboard::kText: return "Text";
	case IClipboard::kBitmap: return "Bitmap";
	case IClipboard::kHTML: return "HTML";
	}
	return "???";
};

IClipboardDumper &IClipboardDumper::wclipcontents( const t_m_added &added, const t_m_data &data ) {
	out() << " {";
	for( int x=0;x < IClipboard::kNumFormats; x++ ) {
		wclipdata( (IClipboard::EFormat)(x), data[x], added[x] );
	}
	out() << "}";
}

IClipboardDumper &IClipboardDumper::wclipdata( IClipboard::EFormat f, const String &data, bool added ) {
	if( added ) {
		out() << "{" << format_desc(f);
		if( f != IClipboard::kBitmap ) {
			out() << ":";
			wrepr( data, 20 );
		}
		out() << "},";
	}
	return *this;
}

IClipboardDumper &IClipboardDumper::wrepr( const std::string &str, size_t max ) {

	size_t last = str.size();
	if( !max ) {
		max = last;
	} else {
		if( max < last ) last = max;
	}
	out() << "\"";
	for( size_t i = 0; i < last; i++ ) {
		char c = str[i];
		if( c >= 32 ) {
			switch(c) {
			case '\\': out() << "\\\\"; break;
			case '"': out() << "\\\""; break;
			default: out() << c;
			}
		} else {
			switch(c) {
			case '\n': out() << "\\n"; break;
			case '\t': out() << "\\t"; break;
			case '\r': out() << "\\r"; break;
			default: out() << "\\x" << std::setw(2) << std::hex << (unsigned) c;
			}
		}
	}
	if( str.size() > last ) out() << "...";
	out() << "\"";
	return *this;
}


bool IClipboard::empty() { return v_empty(); }

void IClipboard::add(EFormat f, const String& data) { v_add( f, data); }

bool IClipboard::open(Time time) const { return v_open(time); }

void IClipboard::close() const { v_close(); }

IClipboard::Time IClipboard::getTime() const { return v_getTime(); }

bool IClipboard::has(EFormat f) const { return v_has(f); }

String IClipboard::get(EFormat f) const { return v_get(f); }

void
IClipboard::unmarshall(IClipboard* clipboard, const String& data, Time time)
{
	assert(clipboard != NULL);

	const char* index = data.data();

	if (clipboard->open(time)) {
		// clear existing data
		clipboard->empty();

		// read the number of formats
		const UInt32 numFormats = readUInt32(index);
		index += 4;

		// read each format
		for (UInt32 i = 0; i < numFormats; ++i) {
			// get the format id
			IClipboard::EFormat format =
				static_cast<IClipboard::EFormat>(readUInt32(index));
			index += 4;

			// get the size of the format data
			UInt32 size = readUInt32(index);
			index += 4;

			// save the data if it's a known format.  if either the client
			// or server supports more clipboard formats than the other
			// then one of them will get a format >= kNumFormats here.
			if (format <IClipboard::kNumFormats) {
				clipboard->add(format, String(index, size));
			}
			index += size;
		}

		// done
		clipboard->close();
	}
}

String
IClipboard::marshall(const IClipboard* clipboard)
{
	// return data format: 
	// 4 bytes => number of formats included
	// 4 bytes => format enum
	// 4 bytes => clipboard data size n
	// n bytes => clipboard data
	// back to the second 4 bytes if there is another format
	
	assert(clipboard != NULL);

	String data;

	std::vector<String> formatData;
	formatData.resize(IClipboard::kNumFormats);
	// FIXME -- use current time
	if (clipboard->open(0)) {

		// compute size of marshalled data
		UInt32 size = 4;
		UInt32 numFormats = 0;
		for (UInt32 format = 0; format != IClipboard::kNumFormats; ++format) {
			if (clipboard->has(static_cast<IClipboard::EFormat>(format))) {
				++numFormats;
				formatData[format] =
					clipboard->get(static_cast<IClipboard::EFormat>(format));
				size += 4 + 4 + (UInt32)formatData[format].size();
			}
		}

		// allocate space
		data.reserve(size);

		// marshall the data
		writeUInt32(&data, numFormats);
		for (UInt32 format = 0; format != IClipboard::kNumFormats; ++format) {
			if (clipboard->has(static_cast<IClipboard::EFormat>(format))) {
				writeUInt32(&data, format);
				writeUInt32(&data, (UInt32)formatData[format].size());
				data += formatData[format];
			}
		}
		clipboard->close();
	}

	return data;
}

bool
IClipboard::copy(IClipboard* dst, const IClipboard* src)
{
	assert(dst != NULL);
	assert(src != NULL);

	return copy(dst, src, src->getTime());
}

bool
IClipboard::copy(IClipboard* dst, const IClipboard* src, Time time)
{
	assert(dst != NULL);
	assert(src != NULL);

	bool success = false;
	if (src->open(time)) {
		if (dst->open(time)) {
			if (dst->empty()) {
				for (SInt32 format = 0;
								format != IClipboard::kNumFormats; ++format) {
					IClipboard::EFormat eFormat = (IClipboard::EFormat)format;
					if (src->has(eFormat)) {
						dst->add(eFormat, src->get(eFormat));
					}
				}
				success = true;
			}
			dst->close();
		}
		src->close();
	}

	return success;
}

UInt32
IClipboard::readUInt32(const char* buf)
{
	const unsigned char* ubuf = reinterpret_cast<const unsigned char*>(buf);
	return	(static_cast<UInt32>(ubuf[0]) << 24) |
			(static_cast<UInt32>(ubuf[1]) << 16) |
			(static_cast<UInt32>(ubuf[2]) <<  8) |
			 static_cast<UInt32>(ubuf[3]);
}

void
IClipboard::writeUInt32(String* buf, UInt32 v)
{
	*buf += static_cast<UInt8>((v >> 24) & 0xff);
	*buf += static_cast<UInt8>((v >> 16) & 0xff);
	*buf += static_cast<UInt8>((v >>  8) & 0xff);
	*buf += static_cast<UInt8>( v        & 0xff);
}
