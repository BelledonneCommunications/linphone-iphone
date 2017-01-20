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

if(MSVC AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone" AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	find_library(LIBGCC NAMES gcc)
	find_library(LIBMINGWEX NAMES mingwex)
endif()

if(NOT WIN32)
	find_package(Iconv QUIET)
endif()


set(LINPHONE_PRIVATE_HEADER_FILES
	bellesip_sal/sal_impl.h
	carddav.h
	conference_private.h
	contact_providers_priv.h
	enum.h
	lime.h
	lpc2xml.h
	offeranswer.h
	private.h
	quality_reporting.h
	sqlite3_bctbx_vfs.h
	vcard_private.h
	xml2lpc.h
)

set(LINPHONE_SOURCE_FILES_C
	account_creator.c
	address.c
	authentication.c
	bellesip_sal/sal_address_impl.c
	bellesip_sal/sal_impl.c
	bellesip_sal/sal_op_call.c
	bellesip_sal/sal_op_call_transfer.c
	bellesip_sal/sal_op_events.c
	bellesip_sal/sal_op_impl.c
	bellesip_sal/sal_op_info.c
	bellesip_sal/sal_op_message.c
	bellesip_sal/sal_op_presence.c
	bellesip_sal/sal_op_publish.c
	bellesip_sal/sal_op_registration.c
	bellesip_sal/sal_sdp.c
	buffer.c
	callbacks.c
	call_log.c
	call_params.c
	carddav.c
	chat.c
	chat_file_transfer.c
	contactprovider.c
	content.c
	dial_plan.c
	dict.c
	ec-calibrator.c
	echo-tester.c
	enum.c
	error_info.c
	event.c
	factory.c
	friend.c
	friendlist.c
	im_notif_policy.c
	info.c
	ldapprovider.c
	lime.c
	im_encryption_engine.c
	linphonecall.c
	linphonecore.c
	linphone_tunnel_config.c
	localplayer.c
	lpc2xml.c
	lpconfig.c
	lsd.c
	message_storage.c
	misc.c
	nat_policy.c
	offeranswer.c
	player.c
	presence.c
	proxy.c
	quality_reporting.c
	remote_provisioning.c
	ringtoneplayer.c
	sal.c
	siplogin.c
	sipsetup.c
	sqlite3_bctbx_vfs.c
	xml2lpc.c
	xml.c
	xmlrpc.c
	vtables.c
)
set(LINPHONE_SOURCE_FILES_CXX conference.cc)
if(ANDROID)
	list(APPEND LINPHONE_SOURCE_FILES_CXX linphonecore_jni.cc)
	set_source_files_properties(linphonecore_jni.cc PROPERTIES COMPILE_DEFINITIONS "USE_JAVAH")
endif()

set(LINPHONE_SOURCE_FILES_OBJC)
if (IOS)
	list(APPEND LINPHONE_SOURCE_FILES_OBJC ringtoneplayer_ios.m ringtoneplayer_ios.h)
endif()


if(ENABLE_TUNNEL)
	list(APPEND LINPHONE_SOURCE_FILES_CXX
		linphone_tunnel.cc
		TunnelManager.cc
	)
	add_definitions(-DTUNNEL_ENABLED)
else()
	list(APPEND LINPHONE_SOURCE_FILES_C linphone_tunnel_stubs.c)
endif()

find_package(Git)
add_custom_target(liblinphone-git-version
	COMMAND ${CMAKE_COMMAND} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DLINPHONE_VERSION=${LINPHONE_VERSION} -DWORK_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/gitversion.cmake
	BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/liblinphone_gitversion.h"
)

add_definitions(
	-DUSE_BELLESIP
	-DLIBLINPHONE_EXPORTS
)

set(LIBS
	${BCTOOLBOX_CORE_LIBRARIES}
	${BELLESIP_LIBRARIES}
	${MEDIASTREAMER2_LIBRARIES}
	${XML2_LIBRARIES}
)
if(ZLIB_FOUND)
	list(APPEND LIBS ${ZLIB_LIBRARIES})
endif()
if(SQLITE3_FOUND)
	list(APPEND LIBS ${SQLITE3_LIBRARIES})
endif()
if(ICONV_FOUND)
	list(APPEND LIBS ${ICONV_LIBRARIES})
endif()
if(ENABLE_TUNNEL)
	list(APPEND LIBS ${TUNNEL_LIBRARIES})
endif()
if(MSVC AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone" AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	list(APPEND LIBS ${LIBGCC} ${LIBMINGWEX})
endif()
if(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone" AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	list(APPEND LIBS shlwapi)
endif()
if(INTL_FOUND)
	list(APPEND LIBS ${INTL_LIBRARIES})
endif()
if(BELCARD_FOUND)
	list(APPEND LIBS ${BELCARD_LIBRARIES})
	list(APPEND LINPHONE_SOURCE_FILES_CXX vcard.cc)
	if(NOT MSVC)
		list(APPEND STRICT_OPTIONS_CXX "-std=c++11")
	endif()
	if(APPLE)
		list(APPEND STRICT_OPTIONS_CXX "-stdlib=libc++")
	endif()
else()
	list(APPEND LINPHONE_SOURCE_FILES_C vcard_stubs.c)
endif()

apply_compile_flags(LINPHONE_SOURCE_FILES_C "CPP" "C")
apply_compile_flags(LINPHONE_SOURCE_FILES_CXX "CPP" "CXX")
apply_compile_flags(LINPHONE_SOURCE_FILES_OBJC "CPP" "OBJC")

if(ENABLE_STATIC)
	add_library(linphone-static STATIC ${LINPHONE_HEADER_FILES} ${LINPHONE_PRIVATE_HEADER_FILES} ${LINPHONE_SOURCE_FILES_C} ${LINPHONE_SOURCE_FILES_CXX} ${LINPHONE_SOURCE_FILES_OBJC})
	set_target_properties(linphone-static PROPERTIES OUTPUT_NAME linphone)
	add_dependencies(linphone-static liblinphone-git-version)
	target_link_libraries(linphone-static ${LIBS})
	if(ANDROID)
		add_dependencies(linphone-static linphonecore-jni-header)
	endif()
	if(IOS)
		target_link_libraries(linphone-static "-framework Foundation" "-framework AVFoundation")
	endif()
	install(TARGETS linphone-static EXPORT ${EXPORT_TARGETS_NAME}Targets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif()
if(ENABLE_SHARED)
	add_library(linphone SHARED ${LINPHONE_HEADER_FILES} ${LINPHONE_PRIVATE_HEADER_FILES} ${LINPHONE_SOURCE_FILES_C} ${LINPHONE_SOURCE_FILES_CXX} ${LINPHONE_SOURCE_FILES_OBJC})

	if(BELCARD_FOUND)
		if(APPLE)
	        set_target_properties(linphone PROPERTIES LINK_FLAGS "-stdlib=libc++")
    	endif()
	endif()

	set_target_properties(linphone PROPERTIES LINKER_LANGUAGE CXX)
	if(NOT ANDROID)
		# Do not version shared library on Android
		set_target_properties(linphone PROPERTIES SOVERSION ${LINPHONE_SO_VERSION})
	endif()
	add_dependencies(linphone liblinphone-git-version)
	target_link_libraries(linphone PRIVATE ${LIBS})
	if(WIN32 AND CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone" AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
		set_target_properties(linphone PROPERTIES PREFIX "lib")
	elseif(ANDROID)
		set_target_properties(linphone PROPERTIES OUTPUT_NAME "linphone-${NDK_ARCHITECTURE}")
		add_dependencies(linphone linphonecore-jni-header)
	endif()
	if(MSVC)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
			install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/linphone.pdb
				DESTINATION ${CMAKE_INSTALL_BINDIR}
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()
	endif()
	install(TARGETS linphone EXPORT ${EXPORT_TARGETS_NAME}Targets
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
	)
endif()
if(ICONV_FOUND)
	if(APPLE)
		# Prevent conflict between the system iconv.h header and the one from macports.
		if(ENABLE_STATIC)
			target_compile_options(linphone-static PRIVATE "-include" "${ICONV_INCLUDE_DIRS}/iconv.h")
		endif()
		if(ENABLE_SHARED)
			target_compile_options(linphone PRIVATE "-include" "${ICONV_INCLUDE_DIRS}/iconv.h")
		endif()
	else()
		if(ENABLE_STATIC)
			target_include_directories(linphone-static PRIVATE ${ICONV_INCLUDE_DIRS})
		endif()
		if(ENABLE_SHARED)
			target_include_directories(linphone PRIVATE ${ICONV_INCLUDE_DIRS})
		endif()
	endif()
endif()

add_subdirectory(help)
