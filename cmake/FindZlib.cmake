############################################################################
# FindZlib.txt
# Copyright (C) 2015  Belledonne Communications, Grenoble France
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
# - Find the zlib include file and library
#
#  ZLIB_FOUND - system has zlib
#  ZLIB_INCLUDE_DIRS - the zlib include directory
#  ZLIB_LIBRARIES - The libraries needed to use zlib

if(APPLE AND NOT IOS)
	set(ZLIB_HINTS "/usr")
endif()
if(ZLIB_HINTS)
	set(ZLIB_LIBRARIES_HINTS "${ZLIB_HINTS}/lib")
endif()

find_path(ZLIB_INCLUDE_DIRS
	NAMES zlib.h
	HINTS "${ZLIB_HINTS}"
	PATH_SUFFIXES include
)

if(ZLIB_INCLUDE_DIRS)
	set(HAVE_ZLIB_H 1)
endif()

if(ENABLE_STATIC)
	find_library(ZLIB_LIBRARIES
		NAMES zstatic zlibstatic zlibstaticd z
		HINTS "${ZLIB_LIBRARIES_HINTS}"
	)
else()
	find_library(ZLIB_LIBRARIES
		NAMES z zlib zlibd
		HINTS "${ZLIB_LIBRARIES_HINTS}"
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Zlib
	DEFAULT_MSG
	ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES HAVE_ZLIB_H
)

mark_as_advanced(ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES HAVE_ZLIB_H)
