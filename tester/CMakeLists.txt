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

set(OTHER_LIBS_FOR_TESTER ${BCTOOLBOX_LIBRARIES} ${ORTP_LIBRARIES} ${MEDIASTREAMER2_LIBRARIES} ${BELLESIP_LIBRARIES} ${XML2_LIBRARIES})
if(INTL_FOUND)
	list(APPEND OTHER_LIBS_FOR_TESTER ${INTL_LIBRARIES})
endif()
if(SQLITE3_FOUND)
	list(APPEND OTHER_LIBS_FOR_TESTER ${SQLITE3_LIBRARIES})
endif()
if(ZLIB_FOUND)
	list(APPEND OTHER_LIBS_FOR_TESTER ${ZLIB_LIBRARIES})
endif()

set(SOUND_FILES
	sounds/ahbahouaismaisbon.wav
	sounds/hello44100.wav
	sounds/hello8000.mkv
	sounds/hello8000_mkv_ref.wav
	sounds/hello8000.wav
	sounds/oldphone.wav
	sounds/ringback.wav
	sounds/sintel_trailer_opus_h264.mkv
	sounds/sintel_trailer_opus_vp8.mkv
	sounds/sintel_trailer_pcmu_h264.mkv
	sounds/vrroom.wav
)

set(SIPP_FILES
	sipp/call_invite_200ok_without_contact_header.xml
	sipp/call_with_audio_mline_before_video_in_sdp.xml
	sipp/call_with_multiple_audio_mline_in_sdp.xml
	sipp/call_with_multiple_video_mline_in_sdp.xml
	sipp/call_with_video_mline_before_audio_in_sdp.xml
	sipp/sip_update_within_icoming_reinvite_with_no_sdp.xml
)

set(CERTIFICATE_ALT_FILES
	certificates/altname/agent.pem
	certificates/altname/cafile.pem
	certificates/altname/openssl-altname.cnf
)

set(CERTIFICATE_CN_FILES
	certificates/cn/agent.pem
	certificates/cn/cafile.pem
	certificates/cn/openssl-cn.cnf
)

set(CERTIFICATE_CLIENT_FILES
	certificates/client/cert.pem
	certificates/client/key.pem
	certificates/client/cert2.pem
	certificates/client/key2.pem
	certificates/client/cert2-signed-by-other-ca.pem
)

set(CERTIFICATE_FILES ${CERTIFICATE_ALT_FILES} ${CERTIFICATE_CN_FILES} ${CERTIFICATE_CLIENT_FILES})

set(RC_FILES
	rcfiles/carddav_rc
	rcfiles/conference_focus_rc
	rcfiles/empty_rc
	rcfiles/friends_rc
	rcfiles/laure_call_logs_rc
	rcfiles/laure_rc_udp
	rcfiles/laure_tcp_rc
	rcfiles/marie_early_rc
	rcfiles/marie_h264_rc
	rcfiles/marie_nat64_rc
	rcfiles/marie_quality_reporting_rc
	rcfiles/marie_rc
	rcfiles/marie2_rc
	rcfiles/marie_rc_rtcp_xr
	rcfiles/marie_remote_404_rc
	rcfiles/marie_remote_default_values_rc
	rcfiles/marie_remote_https_rc
	rcfiles/marie_remote_invalid_rc
	rcfiles/marie_remote_invalid_uri_rc
	rcfiles/marie_remote_localfile2_rc
	rcfiles/marie_remote_localfile_android_rc
	rcfiles/marie_remote_localfile_rc
	rcfiles/marie_remote_localfile_win10_rc
	rcfiles/marie_remote_rc
	rcfiles/marie_sips_rc
	rcfiles/marie_transient_remote_rc
	rcfiles/marie_v4proxy_rc
	rcfiles/marie_zrtp_aes256_rc
	rcfiles/marie_zrtp_b256_rc
	rcfiles/marie_zrtp_srtpsuite_aes256_rc
	rcfiles/michelle_rc_udp
	rcfiles/multi_account_rc
	rcfiles/pauline_alt_rc
	rcfiles/pauline_as_friend_rc
	rcfiles/pauline_h264_rc
	rcfiles/pauline_nat64_rc
	rcfiles/pauline_rc
	rcfiles/pauline_rc_rtcp_xr
	rcfiles/pauline_sips_rc
	rcfiles/pauline_tcp_rc
	rcfiles/pauline_tls_client_rc
	rcfiles/pauline_tunnel_verify_server_certificate_rc
	rcfiles/pauline_v4proxy_rc
	rcfiles/pauline_wild_rc
	rcfiles/pauline_zrtp_aes256_rc
	rcfiles/pauline_zrtp_b256_rc
	rcfiles/pauline_zrtp_srtpsuite_aes256_rc
	rcfiles/remote_zero_length_params_rc
	rcfiles/stun_rc
	rcfiles/upnp_rc
	rcfiles/zero_length_params_rc
)

