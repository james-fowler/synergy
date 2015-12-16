/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Inc.
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

#include "synergy/Chunk.h"
#include "base/String.h"
#include "base/Log.h"

Chunk::Chunk(size_t size)
{
	m_chunk = new char[size];
	memset(m_chunk, 0, size);
	LOG((CLOG_DEBUG5 "Chunk @%p allocated %d bytes @%p", (void*)this, (int)size, (void*) m_chunk ));
}

Chunk::~Chunk()
{
	LOG((CLOG_DEBUG5 "Chunk @%p freeing bytes @%p", (void*)this, (void*) m_chunk ));

	delete[] m_chunk;
}
