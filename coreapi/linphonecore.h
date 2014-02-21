/*
linphone
Copyright (C) 2000 - 2010 Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef LINPHONECORE_H
#define LINPHONECORE_H

#include "ortp/ortp.h"
#include "ortp/payloadtype.h"
#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/mediastream.h"

#ifdef IN_LINPHONE
#include "sipsetup.h"
#else
#include "linphone/sipsetup.h"
#endif

#include "lpconfig.h"

#define LINPHONE_IPADDR_SIZE 64
#define LINPHONE_HOSTNAME_SIZE 128

#ifndef LINPHONE_PUBLIC
	#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _LinphoneCore;
/**
 * Linphone core main object created by function linphone_core_new() .
 * @ingroup initializing
 */
typedef struct _LinphoneCore LinphoneCore;


/**
 * Disable a sip transport
 * Use with #LCSipTransports
 * @ingroup initializing
 */
#define LC_SIP_TRANSPORT_DISABLED 0
/**
 * Randomly chose a sip port for this transport
 * Use with #LCSipTransports
 * @ingroup initializing
 */
#define LC_SIP_TRANSPORT_RANDOM -1

/**
 * Linphone core SIP transport ports.
 * Use with #linphone_core_set_sip_transports
 * @ingroup initializing
 */
typedef struct _LCSipTransports{
	/**
	 * udp port to listening on, negative value if not set
	 * */
	int udp_port;
	/**
	 * tcp port to listening on, negative value if not set
	 * */
	int tcp_port;
	/**
	 * dtls port to listening on, negative value if not set
	 * */
	int dtls_port;
	/**
	 * tls port to listening on, negative value if not set
	 * */
	int tls_port;
} LCSipTransports;


/**
 * Enum describing transport type for LinphoneAddress.
 * @ingroup linphone_address
**/
enum _LinphoneTransportType{
	LinphoneTransportUdp,
	LinphoneTransportTcp,
	LinphoneTransportTls,
	LinphoneTransportDtls
};
/*this enum MUST be kept in sync with the SalTransport from sal.h*/

/**
 * Typedef for transport type enum.
 * @ingroup linphone_address
**/
typedef enum _LinphoneTransportType LinphoneTransportType;

/**
 * Object that represents a SIP address.
 *
 * The LinphoneAddress is an opaque object to represents SIP addresses, ie
 * the content of SIP's 'from' and 'to' headers.
 * A SIP address is made of display name, username, domain name, port, and various
 * uri headers (such as tags). It looks like 'Alice <sip:alice@example.net>'.
 * The LinphoneAddress has methods to extract and manipulate all parts of the address.
 * When some part of the address (for example the username) is empty, the accessor methods
 * return NULL.
 *
 * @ingroup linphone_address
 * @var LinphoneAddress
 */
typedef struct SalAddress LinphoneAddress;

typedef struct belle_sip_dict LinphoneDictionary;

/**
 * The LinphoneContent struct holds data that can be embedded in a signaling message.
 * @ingroup misc
**/
struct _LinphoneContent{
	char *type; /**<mime type for the data, for example "application"*/
	char *subtype; /**<mime subtype for the data, for example "html"*/
	void *data; /**<the actual data buffer, usually a string */
	size_t size; /**<the size of the data buffer, excluding null character despite null character is always set for convenience.*/
	char *encoding; /**<The encoding of the data buffer, for example "gzip"*/
};

/**
 * Alias to the LinphoneContent struct.
 * @ingroup misc
**/
typedef struct _LinphoneContent LinphoneContent;

/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
 * @ingroup call_control
**/
struct _LinphoneCall;
/**
 * The LinphoneCall object represents a call issued or received by the LinphoneCore
 * @ingroup call_control
**/
typedef struct _LinphoneCall LinphoneCall;

/**
 * Enum describing various failure reasons or contextual information for some events.
 * @see linphone_call_get_reason()
 * @see linphone_proxy_config_get_error()
 * @ingroup misc
**/
enum _LinphoneReason{
	LinphoneReasonNone,
	LinphoneReasonNoResponse, /**<No response received from remote*/
	LinphoneReasonBadCredentials, /**<Authentication failed due to bad*/
	LinphoneReasonDeclined, /**<The call has been declined*/
	LinphoneReasonNotFound, /**<Destination of the calls was not found.*/
	LinphoneReasonNotAnswered, /**<The call was not answered in time*/
	LinphoneReasonBusy, /**<Phone line was busy */
	LinphoneReasonUnsupportedContent, /**<Unsupported content */
	LinphoneReasonIOError, /**<Transport error: connection failures, disconnections etc...*/
	LinphoneReasonDoNotDisturb, /**<Do not disturb reason*/
	LinphoneReasonUnauthorized, /**<Operation is unauthorized because missing credential*/
	LinphoneReasonNotAcceptable, /**<Operation like call update rejected by peer*/
	LinphoneReasonNoMatch /**<Operation could not be executed by server or remote client because it didn't have any context for it*/
};

/*for compatibility*/
#define LinphoneReasonMedia LinphoneReasonUnsupportedContent

/**
 * Enum describing failure reasons.
 * @ingroup misc
**/
typedef enum _LinphoneReason LinphoneReason;

/**
 * Converts a LinphoneReason enum to a string.
 * @ingroup misc
**/
const char *linphone_reason_to_string(LinphoneReason err);


/* linphone dictionary */
LINPHONE_PUBLIC	LinphoneDictionary* linphone_dictionary_new();
LinphoneDictionary * linphone_dictionary_clone(const LinphoneDictionary* src);
LinphoneDictionary * linphone_dictionary_ref(LinphoneDictionary* obj);
void linphone_dictionary_unref(LinphoneDictionary* obj);
LINPHONE_PUBLIC void linphone_dictionary_set_int(LinphoneDictionary* obj, const char* key, int value);
LINPHONE_PUBLIC int linphone_dictionary_get_int(LinphoneDictionary* obj, const char* key, int default_value);
LINPHONE_PUBLIC void linphone_dictionary_set_string(LinphoneDictionary* obj, const char* key, const char*value);
LINPHONE_PUBLIC const char* linphone_dictionary_get_string(LinphoneDictionary* obj, const char* key, const char* default_value);
LINPHONE_PUBLIC void linphone_dictionary_set_int64(LinphoneDictionary* obj, const char* key, int64_t value);
LINPHONE_PUBLIC int64_t linphone_dictionary_get_int64(LinphoneDictionary* obj, const char* key, int64_t default_value);
LINPHONE_PUBLIC int linphone_dictionary_remove(LinphoneDictionary* obj, const char* key);
LINPHONE_PUBLIC void linphone_dictionary_clear(LinphoneDictionary* obj);
LINPHONE_PUBLIC int linphone_dictionary_haskey(const LinphoneDictionary* obj, const char* key);
LINPHONE_PUBLIC void linphone_dictionary_foreach( const LinphoneDictionary* obj, void (*apply_func)(const char*key, void* value, void* userdata), void* userdata);
/**
 * Converts a config section into a dictionary.
 * @return a #LinphoneDictionary with all the keys from a section, or NULL if the section doesn't exist
 * @ingroup misc
 */
LinphoneDictionary* lp_config_section_to_dict( const LpConfig* lpconfig, const char* section );

/**
 * Loads a dictionary into a section of the lpconfig. If the section doesn't exist it is created.
 * Overwrites existing keys, creates non-existing keys.
 * @ingroup misc
 */
void lp_config_load_dict_to_section( LpConfig* lpconfig, const char* section, const LinphoneDictionary* dict);


#ifdef IN_LINPHONE
#include "linphonefriend.h"
#include "event.h"
#else
#include "linphone/linphonefriend.h"
#include "linphone/event.h"
#endif

LINPHONE_PUBLIC	LinphoneAddress * linphone_address_new(const char *addr);
LINPHONE_PUBLIC LinphoneAddress * linphone_address_clone(const LinphoneAddress *addr);
LINPHONE_PUBLIC LinphoneAddress * linphone_address_ref(LinphoneAddress *addr);
LINPHONE_PUBLIC void linphone_address_unref(LinphoneAddress *addr);
LINPHONE_PUBLIC const char *linphone_address_get_scheme(const LinphoneAddress *u);
LINPHONE_PUBLIC	const char *linphone_address_get_display_name(const LinphoneAddress* u);
LINPHONE_PUBLIC	const char *linphone_address_get_username(const LinphoneAddress *u);
LINPHONE_PUBLIC	const char *linphone_address_get_domain(const LinphoneAddress *u);
LINPHONE_PUBLIC int linphone_address_get_port(const LinphoneAddress *u);
LINPHONE_PUBLIC	void linphone_address_set_display_name(LinphoneAddress *u, const char *display_name);
LINPHONE_PUBLIC	void linphone_address_set_username(LinphoneAddress *uri, const char *username);
LINPHONE_PUBLIC	void linphone_address_set_domain(LinphoneAddress *uri, const char *host);
LINPHONE_PUBLIC	void linphone_address_set_port(LinphoneAddress *uri, int port);
/*remove tags, params etc... so that it is displayable to the user*/
LINPHONE_PUBLIC	void linphone_address_clean(LinphoneAddress *uri);
LINPHONE_PUBLIC bool_t linphone_address_is_secure(const LinphoneAddress *uri);
LINPHONE_PUBLIC LinphoneTransportType linphone_address_get_transport(const LinphoneAddress *uri);
LINPHONE_PUBLIC void linphone_address_set_transport(LinphoneAddress *uri,LinphoneTransportType type);
LINPHONE_PUBLIC	char *linphone_address_as_string(const LinphoneAddress *u);
LINPHONE_PUBLIC	char *linphone_address_as_string_uri_only(const LinphoneAddress *u);
LINPHONE_PUBLIC	bool_t linphone_address_weak_equal(const LinphoneAddress *a1, const LinphoneAddress *a2);
LINPHONE_PUBLIC	void linphone_address_destroy(LinphoneAddress *u);

/**
 * Create a #LinphoneAddress object by parsing the user supplied address, given as a string.
 * @param[in] lc #LinphoneCore object
 * @param[in] address String containing the user supplied address
 * @return The create #LinphoneAddress object
 * @ingroup linphone_address
 */
LINPHONE_PUBLIC LinphoneAddress * linphone_core_create_address(LinphoneCore *lc, const char *address);

struct _SipSetupContext;


/**
 * Enum representing the direction of a call.
 * @ingroup call_logs
**/
enum _LinphoneCallDir {
	LinphoneCallOutgoing, /**< outgoing calls*/
	LinphoneCallIncoming  /**< incoming calls*/
};

/**
 * Typedef for enum
 * @ingroup call_logs
**/
typedef enum _LinphoneCallDir LinphoneCallDir;

/**
 * Enum representing the status of a call
 * @ingroup call_logs
**/
typedef enum _LinphoneCallStatus {
	LinphoneCallSuccess, /**< The call was sucessful*/
	LinphoneCallAborted, /**< The call was aborted */
	LinphoneCallMissed, /**< The call was missed (unanswered)*/
	LinphoneCallDeclined /**< The call was declined, either locally or by remote end*/
} LinphoneCallStatus;

/**
 * Structure representing a call log.
 *
 * @ingroup call_logs
 *
**/
typedef struct _LinphoneCallLog LinphoneCallLog;

/**
 * Enum describing type of media encryption types.
 * @ingroup media_parameters
**/
enum _LinphoneMediaEncryption {
	LinphoneMediaEncryptionNone, /**< No media encryption is used */
	LinphoneMediaEncryptionSRTP, /**< Use SRTP media encryption */
	LinphoneMediaEncryptionZRTP /**< Use ZRTP media encryption */
};

/**
 * Enum describing type of media encryption types.
 * @ingroup media_parameters
**/
typedef enum _LinphoneMediaEncryption LinphoneMediaEncryption;

/**
 * Convert enum member to string.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC const char *linphone_media_encryption_to_string(LinphoneMediaEncryption menc);

/*public: */
LINPHONE_PUBLIC	LinphoneAddress *linphone_call_log_get_from(LinphoneCallLog *cl);
LINPHONE_PUBLIC	LinphoneAddress *linphone_call_log_get_to(LinphoneCallLog *cl);
LINPHONE_PUBLIC	LinphoneAddress *linphone_call_log_get_remote_address(LinphoneCallLog *cl);
LINPHONE_PUBLIC	LinphoneCallDir linphone_call_log_get_dir(LinphoneCallLog *cl);
LINPHONE_PUBLIC	LinphoneCallStatus linphone_call_log_get_status(LinphoneCallLog *cl);
LINPHONE_PUBLIC	bool_t linphone_call_log_video_enabled(LinphoneCallLog *cl);
LINPHONE_PUBLIC	time_t linphone_call_log_get_start_date(LinphoneCallLog *cl);
LINPHONE_PUBLIC	int linphone_call_log_get_duration(LinphoneCallLog *cl);
LINPHONE_PUBLIC	float linphone_call_log_get_quality(LinphoneCallLog *cl);
LINPHONE_PUBLIC	void linphone_call_log_set_user_pointer(LinphoneCallLog *cl, void *up);
LINPHONE_PUBLIC	void *linphone_call_log_get_user_pointer(const LinphoneCallLog *cl);
void linphone_call_log_set_ref_key(LinphoneCallLog *cl, const char *refkey);
const char *linphone_call_log_get_ref_key(const LinphoneCallLog *cl);
LINPHONE_PUBLIC	const rtp_stats_t *linphone_call_log_get_local_stats(const LinphoneCallLog *cl);
LINPHONE_PUBLIC	const rtp_stats_t *linphone_call_log_get_remote_stats(const LinphoneCallLog *cl);
LINPHONE_PUBLIC	const char *linphone_call_log_get_call_id(const LinphoneCallLog *cl);
LINPHONE_PUBLIC	char * linphone_call_log_to_str(LinphoneCallLog *cl);

