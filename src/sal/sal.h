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

#include <list>
#include <vector>

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

class Sal {
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
	void setSupportedTags (const std::string &tags);
	const std::string &getSupportedTags () const;
	void addSupportedTag (const std::string &tag);
	void removeSupportedTag (const std::string &tag);

	void setUserAgent (const std::string &value);
	const std::string &getUserAgent () const;
	void appendStackStringToUserAgent ();

	bool isContentEncodingAvailable (const std::string &contentEncoding) const;
	bool isContentTypeSupported (const std::string &contentType) const;
	void addContentTypeSupport (const std::string &contentType);
	void removeContentTypeSupport (const std::string &contentType);

	void setDefaultSdpHandling (SalOpSDPHandling sdpHandlingMethod);

	void setUuid (const std::string &value) { mUuid = value; }
	int createUuid (char *uuid, size_t len);
	static int generateUuid (char *uuid, size_t len);

	void enableNatHelper (bool value);
	bool natHelperEnabled () const { return mNatHelperEnabled; }

	bool pendingTransactionCheckingEnabled () const { return mPendingTransactionChecking; }
	void enablePendingTransactionChecking (bool value) { mPendingTransactionChecking = value; }

	void setRefresherRetryAfter (int value) { mRefresherRetryAfter = value; }
	int getRefresherRetryAfter () const { return mRefresherRetryAfter; }

	void enableSipUpdateMethod (bool value) { mEnableSipUpdate = value; }
	void useSessionTimers (int expires) { mSessionExpires = expires; }
	void useDates (bool value) { mUseDates = value; }
	void useOneMatchingCodecPolicy (bool value) { mOneMatchingCodec = value; }
	void useRport (bool value);
	void enableAutoContacts (bool value) { mAutoContacts = value; }
	void enableTestFeatures (bool value) { mEnableTestFeatures = value; }
	void useNoInitialRoute (bool value) { mNoInitialRoute = value; }
	void enableUnconditionalAnswer (int value) { belle_sip_provider_enable_unconditional_answer(mProvider, value); }
	void enableReconnectToPrimaryAsap (bool value) { belle_sip_stack_enable_reconnect_to_primary_asap(mStack, value); }

	const std::list<SalOp *> &getPendingAuths () const { return mPendingAuths; }

	void setContactLinphoneSpecs (const std::string &value) { mLinphoneSpecs = value; }


	// ---------------------------------------------------------------------------
	// Network parameters
	// ---------------------------------------------------------------------------
	int setListenPort (const std::string &addr, int port, SalTransport tr, bool isTunneled);
	int getListeningPort (SalTransport tr);
	int isTransportAvailable (SalTransport t);

	void getDefaultLocalIp (int addressFamily, char *ip, size_t ipLen);

	void setTransportTimeout (int value) { belle_sip_stack_set_transport_timeout(mStack, value); }
	int getTransportTimeout () const { return belle_sip_stack_get_transport_timeout(mStack); }

	void setKeepAlivePeriod (unsigned int value);
	unsigned int getKeepAlivePeriod () const { return mKeepAlive; }
	void useTcpTlsKeepAlive (bool value) { mUseTcpTlsKeepAlive = value; }

	void setDscp (int dscp) { belle_sip_stack_set_default_dscp(mStack, dscp); }

	int setTunnel (void *tunnelClient);

	void setHttpProxyHost (const std::string &value);
	const std::string &getHttpProxyHost () const;

	void setHttpProxyPort (int value) { belle_sip_stack_set_http_proxy_port(mStack, value); }
	int getHttpProxyPort () const { return belle_sip_stack_get_http_proxy_port(mStack); }

	ortp_socket_t getSocket () const;

	int unlistenPorts ();
	int resetTransports ();


	// ---------------------------------------------------------------------------
	// TLS parameters
	// ---------------------------------------------------------------------------
	void setSslConfig (void *sslConfig);
	void setRootCa (const std::string &value);
	void setRootCaData (const std::string &value);
	const std::string &getRootCa () const { return mRootCa; }

	void verifyServerCertificates (bool value);
	void verifyServerCn (bool value);


	// ---------------------------------------------------------------------------
	// DNS resolution
	// ---------------------------------------------------------------------------
	void setDnsTimeout (int value) { belle_sip_stack_set_dns_timeout(mStack, value); }
	int getDnsTimeout () const { return belle_sip_stack_get_dns_timeout(mStack); }

	void setDnsServers (const bctbx_list_t *servers);

	void enableDnsSearch (bool value) { belle_sip_stack_enable_dns_search(mStack, (unsigned char)value); }
	bool dnsSearchEnabled () const { return !!belle_sip_stack_dns_search_enabled(mStack); }

	void enableDnsSrv (bool value) { belle_sip_stack_enable_dns_srv(mStack, (unsigned char)value); }
	bool dnsSrvEnabled () const { return !!belle_sip_stack_dns_srv_enabled(mStack); }

	void setDnsUserHostsFile (const std::string &value);
	const std::string &getDnsUserHostsFile () const;

	belle_sip_resolver_context_t *resolveA (const std::string &name, int port, int family, belle_sip_resolver_callback_t cb, void *data);
	belle_sip_resolver_context_t *resolve (const std::string &service, const std::string &transport, const std::string &name, int port, int family, belle_sip_resolver_callback_t cb, void *data); 


	// ---------------------------------------------------------------------------
	// Timers
	// ---------------------------------------------------------------------------
	belle_sip_source_t *createTimer (belle_sip_source_func_t func, void *data, unsigned int timeoutValueMs, const std::string &timerName);
	void cancelTimer (belle_sip_source_t *timer);


private:
	struct SalUuid {
		unsigned int timeLow;
		unsigned short timeMid;
		unsigned short timeHiAndVersion;
		unsigned char clockSeqHiAndReserved;
		unsigned char clockSeqLow;
		unsigned char node[6];
	};

	void setTlsProperties ();
	int addListenPort (SalAddress *addr, bool isTunneled);
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
	std::list<SalOp *> mPendingAuths;
	belle_sip_stack_t *mStack = nullptr;
	belle_sip_provider_t *mProvider = nullptr;
	belle_sip_header_user_agent_t *mUserAgentHeader = nullptr;
	belle_sip_listener_t *mListener = nullptr;
	void *mTunnelClient = nullptr;
	void *mUserPointer = nullptr; // User pointer
	int mSessionExpires = 0;
	unsigned int mKeepAlive = 0;
	std::string mRootCa;
	std::string mRootCaData;
	std::string mUuid;
	int mRefresherRetryAfter = 60000; // Retry after value for refresher
	std::vector<std::string> mSupportedTags;
	belle_sip_header_t *mSupportedHeader = nullptr;
	bool mOneMatchingCodec = false;
	bool mUseTcpTlsKeepAlive = false;
	bool mNatHelperEnabled = false;
	bool mTlsVerify = true;
	bool mTlsVerifyCn = true;
	bool mUseDates = false;
	bool mAutoContacts = true;
	bool mEnableTestFeatures = false;
	bool mNoInitialRoute = false;
	bool mEnableSipUpdate = true;
	SalOpSDPHandling mDefaultSdpHandling = SalOpSDPNormal;
	bool mPendingTransactionChecking = true; // For testing purposes
	void *mSslConfig = nullptr;
	std::vector<std::string> mSupportedContentTypes;
	std::string mLinphoneSpecs;

	// Cache values
	mutable std::string mDnsUserHostsFile;
	mutable std::string mHttpProxyHost;
	mutable std::string mSupported;
	mutable std::string mUserAgent;

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

