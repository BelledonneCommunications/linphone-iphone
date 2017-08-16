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
	c-wrapper/c-private-types.h
	c-wrapper/c-tools.h
	call/call.h
	call/call-listener.h
	call/call-p.h
	chat/chat-room-p.h
	chat/chat-room.h
	chat/imdn.h
	chat/is-composing.h
	conference/conference.h
	conference/conference-listener.h
	conference/conference-p.h
	conference/local-conference.h
	conference/params/call-session-params.h
	conference/params/call-session-params-p.h
	conference/params/media-session-params.h
	conference/params/media-session-params-p.h
	conference/participant.h
	conference/participant-p.h
	conference/remote-conference.h
	conference/session/call-session.h
	conference/session/call-session-listener.h
	conference/session/call-session-p.h
	conference/session/media-session.h
	conference/session/port-config.h
	content/content.h
	core/core.h
	cpim/cpim.h
	cpim/header/cpim-core-headers.h
	cpim/header/cpim-generic-header.h
	cpim/header/cpim-header-p.h
	cpim/header/cpim-header.h
	cpim/message/cpim-message.h
	cpim/parser/cpim-grammar.h
	cpim/parser/cpim-parser.h
	db/abstract/abstract-db-p.h
	db/abstract/abstract-db.h
	db/events-db.h
	db/provider/db-session-p.h
	db/provider/db-session-provider.h
	db/provider/db-session.h
	enums.h
	event-log/call-event.h
	event-log/conference-event-p.h
	event-log/conference-event.h
	event-log/conference-participant-event.h
	event-log/event-log-p.h
	event-log/event-log.h
	event-log/message-event.h
	logger/logger.h
	message/message.h
	nat/ice-agent.h
	nat/stun-client.h
	object/clonable-object-p.h
	object/clonable-object.h
	object/object-p.h
	object/object.h
	object/singleton.h
	utils/content-type.h
	utils/payload-type-handler.h
)

set(LINPHONE_CXX_OBJECTS_SOURCE_FILES
	address/address.cpp
	c-wrapper/api/c-address.cpp
	c-wrapper/api/c-event-log.cpp
	call/call.cpp
	chat/chat-room.cpp
	content/content.cpp
	core/core.cpp
	chat/imdn.cpp
	chat/is-composing.cpp
	conference/conference.cpp
	conference/local-conference.cpp
	conference/params/call-session-params.cpp
	conference/params/media-session-params.cpp
	conference/participant.cpp
	conference/remote-conference.cpp
	conference/session/call-session.cpp
	conference/session/media-session.cpp
	cpim/header/cpim-core-headers.cpp
	cpim/header/cpim-generic-header.cpp
	cpim/header/cpim-header.cpp
	cpim/message/cpim-message.cpp
	cpim/parser/cpim-grammar.cpp
	cpim/parser/cpim-parser.cpp
	db/abstract/abstract-db.cpp
	db/events-db.cpp
	db/provider/db-session-provider.cpp
	db/provider/db-session.cpp
	event-log/call-event.cpp
	event-log/conference-event.cpp
	event-log/conference-participant-event.cpp
	event-log/event-log.cpp
	event-log/message-event.cpp
	logger/logger.cpp
	message/message.cpp
	nat/ice-agent.cpp
	nat/stun-client.cpp
	object/clonable-object.cpp
	object/object.cpp
	utils/content-type.cpp
	utils/general.cpp
	utils/payload-type-handler.cpp
	utils/utils.cpp
)

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
	add_library(linphone-cxx-objects-static OBJECT ${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES} ${LINPHONE_CXX_OBJECTS_SOURCE_FILES})
	target_compile_definitions(linphone-cxx-objects-static PRIVATE ${LINPHONE_CXX_OBJECTS_DEFINITIONS})
	target_include_directories(linphone-cxx-objects-static SYSTEM PRIVATE ${LINPHONE_CXX_OBJECTS_INCLUDE_DIRS} ${LINPHONE_INCLUDE_DIRS})
endif()

if(ENABLE_SHARED)
	add_library(linphone-cxx-objects OBJECT ${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES} ${LINPHONE_CXX_OBJECTS_SOURCE_FILES})
	target_compile_definitions(linphone-cxx-objects PRIVATE ${LINPHONE_CXX_OBJECTS_DEFINITIONS})
	target_include_directories(linphone-cxx-objects SYSTEM PRIVATE ${LINPHONE_CXX_OBJECTS_INCLUDE_DIRS} ${LINPHONE_INCLUDE_DIRS})
	target_compile_options(linphone-cxx-objects PRIVATE "-fPIC")
endif()
