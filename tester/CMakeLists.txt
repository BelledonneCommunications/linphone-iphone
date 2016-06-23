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

if(ENABLE_STATIC)
	set(LINPHONE_LIBS_FOR_TESTER linphone-static)
else()
	set(LINPHONE_LIBS_FOR_TESTER linphone)
endif()
set(OTHER_LIBS_FOR_TESTER ${BCTOOLBOX_LIBRARIES} ${BCTOOLBOX_TESTER_LIBRARIES} ${ORTP_LIBRARIES} ${MEDIASTREAMER2_LIBRARIES} ${BELLESIP_LIBRARIES} ${XML2_LIBRARIES})
if(INTL_FOUND)
	list(APPEND OTHER_LIBS_FOR_TESTER ${INTL_LIBRARIES})
endif()
if(SQLITE3_FOUND)
	list(APPEND OTHER_LIBS_FOR_TESTER ${SQLITE3_LIBRARIES})
endif()
if(ZLIB_FOUND)
	list(APPEND OTHER_LIBS_FOR_TESTER ${ZLIB_LIBRARIES})
endif()

set(RESOURCES_FILES
	certificates
	flexisip
	images
	local_tester_hosts
	messages.db
	rcfiles
	sipp
	sounds
	tester_hosts
	vcards
)

file(GLOB SOURCE_FILES_C "*_tester.c")
list(APPEND SOURCE_FILES_C accountmanager.c tester.c)

set(SOURCE_FILES_OBJC )
if(APPLE)
	if (IOS)
		list(APPEND SOURCE_FILES_OBJC liblinphone_tester_ios.m)
	endif()
endif()

apply_compile_flags(SOURCE_FILES_C "CPP" "C")
apply_compile_flags(SOURCE_FILES_OBJC "CPP" "OBJC")

if(MSVC)
	get_source_file_property(MESSAGE_TESTER_C_COMPILE_FLAGS message_tester.c COMPILE_FLAGS)
	set(MESSAGE_TESTER_C_COMPILE_FLAGS "${MESSAGE_TESTER_C_COMPILE_FLAGS} /wd4996") # Disable "was declared deprecated" warnings
	set_source_files_properties(message_tester.c PROPERTY COMPILE_FLAGS "${MESSAGE_TESTER_C_COMPILE_FLAGS}")
	get_source_file_property(VCARD_TESTER_C_COMPILE_FLAGS vcard_tester.c COMPILE_FLAGS)
	set(VCARD_TESTER_C_COMPILE_FLAGS "${VCARD_TESTER_C_COMPILE_FLAGS} /wd4996") # Disable "was declared deprecated" warnings
	set_source_files_properties(vcard_tester.c PROPERTY COMPILE_FLAGS "${VCARD_TESTER_C_COMPILE_FLAGS}")
endif()

if(NOT IOS OR NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	# Executable must be available on root path, not host one
	find_program(SIPP_PROGRAM NAMES sipp sipp.exe ONLY_CMAKE_FIND_ROOT_PATH)
	if(SIPP_PROGRAM)
		add_definitions(-DHAVE_SIPP=1)
		add_definitions(-DSIPP_COMMAND="${SIPP_PROGRAM}")
	endif()
endif()

# on mobile platforms, we compile the tester as a library so that we can link with it directly from native applications
if(ANDROID OR IOS)
	if(ANDROID)
		add_library(linphonetester SHARED ${SOURCE_FILES_C})
		set_target_properties(linphonetester PROPERTIES OUTPUT_NAME "linphonetester-${CMAKE_SYSTEM_PROCESSOR}")
	endif()
	if(IOS)
		add_library(linphonetester STATIC ${SOURCE_FILES_C})
	endif()
	target_include_directories(linphonetester PUBLIC ${BCTOOLBOX_TESTER_INCLUDE_DIRS})
	target_link_libraries(linphonetester linphone ${OTHER_LIBS_FOR_TESTER})
	install(TARGETS linphonetester
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
	install(FILES "liblinphone_tester.h"
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/linphone
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)
elseif(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	add_library(linphone_tester_static STATIC ${SOURCE_FILES_C})
	target_include_directories(linphone_tester_static PUBLIC ${BCTOOLBOX_TESTER_INCLUDE_DIRS})
	target_link_libraries(linphone_tester_static linphone ${OTHER_LIBS_FOR_TESTER})

	set(RUNTIME_COMPONENT_SOURCES
		liblinphone_tester_windows.cpp
		liblinphone_tester_windows.h
	)
	add_library(linphone_tester_runtime MODULE ${RUNTIME_COMPONENT_SOURCES})
	target_compile_options(linphone_tester_runtime PRIVATE "/wd4996")
	target_link_libraries(linphone_tester_runtime linphone_tester_static)
	set_target_properties(linphone_tester_runtime PROPERTIES VS_WINRT_COMPONENT TRUE)
	set_target_properties(linphone_tester_runtime PROPERTIES LIBRARY_OUTPUT_NAME "BelledonneCommunications.Linphone.Tester")
	set_target_properties(linphone_tester_runtime PROPERTIES LINK_FLAGS "/WINMDFILE:BelledonneCommunications.Linphone.Tester.winmd")

	install(TARGETS linphone_tester_runtime
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/BelledonneCommunications.Linphone.Tester.winmd" DESTINATION lib)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/BelledonneCommunications.Linphone.Tester.pdb
			DESTINATION ${CMAKE_INSTALL_LIBDIR}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
	endif()
endif()

# on iOS though, we also build the exectuable so that one can compile it from Xcode and run it directly
if (NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	if (IOS)
		set_source_files_properties(${RESOURCES_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		add_executable(liblinphone_tester MACOSX_BUNDLE ${RESOURCES_FILES} ${SOURCE_FILES_C} ${SOURCE_FILES_OBJC})
		set_target_properties(liblinphone_tester PROPERTIES LINK_FLAGS "-framework CoreFoundation -framework AudioToolbox -framework CoreAudio -framework Foundation -framework QuartzCore -framework OpenGLES -framework UIKit -framework AVFoundation -framework CoreGraphics -framework CoreMedia -framework CoreVideo -framework VideoToolbox")
	else()
		add_executable(liblinphone_tester ${SOURCE_FILES_C} ${SOURCE_FILES_OBJC})
	endif()
	set_target_properties(liblinphone_tester PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(liblinphone_tester PUBLIC ${BCTOOLBOX_TESTER_INCLUDE_DIRS})
	target_link_libraries(liblinphone_tester ${LINPHONE_LIBS_FOR_TESTER} ${OTHER_LIBS_FOR_TESTER})
	if (GTK2_FOUND)
		target_compile_definitions(liblinphone_tester PRIVATE HAVE_GTK)
		target_include_directories(liblinphone_tester PUBLIC ${GTK2_INCLUDE_DIRS})
		target_link_libraries(liblinphone_tester ${GTK2_LIBRARIES})
		if(GTKMACINTEGRATION_FOUND)
			target_include_directories(liblinphone_tester PUBLIC ${GTKMACINTEGRATION_INCLUDE_DIRS})
			target_link_libraries(liblinphone_tester ${GTKMACINTEGRATION_LIBRARIES})
		endif()
	endif()
endif()
