/*
 * private_structs.h
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

#ifndef _PRIVATE_STRUCTS_H_
#define _PRIVATE_STRUCTS_H_

#include <bctoolbox/map.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "carddav.h"
#include "sal/register-op.h"

struct _LinphoneQualityReporting{
	reporting_session_report_t * reports[3]; /**Store information on audio and video media streams (RFC 6035) */
	bool_t was_video_running; /*Keep video state since last check in order to detect its (de)activation*/
	LinphoneQualityReportingReportSendCb on_report_sent;
};

struct _LinphoneCallLog{
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *lc;
	LinphoneCallDir dir; /**< The direction of the call*/
	LinphoneCallStatus status; /**< The status of the call*/
	LinphoneAddress *from; /**<Originator of the call as a LinphoneAddress object*/
	LinphoneAddress *to; /**<Destination of the call as a LinphoneAddress object*/
	char start_date[128]; /**<Human readable string containing the start date*/
	int duration; /**<Duration of the call starting in connected state in seconds*/
	char *refkey;
	rtp_stats_t local_stats;
	rtp_stats_t remote_stats;
	float quality;
	time_t start_date_time; /**Start date of the call in seconds as expressed in a time_t */
	time_t connected_date_time; /**Connecting date of the call in seconds as expressed in a time_t */
	char* call_id; /**unique id of a call*/
	struct _LinphoneQualityReporting reporting;
	unsigned int storage_id;
	LinphoneErrorInfo *error_info;
	bool_t video_enabled;
	bool_t was_conference; /**<That call was a call with a conference server */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneCallLog);

struct _CallCallbackObj
{
	LinphoneCallCbFunc _func;
	void * _user_data;
};

struct StunCandidate{
	char addr[64];
	int port;
};

struct _PortConfig{
	char multicast_ip[LINPHONE_IPADDR_SIZE];
	char multicast_bind_ip[LINPHONE_IPADDR_SIZE];
	int rtp_port;
	int rtcp_port;
};

struct _LinphoneProxyConfig
{
	belle_sip_object_t base;
	void *user_data;
	struct _LinphoneCore *lc;
	LinphoneErrorInfo *ei;
	char *reg_proxy;
	char *reg_identity;
	LinphoneAddress* identity_address;
	LinphoneAddress *contact_address;
	LinphoneAddress *contact_address_without_params;
	bctbx_list_t *reg_routes;
	char *quality_reporting_collector;
	char *realm;
	char *contact_params;
	char *contact_uri_params;
	int expires;
	int publish_expires;
	LinphonePrivate::SalRegisterOp *op;
	SalCustomHeader *sent_headers;
	char *type;
	struct _SipSetupContext *ssctx;
	int auth_failures;
	char *dial_prefix;
	LinphoneRegistrationState state;
	LinphoneAVPFMode avpf_mode;
	LinphoneNatPolicy *nat_policy;
	int quality_reporting_interval;

	bool_t commit;
	bool_t reg_sendregister;
	bool_t publish;
	bool_t dial_escape_plus;

	bool_t send_publish;
	bool_t quality_reporting_enabled;
	uint8_t avpf_rr_interval;
	bool_t register_changed;

	time_t deletion_date;
	LinphonePrivacyMask privacy;
	/*use to check if server config has changed  between edit() and done()*/
	LinphoneAddress *saved_proxy;
	LinphoneAddress *saved_identity;
	
	/*---*/
	LinphoneAddress *pending_contact; /*use to store previous contact in case of network failure*/
	LinphoneEvent *presence_publish_event;
	unsigned long long previous_publish_config_hash[2];

	char *refkey;
	char *sip_etag; /*publish context*/
	char *conference_factory_uri;

	bool_t push_notification_allowed;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneProxyConfig);

struct _LinphoneAuthInfo
{
	belle_sip_object_t base;
	char *username;
	char *realm;
	char *userid;
	char *passwd;
	char *ha1;
	char *domain;
	char *tls_cert;
	char *tls_key;
	char *tls_cert_path;
	char *tls_key_path;
	char *algorithm;
};

struct _LinphoneFriendPresence {
	char *uri_or_tel;
	LinphonePresenceModel *presence;
};

