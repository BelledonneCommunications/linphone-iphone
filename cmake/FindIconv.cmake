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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################
#
# - Find the iconv include file and library
#
#  ICONV_FOUND - system has libiconv
#  ICONV_INCLUDE_DIRS - the libiconv include directory
#  ICONV_LIBRARIES - The libraries needed to use libiconv

set(_ICONV_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(ICONV_INCLUDE_DIRS
	NAMES iconv.h
	HINTS _ICONV_ROOT_PATHS
	PATH_SUFFIXES include
)

if(ICONV_INCLUDE_DIRS)
	set(HAVE_ICONV_H 1)
endif()

find_library(ICONV_LIBRARIES
	NAMES iconv
	HINTS ${_ICONV_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Iconv
	DEFAULT_MSG
	ICONV_INCLUDE_DIRS ICONV_LIBRARIES HAVE_ICONV_H
)

mark_as_advanced(ICONV_INCLUDE_DIRS ICONV_LIBRARIES HAVE_ICONV_H)
