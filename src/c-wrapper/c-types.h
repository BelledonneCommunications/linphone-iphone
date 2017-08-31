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

// Do not move this define.
// Enable C enums.
#define L_USE_C_ENUM

#include "event-log/event-log-enums.h"

#define L_DECLARE_C_ENUM(CLASS, ENUM, VALUES) enum Linphone ## CLASS ## ENUM { VALUES }
#define L_DECLARE_C_STRUCT(STRUCT) typedef struct _Linphone ## STRUCT Linphone ## STRUCT;

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif

// =============================================================================
// C Structures.
// =============================================================================

L_DECLARE_C_STRUCT(Call);
L_DECLARE_C_STRUCT(CallEvent);
L_DECLARE_C_STRUCT(ConferenceEvent);
L_DECLARE_C_STRUCT(ConferenceParticipantEvent);
L_DECLARE_C_STRUCT(EventLog);
L_DECLARE_C_STRUCT(Message);
L_DECLARE_C_STRUCT(MessageEvent);

// TODO: Remove me in the future.
typedef struct SalAddress LinphoneAddress;

// =============================================================================
// C Enums.
// =============================================================================

L_DECLARE_C_ENUM(EventLog, Type, L_ENUM_VALUES_EVENT_LOG_TYPE);

#ifdef __cplusplus
	}
#endif

#endif // ifndef _C_TYPES_H_
