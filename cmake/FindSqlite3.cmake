############################################################################
# FindSqlite3.cmake
# Copyright (C) 2014  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################
#
# - Find the sqlite3 include file and library
#
#  SQLITE3_FOUND - system has sqlite3
#  SQLITE3_INCLUDE_DIRS - the sqlite3 include directory
#  SQLITE3_LIBRARIES - The libraries needed to use sqlite3

if(APPLE AND NOT IOS)
	set(SQLITE3_HINTS "/usr")
endif()
if(SQLITE3_HINTS)
	set(SQLITE3_LIBRARIES_HINTS "${SQLITE3_HINTS}/lib")
endif()

find_path(SQLITE3_INCLUDE_DIRS
	NAMES sqlite3.h
	HINTS "${SQLITE3_HINTS}"
	PATH_SUFFIXES include
)

if(SQLITE3_INCLUDE_DIRS)
	set(HAVE_SQLITE3_H 1)
endif()

find_library(SQLITE3_LIBRARIES
	NAMES sqlite3
	HINTS "${SQLITE3_LIBRARIES_HINTS}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sqlite3
	DEFAULT_MSG
	SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES HAVE_SQLITE3_H
)

mark_as_advanced(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES HAVE_SQLITE3_H)
