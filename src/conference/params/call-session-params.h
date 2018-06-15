/*
 * call-session-params.h
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

#ifndef _L_CALL_SESSION_PARAMS_H_
#define _L_CALL_SESSION_PARAMS_H_

#include "object/clonable-object.h"

#include "linphone/types.h"
#include "c-wrapper/internal/c-sal.h"
#include "sal/sal.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionParamsPrivate;
class Core;

class LINPHONE_PUBLIC CallSessionParams : public ClonableObject {
	friend class CallSession;
	friend class CallSessionPrivate;
	friend class ClientGroupChatRoom;

public:
	CallSessionParams ();
	CallSessionParams (const CallSessionParams &other);
	virtual ~CallSessionParams ();

	CallSessionParams &operator= (const CallSessionParams &other);

	virtual void initDefault (const std::shared_ptr<Core> &core);

	const std::string& getSessionName () const;
	void setSessionName (const std::string &sessionName);

	LinphonePrivacyMask getPrivacy () const;
	void setPrivacy (LinphonePrivacyMask privacy);

	void addCustomHeader (const std::string &headerName, const std::string &headerValue);
	void clearCustomHeaders ();
	const char * getCustomHeader (const std::string &headerName) const;

	void addCustomContactParameter (const std::string &paramName, const std::string &paramValue = "");
	void clearCustomContactParameters ();
	std::string getCustomContactParameter (const std::string &paramName) const;

protected:
	explicit CallSessionParams (CallSessionParamsPrivate &p);

private:
	L_DECLARE_PRIVATE(CallSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_CALL_SESSION_PARAMS_H_
