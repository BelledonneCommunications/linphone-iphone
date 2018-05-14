/*
 * chat-message-enums.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _L_CHAT_MESSAGE_ENUMS_H_
#define _L_CHAT_MESSAGE_ENUMS_H_

// =============================================================================

#define L_ENUM_VALUES_CHAT_MESSAGE_STATE(F) \
	F(Idle /**< Initial state */) \
	F(InProgress /**< Delivery in progress */) \
	F(Delivered /**< Message successfully delivered and acknowledged by the server */) \
	F(NotDelivered /**< Message was not delivered */) \
	F(FileTransferError /**< Message was received and acknowledged but cannot get file from server */) \
	F(FileTransferDone /**< File transfer has been completed successfully */) \
	F(DeliveredToUser /**< Message successfully delivered an acknowledged by the remote user */) \
	F(Displayed /**< Message successfully displayed to the remote user */)

#define L_ENUM_VALUES_CHAT_MESSAGE_DIRECTION(F) \
	F(Incoming /**< Incoming message */) \
	F(Outgoing /**< Outgoing message */)

#endif // ifndef _L_CHAT_MESSAGE_ENUMS_H_
