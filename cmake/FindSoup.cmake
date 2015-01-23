############################################################################
# FindSoup.cmake
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
# - Find the soup include file and library
#
#  SOUP_FOUND - system has libsoup
#  SOUP_INCLUDE_DIRS - the libsoup include directory
#  SOUP_LIBRARIES - The libraries needed to use libsoup

find_package(GTK2 2.18 REQUIRED gtk)

set(_SOUP_ROOT_PATHS
	${WITH_SOUP}
	${CMAKE_INSTALL_PREFIX}
)

find_path(SOUP_INCLUDE_DIRS
	NAMES libsoup/soup.h
	HINTS _SOUP_ROOT_PATHS
	PATH_SUFFIXES include/libsoup-2.4
)

if(SOUP_INCLUDE_DIRS)
	set(HAVE_LIBSOUP_SOUP_H 1)
	list(APPEND SOUP_INCLUDE_DIRS ${GTK2_INCLUDE_DIRS})
endif()

find_library(SOUP_LIBRARIES
	NAMES soup-2.4
	HINTS ${_SOUP_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

if(SOUP_LIBRARIES)
	list(APPEND SOUP_LIBRARIES ${GTK2_LIBRARIES})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Soup
	DEFAULT_MSG
	SOUP_INCLUDE_DIRS SOUP_LIBRARIES HAVE_LIBSOUP_SOUP_H
)

mark_as_advanced(SOUP_INCLUDE_DIRS SOUP_LIBRARIES HAVE_LIBSOUP_SOUP_H)
