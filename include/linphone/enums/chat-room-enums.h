/*
 * chat-room-enums.h
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

#ifndef _L_CHAT_ROOM_ENUMS_H_
#define _L_CHAT_ROOM_ENUMS_H_

// =============================================================================

#define L_ENUM_VALUES_CHAT_ROOM_STATE(F) \
	F(None /**< Initial state */) \
	F(Instantiated /**< Chat room is now instantiated on local */) \
	F(CreationPending /**< One creation request was sent to the server */) \
	F(Created /**< Chat room was created on the server */) \
	F(CreationFailed /**< Chat room creation failed */) \
	F(TerminationPending /**< Wait for chat room termination */) \
	F(Terminated /**< Chat room exists on server but not in local */) \
	F(TerminationFailed /**< The chat room termination failed */) \
	F(Deleted /**< Chat room was deleted on the server */)

#define L_ENUM_VALUES_CHAT_ROOM_CAPABILITIES(F) \
	F(Basic /**< No server. It&apos;s a direct communication */, 1 << 0) \
	F(RealTimeText /**< Supports RTT */, 1 << 1) \
	F(Conference /**< Use server (supports group chat) */, 1 << 2) \
	F(Proxy /**< Special proxy chat room flag */, 1 << 3) \
	F(Migratable /**< Chat room migratable from Basic to Conference */, 1 << 4) \
	F(OneToOne /**< A communication between two participants (can be Basic or Conference) */, 1 << 5)

#endif // ifndef _L_CHAT_ROOM_ENUMS_H_