set(IMAGE_FILES 
	images/nowebcamCIF.jpg
	images/nowebcamVGA.jpg)

set(VCARD_FILES
	vcards/thousand_vcards.vcf
	vcards/vcards.vcf
)

set(OTHER_FILES
	tester_hosts
	local_tester_hosts
	messages.db
)

set(IOS_RESOURCES_FILES
	${OTHER_FILES}
	certificates
	images
	rcfiles
	sipp
	sounds
	vcards
)

set(SOURCE_FILES_C
	accountmanager.c
	audio_bypass_tester.c
	call_multicast_tester.c
	call_multi_tester.c
	call_single_tester.c
	call_video_tester.c
	complex_sip_case_tester.c
	dtmf_tester.c
	eventapi_tester.c
	flexisip_tester.c
	liblinphone_tester.c
	liblinphone_tester.h
	log_collection_tester.c
	message_tester.c
	offeranswer_tester.c
	player_tester.c
	presence_server_tester.c
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
	vcard_tester.c
	video_tester.c
)

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
		set_target_properties(linphonetester PROPERTIES OUTPUT_NAME "linphonetester-${NDK_ARCHITECTURE}")
	endif()
	if(IOS)
		add_library(linphonetester STATIC ${SOURCE_FILES_C})
	endif()
	target_include_directories(linphonetester PUBLIC ${BCTOOLBOX_TESTER_INCLUDE_DIRS})
	target_link_libraries(linphonetester ${LINPHONE_LIBS_FOR_TOOLS} ${OTHER_LIBS_FOR_TESTER})
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
	target_link_libraries(linphone_tester_static ${LINPHONE_LIBS_FOR_TOOLS} ${OTHER_LIBS_FOR_TESTER})

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
	if(IOS)
		set_source_files_properties(${IOS_RESOURCES_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		add_executable(liblinphone_tester MACOSX_BUNDLE ${IOS_RESOURCES_FILES} ${SOURCE_FILES_C} ${SOURCE_FILES_OBJC})
	else()
		add_executable(liblinphone_tester ${SOURCE_FILES_C} ${SOURCE_FILES_OBJC})
	endif()
	set_target_properties(liblinphone_tester PROPERTIES LINK_FLAGS "${LINPHONE_LDFLAGS}")
	set_target_properties(liblinphone_tester PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(liblinphone_tester PUBLIC ${BCTOOLBOX_TESTER_INCLUDE_DIRS})
	target_link_libraries(liblinphone_tester ${LINPHONE_LIBS_FOR_TOOLS} ${OTHER_LIBS_FOR_TESTER})
	if (GTK2_FOUND)
		target_compile_definitions(liblinphone_tester PRIVATE HAVE_GTK)
		target_include_directories(liblinphone_tester PUBLIC ${GTK2_INCLUDE_DIRS})
		target_link_libraries(liblinphone_tester ${GTK2_LIBRARIES})
		if(GTKMACINTEGRATION_FOUND)
			target_include_directories(liblinphone_tester PUBLIC ${GTKMACINTEGRATION_INCLUDE_DIRS})
			target_link_libraries(liblinphone_tester ${GTKMACINTEGRATION_LIBRARIES})
		endif()
	endif()

	if(NOT IOS)
		install(TARGETS liblinphone_tester
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		)
		install(FILES ${OTHER_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester")
		install(FILES ${SOUND_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/sounds")
		install(FILES ${SIPP_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/sipp")
		install(FILES ${CERTIFICATE_ALT_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/certificates/altname")
		install(FILES ${CERTIFICATE_CN_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/certificates/cn")
		install(FILES ${CERTIFICATE_CLIENT_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/certificates/client")
		install(FILES ${RC_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/rcfiles")
		install(FILES ${IMAGE_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/images")
		install(FILES ${VCARD_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/liblinphone_tester/vcards")
	endif()
endif()
