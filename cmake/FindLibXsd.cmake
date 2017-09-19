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
#  XSD_FOUND - system has libxsd
#  XSD_LIBRARIES - The libraries needed to use libxsd

if(APPLE)
set(XSDCXX_DEFAULT_ROOT_PATH "/usr/local")
else ()
set(XSDCXX_DEFAULT_ROOT_PATH "/usr")
endif()

set(XSDCXX_ROOT_PATH ${XSDCXX_DEFAULT_ROOT_PATH}  CACHE STRING "Path of where the bin/xsdcxx executable will be found. Comes from http://www.codesynthesis.com/products/xsd/download.xhtml. On mac use 'brew install xsd'")

find_program(XSDCXX_PROG NAMES "xsdcxx" "xsd"
        HINTS ${XSDCXX_ROOT_PATH}/bin
)
if(XSDCXX_PROG)
        set(XSD_FOUND 1)
        message(STATUS "XSD found at ${XSDCXX_PROG}, enabling XSD")
        # TODO: check XSD is the correct executable
        find_library(XERCES_LIBS NAMES xerces-c)
        if(NOT XERCES_LIBS)
                message(FATAL_ERROR "Failed to find the Xerces library.")
        endif()
        find_path(XERCES_INCLUDE_DIRS NAMES xercesc/util/XercesDefs.hpp)
        if(NOT XERCES_INCLUDE_DIRS)
                message(FATAL_ERROR "Failed to find the Xerces includes.")
        endif()
        set(XSD_LIBRARIES ${XERCES_LIBS})
else()
        set(XSD_FOUND 0)
        message(STATUS "Program 'xsdcxx' could not be found in ${XSDCXX_ROOT_PATH}/bin, disabling XSD features")
endif()

mark_as_advanced(XSD_FOUND)
