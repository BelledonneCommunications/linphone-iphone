############################################################################
# CMakeLists.txt
# Copyright (C) 2010-2016  Belledonne Communications, Grenoble France
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

set(HEADER_FILES
	account_creator.h
	address.h
	auth_info.h
	buffer.h
	call.h
	call_log.h
	call_params.h
	call_stats.h
	chat.h
	conference.h
	contactprovider.h
	content.h
	core.h
	core_utils.h
	dictionary.h
	error_info.h
	event.h
	factory.h
	friend.h
	friendlist.h
	im_notif_policy.h
	ldapprovider.h
	lpconfig.h
	nat_policy.h
	presence.h
	proxy_config.h
	ringtoneplayer.h
	sipsetup.h
	tunnel.h
	vcard.h
	xmlrpc.h
	im_encryption_engine.h
	wrapper_utils.h

	# Deprecated header files
	linphonecore.h
	linphonecore_utils.h
	linphonefriend.h
	linphonepresence.h
	linphone_proxy_config.h
	linphone_tunnel.h
)

set(LINPHONE_HEADER_FILES )
foreach(HEADER_FILE ${HEADER_FILES})
	list(APPEND LINPHONE_HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/linphone/${HEADER_FILE}")
endforeach()
set(LINPHONE_HEADER_FILES ${LINPHONE_HEADER_FILES} PARENT_SCOPE)

install(FILES ${LINPHONE_HEADER_FILES}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
