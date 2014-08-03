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

if(MSVC)
	find_library(LIBGCC NAMES gcc)
	find_library(LIBMINGWEX NAMES mingwex)
endif()

find_program(GIT git)

set(GIT_VERSION "unknown")
if(GIT)
	execute_process(
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${GIT} describe --always
		OUTPUT_VARIABLE GIT_DESCRIBE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${GIT} describe --abbrev=0
		OUTPUT_VARIABLE GIT_TAG
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMAND ${GIT} rev-parse HEAD
		OUTPUT_VARIABLE GIT_REVISION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if(GIT_DESCRIBE)
		set(GIT_VERSION ${GIT_DESCRIBE})
	else(GIT_DESCRIBE)
		if(GIT_REVISION)
			set(GIT_VERSION ${GIT_REVISION})
		endif(GIT_REVISION)
	endif(GIT_DESCRIBE)
endif(GIT)
execute_process(
	COMMAND ${CMAKE_COMMAND} -E echo "#define LIBLINPHONE_GIT_VERSION \"${GIT_VERSION}\""
	OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/liblinphone_gitversion.h
)

set(SOURCE_FILES
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
	callbacks.c
	chat.c
	conference.c
	ec-calibrator.c
	enum.c
	event.c
	friend.c
	info.c
	linphonecall.c
	linphonecore.c
	#linphone_tunnel.cc
	linphone_tunnel_stubs.c
	linphone_tunnel_config.c
	lpconfig.c
	lsd.c
	message_storage.c
	misc.c
	offeranswer.c
	presence.c
	proxy.c
	quality_reporting.c
	remote_provisioning.c
	sal.c
	siplogin.c
	sipsetup.c
	#TunnelManager.cc
	xml.c
	xml2lpc.c
	bellesip_sal/sal_impl.h
	enum.h
	event.h
	linphonecore.h
	linphonecore_utils.h
	linphonefriend.h
	linphone_tunnel.h
	lpconfig.h
	offeranswer.h
	private.h
	sipsetup.h
	xml2lpc.h
)

add_definitions(
	-D_TRUE_TIME
	-DIN_LINPHONE
	-DUSE_BELLESIP
	#-DTUNNEL_ENABLED
	-DLINPHONE_PACKAGE_NAME="linphone"
	-DLINPHONE_VERSION="Devel"
	-DLIBLINPHONE_EXPORTS
	-DLINPHONE_PLUGINS_DIR=""
)

if(ENABLE_VIDEO)
	add_definitions(-DVIDEO_ENABLED)
endif()

set(LIBS
	${LIBGCC}
	${LIBMINGWEX}
	${BELLESIP_LIBRARIES}
	${MS2_LIBRARIES}
	${XML2_LIBRARIES}
)
if(WIN32)
add_definitions(
	-DWINDOW_NATIVE
	/FIliblinphone_gitversion.h
)
endif()

add_library(linphone SHARED ${SOURCE_FILES})
set_target_properties(linphone PROPERTIES VERSION 3.7.0 SOVERSION 5)

target_link_libraries(linphone ${LIBS})

install(TARGETS linphone
    RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)


file(GLOB HEADER_FILES "*.h")

install(FILES ${HEADER_FILES}
	DESTINATION include/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
if(WIN32)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/linphone.pdb
	DESTINATION bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
endif()
endif()
