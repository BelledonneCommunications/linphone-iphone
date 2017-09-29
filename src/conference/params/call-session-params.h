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

#ifndef _CALL_SESSION_PARAMS_H_
#define _CALL_SESSION_PARAMS_H_

#include "object/clonable-object.h"

#include "linphone/types.h"
#include "sal/sal.h"
#include "sal/sal.hpp"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;
class CallSessionParamsPrivate;
class CallSessionPrivate;

class CallSessionParams : public ClonableObject {
	friend class CallSession;
	friend class CallSessionPrivate;

public:
	CallSessionParams ();
	CallSessionParams (const CallSessionParams &src);
	virtual ~CallSessionParams () = default;

	CallSessionParams &operator= (const CallSessionParams &src);

	virtual void initDefault (LinphoneCore *core);

	const std::string& getSessionName () const;
	void setSessionName (const std::string &sessionName);

	LinphonePrivacyMask getPrivacy () const;
	void setPrivacy (LinphonePrivacyMask privacy);

	void addCustomHeader (const std::string &headerName, const std::string &headerValue);
	void clearCustomHeaders ();
	const char * getCustomHeader (const std::string &headerName) const;

protected:
	explicit CallSessionParams (CallSessionParamsPrivate &p);

private:
	L_DECLARE_PRIVATE(CallSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_PARAMS_H_
