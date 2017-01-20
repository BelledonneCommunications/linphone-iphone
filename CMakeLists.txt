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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

cmake_minimum_required(VERSION 3.0)
project(linphone VERSION 3.10.2 LANGUAGES C CXX)


set(LINPHONE_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set(LINPHONE_MINOR_VERSION ${PROJECT_VERSION_MINOR})
set(LINPHONE_MICRO_VERSION ${PROJECT_VERSION_PATCH})
set(LINPHONE_VERSION ${PROJECT_VERSION})
set(LINPHONE_SO_VERSION "9")

file(GLOB LINPHONE_PO_FILES RELATIVE "${CMAKE_CURRENT_LIST_DIR}/po" "${CMAKE_CURRENT_LIST_DIR}/po/*.po")
string(REGEX REPLACE "([a-zA-Z_]+)\\.po" "\\1" LINPHONE_ALL_LANGS_LIST "${LINPHONE_PO_FILES}")
string(REPLACE ";" " " LINPHONE_ALL_LANGS "${LINPHONE_ALL_LANGS_LIST}")

include(CMakeDependentOption)

option(ENABLE_SHARED "Build shared library." YES)
option(ENABLE_STATIC "Build static library." YES)
option(ENABLE_CONSOLE_UI "Turn on or off compilation of console interface." YES)
option(ENABLE_DATE "Use build date in internal version number." NO)
option(ENABLE_DAEMON "Enable the linphone daemon interface." YES)
option(ENABLE_DOC "Enable documentation generation with Doxygen." YES)
option(ENABLE_JAVADOC "Add a target to generate documentation for Java API" NO)
option(ENABLE_GTK_UI "Turn on or off compilation of gtk interface." YES)
option(ENABLE_LDAP "Enable LDAP support." NO)
option(ENABLE_LIME "Enable Instant Messaging Encryption." YES)
option(ENABLE_SQLITE_STORAGE "Turn on compilation sqlite storage, for messages, contacts, history" YES)
cmake_dependent_option(ENABLE_NOTIFY "Enable libnotify support." YES "ENABLE_GTK_UI;NOT APPLE" NO)
option(ENABLE_RELATIVE_PREFIX "Find resources relatively to the installation directory." NO)
option(ENABLE_STRICT "Build with strict compile options." YES)
option(ENABLE_TOOLS "Turn on or off compilation of tools." YES)
option(ENABLE_TUNNEL "Turn on compilation of tunnel support." NO)
option(ENABLE_TUTORIALS "Enable compilation of tutorials." YES)
option(ENABLE_UNIT_TESTS "Enable compilation of unit tests." YES)
option(ENABLE_UPNP "Build with uPnP support." YES)
option(ENABLE_VIDEO "Build with video support." YES)
cmake_dependent_option(ENABLE_ASSISTANT "Turn on assistant compiling." YES "ENABLE_GTK_UI" NO)
option(ENABLE_DEBUG_LOGS "Turn on or off debug level logs." NO)
option(ENABLE_NLS "Build with internationalisation support" YES)
option(ENABLE_VCARD "Turn on compilation of vcard4 support." YES)
option(CXX_WRAPPER "Build the C++ wrapper for Liblinphone." OFF)


macro(apply_compile_flags SOURCE_FILES)
	if(${SOURCE_FILES})
		set(options "")
		foreach(a ${ARGV})
			if(STRICT_OPTIONS_${a})
				string(REPLACE ";" " " options_${a} "${STRICT_OPTIONS_${a}}")
				set(options "${options} ${options_${a}}")
			endif()
		endforeach()
		if(options)
			set_source_files_properties(${${SOURCE_FILES}} PROPERTIES COMPILE_FLAGS "${options}")
		endif()
	endif()
endmacro()

if(ENABLE_STATIC)
	set(LINPHONE_LIBS_FOR_TOOLS linphone-static)
else()
	set(LINPHONE_LIBS_FOR_TOOLS linphone)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CheckSymbolExists)
include(CMakePushCheckState)
include(GNUInstallDirs)

