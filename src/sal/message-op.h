/*
 * message-op.h
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

#ifndef _L_SAL_MESSAGE_OP_H_
#define _L_SAL_MESSAGE_OP_H_

#include "sal/op.h"
#include "sal/message-op-interface.h"

LINPHONE_BEGIN_NAMESPACE

class SalMessageOp : public SalOp, public SalMessageOpInterface {
public:
	SalMessageOp (Sal *sal) : SalOp(sal) {}

	int sendMessage (const Content &content) override;
	int reply (SalReason reason) override { return SalOp::replyMessage(reason); }

private:
	virtual void fillCallbacks () override;
	void processError ();

	static void processIoErrorCb (void *userCtx, const belle_sip_io_error_event_t *event);
	static void processResponseEventCb (void *userCtx, const belle_sip_response_event_t *event);
	static void processTimeoutCb (void *userCtx, const belle_sip_timeout_event_t *event);
	static void processRequestEventCb (void *userCtx, const belle_sip_request_event_t *event);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_MESSAGE_OP_H_
