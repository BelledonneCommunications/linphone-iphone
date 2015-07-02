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

if(MSVC AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsPhone")
	find_library(LIBGCC NAMES gcc)
	find_library(LIBMINGWEX NAMES mingwex)
endif()

if(NOT WIN32)
	find_package(Iconv)
endif()


set(SOURCE_FILES
	account_creator.c
	account_creator.h
	address.c
	authentication.c
	bellesip_sal/sal_address_impl.c
	bellesip_sal/sal_impl.c
	bellesip_sal/sal_impl.h
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
	chat.c
	conference.c
	contactprovider.c
	content.c
	dict.c
	ec-calibrator.c
	enum.c
	enum.h
	event.c
	event.h
	friend.c
	info.c
	ldap/ldapprovider.c
	lime.c
	linphonecall.c
	linphonecore.c
	linphonecore.h
	linphonecore_utils.h
	linphonefriend.h
	linphone_tunnel_config.c
	linphone_tunnel.h
	localplayer.c
	lpconfig.c
	lpconfig.h
	lsd.c
	message_storage.c
	misc.c
	offeranswer.c
	offeranswer.h
	player.c
	presence.c
	private.h
	proxy.c
	quality_reporting.c
	remote_provisioning.c
	sal.c
	siplogin.c
	sipsetup.c
	sipsetup.h
	xml2lpc.c
	xml2lpc.h
	xml.c
	xmlrpc.c
	xmlrpc.h
	vtables.c
)
if(ENABLE_TUNNEL)
	list(APPEND SOURCE_FILES
		linphone_tunnel.cc
		TunnelManager.cc
	)
	add_definitions(-DTUNNEL_ENABLED)
else()
	list(APPEND SOURCE_FILES linphone_tunnel_stubs.c)
endif()

set(GENERATED_SOURCE_FILES
	${CMAKE_CURRENT_BINARY_DIR}/liblinphone_gitversion.h
)
set_source_files_properties(${GENERATED_SOURCE_FILES} PROPERTIES GENERATED TRUE)
find_package(Git)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/liblinphone_gitversion.h
	COMMAND ${CMAKE_COMMAND} -DGIT_EXECUTABLE=${GIT_EXECUTABLE} -DWORK_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/gitversion.cmake)

add_definitions(
	-DUSE_BELLESIP
	-DLIBLINPHONE_EXPORTS
)

set(LIBS
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
if(MSVC AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsPhone")
	list(APPEND LIBS ${LIBGCC} ${LIBMINGWEX})
endif()
if(WIN32 AND NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsPhone")
	list(APPEND LIBS shlwapi)
endif()
if(INTL_FOUND)
	list(APPEND LIBS ${INTL_LIBRARIES})
endif()

if(ENABLE_STATIC)
	add_library(linphone STATIC ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})
	target_link_libraries(linphone ${LIBS})
else()
	add_library(linphone SHARED ${SOURCE_FILES} ${GENERATED_SOURCE_FILES})
	set_target_properties(linphone PROPERTIES VERSION ${LINPHONE_SO_VERSION} LINKER_LANGUAGE CXX)
	target_link_libraries(linphone ${LIBS})
	if(MSVC)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/linphone.pdb
				DESTINATION bin
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()
	endif()
endif()
if(WIN32 AND "${CMAKE_SYSTEM_NAME}" STREQUAL "WindowsPhone")
	set_target_properties(linphone PROPERTIES PREFIX "lib")
endif()
if(ICONV_FOUND)
	if(APPLE)
		# Prevent conflict between the system iconv.h header and the one from macports.
		target_compile_options(linphone PRIVATE "-include" "${ICONV_INCLUDE_DIRS}/iconv.h")
	else()
		target_include_directories(linphone PRIVATE ${ICONV_INCLUDE_DIRS})
	endif()
endif()

install(TARGETS linphone EXPORT LinphoneTargets
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)


set(HEADER_FILES
	buffer.h
	call_log.h
	call_params.h
	content.h
	event.h
	linphonecore.h
	linphonecore_utils.h
	linphonefriend.h
	linphonepresence.h
	linphone_tunnel.h
	lpc2xml.h
	lpconfig.h
	sipsetup.h
	xml2lpc.h
)

install(FILES ${HEADER_FILES}
	DESTINATION include/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if(ENABLE_DOC)
	add_subdirectory(help)
endif()