if(NOT CMAKE_INSTALL_RPATH AND CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
	message(STATUS "Setting install rpath to ${CMAKE_INSTALL_RPATH}")
endif()

set(MSVC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/MSVC")
if(MSVC)
	list(APPEND CMAKE_REQUIRED_INCLUDES "${MSVC_INCLUDE_DIR}")
endif()

# find_package should be invoked here to check for libraries - however do NOT
# call include_directories here (see below)

if (ENABLE_VCARD)
	if(LINPHONE_BUILDER_GROUP_EXTERNAL_SOURCE_PATH_BUILDERS)
		include("${EP_belcard_CONFIG_DIR}/BelcardConfig.cmake")
	else()
		find_package(Belcard)
	endif()
	if(NOT BELCARD_FOUND)
		message(WARNING "Could not find the belcard library!")
		set(ENABLE_VCARD OFF CACHE BOOL "Enable vcard support." FORCE)
	else()
		add_definitions(-DVCARD_ENABLED)
	endif()
endif()

if(LINPHONE_BUILDER_GROUP_EXTERNAL_SOURCE_PATH_BUILDERS)
	include("${EP_bellesip_CONFIG_DIR}/BelleSIPConfig.cmake")
	include("${EP_ms2_CONFIG_DIR}/Mediastreamer2Config.cmake")
	set(BcToolbox_FIND_COMPONENTS tester)
	include("${EP_bctoolbox_CONFIG_DIR}/BcToolboxConfig.cmake")
else()
	find_package(BelleSIP REQUIRED)
	find_package(Mediastreamer2 REQUIRED)
	find_package(BcToolbox 0.0.3 REQUIRED OPTIONAL_COMPONENTS tester)
endif()
find_package(XML2 REQUIRED)
find_package(Zlib)
if(ENABLE_TUNNEL)
	if(LINPHONE_BUILDER_GROUP_EXTERNAL_SOURCE_PATH_BUILDERS)
		include("${EP_tunnel_CONFIG_DIR}/TunnelConfig.cmake")
	else()
		find_package(Tunnel)
	endif()
	if(NOT TUNNEL_FOUND)
		message(WARNING "Could not find the tunnel library!")
		set(ENABLE_TUNNEL OFF CACHE BOOL "Enable tunnel support." FORCE)
	endif()
endif()
if(ENABLE_SQLITE_STORAGE)
	find_package(Sqlite3 REQUIRED)
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
if(ENABLE_GTK_UI)
	if(WIN32)
		set(GTK2_ADDITIONAL_SUFFIXES "../lib/glib-2.0/include" "../lib/gtk-2.0/include")
	endif()
	find_package(GTK2 2.18 REQUIRED gtk)
	if(ENABLE_ASSISTANT AND GTK2_VERSION VERSION_LESS 2.22)
		message(WARNING "You need at least GTK 2.22 to enable the assistant")
		set(ENABLE_ASSISTANT OFF CACHE BOOL "Turn on assistant compiling." FORCE)
	endif()
	if(APPLE)
		find_package(GtkMacIntegration)
		if(GTKMACINTEGRATION_FOUND)
			set(HAVE_GTK_OSX 1)
			add_definitions("${GTKMACINTEGRATION_CPPFLAGS}")
		else()
			message(WARNING "gtk-mac-integration not found. Please install gtk-osx-application package.")
		endif()
	endif()
endif()
if(ENABLE_ASSISTANT)
	set(BUILD_WIZARD 1)
endif()
if(ENABLE_NLS)
	find_package(Gettext REQUIRED)
	find_package(Intl REQUIRED)
endif()
if(ENABLE_LIME)
	set(HAVE_LIME 1)
endif()
if (CXX_WRAPPER)
	set(Python_ADDITIONAL_VERSIONS 2)
	find_package(PythonInterp REQUIRED)
endif()

if(UNIX AND NOT APPLE)
	include(CheckIncludeFiles)
	check_include_files(libudev.h HAVE_LIBUDEV_H)
endif()

set(LINPHONE_LDFLAGS "${BELLESIP_LDFLAGS} ${MEDIASTREAMER2_LDFLAGS}")
if(BELCARD_FOUND AND APPLE)
    set(LINPHONE_LDFLAGS "${LINPHONE_LDFLAGS} -stdlib=libc++")
endif()

# include_directories must be called only UNDER THIS LINE in order to use our
# projects submodules first (we do NOT want to have system headers in first position)
include_directories(
	include/
	coreapi/
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_BINARY_DIR}/coreapi/
	${BELLESIP_INCLUDE_DIRS}
	${MEDIASTREAMER2_INCLUDE_DIRS}
	${BCTOOLBOX_CORE_INCLUDE_DIRS}
)
if(ANDROID)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/java)
endif()
if(ENABLE_TUNNEL)
	include_directories(${TUNNEL_INCLUDE_DIRS})
endif()
if (ENABLE_VCARD)
	include_directories(${BELCARD_INCLUDE_DIRS})
endif()

include_directories(${XML2_INCLUDE_DIRS})

if(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIRS})
	set(HAVE_ZLIB 1)
endif()
if(SQLITE3_FOUND)
	include_directories(${SQLITE3_INCLUDE_DIRS})
	if(ENABLE_SQLITE_STORAGE)
		add_definitions("-DSQLITE_STORAGE_ENABLED")
	endif()
endif()
if(INTL_FOUND)
	set(HAVE_INTL 1)
	include_directories(${INTL_INCLUDE_DIRS})
endif()
if(MSVC)
	include_directories(${MSVC_INCLUDE_DIR})
endif()

if(ENABLE_DEBUG_LOGS)
	add_definitions("-DDEBUG")
endif()
if(ANDROID)
	add_definitions("-DANDROID")
endif()

set(STRICT_OPTIONS_CPP )
set(STRICT_OPTIONS_C )
set(STRICT_OPTIONS_CXX )
set(STRICT_OPTIONS_OBJC )
if(MSVC)
	list(APPEND STRICT_OPTIONS_CPP "/wd4995") # Disable "name was marked as #pragma deprecated" warnings
	list(APPEND STRICT_OPTIONS_CPP "/wd4996") # Disable deprecated function warnings
	if(ENABLE_STRICT)
		list(APPEND STRICT_OPTIONS_CPP "/WX")
	endif()
