/*
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

#include "synergy/IClipboard.h"
#include "common/basic_types.h"
#include <sstream>
#include <iomanip>

class IClipboardAccess {
public:
	static void dump_clipboards();
};


class IClipboardDumper {
public:
	IClipboardDumper() {}
	std::ostream &out() { return _out; }

	template <class T>
	IClipboardDumper &wattr( const std::string &name, T val ) {
		out() << " " << name << ":" << val;
		return *this;
	};
	IClipboardDumper &wattr( const std::string &name, UInt8 val ) {
		return wattr( name, (unsigned)val );
	}

	typedef bool				t_m_added[IClipboard::kNumFormats];
	typedef String				t_m_data[IClipboard::kNumFormats];

	IClipboardDumper &wclipcontents( const t_m_added &added, const t_m_data &data );
	IClipboardDumper &wclipdata( IClipboard::EFormat f, const String &data, bool added=true );
	IClipboardDumper &wrepr( const std::string &str, size_t max=0 );


	std::ostringstream _out;

};
