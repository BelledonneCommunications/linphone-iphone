/*
 * event-log-enums.h
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

#ifndef _EVENT_LOG_ENUMS_H_
#define _EVENT_LOG_ENUMS_H_

#include "utils/enum-generator.h"

// =============================================================================

#define L_ENUM_VALUES_EVENT_LOG_TYPE \
	L_DECLARE_ENUM_VALUES(EventLog, Type, \
		None, \
		Message, \
		CallStart, \
		CallEnd, \
		ConferenceCreated, \
		ConferenceDestroyed, \
		ConferenceParticipantAdded, \
		ConferenceParticipantRemoved, \
		ConferenceParticipantSetAdmin, \
		ConferenceParticipantUnsetAdmin \
	)

#endif // ifndef _EVENT_LOG_ENUMS_H_