else()
	list(APPEND STRICT_OPTIONS_CPP "-Wall" "-Wuninitialized" "-Wno-error=deprecated-declarations")
	list(APPEND STRICT_OPTIONS_C "-Wstrict-prototypes" "-Werror=strict-prototypes")
	if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
		list(APPEND STRICT_OPTIONS_C "-fno-inline-small-functions")
	endif()
	if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		list(APPEND STRICT_OPTIONS_CPP "-Qunused-arguments" "-Wno-array-bounds")
	endif()
	if(APPLE)
		list(APPEND STRICT_OPTIONS_CPP "-Wno-error=unknown-warning-option" "-Qunused-arguments" "-Wno-tautological-compare" "-Wno-unused-function" "-Wno-array-bounds")
	endif()
	if(ENABLE_STRICT)
		list(APPEND STRICT_OPTIONS_C "-Werror" "-Wextra" "-Wno-unused-parameter" "-Wno-missing-field-initializers" "-fno-strict-aliasing")
		list(APPEND STRICT_OPTIONS_CPP "-Werror" "-Wextra" "-Wno-unused-parameter" "-Wno-missing-field-initializers" "-fno-strict-aliasing")
	endif()
endif()
if(STRICT_OPTIONS_CPP)
	list(REMOVE_DUPLICATES STRICT_OPTIONS_CPP)
endif()
if(STRICT_OPTIONS_C)
	list(REMOVE_DUPLICATES STRICT_OPTIONS_C)
endif()


set(GETTEXT_PACKAGE "linphone")
if(ENABLE_RELATIVE_PREFIX)
	set(LINPHONE_DATA_DIR ".")
else()
	set(LINPHONE_DATA_DIR "${CMAKE_INSTALL_PREFIX}")
endif()
set(LINPHONE_PLUGINS_DIR "${LINPHONE_DATA_DIR}/${CMAKE_INSTALL_LIBDIR}/liblinphone/plugins")
if(WIN32)
	set(LINPHONE_CONFIG_DIR "Linphone")
endif()
set(PACKAGE_LOCALE_DIR "${LINPHONE_DATA_DIR}/${CMAKE_INSTALL_DATADIR}/locale")
set(PACKAGE_DATA_DIR "${LINPHONE_DATA_DIR}/${CMAKE_INSTALL_DATADIR}")
set(PACKAGE_SOUND_DIR "${LINPHONE_DATA_DIR}/${CMAKE_INSTALL_DATADIR}/sounds/linphone")
set(PACKAGE_RING_DIR "${PACKAGE_SOUND_DIR}/rings")
set(PACKAGE_FREEDESKTOP_DIR "${PACKAGE_DATA_DIR}/applications")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/config.h PROPERTIES GENERATED ON)
add_definitions(-DHAVE_CONFIG_H)


if(ENABLE_VIDEO)
	add_definitions(-DVIDEO_ENABLED)
endif()


if(LINPHONE_BUILDER_GROUP_EXTERNAL_SOURCE_PATH_BUILDERS)
	set(EXPORT_TARGETS_NAME "LinphoneBuilder")
else()
	set(EXPORT_TARGETS_NAME "Linphone")
endif()


add_subdirectory(java)
add_subdirectory(coreapi)
add_subdirectory(include)
add_subdirectory(share)
if(ENABLE_CONSOLE_UI)
	add_subdirectory(console)
endif()
if(ENABLE_DAEMON)
	add_subdirectory(daemon)
endif()
if(ENABLE_GTK_UI)
	add_subdirectory(gtk)
	add_subdirectory(pixmaps)
	add_subdirectory(po)
endif()
if(ENABLE_TOOLS)
	add_subdirectory(tools)
endif()
if(ENABLE_UNIT_TESTS AND BCTOOLBOX_TESTER_FOUND)
	add_subdirectory(tester)
endif()
if(CXX_WRAPPER)
	add_subdirectory(wrappers/cpp)
endif()


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/LinphoneConfigVersion.cmake"
	VERSION ${LINPHONE_VERSION}
	COMPATIBILITY AnyNewerVersion
)
export(EXPORT ${EXPORT_TARGETS_NAME}Targets
	FILE "${CMAKE_CURRENT_BINARY_DIR}/LinphoneTargets.cmake"
)
configure_file(cmake/LinphoneConfig.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/LinphoneConfig.cmake"
	@ONLY
)

set(CONFIG_PACKAGE_LOCATION "${CMAKE_INSTALL_DATADIR}/Linphone/cmake")
install(EXPORT ${EXPORT_TARGETS_NAME}Targets
	FILE LinphoneTargets.cmake
	DESTINATION ${CONFIG_PACKAGE_LOCATION}
)
install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/LinphoneConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/LinphoneConfigVersion.cmake"
	DESTINATION ${CONFIG_PACKAGE_LOCATION}
)