struct _LinphoneFriendPhoneNumberSipUri {
	char *number;
	char *uri;
};

struct _LinphoneFriend{
	belle_sip_object_t base;
	void *user_data;
	LinphoneAddress *uri;
	MSList *insubs; /*list of SalOp. There can be multiple instances of a same Friend that subscribe to our presence*/
	LinphonePrivate::SalPresenceOp *outsub;
	LinphoneSubscribePolicy pol;
	MSList *presence_models; /* list of LinphoneFriendPresence. It associates SIP URIs and phone numbers with their respective presence models. */
	MSList *phone_number_sip_uri_map; /* list of LinphoneFriendPhoneNumberSipUri. It associates phone numbers with their corresponding SIP URIs. */
	struct _LinphoneCore *lc;
	BuddyInfo *info;
	char *refkey;
	bool_t subscribe;
	bool_t subscribe_active;
	bool_t inc_subscribe_pending;
	bool_t commit;
	bool_t initial_subscribes_sent; /*used to know if initial subscribe message was sent or not*/
	bool_t presence_received;
	LinphoneVcard *vcard;
	unsigned int storage_id;
	LinphoneFriendList *friend_list;
	LinphoneSubscriptionState out_sub_state;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneFriend);

struct _LinphoneFriendListCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneFriendListCbsContactCreatedCb contact_created_cb;
	LinphoneFriendListCbsContactDeletedCb contact_deleted_cb;
	LinphoneFriendListCbsContactUpdatedCb contact_updated_cb;
	LinphoneFriendListCbsSyncStateChangedCb sync_state_changed_cb;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneFriendListCbs);

struct _LinphoneFriendList {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	LinphoneEvent *event;
	char *display_name;
	char *rls_uri; /*this field is take in sync with rls_addr*/
	LinphoneAddress *rls_addr;
	MSList *friends;
	bctbx_map_t *friends_map;
	bctbx_map_t *friends_map_uri;
	unsigned char *content_digest;
	int expected_notification_version;
	unsigned int storage_id;
	char *uri;
	MSList *dirty_friends_to_update;
	int revision;
	LinphoneFriendListCbs *cbs;
	bool_t enable_subscriptions;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneFriendList);

struct sip_config
{
	char *contact;
	char *guessed_contact;
	MSList *proxies;
	MSList *deleted_proxies;
	int inc_timeout;	/*timeout after an un-answered incoming call is rejected*/
	int in_call_timeout;	/*timeout after a call is hangup */
	int delayed_timeout; 	/*timeout after a delayed call is resumed */
	unsigned int keepalive_period; /* interval in ms between keep alive messages sent to the proxy server*/
	LinphoneSipTransports transports;
	bool_t guess_hostname;
	bool_t loopback_only;
	bool_t ipv6_enabled;
	bool_t sdp_200_ack;
	bool_t register_only_when_network_is_up;
	bool_t register_only_when_upnp_is_ok;
	bool_t ping_with_options;
	bool_t auto_net_state_mon;
	bool_t tcp_tls_keepalive;
	bool_t vfu_with_info; /*use to enable vfu request using sip info*/
	bool_t save_auth_info; // if true, auth infos will be write in the config file when they are added to the list
};

struct rtp_config
{
	int audio_rtp_min_port;
	int audio_rtp_max_port;
	int video_rtp_min_port;
	int video_rtp_max_port;
	int audio_jitt_comp;  /*jitter compensation*/
	int video_jitt_comp;  /*jitter compensation*/
	int nortp_timeout;
	int disable_upnp;
	MSCryptoSuite *srtp_suites;
	LinphoneAVPFMode avpf_mode;
	bool_t rtp_no_xmit_on_audio_mute;
							  /* stop rtp xmit when audio muted */
	bool_t audio_adaptive_jitt_comp_enabled;
	bool_t video_adaptive_jitt_comp_enabled;
	bool_t pad;
	char* audio_multicast_addr;
	bool_t audio_multicast_enabled;
	int audio_multicast_ttl;
	char* video_multicast_addr;
	int video_multicast_ttl;
	bool_t video_multicast_enabled;
	int text_rtp_min_port;
	int text_rtp_max_port;
};