/**
 * Private structure definition for LinphoneCallParams.
 * @ingroup call_control
**/
struct _LinphoneCallParams;

/**
 * The LinphoneCallParams is an object containing various call related parameters.
 * It can be used to retrieve parameters from a currently running call or modify the call's characteristics
 * dynamically.
 * @ingroup call_control
**/
typedef struct _LinphoneCallParams LinphoneCallParams;

LINPHONE_PUBLIC	const PayloadType* linphone_call_params_get_used_audio_codec(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	const PayloadType* linphone_call_params_get_used_video_codec(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	LinphoneCallParams * linphone_call_params_copy(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	void linphone_call_params_enable_video(LinphoneCallParams *cp, bool_t enabled);
LINPHONE_PUBLIC	bool_t linphone_call_params_video_enabled(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	LinphoneMediaEncryption linphone_call_params_get_media_encryption(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	void linphone_call_params_set_media_encryption(LinphoneCallParams *cp, LinphoneMediaEncryption e);
LINPHONE_PUBLIC	void linphone_call_params_enable_early_media_sending(LinphoneCallParams *cp, bool_t enabled);
LINPHONE_PUBLIC	bool_t linphone_call_params_early_media_sending_enabled(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	bool_t linphone_call_params_local_conference_mode(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	void linphone_call_params_set_audio_bandwidth_limit(LinphoneCallParams *cp, int bw);
LINPHONE_PUBLIC	void linphone_call_params_destroy(LinphoneCallParams *cp);
LINPHONE_PUBLIC	bool_t linphone_call_params_low_bandwidth_enabled(const LinphoneCallParams *cp);
LINPHONE_PUBLIC	void linphone_call_params_enable_low_bandwidth(LinphoneCallParams *cp, bool_t enabled);
LINPHONE_PUBLIC	void linphone_call_params_set_record_file(LinphoneCallParams *cp, const char *path);
LINPHONE_PUBLIC	const char *linphone_call_params_get_record_file(const LinphoneCallParams *cp);
LINPHONE_PUBLIC const char *linphone_call_params_get_session_name(const LinphoneCallParams *cp);
LINPHONE_PUBLIC void linphone_call_params_set_session_name(LinphoneCallParams *cp, const char *subject);

/**
 * Add a custom SIP header in the INVITE for a call.
 * @param[in] params The #LinphoneCallParams to add a custom SIP header to.
 * @param[in] header_name The name of the header to add.
 * @param[in] header_value The content of the header to add.
 * @ingroup call_control
**/
LINPHONE_PUBLIC	void linphone_call_params_add_custom_header(LinphoneCallParams *params, const char *header_name, const char *header_value);

/**
 * Get a custom SIP header.
 * @param[in] params The #LinphoneCallParams to get the custom SIP header from.
 * @param[in] header_name The name of the header to get.
 * @returns The content of the header or NULL if not found.
 * @ingroup call_control
**/
LINPHONE_PUBLIC	const char *linphone_call_params_get_custom_header(const LinphoneCallParams *params, const char *header_name);

/**
 * Gets the size of the video that is sent.
 * @param[in] cp The call parameters for which to get the sent video size.
 * @return The sent video size or MS_VIDEO_SIZE_UNKNOWN if not available.
 */
LINPHONE_PUBLIC MSVideoSize linphone_call_params_get_sent_video_size(const LinphoneCallParams *cp);

/**
 * Gets the size of the video that is received.
 * @param[in] cp The call paramaters for which to get the received video size.
 * @return The received video size or MS_VIDEO_SIZE_UNKNOWN if not available.
 */
LINPHONE_PUBLIC MSVideoSize linphone_call_params_get_received_video_size(const LinphoneCallParams *cp);


/*
 * Note for developers: this enum must be kept synchronized with the SalPrivacy enum declared in sal.h
 */
/**
 * @ingroup call_control
 * Defines privacy policy to apply as described by rfc3323
**/
typedef enum _LinphonePrivacy {
	/**
	 * Privacy services must not perform any privacy function
	 */
	LinphonePrivacyNone=0x0,
	/**
	 * Request that privacy services provide a user-level privacy
	 * function.
	 * With this mode, "from" header is hidden, usually replaced by  From: "Anonymous" <sip:anonymous@anonymous.invalid>
	 */
	LinphonePrivacyUser=0x1,
	/**
	 * Request that privacy services modify headers that cannot
	 * be set arbitrarily by the user (Contact/Via).
	 */
	LinphonePrivacyHeader=0x2,
	/**
	 *  Request that privacy services provide privacy for session
	 * media
	 */
	LinphonePrivacySession=0x4,
	/**
	 * rfc3325
	 * The presence of this privacy type in
	 * a Privacy header field indicates that the user would like the Network
	 * Asserted Identity to be kept private with respect to SIP entities
	 * outside the Trust Domain with which the user authenticated.  Note
	 * that a user requesting multiple types of privacy MUST include all of
	 * the requested privacy types in its Privacy header field value
	 *
	 */
	LinphonePrivacyId=0x8,
	/**
	 * Privacy service must perform the specified services or
	 * fail the request
	 *
	 **/
	LinphonePrivacyCritical=0x10,

	/**
	 * Special keyword to use privacy as defined either globally or by proxy using linphone_proxy_config_set_privacy()
	 */
	LinphonePrivacyDefault=0x8000,
} LinphonePrivacy;
/*
 * a mask  of #LinphonePrivacy values
 * */
typedef unsigned int LinphonePrivacyMask;


LINPHONE_PUBLIC const char* linphone_privacy_to_string(LinphonePrivacy privacy);
LINPHONE_PUBLIC void linphone_call_params_set_privacy(LinphoneCallParams *params, LinphonePrivacyMask privacy);
LINPHONE_PUBLIC LinphonePrivacyMask linphone_call_params_get_privacy(const LinphoneCallParams *params);


struct _LinphoneInfoMessage;
/**
 * The LinphoneInfoMessage is an object representing an informational message sent or received by the core.
**/
typedef struct _LinphoneInfoMessage LinphoneInfoMessage;

LINPHONE_PUBLIC LinphoneInfoMessage *linphone_core_create_info_message(LinphoneCore*lc);
LINPHONE_PUBLIC int linphone_call_send_info_message(struct _LinphoneCall *call, const LinphoneInfoMessage *info);
LINPHONE_PUBLIC void linphone_info_message_add_header(LinphoneInfoMessage *im, const char *name, const char *value);
LINPHONE_PUBLIC const char *linphone_info_message_get_header(const LinphoneInfoMessage *im, const char *name);
LINPHONE_PUBLIC void linphone_info_message_set_content(LinphoneInfoMessage *im,  const LinphoneContent *content);
LINPHONE_PUBLIC const LinphoneContent * linphone_info_message_get_content(const LinphoneInfoMessage *im);
LINPHONE_PUBLIC const char *linphone_info_message_get_from(const LinphoneInfoMessage *im);
LINPHONE_PUBLIC void linphone_info_message_destroy(LinphoneInfoMessage *im);
LINPHONE_PUBLIC LinphoneInfoMessage *linphone_info_message_copy(const LinphoneInfoMessage *orig);



/**
 * Structure describing policy regarding video streams establishments.
 * @ingroup media_parameters
**/
struct _LinphoneVideoPolicy{
	bool_t automatically_initiate; /**<Whether video shall be automatically proposed for outgoing calls.*/
	bool_t automatically_accept; /**<Whether video shall be automatically accepted for incoming calls*/
	bool_t unused[2];
};

/**
 * Structure describing policy regarding video streams establishments.
 * @ingroup media_parameters
**/
typedef struct _LinphoneVideoPolicy LinphoneVideoPolicy;




/**
 * @addtogroup call_misc
 * @{
**/

#define LINPHONE_CALL_STATS_AUDIO 0
#define LINPHONE_CALL_STATS_VIDEO 1

/**
 * Enum describing ICE states.
 * @ingroup initializing
**/
enum _LinphoneIceState{
	LinphoneIceStateNotActivated, /**< ICE has not been activated for this call */
	LinphoneIceStateFailed, /**< ICE processing has failed */
	LinphoneIceStateInProgress, /**< ICE process is in progress */
	LinphoneIceStateHostConnection, /**< ICE has established a direct connection to the remote host */
	LinphoneIceStateReflexiveConnection, /**< ICE has established a connection to the remote host through one or several NATs */
	LinphoneIceStateRelayConnection /**< ICE has established a connection through a relay */
};

/**
 * Enum describing Ice states.
 * @ingroup initializing
**/
typedef enum _LinphoneIceState LinphoneIceState;

/**
 * Enum describing uPnP states.
 * @ingroup initializing
**/
enum _LinphoneUpnpState{
	LinphoneUpnpStateIdle, /**< uPnP is not activate */
	LinphoneUpnpStatePending, /**< uPnP process is in progress */
	LinphoneUpnpStateAdding,   /**< Internal use: Only used by port binding */
	LinphoneUpnpStateRemoving, /**< Internal use: Only used by port binding */
	LinphoneUpnpStateNotAvailable,  /**< uPnP is not available */
	LinphoneUpnpStateOk, /**< uPnP is enabled */
	LinphoneUpnpStateKo, /**< uPnP processing has failed */
	LinphoneUpnpStateBlacklisted, /**< IGD router is blacklisted */
};

/**
 * Enum describing uPnP states.
 * @ingroup initializing
**/
typedef enum _LinphoneUpnpState LinphoneUpnpState;


/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #CallStatsUpdated callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
typedef struct _LinphoneCallStats LinphoneCallStats;

/**
 * The LinphoneCallStats objects carries various statistic informations regarding quality of audio or video streams.
 *
 * To receive these informations periodically and as soon as they are computed, the application is invited to place a #CallStatsUpdated callback in the LinphoneCoreVTable structure
 * it passes for instanciating the LinphoneCore object (see linphone_core_new() ).
 *
 * At any time, the application can access last computed statistics using linphone_call_get_audio_stats() or linphone_call_get_video_stats().
**/
struct _LinphoneCallStats {
	int		type; /**< Can be either LINPHONE_CALL_STATS_AUDIO or LINPHONE_CALL_STATS_VIDEO*/
	jitter_stats_t	jitter_stats; /**<jitter buffer statistics, see oRTP documentation for details */
	mblk_t*		received_rtcp; /**<Last RTCP packet received, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	mblk_t*		sent_rtcp;/**<Last RTCP packet sent, as a mblk_t structure. See oRTP documentation for details how to extract information from it*/
	float		round_trip_delay; /**<Round trip propagation time in seconds if known, -1 if unknown.*/
	LinphoneIceState	ice_state; /**< State of ICE processing. */
	LinphoneUpnpState	upnp_state; /**< State of uPnP processing. */
	float download_bandwidth; /**<Download bandwidth measurement of received stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	float upload_bandwidth; /**<Download bandwidth measurement of sent stream, expressed in kbit/s, including IP/UDP/RTP headers*/
	float local_late_rate; /**<percentage of packet received too late over last second*/
	float local_loss_rate; /**<percentage of lost packet over last second*/
};

/**
 * @}
**/

LINPHONE_PUBLIC const LinphoneCallStats *linphone_call_get_audio_stats(LinphoneCall *call);
LINPHONE_PUBLIC const LinphoneCallStats *linphone_call_get_video_stats(LinphoneCall *call);
LINPHONE_PUBLIC float linphone_call_stats_get_sender_loss_rate(const LinphoneCallStats *stats);
LINPHONE_PUBLIC float linphone_call_stats_get_receiver_loss_rate(const LinphoneCallStats *stats);
LINPHONE_PUBLIC float linphone_call_stats_get_sender_interarrival_jitter(const LinphoneCallStats *stats, LinphoneCall *call);
LINPHONE_PUBLIC float linphone_call_stats_get_receiver_interarrival_jitter(const LinphoneCallStats *stats, LinphoneCall *call);
LINPHONE_PUBLIC uint64_t linphone_call_stats_get_late_packets_cumulative_number(const LinphoneCallStats *stats, LinphoneCall *call);

/** Callback prototype */
typedef void (*LinphoneCallCbFunc)(LinphoneCall *call,void * user_data);

/**
 * LinphoneCallState enum represents the different state a call can reach into.
 * The application is notified of state changes through the LinphoneCoreVTable::call_state_changed callback.
 * @ingroup call_control
**/
typedef enum _LinphoneCallState{
	LinphoneCallIdle,					/**<Initial call state */
	LinphoneCallIncomingReceived, /**<This is a new incoming call */
	LinphoneCallOutgoingInit, /**<An outgoing call is started */
	LinphoneCallOutgoingProgress, /**<An outgoing call is in progress */
	LinphoneCallOutgoingRinging, /**<An outgoing call is ringing at remote end */
	LinphoneCallOutgoingEarlyMedia, /**<An outgoing call is proposed early media */
	LinphoneCallConnected, /**<Connected, the call is answered */
	LinphoneCallStreamsRunning, /**<The media streams are established and running*/
	LinphoneCallPausing, /**<The call is pausing at the initiative of local end */
	LinphoneCallPaused, /**< The call is paused, remote end has accepted the pause */
	LinphoneCallResuming, /**<The call is being resumed by local end*/
	LinphoneCallRefered, /**<The call is being transfered to another party, resulting in a new outgoing call to follow immediately*/
	LinphoneCallError, /**<The call encountered an error*/
	LinphoneCallEnd, /**<The call ended normally*/
	LinphoneCallPausedByRemote, /**<The call is paused by remote end*/
	LinphoneCallUpdatedByRemote, /**<The call's parameters change is requested by remote end, used for example when video is added by remote */
	LinphoneCallIncomingEarlyMedia, /**<We are proposing early media to an incoming call */
	LinphoneCallUpdating, /**<A call update has been initiated by us */
	LinphoneCallReleased /**< The call object is no more retained by the core */
} LinphoneCallState;

LINPHONE_PUBLIC	const char *linphone_call_state_to_string(LinphoneCallState cs);

LINPHONE_PUBLIC LinphoneCore *linphone_call_get_core(const LinphoneCall *call);
LINPHONE_PUBLIC	LinphoneCallState linphone_call_get_state(const LinphoneCall *call);
LINPHONE_PUBLIC bool_t linphone_call_asked_to_autoanswer(LinphoneCall *call);
LINPHONE_PUBLIC	const LinphoneAddress * linphone_core_get_current_call_remote_address(LinphoneCore *lc);
LINPHONE_PUBLIC	const LinphoneAddress * linphone_call_get_remote_address(const LinphoneCall *call);
LINPHONE_PUBLIC	char *linphone_call_get_remote_address_as_string(const LinphoneCall *call);
LINPHONE_PUBLIC	LinphoneCallDir linphone_call_get_dir(const LinphoneCall *call);
LINPHONE_PUBLIC	LinphoneCall * linphone_call_ref(LinphoneCall *call);
LINPHONE_PUBLIC	void linphone_call_unref(LinphoneCall *call);
LINPHONE_PUBLIC	LinphoneCallLog *linphone_call_get_call_log(const LinphoneCall *call);
LINPHONE_PUBLIC const char *linphone_call_get_refer_to(const LinphoneCall *call);
LINPHONE_PUBLIC bool_t linphone_call_has_transfer_pending(const LinphoneCall *call);
LINPHONE_PUBLIC LinphoneCall *linphone_call_get_transferer_call(const LinphoneCall *call);
LINPHONE_PUBLIC LinphoneCall *linphone_call_get_transfer_target_call(const LinphoneCall *call);
LINPHONE_PUBLIC	LinphoneCall *linphone_call_get_replaced_call(LinphoneCall *call);
LINPHONE_PUBLIC	int linphone_call_get_duration(const LinphoneCall *call);
LINPHONE_PUBLIC	const LinphoneCallParams * linphone_call_get_current_params(LinphoneCall *call);
LINPHONE_PUBLIC	const LinphoneCallParams * linphone_call_get_remote_params(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_call_enable_camera(LinphoneCall *lc, bool_t enabled);
LINPHONE_PUBLIC bool_t linphone_call_camera_enabled(const LinphoneCall *lc);
LINPHONE_PUBLIC int linphone_call_take_video_snapshot(LinphoneCall *call, const char *file);
LINPHONE_PUBLIC	LinphoneReason linphone_call_get_reason(const LinphoneCall *call);
LINPHONE_PUBLIC	const char *linphone_call_get_remote_user_agent(LinphoneCall *call);
LINPHONE_PUBLIC	const char *linphone_call_get_remote_contact(LinphoneCall *call);
LINPHONE_PUBLIC LinphoneAddress *linphone_call_get_remote_contact_address(LinphoneCall *call);
LINPHONE_PUBLIC	float linphone_call_get_play_volume(LinphoneCall *call);
LINPHONE_PUBLIC	float linphone_call_get_record_volume(LinphoneCall *call);
LINPHONE_PUBLIC	float linphone_call_get_current_quality(LinphoneCall *call);
LINPHONE_PUBLIC	float linphone_call_get_average_quality(LinphoneCall *call);
LINPHONE_PUBLIC	const char* linphone_call_get_authentication_token(LinphoneCall *call);
LINPHONE_PUBLIC	bool_t linphone_call_get_authentication_token_verified(LinphoneCall *call);
LINPHONE_PUBLIC	void linphone_call_set_authentication_token_verified(LinphoneCall *call, bool_t verified);
LINPHONE_PUBLIC void linphone_call_send_vfu_request(LinphoneCall *call);
LINPHONE_PUBLIC void *linphone_call_get_user_pointer(LinphoneCall *call);
LINPHONE_PUBLIC	void linphone_call_set_user_pointer(LinphoneCall *call, void *user_pointer);
LINPHONE_PUBLIC	void linphone_call_set_next_video_frame_decoded_callback(LinphoneCall *call, LinphoneCallCbFunc cb, void* user_data);
LINPHONE_PUBLIC LinphoneCallState linphone_call_get_transfer_state(LinphoneCall *call);
LINPHONE_PUBLIC void linphone_call_zoom_video(LinphoneCall* call, float zoom_factor, float* cx, float* cy);
LINPHONE_PUBLIC	void linphone_call_start_recording(LinphoneCall *call);
LINPHONE_PUBLIC	void linphone_call_stop_recording(LinphoneCall *call);

/**
 * Return TRUE if this call is currently part of a conference
 * @param call #LinphoneCall
 * @return TRUE if part of a conference.
 *
 * @deprecated
 * @ingroup call_control
 */
LINPHONE_PUBLIC	bool_t linphone_call_is_in_conference(const LinphoneCall *call);
/**
 * Enables or disable echo cancellation for this call
 * @param call
 * @param val
 *
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC	void linphone_call_enable_echo_cancellation(LinphoneCall *call, bool_t val) ;
/**
 * Returns TRUE if echo cancellation is enabled.
 *
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC	bool_t linphone_call_echo_cancellation_enabled(LinphoneCall *lc);
/**
 * Enables or disable echo limiter for this call
 * @param call
 * @param val
 *
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC	void linphone_call_enable_echo_limiter(LinphoneCall *call, bool_t val);
/**
 * Returns TRUE if echo limiter is enabled.
 *
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC	bool_t linphone_call_echo_limiter_enabled(const LinphoneCall *call);

/*keep this in sync with mediastreamer2/msvolume.h*/

/**
 * Lowest volume measurement that can be returned by linphone_call_get_play_volume() or linphone_call_get_record_volume(), corresponding to pure silence.
 * @ingroup call_misc
**/
#define LINPHONE_VOLUME_DB_LOWEST (-120)

/**
 * @addtogroup proxies
 * @{
**/
/**
 * The LinphoneProxyConfig object represents a proxy configuration to be used
 * by the LinphoneCore object.
 * Its fields must not be used directly in favour of the accessors methods.
 * Once created and filled properly the LinphoneProxyConfig can be given to
 * LinphoneCore with linphone_core_add_proxy_config().
 * This will automatically triggers the registration, if enabled.
 *
 * The proxy configuration are persistent to restarts because they are saved
 * in the configuration file. As a consequence, after linphone_core_new() there
 * might already be a list of configured proxy that can be examined with
 * linphone_core_get_proxy_config_list().
 *
 * The default proxy (see linphone_core_set_default_proxy() ) is the one of the list
 * that is used by default for calls.
**/
typedef struct _LinphoneProxyConfig LinphoneProxyConfig;

/**
 * LinphoneRegistrationState describes proxy registration states.
**/
typedef enum _LinphoneRegistrationState{
	LinphoneRegistrationNone, /**<Initial state for registrations */
	LinphoneRegistrationProgress, /**<Registration is in progress */
	LinphoneRegistrationOk,	/**< Registration is successful */
	LinphoneRegistrationCleared, /**< Unregistration succeeded */
	LinphoneRegistrationFailed	/**<Registration failed */
}LinphoneRegistrationState;

/**
 * Human readable version of the #LinphoneRegistrationState
 * @param cs sate
 */
LINPHONE_PUBLIC	const char *linphone_registration_state_to_string(LinphoneRegistrationState cs);
LINPHONE_PUBLIC	LinphoneProxyConfig *linphone_proxy_config_new(void);
LINPHONE_PUBLIC	int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *obj, const char *server_addr);
LINPHONE_PUBLIC	int linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity);
LINPHONE_PUBLIC	int linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route);
LINPHONE_PUBLIC	void linphone_proxy_config_set_expires(LinphoneProxyConfig *obj, int expires);
#define linphone_proxy_config_expires linphone_proxy_config_set_expires
/**
 * Indicates  either or not, REGISTRATION must be issued for this #LinphoneProxyConfig .
 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
 * @param obj object pointer
 * @param val if true, registration will be engaged
 */
LINPHONE_PUBLIC	void linphone_proxy_config_enable_register(LinphoneProxyConfig *obj, bool_t val);
#define linphone_proxy_config_enableregister linphone_proxy_config_enable_register
LINPHONE_PUBLIC	void linphone_proxy_config_edit(LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	int linphone_proxy_config_done(LinphoneProxyConfig *obj);
/**
 * Indicates  either or not, PUBLISH must be issued for this #LinphoneProxyConfig .
 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
 * @param obj object pointer
 * @param val if true, publish will be engaged
 *
 */
LINPHONE_PUBLIC	void linphone_proxy_config_enable_publish(LinphoneProxyConfig *obj, bool_t val);
LINPHONE_PUBLIC	void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val);
LINPHONE_PUBLIC	void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix);

/**
 * Get the registration state of the given proxy config.
 * @param[in] obj #LinphoneProxyConfig object.
 * @returns The registration state of the proxy config.
**/
LINPHONE_PUBLIC	LinphoneRegistrationState linphone_proxy_config_get_state(const LinphoneProxyConfig *obj);

LINPHONE_PUBLIC	bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *obj);

/**
 * Get the domain name of the given proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @returns The domain name of the proxy config.
**/
LINPHONE_PUBLIC	const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg);

LINPHONE_PUBLIC	const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	const char *linphone_proxy_config_get_addr(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	int linphone_proxy_config_get_expires(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	void linphone_proxy_config_refresh_register(LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	const char *linphone_proxy_config_get_contact_parameters(const LinphoneProxyConfig *obj);
LINPHONE_PUBLIC	void linphone_proxy_config_set_contact_parameters(LinphoneProxyConfig *obj, const char *contact_params);
LINPHONE_PUBLIC void linphone_proxy_config_set_contact_uri_parameters(LinphoneProxyConfig *obj, const char *contact_uri_params);
LINPHONE_PUBLIC const char* linphone_proxy_config_get_contact_uri_parameters(const LinphoneProxyConfig *obj);

/**
 * Get the #LinphoneCore object to which is associated the #LinphoneProxyConfig.
 * @param[in] obj #LinphoneProxyConfig object.
 * @returns The #LinphoneCore object to which is associated the #LinphoneProxyConfig.
**/
LINPHONE_PUBLIC LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *obj);

LINPHONE_PUBLIC	bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg);
LINPHONE_PUBLIC	const char * linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg);

/**
 * Get the reason why registration failed when the proxy config state is LinphoneRegistrationFailed.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @returns The reason why registration failed for this proxy config.
**/
LINPHONE_PUBLIC LinphoneReason linphone_proxy_config_get_error(const LinphoneProxyConfig *cfg);

/*
 * return the transport from either : service route, route, or addr
 * @returns cfg object
 * @return transport as string (I.E udp, tcp, tls, dtls)*/

LINPHONE_PUBLIC const char* linphone_proxy_config_get_transport(const LinphoneProxyConfig *cfg);


/* destruction is called automatically when removing the proxy config */
LINPHONE_PUBLIC void linphone_proxy_config_destroy(LinphoneProxyConfig *cfg);
LINPHONE_PUBLIC void linphone_proxy_config_set_sip_setup(LinphoneProxyConfig *cfg, const char *type);
SipSetupContext *linphone_proxy_config_get_sip_setup_context(LinphoneProxyConfig *cfg);
LINPHONE_PUBLIC SipSetup *linphone_proxy_config_get_sip_setup(LinphoneProxyConfig *cfg);
/**
 * normalize a human readable phone number into a basic string. 888-444-222 becomes 888444222
 */
LINPHONE_PUBLIC	int linphone_proxy_config_normalize_number(LinphoneProxyConfig *proxy, const char *username, char *result, size_t result_len);
/*
 *  attached a user data to a proxy config
 */
LINPHONE_PUBLIC	void linphone_proxy_config_set_user_data(LinphoneProxyConfig *cr, void * ud);
/*
 *  get user data to a proxy config. return null if any
 */
LINPHONE_PUBLIC	void * linphone_proxy_config_get_user_data(LinphoneProxyConfig *cr);

/**
 * Set default privacy policy for all calls routed through this proxy.
 * @param params to be modified
 * @param LinphonePrivacy to configure privacy
 * */
LINPHONE_PUBLIC void linphone_proxy_config_set_privacy(LinphoneProxyConfig *params, LinphonePrivacyMask privacy);
/**
 * Get default privacy policy for all calls routed through this proxy.
 * @param params object
 * @return Privacy mode
 * */
LINPHONE_PUBLIC LinphonePrivacyMask linphone_proxy_config_get_privacy(const LinphoneProxyConfig *params);

/**
 * @}
**/

typedef struct _LinphoneAccountCreator{
	LinphoneCore *lc;
	struct _SipSetupContext *ssctx;
	char *username;
	char *password;
	char *domain;
	char *route;
	char *email;
	int suscribe;
	bool_t succeeded;
}LinphoneAccountCreator;

LinphoneAccountCreator *linphone_account_creator_new(LinphoneCore *core, const char *type);
void linphone_account_creator_set_username(LinphoneAccountCreator *obj, const char *username);
void linphone_account_creator_set_password(LinphoneAccountCreator *obj, const char *password);
void linphone_account_creator_set_domain(LinphoneAccountCreator *obj, const char *domain);
void linphone_account_creator_set_route(LinphoneAccountCreator *obj, const char *route);
void linphone_account_creator_set_email(LinphoneAccountCreator *obj, const char *email);
void linphone_account_creator_set_suscribe(LinphoneAccountCreator *obj, int suscribre);
const char * linphone_account_creator_get_username(LinphoneAccountCreator *obj);
const char * linphone_account_creator_get_domain(LinphoneAccountCreator *obj);
int linphone_account_creator_test_existence(LinphoneAccountCreator *obj);
int linphone_account_creator_test_validation(LinphoneAccountCreator *obj);
LinphoneProxyConfig * linphone_account_creator_validate(LinphoneAccountCreator *obj);
void linphone_account_creator_destroy(LinphoneAccountCreator *obj);

struct _LinphoneAuthInfo;

/**
 * @ingroup authentication
 * Object holding authentication information.
 *
 * @note The object's fields should not be accessed directly. Prefer using
 * the accessor methods.
 *
 * In most case, authentication information consists of a username and password.
 * Sometimes, a userid is required by proxy, and realm can be useful to discriminate
 * different SIP domains.
 *
 * Once created and filled, a LinphoneAuthInfo must be added to the LinphoneCore in
 * order to become known and used automatically when needed.
 * Use linphone_core_add_auth_info() for that purpose.
 *
 * The LinphoneCore object can take the initiative to request authentication information
 * when needed to the application through the auth_info_requested callback of the
 * LinphoneCoreVTable structure.
 *
 * The application can respond to this information request later using
 * linphone_core_add_auth_info(). This will unblock all pending authentication
 * transactions and retry them with authentication headers.
 *
**/
typedef struct _LinphoneAuthInfo LinphoneAuthInfo;

LINPHONE_PUBLIC	LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid,
		const char *passwd, const char *ha1,const char *realm, const char *domain);
/**
 * @addtogroup authentication
 * Instanciate a new auth info with values from source
 * @param source auth info object to be cloned
 * @return newly created auth info
 */
LINPHONE_PUBLIC	LinphoneAuthInfo *linphone_auth_info_clone(const LinphoneAuthInfo* source);
LINPHONE_PUBLIC void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd);
LINPHONE_PUBLIC void linphone_auth_info_set_username(LinphoneAuthInfo *info, const char *username);
LINPHONE_PUBLIC void linphone_auth_info_set_userid(LinphoneAuthInfo *info, const char *userid);
LINPHONE_PUBLIC void linphone_auth_info_set_realm(LinphoneAuthInfo *info, const char *realm);
LINPHONE_PUBLIC void linphone_auth_info_set_domain(LinphoneAuthInfo *info, const char *domain);
LINPHONE_PUBLIC void linphone_auth_info_set_ha1(LinphoneAuthInfo *info, const char *ha1);

