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

#include "synergy/IClipboard.h"

//! Memory buffer clipboard
/*!
This class implements a clipboard that stores data in memory.
*/
class Clipboard : public IClipboard {
public:
	Clipboard();
	virtual ~Clipboard();

	//! @name manipulators
	//@{

	//! Unmarshall clipboard data
	/*!
	Extract marshalled clipboard data and store it in this clipboard.
	Sets the clipboard time to \c time.
	*/
	void				unmarshall(const String& data, Time time);

	//@}
	//! @name accessors
	//@{

	//! Marshall clipboard data
	/*!
	Merge this clipboard's data into a single buffer that can be later
	unmarshalled to restore the clipboard and return the buffer.
	*/
	String				marshall() const;

	//@}

	// IClipboard overrides
	virtual bool		v_empty();
	virtual void		v_add(EFormat, const String& data);
	virtual bool		v_open(Time) const;
	virtual void		v_close() const;
	virtual Time		v_getTime() const;
	virtual bool		v_has(EFormat) const;
	virtual String		v_get(EFormat) const;
	virtual void 		v_dump_internals( IClipboardDumper & ) const;

private:
	mutable bool		m_open;
	mutable Time		m_time;
	bool				m_owner;
	Time				m_timeOwned;
	bool				m_added[kNumFormats];
	String				m_data[kNumFormats];
};
