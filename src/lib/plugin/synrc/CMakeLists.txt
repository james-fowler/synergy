# synergy -- mouse and keyboard sharing utility
# Copyright (C) 2012 Synergy Si Ltd.
# Copyright (C) 2012 Nick Bolton
# 
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
# 
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

if (SYNERGY_ADD_HEADERS)
	list(APPEND sources ${headers})
endif()

include_directories(
	../../../lib/
	../../../..
)

add_library(synrc SHARED ${sources})

add_custom_command(
	TARGET winmmjoy 
	POST_BUILD
	COMMAND xcopy /Y /Q
		..\\..\\..\\..\\..\\lib\\${CMAKE_CFG_INTDIR}\\synrc.*
		..\\..\\..\\..\\..\\bin\\${CMAKE_CFG_INTDIR}\\plugins\\
)
