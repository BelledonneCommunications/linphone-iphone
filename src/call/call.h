/*
 * call.h
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

#ifndef _CALL_CALL_H_
#define _CALL_CALL_H_

#include "linphone/types.h"

#include "object/object.h"

// =============================================================================

namespace LinphonePrivate {
	namespace Call {
		class CallPrivate;

		class Call : public Object {
		public:
			Call (LinphoneCallDir direction);

			LinphoneCallDir getDirection() const;
			LinphoneCallState getState() const;

		private:
			L_DECLARE_PRIVATE(Call);
			L_DISABLE_COPY(Call);
		};
	}
}

#endif // ifndef _CALL_CALL_H_