struct net_config
{
	char *nat_address; /* may be IP or host name */
	char *nat_address_ip; /* ip translated from nat_address */
	struct addrinfo *stun_addrinfo;
	int download_bw;
	int upload_bw;
	int mtu;
	OrtpNetworkSimulatorParams netsim_params;
	bool_t nat_sdp_only;
};

struct sound_config
{
	struct _MSSndCard * ring_sndcard;	/* the playback sndcard currently used */
	struct _MSSndCard * play_sndcard;	/* the playback sndcard currently used */
	struct _MSSndCard * capt_sndcard; /* the capture sndcard currently used */
	struct _MSSndCard * lsd_card; /* dummy playback card for Linphone Sound Daemon extension */
	const char **cards;
	int latency;	/* latency in samples of the current used sound device */
	float soft_play_lev; /*playback gain in db.*/
	float soft_mic_lev; /*mic gain in db.*/
	char rec_lev;
	char play_lev;
	char ring_lev;
	char source;
	char *local_ring;
	char *remote_ring;
	char *ringback_tone;
	bool_t ec;
	bool_t ea;
	bool_t agc;
};

struct codecs_config
{
	MSList *audio_codecs;  /* list of audio codecs in order of preference*/
	MSList *video_codecs;
	MSList *text_codecs;
	int dyn_pt;
	int telephone_event_pt;
};

struct video_config{
	struct _MSWebCam *device;
	const char **cams;
	MSVideoSize vsize;
	MSVideoSize preview_vsize; /*is 0,0 if no forced preview size is set, in which case vsize field above is used.*/
	LinphoneVideoDefinition *vdef;
	LinphoneVideoDefinition *preview_vdef;
	float fps;
	bool_t capture;
	bool_t show_local;
	bool_t display;
	bool_t selfview; /*during calls*/
	bool_t reuse_preview_source;
};

struct text_config{
	bool_t enabled;
};

struct ui_config
{
	int is_daemon;
	int is_applet;
	unsigned int timer_id;  /* the timer id for registration */
};

struct autoreplier_config
{
	int enabled;
	int after_seconds;		/* accept the call after x seconds*/
	int max_users;			/* maximum number of user that can call simultaneously */
	int max_rec_time;  	/* the max time of incoming voice recorded */
	int max_rec_msg;		/* maximum number of recorded messages */
	const char *message;		/* the path of the file to be played */
};

struct _LinphoneToneDescription{
	LinphoneReason reason; /*the call error code*/
	LinphoneToneID toneid; /*A tone type to play when this error arrives. This is played using tone generator*/
	char *audiofile; /*An override audio file to play instead, when this error arrives*/
	/*Note that some tones are not affected to any error, in which case it is affected LinphoneReasonNone*/
};

struct _LinphoneTaskList{
	MSList *hooks;
};

struct _LinphoneCoreCbs {
	belle_sip_object_t base;
	LinphoneCoreVTable *vtable;
	bool_t autorelease;
};

struct _LCCallbackObj {
	LinphoneCoreCbFunc _func;
	void *_user_data;
};

struct _LinphoneEventCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneEventCbsNotifyResponseCb notify_response_cb;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneEventCbs);

struct _LinphoneEvent{
	belle_sip_object_t base;
	LinphoneErrorInfo *ei;
	LinphoneSubscriptionDir dir;
	LinphoneCore *lc;
	LinphonePrivate::SalEventOp *op;
	SalCustomHeader *send_custom_headers;
	LinphoneSubscriptionState subscription_state;
	LinphonePublishState publish_state;
	void *userdata;
	char *name;
	LinphoneEventCbs *callbacks;
	int expires;
	bool_t terminating;
	bool_t is_out_of_dialog_op; /*used for out of dialog notify*/
	bool_t internal;
	bool_t oneshot;

	// For migration purpose. (Do not use directly!)
	// Cache.
	LinphoneAddress *to_address;
	LinphoneAddress *from_address;
	LinphoneAddress *remote_contact_address;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneEvent);

