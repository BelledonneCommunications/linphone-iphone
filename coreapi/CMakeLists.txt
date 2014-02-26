find_library(LIBORTP NAMES ortp)
find_library(LIBMEDIASTREAMER_BASE NAMES mediastreamer_base)
find_library(LIBMEDIASTREAMER_VOIP NAMES mediastreamer_voip)
find_library(LIBBELLESIP NAMES bellesip)
find_library(LIBXML2 NAMES xml2)

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
	sal.c
	siplogin.c
	sipsetup.c
	#TunnelManager.cc
	xml.c
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

if(LINPHONE_ENABLE_VIDEO)
	add_definitions(-DVIDEO_ENABLED)
endif(LINPHONE_ENABLE_VIDEO)

if(WIN32)
add_definitions(
	-DWINDOW_NATIVE
	/FIliblinphone_gitversion.h
)

set(LIBS ws2_32)
endif(WIN32)
set(LIBS ${LIBS} ${LIBORTP} ${LIBMEDIASTREAMER_BASE} ${LIBMEDIASTREAMER_VOIP} ${LIBBELLESIP} ${LIBXML2})

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
endif(WIN32)
