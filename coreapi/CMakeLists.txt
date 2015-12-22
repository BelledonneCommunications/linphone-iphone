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

if(MSVC AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone")
	find_library(LIBGCC NAMES gcc)
	find_library(LIBMINGWEX NAMES mingwex)
endif()

if(NOT WIN32)
	find_package(Iconv QUIET)
endif()


set(LINPHONE_HEADER_FILES
	account_creator.h
	buffer.h
	call_log.h
	call_params.h
	content.h
	event.h
	friendlist.h
	linphonecore.h
	linphonecore_utils.h
	linphonefriend.h
	linphonepresence.h
	linphone_proxy_config.h
	linphone_tunnel.h
	lpc2xml.h
	lpconfig.h
	sipsetup.h
	xml2lpc.h
	xmlrpc.h
	vcard.h
)

set(LINPHONE_SOURCE_FILES_C
	account_creator.c
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
	chat_file_transfer.c
	conference.c
	contactprovider.c
	content.c
	dict.c
	ec-calibrator.c
	enum.c
	enum.h
	event.c
	friend.c
	friendlist.c
	info.c
	ldap/ldapprovider.c
	lime.c
	linphonecall.c
	linphonecore.c
	linphone_tunnel_config.c
	localplayer.c
	lpc2xml.c
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
	xml2lpc.c
	xml.c
	xmlrpc.c
	vtables.c
)
set(LINPHONE_SOURCE_FILES_CXX )

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
if(MSVC AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone")
	list(APPEND LIBS ${LIBGCC} ${LIBMINGWEX})
endif()
if(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone")
	list(APPEND LIBS shlwapi)
endif()
if(INTL_FOUND)
	list(APPEND LIBS ${INTL_LIBRARIES})
endif()
if(BELCARD_FOUND)
	list(APPEND LIBS ${BELCARD_LIBRARIES})
	list(APPEND LINPHONE_SOURCE_FILES_CXX vcard.cc)
	add_definitions(-DVCARD_ENABLED)
else()
	list(APPEND LINPHONE_SOURCE_FILES_C vcard_stubs.c)
endif()

apply_compile_flags(LINPHONE_SOURCE_FILES_C "CPP" "C")
apply_compile_flags(LINPHONE_SOURCE_FILES_CXX "CPP" "CXX")

if(ENABLE_STATIC)
	add_library(linphone STATIC ${LINPHONE_HEADER_FILES} ${LINPHONE_SOURCE_FILES_C} ${LINPHONE_SOURCE_FILES_CXX})
	target_link_libraries(linphone ${LIBS})
else()
	add_library(linphone SHARED ${LINPHONE_HEADER_FILES} ${LINPHONE_SOURCE_FILES_C} ${LINPHONE_SOURCE_FILES_CXX})
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
add_dependencies(linphone liblinphone-git-version)
if(WIN32 AND CMAKE_SYSTEM_NAME STREQUAL "WindowsPhone")
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

install(TARGETS linphone EXPORT ${EXPORT_TARGETS_NAME}Targets
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

install(FILES ${LINPHONE_HEADER_FILES}
	DESTINATION include/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if(ENABLE_DOC)
	add_subdirectory(help)
endif()