struct _EcCalibrator{
	MSFactory *factory;
	ms_thread_t thread;
	MSSndCard *play_card,*capt_card;
	MSFilter *sndread,*det,*rec;
	MSFilter *play, *gen, *sndwrite;
	MSFilter *read_resampler,*write_resampler;
	MSTicker *ticker;
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
	LinphoneEcCalibrationCallback cb;
	void *cb_data;
	LinphoneEcCalibrationAudioInit audio_init_cb;
	LinphoneEcCalibrationAudioUninit audio_uninit_cb;
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif
	int64_t acc;
	int delay;
	unsigned int rate;
	LinphoneEcCalibratorStatus status;
	bool_t freq1,freq2,freq3;
	bool_t play_cool_tones;
};

struct _EchoTester {
    MSFactory *factory;
    MSFilter *in,*out;
    MSSndCard *capture_card;
    MSSndCard *playback_card;
    MSTicker *ticker;
    unsigned int rate;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneContent);

struct _LinphoneBuffer {
	belle_sip_object_t base;
	void *user_data;
	uint8_t *content;	/**< A pointer to the buffer content */
	size_t size;	/**< The size of the buffer content */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneBuffer);

struct _LinphoneNatPolicy {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	belle_sip_resolver_context_t *stun_resolver_context;
	struct addrinfo *stun_addrinfo;
	char *stun_server;
	char *stun_server_username;
	char *ref;
	bool_t stun_enabled;
	bool_t turn_enabled;
	bool_t ice_enabled;
	bool_t upnp_enabled;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneNatPolicy);

struct _LinphoneImNotifPolicy {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	bool_t send_is_composing;
	bool_t recv_is_composing;
	bool_t send_imdn_delivered;
	bool_t recv_imdn_delivered;
	bool_t send_imdn_displayed;
	bool_t recv_imdn_displayed;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneImNotifPolicy);


/*****************************************************************************
 * XML-RPC interface                                                         *
 ****************************************************************************/

struct _LinphoneXmlRpcArg {
	LinphoneXmlRpcArgType type;
	union {
		int i;
		char *s;
	} data;
};

struct _LinphoneXmlRpcRequestCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneXmlRpcRequestCbsResponseCb response;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneXmlRpcRequestCbs);

struct _LinphoneXmlRpcRequest {
	belle_sip_object_t base;
	void *user_data;
	LinphoneXmlRpcRequestCbs *callbacks;
	belle_sip_list_t *arg_list;
	char *content;	/**< The string representation of the XML-RPC request */
	char *method;
	LinphoneXmlRpcStatus status;
	struct _LinphoneXmlRpcArg response;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneXmlRpcRequest);

struct _LinphoneXmlRpcSession {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *core;
	char *url;
	bool_t released;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneXmlRpcSession);


/*****************************************************************************
 * CardDAV interface                                                         *
 ****************************************************************************/

struct _LinphoneCardDavContext {
	LinphoneFriendList *friend_list;
	int ctag;
	void *user_data;
	LinphoneCardDavContactCreatedCb contact_created_cb;
	LinphoneCardDavContactUpdatedCb contact_updated_cb;
	LinphoneCardDavContactRemovedCb contact_removed_cb;
	LinphoneCardDavSynchronizationDoneCb sync_done_cb;
	LinphoneAuthInfo *auth_info;
};

struct _LinphoneCardDavQuery {
	LinphoneCardDavContext *context;
	char *url;
	const char *method;
	char *body;
	const char *depth;
	const char *ifmatch;
	belle_http_request_listener_t *http_request_listener;
	void *user_data;
	LinphoneCardDavQueryType type;
};

struct _LinphoneCardDavResponse {
	char *etag;
	char *url;
	char *vcard;
};


/*****************************************************************************
 * Player interface                                                          *
 ****************************************************************************/

struct _LinphonePlayerCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphonePlayerCbsEofReachedCb eof;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphonePlayerCbs);

