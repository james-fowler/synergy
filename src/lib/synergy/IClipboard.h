/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#pragma once

#include "base/String.h"
#include "base/EventTypes.h"
#include "common/IInterface.h"

#define SYN_ENABLE_CLIPBOARD_DEBUGGING


class IClipboardDumper;

//! Clipboard interface
/*!
This interface defines the methods common to all clipboards.
*/
class IClipboard : public IInterface {
public:
	//! Timestamp type
	/*!
	Timestamp type.  Timestamps are in milliseconds from some
	arbitrary starting time.  Timestamps will wrap around to 0
	after about 49 3/4 days.
	*/
	typedef UInt32 Time;

	//! Clipboard formats
	/*!
	The list of known clipboard formats.  kNumFormats must be last and
	formats must be sequential starting from zero.  Clipboard data set
	via add() and retrieved via get() must be in one of these formats.
	Platform dependent clipboard subclasses can and should present any
	suitable formats derivable from these formats.

	\c kText is a text format encoded in UTF-8.  Newlines are LF (not
	CR or LF/CR).

	\c kBitmap is an image format.  The data is a BMP file without the
	14 byte header (i.e. starting at the INFOHEADER) and with the image
	data immediately following the 40 byte INFOHEADER.

	\c kHTML is a text format encoded in UTF-8 and containing a valid
	HTML fragment (but not necessarily a complete HTML document).
	Newlines are LF.
	*/
	enum EFormat {
		kText,			//!< Text format, UTF-8, newline is LF
		kBitmap,		//!< Bitmap format, BMP 24/32bpp, BI_RGB
		kHTML,			//!< HTML format, HTML fragment, UTF-8, newline is LF
		kNumFormats		//!< The number of clipboard formats
	};

	IClipboard();
	virtual ~IClipboard();

	//! @name manipulators
	//@{

	//! Empty clipboard
	/*!
	Take ownership of the clipboard and clear all data from it.
	This must be called between a successful open() and close().
	Return false if the clipboard ownership could not be taken;
	the clipboard should not be emptied in this case.
	*/
	bool				empty();
	virtual bool		v_empty() = 0;

	//! Add data
	/*!
	Add data in the given format to the clipboard.  May only be
	called after a successful empty().
	*/
	void				add(EFormat, const String& data);
	virtual void		v_add(EFormat, const String& data) = 0;

	//@}
	//! @name accessors
	//@{

	//! Open clipboard
	/*!
	Open the clipboard.  Return true iff the clipboard could be
	opened.  If open() returns true then the client must call
	close() at some later time;  if it returns false then close()
	must not be called.  \c time should be the current time or
	a time in the past when the open should effectively have taken
	place.
	*/
	bool				open(Time time) const;
	virtual bool		v_open(Time time) const = 0;

	//! Close clipboard
	/*!
	Close the clipboard.  close() must match a preceding successful
	open().  This signals that the clipboard has been filled with
	all the necessary data or all data has been read.  It does not
	mean the clipboard ownership should be released (if it was
	taken).
	*/
	void				close() const;
	virtual void		v_close() const = 0;

	//! Get time
	/*!
	Return the timestamp passed to the last successful open().
	*/
	Time				getTime() const;
	virtual Time		v_getTime() const = 0;

	//! Check for data
	/*!
	Return true iff the clipboard contains data in the given
	format.  Must be called between a successful open() and close().
	*/
	bool				has(EFormat) const;
	virtual bool		v_has(EFormat) const = 0;

	//! Get data
	/*!
	Return the data in the given format.  Returns the empty string
	if there is no data in that format.  Must be called between
	a successful open() and close().
	*/
	String				get(EFormat) const;
	virtual String		v_get(EFormat) const = 0;


	virtual void v_dump_internals( IClipboardDumper & ) const = 0;

	//! Marshall clipboard data
	/*!
	Merge \p clipboard's data into a single buffer that can be later
	unmarshalled to restore the clipboard and return the buffer.
	*/
	static String		marshall(const IClipboard* clipboard);

	//! Unmarshall clipboard data
	/*!
	Extract marshalled clipboard data and store it in \p clipboard.
	Sets the clipboard time to \c time.
	*/
	static void			unmarshall(IClipboard* clipboard,
							const String& data, Time time);

	//! Copy clipboard
	/*!
	Transfers all the data in one clipboard to another.  The
	clipboards can be of any concrete clipboard type (and
	they don't have to be the same type).  This also sets
	the destination clipboard's timestamp to source clipboard's
	timestamp.  Returns true iff the copy succeeded.
	*/
	static bool			copy(IClipboard* dst, const IClipboard* src);

	//! Copy clipboard
	/*!
	Transfers all the data in one clipboard to another.  The
	clipboards can be of any concrete clipboard type (and they
	don't have to be the same type).  This also sets the
	timestamp to \c time.  Returns true iff the copy succeeded.
	*/
	static bool			copy(IClipboard* dst, const IClipboard* src, Time);

	//@}

private:
	static UInt32		readUInt32(const char*);
	static void			writeUInt32(String*, UInt32);

#ifdef SYN_ENABLE_CLIPBOARD_DEBUGGING
	friend class IClipboardAccess;
	friend class IClipboardDumper;
	static int next_instance_id;
	int _instance_id;
public:

#endif
};
