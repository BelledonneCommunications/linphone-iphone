/*
 * call-sessio-params-p.h
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

#ifndef _L_CALL_SESSION_PARAMS_P_H_
#define _L_CALL_SESSION_PARAMS_P_H_

#include <unordered_map>

#include "object/clonable-object-p.h"

#include "call-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSession;

class CallSessionParamsPrivate : public ClonableObjectPrivate {
public:
	void clone (const CallSessionParamsPrivate *src);

	bool getInConference () const { return inConference; }
	void setInConference (bool value) { inConference = value; }
	bool getInternalCallUpdate () const { return internalCallUpdate; }
	void setInternalCallUpdate (bool value) { internalCallUpdate = value; }
	bool getNoUserConsent () const { return noUserConsent; }
	void setNoUserConsent (bool value) { noUserConsent = value; }

	SalCustomHeader * getCustomHeaders () const;
	void setCustomHeaders (const SalCustomHeader *ch);

	const std::unordered_map<std::string, std::string> &getCustomContactParameters () const { return customContactParameters; }

	std::shared_ptr<CallSession> getReferer () const { return referer; }
	void setReferer (std::shared_ptr<CallSession> session) { referer = session; }

public:
	std::string sessionName;

	LinphonePrivacyMask privacy = LinphonePrivacyNone;

private:
	bool inConference = false;
	bool internalCallUpdate = false;
	bool noUserConsent = false; /* When set to true an UPDATE request will be used instead of reINVITE */
	SalCustomHeader *customHeaders = nullptr;
	std::unordered_map<std::string, std::string> customContactParameters;
	std::shared_ptr<CallSession> referer; /* In case call creation is consecutive to an incoming transfer, this points to the original call */

	L_DECLARE_PUBLIC(CallSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CALL_SESSION_PARAMS_P_H_
