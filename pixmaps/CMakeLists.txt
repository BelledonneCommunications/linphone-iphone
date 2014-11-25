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

set(PIXMAPS
	active_chat.png
	addcall-green.png
	call.png
	call_status_incoming.png
	call_status_outgoing.png
	chat_message_delivered.png
	chat_message_inprogress.png
	chat_message_not_delivered.png
	chat.png
	composing_active_chat.png
	composing_chat.png
	contact-orange.png
	contact_starred.png
	contact_unstarred.png
	dialer-orange.png
	dialer.png
	history-orange.png
	hold_off.png
	hold_on.png
	linphone-banner.png
	linphone.icns
	linphone.png
	mic_active.png
	mic_muted.png
	notok.png
	ok.png
	speaker.png
	startcall-green.png
	startcall-small.png
	status-green.png
	status-offline.png
	status-orange.png
	status-red.png
	stopcall-red.png
	stopcall-small.png
)

install(FILES ${PIXMAPS}
	DESTINATION ${PACKAGE_DATA_DIR}/pixmaps/linphone
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
