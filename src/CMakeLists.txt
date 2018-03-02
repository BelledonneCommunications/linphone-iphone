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
	address/identity-address.h
	c-wrapper/c-wrapper.h
	c-wrapper/internal/c-sal.h
	c-wrapper/internal/c-tools.h
	call/call-p.h
	call/call.h
	call/local-conference-call-p.h
	call/local-conference-call.h
	call/remote-conference-call-p.h
	call/remote-conference-call.h
	chat/chat-message/chat-message-p.h
	chat/chat-message/chat-message.h
	chat/chat-room/abstract-chat-room-p.h
	chat/chat-room/abstract-chat-room.h
	chat/chat-room/basic-chat-room-p.h
	chat/chat-room/basic-chat-room.h
	chat/chat-room/basic-to-client-group-chat-room.h
	chat/chat-room/chat-room-id.h
	chat/chat-room/chat-room-listener.h
	chat/chat-room/chat-room-p.h
	chat/chat-room/chat-room.h
	chat/chat-room/client-group-chat-room-p.h
	chat/chat-room/client-group-chat-room.h
	chat/chat-room/client-group-to-basic-chat-room.h
	chat/chat-room/proxy-chat-room-p.h
	chat/chat-room/proxy-chat-room.h
	chat/chat-room/real-time-text-chat-room-p.h
	chat/chat-room/real-time-text-chat-room.h
	chat/chat-room/server-group-chat-room-p.h
	chat/chat-room/server-group-chat-room.h
	chat/cpim/cpim.h
	chat/cpim/header/cpim-core-headers.h
	chat/cpim/header/cpim-generic-header.h
	chat/cpim/header/cpim-header-p.h
	chat/cpim/header/cpim-header.h
	chat/cpim/message/cpim-message.h
	chat/cpim/parser/cpim-parser.h
	chat/modifier/chat-message-modifier.h
	chat/modifier/cpim-chat-message-modifier.h
	chat/modifier/encryption-chat-message-modifier.h
	chat/modifier/file-transfer-chat-message-modifier.h
	chat/modifier/multipart-chat-message-modifier.h
	chat/notification/imdn.h
	chat/notification/is-composing-listener.h
	chat/notification/is-composing.h
	conference/conference-listener.h
	conference/conference-p.h
	conference/conference.h
	conference/handlers/local-conference-event-handler-p.h
	conference/handlers/local-conference-event-handler.h
	conference/handlers/remote-conference-event-handler-p.h
	conference/handlers/remote-conference-event-handler.h
	conference/local-conference-p.h
	conference/local-conference.h
	conference/params/call-session-params-p.h
	conference/params/call-session-params.h
	conference/params/media-session-params-p.h
	conference/params/media-session-params.h
	conference/participant-device.h
	conference/participant-p.h
	conference/participant.h
	conference/remote-conference-p.h
	conference/remote-conference.h
	conference/session/call-session-listener.h
	conference/session/call-session-p.h
	conference/session/call-session.h
	conference/session/media-session.h
	conference/session/port-config.h
	containers/lru-cache.h
	content/content-manager.h
	content/content-p.h
	content/content-type.h
	content/content.h
	content/file-content.h
	content/file-transfer-content.h
	core/core-accessor.h
	core/core-listener.h
	core/core-p.h
	core/core.h
	core/paths/paths.h
	core/platform-helpers/platform-helpers.h
	db/abstract/abstract-db-p.h
	db/abstract/abstract-db.h
	db/internal/db-transaction.h
	db/internal/statements.h
	db/main-db-chat-message-key.h
	db/main-db-event-key.h
	db/main-db-key-p.h
	db/main-db-key.h
	db/main-db-p.h
	db/main-db.h
	db/session/db-session.h
	dial-plan/dial-plan-p.h
	dial-plan/dial-plan.h
	enums.h
	event-log/conference/conference-call-event.h
	event-log/conference/conference-event-p.h
	event-log/conference/conference-event.h
	event-log/conference/conference-notified-event-p.h
	event-log/conference/conference-notified-event.h
	event-log/conference/conference-participant-device-event.h
	event-log/conference/conference-participant-event-p.h
	event-log/conference/conference-participant-event.h
	event-log/conference/conference-subject-event.h
	event-log/event-log-p.h
	event-log/event-log.h
	event-log/events.h
	hacks/hacks.h
	logger/logger.h
	nat/ice-agent.h
	nat/stun-client.h
	object/app-data-container.h
	object/base-object-p.h
	object/base-object.h
	object/clonable-object-p.h
	object/clonable-object.h
	object/clonable-shared-pointer.h
	object/object-head-p.h
	object/object-head.h
	object/object-p.h
	object/object.h
	object/property-container.h
	object/singleton.h
	sal/sal.h
	search/magic-search-p.h
	search/magic-search.h
	search/search-result-p.h
	search/search-result.h
	utils/background-task.h
	utils/payload-type-handler.h
	variant/variant.h
	xml/conference-info.h
	xml/resource-lists.h
	xml/xml.h
)

