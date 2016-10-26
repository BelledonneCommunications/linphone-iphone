############################################################################
# FindIconv.cmake
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
# - Find the iconv include file and library
#
#  ICONV_FOUND - system has libiconv
#  ICONV_INCLUDE_DIRS - the libiconv include directory
#  ICONV_LIBRARIES - The libraries needed to use libiconv

if(APPLE AND NOT IOS)
	set(ICONV_HINTS "${CMAKE_OSX_SYSROOT}/usr" "/usr")
endif()
if(ICONV_HINTS)
	set(ICONV_LIBRARIES_HINTS "${ICONV_HINTS}/lib")
endif()

find_path(ICONV_INCLUDE_DIRS
	NAMES iconv.h
	HINTS "${ICONV_HINTS}"
	PATH_SUFFIXES include
)

if(ICONV_INCLUDE_DIRS)
	set(HAVE_ICONV_H 1)
endif()

find_library(ICONV_LIBRARIES
	NAMES iconv
	HINTS "${ICONV_LIBRARIES_HINTS}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iconv
	DEFAULT_MSG
	ICONV_INCLUDE_DIRS ICONV_LIBRARIES HAVE_ICONV_H
)

mark_as_advanced(ICONV_INCLUDE_DIRS ICONV_LIBRARIES HAVE_ICONV_H)
