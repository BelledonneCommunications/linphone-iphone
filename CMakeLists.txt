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

cmake_minimum_required(VERSION 2.8.12)
project(LINPHONE C CXX)


set(LINPHONE_MAJOR_VERSION "3")
set(LINPHONE_MINOR_VERSION "7")
set(LINPHONE_MICRO_VERSION "0")
set(LINPHONE_VERSION "${LINPHONE_MAJOR_VERSION}.${LINPHONE_MINOR_VERSION}.${LINPHONE_MICRO_VERSION}")
set(LINPHONE_SO_VERSION "6")

set(LINPHONE_ALL_LANGS "cs de es fr he hu it ja nb_NO nl pl pt_BR ru sr sv zh_CN zh_TW")


include(CMakeDependentOption)

option(ENABLE_STATIC "Build static library (default is shared library)." NO)
option(ENABLE_CONSOLE_UI "Turn on or off compilation of console interface." YES)
option(ENABLE_DATE "Use build date in internal version number." NO)
option(ENABLE_GTK_UI "Turn on or off compilation of gtk interface." YES)
option(ENABLE_LDAP "Enable LDAP support." NO)
option(ENABLE_MSG_STORAGE "Turn on compilation of message storage." YES)
cmake_dependent_option(ENABLE_NOTIFY "Enable libnotify support." YES "ENABLE_GTK_UI" NO)
option(ENABLE_RELATIVE_PREFIX "Find resources relatively to the installation directory." NO)
option(ENABLE_TOOLS "Turn on or off compilation of tools." YES)
option(ENABLE_TUNNEL "Turn on compilation of tunnel support." NO)
option(ENABLE_TUTORIALS "Enable compilation of tutorials." YES)
option(ENABLE_UNIT_TESTS "Enable compilation of unit tests." YES)
option(ENABLE_UPNP "Build with uPnP support." YES)
option(ENABLE_VIDEO "Build with video support." YES)
cmake_dependent_option(ENABLE_ASSISTANT "Turn on assistant compiling." YES "ENABLE_GTK_UI" NO)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_PREFIX_PATH}/share/cmake/Modules")

include(CheckIncludeFile)
include(CheckSymbolExists)

if(MSVC)
	list(APPEND CMAKE_REQUIRED_INCLUDES "${CMAKE_PREFIX_PATH}/include/MSVC")
endif()

find_package(BelleSIP REQUIRED)
find_package(MS2 REQUIRED)
find_package(XML2 REQUIRED)
if(ENABLE_UNIT_TESTS)
	find_package(CUnit)
	if(CUNIT_FOUND)
		cmake_push_check_state(RESET)
		list(APPEND CMAKE_REQUIRED_INCLUDES ${CUNIT_INCLUDE_DIRS})
		list(APPEND CMAKE_REQUIRED_LIBRARIES ${CUNIT_LIBRARIES})
		check_symbol_exists("CU_get_suite" "CUnit/CUnit.h" HAVE_CU_GET_SUITE)
		check_symbol_exists("CU_curses_run_tests" "CUnit/CUnit.h" HAVE_CU_CURSES)
		cmake_pop_check_state()
	else()
		message(WARNING "Could not find the cunit library!")
		set(ENABLE_UNIT_TESTS OFF CACHE BOOL "Enable compilation of unit tests." FORCE)
	endif()
endif()
if(ENABLE_TUNNEL)
	find_package(Tunnel)
	if(NOT TUNNEL_FOUND)
		message(WARNING "Could not find the tunnel library!")
		set(ENABLE_TUNNEL OFF CACHE BOOL "Enable tunnel support." FORCE)
	endif()
endif()
if(ENABLE_MSG_STORAGE)
	find_package(Sqlite3)
	if(NOT SQLITE3_FOUND)
		message(FATAL_ERROR "Could not find the sqlite3 library!")
	endif()
endif()
if(ENABLE_NOTIFY)
	find_package(Notify)
	if(NOTIFY_FOUND)
		set(HAVE_NOTIFY4 1)
	else()
		message(WARNING "Could not find the notify library!")
		set(ENABLE_NOTIFY OFF CACHE BOOL "Enable libnotify support." FORCE)
	endif()
endif()
find_package(Gettext)


include_directories(
	include/
	coreapi/
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/coreapi/
	${BELLESIP_INCLUDE_DIRS}
	${MS2_INCLUDE_DIRS}
	${XML2_INCLUDE_DIRS}
)
if(SQLITE3_FOUND)
	include_directories(${SQLITE3_INCLUDE_DIRS})
	add_definitions("-DMSG_STORAGE_ENABLED")
endif()
if(ENABLE_TUNNEL)
	include_directories(${TUNNEL_INCLUDE_DIRS})
endif()

if(MSVC)
	include_directories(${CMAKE_PREFIX_PATH}/include/MSVC)
endif()

add_definitions("-DIN_LINPHONE")


if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wuninitialized -Wdeclaration-after-statement -fno-strict-aliasing -Werror")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wuninitialized -Werror")
	if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Qunused-arguments -Wno-array-bounds")
	endif()
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qunused-arguments -Wno-array-bounds")
	endif()
	if(APPLE)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unknown-warning-option -Wno-tautological-compare -Wno-unused-function")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unknown-warning-option -Wno-tautological-compare -Wno-unused-function")
	endif()
endif()


set(GETTEXT_PACKAGE "linphone")
if(ENABLE_RELATIVE_PREFIX)
	set(LINPHONE_DATA_DIR ".")
else()
	set(LINPHONE_DATA_DIR "${CMAKE_INSTALL_PREFIX}")
endif()
set(LINPHONE_PLUGINS_DIR "${LINPHONE_DATA_DIR}/lib/liblinphone/plugins")
set(PACKAGE_LOCALE_DIR "${LINPHONE_DATA_DIR}/share/locale")
set(PACKAGE_DATA_DIR "${LINPHONE_DATA_DIR}/share")
set(PACKAGE_SOUND_DIR "${LINPHONE_DATA_DIR}/share/sounds/linphone")
set(PACKAGE_RING_DIR "${PACKAGE_SOUND_DIR}/rings")
set(PACKAGE_FREEDESKTOP_DIR "${PACKAGE_DATA_DIR}/applications")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/config.h PROPERTIES GENERATED ON)
add_definitions(-DHAVE_CONFIG_H)


if(ENABLE_VIDEO)
	add_definitions(-DVIDEO_ENABLED)
endif()


add_subdirectory(coreapi)
add_subdirectory(share)
if(ENABLE_GTK_UI)
	add_subdirectory(gtk)
	add_subdirectory(pixmaps)
	add_subdirectory(po)
endif()
if(ENABLE_TOOLS)
	add_subdirectory(tools)
endif()
if(ENABLE_UNIT_TESTS)
	add_subdirectory(tester)
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLinphone.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/FindLinphone.cmake)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/FindLinphone.cmake
	DESTINATION share/cmake/Modules
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
