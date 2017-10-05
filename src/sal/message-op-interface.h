/*
 * message-op-interface.h
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

#ifndef _SAL_MESSAGE_OP_INTERFACE_H_
#define _SAL_MESSAGE_OP_INTERFACE_H_

LINPHONE_BEGIN_NAMESPACE

class SalMessageOpInterface {
public:
	virtual ~SalMessageOpInterface() = default;

	int send_message(const char *from, const char *to, const char *msg) {return send_message(from,to,"text/plain",msg, nullptr);}
	virtual int send_message(const char *from, const char *to, const char* content_type, const char *msg, const char *peer_uri) = 0;
	virtual int reply(SalReason reason) = 0;

protected:
	void prepare_message_request(belle_sip_request_t *req, const char* content_type, const char *msg, const char *peer_uri);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _SAL_MESSAGE_OP_INTERFACE_H_
