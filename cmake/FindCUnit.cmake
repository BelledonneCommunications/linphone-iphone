############################################################################
# FindCUnit.txt
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
# - Find the CUnit include file and library
#
#  CUNIT_FOUND - system has CUnit
#  CUNIT_INCLUDE_DIRS - the CUnit include directory
#  CUNIT_LIBRARIES - The libraries needed to use CUnit

include(CheckIncludeFile)
include(CheckLibraryExists)

set(_CUNIT_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(CUNIT_INCLUDE_DIRS
	NAMES CUnit/CUnit.h
	HINTS _CUNIT_ROOT_PATHS
	PATH_SUFFIXES include
)

if(CUNIT_INCLUDE_DIRS)
	set(HAVE_CUNIT_CUNIT_H 1)
endif()

find_library(CUNIT_LIBRARIES
	NAMES cunit
	HINTS ${_CUNIT_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CUnit
	DEFAULT_MSG
	CUNIT_INCLUDE_DIRS CUNIT_LIBRARIES
)

mark_as_advanced(CUNIT_INCLUDE_DIRS CUNIT_LIBRARIES)