LINPHONE_PUBLIC const char *linphone_auth_info_get_username(const LinphoneAuthInfo *i);
LINPHONE_PUBLIC const char *linphone_auth_info_get_passwd(const LinphoneAuthInfo *i);
LINPHONE_PUBLIC const char *linphone_auth_info_get_userid(const LinphoneAuthInfo *i);
LINPHONE_PUBLIC const char *linphone_auth_info_get_realm(const LinphoneAuthInfo *i);
LINPHONE_PUBLIC const char *linphone_auth_info_get_domain(const LinphoneAuthInfo *i);
LINPHONE_PUBLIC const char *linphone_auth_info_get_ha1(const LinphoneAuthInfo *i);

/* you don't need those function*/
LINPHONE_PUBLIC void linphone_auth_info_destroy(LinphoneAuthInfo *info);
LINPHONE_PUBLIC LinphoneAuthInfo * linphone_auth_info_new_from_config_file(LpConfig *config, int pos);


struct _LinphoneChatRoom;
/**
 * @addtogroup chatroom
 * @{
 */

/**
 * A chat room message to old content to be sent.
 * <br> Can be created by linphone_chat_room_create_message().
 */
typedef struct _LinphoneChatMessage LinphoneChatMessage;

/**
 * A chat room is the place where text messages are exchanged.
 * <br> Can be created by linphone_core_create_chat_room().
 */
