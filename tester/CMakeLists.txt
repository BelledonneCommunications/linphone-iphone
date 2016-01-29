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

set(SOURCE_FILES
	common/bc_tester_utils.c
	accountmanager.c
	call_tester.c
	complex_sip_call_tester.c
	dtmf_tester.c
	eventapi_tester.c
	flexisip_tester.c
	liblinphone_tester.c
	log_collection_tester.c
	message_tester.c
	multi_call_tester.c
	multicast_call_tester.c
	offeranswer_tester.c
	player_tester.c
	presence_tester.c
	proxy_config_tester.c
	quality_reporting_tester.c
	register_tester.c
	remote_provisioning_tester.c
	setup_tester.c
	stun_tester.c
	tester.c
	tunnel_tester.c
	upnp_tester.c
	video_tester.c
	vcard_tester.c
)

apply_compile_flags(SOURCE_FILES "CPP" "C")

if(NOT IOS OR NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	# Executable must be available on root path, not host one
	find_program(SIPP_PROGRAM NAMES sipp sipp.exe ONLY_CMAKE_FIND_ROOT_PATH)
	if(SIPP_PROGRAM)
		add_definitions(-DHAVE_SIPP=1)
		add_definitions(-DSIPP_COMMAND="${SIPP_PROGRAM}")
	endif()
endif()

add_definitions(-DBC_CONFIG_FILE="config.h")

if(IOS)
	add_library(linphonetester STATIC ${SOURCE_FILES})
	target_include_directories(linphonetester PUBLIC ${CUNIT_INCLUDE_DIRS} PRIVATE common)
	target_link_libraries(linphonetester linphone ${CUNIT_LIBRARIES})
	install(TARGETS linphonetester
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
	install(FILES "liblinphone_tester.h" "common/bc_tester_utils.h"
		DESTINATION include/linphone
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
		)
elseif(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	add_library(linphone_tester_static STATIC ${SOURCE_FILES})
	target_include_directories(linphone_tester_static PUBLIC ${CUNIT_INCLUDE_DIRS} PRIVATE common)
	target_link_libraries(linphone_tester_static linphone ${CUNIT_LIBRARIES})

	set(RUNTIME_COMPONENT_SOURCES
		liblinphone_tester_windows.cpp
		liblinphone_tester_windows.h
	)
	add_library(linphone_tester_runtime MODULE ${RUNTIME_COMPONENT_SOURCES})
	target_include_directories(linphone_tester_runtime PRIVATE common)
	target_compile_options(linphone_tester_runtime PRIVATE "/wd4996")
	target_link_libraries(linphone_tester_runtime linphone_tester_static)
	set_target_properties(linphone_tester_runtime PROPERTIES VS_WINRT_COMPONENT TRUE)
	set_target_properties(linphone_tester_runtime PROPERTIES LIBRARY_OUTPUT_NAME "BelledonneCommunications.Linphone.Tester")
	set_target_properties(linphone_tester_runtime PROPERTIES LINK_FLAGS "/WINMDFILE:BelledonneCommunications.Linphone.Tester.winmd")

	install(TARGETS linphone_tester_runtime
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/BelledonneCommunications.Linphone.Tester.winmd" DESTINATION lib)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/BelledonneCommunications.Linphone.Tester.pdb
			DESTINATION lib
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
	endif()
else()
	add_executable(liblinphone_tester ${SOURCE_FILES})
	set_target_properties(liblinphone_tester PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(liblinphone_tester PUBLIC ${CUNIT_INCLUDE_DIRS} PRIVATE common)
	target_link_libraries(liblinphone_tester linphone ${CUNIT_LIBRARIES})
	if (GTK2_FOUND)
		target_compile_definitions(liblinphone_tester PRIVATE HAVE_GTK)
		target_include_directories(liblinphone_tester PUBLIC ${GTK2_INCLUDE_DIRS})
		target_link_libraries(liblinphone_tester linphone ${GTK2_LIBRARIES})
		if(GTKMACINTEGRATION_FOUND)
			target_include_directories(liblinphone_tester PUBLIC ${GTKMACINTEGRATION_INCLUDE_DIRS})
			target_link_libraries(liblinphone_tester ${GTKMACINTEGRATION_LIBRARIES})
		endif()
	endif()
endif()
