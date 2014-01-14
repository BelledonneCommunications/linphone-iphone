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
	#-DVIDEO_ENABLED
	-DLINPHONE_PACKAGE_NAME="linphone"
	-DLINPHONE_VERSION="Devel"
	-DLIBLINPHONE_EXPORTS
	-DLINPHONE_PLUGINS_DIR=""
)

if(WIN32)
add_definitions(
	-DWINDOW_NATIVE
	/FIliblinphone_gitversion.h
)

set(LIBS ws2_32)
endif(WIN32)
set(LIBS ${LIBS} libortp libmediastreamer_base libmediastreamer_voip libbellesip libxml2)

add_library(liblinphone SHARED ${SOURCE_FILES})
set_target_properties(liblinphone PROPERTIES VERSION 3.6.99 SOVERSION 5)

target_link_libraries(liblinphone ${LIBS})

install(TARGETS liblinphone
		COMPONENT COMP_liblinphone
        DESTINATION ${LIB_INSTALL_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

if(USE_INSTALLED_COMPONENTS)
	add_dependencies(liblinphone
		INSTALL_libortp
		INSTALL_libmediastreamer2
		INSTALL_libbellesip
		INSTALL_libxml2)
endif()



file(GLOB HEADER_FILES "*.h")

install(FILES ${HEADER_FILES}
		COMPONENT COMP_liblinphone
        DESTINATION include/linphone
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
