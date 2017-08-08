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

#include <string>

#include "object/clonable-object.h"

#include "linphone/types.h"
#include "sal/sal.h"

extern "C" {
	bool_t linphone_call_params_get_in_conference(const LinphoneCallParams *params);
	void linphone_call_params_set_in_conference(LinphoneCallParams *params, bool_t value);
	bool_t linphone_call_params_get_internal_call_update(const LinphoneCallParams *params);
	void linphone_call_params_set_internal_call_update(LinphoneCallParams *params, bool_t value);
	bool_t linphone_call_params_get_no_user_consent(const LinphoneCallParams *params);
	void linphone_call_params_set_no_user_consent(LinphoneCallParams *params, bool_t value);
	SalCustomHeader * linphone_call_params_get_custom_headers(const LinphoneCallParams *params);
	void linphone_call_params_set_custom_headers(LinphoneCallParams *params, const SalCustomHeader *ch);
	SalCustomSdpAttribute * linphone_call_params_get_custom_sdp_attributes(const LinphoneCallParams *params);
	void linphone_call_params_set_custom_sdp_attributes(LinphoneCallParams *params, const SalCustomSdpAttribute *csa);
	SalCustomSdpAttribute * linphone_call_params_get_custom_sdp_media_attributes(const LinphoneCallParams *params, LinphoneStreamType type);
	void linphone_call_params_set_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type, const SalCustomSdpAttribute *csa);
	LinphoneCall * linphone_call_params_get_referer(const LinphoneCallParams *params);
	void linphone_call_params_set_referer(LinphoneCallParams *params, LinphoneCall *referer);
}

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class CallSessionParamsPrivate;

class CallSessionParams : public ClonableObject {
	friend unsigned char ::linphone_call_params_get_in_conference(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_in_conference(LinphoneCallParams *params, unsigned char value);
	friend unsigned char ::linphone_call_params_get_internal_call_update(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_internal_call_update(LinphoneCallParams *params, unsigned char value);
	friend unsigned char ::linphone_call_params_get_no_user_consent(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_no_user_consent(LinphoneCallParams *params, unsigned char value);
	friend SalCustomHeader * ::linphone_call_params_get_custom_headers(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_custom_headers(LinphoneCallParams *params, const SalCustomHeader *ch);
	friend SalCustomSdpAttribute * ::linphone_call_params_get_custom_sdp_attributes(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_custom_sdp_attributes(LinphoneCallParams *params, const SalCustomSdpAttribute *csa);
	friend SalCustomSdpAttribute * ::linphone_call_params_get_custom_sdp_media_attributes(const LinphoneCallParams *params, LinphoneStreamType type);
	friend void ::linphone_call_params_set_custom_sdp_media_attributes(LinphoneCallParams *params, LinphoneStreamType type, const SalCustomSdpAttribute *csa);
	friend LinphoneCall * ::linphone_call_params_get_referer(const LinphoneCallParams *params);
	friend void ::linphone_call_params_set_referer(LinphoneCallParams *params, LinphoneCall *referer);

public:
	CallSessionParams ();
	CallSessionParams (const CallSessionParams &src);
	virtual ~CallSessionParams () = default;

	const std::string& getSessionName () const;
	void setSessionName (const std::string &sessionName);

	LinphonePrivacyMask getPrivacy () const;
	void setPrivacy (LinphonePrivacyMask privacy);

	void addCustomHeader (const std::string &headerName, const std::string &headerValue);
	void clearCustomHeaders ();
	const char * getCustomHeader (const std::string &headerName) const;

	void addCustomSdpAttribute (const std::string &attributeName, const std::string &attributeValue);
	void clearCustomSdpAttributes ();
	const char * getCustomSdpAttribute (const std::string &attributeName) const;

	void addCustomSdpMediaAttribute (LinphoneStreamType lst, const std::string &attributeName, const std::string &attributeValue);
	void clearCustomSdpMediaAttributes (LinphoneStreamType lst);
	const char * getCustomSdpMediaAttribute (LinphoneStreamType lst, const std::string &attributeName) const;

protected:
	explicit CallSessionParams (CallSessionParamsPrivate &p);

private:
	L_DECLARE_PRIVATE(CallSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _CALL_SESSION_PARAMS_H_
