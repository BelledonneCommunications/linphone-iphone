############################################################################
# FindiBcToolBox.cmake
# Copyright (C) 2016  Belledonne Communications, Grenoble France
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
# - Find the bctoolbox include file and library
#
#  BCTOOLBOX_FOUND - system has BC Toolbox
#  BCTOOLBOX_INCLUDE_DIRS - the BC Toolbox include directory
#  BCTOOLBOX_LIBRARIES - The libraries needed to use BC Toolbox

include(CMakePushCheckState)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckSymbolExists)

set(_BCTOOLBOX_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(BCTOOLBOX_INCLUDE_DIRS
	NAMES bctoolbox/crypto.h
	HINTS _BCTOOLBOX_ROOT_PATHS
	PATH_SUFFIXES include
)

find_library(BCTOOLBOX_LIBRARIES
	NAMES bctoolbox
	HINTS _BCTOOLBOX_ROOT_PATHS
	PATH_SUFFIXES bin lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BcToolBox
	DEFAULT_MSG
	BCTOOLBOX_INCLUDE_DIRS BCTOOLBOX_LIBRARIES
)

mark_as_advanced(BCTOOLBOX_INCLUDE_DIRS BCTOOLBOX_LIBRARIES)
