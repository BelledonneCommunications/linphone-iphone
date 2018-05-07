/*
 * sal.h
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

#ifndef _L_SAL_H_
#define _L_SAL_H_

#include "linphone/utils/general.h"

#include "c-wrapper/internal/c-sal.h"
#include "logger/logger.h"

LINPHONE_BEGIN_NAMESPACE

class SalOp;
class SalCallOp;
class SalMessageOp;
class SalSubscribeOp;
class SalPresenceOp;
class SalReferOp;

class Sal{
public:
	using OnCallReceivedCb = void (*) (SalCallOp *op);
	using OnCallRingingCb = void (*) (SalOp *op);
	using OnCallAcceptedCb = void (*) (SalOp *op);
	using OnCallAckReceivedCb = void (*) (SalOp *op, SalCustomHeader *ack);
	using OnCallAckBeingSentCb = void (*) (SalOp *op, SalCustomHeader *ack);
	using OnCallUpdatingCb = void (*) (SalOp *op, bool_t isUpdate); // Called when a reINVITE/UPDATE is received
	using OnCallTerminatedCb = void (*) (SalOp *op, const char *from);
	using OnCallFailureCb = void (*) (SalOp *op);
	using OnCallReleasedCb = void (*) (SalOp *op);
	using OnCallCancelDoneCb = void (*) (SalOp *op);
	using OnAuthRequestedLegacyCb = void (*) (SalOp *op, const char *realm, const char *username);
	using OnAuthRequestedCb = bool_t (*) (Sal *sal, SalAuthInfo *info);
	using OnAuthFailureCb = void (*) (SalOp *op, SalAuthInfo *info);
	using OnRegisterSuccessCb = void (*) (SalOp *op, bool_t registered);
	using OnRegisterFailureCb = void (*) (SalOp *op);
	using OnVfuRequestCb = void (*) (SalOp *op);
	using OnDtmfReceivedCb = void (*) (SalOp *op, char dtmf);
	using OnCallReferCb = void (*) (SalOp *op, const SalAddress *referTo);
	using OnReferCb = void (*) (SalOp *op, const SalAddress *referTo);
	using OnMessageReceivedCb = void (*) (SalOp *op, const SalMessage *msg);
	using OnMessageDeliveryUpdateCb = void (*) (SalOp *op, SalMessageDeliveryStatus status);
	using OnNotifyReferCb = void (*) (SalOp *op, SalReferStatus status);
	using OnSubscribeResponseCb = void (*) (SalOp *op, SalSubscribeStatus status, int willRetry);
	using OnNotifyCb = void (*) (SalSubscribeOp *op, SalSubscribeStatus status, const char *event, SalBodyHandler *body);
	using OnSubscribeReceivedCb = void (*) (SalSubscribeOp *op, const char *event, const SalBodyHandler *body);
	using OnIncomingSubscribeClosedCb = void (*) (SalOp *op);
	using OnParsePresenceRequestedCb = void (*) (SalOp *op, const char *contentType, const char *contentSubtype, const char *content, SalPresenceModel **result);
	using OnConvertPresenceToXMLRequestedCb = void (*) (SalOp *op, SalPresenceModel *presence, const char *contact, char **content);
	using OnNotifyPresenceCb = void (*) (SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model, const char *msg);
	using OnSubscribePresenceReceivedCb = void (*) (SalPresenceOp *op, const char *from);
	using OnSubscribePresenceClosedCb = void (*) (SalPresenceOp *op, const char *from);
	using OnPingReplyCb = void (*) (SalOp *op);
	using OnInfoReceivedCb = void (*) (SalOp *op, SalBodyHandler *body);
	using OnPublishResponseCb = void (*) (SalOp *op);
	using OnNotifyResponseCb = void (*) (SalOp *op);
	using OnExpireCb = void (*) (SalOp *op);

	struct Callbacks {
		OnCallReceivedCb call_received;
		OnCallReceivedCb call_rejected;
		OnCallRingingCb call_ringing;
		OnCallAcceptedCb call_accepted;
		OnCallAckReceivedCb call_ack_received;
		OnCallAckBeingSentCb call_ack_being_sent;
		OnCallUpdatingCb call_updating;
		OnCallTerminatedCb call_terminated;
		OnCallFailureCb call_failure;
		OnCallReleasedCb call_released;
		OnCallCancelDoneCb call_cancel_done;
		OnCallReferCb call_refer_received;
		OnAuthFailureCb auth_failure;
		OnRegisterSuccessCb register_success;
		OnRegisterFailureCb register_failure;
		OnVfuRequestCb vfu_request;
		OnDtmfReceivedCb dtmf_received;
		
		OnMessageReceivedCb message_received;
		OnMessageDeliveryUpdateCb message_delivery_update;
		OnNotifyReferCb notify_refer;
		OnSubscribeReceivedCb subscribe_received;
		OnIncomingSubscribeClosedCb incoming_subscribe_closed;
		OnSubscribeResponseCb subscribe_response;
		OnNotifyCb notify;
		OnSubscribePresenceReceivedCb subscribe_presence_received;
		OnSubscribePresenceClosedCb subscribe_presence_closed;
		OnParsePresenceRequestedCb parse_presence_requested;
		OnConvertPresenceToXMLRequestedCb convert_presence_to_xml_requested;
		OnNotifyPresenceCb notify_presence;
		OnPingReplyCb ping_reply;
		OnAuthRequestedCb auth_requested;
		OnInfoReceivedCb info_received;
		OnPublishResponseCb on_publish_response;
		OnExpireCb on_expire;
		OnNotifyResponseCb on_notify_response;
		OnReferCb refer_received; // For out of dialog refer
	};

	Sal(MSFactory *factory);
	~Sal();

	void setFactory (MSFactory *value) { mFactory = value; }

	void setUserPointer (void *value) { mUserPointer = value; }
	void *getUserPointer () const { return mUserPointer; }

	void setCallbacks (const Callbacks *cbs);

	void *getStackImpl() const { return mStack; }

	int iterate () { belle_sip_stack_sleep(mStack, 0); return 0; }

	void setSendError (int value) { belle_sip_stack_set_send_error(mStack, value); }
	void setRecvError (int value) { belle_sip_provider_set_recv_error(mProvider, value); }


	// ---------------------------------------------------------------------------
	// SIP parameters
	// ---------------------------------------------------------------------------
	void setSupportedTags (const char *tags);
	const char *getSupportedTags () const {
		return mSupported ? belle_sip_header_get_unparsed_value(mSupported) : nullptr;
	}
	void addSupportedTag (const char *tag);
	void removeSupportedTag (const char *tag);

	void setUserAgent (const char *userAgent);
	const char *getUserAgent() const;
	void appendStackStringToUserAgent ();

	bool_t isContentEncodingAvailable (const char *contentEncoding) {
		return (bool_t)belle_sip_stack_content_encoding_available(mStack, contentEncoding);
	}
	bool_t isContentTypeSupported (const char *contentType) const;
	void addContentTypeSupport (const char *contentType);
	void removeContentTypeSupport (const char *contentType);

	void setDefaultSdpHandling (SalOpSDPHandling sdpHandlingMethod);

	void setUuid (const char *uuid);
	int createUuid (char *uuid, size_t len);
	static int generateUuid (char *uuid, size_t len);

	void enableNatHelper (bool_t enable);
	bool_t natHelperEnabled () const { return mNatHelperEnabled; }

	bool_t pendingTransactionCheckingEnabled () const { return mPendingTransactionChecking; }
	int enablePendingTransactionChecking (bool_t value) { mPendingTransactionChecking = value; return 0; }

	void setRefresherRetryAfter (int value) { mRefresherRetryAfter = value; }
	int getRefresherRetryAfter () const { return mRefresherRetryAfter; }

	void enableSipUpdateMethod (bool_t value) { mEnableSipUpdate = value; }
	void useSessionTimers (int expires) { mSessionExpires = expires; }
	void useDates (bool_t value) { mUseDates = value; }
	void useOneMatchingCodecPolicy (bool_t value) { mOneMatchingCodec = value; }
	void useRport (bool_t value);
	void enableAutoContacts (bool_t value) { mAutoContacts = value; }
	void enableTestFeatures (bool_t value) { mEnableTestFeatures = value; }
	void useNoInitialRoute (bool_t value) { mNoInitialRoute = value; }
	void enableUnconditionalAnswer (int value) { belle_sip_provider_enable_unconditional_answer(mProvider, value); }
	void enableReconnectToPrimaryAsap (bool_t value) { belle_sip_stack_enable_reconnect_to_primary_asap(mStack, value); }

	bctbx_list_t *getPendingAuths () const { return bctbx_list_copy(mPendingAuths); }

	void setContactLinphoneSpecs (const char *specs);


	// ---------------------------------------------------------------------------
	// Network parameters
	// ---------------------------------------------------------------------------
	int setListenPort (const char *addr, int port, SalTransport tr, bool_t isTunneled);
	int getListeningPort (SalTransport tr);
	int isTransportAvailable (SalTransport t);

	void getDefaultLocalIp (int addressFamily, char *ip, size_t ipLen);

	void setTransportTimeout (int value) { belle_sip_stack_set_transport_timeout(mStack, value); }
	int getTransportTimeout () const { return belle_sip_stack_get_transport_timeout(mStack); }

	void setKeepAlivePeriod (unsigned int value);
	unsigned int getKeepAlivePeriod () const { return mKeepAlive; }
	void useTcpTlsKeepAlive (bool_t value) { mUseTcpTlsKeepAlive = value; }

	void setDscp (int dscp) { belle_sip_stack_set_default_dscp(mStack, dscp); }

	int setTunnel (void *tunnelClient);

	void setHttpProxyHost (const char *value) { belle_sip_stack_set_http_proxy_host(mStack, value); }
	const char *getHttpProxyHost () const { return belle_sip_stack_get_http_proxy_host(mStack); }

	void setHttpProxyPort (int value) { belle_sip_stack_set_http_proxy_port(mStack, value); }
	int getHttpProxyPort () const { return belle_sip_stack_get_http_proxy_port(mStack); }

	ortp_socket_t getSocket () const;

	int unlistenPorts ();
	int resetTransports ();


	// ---------------------------------------------------------------------------
	// TLS parameters
	// ---------------------------------------------------------------------------
	void setSslConfig (void *sslConfig);
	void setRootCa (const char *value);
	void setRootCaData (const char *data);
	const char *getRootCa () const { return mRootCa; }

	void verifyServerCertificates (bool_t value);
	void verifyServerCn (bool_t value);


	// ---------------------------------------------------------------------------
	// DNS resolution
	// ---------------------------------------------------------------------------
	void setDnsTimeout (int value) { belle_sip_stack_set_dns_timeout(mStack, value); }
	int getDnsTimeout () const { return belle_sip_stack_get_dns_timeout(mStack); }

	void setDnsServers (const bctbx_list_t *servers);

	void enableDnsSearch (bool_t value) { belle_sip_stack_enable_dns_search(mStack, (unsigned char)value); }
	bool_t dnsSearchEnabled () const { return (bool_t)belle_sip_stack_dns_search_enabled(mStack); }

	void enableDnsSrv (bool_t value) { belle_sip_stack_enable_dns_srv(mStack, (unsigned char)value); }
	bool_t dnsSrvEnabled () const { return (bool_t)belle_sip_stack_dns_srv_enabled(mStack); }

	void setDnsUserHostsFile (const char *value) { belle_sip_stack_set_dns_user_hosts_file(mStack, value); }
	const char *getDnsUserHostsFile () const { return belle_sip_stack_get_dns_user_hosts_file(mStack); }

	belle_sip_resolver_context_t *resolveA (const char *name, int port, int family, belle_sip_resolver_callback_t cb, void *data) {
		return belle_sip_stack_resolve_a(mStack, name, port, family, cb, data);
	}
	belle_sip_resolver_context_t *resolve (const char *service, const char *transport, const char *name, int port, int family, belle_sip_resolver_callback_t cb, void *data) {
		return belle_sip_stack_resolve(mStack, service, transport, name, port, family, cb, data);
	}


	// ---------------------------------------------------------------------------
	// Timers
	// ---------------------------------------------------------------------------
	belle_sip_source_t *createTimer (belle_sip_source_func_t func, void *data, unsigned int timeoutValueMs, const char *timerName);
	void cancelTimer (belle_sip_source_t *timer);


private:
	struct sal_uuid_t {
		unsigned int time_low;
		unsigned short time_mid;
		unsigned short time_hi_and_version;
		unsigned char clock_seq_hi_and_reserved;
		unsigned char clock_seq_low;
		unsigned char node[6];
	};

	void setTlsProperties ();
	int addListenPort (SalAddress *addr, bool_t isTunneled);
	void makeSupportedHeader ();
	void addPendingAuth (SalOp *op);
	void removePendingAuth (SalOp *op);
	belle_sip_response_t *createResponseFromRequest (belle_sip_request_t *req, int code);

	static void unimplementedStub() { lWarning() << "Unimplemented SAL callback"; }
	static void removeListeningPoint (belle_sip_listening_point_t *lp,belle_sip_provider_t *prov) {
		belle_sip_provider_remove_listening_point(prov, lp);
	}

	// Internal callbacks
	static void processDialogTerminatedCb (void *userCtx, const belle_sip_dialog_terminated_event_t *event);
	static void processIoErrorCb (void *userCtx, const belle_sip_io_error_event_t *event);
	static void processRequestEventCb (void *userCtx, const belle_sip_request_event_t *event);
	static void processResponseEventCb (void *userCtx, const belle_sip_response_event_t *event);
	static void processTimeoutCb (void *userCtx, const belle_sip_timeout_event_t *event);
	static void processTransactionTerminatedCb (void *userCtx, const belle_sip_transaction_terminated_event_t *event);
	static void processAuthRequestedCb (void *userCtx, belle_sip_auth_event_t *event);

	MSFactory *mFactory = nullptr;
	Callbacks mCallbacks = { 0 };
	MSList *mPendingAuths = nullptr; // List of SalOp
	belle_sip_stack_t *mStack = nullptr;
	belle_sip_provider_t *mProvider = nullptr;
	belle_sip_header_user_agent_t *mUserAgent = nullptr;
	belle_sip_listener_t *mListener = nullptr;
	void *mTunnelClient = nullptr;
	void *mUserPointer = nullptr; // User pointer
	int mSessionExpires = 0;
	unsigned int mKeepAlive = 0;
	char *mRootCa = nullptr;
	char *mRootCaData = nullptr;
	char *mUuid = nullptr;
	int mRefresherRetryAfter = 60000; // Retry after value for refresher
	MSList *mSupportedTags = nullptr; // List of char *
	belle_sip_header_t *mSupported = nullptr;
	bool_t mOneMatchingCodec = FALSE;
	bool_t mUseTcpTlsKeepAlive = FALSE;
	bool_t mNatHelperEnabled = FALSE;
	bool_t mTlsVerify = TRUE;
	bool_t mTlsVerifyCn = TRUE;
	bool_t mUseDates = FALSE;
	bool_t mAutoContacts = TRUE;
	bool_t mEnableTestFeatures = FALSE;
	bool_t mNoInitialRoute = FALSE;
	bool_t mEnableSipUpdate = TRUE; // true by default
	SalOpSDPHandling mDefaultSdpHandling = SalOpSDPNormal;
	bool_t mPendingTransactionChecking = TRUE; // For testing purposes
	void *mSslConfig = nullptr;
	bctbx_list_t *mSupportedContentTypes = nullptr; // List of char *
	char *mLinphoneSpecs = nullptr;

	friend class SalOp;
	friend class SalCallOp;
	friend class SalRegisterOp;
	friend class SalMessageOp;
	friend class SalPresenceOp;
	friend class SalSubscribeOp;
	friend class SalPublishOp;
	friend class SalReferOp;
};

int to_sip_code(SalReason r);

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_H_

