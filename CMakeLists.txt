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
project(LINPHONE C CXX)


set(LINPHONE_MAJOR_VERSION "3")
set(LINPHONE_MINOR_VERSION "7")
set(LINPHONE_MICRO_VERSION "0")
set(LINPHONE_VERSION "${LINPHONE_MAJOR_VERSION}.${LINPHONE_MINOR_VERSION}.${LINPHONE_MICRO_VERSION}")
set(LINPHONE_SO_VERSION "6")


include(CMakeDependentOption)

option(ENABLE_STATIC "Build static library (default is shared library)." NO)
option(ENABLE_CONSOLE_UI "Turn on or off compilation of console interface." YES)
option(ENABLE_DATE "Use build date in internal version number." NO)
option(ENABLE_GTK_UI "Turn on or off compilation of gtk interface." YES)
option(ENABLE_LDAP "Enable LDAP support." NO)
option(ENABLE_MSG_STORAGE "Turn on compilation of message storage." YES)
option(ENABLE_NOTIFY "Enable libnotify support." YES)
option(ENABLE_RELATIVE_PREFIX "Find resources relatively to the installation directory." NO)
option(ENABLE_TOOLS "Turn on or off compilation of console interface" YES)
option(ENABLE_TUNNEL "Turn on compilation of tunnel support." NO)
option(ENABLE_TUTORIALS "Enable compilation of tutorials." YES)
option(ENABLE_UNIT_TESTS "Enable compilation of unit tests." YES)
option(ENABLE_UPNP "Build with uPnP support." YES)
option(ENABLE_VIDEO "Build with video support." YES)
cmake_dependent_option(ENABLE_ASSISTANT "Turn on assistant compiling." YES "ENABLE_GTK_UI" NO)


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
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/coreapi/
	${BELLESIP_INCLUDE_DIRS}
	${MS2_INCLUDE_DIRS}
	${XML2_INCLUDE_DIRS}
)

if(MSVC)
	include_directories(${CMAKE_PREFIX_PATH}/include/MSVC)
endif()


if(ENABLE_RELATIVE_PREFIX)
	set(LINPHONE_PLUGINS_DIR "./lib/liblinphone/plugins")
else()
	set(LINPHONE_PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/lib/liblinphone/plugins")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/config.h PROPERTIES GENERATED ON)
add_definitions(-DHAVE_CONFIG_H)


add_subdirectory(coreapi)
add_subdirectory(share)