struct _LinphonePlayer{
	belle_sip_object_t base;
	void *user_data;
	int (*open)(LinphonePlayer* player, const char *filename);
	int (*start)(LinphonePlayer* player);
	int (*pause)(LinphonePlayer* player);
	int (*seek)(LinphonePlayer* player, int time_ms);
	MSPlayerState (*get_state)(LinphonePlayer* player);
	int (*get_duration)(LinphonePlayer *player);
	int (*get_position)(LinphonePlayer *player);
	void (*close)(LinphonePlayer* player);
	void (*destroy)(LinphonePlayer *player);
	void *impl;
	LinphonePlayerCbs *callbacks;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphonePlayer);


/*****************************************************************************
 * XML UTILITY FUNCTIONS                                                     *
 ****************************************************************************/

#define XMLPARSING_BUFFER_LEN 2048
#define MAX_XPATH_LENGTH 256

struct _xmlparsing_context {
	xmlDoc *doc;
	xmlXPathContextPtr xpath_ctx;
	char errorBuffer[XMLPARSING_BUFFER_LEN];
	char warningBuffer[XMLPARSING_BUFFER_LEN];
};


/*****************************************************************************
 * OTHER UTILITY FUNCTIONS                                                     *
 ****************************************************************************/

struct _LinphoneImEncryptionEngineCbs {
	belle_sip_object_t base;
	void *user_data;
	LinphoneImEncryptionEngineCbsIncomingMessageCb process_incoming_message;
	LinphoneImEncryptionEngineCbsOutgoingMessageCb process_outgoing_message;
	LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb is_encryption_enabled_for_file_transfer;
	LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb generate_file_transfer_key;
	LinphoneImEncryptionEngineCbsDownloadingFileCb process_downlading_file;
	LinphoneImEncryptionEngineCbsUploadingFileCb process_uploading_file;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneImEncryptionEngineCbs);

struct _LinphoneImEncryptionEngine {
	belle_sip_object_t base;
	void *user_data;
	LinphoneCore *lc;
	LinphoneImEncryptionEngineCbs *callbacks;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneImEncryptionEngine);

struct _LinphoneRange {
	belle_sip_object_t base;
	void *user_data;
	int min;
	int max;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneRange);

struct _LinphoneTransports {
	belle_sip_object_t base;
	void *user_data;
	int udp_port; /**< SIP/UDP port */
	int tcp_port; /**< SIP/TCP port */
	int dtls_port; /**< SIP/DTLS port */
	int tls_port; /**< SIP/TLS port */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneTransports);

struct _LinphoneVideoActivationPolicy {
	belle_sip_object_t base;
	void *user_data;
	bool_t automatically_initiate; /**<Whether video shall be automatically proposed for outgoing calls.*/
	bool_t automatically_accept; /**<Whether video shall be automatically accepted for incoming calls*/
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneVideoActivationPolicy);


struct _VTableReference{
	LinphoneCoreCbs *cbs;
	bool_t valid;
	bool_t autorelease;
	bool_t internal;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneTunnelConfig);

struct _LinphoneErrorInfo{
	belle_sip_object_t base;
	LinphoneReason reason;
	char *protocol; /* */
	int protocol_code; /*from SIP response*/
	char *phrase; /*from SIP response*/
	char *warnings; /*from SIP response*/
	char *full_string; /*concatenation of status_string + warnings*/
	struct _LinphoneErrorInfo *sub_ei;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneErrorInfo);

struct _LinphoneVideoDefinition {
	belle_sip_object_t base;
	void *user_data;
	unsigned int width; /**< The width of the video */
	unsigned int height; /**< The height of the video */
	char *name; /** The name of the video definition */
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneVideoDefinition);


namespace LinphonePrivate {
	class Core;
};