set(LINPHONE_CXX_OBJECTS_SOURCE_FILES
	address/address.cpp
	address/identity-address.cpp
	c-wrapper/api/c-address.cpp
	c-wrapper/api/c-call-cbs.cpp
	c-wrapper/api/c-call-params.cpp
	c-wrapper/api/c-call-stats.cpp
	c-wrapper/api/c-call.cpp
	c-wrapper/api/c-chat-message-cbs.cpp
	c-wrapper/api/c-chat-message.cpp
	c-wrapper/api/c-chat-room-cbs.cpp
	c-wrapper/api/c-chat-room.cpp
	c-wrapper/api/c-core.cpp
	c-wrapper/api/c-dial-plan.cpp
	c-wrapper/api/c-event-log.cpp
	c-wrapper/api/c-magic-search.cpp
	c-wrapper/api/c-participant.cpp
	c-wrapper/api/c-search-result.cpp
	c-wrapper/internal/c-sal.cpp
	c-wrapper/internal/c-tools.cpp
	call/call.cpp
	call/local-conference-call.cpp
	call/remote-conference-call.cpp
	chat/chat-message/chat-message.cpp
	chat/chat-room/abstract-chat-room.cpp
	chat/chat-room/basic-chat-room.cpp
	chat/chat-room/basic-to-client-group-chat-room.cpp
	chat/chat-room/chat-room-id.cpp
	chat/chat-room/chat-room.cpp
	chat/chat-room/client-group-chat-room.cpp
	chat/chat-room/client-group-to-basic-chat-room.cpp
	chat/chat-room/proxy-chat-room.cpp
	chat/chat-room/real-time-text-chat-room.cpp
	chat/chat-room/server-group-chat-room-stub.cpp
	chat/cpim/header/cpim-core-headers.cpp
	chat/cpim/header/cpim-generic-header.cpp
	chat/cpim/header/cpim-header.cpp
	chat/cpim/message/cpim-message.cpp
	chat/cpim/parser/cpim-parser.cpp
	chat/modifier/cpim-chat-message-modifier.cpp
	chat/modifier/encryption-chat-message-modifier.cpp
	chat/modifier/file-transfer-chat-message-modifier.cpp
	chat/modifier/multipart-chat-message-modifier.cpp
	chat/notification/imdn.cpp
	chat/notification/is-composing.cpp
	conference/conference.cpp
	conference/handlers/local-conference-event-handler.cpp
	conference/handlers/remote-conference-event-handler.cpp
	conference/local-conference.cpp
	conference/params/call-session-params.cpp
	conference/params/media-session-params.cpp
	conference/participant-device.cpp
	conference/participant.cpp
	conference/remote-conference.cpp
	conference/session/call-session.cpp
	conference/session/media-session.cpp
	content/content-manager.cpp
	content/content-type.cpp
	content/content.cpp
	content/file-content.cpp
	content/file-transfer-content.cpp
	core/core-accessor.cpp
	core/core-call.cpp
	core/core-chat-room.cpp
	core/core.cpp
	core/paths/paths.cpp
	core/platform-helpers/platform-helpers.cpp
	db/abstract/abstract-db.cpp
	db/internal/statements.cpp
	db/main-db-chat-message-key.cpp
	db/main-db-event-key.cpp
	db/main-db-key.cpp
	db/main-db.cpp
	db/session/db-session.cpp
	dial-plan/dial-plan.cpp
	event-log/conference/conference-call-event.cpp
	event-log/conference/conference-chat-message-event.cpp
	event-log/conference/conference-event.cpp
	event-log/conference/conference-notified-event.cpp
	event-log/conference/conference-participant-device-event.cpp
	event-log/conference/conference-participant-event.cpp
	event-log/conference/conference-subject-event.cpp
	event-log/event-log.cpp
	hacks/hacks.cpp
	logger/logger.cpp
	nat/ice-agent.cpp
	nat/stun-client.cpp
	object/app-data-container.cpp
	object/base-object.cpp
	object/clonable-object.cpp
	object/object.cpp
	object/property-container.cpp
	sal/call-op.cpp
	sal/event-op.cpp
	sal/message-op.cpp
	sal/op.cpp
	sal/presence-op.cpp
	sal/refer-op.cpp
	sal/register-op.cpp
	sal/sal.cpp
	search/magic-search.cpp
	search/search-result.cpp
	utils/background-task.cpp
	utils/fs.cpp
	utils/general.cpp
	utils/payload-type-handler.cpp
	utils/utils.cpp
	variant/variant.cpp
	xml/conference-info.cpp
	xml/resource-lists.cpp
	xml/xml.cpp
)

