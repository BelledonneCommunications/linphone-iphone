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
	F(None) \
	F(Instantiated) \
	F(CreationPending) \
	F(Created) \
	F(TerminationPending) \
	F(Terminated) \
	F(CreationFailed) \
	F(Deleted)

#define L_ENUM_VALUES_CHAT_ROOM_CAPABILITIES(F) \
	F(Basic, 1 << 0) \
	F(RealTimeText, 1 << 1) \
	F(Conference, 1 << 2) \
	F(Proxy, 1 << 3) \
	F(Migratable, 1 << 4) \
	F(OneToOne, 1 << 5)

#endif // ifndef _L_CHAT_ROOM_ENUMS_H_
