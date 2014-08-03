############################################################################
# CMakeLists.txt
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

cmake_minimum_required(VERSION 2.6)
project(LINPHONE C)


option(ENABLE_STATIC "Build static library (default is shared library)." NO)
option(ENABLE_VIDEO "Build with video support." YES)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_PREFIX_PATH}/share/cmake/Modules)

include(CheckIncludeFile)

if(MSVC)
	list(APPEND CMAKE_REQUIRED_INCLUDES ${CMAKE_PREFIX_PATH}/include/MSVC)
endif()

find_package(BelleSIP REQUIRED)
find_package(MS2 REQUIRED)
find_package(XML2 REQUIRED)


include_directories(
	include/
	coreapi/
	${CMAKE_CURRENT_BINARY_DIR}/coreapi/
	${BELLESIP_INCLUDE_DIRS}
	${MS2_INCLUDE_DIRS}
	${XML2_INCLUDE_DIRS}
)

if(MSVC)
	include_directories(${CMAKE_PREFIX_PATH}/include/MSVC)
endif()


add_subdirectory(coreapi)
add_subdirectory(share)
