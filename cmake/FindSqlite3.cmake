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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################
#
# - Find the sqlite3 include file and library
#
#  SQLITE3_FOUND - system has sqlite3
#  SQLITE3_INCLUDE_DIRS - the sqlite3 include directory
#  SQLITE3_LIBRARIES - The libraries needed to use sqlite3

set(_SQLITE3_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(SQLITE3_INCLUDE_DIRS
	NAMES sqlite3.h
	HINTS _SQLITE3_ROOT_PATHS
	PATH_SUFFIXES include
)

if(SQLITE3_INCLUDE_DIRS)
	set(HAVE_SQLITE3_H 1)
endif()

find_library(SQLITE3_LIBRARIES
	NAMES sqlite3
	HINTS ${_SQLITE3_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sqlite3
	DEFAULT_MSG
	SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES HAVE_SQLITE3_H
)

mark_as_advanced(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES HAVE_SQLITE3_H)
