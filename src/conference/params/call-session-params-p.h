/*
 * call-session-params-p.h
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

#ifndef _CALL_SESSION_PARAMS_P_H_
#define _CALL_SESSION_PARAMS_P_H_

#include "object/clonable-object-p.h"

#include "call-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionParamsPrivate : public ClonableObjectPrivate {
public:
	CallSessionParamsPrivate () = default;
	CallSessionParamsPrivate (const CallSessionParamsPrivate &src);
	virtual ~CallSessionParamsPrivate ();

	CallSessionParamsPrivate &operator= (const CallSessionParamsPrivate &src);

	static void clone (const CallSessionParamsPrivate &src, CallSessionParamsPrivate &dst);

	bool getInConference () const { return inConference; }
	void setInConference (bool value) { inConference = value; }
	bool getInternalCallUpdate () const { return internalCallUpdate; }
	void setInternalCallUpdate (bool value) { internalCallUpdate = value; }
	bool getNoUserConsent () const { return noUserConsent; }
	void setNoUserConsent (bool value) { noUserConsent = value; }

	SalCustomHeader * getCustomHeaders () const;
	void setCustomHeaders (const SalCustomHeader *ch);

	LinphoneCall *getReferer () const { return referer; }
	void setReferer (LinphoneCall *call) { referer = call; }

public:
	std::string sessionName;

	LinphonePrivacyMask privacy = LinphonePrivacyNone;

private:
	bool inConference = false;
	bool internalCallUpdate = false;
	bool noUserConsent = false; /* When set to true an UPDATE request will be used instead of reINVITE */
	SalCustomHeader *customHeaders = nullptr;
	LinphoneCall *referer = nullptr; /* In case call creation is consecutive to an incoming transfer, this points to the original call */

	L_DECLARE_PUBLIC(CallSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_PARAMS_P_H_
