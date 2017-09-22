/***************************************************************************
 *            chat_file_transfer.c
 *
 *  Sun Jun  5 19:34:18 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphone/core.h"
#include "private.h"

#include "c-wrapper/c-wrapper.h"
#include "chat/chat-room.h"

LinphoneChatMessage *linphone_chat_room_create_file_transfer_message(LinphoneChatRoom *cr, const LinphoneContent *initial_content) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(cr)->createFileTransferMessage(initial_content);
}