typedef struct _LinphoneChatRoom LinphoneChatRoom;

/**
 * LinphoneChatMessageState is used to notify if messages have been succesfully delivered or not.
 */
typedef enum _LinphoneChatMessageState {
	LinphoneChatMessageStateIdle, /**< Initial state */
	LinphoneChatMessageStateInProgress, /**< Delivery in progress */
	LinphoneChatMessageStateDelivered, /**< Message succesffully delivered an acknoleged by remote end point */
	LinphoneChatMessageStateNotDelivered /**< Message was not delivered */
} LinphoneChatMessageState;

/**
 * Call back used to notify message delivery status
 *@param msg #LinphoneChatMessage object
 *@param status LinphoneChatMessageState
 *@param ud application user data
 */
typedef void (*LinphoneChatMessageStateChangedCb)(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud);

LINPHONE_PUBLIC void linphone_core_set_chat_database_path(LinphoneCore *lc, const char *path);
LINPHONE_PUBLIC	LinphoneChatRoom * linphone_core_create_chat_room(LinphoneCore *lc, const char *to);
LINPHONE_PUBLIC	LinphoneChatRoom * linphone_core_get_or_create_chat_room(LinphoneCore *lc, const char *to);
LINPHONE_PUBLIC LinphoneChatRoom *linphone_core_get_chat_room(LinphoneCore *lc, const LinphoneAddress *addr);
LINPHONE_PUBLIC void linphone_chat_room_destroy(LinphoneChatRoom *cr);
LINPHONE_PUBLIC	LinphoneChatMessage* linphone_chat_room_create_message(LinphoneChatRoom *cr,const char* message);
LINPHONE_PUBLIC	LinphoneChatMessage* linphone_chat_room_create_message_2(LinphoneChatRoom *cr, const char* message, const char* external_body_url, LinphoneChatMessageState state, time_t time, bool_t is_read, bool_t is_incoming);
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_room_get_peer_address(LinphoneChatRoom *cr);
LINPHONE_PUBLIC	void linphone_chat_room_send_message(LinphoneChatRoom *cr, const char *msg);
LINPHONE_PUBLIC	void linphone_chat_room_send_message2(LinphoneChatRoom *cr, LinphoneChatMessage* msg,LinphoneChatMessageStateChangedCb status_cb,void* ud);
LINPHONE_PUBLIC void linphone_chat_room_update_url(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
LINPHONE_PUBLIC MSList *linphone_chat_room_get_history(LinphoneChatRoom *cr,int nb_message);
LINPHONE_PUBLIC void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr);
LINPHONE_PUBLIC void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg);
LINPHONE_PUBLIC void linphone_chat_room_delete_history(LinphoneChatRoom *cr);

/**
 * Notify the destination of the chat message being composed that the user is typing a new message.
 * @param[in] cr The #LinphoneChatRoom object corresponding to the conversation for which a new message is being typed.
 */
LINPHONE_PUBLIC void linphone_chat_room_compose(LinphoneChatRoom *cr);

/**
 * Tells whether the remote is currently composing a message.
 * @param[in] cr The "LinphoneChatRoom object corresponding to the conversation.
 * @return TRUE if the remote is currently composing a message, FALSE otherwise.
 */
LINPHONE_PUBLIC bool_t linphone_chat_room_is_remote_composing(const LinphoneChatRoom *cr);

LINPHONE_PUBLIC int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr);
LINPHONE_PUBLIC LinphoneCore* linphone_chat_room_get_lc(LinphoneChatRoom *cr);
LINPHONE_PUBLIC	void linphone_chat_room_set_user_data(LinphoneChatRoom *cr, void * ud);
LINPHONE_PUBLIC	void * linphone_chat_room_get_user_data(LinphoneChatRoom *cr);
LINPHONE_PUBLIC MSList* linphone_core_get_chat_rooms(LinphoneCore *lc);
LINPHONE_PUBLIC unsigned int linphone_chat_message_store(LinphoneChatMessage *msg);

LINPHONE_PUBLIC	const char* linphone_chat_message_state_to_string(const LinphoneChatMessageState state);
LINPHONE_PUBLIC	LinphoneChatMessageState linphone_chat_message_get_state(const LinphoneChatMessage* message);
LINPHONE_PUBLIC LinphoneChatMessage* linphone_chat_message_clone(const LinphoneChatMessage* message);
LINPHONE_PUBLIC void linphone_chat_message_destroy(LinphoneChatMessage* msg);
LINPHONE_PUBLIC void linphone_chat_message_set_from(LinphoneChatMessage* message, const LinphoneAddress* from);
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_message_get_from(const LinphoneChatMessage* message);
LINPHONE_PUBLIC void linphone_chat_message_set_to(LinphoneChatMessage* message, const LinphoneAddress* from);
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_message_get_to(const LinphoneChatMessage* message);
LINPHONE_PUBLIC	const char* linphone_chat_message_get_external_body_url(const LinphoneChatMessage* message);
LINPHONE_PUBLIC	void linphone_chat_message_set_external_body_url(LinphoneChatMessage* message,const char* url);
LINPHONE_PUBLIC	const char * linphone_chat_message_get_text(const LinphoneChatMessage* message);
LINPHONE_PUBLIC	time_t linphone_chat_message_get_time(const LinphoneChatMessage* message);
LINPHONE_PUBLIC	void* linphone_chat_message_get_user_data(const LinphoneChatMessage* message);
LINPHONE_PUBLIC	void linphone_chat_message_set_user_data(LinphoneChatMessage* message,void*);
LINPHONE_PUBLIC LinphoneChatRoom* linphone_chat_message_get_chat_room(LinphoneChatMessage *msg);
LINPHONE_PUBLIC	const LinphoneAddress* linphone_chat_message_get_peer_address(LinphoneChatMessage *msg);
LINPHONE_PUBLIC	LinphoneAddress *linphone_chat_message_get_local_address(const LinphoneChatMessage* message);
LINPHONE_PUBLIC	void linphone_chat_message_add_custom_header(LinphoneChatMessage* message, const char *header_name, const char *header_value);
LINPHONE_PUBLIC	const char * linphone_chat_message_get_custom_header(LinphoneChatMessage* message, const char *header_name);
LINPHONE_PUBLIC bool_t linphone_chat_message_is_read(LinphoneChatMessage* message);
LINPHONE_PUBLIC bool_t linphone_chat_message_is_outgoing(LinphoneChatMessage* message);
LINPHONE_PUBLIC unsigned int linphone_chat_message_get_storage_id(LinphoneChatMessage* message);
LINPHONE_PUBLIC LinphoneReason linphone_chat_message_get_reason(LinphoneChatMessage* msg);
/**
 * @}
 */


/**
 * @addtogroup initializing
 * @{
**/

/**
 * LinphoneGlobalState describes the global state of the LinphoneCore object.
 * It is notified via the LinphoneCoreVTable::global_state_changed
**/
typedef enum _LinphoneGlobalState{
	LinphoneGlobalOff,
	LinphoneGlobalStartup,
	LinphoneGlobalOn,
	LinphoneGlobalShutdown,
	LinphoneGlobalConfiguring
}LinphoneGlobalState;

const char *linphone_global_state_to_string(LinphoneGlobalState gs);


/**
 * Global state notification callback.
 * @param lc
 * @param gstate the global state
 * @param message informational message.
 */
typedef void (*LinphoneCoreGlobalStateChangedCb )(LinphoneCore *lc, LinphoneGlobalState gstate, const char *message);
/**
 * Call state notification callback.
 * @param lc the LinphoneCore
 * @param call the call object whose state is changed.
 * @param cstate the new state of the call
 * @param message an informational message about the state.
 */
typedef void (*LinphoneCoreCallStateChangedCb)(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *message);

/**
 * Call encryption changed callback.
 * @param lc the LinphoneCore
 * @param call the call on which encryption is changed.
 * @param on whether encryption is activated.
 * @param authentication_token an authentication_token, currently set for ZRTP kind of encryption only.
 */
typedef void (*LinphoneCoreCallEncryptionChangedCb)(LinphoneCore *lc, LinphoneCall *call, bool_t on, const char *authentication_token);

