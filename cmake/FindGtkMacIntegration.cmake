############################################################################
# FindGtkMacIntegration.txt
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
# - Find the libgtkmacintegration include file and library
#
#  GTKMACINTEGRATION_FOUND - system has libgtkmacintegration
#  GTKMACINTEGRATION_INCLUDE_DIRS - the libgtkmacintegration include directory
#  GTKMACINTEGRATION_LIBRARIES - The libraries needed to use libgtkmacintegration
#  GTKMACINTEGRATION_CPPFLAGS - The cflags needed to use libgtkmacintegration

set(_GTKMACINTEGRATION_ROOT_PATHS
	${CMAKE_INSTALL_PREFIX}
)

find_path(GTKMACINTEGRATION_INCLUDE_DIRS
	NAMES gtkosxapplication.h
	HINTS _GTKMACINTEGRATION_ROOT_PATHS
	PATH_SUFFIXES include/gtkmacintegration-gtk2 include/gtkmacintegration
)

find_library(GTKMACINTEGRATION_LIBRARIES
	NAMES gtkmacintegration-gtk2 gtkmacintegration
	HINTS ${_GTKMACINTEGRATION_ROOT_PATHS}
	PATH_SUFFIXES bin lib
)

set(GTKMACINTEGRATION_CPPFLAGS "-DMAC_INTEGRATION")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTKMACINTEGRATION
	DEFAULT_MSG
	GTKMACINTEGRATION_INCLUDE_DIRS GTKMACINTEGRATION_LIBRARIES GTKMACINTEGRATION_CPPFLAGS
)

mark_as_advanced(GTKMACINTEGRATION_INCLUDE_DIRS GTKMACINTEGRATION_LIBRARIES GTKMACINTEGRATION_CPPFLAGS)
