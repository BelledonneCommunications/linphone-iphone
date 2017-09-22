############################################################################
# CMakeLists.txt
# Copyright (C) 2017  Belledonne Communications, Grenoble France
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

set(LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES
	address/address-p.h
	address/address.h
	c-wrapper/c-wrapper.h
	c-wrapper/internal/c-tools.h
	call/call-listener.h
	call/call-p.h
	call/call.h
	chat/basic-chat-room-p.h
	chat/basic-chat-room.h
	chat/chat-message-p.h
	chat/chat-message.h
	chat/chat-room-p.h
	chat/chat-room.h
	chat/client-group-chat-room-p.h
	chat/client-group-chat-room.h
	chat/cpim/cpim.h
	chat/cpim/header/cpim-core-headers.h
	chat/cpim/header/cpim-generic-header.h
	chat/cpim/header/cpim-header-p.h
	chat/cpim/header/cpim-header.h
	chat/cpim/message/cpim-message.h
	chat/cpim/parser/cpim-grammar.h
	chat/cpim/parser/cpim-parser.h
	chat/imdn.h
	chat/is-composing.h
	chat/modifier/chat-message-modifier.h
	chat/modifier/cpim-chat-message-modifier.h
	chat/modifier/multipart-chat-message-modifier.h
	chat/real-time-text-chat-room-p.h
	chat/real-time-text-chat-room.h
	conference/conference-listener.h
	conference/conference.h
	conference/local-conference.h
	conference/local-conference-event-handler.h
	conference/params/call-session-params-p.h
	conference/params/call-session-params.h
	conference/params/media-session-params-p.h
	conference/params/media-session-params.h
	conference/participant-p.h
	conference/participant.h
	conference/remote-conference.h
	conference/remote-conference-event-handler.h
	conference/session/call-session-listener.h
	conference/session/call-session-p.h
	conference/session/call-session.h
	conference/session/media-session.h
	conference/session/port-config.h
	content/content-type.h
	content/content.h
	core/core.h
	db/abstract/abstract-db-p.h
	db/abstract/abstract-db.h
	db/events-db.h
	db/provider/db-session-p.h
	db/provider/db-session-provider.h
	db/provider/db-session.h
	enums.h
	event-log/call-event.h
	event-log/chat-message-event.h
	event-log/conference-event-p.h
	event-log/conference-event.h
	event-log/conference-participant-event.h
	event-log/event-log-p.h
	event-log/event-log.h
	logger/logger.h
	nat/ice-agent.h
	nat/stun-client.h
	object/clonable-object-p.h
	object/clonable-object.h
	object/object-p.h
	object/object.h
	object/property-container.h
	object/singleton.h
	utils/payload-type-handler.h
	variant/variant.h
	xml/conference-info.h
	xml/resource-lists.h
	xml/xml.h
)

set(LINPHONE_CXX_OBJECTS_SOURCE_FILES
	address/address.cpp
	c-wrapper/api/c-address.cpp
	c-wrapper/api/c-call.cpp
	c-wrapper/api/c-call-cbs.cpp
	c-wrapper/api/c-call-params.cpp
	c-wrapper/api/c-call-stats.cpp
	c-wrapper/api/c-chat-message.cpp
	c-wrapper/api/c-chat-message-cbs.cpp
	c-wrapper/api/c-chat-room.cpp
	c-wrapper/api/c-chat-room-cbs.cpp
	c-wrapper/api/c-event-log.cpp
	c-wrapper/api/c-participant.cpp
	call/call.cpp
	chat/basic-chat-room.cpp
	chat/chat-message.cpp
	chat/chat-room.cpp
	chat/client-group-chat-room.cpp
	chat/cpim/header/cpim-core-headers.cpp
	chat/cpim/header/cpim-generic-header.cpp
	chat/cpim/header/cpim-header.cpp
	chat/cpim/message/cpim-message.cpp
	chat/cpim/parser/cpim-grammar.cpp
	chat/cpim/parser/cpim-parser.cpp
	chat/imdn.cpp
	chat/is-composing.cpp
	chat/modifier/cpim-chat-message-modifier.cpp
	chat/modifier/multipart-chat-message-modifier.cpp
	chat/real-time-text-chat-room.cpp
	conference/conference.cpp
	conference/local-conference.cpp
	conference/local-conference-event-handler.cpp
	conference/params/call-session-params.cpp
	conference/params/media-session-params.cpp
	conference/participant.cpp
	conference/remote-conference.cpp
	conference/remote-conference-event-handler.cpp
	conference/session/call-session.cpp
	conference/session/media-session.cpp
	content/content-type.cpp
	content/content.cpp
	core/core.cpp
	db/abstract/abstract-db.cpp
	db/events-db.cpp
	db/provider/db-session-provider.cpp
	db/provider/db-session.cpp
	event-log/call-event.cpp
	event-log/chat-message-event.cpp
	event-log/conference-event.cpp
	event-log/conference-participant-event.cpp
	event-log/event-log.cpp
	logger/logger.cpp
	nat/ice-agent.cpp
	nat/stun-client.cpp
	object/clonable-object.cpp
	object/object.cpp
	object/property-container.cpp
	utils/general.cpp
	utils/payload-type-handler.cpp
	utils/utils.cpp
	variant/variant.cpp
	xml/conference-info.cpp
	xml/resource-lists.cpp
	xml/xml.cpp
)

set(LINPHONE_CXX_OBJECTS_INCLUDE_DIRS ${BELR_INCLUDE_DIRS} ${LIBXSD_INCLUDE_DIRS})
set(LINPHONE_CXX_OBJECTS_DEFINITIONS "-DLIBLINPHONE_EXPORTS")
set(LINPHONE_CXX_OBJECTS_INCLUDE_DIRS ${BELR_INCLUDE_DIRS})

if(SOCI_FOUND)
	list(APPEND LINPHONE_CXX_OBJECTS_INCLUDE_DIRS ${SOCI_INCLUDE_DIRS} ${SOCI_MYSQL_INCLUDES})
	add_definitions(-DSOCI_ENABLED)
endif()

set(LINPHONE_PRIVATE_HEADER_FILES)
foreach(header ${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES})
	list(APPEND LINPHONE_PRIVATE_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
endforeach()
set(LINPHONE_PRIVATE_HEADER_FILES ${LINPHONE_PRIVATE_HEADER_FILES} PARENT_SCOPE)

bc_apply_compile_flags(LINPHONE_CXX_OBJECTS_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_CXX)

if(ENABLE_STATIC)
	add_library(
		linphone-cxx-objects-static OBJECT
		${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES} ${LINPHONE_CXX_OBJECTS_SOURCE_FILES}
	)
	target_compile_definitions(linphone-cxx-objects-static PRIVATE ${LINPHONE_CXX_OBJECTS_DEFINITIONS})
	target_include_directories(linphone-cxx-objects-static SYSTEM PRIVATE ${LINPHONE_CXX_OBJECTS_INCLUDE_DIRS} ${LINPHONE_INCLUDE_DIRS})
endif()

if(ENABLE_SHARED)
	add_library(
		linphone-cxx-objects OBJECT
		${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES} ${LINPHONE_CXX_OBJECTS_SOURCE_FILES}
	)
	target_compile_definitions(linphone-cxx-objects PRIVATE ${LINPHONE_CXX_OBJECTS_DEFINITIONS})
	target_include_directories(linphone-cxx-objects SYSTEM PRIVATE ${LINPHONE_CXX_OBJECTS_INCLUDE_DIRS} ${LINPHONE_INCLUDE_DIRS})
	target_compile_options(linphone-cxx-objects PRIVATE "-fPIC")
endif()
