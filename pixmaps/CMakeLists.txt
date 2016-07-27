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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

set(ICONS_INSTALL_DIR ${PACKAGE_DATA_DIR}/icons/hicolor)

install(FILES "linphone.icns" "linphone-banner.png"
	DESTINATION ${PACKAGE_DATA_DIR}/pixmaps/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

install(FILES
	svg/linphone-micro-muted.svg
	svg/linphone-speaker-muted.svg
	svg/linphone-micro-enabled.svg
	svg/linphone-speaker-enabled.svg
	svg/linphone-status-online.svg
	svg/linphone-status-away.svg
	svg/linphone-status-donotdisturb.svg
	svg/linphone-status-offline.svg
	svg/linphone-call-status-incoming.svg
	svg/linphone-call-status-missed.svg
	svg/linphone-call-status-outgoing.svg
	svg/linphone-chat-new-message-and-writing.svg
	svg/linphone-chat-new-message.svg
	svg/linphone-chat-nothing.svg
	svg/linphone-chat-writing.svg
	svg/linphone-ok.svg
	svg/linphone-inprogress.svg
	svg/linphone-failed.svg
	svg/linphone-camera-enabled.svg
	svg/linphone-camera-disabled.svg
	svg/linphone-security-ok.svg
	svg/linphone-security-pending.svg
	svg/linphone-media-play.svg
	svg/linphone-media-pause.svg
	svg/linphone-warning.svg
	DESTINATION ${ICONS_INSTALL_DIR}/scalable/status
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES
	svg/linphone-start-call.svg
	svg/linphone-add-call.svg
	svg/linphone-hold-off.svg
	svg/linphone-hold-on.svg
	svg/linphone-start-call2.svg
	svg/linphone-start-chat.svg
	svg/linphone-history.svg
	svg/linphone-edit.svg
	svg/linphone-delete.svg
	svg/linphone-contact-add.svg
	svg/linphone-conference-start.svg
	svg/linphone-call-transfer.svg
	svg/linphone-record.svg
	svg/linphone-chat-send.svg
	DESTINATION ${ICONS_INSTALL_DIR}/scalable/actions
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES
	linphone-micro-muted.png
	linphone-speaker-muted.png
	linphone-micro-enabled.png
	linphone-speaker-enabled.png
	linphone-status-online.png
	linphone-status-away.png
	linphone-status-donotdisturb.png
	linphone-status-offline.png
	linphone-chat-nothing.png
	linphone-chat-new-message.png
	linphone-chat-writing.png
	linphone-chat-new-message-and-writing.png
	linphone-call-status-incoming.png
	linphone-call-status-outgoing.png
	linphone-call-status-missed.png
	linphone-ok.png
	linphone-inprogress.png
	linphone-failed.png
	linphone-camera-enabled.png
	linphone-camera-disabled.png
	linphone-security-ok.png
	linphone-security-pending.png
	linphone-media-play.png
	linphone-media-pause.png
	linphone-warning.png
	DESTINATION ${ICONS_INSTALL_DIR}/48x48/status
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES
	linphone-start-call2.png
	linphone-add-call.png
	linphone-start-call.png
	linphone-start-chat.png
	linphone-stop-call.png
	linphone-hold-on.png
	linphone-hold-off.png
	linphone-history.png
	linphone-edit.png
	linphone-delete.png
	linphone-contact-add.png
	linphone-conference-start.png
	linphone-call-transfer.png
	linphone-record.png
	linphone-chat-send.png
	linphone-take-screenshot.png
	DESTINATION ${ICONS_INSTALL_DIR}/48x48/actions
	PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
)

install(FILES linphone.png
	DESTINATION ${ICONS_INSTALL_DIR}/48x48/apps
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

if(WIN32)
	install(FILES index.theme
		DESTINATION ${ICONS_INSTALL_DIR}
		PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	)
endif()
