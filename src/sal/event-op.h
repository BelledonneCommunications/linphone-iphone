/*
 * event-op.h
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

#ifndef _L_SAL_EVENT_OP_H_
#define _L_SAL_EVENT_OP_H_

#include "sal/op.h"

LINPHONE_BEGIN_NAMESPACE

class SalEventOp : public SalOp {
public:
	SalEventOp (Sal *sal) : SalOp(sal) {}
};

class SalSubscribeOp: public SalEventOp {
public:
	SalSubscribeOp (Sal *sal): SalEventOp(sal) {}

	int subscribe (const char *from, const char *to, const char *eventName, int expires, const SalBodyHandler *bodyHandler);
	int unsubscribe () { return SalOp::unsubscribe(); }
	int accept ();
	int decline (SalReason reason);
	int notifyPendingState ();
	int notify (const SalBodyHandler *bodyHandler);
	int closeNotify ();

private:
	virtual void fillCallbacks () override;
	void handleNotify (belle_sip_request_t *request, const char *eventName, SalBodyHandler *bodyHandler);

	static void subscribeProcessIoErrorCb (void *userCtx, const belle_sip_io_error_event_t *event);
	static void subscribeResponseEventCb (void *userCtx, const belle_sip_response_event_t *event);
	static void subscribeProcessTimeoutCb (void *userCtx, const belle_sip_timeout_event_t *event);
	static void subscribeProcessTransactionTerminatedCb (void *userCtx, const belle_sip_transaction_terminated_event_t *event) {}
	static void subscribeProcessRequestEventCb (void *userCtx, const belle_sip_request_event_t *event);
	static void subscribeProcessDialogTerminatedCb (void *userCtx, const belle_sip_dialog_terminated_event_t *event);
	static void releaseCb (SalOp *op);
	static void subscribeRefresherListenerCb (belle_sip_refresher_t *refresher, void *userCtx, unsigned int statusCode, const char *reasonPhrase, int willRetry);
};

class SalPublishOp : public SalEventOp {
public:
	SalPublishOp (Sal *sal) : SalEventOp(sal) {}

	int publish (const char *from, const char *to, const char *eventName, int expires, const SalBodyHandler *bodyHandler);
	int unpublish ();

private:
	virtual void fillCallbacks () override;

	static void publishResponseEventCb (void *userCtx, const belle_sip_response_event_t *event);
	static void publishRefresherListenerCb (belle_sip_refresher_t *refresher, void *userCtx, unsigned int statusCode, const char *reasonPhrase, int willRetry);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_EVENT_OP_H_