set(LINPHONE_OBJC_SOURCE_FILES)
if (APPLE)
	list(APPEND LINPHONE_OBJC_SOURCE_FILES core/paths/paths-apple.mm)
	list(APPEND LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES core/paths/paths-apple.h)
elseif (ANDROID)
	list(APPEND LINPHONE_CXX_OBJECTS_SOURCE_FILES core/paths/paths-android.cpp core/platform-helpers/android-platform-helpers.cpp)
	list(APPEND LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES core/paths/paths-android.h)
elseif (WIN32)
	list(APPEND LINPHONE_CXX_OBJECTS_SOURCE_FILES core/paths/paths-windows.cpp)
	list(APPEND LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES core/paths/paths-windows.h)
elseif (UNIX)
	list(APPEND LINPHONE_CXX_OBJECTS_SOURCE_FILES core/paths/paths-linux.cpp)
	list(APPEND LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES core/paths/paths-linux.h)
endif ()

set(LINPHONE_CXX_OBJECTS_INCLUDE_DIRS
	${BELR_INCLUDE_DIRS}
	${LIBXSD_INCLUDE_DIRS}
	${SOCI_INCLUDE_DIRS}
	${SOCI_MYSQL_INCLUDES}
)
set(LINPHONE_CXX_OBJECTS_DEFINITIONS "-DLIBLINPHONE_EXPORTS")
set(LINPHONE_CXX_OBJECTS_INCLUDE_DIRS ${BELR_INCLUDE_DIRS})

set(LINPHONE_PRIVATE_HEADER_FILES)
foreach (header ${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES})
	list(APPEND LINPHONE_PRIVATE_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/${header}")
endforeach ()
set(LINPHONE_PRIVATE_HEADER_FILES ${LINPHONE_PRIVATE_HEADER_FILES} PARENT_SCOPE)

bc_apply_compile_flags(LINPHONE_CXX_OBJECTS_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_CXX)
bc_apply_compile_flags(LINPHONE_OBJC_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_OBJC)

if (ENABLE_STATIC)
	add_library(
		linphone-cxx-objects-static OBJECT
		${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES} ${LINPHONE_CXX_OBJECTS_SOURCE_FILES} ${LINPHONE_OBJC_SOURCE_FILES}
	)
	target_compile_definitions(linphone-cxx-objects-static PRIVATE ${LINPHONE_CXX_OBJECTS_DEFINITIONS})
	target_include_directories(linphone-cxx-objects-static SYSTEM PRIVATE ${LINPHONE_CXX_OBJECTS_INCLUDE_DIRS} ${LINPHONE_INCLUDE_DIRS})
endif ()

if (ENABLE_SHARED)
	add_library(
		linphone-cxx-objects OBJECT
		${LINPHONE_CXX_OBJECTS_PRIVATE_HEADER_FILES} ${LINPHONE_CXX_OBJECTS_SOURCE_FILES} ${LINPHONE_OBJC_SOURCE_FILES}
	)
	target_compile_definitions(linphone-cxx-objects PRIVATE ${LINPHONE_CXX_OBJECTS_DEFINITIONS})
	target_include_directories(linphone-cxx-objects SYSTEM PRIVATE ${LINPHONE_CXX_OBJECTS_INCLUDE_DIRS} ${LINPHONE_INCLUDE_DIRS})
	target_compile_options(linphone-cxx-objects PRIVATE "-fPIC")
endif ()
