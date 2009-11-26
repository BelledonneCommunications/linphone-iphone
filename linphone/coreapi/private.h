/***************************************************************************
 *            private.h
 *
 *  Mon Jun 13 14:23:23 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _PRIVATE_H
#define _PRIVATE_H

#include "linphonecore.h"
#include <eXosip2/eXosip.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef LIBLINPHONE_VERSION 
#define LIBLINPHONE_VERSION LINPHONE_VERSION
#endif

#ifndef PACKAGE_SOUND_DIR 
#define PACKAGE_SOUND_DIR "."
#endif

#ifdef WIN32
#include <io.h> /* for access() */
#endif

#ifdef HAVE_GETTEXT
#include <libintl.h>
#ifndef _
#define _(String) gettext(String)
#endif
#else
#ifndef _
#define _(something)	(something)
#endif
#ifndef ngettext
#define ngettext(singular, plural, number)	(((number)==1)?(singular):(plural))
#endif
#endif

	
typedef struct _StreamParams
{
	int initialized;
	int line;
	int localport;
	int remoteport;
	int remotertcpport;
	int pt;
	char *relay_session_id;
	int natd_port;
	char remoteaddr[LINPHONE_HOSTNAME_SIZE];
	char natd_addr[LINPHONE_HOSTNAME_SIZE];
} StreamParams;

typedef enum _LCState{
	LCStateInit,
	LCStateRinging,
	LCStateAVRunning
}LCState;


typedef struct _LinphoneCall
{
	struct _LinphoneCore *core;
	StreamParams audio_params;
	StreamParams video_params;
	LinphoneCallDir dir;
	struct _RtpProfile *profile;	/*points to the local_profile or to the remote "guessed" profile*/
	struct _LinphoneCallLog *log;
	int cid; /*call id */
	int did; /*dialog id */
	int tid; /*last transaction id*/
	char localip[LINPHONE_IPADDR_SIZE]; /* our best guess for local ipaddress for this call */
	struct _sdp_context *sdpctx;
	time_t start_time; /*time at which the call was initiated*/
	time_t media_start_time; /*time at which it was accepted, media streams established*/
	LCState	state;
	bool_t auth_pending;
	bool_t supports_session_timers;
} LinphoneCall;

LinphoneCall * linphone_call_new_outgoing(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to);
LinphoneCall * linphone_call_new_incoming(struct _LinphoneCore *lc, LinphoneAddress *from, LinphoneAddress *to, eXosip_event_t *ev);
#define linphone_call_set_state(lcall,st)	(lcall)->state=(st)
void linphone_call_destroy(struct _LinphoneCall *obj);

/* private: */
LinphoneCallLog * linphone_call_log_new(LinphoneCall *call, LinphoneAddress *local, LinphoneAddress * remote);
void linphone_call_log_completed(LinphoneCallLog *calllog, LinphoneCall *call);
void linphone_call_log_destroy(LinphoneCallLog *cl);


void linphone_core_init_media_streams(LinphoneCore *lc);

void linphone_auth_info_write_config(struct _LpConfig *config, LinphoneAuthInfo *obj, int pos);

void linphone_core_update_proxy_register(LinphoneCore *lc);
void linphone_core_refresh_subscribes(LinphoneCore *lc);

int linphone_proxy_config_send_publish(LinphoneProxyConfig *cfg, LinphoneOnlineStatus os);

int linphone_online_status_to_eXosip(LinphoneOnlineStatus os);

void linphone_friend_set_sid(LinphoneFriend *lf, int sid);
void linphone_friend_set_nid(LinphoneFriend *lf, int nid);
void linphone_friend_notify(LinphoneFriend *lf, int ss, LinphoneOnlineStatus os);

int set_lock_file();
int get_lock_file();
int remove_lock_file();
int do_registration(LinphoneCore *lc, bool_t doit);
void check_for_registration(LinphoneCore *lc);
char *int2str(int number);
int from_2char_without_params(osip_from_t *from,char **str);
void check_sound_device(LinphoneCore *lc);
void linphone_core_setup_local_rtp_profile(LinphoneCore *lc);
void linphone_core_get_local_ip(LinphoneCore *lc, const char *to, char *result);
bool_t host_has_ipv6_network();
bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret);

static inline int get_min_bandwidth(int dbw, int ubw){
	if (dbw<0) return ubw;
	if (ubw<0) return dbw;
	return MIN(dbw,ubw);
}

static inline bool_t bandwidth_is_greater(int bw1, int bw2){
	if (bw1<0) return TRUE;
	else if (bw2<0) return FALSE;
	else return bw1>=bw2;
}

static inline void set_string(char **dest, const char *src){
	if (*dest){
		ms_free(*dest);
		*dest=NULL;
	}
	if (src)
		*dest=ms_strdup(src);
}

#define PAYLOAD_TYPE_ENABLED	PAYLOAD_TYPE_USER_FLAG_0
bool_t linphone_proxy_config_register_again_with_updated_contact(LinphoneProxyConfig *obj, osip_message_t *orig_request, osip_message_t *last_answer);
void linphone_process_authentication(LinphoneCore* lc, eXosip_event_t *ev);
void linphone_authentication_ok(LinphoneCore *lc, eXosip_event_t *ev);
void linphone_subscription_new(LinphoneCore *lc, eXosip_event_t *ev);
void linphone_notify_recv(LinphoneCore *lc,eXosip_event_t *ev);
LinphoneProxyConfig *linphone_core_get_proxy_config_from_rid(LinphoneCore *lc, int rid);
void linphone_proxy_config_process_authentication_failure(LinphoneCore *lc, eXosip_event_t *ev);

void linphone_subscription_answered(LinphoneCore *lc, eXosip_event_t *ev);
void linphone_subscription_closed(LinphoneCore *lc, eXosip_event_t *ev);

void linphone_call_init_media_params(LinphoneCall *call);

void linphone_set_sdp(osip_message_t *sip, const char *sdp);

MSList *linphone_find_friend(MSList *fl, const LinphoneAddress *fri, LinphoneFriend **lf);
LinphoneFriend *linphone_find_friend_by_nid(MSList *l, int nid);
LinphoneFriend *linphone_find_friend_by_sid(MSList *l, int sid);

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc);
void linphone_core_update_allocated_audio_bandwidth_in_call(LinphoneCore *lc, const PayloadType *pt);
void linphone_core_run_stun_tests(LinphoneCore *lc, LinphoneCall *call);

void linphone_core_write_friends_config(LinphoneCore* lc);
void linphone_proxy_config_update(LinphoneProxyConfig *cfg);
void linphone_proxy_config_get_contact(LinphoneProxyConfig *cfg, const char **ip, int *port);
LinphoneProxyConfig * linphone_core_lookup_known_proxy(LinphoneCore *lc, const LinphoneAddress *uri);
int linphone_core_get_local_ip_for(const char *dest, char *result);

#endif /* _PRIVATE_H */
