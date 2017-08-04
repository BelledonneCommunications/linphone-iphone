/*
 * call-session-params.h
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

#ifndef _CONFERENCE_CALL_SESSION_PARAMS_H_
#define _CONFERENCE_CALL_SESSION_PARAMS_H_

#include "object/object.h"

// =============================================================================

namespace LinphonePrivate {
	namespace Conference {
		class CallSessionParamsPrivate;

		class CallSessionParams : public Object {
		public:
			CallSessionParams ();

		protected:
			explicit CallSessionParams (CallSessionParamsPrivate &p);

		private:
			L_DECLARE_PRIVATE(CallSessionParams);
			L_DISABLE_COPY(CallSessionParams);
		};
	}
}

#endif // ifndef _CONFERENCE_CALL_SESSION_PARAMS_H_
