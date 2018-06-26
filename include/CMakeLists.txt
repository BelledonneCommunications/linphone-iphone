################################################################################
# CMakeLists.txt
# Copyright (C) 2017  Belledonne Communications, Grenoble France
#
################################################################################
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
################################################################################

set(ROOT_HEADER_FILES
	account_creator_service.h
	account_creator.h
	auth_info.h
	buffer.h
	call_log.h
	call_params.h
	call_stats.h
	call.h
	callbacks.h
	chat.h
	conference.h
	contactprovider.h
	core_utils.h
	core.h
	defs.h
	dictionary.h
	error_info.h
	event.h
	factory.h
	friend.h
	friendlist.h
	headers.h
	im_encryption_engine.h
	im_notif_policy.h
	info_message.h
	ldapprovider.h
	logging.h
	lpconfig.h
	misc.h
	nat_policy.h
	payload_type.h
	player.h
	presence.h
	proxy_config.h
	ringtoneplayer.h
	sipsetup.h
	tunnel.h
	types.h
	vcard.h
	video_definition.h
	wrapper_utils.h
	xmlrpc.h

	# Deprecated header files
	linphone_proxy_config.h
	linphone_tunnel.h
	linphonecore_utils.h
	linphonecore.h
	linphonefriend.h
	linphonepresence.h
)

set(C_API_HEADER_FILES
	c-address.h
	c-api.h
	c-call-cbs.h
	c-call-stats.h
	c-call.h
	c-callbacks.h
	c-chat-message-cbs.h
	c-chat-message.h
	c-chat-room-cbs.h
	c-chat-room.h
	c-content.h
	c-dial-plan.h
	c-event-log.h
	c-magic-search.h
	c-participant.h
	c-participant-imdn-state.h
	c-search-result.h
	c-types.h
)

set(ENUMS_HEADER_FILES
	call-enums.h
	chat-message-enums.h
	chat-room-enums.h
	event-log-enums.h
)

set(UTILS_HEADER_FILES
	algorithm.h
	enum-generator.h
	enum-mask.h
	fs.h
	general.h
	magic-macros.h
	traits.h
	utils.h
)

# ------------------------------------------------------------------------------

function (PREPEND OUT_LIST PREFIX)
	set(TMP_LIST )
	foreach (FILENAME ${ARGN})
		list(APPEND TMP_LIST "${PREFIX}/${FILENAME}")
	endforeach ()
	set(${OUT_LIST} "${TMP_LIST}" PARENT_SCOPE)
endfunction ()

# ------------------------------------------------------------------------------

set(SRC_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/linphone")

PREPEND(ROOT_HEADER_FILES "${SRC_ROOT_DIRECTORY}" ${ROOT_HEADER_FILES})
PREPEND(C_API_HEADER_FILES "${SRC_ROOT_DIRECTORY}/api" ${C_API_HEADER_FILES})
PREPEND(ENUMS_HEADER_FILES "${SRC_ROOT_DIRECTORY}/enums" ${ENUMS_HEADER_FILES})
PREPEND(UTILS_HEADER_FILES "${SRC_ROOT_DIRECTORY}/utils" ${UTILS_HEADER_FILES})

set(LINPHONE_HEADER_FILES ${ROOT_HEADER_FILES} ${C_API_HEADER_FILES} ${ENUMS_HEADER_FILES} ${UTILS_HEADER_FILES} PARENT_SCOPE)
set(LINPHONE_HEADER_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}" PARENT_SCOPE)

# ------------------------------------------------------------------------------
if (IOS AND ENABLE_SHARED) 
	#cmake 3.10 seems not able to handle subdirectories for PUBLIC_HEADER. My be rework in the futur
	set(DEST_ROOT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/Frameworks/linphone.framework/Headers")
else()
	set(DEST_ROOT_DIRECTORY "${CMAKE_INSTALL_INCLUDEDIR}/linphone")
endif()

install(FILES ${ROOT_HEADER_FILES}
	DESTINATION "${DEST_ROOT_DIRECTORY}"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install(FILES ${C_API_HEADER_FILES}
	DESTINATION "${DEST_ROOT_DIRECTORY}/api"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(FILES ${ENUMS_HEADER_FILES}
	DESTINATION "${DEST_ROOT_DIRECTORY}/enums"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install(FILES ${UTILS_HEADER_FILES}
	DESTINATION "${DEST_ROOT_DIRECTORY}/utils"
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
