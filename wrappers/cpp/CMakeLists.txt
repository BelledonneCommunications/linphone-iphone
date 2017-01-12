set(GENERATED_SOURCES
	src/account_creator.cc
	src/address.cc
	src/auth_info.cc
	src/buffer.cc
	src/call.cc
	src/call_log.cc
	src/call_params.cc
	src/call_stats.cc
	src/chat_message.cc
	src/chat_room.cc
	src/conference.cc
	src/conference_params.cc
	src/config.cc
	src/content.cc
	src/core.cc
	src/core_v_table.cc
	src/error_info.cc
	src/event.cc
	src/factory.cc
	src/friend.cc
	src/friend_list.cc
	src/nat_policy.cc
	src/payload_type.cc
	src/player.cc
	src/presence_activity.cc
	src/presence_model.cc
	src/presence_note.cc
	src/presence_person.cc
	src/presence_service.cc
	src/proxy_config.cc
	src/sip_transports.cc
	src/tunnel.cc
	src/tunnel_config.cc
	src/vcard.cc
	src/video_policy.cc
	src/xml_rpc_request.cc
	src/xml_rpc_session.cc
)
set(SOURCES
	${GENERATED_SOURCES}
	object.cc
)
set(GENERATED_HEADERS
	${CMAKE_CURRENT_BINARY_DIR}/include/account_creator.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/account_creator_listener.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/address.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/auth_info.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/buffer.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/call.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/call_log.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/call_params.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/call_stats.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/chat_message.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/chat_message_listener.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/chat_room.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/conference.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/conference_params.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/config.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/content.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/core.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/core_listener.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/core_v_table.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/enums.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/error_info.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/event.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/factory.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/friend.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/friend_list.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/friend_list_listener.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/linphone.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/nat_policy.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/payload_type.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/player.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/presence_activity.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/presence_model.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/presence_note.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/presence_person.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/presence_service.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/proxy_config.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/sip_transports.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/tunnel_config.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/tunnel.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/vcard.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/video_policy.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/xml_rpc_request.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/xml_rpc_request_listener.hh
	${CMAKE_CURRENT_BINARY_DIR}/include/xml_rpc_session.hh
)
set(HEADERS
	${GENERATED_HEADERS}
	object.hh
)

add_custom_command(OUTPUT ${GENERATED_SOURCES} ${GENERATED_HEADERS}
	COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/genheaders.py" "${PROJECT_BINARY_DIR}/coreapi/help/doc/xml"
	DEPENDS *.py *.mustache linphone-doc "${PROJECT_BINARY_DIR}/coreapi/help/doc/xml/index.xml"
)

add_compile_options(-Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -Werror --std=c++11)
add_definitions(-DWRAPPER_BUILD)

add_library(linphone++ SHARED ${SOURCES})
target_link_libraries(linphone++ PRIVATE linphone)
target_include_directories(linphone++
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${PROJECT_SOURCE_DIR}
)
set_target_properties(linphone++
	PROPERTIES SOVERSION ${LINPHONE_SO_VERSION}
)

install(TARGETS linphone++
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ${HEADERS}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/linphone++
)