/** @ingroup Proxies
 * Registration state notification callback prototype
 * */
typedef void (*LinphoneCoreRegistrationStateChangedCb)(LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
/** Callback prototype
 * @deprecated
 */
typedef void (*ShowInterfaceCb)(LinphoneCore *lc);
/** Callback prototype
 * @deprecated
 */
typedef void (*DisplayStatusCb)(LinphoneCore *lc, const char *message);
/** Callback prototype
 * @deprecated
 */
typedef void (*DisplayMessageCb)(LinphoneCore *lc, const char *message);
/** Callback prototype
 * @deprecated
 */
typedef void (*DisplayUrlCb)(LinphoneCore *lc, const char *message, const char *url);
/** Callback prototype
 */
typedef void (*LinphoneCoreCbFunc)(LinphoneCore *lc,void * user_data);
/**
 * Report status change for a friend previously \link linphone_core_add_friend() added \endlink to #LinphoneCore.
 * @param lc #LinphoneCore object .
 * @param lf Updated #LinphoneFriend .
 */
typedef void (*LinphoneCoreNotifyPresenceReceivedCb)(LinphoneCore *lc, LinphoneFriend * lf);
/**
 *  Reports that a new subscription request has been received and wait for a decision.
 *  Status on this subscription request is notified by \link linphone_friend_set_inc_subscribe_policy() changing policy \endlink for this friend
 * @param lc #LinphoneCore object
 * @param lf #LinphoneFriend corresponding to the subscriber
 * @param url of the subscriber
 *  Callback prototype
 */
typedef void (*LinphoneCoreNewSubscriptionRequestedCb)(LinphoneCore *lc, LinphoneFriend *lf, const char *url);
/**
 * Callback for requesting authentication information to application or user.
 * @param lc the LinphoneCore
 * @param realm the realm (domain) on which authentication is required.
 * @param username the username that needs to be authenticated.
 * Application shall reply to this callback using linphone_core_add_auth_info().
 */
typedef void (*LinphoneCoreAuthInfoRequestedCb)(LinphoneCore *lc, const char *realm, const char *username, const char *domain);

/**
 * Callback to notify a new call-log entry has been added.
 * This is done typically when a call terminates.
 * @param lc the LinphoneCore
 * @param newcl the new call log entry added.
 */
typedef void (*LinphoneCoreCallLogUpdatedCb)(LinphoneCore *lc, LinphoneCallLog *newcl);

/**
 * Callback prototype
 * @deprecated use #LinphoneMessageReceived instead.
 *
 * @param lc #LinphoneCore object
 * @param room #LinphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param from #LinphoneAddress from
 * @param message incoming message
 */
typedef void (*LinphoneCoreTextMessageReceivedCb)(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from, const char *message);

/**
 * Chat message callback prototype
 *
 * @param lc #LinphoneCore object
 * @param room #LinphoneChatRoom involved in this conversation. Can be be created by the framework in case \link #LinphoneAddress the from \endlink is not present in any chat room.
 * @param LinphoneChatMessage incoming message
 */
typedef void (*LinphoneCoreMessageReceivedCb)(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message);

/**
 * Is composing notification callback prototype.
 *
 * @param[in] lc #LinphoneCore object
 * @param[in] room #LinphoneChatRoom involved in the conversation.
 */
typedef void (*LinphoneCoreIsComposingReceivedCb)(LinphoneCore *lc, LinphoneChatRoom *room);

/**
 * Callback for being notified of DTMFs received.
 * @param lc the linphone core
 * @param call the call that received the dtmf
 * @param dtmf the ascii code of the dtmf
 */
typedef void (*LinphoneCoreDtmfReceivedCb)(LinphoneCore* lc, LinphoneCall *call, int dtmf);

/** Callback prototype */
typedef void (*LinphoneCoreReferReceivedCb)(LinphoneCore *lc, const char *refer_to);
/** Callback prototype */
typedef void (*LinphoneCoreBuddyInfoUpdatedCb)(LinphoneCore *lc, LinphoneFriend *lf);
/**
 * Callback for notifying progresses of transfers.
 * @param lc the LinphoneCore
 * @param transfered the call that was transfered
 * @param new_call_state the state of the call to transfer target at the far end.
 */
typedef void (*LinphoneCoreTransferStateChangedCb)(LinphoneCore *lc, LinphoneCall *transfered, LinphoneCallState new_call_state);

/**
 * Callback for receiving quality statistics for calls.
 * @param lc the LinphoneCore
 * @param call the call
 * @param stats the call statistics.
 */
typedef void (*LinphoneCoreCallStatsUpdatedCb)(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats);

/**
 * Callback prototype for receiving info messages.
 * @param lc the LinphoneCore
 * @param call the call whose info message belongs to.
 * @param msg the info message.
 */
typedef void (*LinphoneCoreInfoReceivedCb)(LinphoneCore *lc, LinphoneCall *call, const LinphoneInfoMessage *msg);

/**
 * LinphoneGlobalState describes the global state of the LinphoneCore object.
 * It is notified via the LinphoneCoreVTable::global_state_changed
**/
typedef enum _LinphoneConfiguringState {
	LinphoneConfiguringSuccessful,
	LinphoneConfiguringFailed,
	LinphoneConfiguringSkipped
} LinphoneConfiguringState;

/**
 * Callback prototype for configuring status changes notification
 * @param lc the LinphoneCore
 * @param message informational message.
 */
typedef void (*LinphoneCoreConfiguringStatusCb)(LinphoneCore *lc, LinphoneConfiguringState status, const char *message);

/**
 * This structure holds all callbacks that the application should implement.
 *  None is mandatory.
**/
typedef struct _LinphoneCoreVTable{
	LinphoneCoreGlobalStateChangedCb global_state_changed; /**<Notifies global state changes*/
	LinphoneCoreRegistrationStateChangedCb registration_state_changed;/**<Notifies registration state changes*/
	LinphoneCoreCallStateChangedCb call_state_changed;/**<Notifies call state changes*/
	LinphoneCoreNotifyPresenceReceivedCb notify_presence_received; /**< Notify received presence events*/
	LinphoneCoreNewSubscriptionRequestedCb new_subscription_requested; /**< Notify about pending presence subscription request */
	LinphoneCoreAuthInfoRequestedCb auth_info_requested; /**< Ask the application some authentication information */
	LinphoneCoreCallLogUpdatedCb call_log_updated; /**< Notifies that call log list has been updated */
	LinphoneCoreMessageReceivedCb message_received; /** a message is received, can be text or external body*/
	LinphoneCoreIsComposingReceivedCb is_composing_received; /**< An is-composing notification has been received */
	LinphoneCoreDtmfReceivedCb dtmf_received; /**< A dtmf has been received received */
	LinphoneCoreReferReceivedCb refer_received; /**< An out of call refer was received */
	LinphoneCoreCallEncryptionChangedCb call_encryption_changed; /**<Notifies on change in the encryption of call streams */
	LinphoneCoreTransferStateChangedCb transfer_state_changed; /**<Notifies when a transfer is in progress */
	LinphoneCoreBuddyInfoUpdatedCb buddy_info_updated; /**< a LinphoneFriend's BuddyInfo has changed*/
	LinphoneCoreCallStatsUpdatedCb call_stats_updated; /**<Notifies on refreshing of call's statistics. */
	LinphoneCoreInfoReceivedCb info_received; /**<Notifies an incoming informational message received.*/
	LinphoneCoreSubscriptionStateChangedCb subscription_state_changed; /**<Notifies subscription state change */
	LinphoneCoreNotifyReceivedCb notify_received; /**< Notifies a an event notification, see linphone_core_subscribe() */
	LinphoneCorePublishStateChangedCb publish_state_changed;/**Notifies publish state change (only from #LinphoneEvent api)*/
	LinphoneCoreConfiguringStatusCb configuring_status; /** Notifies configuring status changes */
	DisplayStatusCb display_status; /**< @deprecated Callback that notifies various events with human readable text.*/
	DisplayMessageCb display_message;/**< @deprecated Callback to display a message to the user */
	DisplayMessageCb display_warning;/**< @deprecated Callback to display a warning to the user */
	DisplayUrlCb display_url; /**< @deprecated */
	ShowInterfaceCb show; /**< @deprecated Notifies the application that it should show up*/
	LinphoneCoreTextMessageReceivedCb text_received; /** @deprecated, use #message_received instead <br> A text message has been received */
} LinphoneCoreVTable;

/**
 * @}
**/

typedef struct _LCCallbackObj
{
  LinphoneCoreCbFunc _func;
  void * _user_data;
}LCCallbackObj;


/**
 * Policy to use to pass through firewalls.
 * @ingroup network_parameters
**/
typedef enum _LinphoneFirewallPolicy {
	LinphonePolicyNoFirewall, /**< Do not use any mechanism to pass through firewalls */
	LinphonePolicyUseNatAddress, /**< Use the specified public adress */
	LinphonePolicyUseStun, /**< Use a STUN server to get the public address */
	LinphonePolicyUseIce, /**< Use the ICE protocol */
	LinphonePolicyUseUpnp, /**< Use the uPnP protocol */
} LinphoneFirewallPolicy;

typedef enum _LinphoneWaitingState{
	LinphoneWaitingStart,
	LinphoneWaitingProgress,
	LinphoneWaitingFinished
} LinphoneWaitingState;
typedef void * (*LinphoneCoreWaitingCallback)(LinphoneCore *lc, void *context, LinphoneWaitingState ws, const char *purpose, float progress);


/* THE main API */

/**
 * Define a log handler.
 *
 * @ingroup misc
 *
 * @param logfunc The function pointer of the log handler.
 */
LINPHONE_PUBLIC void linphone_core_set_log_handler(OrtpLogFunc logfunc);
/**
 * Define a log file.
 *
 * @ingroup misc
 *
 * If the file pointer passed as an argument is NULL, stdout is used instead.
 *
 * @param file A pointer to the FILE structure of the file to write to.
 */
LINPHONE_PUBLIC void linphone_core_set_log_file(FILE *file);
/**
 * Define the log level.
 *
 * @ingroup misc
 *
 * The loglevel parameter is a bitmask parameter. Therefore to enable only warning and error
 * messages, use ORTP_WARNING | ORTP_ERROR. To disable logs, simply set loglevel to 0.
 *
 * @param loglevel A bitmask of the log levels to set.
 */
LINPHONE_PUBLIC void linphone_core_set_log_level(OrtpLogLevel loglevel);
LINPHONE_PUBLIC void linphone_core_enable_logs(FILE *file);
LINPHONE_PUBLIC void linphone_core_enable_logs_with_cb(OrtpLogFunc logfunc);
LINPHONE_PUBLIC void linphone_core_disable_logs(void);
LINPHONE_PUBLIC	const char *linphone_core_get_version(void);
LINPHONE_PUBLIC	const char *linphone_core_get_user_agent_name(void);
LINPHONE_PUBLIC	const char *linphone_core_get_user_agent_version(void);

LINPHONE_PUBLIC LinphoneCore *linphone_core_new(const LinphoneCoreVTable *vtable,
						const char *config_path, const char *factory_config, void* userdata);

/**
 * Instantiates a LinphoneCore object with a given LpConfig.
 * @ingroup initializing
 *
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param vtable a LinphoneCoreVTable structure holding your application callbacks
 * @param config a pointer to an LpConfig object holding the configuration of the LinphoneCore to be instantiated.
 * @param userdata an opaque user pointer that can be retrieved at any time (for example in
 *        callbacks) using linphone_core_get_user_data().
 * @see linphone_core_new
**/
LINPHONE_PUBLIC LinphoneCore *linphone_core_new_with_config(const LinphoneCoreVTable *vtable, LpConfig *config, void *userdata);

/* function to be periodically called in a main loop */
/* For ICE to work properly it should be called every 20ms */
LINPHONE_PUBLIC	void linphone_core_iterate(LinphoneCore *lc);
#if 0 /*not implemented yet*/
/**
 * @ingroup initializing
 * Provide Linphone Core with an unique identifier. This be later used to identified contact address coming from this device.
 * Value is not saved.
 * @param lc object
 * @param string identifying the device, can be EMEI or UDID
 *
 */
void linphone_core_set_device_identifier(LinphoneCore *lc,const char* device_id);
/**
 * @ingroup initializing
 * get Linphone unique identifier
 *
 */
const char*  linphone_core_get_device_identifier(const LinphoneCore *lc);

#endif

/*sets the user-agent string in sip messages, ideally called just after linphone_core_new() or linphone_core_init() */
LINPHONE_PUBLIC	void linphone_core_set_user_agent(LinphoneCore *lc, const char *ua_name, const char *version);

LINPHONE_PUBLIC	LinphoneAddress * linphone_core_interpret_url(LinphoneCore *lc, const char *url);

LINPHONE_PUBLIC	LinphoneCall * linphone_core_invite(LinphoneCore *lc, const char *url);

LINPHONE_PUBLIC	LinphoneCall * linphone_core_invite_address(LinphoneCore *lc, const LinphoneAddress *addr);

LINPHONE_PUBLIC	LinphoneCall * linphone_core_invite_with_params(LinphoneCore *lc, const char *url, const LinphoneCallParams *params);

LINPHONE_PUBLIC	LinphoneCall * linphone_core_invite_address_with_params(LinphoneCore *lc, const LinphoneAddress *addr, const LinphoneCallParams *params);

LINPHONE_PUBLIC	int linphone_core_transfer_call(LinphoneCore *lc, LinphoneCall *call, const char *refer_to);

LINPHONE_PUBLIC	int linphone_core_transfer_call_to_another(LinphoneCore *lc, LinphoneCall *call, LinphoneCall *dest);

LINPHONE_PUBLIC LinphoneCall * linphone_core_start_refered_call(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

LINPHONE_PUBLIC	bool_t linphone_core_inc_invite_pending(LinphoneCore*lc);

LINPHONE_PUBLIC bool_t linphone_core_in_call(const LinphoneCore *lc);

LINPHONE_PUBLIC	LinphoneCall *linphone_core_get_current_call(const LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_accept_call(LinphoneCore *lc, LinphoneCall *call);

LINPHONE_PUBLIC	int linphone_core_accept_call_with_params(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

LINPHONE_PUBLIC int linphone_core_accept_early_media_with_params(LinphoneCore* lc, LinphoneCall* call, const LinphoneCallParams* params);

LINPHONE_PUBLIC int linphone_core_accept_early_media(LinphoneCore* lc, LinphoneCall* call);

LINPHONE_PUBLIC	int linphone_core_terminate_call(LinphoneCore *lc, LinphoneCall *call);

/**
 * Redirect the specified call to the given redirect URI.
 * @param[in] lc #LinphoneCore object.
 * @param[in] call The #LinphoneCall to redirect.
 * @param[in] redirect_uri The URI to redirect the call to.
 * @returns 0 if successful, -1 on error.
 * @ingroup call_control
 */
LINPHONE_PUBLIC int linphone_core_redirect_call(LinphoneCore *lc, LinphoneCall *call, const char *redirect_uri);

LINPHONE_PUBLIC	int linphone_core_decline_call(LinphoneCore *lc, LinphoneCall * call, LinphoneReason reason);

LINPHONE_PUBLIC	int linphone_core_terminate_all_calls(LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_pause_call(LinphoneCore *lc, LinphoneCall *call);

LINPHONE_PUBLIC	int linphone_core_pause_all_calls(LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_resume_call(LinphoneCore *lc, LinphoneCall *call);

LINPHONE_PUBLIC	int linphone_core_update_call(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);

LINPHONE_PUBLIC	int linphone_core_defer_call_update(LinphoneCore *lc, LinphoneCall *call);

LINPHONE_PUBLIC	int linphone_core_accept_call_update(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallParams *params);
/**
 * @ingroup media_parameters
 * Get default call parameters reflecting current linphone core configuration
 * @param LinphoneCore object
 * @return  LinphoneCallParams
 */
LINPHONE_PUBLIC	LinphoneCallParams *linphone_core_create_default_call_parameters(LinphoneCore *lc);

LINPHONE_PUBLIC	LinphoneCall *linphone_core_get_call_by_remote_address(LinphoneCore *lc, const char *remote_address);

LINPHONE_PUBLIC	void linphone_core_send_dtmf(LinphoneCore *lc,char dtmf);

LINPHONE_PUBLIC	int linphone_core_set_primary_contact(LinphoneCore *lc, const char *contact);

LINPHONE_PUBLIC	const char *linphone_core_get_primary_contact(LinphoneCore *lc);

LINPHONE_PUBLIC	const char * linphone_core_get_identity(LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_guess_hostname(LinphoneCore *lc, bool_t val);
LINPHONE_PUBLIC bool_t linphone_core_get_guess_hostname(LinphoneCore *lc);

LINPHONE_PUBLIC	bool_t linphone_core_ipv6_enabled(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_enable_ipv6(LinphoneCore *lc, bool_t val);

LINPHONE_PUBLIC	LinphoneAddress *linphone_core_get_primary_contact_parsed(LinphoneCore *lc);
LINPHONE_PUBLIC	const char * linphone_core_get_identity(LinphoneCore *lc);
/*0= no bandwidth limit*/
LINPHONE_PUBLIC	void linphone_core_set_download_bandwidth(LinphoneCore *lc, int bw);
LINPHONE_PUBLIC	void linphone_core_set_upload_bandwidth(LinphoneCore *lc, int bw);

LINPHONE_PUBLIC	int linphone_core_get_download_bandwidth(const LinphoneCore *lc);
LINPHONE_PUBLIC	int linphone_core_get_upload_bandwidth(const LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_enable_adaptive_rate_control(LinphoneCore *lc, bool_t enabled);
LINPHONE_PUBLIC bool_t linphone_core_adaptive_rate_control_enabled(const LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_download_ptime(LinphoneCore *lc, int ptime);
LINPHONE_PUBLIC	int  linphone_core_get_download_ptime(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_upload_ptime(LinphoneCore *lc, int ptime);

LINPHONE_PUBLIC	int linphone_core_get_upload_ptime(LinphoneCore *lc);

/**
 * Enable or disable DNS SRV resolution.
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable DNS SRV resolution, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_dns_srv(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether DNS SRV resolution is enabled.
 * @param[in] lc #LinphoneCore object.
 * @returns TRUE if DNS SRV resolution is enabled, FALSE if disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_dns_srv_enabled(const LinphoneCore *lc);

/* returns a MSList of PayloadType */
LINPHONE_PUBLIC	const MSList *linphone_core_get_audio_codecs(const LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_core_set_audio_codecs(LinphoneCore *lc, MSList *codecs);
/* returns a MSList of PayloadType */
LINPHONE_PUBLIC const MSList *linphone_core_get_video_codecs(const LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_core_set_video_codecs(LinphoneCore *lc, MSList *codecs);

/**
 * Tells whether the specified payload type is enabled.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pt The #PayloadType we want to know is enabled or not.
 * @returns TRUE if the payload type is enabled, FALSE if disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_payload_type_enabled(LinphoneCore *lc, const PayloadType *pt);

/**
 * Enable or disable the use of the specified payload type.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pt The #PayloadType to enable or disable. It can be retrieved using #linphone_core_find_payload_type
 * @param[in] enable TRUE to enable the payload type, FALSE to disable it.
 * @return 0 if successful, any other value otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC	int linphone_core_enable_payload_type(LinphoneCore *lc, PayloadType *pt, bool_t enable);

/**
 * Wildcard value used by #linphone_core_find_payload_type to ignore rate in search algorithm
 * @ingroup media_parameters
 */
#define LINPHONE_FIND_PAYLOAD_IGNORE_RATE -1
/**
 * Wildcard value used by #linphone_core_find_payload_type to ignore channel in search algorithm
 * @ingroup media_parameters
 */
#define LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS -1
/**
 * Get payload type from mime type and clock rate
 * @ingroup media_parameters
 * This function searches in audio and video codecs for the given payload type name and clockrate.
 * @param lc #LinphoneCore object
 * @param type payload mime type (I.E SPEEX, PCMU, VP8)
 * @param rate can be #LINPHONE_FIND_PAYLOAD_IGNORE_RATE
 * @param channels  number of channels, can be #LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS
 * @return Returns NULL if not found.
 */
LINPHONE_PUBLIC	PayloadType* linphone_core_find_payload_type(LinphoneCore* lc, const char* type, int rate, int channels) ;

LINPHONE_PUBLIC	int linphone_core_get_payload_type_number(LinphoneCore *lc, const PayloadType *pt);

LINPHONE_PUBLIC	const char *linphone_core_get_payload_type_description(LinphoneCore *lc, PayloadType *pt);

LINPHONE_PUBLIC	bool_t linphone_core_check_payload_type_usability(LinphoneCore *lc, PayloadType *pt);

/**
 * Create a proxy config with default values from Linphone core.
 * @param[in] lc #LinphoneCore object
 * @return #LinphoneProxyConfig with default values set
 * @ingroup proxy
 */
LINPHONE_PUBLIC	LinphoneProxyConfig * linphone_core_create_proxy_config(LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

LINPHONE_PUBLIC	void linphone_core_clear_proxy_config(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *config);

LINPHONE_PUBLIC	const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_default_proxy(LinphoneCore *lc, LinphoneProxyConfig *config);

void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index);

LINPHONE_PUBLIC	int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config);

/**
 * Create an authentication information with default values from Linphone core.
 * @param[in] lc #LinphoneCore object
 * @param[in] username String containing the username part of the authentication credentials
 * @param[in] userid String containing the username to use to calculate the authentication digest (optional)
 * @param[in] passwd String containing the password of the authentication credentials (optional, either passwd or ha1 must be set)
 * @param[in] ha1 String containing a ha1 hash of the password (optional, either passwd or ha1 must be set)
 * @param[in] realm String used to discriminate different SIP authentication domains (optional)
 * @return #LinphoneAuthInfo with default values set
 * @ingroup authentication
 */
LINPHONE_PUBLIC LinphoneAuthInfo * linphone_core_create_auth_info(LinphoneCore *lc, const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain);

LINPHONE_PUBLIC	void linphone_core_add_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info);

LINPHONE_PUBLIC void linphone_core_remove_auth_info(LinphoneCore *lc, const LinphoneAuthInfo *info);

LINPHONE_PUBLIC const MSList *linphone_core_get_auth_info_list(const LinphoneCore *lc);

LINPHONE_PUBLIC const LinphoneAuthInfo *linphone_core_find_auth_info(LinphoneCore *lc, const char *realm, const char *username, const char *sip_domain);

LINPHONE_PUBLIC void linphone_core_abort_authentication(LinphoneCore *lc,  LinphoneAuthInfo *info);

LINPHONE_PUBLIC	void linphone_core_clear_all_auth_info(LinphoneCore *lc);

/**
 * Enable or disable the audio adaptive jitter compensation.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable the audio adaptive jitter compensation, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_audio_adaptive_jittcomp(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the audio adaptive jitter compensation is enabled.
 * @param[in] lc #LinphoneCore object
 * @returns TRUE if the audio adaptive jitter compensation is enabled, FALSE otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_audio_adaptive_jittcomp_enabled(LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_core_get_audio_jittcomp(LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_audio_jittcomp(LinphoneCore *lc, int value);

/**
 * Enable or disable the video adaptive jitter compensation.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable the video adaptive jitter compensation, FALSE to disable it.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC void linphone_core_enable_video_adaptive_jittcomp(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the video adaptive jitter compensation is enabled.
 * @param[in] lc #LinphoneCore object
 * @returns TRUE if the video adaptive jitter compensation is enabled, FALSE otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC bool_t linphone_core_video_adaptive_jittcomp_enabled(LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_core_get_video_jittcomp(LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_video_jittcomp(LinphoneCore *lc, int value);

LINPHONE_PUBLIC	int linphone_core_get_audio_port(const LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_get_audio_port_range(const LinphoneCore *lc, int *min_port, int *max_port);

LINPHONE_PUBLIC	int linphone_core_get_video_port(const LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_get_video_port_range(const LinphoneCore *lc, int *min_port, int *max_port);

LINPHONE_PUBLIC	int linphone_core_get_nortp_timeout(const LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_audio_port(LinphoneCore *lc, int port);

LINPHONE_PUBLIC	void linphone_core_set_audio_port_range(LinphoneCore *lc, int min_port, int max_port);

LINPHONE_PUBLIC	void linphone_core_set_video_port(LinphoneCore *lc, int port);

LINPHONE_PUBLIC	void linphone_core_set_video_port_range(LinphoneCore *lc, int min_port, int max_port);

LINPHONE_PUBLIC	void linphone_core_set_nortp_timeout(LinphoneCore *lc, int port);

LINPHONE_PUBLIC	void linphone_core_set_use_info_for_dtmf(LinphoneCore *lc, bool_t use_info);

LINPHONE_PUBLIC	bool_t linphone_core_get_use_info_for_dtmf(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_use_rfc2833_for_dtmf(LinphoneCore *lc,bool_t use_rfc2833);

LINPHONE_PUBLIC	bool_t linphone_core_get_use_rfc2833_for_dtmf(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_sip_port(LinphoneCore *lc, int port);

LINPHONE_PUBLIC	int linphone_core_get_sip_port(LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_set_sip_transports(LinphoneCore *lc, const LCSipTransports *transports);

LINPHONE_PUBLIC	int linphone_core_get_sip_transports(LinphoneCore *lc, LCSipTransports *transports);

LINPHONE_PUBLIC	bool_t linphone_core_sip_transport_supported(const LinphoneCore *lc, LinphoneTransportType tp);
/**
 *
 * Give access to the UDP sip socket. Can be useful to configure this socket as persistent I.E kCFStreamNetworkServiceType set to kCFStreamNetworkServiceTypeVoIP)
 * @param lc #LinphoneCore
 * @return socket file descriptor
 */
ortp_socket_t linphone_core_get_sip_socket(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_inc_timeout(LinphoneCore *lc, int seconds);

LINPHONE_PUBLIC	int linphone_core_get_inc_timeout(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_in_call_timeout(LinphoneCore *lc, int seconds);

LINPHONE_PUBLIC	int linphone_core_get_in_call_timeout(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_delayed_timeout(LinphoneCore *lc, int seconds);

LINPHONE_PUBLIC	int linphone_core_get_delayed_timeout(LinphoneCore *lc);

/**
 * Set the STUN server address to use when the firewall policy is set to STUN.
 * @param[in] lc #LinphoneCore object
 * @param[in] server The STUN server address to use.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC	void linphone_core_set_stun_server(LinphoneCore *lc, const char *server);

/**
 * Get the STUN server address being used.
 * @param[in] lc #LinphoneCore object
 * @returns The STUN server address being used.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC	const char * linphone_core_get_stun_server(const LinphoneCore *lc);

/**
 * @ingroup network_parameters
 * Return the availability of uPnP.
 *
 * @return true if uPnP is available otherwise return false.
 */
LINPHONE_PUBLIC bool_t linphone_core_upnp_available();

/**
 * @ingroup network_parameters
 * Return the internal state of uPnP.
 *
 * @param lc #LinphoneCore
 * @return an LinphoneUpnpState.
 */
LINPHONE_PUBLIC LinphoneUpnpState linphone_core_get_upnp_state(const LinphoneCore *lc);

/**
 * @ingroup network_parameters
 * Return the external ip address of router.
 * In some cases the uPnP can have an external ip address but not a usable uPnP
 * (state different of Ok).
 *
 * @param lc #LinphoneCore
 * @return a null terminated string containing the external ip address. If the
 * the external ip address is not available return null.
 */
LINPHONE_PUBLIC const char * linphone_core_get_upnp_external_ipaddress(const LinphoneCore *lc);

/**
 * Set the public IP address of NAT when using the firewall policy is set to use NAT.
 * @param[in] lc #LinphoneCore object.
 * @param[in] addr The public IP address of NAT to use.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC void linphone_core_set_nat_address(LinphoneCore *lc, const char *addr);

/**
 * Get the public IP address of NAT being used.
 * @param[in] lc #LinphoneCore object.
 * @returns The public IP address of NAT being used.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_nat_address(const LinphoneCore *lc);

/**
 * Set the policy to use to pass through firewalls.
 * @param[in] lc #LinphoneCore object.
 * @param[in] pol The #LinphoneFirewallPolicy to use.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC	void linphone_core_set_firewall_policy(LinphoneCore *lc, LinphoneFirewallPolicy pol);

/**
 * Get the policy that is used to pass through firewalls.
 * @param[in] lc #LinphoneCore object.
 * @returns The #LinphoneFirewallPolicy that is being used.
 * @ingroup network_parameters
 */
LINPHONE_PUBLIC	LinphoneFirewallPolicy linphone_core_get_firewall_policy(const LinphoneCore *lc);

/* sound functions */
/* returns a null terminated static array of string describing the sound devices */
LINPHONE_PUBLIC const char**  linphone_core_get_sound_devices(LinphoneCore *lc);

/**
 * Update detection of sound devices.
 *
 * Use this function when the application is notified of USB plug events, so that
 * list of available hardwares for sound playback and capture is updated.
 * @param[in] lc #LinphoneCore object.
 * @ingroup media_parameters
 **/
LINPHONE_PUBLIC void linphone_core_reload_sound_devices(LinphoneCore *lc);

LINPHONE_PUBLIC bool_t linphone_core_sound_device_can_capture(LinphoneCore *lc, const char *device);
LINPHONE_PUBLIC bool_t linphone_core_sound_device_can_playback(LinphoneCore *lc, const char *device);
LINPHONE_PUBLIC	int linphone_core_get_ring_level(LinphoneCore *lc);
LINPHONE_PUBLIC	int linphone_core_get_play_level(LinphoneCore *lc);
LINPHONE_PUBLIC int linphone_core_get_rec_level(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_set_ring_level(LinphoneCore *lc, int level);
LINPHONE_PUBLIC	void linphone_core_set_play_level(LinphoneCore *lc, int level);

LINPHONE_PUBLIC	void linphone_core_set_mic_gain_db(LinphoneCore *lc, float level);
LINPHONE_PUBLIC	float linphone_core_get_mic_gain_db(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_set_playback_gain_db(LinphoneCore *lc, float level);
LINPHONE_PUBLIC	float linphone_core_get_playback_gain_db(LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_rec_level(LinphoneCore *lc, int level);
LINPHONE_PUBLIC const char * linphone_core_get_ringer_device(LinphoneCore *lc);
LINPHONE_PUBLIC const char * linphone_core_get_playback_device(LinphoneCore *lc);
LINPHONE_PUBLIC const char * linphone_core_get_capture_device(LinphoneCore *lc);
LINPHONE_PUBLIC int linphone_core_set_ringer_device(LinphoneCore *lc, const char * devid);
LINPHONE_PUBLIC int linphone_core_set_playback_device(LinphoneCore *lc, const char * devid);
LINPHONE_PUBLIC int linphone_core_set_capture_device(LinphoneCore *lc, const char * devid);
char linphone_core_get_sound_source(LinphoneCore *lc);
void linphone_core_set_sound_source(LinphoneCore *lc, char source);
LINPHONE_PUBLIC void linphone_core_stop_ringing(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_set_ring(LinphoneCore *lc, const char *path);
LINPHONE_PUBLIC const char *linphone_core_get_ring(const LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_verify_server_certificates(LinphoneCore *lc, bool_t yesno);
LINPHONE_PUBLIC void linphone_core_verify_server_cn(LinphoneCore *lc, bool_t yesno);
LINPHONE_PUBLIC void linphone_core_set_root_ca(LinphoneCore *lc, const char *path);
LINPHONE_PUBLIC const char *linphone_core_get_root_ca(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_set_ringback(LinphoneCore *lc, const char *path);
LINPHONE_PUBLIC const char * linphone_core_get_ringback(const LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_remote_ringback_tone(LinphoneCore *lc,const char *);
LINPHONE_PUBLIC const char *linphone_core_get_remote_ringback_tone(const LinphoneCore *lc);

LINPHONE_PUBLIC int linphone_core_preview_ring(LinphoneCore *lc, const char *ring,LinphoneCoreCbFunc func,void * userdata);
LINPHONE_PUBLIC	void linphone_core_enable_echo_cancellation(LinphoneCore *lc, bool_t val);
LINPHONE_PUBLIC	bool_t linphone_core_echo_cancellation_enabled(LinphoneCore *lc);

/**
 * Enables or disable echo limiter.
 * @param[in] lc #LinphoneCore object.
 * @param[in] val TRUE to enable echo limiter, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC	void linphone_core_enable_echo_limiter(LinphoneCore *lc, bool_t val);

/**
 * Tells whether echo limiter is enabled.
 * @param[in] lc #LinphoneCore object.
 * @returns TRUE if the echo limiter is enabled, FALSE otherwise.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC	bool_t linphone_core_echo_limiter_enabled(const LinphoneCore *lc);

void linphone_core_enable_agc(LinphoneCore *lc, bool_t val);
bool_t linphone_core_agc_enabled(const LinphoneCore *lc);

/**
 * @deprecated Use #linphone_core_enable_mic instead.
**/
LINPHONE_PUBLIC	void linphone_core_mute_mic(LinphoneCore *lc, bool_t muted);

/**
 * Get mic state.
 * @deprecated Use #linphone_core_is_mic_enabled instead
**/
LINPHONE_PUBLIC	bool_t linphone_core_is_mic_muted(LinphoneCore *lc);

/**
 * Enable or disable the microphone.
 * @param[in] lc #LinphoneCore object
 * @param[in] enable TRUE to enable the microphone, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_mic(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether the microphone is enabled.
 * @param[in] lc #LinphoneCore object
 * @returns TRUE if the microphone is enabled, FALSE if disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_mic_enabled(LinphoneCore *lc);

bool_t linphone_core_is_rtp_muted(LinphoneCore *lc);

bool_t linphone_core_get_rtp_no_xmit_on_audio_mute(const LinphoneCore *lc);
void linphone_core_set_rtp_no_xmit_on_audio_mute(LinphoneCore *lc, bool_t val);


/* returns a list of LinphoneCallLog */
LINPHONE_PUBLIC	const MSList * linphone_core_get_call_logs(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_clear_call_logs(LinphoneCore *lc);

/**
 * Get the number of missed calls.
 * Once checked, this counter can be reset with linphone_core_reset_missed_calls_count().
 * @param[in] lc #LinphoneCore object.
 * @returns The number of missed calls.
 * @ingroup call_logs
**/
LINPHONE_PUBLIC	int linphone_core_get_missed_calls_count(LinphoneCore *lc);

/**
 * Reset the counter of missed calls.
 * @param[in] lc #LinphoneCore object.
 * @ingroup call_logs
**/
LINPHONE_PUBLIC	void linphone_core_reset_missed_calls_count(LinphoneCore *lc);

/**
 * Remove a specific call log from call history list.
 * This function destroys the call log object. It must not be accessed anymore by the application after calling this function.
 * @param[in] lc #LinphoneCore object
 * @param[in] call_log #LinphoneCallLog object to remove.
 * @ingroup call_logs
**/
LINPHONE_PUBLIC	void linphone_core_remove_call_log(LinphoneCore *lc, LinphoneCallLog *call_log);

/* video support */
LINPHONE_PUBLIC bool_t linphone_core_video_supported(LinphoneCore *lc);

/**
 * Enables video globally.
 *
 * This function does not have any effect during calls. It just indicates LinphoneCore to
 * initiate future calls with video or not. The two boolean parameters indicate in which
 * direction video is enabled. Setting both to false disables video entirely.
 *
 * @param lc The LinphoneCore object
 * @param vcap_enabled indicates whether video capture is enabled
 * @param display_enabled indicates whether video display should be shown
 * @ingroup media_parameters
 * @deprecated Use #linphone_core_enable_video_capture and #linphone_core_enable_video_display instead.
**/
LINPHONE_PUBLIC	void linphone_core_enable_video(LinphoneCore *lc, bool_t vcap_enabled, bool_t display_enabled);

/**
 * Returns TRUE if video is enabled, FALSE otherwise.
 * @ingroup media_parameters
 * @deprecated Use #linphone_core_video_capture_enabled and #linphone_core_video_display_enabled instead.
**/
LINPHONE_PUBLIC bool_t linphone_core_video_enabled(LinphoneCore *lc);

/**
 * Enable or disable video capture.
 *
 * This function does not have any effect during calls. It just indicates the #LinphoneCore to
 * initiate future calls with video capture or not.
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable video capture, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_video_capture(LinphoneCore *lc, bool_t enable);

/**
 * Enable or disable video display.
 *
 * This function does not have any effect during calls. It just indicates the #LinphoneCore to
 * initiate future calls with video display or not.
 * @param[in] lc #LinphoneCore object.
 * @param[in] enable TRUE to enable video display, FALSE to disable it.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_enable_video_display(LinphoneCore *lc, bool_t enable);

/**
 * Tells whether video capture is enabled.
 * @param[in] lc #LinphoneCore object.
 * @returns TRUE if video capture is enabled, FALSE if disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_video_capture_enabled(LinphoneCore *lc);

/**
 * Tells whether video display is enabled.
 * @param[in] lc #LinphoneCore object.
 * @returns TRUE if video display is enabled, FALSE if disabled.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC bool_t linphone_core_video_display_enabled(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_set_video_policy(LinphoneCore *lc, const LinphoneVideoPolicy *policy);
LINPHONE_PUBLIC const LinphoneVideoPolicy *linphone_core_get_video_policy(LinphoneCore *lc);

typedef struct MSVideoSizeDef{
	MSVideoSize vsize;
	const char *name;
}MSVideoSizeDef;
/* returns a zero terminated table of MSVideoSizeDef*/
LINPHONE_PUBLIC const MSVideoSizeDef *linphone_core_get_supported_video_sizes(LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_set_preferred_video_size(LinphoneCore *lc, MSVideoSize vsize);
LINPHONE_PUBLIC MSVideoSize linphone_core_get_preferred_video_size(LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_set_preferred_video_size_by_name(LinphoneCore *lc, const char *name);

LINPHONE_PUBLIC void linphone_core_enable_video_preview(LinphoneCore *lc, bool_t val);
LINPHONE_PUBLIC bool_t linphone_core_video_preview_enabled(const LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_enable_self_view(LinphoneCore *lc, bool_t val);
LINPHONE_PUBLIC bool_t linphone_core_self_view_enabled(const LinphoneCore *lc);


/**
 * Update detection of camera devices.
 *
 * Use this function when the application is notified of USB plug events, so that
 * list of available hardwares for video capture is updated.
 * @param[in] lc #LinphoneCore object.
 * @ingroup media_parameters
 **/
LINPHONE_PUBLIC void linphone_core_reload_video_devices(LinphoneCore *lc);

/* returns a null terminated static array of string describing the webcams */
LINPHONE_PUBLIC const char**  linphone_core_get_video_devices(const LinphoneCore *lc);
LINPHONE_PUBLIC int linphone_core_set_video_device(LinphoneCore *lc, const char *id);
LINPHONE_PUBLIC const char *linphone_core_get_video_device(const LinphoneCore *lc);

/* Set and get static picture to be used when "Static picture" is the video device */
/**
 * Set the path to the image file to stream when "Static picture" is set as the video device.
 * @param[in] lc #LinphoneCore object.
 * @param[in] path The path to the image file to use.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC int linphone_core_set_static_picture(LinphoneCore *lc, const char *path);

/**
 * Get the path to the image file streamed when "Static picture" is set as the video device.
 * @param[in] lc #LinphoneCore object.
 * @returns The path to the image file streamed when "Static picture" is set as the video device.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC const char *linphone_core_get_static_picture(LinphoneCore *lc);

/**
 * Set the frame rate for static picture.
 * @param[in] lc #LinphoneCore object.
 * @param[in] fps The new frame rate to use for static picture.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC int linphone_core_set_static_picture_fps(LinphoneCore *lc, float fps);

/**
 * Get the frame rate for static picture
 * @param[in] lc #LinphoneCore object.
 * @return The frame rate used for static picture.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC float linphone_core_get_static_picture_fps(LinphoneCore *lc);

/*function to be used for eventually setting window decorations (icons, title...)*/
LINPHONE_PUBLIC unsigned long linphone_core_get_native_video_window_id(const LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_set_native_video_window_id(LinphoneCore *lc, unsigned long id);

LINPHONE_PUBLIC unsigned long linphone_core_get_native_preview_window_id(const LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_set_native_preview_window_id(LinphoneCore *lc, unsigned long id);

/**
 * Tells the core to use a separate window for local camera preview video, instead of
 * inserting local view within the remote video window.
 * @param[in] lc #LinphoneCore object.
 * @param[in] yesno TRUE to use a separate window, FALSE to insert the preview in the remote video window.
 * @ingroup media_parameters
**/
LINPHONE_PUBLIC void linphone_core_use_preview_window(LinphoneCore *lc, bool_t yesno);

int linphone_core_get_device_rotation(LinphoneCore *lc );
void linphone_core_set_device_rotation(LinphoneCore *lc, int rotation);

/**
 * Get the camera sensor rotation.
 *
 * This is needed on some mobile platforms to get the number of degrees the camera sensor
 * is rotated relative to the screen.
 *
 * @param lc The linphone core related to the operation
 * @return The camera sensor rotation in degrees (0 to 360) or -1 if it could not be retrieved
 */
LINPHONE_PUBLIC int linphone_core_get_camera_sensor_rotation(LinphoneCore *lc);

/* start or stop streaming video in case of embedded window */
void linphone_core_show_video(LinphoneCore *lc, bool_t show);

/*play/record support: use files instead of soundcard*/
void linphone_core_use_files(LinphoneCore *lc, bool_t yesno);
LINPHONE_PUBLIC void linphone_core_set_play_file(LinphoneCore *lc, const char *file);
LINPHONE_PUBLIC void linphone_core_set_record_file(LinphoneCore *lc, const char *file);

LINPHONE_PUBLIC void linphone_core_play_dtmf(LinphoneCore *lc, char dtmf, int duration_ms);
LINPHONE_PUBLIC void linphone_core_stop_dtmf(LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_get_current_call_duration(const LinphoneCore *lc);


LINPHONE_PUBLIC int linphone_core_get_mtu(const LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_set_mtu(LinphoneCore *lc, int mtu);

/**
 * @ingroup network_parameters
 * This method is called by the application to notify the linphone core library when network is reachable.
 * Calling this method with true trigger linphone to initiate a registration process for all proxies.
 * Calling this method disables the automatic network detection mode. It means you must call this method after each network state changes.
 */
LINPHONE_PUBLIC	void linphone_core_set_network_reachable(LinphoneCore* lc,bool_t value);
/**
 * @ingroup network_parameters
 * return network state either as positioned by the application or by linphone itself.
 */
LINPHONE_PUBLIC	bool_t linphone_core_is_network_reachable(LinphoneCore* lc);

/**
 *  @ingroup network_parameters
 *  enable signaling keep alive. small udp packet sent periodically to keep udp NAT association
 */
LINPHONE_PUBLIC	void linphone_core_enable_keep_alive(LinphoneCore* lc,bool_t enable);
/**
 *  @ingroup network_parameters
 * Is signaling keep alive
 */
LINPHONE_PUBLIC	bool_t linphone_core_keep_alive_enabled(LinphoneCore* lc);

LINPHONE_PUBLIC	void *linphone_core_get_user_data(LinphoneCore *lc);
LINPHONE_PUBLIC	void linphone_core_set_user_data(LinphoneCore *lc, void *userdata);

/* returns LpConfig object to read/write to the config file: usefull if you wish to extend
the config file with your own sections */
LINPHONE_PUBLIC LpConfig * linphone_core_get_config(LinphoneCore *lc);

/**
 * Create a LpConfig object from a user config file.
 * @param[in] lc #LinphoneCore object
 * @param[in] filename The filename of the config file to read to fill the instantiated LpConfig
 * @ingroup misc
 */
LINPHONE_PUBLIC LpConfig * linphone_core_create_lp_config(LinphoneCore *lc, const char *filename);

/*set a callback for some blocking operations, it takes you informed of the progress of the operation*/
void linphone_core_set_waiting_callback(LinphoneCore *lc, LinphoneCoreWaitingCallback cb, void *user_context);

/*returns the list of registered SipSetup (linphonecore plugins) */
const MSList * linphone_core_get_sip_setups(LinphoneCore *lc);

LINPHONE_PUBLIC	void linphone_core_destroy(LinphoneCore *lc);

/*for advanced users:*/
typedef RtpTransport * (*LinphoneCoreRtpTransportFactoryFunc)(void *data, int port);
struct _LinphoneRtpTransportFactories{
	LinphoneCoreRtpTransportFactoryFunc audio_rtp_func;
	void *audio_rtp_func_data;
	LinphoneCoreRtpTransportFactoryFunc audio_rtcp_func;
	void *audio_rtcp_func_data;
	LinphoneCoreRtpTransportFactoryFunc video_rtp_func;
	void *video_rtp_func_data;
	LinphoneCoreRtpTransportFactoryFunc video_rtcp_func;
	void *video_rtcp_func_data;
};
typedef struct _LinphoneRtpTransportFactories LinphoneRtpTransportFactories;

void linphone_core_set_rtp_transport_factories(LinphoneCore* lc, LinphoneRtpTransportFactories *factories);

int linphone_core_get_current_call_stats(LinphoneCore *lc, rtp_stats_t *local, rtp_stats_t *remote);

LINPHONE_PUBLIC	int linphone_core_get_calls_nb(const LinphoneCore *lc);

LINPHONE_PUBLIC	const MSList *linphone_core_get_calls(LinphoneCore *lc);

LinphoneGlobalState linphone_core_get_global_state(const LinphoneCore *lc);
/**
 * force registration refresh to be initiated upon next iterate
 * @ingroup proxies
 */
LINPHONE_PUBLIC void linphone_core_refresh_registers(LinphoneCore* lc);

/**
 * Set the path to the file storing the zrtp secrets cache.
 * @param[in] lc #LinphoneCore object
 * @param[in] file The path to the file to use to store the zrtp secrets cache.
 * @ingroup initializing
 */
LINPHONE_PUBLIC void linphone_core_set_zrtp_secrets_file(LinphoneCore *lc, const char* file);

/**
 * Get the path to the file storing the zrtp secrets cache.
 * @param[in] lc #LinphoneCore object.
 * @returns The path to the file storing the zrtp secrets cache.
 * @ingroup initializing
 */
LINPHONE_PUBLIC const char *linphone_core_get_zrtp_secrets_file(LinphoneCore *lc);

/**
 * Search from the list of current calls if a remote address match uri
 * @ingroup call_control
 * @param lc
 * @param uri which should match call remote uri
 * @return LinphoneCall or NULL is no match is found
 */
LINPHONE_PUBLIC LinphoneCall* linphone_core_find_call_from_uri(const LinphoneCore *lc, const char *uri);

LINPHONE_PUBLIC	int linphone_core_add_to_conference(LinphoneCore *lc, LinphoneCall *call);
LINPHONE_PUBLIC	int linphone_core_add_all_to_conference(LinphoneCore *lc);
LINPHONE_PUBLIC	int linphone_core_remove_from_conference(LinphoneCore *lc, LinphoneCall *call);
LINPHONE_PUBLIC	bool_t linphone_core_is_in_conference(const LinphoneCore *lc);
LINPHONE_PUBLIC	int linphone_core_enter_conference(LinphoneCore *lc);
LINPHONE_PUBLIC	int linphone_core_leave_conference(LinphoneCore *lc);
LINPHONE_PUBLIC	float linphone_core_get_conference_local_input_volume(LinphoneCore *lc);

LINPHONE_PUBLIC	int linphone_core_terminate_conference(LinphoneCore *lc);
LINPHONE_PUBLIC	int linphone_core_get_conference_size(LinphoneCore *lc);
int linphone_core_start_conference_recording(LinphoneCore *lc, const char *path);
int linphone_core_stop_conference_recording(LinphoneCore *lc);
/**
 * Get the maximum number of simultaneous calls Linphone core can manage at a time. All new call above this limit are declined with a busy answer
 * @ingroup initializing
 * @param lc core
 * @return max number of simultaneous calls
 */
LINPHONE_PUBLIC int linphone_core_get_max_calls(LinphoneCore *lc);
/**
 * Set the maximum number of simultaneous calls Linphone core can manage at a time. All new call above this limit are declined with a busy answer
 * @ingroup initializing
 * @param lc core
 * @param max number of simultaneous calls
 */
LINPHONE_PUBLIC void linphone_core_set_max_calls(LinphoneCore *lc, int max);

LINPHONE_PUBLIC	bool_t linphone_core_sound_resources_locked(LinphoneCore *lc);

LINPHONE_PUBLIC	bool_t linphone_core_media_encryption_supported(const LinphoneCore *lc, LinphoneMediaEncryption menc);

/**
 * Choose the media encryption policy to be used for RTP packets.
 * @param[in] lc #LinphoneCore object.
 * @param[in] menc The media encryption policy to be used.
 * @returns 0 if successful, any other value otherwise.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC	int linphone_core_set_media_encryption(LinphoneCore *lc, LinphoneMediaEncryption menc);

/**
 * Get the media encryption policy being used for RTP packets.
 * @param[in] lc #LinphoneCore object.
 * @returns The media encryption policy being used.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC	LinphoneMediaEncryption linphone_core_get_media_encryption(LinphoneCore *lc);

/**
 * Get behaviour when encryption parameters negociation fails on outgoing call.
 * @param[in] lc #LinphoneCore object.
 * @returns TRUE means the call will fail; FALSE means an INVITE will be resent with encryption disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC	bool_t linphone_core_is_media_encryption_mandatory(LinphoneCore *lc);

/**
 * Define behaviour when encryption parameters negociation fails on outgoing call.
 * @param[in] lc #LinphoneCore object.
 * @param[in] m If set to TRUE call will fail; if set to FALSE will resend an INVITE with encryption disabled.
 * @ingroup media_parameters
 */
LINPHONE_PUBLIC	void linphone_core_set_media_encryption_mandatory(LinphoneCore *lc, bool_t m);

/**
 * Init call params using LinphoneCore's current configuration
 */
LINPHONE_PUBLIC	void linphone_core_init_default_params(LinphoneCore*lc, LinphoneCallParams *params);

/**
 * True if tunnel support was compiled.
 */
LINPHONE_PUBLIC	bool_t linphone_core_tunnel_available(void);

typedef struct _LinphoneTunnel LinphoneTunnel;
/**
* get tunnel instance if available
*/
LINPHONE_PUBLIC	LinphoneTunnel *linphone_core_get_tunnel(LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_sip_dscp(LinphoneCore *lc, int dscp);
LINPHONE_PUBLIC int linphone_core_get_sip_dscp(const LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_audio_dscp(LinphoneCore *lc, int dscp);
LINPHONE_PUBLIC int linphone_core_get_audio_dscp(const LinphoneCore *lc);

LINPHONE_PUBLIC void linphone_core_set_video_dscp(LinphoneCore *lc, int dscp);
LINPHONE_PUBLIC int linphone_core_get_video_dscp(const LinphoneCore *lc);

LINPHONE_PUBLIC const char *linphone_core_get_video_display_filter(LinphoneCore *lc);
LINPHONE_PUBLIC void linphone_core_set_video_display_filter(LinphoneCore *lc, const char *filtername);

/** Contact Providers
  */

typedef unsigned int ContactSearchID;

typedef struct _LinphoneContactSearch LinphoneContactSearch;
typedef struct _LinphoneContactProvider LinphoneContactProvider;

typedef void (*ContactSearchCallback)( LinphoneContactSearch* id, MSList* friends, void* data );

/*
 * Remote provisioning
 */

/**
 * Set URI where to download xml configuration file at startup.
 * This can also be set from configuration file or factory config file, from [misc] section, item "config-uri".
 * Calling this function does not load the configuration. It will write the value into configuration so that configuration
 * from remote URI will take place at next LinphoneCore start.
 * @param lc the linphone core
 * @param uri the http or https uri to use in order to download the configuration.
 * @ingroup initializing
**/
LINPHONE_PUBLIC void linphone_core_set_provisioning_uri(LinphoneCore *lc, const char*uri);

/**
 * Get provisioning URI.
 * @param lc the linphone core
 * @return the provisioning URI.
 * @ingroup initializing
**/
LINPHONE_PUBLIC const char* linphone_core_get_provisioning_uri(const LinphoneCore *lc);


LINPHONE_PUBLIC int linphone_core_migrate_to_multi_transport(LinphoneCore *lc);
#ifdef __cplusplus
}
#endif

#endif
