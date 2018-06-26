############################################################################
# FindLibXsd.cmake
# Copyright (C) 2017  Belledonne Communications, Grenoble France
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
# - Find the libxsd library
#
#  LIBXSD_FOUND - system has libxsd
#  LIBXSD_INCLUDE_DIRS - the libxsd include directory
#  LIBXSD_LIBRARIES - The libraries needed to use libxsd


find_package(XercesC)

find_path(LIBXSD_INCLUDE_DIRS
	NAMES xsd/cxx/config.hxx
	PATH_SUFFIXES include
)

if(LIBXSD_INCLUDE_DIRS)
	list(APPEND LIBXSD_INCLUDE_DIRS ${XercesC_INCLUDE_DIRS})
endif()
set(LIBXSD_LIBRARIES ${XercesC_LIBRARIES})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibXsd
	DEFAULT_MSG
	LIBXSD_INCLUDE_DIRS LIBXSD_LIBRARIES
)

mark_as_advanced(LIBXSD_INCLUDE_DIRS LIBXSD_LIBRARIES)
