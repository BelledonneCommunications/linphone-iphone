############################################################################
# FindBelcard.cmake
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
############################################################################
#
# - Find the belcard include file and library
#
#  BELCARD_FOUND - system has belcard
#  BELCARD_INCLUDE_DIRS - the belcard include directory
#  BELCARD_LIBRARIES - The libraries needed to use belcard

set(_BELCARD_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(BELCARD_INCLUDE_DIRS
	NAMES belcard.hpp
	HINTS _BELCARD_ROOT_PATHS
	PATH_SUFFIXES include
)

if(BELCARD_INCLUDE_DIRS)
	set(HAVE_BELCARD_H 1)
endif()

find_library(BELCARD_LIBRARIES
	NAMES belr belcard
	HINTS ${_BELCARD_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Belcard
	DEFAULT_MSG
	BELCARD_INCLUDE_DIRS BELCARD_LIBRARIES HAVE_BELCARD_H
)

mark_as_advanced(BELCARD_INCLUDE_DIRS BELCARD_LIBRARIES HAVE_BELCARD_H)
