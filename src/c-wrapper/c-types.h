/*
 * c-types.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _C_TYPES_H_
#define _C_TYPES_H_

// Do not move these defines.
#define L_DECLARE_ENUM(CLASS, ENUM) enum Linphone ## CLASS ## ENUM
#define L_DECLARE_C_STRUCT(STRUCT) typedef struct _Linphone ## STRUCT Linphone ## STRUCT;

#include "event-log/event-log-enums.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif

L_DECLARE_C_STRUCT(Call);
L_DECLARE_C_STRUCT(CallEvent);
L_DECLARE_C_STRUCT(ConferenceEvent);
L_DECLARE_C_STRUCT(ConferenceParticipantEvent);
L_DECLARE_C_STRUCT(EventLog);
L_DECLARE_C_STRUCT(Message);
L_DECLARE_C_STRUCT(MessageEvent);

// TODO: Remove me in the future.
typedef struct SalAddress LinphoneAddress;

#ifdef __cplusplus
	}
#endif

#endif // ifndef _C_TYPES_H_
