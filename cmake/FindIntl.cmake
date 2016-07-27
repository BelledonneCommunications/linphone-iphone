############################################################################
# FindIntl.cmake
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
# - Find the libintl include file and library
#
#  INTL_FOUND - system has libintl
#  INTL_INCLUDE_DIRS - the libintl include directory
#  INTL_LIBRARIES - The libraries needed to use libintl

set(_INTL_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(INTL_INCLUDE_DIRS
	NAMES libintl.h
	HINTS _INTL_ROOT_PATHS
	PATH_SUFFIXES include
)

if(INTL_INCLUDE_DIRS)
	set(HAVE_LIBINTL_H 1)
endif()

set(INTL_ARGS INTL_INCLUDE_DIRS HAVE_LIBINTL_H)
if(NOT UNIX OR APPLE)
	find_library(INTL_LIBRARIES
		NAMES intl
		HINTS ${_INTL_ROOT_PATHS}
		PATH_SUFFIXES bin lib
	)
	list(APPEND INTL_ARGS INTL_LIBRARIES)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Intl DEFAULT_MSG ${INTL_ARGS})

mark_as_advanced(${INTL_ARGS})