#define LINPHONE_CORE_STRUCT_BASE_FIELDS \
	MSFactory* factory; \
	MSList* vtable_refs; \
	int vtable_notify_recursion; \
	LinphonePrivate::Sal *sal; \
	void *platform_helper; \
	LinphoneGlobalState state; \
	struct _LpConfig *config; \
	MSList *default_audio_codecs; \
	MSList *default_video_codecs; \
	MSList *default_text_codecs; \
	net_config_t net_conf; \
	sip_config_t sip_conf; \
	rtp_config_t rtp_conf; \
	sound_config_t sound_conf; \
	video_config_t video_conf; \
	text_config_t text_conf; \
	codecs_config_t codecs_conf; \
	ui_config_t ui_conf; \
	autoreplier_config_t autoreplier_conf; \
	LinphoneProxyConfig *default_proxy; \
	MSList *friends_lists; \
	MSList *auth_info; \
	struct _RingStream *ringstream; \
	time_t dmfs_playing_start_time; \
	LCCallbackObj preview_finished_cb; \
	MSList *queued_calls; \
	MSList *call_logs; \
	int max_call_logs; \
	int missed_calls; \
	VideoPreview *previewstream; \
	struct _MSEventQueue *msevq; \
	LinphoneRtpTransportFactories *rtptf; \
	MSList *bl_reqs; \
	MSList *subscribers; \
	int minutes_away; \
	LinphonePresenceModel *presence_model; \
	void *data; \
	char *play_file; \
	char *rec_file; \
	uint64_t prevtime_ms; \
	int audio_bw; \
	LinphoneCoreWaitingCallback wait_cb; \
	void *wait_ctx; \
	void *video_window_id; \
	void *preview_window_id; \
	time_t netup_time; \
	struct _EcCalibrator *ecc; \
	struct _EchoTester *ect; \
	LinphoneTaskList hooks; \
	LinphoneConference *conf_ctx; \
	char* zrtp_secrets_cache; \
	char* user_certificates_path; \
	LinphoneVideoPolicy video_policy; \
	time_t network_last_check; \
	LinphoneNatPolicy *nat_policy; \
	LinphoneImNotifPolicy *im_notif_policy; \
	bool_t use_files; \
	bool_t apply_nat_settings; \
	bool_t initial_subscribes_sent; \
	bool_t bl_refresh; \
	bool_t preview_finished; \
	bool_t auto_net_state_mon; \
	bool_t sip_network_reachable; \
	bool_t media_network_reachable; \
	bool_t network_reachable_to_be_notified; \
	bool_t use_preview_window; \
	bool_t network_last_status; \
	bool_t ringstream_autorelease; \
	bool_t vtables_running; \
	bool_t send_call_stats_periodical_updates; \
	bool_t forced_ice_relay; \
	bool_t short_turn_refresh; \
	char localip[LINPHONE_IPADDR_SIZE]; \
	int device_rotation; \
	int max_calls; \
	LinphoneTunnel *tunnel; \
	char* device_id; \
	char *logs_db_file; \
	char *friends_db_file; \
	belle_http_provider_t *http_provider; \
	belle_tls_crypto_config_t *http_crypto_config; \
	belle_http_request_listener_t *provisioning_http_listener; \
	MSList *tones; \
	LinphoneReason chat_deny_code; \
	char *file_transfer_server; \
	const char **supported_formats; \
	LinphoneContent *log_collection_upload_information; \
	LinphoneCoreCbs *current_cbs; \
	LinphoneRingtonePlayer *ringtoneplayer; \
	LinphoneVcardContext *vcard_context; \
	bool_t zrtp_not_available_simulation; \
	char *tls_cert; \
	char *tls_key; \
	LinphoneAddress *default_rls_addr; \
	LinphoneImEncryptionEngine *im_encryption_engine; \
	struct _LinphoneAccountCreatorService *default_ac_service; \
	MSBandwidthController *bw_controller; \
	belle_http_request_listener_t *update_check_http_listener; \
	char *update_check_current_version; \
	bctbx_list_t *chat_rooms; \
	bctbx_list_t *callsCache; \
	bool_t dns_set_by_app; \

#ifdef SQLITE_STORAGE_ENABLED
#define LINPHONE_CORE_STRUCT_FIELDS \
	LINPHONE_CORE_STRUCT_BASE_FIELDS \
	sqlite3 *zrtp_cache_db; \
	sqlite3 *logs_db; \
	sqlite3 *friends_db; \
	bool_t debug_storage;
#else
#define LINPHONE_CORE_STRUCT_FIELDS \
	LINPHONE_CORE_STRUCT_BASE_FIELDS
#endif


#endif /* _PRIVATE_STRUCTS_H_ */
