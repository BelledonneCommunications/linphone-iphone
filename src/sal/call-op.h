/*
 * call-op.h
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

#ifndef _L_SAL_CALL_OP_H_
#define _L_SAL_CALL_OP_H_

#include "sal/op.h"
#include "sal/message-op-interface.h"

LINPHONE_BEGIN_NAMESPACE

class SalCallOp: public SalOp, public SalMessageOpInterface {
public:
	SalCallOp(Sal *sal): SalOp(sal) {}
	~SalCallOp() override;

	SalMediaDescription *get_local_media_description () const { return local_media; }
	int set_local_media_description(SalMediaDescription *desc);
	int set_local_body(const Content &body);
	int set_local_body(const Content &&body);

	SalMediaDescription *get_remote_media_description() {return this->remote_media;}
	const Content &get_remote_body() const {return this->remote_body;}
	SalMediaDescription *get_final_media_description();

	int call(const char *from, const char *to, const char *subject);
	int notify_ringing(bool_t early_media);
	int accept();
	int decline(SalReason reason, const char *redirection /*optional*/);
	int decline_with_error_info(const SalErrorInfo *info, const SalAddress *redirectionAddr /*optional*/);
	int update(const char *subject, bool_t no_user_consent);
	int cancel_invite() { return cancel_invite_with_info(NULL);}
	int cancel_invite_with_info(const SalErrorInfo *info);
	int refer(const char *refer_to_);
	int refer_with_replaces(SalCallOp *other_call_op);
	int set_referer(SalCallOp *refered_call);
	SalCallOp *get_replaces();
	int send_dtmf(char dtmf);
	int terminate() {return terminate_with_error(NULL);}
	int terminate_with_error(const SalErrorInfo *info);
	bool_t autoanswer_asked() const {return this->auto_answer_asked;}
	void send_vfu_request();
	int is_offerer() const {return this->sdp_offering;}
	int notify_refer_state(SalCallOp *newcall);
	bool_t compare_op(const SalCallOp *op2) const;
	bool_t dialog_request_pending() const {return (belle_sip_dialog_request_pending(this->dialog) != 0);}
	const char *get_local_tag() {return belle_sip_dialog_get_local_tag(this->dialog);}
	const char *get_remote_tag() {return belle_sip_dialog_get_remote_tag(this->dialog);}
	void set_replaces(const char *call_id, const char *from_tag, const char *to_tag);
	void set_sdp_handling(SalOpSDPHandling handling);

	// Implementation of SalMessageOpInterface
	int sendMessage (const Content &content) override;
	int reply(SalReason reason) override {return SalOp::reply_message(reason);}

private:
	virtual void fill_cbs() override;
	void set_released();

	void set_error(belle_sip_response_t* response, bool_t fatal);
	void call_terminated(belle_sip_server_transaction_t* server_transaction, int status_code, belle_sip_request_t* cancel_request);
	void reset_descriptions();

	int parse_sdp_body(const Content &body,belle_sdp_session_description_t** session_desc, SalReason *error);
	void sdp_process();
	void handle_body_from_response(belle_sip_response_t* response);
	SalReason process_body_for_invite(belle_sip_request_t* invite);
	SalReason process_body_for_ack(belle_sip_request_t *ack);
	void handle_offer_answer_response(belle_sip_response_t* response);

	void fill_invite(belle_sip_request_t* invite);
	void cancelling_invite(const SalErrorInfo *info);
	int refer_to(belle_sip_header_refer_to_t* refer_to, belle_sip_header_referred_by_t* referred_by);
	int send_notify_for_refer(int code, const char *reason);
	void notify_last_response(SalCallOp *newcall);
	void process_refer(const belle_sip_request_event_t *event, belle_sip_server_transaction_t *server_transaction);
	void process_notify(const belle_sip_request_event_t *event, belle_sip_server_transaction_t* server_transaction);

	static void set_addr_to_0000(char value[], size_t sz);
	static int is_media_description_acceptable(SalMediaDescription *md);
	static bool_t is_a_pending_invite_incoming_transaction(belle_sip_transaction_t *tr);
	static void set_call_as_released(SalCallOp *op);
	static void unsupported_method(belle_sip_server_transaction_t* server_transaction,belle_sip_request_t* request);
	static belle_sip_header_reason_t *make_reason_header( const SalErrorInfo *info);
	static belle_sip_header_allow_t *create_allow(bool_t enable_update);
	static std::vector<char> marshal_media_description(belle_sdp_session_description_t *session_desc, belle_sip_error_code &error);

	// belle_sip_message handlers
	static int set_custom_body(belle_sip_message_t *msg, const Content &body);
	static int set_sdp(belle_sip_message_t *msg,belle_sdp_session_description_t* session_desc);
	static int set_sdp_from_desc(belle_sip_message_t *msg, const SalMediaDescription *desc);
	static void process_io_error_cb(void *user_ctx, const belle_sip_io_error_event_t *event);
	static Content extract_body(belle_sip_message_t *message);

	// Callbacks
	static int vfu_retry_cb (void *user_data, unsigned int events);
	static void process_response_cb(void *op_base, const belle_sip_response_event_t *event);
	static void process_timeout_cb(void *user_ctx, const belle_sip_timeout_event_t *event);
	static void process_transaction_terminated_cb(void *user_ctx, const belle_sip_transaction_terminated_event_t *event);
	static void process_request_event_cb(void *op_base, const belle_sip_request_event_t *event);
	static void process_dialog_terminated_cb(void *ctx, const belle_sip_dialog_terminated_event_t *event);

	// Private constants
	static const size_t SIP_MESSAGE_BODY_LIMIT = 16*1024; // 16kB

	// Attributes
	SalMediaDescription *local_media = NULL;
	SalMediaDescription *remote_media = NULL;
	Content local_body;
	Content remote_body;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_CALL_OP_H_
