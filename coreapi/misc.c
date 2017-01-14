
/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "private.h"
#include "linphone/lpconfig.h"
#include "mediastreamer2/mediastream.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SIGHANDLER_T
#include <signal.h>
#endif /*HAVE_SIGHANDLER_T*/

#include <string.h>
#if !defined(_WIN32_WCE)
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#endif /*_WIN32_WCE*/

#undef snprintf
#include <mediastreamer2/stun.h>

#ifdef HAVE_GETIFADDRS
#include <net/if.h>
#include <ifaddrs.h>
#endif
#include <math.h>
#if _MSC_VER
#define snprintf _snprintf
#define popen _popen
#define pclose _pclose
#endif


#define UDP_HDR_SZ 8
#define RTP_HDR_SZ 12
#define IP4_HDR_SZ 20   /*20 is the minimum, but there may be some options*/

static void clear_ice_check_list(LinphoneCall *call, IceCheckList *removed);

bool_t linphone_core_payload_type_enabled(LinphoneCore *lc, const LinphonePayloadType *pt){
	if (bctbx_list_find(lc->codecs_conf.audio_codecs, (PayloadType*) pt) || bctbx_list_find(lc->codecs_conf.video_codecs, (PayloadType*)pt) || bctbx_list_find(lc->codecs_conf.text_codecs, (PayloadType*)pt)){
		return payload_type_enabled(pt);
	}
	ms_error("Getting enablement status of codec not in audio or video list of PayloadType !");
	return FALSE;
}

bool_t linphone_core_payload_type_is_vbr(LinphoneCore *lc, const LinphonePayloadType *pt){
	if (pt->type==PAYLOAD_VIDEO) return TRUE;
	return !!(pt->flags & PAYLOAD_TYPE_IS_VBR);
}

int linphone_core_enable_payload_type(LinphoneCore *lc, LinphonePayloadType *pt, bool_t enabled){
	if (bctbx_list_find(lc->codecs_conf.audio_codecs,pt) || bctbx_list_find(lc->codecs_conf.video_codecs,pt) || bctbx_list_find(lc->codecs_conf.text_codecs,pt)){
		payload_type_set_enable(pt,enabled);
		_linphone_core_codec_config_write(lc);
		linphone_core_update_allocated_audio_bandwidth(lc);
		return 0;
	}
	ms_error("Enabling codec not in audio or video list of PayloadType !");
	return -1;
}

int linphone_core_get_payload_type_number(LinphoneCore *lc, const PayloadType *pt){
	return payload_type_get_number(pt);
}

void linphone_core_set_payload_type_number(LinphoneCore *lc, PayloadType *pt, int number){
	payload_type_set_number(pt,number);
}

const char *linphone_core_get_payload_type_description(LinphoneCore *lc, PayloadType *pt){
	//if (ms_filter_codec_supported(pt->mime_type)){
	if (ms_factory_codec_supported(lc->factory, pt->mime_type)){
		MSFilterDesc *desc=ms_factory_get_encoder(lc->factory, pt->mime_type);
#ifdef ENABLE_NLS
		return dgettext("mediastreamer",desc->text);
#else
		return desc->text;
#endif
	}
	return NULL;
}

void linphone_core_set_payload_type_bitrate(LinphoneCore *lc, LinphonePayloadType *pt, int bitrate){
	if (bctbx_list_find(lc->codecs_conf.audio_codecs, (PayloadType*) pt) || bctbx_list_find(lc->codecs_conf.video_codecs, (PayloadType*)pt) || bctbx_list_find(lc->codecs_conf.text_codecs, (PayloadType*)pt)){
		if (pt->type==PAYLOAD_VIDEO || pt->flags & PAYLOAD_TYPE_IS_VBR){
			pt->normal_bitrate=bitrate*1000;
			pt->flags|=PAYLOAD_TYPE_BITRATE_OVERRIDE;
			linphone_core_update_allocated_audio_bandwidth(lc);
		}else{
			ms_error("Cannot set an explicit bitrate for codec %s/%i, because it is not VBR.",pt->mime_type,pt->clock_rate);
			return;
		}
	} else {
		ms_error("linphone_core_set_payload_type_bitrate() payload type not in audio or video list !");
	}
}


/*
 *((codec-birate*ptime/8) + RTP header + UDP header + IP header)*8/ptime;
 *ptime=1/npacket
 */

static double get_audio_payload_bandwidth_from_codec_bitrate(const PayloadType *pt){
	double npacket=50;
	double packet_size;
	int bitrate;

	if (strcmp(payload_type_get_mime(&payload_type_aaceld_44k), payload_type_get_mime(pt))==0) {
		/*special case of aac 44K because ptime= 10ms*/
		npacket=100;
	}else if (strcmp(payload_type_get_mime(&payload_type_ilbc), payload_type_get_mime(pt))==0) {
		npacket=1000/30.0;
	}

	bitrate=pt->normal_bitrate;
	packet_size= (((double)bitrate)/(npacket*8))+UDP_HDR_SZ+RTP_HDR_SZ+IP4_HDR_SZ;
	return packet_size*8.0*npacket;
}

typedef struct vbr_codec_bitrate{
	int max_avail_bitrate;
	int min_rate;
	int recomended_bitrate;
}vbr_codec_bitrate_t;

static vbr_codec_bitrate_t defauls_vbr[]={
	//{ 100, 44100, 100 },
	{ 64, 44100, 50 },
	{ 64, 16000, 40 },
	{ 32, 16000, 32 },
	{ 32, 8000, 32 },
	{ 0 , 8000, 24 },
	{ 0 , 0, 0 }
};

static int lookup_vbr_typical_bitrate(int maxbw, int clock_rate){
	vbr_codec_bitrate_t *it;
	if (maxbw<=0) maxbw=defauls_vbr[0].max_avail_bitrate;
	for(it=defauls_vbr;it->min_rate!=0;it++){
		if (maxbw>=it->max_avail_bitrate && clock_rate>=it->min_rate)
			return it->recomended_bitrate;
	}
	ms_error("lookup_vbr_typical_bitrate(): should not happen.");
	return 32;
}

static int get_audio_payload_bandwidth(LinphoneCore *lc, const PayloadType *pt, int maxbw){
	if (linphone_core_payload_type_is_vbr(lc,pt)){
		if (pt->flags & PAYLOAD_TYPE_BITRATE_OVERRIDE){
			ms_debug("PayloadType %s/%i has bitrate override",pt->mime_type,pt->clock_rate);
			return pt->normal_bitrate/1000;
		}
		return lookup_vbr_typical_bitrate(maxbw,pt->clock_rate);
	}else return (int)ceil(get_audio_payload_bandwidth_from_codec_bitrate(pt)/1000.0);/*rounding codec bandwidth should be avoid, specially for AMR*/
}

int linphone_core_get_payload_type_bitrate(LinphoneCore *lc, const LinphonePayloadType *pt){
	int maxbw=get_min_bandwidth(linphone_core_get_download_bandwidth(lc),
					linphone_core_get_upload_bandwidth(lc));
	if (pt->type==PAYLOAD_AUDIO_CONTINUOUS || pt->type==PAYLOAD_AUDIO_PACKETIZED){
		return get_audio_payload_bandwidth(lc,pt,maxbw);
	}else if (pt->type==PAYLOAD_VIDEO){
		int video_bw;
		if (maxbw<=0) {
			video_bw=1500; /*default bitrate for video stream when no bandwidth limit is set, around 1.5 Mbit/s*/
		}else{
			video_bw=get_remaining_bandwidth_for_video(maxbw,lc->audio_bw);
		}
		return video_bw;
	}
	return 0;
}

void linphone_core_update_allocated_audio_bandwidth_in_call(LinphoneCall *call, const PayloadType *pt, int maxbw){
	call->audio_bw=get_audio_payload_bandwidth(call->core,pt,maxbw);
	ms_message("Audio bandwidth for this call is %i",call->audio_bw);
}

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc){
	const bctbx_list_t *elem;
	int maxbw=get_min_bandwidth(linphone_core_get_download_bandwidth(lc),
					linphone_core_get_upload_bandwidth(lc));
	int max_codec_bitrate=0;

	for(elem=linphone_core_get_audio_codecs(lc);elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		if (payload_type_enabled(pt)){
			int pt_bitrate=get_audio_payload_bandwidth(lc,pt,maxbw);
			if (max_codec_bitrate==0) {
				max_codec_bitrate=pt_bitrate;
			}else if (max_codec_bitrate<pt_bitrate){
				max_codec_bitrate=pt_bitrate;
			}
		}
	}
	if (max_codec_bitrate) {
		lc->audio_bw=max_codec_bitrate;
	}
}

bool_t linphone_core_is_payload_type_usable_for_bandwidth(LinphoneCore *lc, const PayloadType *pt, int bandwidth_limit){
	double codec_band;
	const int video_enablement_limit = 99;
	bool_t ret=FALSE;

	switch (pt->type){
		case PAYLOAD_AUDIO_CONTINUOUS:
		case PAYLOAD_AUDIO_PACKETIZED:
			codec_band=get_audio_payload_bandwidth(lc,pt,bandwidth_limit);
			ret=bandwidth_is_greater(bandwidth_limit,(int)codec_band);
			/*ms_message("Payload %s: codec_bandwidth=%g, bandwidth_limit=%i",pt->mime_type,codec_band,bandwidth_limit);*/
			break;
		case PAYLOAD_VIDEO:
			if (bandwidth_limit<=0 || bandwidth_limit >= video_enablement_limit) {/* infinite or greater than video_enablement_limit*/
				ret=TRUE;
			}
			else ret=FALSE;
			break;
		case PAYLOAD_TEXT:
			ret=TRUE;
			break;
	}
	return ret;
}

bool_t linphone_core_check_payload_type_usability(LinphoneCore *lc, const PayloadType *pt){
	int maxbw=get_min_bandwidth(linphone_core_get_download_bandwidth(lc),
					linphone_core_get_upload_bandwidth(lc));
	bool_t ret=linphone_core_is_payload_type_usable_for_bandwidth(lc, pt, maxbw);
	if ((pt->type==PAYLOAD_AUDIO_CONTINUOUS || pt->type==PAYLOAD_AUDIO_PACKETIZED)
		&& lc->sound_conf.capt_sndcard
		&& !(ms_snd_card_get_capabilities(lc->sound_conf.capt_sndcard) & MS_SND_CARD_CAP_BUILTIN_ECHO_CANCELLER)
		&& linphone_core_echo_cancellation_enabled(lc)
		&& (pt->clock_rate!=16000 && pt->clock_rate!=8000)
		&& strcasecmp(pt->mime_type,"opus")!=0
		&& ms_factory_lookup_filter_by_name(lc->factory, "MSWebRTCAEC")!=NULL){
		ms_warning("Payload type %s/%i cannot be used because software echo cancellation is required but is unable to operate at this rate.",
			   pt->mime_type,pt->clock_rate);
		ret=FALSE;
	}
	return ret;
}

bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret){
#if !defined(_WIN32_WCE) && !defined(LINPHONE_WINDOWS_UNIVERSAL)
	FILE *f=popen(command,"r");
	if (f!=NULL){
		int err;
		*result=ms_malloc(4096);
		err=fread(*result,1,4096-1,f);
		if (err<0){
			ms_warning("Error reading command output:%s",strerror(errno));
			ms_free(result);
			return FALSE;
		}
		(*result)[err]=0;
		err=pclose(f);
		if (command_ret!=NULL) *command_ret=err;
		return TRUE;
	}
#endif /*_WIN32_WCE*/
	return FALSE;
}

static ortp_socket_t create_socket(int local_port){
	struct sockaddr_in laddr;
	ortp_socket_t sock;
	int optval;
	sock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (sock<0) {
		ms_error("Fail to create socket");
		return -1;
	}
	memset (&laddr,0,sizeof(laddr));
	laddr.sin_family=AF_INET;
	laddr.sin_addr.s_addr=INADDR_ANY;
	laddr.sin_port=htons(local_port);
	if (bind(sock,(struct sockaddr*)&laddr,sizeof(laddr))<0){
		ms_error("Bind socket to 0.0.0.0:%i failed: %s",local_port,getSocketError());
		close_socket(sock);
		return -1;
	}
	optval=1;
	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
				(SOCKET_OPTION_VALUE)&optval, sizeof (optval))<0){
		ms_warning("Fail to set SO_REUSEADDR");
	}
	set_non_blocking_socket(sock);
	return sock;
}

static int send_stun_request(int sock, const struct sockaddr *server, socklen_t addrlen, int id, bool_t change_addr){
	char *buf = NULL;
	size_t len;
	int err = 0;
	MSStunMessage *req = ms_stun_binding_request_create();
	UInt96 tr_id = ms_stun_message_get_tr_id(req);
	tr_id.octet[0] = id;
	ms_stun_message_set_tr_id(req, tr_id);
	ms_stun_message_enable_change_ip(req, change_addr);
	ms_stun_message_enable_change_port(req, change_addr);
	len = ms_stun_message_encode(req, &buf);
	if (len <= 0) {
		ms_error("Fail to encode stun message.");
		err = -1;
	} else {
		err = bctbx_sendto(sock, buf, len, 0, server, addrlen);
		if (err < 0) {
			ms_error("sendto failed: %s",strerror(errno));
			err = -1;
		}
	}
	if (buf != NULL) ms_free(buf);
	ms_free(req);
	return err;
}

int linphone_parse_host_port(const char *input, char *host, size_t hostlen, int *port){
	char tmphost[NI_MAXHOST]={0};
	char *p1, *p2;

	if ((sscanf(input, "[%64[^]]]:%d", tmphost, port) == 2) || (sscanf(input, "[%64[^]]]", tmphost) == 1)) {

	} else {
		p1 = strchr(input, ':');
		p2 = strrchr(input, ':');
		if (p1 && p2 && (p1 != p2)) {/* an ipv6 address without port*/
			strncpy(tmphost, input, sizeof(tmphost) - 1);
		} else if (sscanf(input, "%[^:]:%d", tmphost, port) != 2) {
			/*no port*/
			strncpy(tmphost, input, sizeof(tmphost) - 1);
		}
	}
	strncpy(host,tmphost,hostlen);
	return 0;
}

int parse_hostname_to_addr(const char *server, struct sockaddr_storage *ss, socklen_t *socklen, int default_port){
	struct addrinfo hints,*res=NULL;
	char port[6];
	char host[NI_MAXHOST];
	int port_int=default_port;
	int ret;

	linphone_parse_host_port(server,host,sizeof(host),&port_int);

	snprintf(port, sizeof(port), "%d", port_int);
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_protocol=IPPROTO_UDP;
	ret=getaddrinfo(host,port,&hints,&res);
	if (ret!=0){
		ms_error("getaddrinfo() failed for %s:%s : %s",host,port,gai_strerror(ret));
		return -1;
	}
	if (!res) return -1;
	memcpy(ss,res->ai_addr,res->ai_addrlen);
	*socklen=(socklen_t)res->ai_addrlen;
	freeaddrinfo(res);
	return 0;
}

static int recv_stun_response(ortp_socket_t sock, char *ipaddr, int *port, int *id) {
	char buf[MS_STUN_MAX_MESSAGE_SIZE];
	int len = MS_STUN_MAX_MESSAGE_SIZE;
	MSStunMessage *resp;

	len = recv(sock, buf, len, 0);
	if (len > 0) {
		struct in_addr ia;
		resp = ms_stun_message_create_from_buffer_parsing((uint8_t *)buf, (ssize_t)len);
		if (resp != NULL) {
			const MSStunAddress *stun_addr;
			UInt96 tr_id = ms_stun_message_get_tr_id(resp);
			*id = tr_id.octet[0];
			stun_addr = ms_stun_message_get_xor_mapped_address(resp);
			if (stun_addr != NULL) {
				*port = stun_addr->ip.v4.port;
				ia.s_addr = htonl(stun_addr->ip.v4.addr);
			} else {
				stun_addr = ms_stun_message_get_mapped_address(resp);
				if (stun_addr != NULL) {
					*port = stun_addr->ip.v4.port;
					ia.s_addr = htonl(stun_addr->ip.v4.addr);
				} else len = -1;
			}
			if (len > 0) strncpy(ipaddr, inet_ntoa(ia), LINPHONE_IPADDR_SIZE);
		}
	}
	return len;
}

/* this functions runs a simple stun test and return the number of milliseconds to complete the tests, or -1 if the test were failed.*/
int linphone_core_run_stun_tests(LinphoneCore *lc, LinphoneCall *call){
	const char *server=linphone_core_get_stun_server(lc);
	StunCandidate *ac=&call->ac;
	StunCandidate *vc=&call->vc;
	StunCandidate *tc=&call->tc;

	if (lc->sip_conf.ipv6_enabled){
		ms_warning("stun support is not implemented for ipv6");
		return -1;
	}
	if (call->media_ports[call->main_audio_stream_index].rtp_port==-1){
		ms_warning("Stun-only support not available for system random port");
		return -1;
	}
	if (server!=NULL){
		const struct addrinfo *ai=linphone_core_get_stun_server_addrinfo(lc);
		ortp_socket_t sock1=-1, sock2=-1, sock3=-1;
		int loops=0;
		bool_t video_enabled=linphone_core_video_enabled(lc);
		bool_t got_audio,got_video,got_text;
		bool_t cone_audio=FALSE,cone_video=FALSE,cone_text=FALSE;
		struct timeval init,cur;
		double elapsed;
		int ret=0;

		if (ai==NULL){
			ms_error("Could not obtain stun server addrinfo.");
			return -1;
		}
		linphone_core_notify_display_status(lc,_("Stun lookup in progress..."));

		/*create the two audio and video RTP sockets, and send STUN message to our stun server */
		sock1=create_socket(call->media_ports[call->main_audio_stream_index].rtp_port);
		if (sock1==-1) return -1;
		if (video_enabled){
			sock2=create_socket(call->media_ports[call->main_video_stream_index].rtp_port);
			if (sock2==-1) return -1;
		}
		sock3=create_socket(call->media_ports[call->main_text_stream_index].rtp_port);
		if (sock3==-1) return -1;
		
		got_audio=FALSE;
		got_video=FALSE;
		got_text=FALSE;
		ortp_gettimeofday(&init,NULL);
		do{

			int id;
			if (loops%20==0){
				ms_message("Sending stun requests...");
				send_stun_request((int)sock1,ai->ai_addr,(socklen_t)ai->ai_addrlen,11,TRUE);
				send_stun_request((int)sock1,ai->ai_addr,(socklen_t)ai->ai_addrlen,1,FALSE);
				if (sock2!=-1){
					send_stun_request((int)sock2,ai->ai_addr,(socklen_t)ai->ai_addrlen,22,TRUE);
					send_stun_request((int)sock2,ai->ai_addr,(socklen_t)ai->ai_addrlen,2,FALSE);
				}
				if (sock3!=-1){
					send_stun_request((int)sock3,ai->ai_addr,(socklen_t)ai->ai_addrlen,33,TRUE);
					send_stun_request((int)sock3,ai->ai_addr,(socklen_t)ai->ai_addrlen,3,FALSE);
				}
			}
			ms_usleep(10000);

			if (recv_stun_response(sock1, ac->addr, &ac->port, &id) > 0) {
				ms_message("STUN test result: local audio port maps to %s:%i", ac->addr, ac->port);
				if (id==11) cone_audio=TRUE;
				got_audio=TRUE;
			}
			if (recv_stun_response(sock2, vc->addr, &vc->port, &id) > 0) {
				ms_message("STUN test result: local video port maps to %s:%i", vc->addr, vc->port);
				if (id==22) cone_video=TRUE;
				got_video=TRUE;
			}
			if (recv_stun_response(sock3, tc->addr, &tc->port, &id)>0) {
				ms_message("STUN test result: local text port maps to %s:%i", tc->addr, tc->port);
				if (id==33) cone_text=TRUE;
				got_text=TRUE;
			}
			ortp_gettimeofday(&cur,NULL);
			elapsed=((cur.tv_sec-init.tv_sec)*1000.0) +  ((cur.tv_usec-init.tv_usec)/1000.0);
			if (elapsed>2000)  {
				ms_message("Stun responses timeout, going ahead.");
				ret=-1;
				break;
			}
			loops++;
		}while(!(got_audio && (got_video||sock2==-1) && (got_text||sock3==-1)  ) );
		if (ret==0) ret=(int)elapsed;
		if (!got_audio){
			ms_error("No stun server response for audio port.");
		}else{
			if (!cone_audio) {
				ms_message("NAT is symmetric for audio port");
			}
		}
		if (sock2!=-1){
			if (!got_video){
				ms_error("No stun server response for video port.");
			}else{
				if (!cone_video) {
					ms_message("NAT is symmetric for video port.");
				}
			}
		}
		if (sock3!=-1){
			if (!got_text){
				ms_error("No stun server response for text port.");
			}else{
				if (!cone_text) {
					ms_message("NAT is symmetric for text port.");
				}
			}
		}
		close_socket(sock1);
		if (sock2!=-1) close_socket(sock2);
		if (sock3!=-1) close_socket(sock3);
		return ret;
	}
	return -1;
}

int linphone_core_get_edge_bw(LinphoneCore *lc){
	int edge_bw=lp_config_get_int(lc->config,"net","edge_bw",20);
	return edge_bw;
}

int linphone_core_get_edge_ptime(LinphoneCore *lc){
	int edge_ptime=lp_config_get_int(lc->config,"net","edge_ptime",100);
	return edge_ptime;
}

void linphone_core_adapt_to_network(LinphoneCore *lc, int ping_time_ms, LinphoneCallParams *params){
	int threshold;
	if (ping_time_ms>0 && lp_config_get_int(lc->config,"net","activate_edge_workarounds",0)==1){
		ms_message("Stun server ping time is %i ms",ping_time_ms);
		threshold=lp_config_get_int(lc->config,"net","edge_ping_time",500);

		if (ping_time_ms>threshold){
			/* we might be in a 2G network*/
			params->low_bandwidth=TRUE;
		}/*else use default settings */
	}
	if (params->low_bandwidth){
		params->up_bw=params->down_bw=linphone_core_get_edge_bw(lc);
		params->up_ptime=params->down_ptime=linphone_core_get_edge_ptime(lc);
		params->has_video=FALSE;
	}
}


void linphone_core_resolve_stun_server(LinphoneCore *lc){
	if (lc->nat_policy != NULL) {
		linphone_nat_policy_resolve_stun_server(lc->nat_policy);
	} else {
		ms_error("linphone_core_resolve_stun_server(): called without nat_policy, this should not happen.");
	}
}

const struct addrinfo *linphone_core_get_stun_server_addrinfo(LinphoneCore *lc){
	if (lc->nat_policy != NULL) {
		return linphone_nat_policy_get_stun_server_addrinfo(lc->nat_policy);
	} else {
		ms_error("linphone_core_get_stun_server_addrinfo(): called without nat_policy, this should not happen.");
	}
	return NULL;
}

void linphone_core_enable_forced_ice_relay(LinphoneCore *lc, bool_t enable) {
	lc->forced_ice_relay = enable;
}

void linphone_core_enable_short_turn_refresh(LinphoneCore *lc, bool_t enable) {
	lc->short_turn_refresh = enable;
}

static void stun_auth_requested_cb(LinphoneCall *call, const char *realm, const char *nonce, const char **username, const char **password, const char **ha1) {
	LinphoneProxyConfig *proxy = NULL;
	const LinphoneNatPolicy *nat_policy = NULL;
	const LinphoneAddress *addr = NULL;
	const LinphoneAuthInfo *auth_info = NULL;
	LinphoneCore *lc = call->core;
	const char *user = NULL;

	// Get the username from the nat policy or the proxy config
	if (call->dest_proxy != NULL) proxy = call->dest_proxy;
	else proxy = linphone_core_get_default_proxy_config(call->core);
	if (proxy == NULL) return;
	nat_policy = linphone_proxy_config_get_nat_policy(proxy);
	if (nat_policy != NULL) {
		user = linphone_nat_policy_get_stun_server_username(nat_policy);
	} else {
		nat_policy = linphone_core_get_nat_policy(call->core);
		if (nat_policy != NULL) {
			user = linphone_nat_policy_get_stun_server_username(nat_policy);
		}
	}
	if (user == NULL) {
		/* If the username has not been found in the nat_policy, take the username from the currently used proxy config. */
		addr = linphone_proxy_config_get_identity_address(proxy);
		if (addr == NULL) return;
		user = linphone_address_get_username(addr);
	}
	if (user == NULL) return;

	auth_info = linphone_core_find_auth_info(lc, realm, user, NULL);
	if (auth_info != NULL) {
		const char *hash = linphone_auth_info_get_ha1(auth_info);
		if (hash != NULL) {
			*ha1 = hash;
		} else {
			*password = linphone_auth_info_get_passwd(auth_info);
		}
		*username = user;
	} else {
		ms_warning("No auth info found for STUN auth request");
	}
}

static void linphone_core_add_local_ice_candidates(LinphoneCall *call, int family, const char *addr, IceCheckList *audio_cl, IceCheckList *video_cl, IceCheckList *text_cl) {
	if ((ice_check_list_state(audio_cl) != ICL_Completed) && (ice_check_list_candidates_gathered(audio_cl) == FALSE)) {
		ice_add_local_candidate(audio_cl, "host", family, addr, call->media_ports[call->main_audio_stream_index].rtp_port, 1, NULL);
		ice_add_local_candidate(audio_cl, "host", family, addr, call->media_ports[call->main_audio_stream_index].rtcp_port, 2, NULL);
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateInProgress;
	}
	if (linphone_core_video_enabled(call->core) && (video_cl != NULL)
		&& (ice_check_list_state(video_cl) != ICL_Completed) && (ice_check_list_candidates_gathered(video_cl) == FALSE)) {
		ice_add_local_candidate(video_cl, "host", family, addr, call->media_ports[call->main_video_stream_index].rtp_port, 1, NULL);
		ice_add_local_candidate(video_cl, "host", family, addr, call->media_ports[call->main_video_stream_index].rtcp_port, 2, NULL);
		call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateInProgress;
	}
	if (call->params->realtimetext_enabled && (text_cl != NULL)
		&& (ice_check_list_state(text_cl) != ICL_Completed) && (ice_check_list_candidates_gathered(text_cl) == FALSE)) {
		ice_add_local_candidate(text_cl, "host", family, addr, call->media_ports[call->main_text_stream_index].rtp_port, 1, NULL);
		ice_add_local_candidate(text_cl, "host", family, addr, call->media_ports[call->main_text_stream_index].rtcp_port, 2, NULL);
		call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateInProgress;
	}
}

static const struct addrinfo * get_preferred_stun_server_addrinfo(const struct addrinfo *ai) {
	char ip[NI_MAXHOST];
	const struct addrinfo *preferred_ai = NULL;

	while (ai != NULL) {
		bctbx_addrinfo_to_printable_ip_address(ai, ip, sizeof(ip));
		if (ai->ai_family == AF_INET) {
			preferred_ai = ai;
			break;
		}
		else if (ai->ai_family == AF_INET6) {
			struct sockaddr_storage ss;
			socklen_t sslen = sizeof(ss);
			bctbx_sockaddr_ipv6_to_ipv4(ai->ai_addr, (struct sockaddr *)&ss, &sslen);
			if ((ss.ss_family == AF_INET) && (preferred_ai == NULL)) preferred_ai = ai;
		}
		ai = ai->ai_next;
	}

	bctbx_addrinfo_to_printable_ip_address(preferred_ai, ip, sizeof(ip));
	return preferred_ai;
}

/* Return values:
 * 1 :  STUN gathering is started
 * 0 :  no STUN gathering is started, but it's ok to proceed with ICE anyway (with local candidates only or because STUN gathering was already done before)
 * -1: no gathering started and something went wrong with local candidates. There is no way to start the ICE session.
 */
int linphone_core_gather_ice_candidates(LinphoneCore *lc, LinphoneCall *call){
	char local_addr[64];
	const struct addrinfo *ai = NULL;
	IceCheckList *audio_cl;
	IceCheckList *video_cl;
	IceCheckList *text_cl;
	LinphoneNatPolicy *nat_policy = NULL;
	const char *server = NULL;

	if (call->dest_proxy != NULL) nat_policy = linphone_proxy_config_get_nat_policy(call->dest_proxy);
	if (nat_policy == NULL) nat_policy = linphone_core_get_nat_policy(lc);
	if (nat_policy != NULL) server = linphone_nat_policy_get_stun_server(nat_policy);

	if (call->ice_session == NULL) return -1;
	audio_cl = ice_session_check_list(call->ice_session, call->main_audio_stream_index);
	video_cl = ice_session_check_list(call->ice_session, call->main_video_stream_index);
	text_cl = ice_session_check_list(call->ice_session, call->main_text_stream_index);
	if ((audio_cl == NULL) && (video_cl == NULL) && (text_cl == NULL)) return -1;

	if ((nat_policy != NULL) && (server != NULL) && (server[0] != '\0')) {
		ai=linphone_nat_policy_get_stun_server_addrinfo(nat_policy);
		if (ai==NULL){
			ms_warning("Fail to resolve STUN server for ICE gathering, continuing without stun.");
		} else {
			ai = get_preferred_stun_server_addrinfo(ai);
		}
	}else{
		ms_warning("Ice is used without stun server.");
	}
	linphone_core_notify_display_status(lc, _("ICE local candidates gathering in progress..."));

	ice_session_enable_forced_relay(call->ice_session, lc->forced_ice_relay);
	ice_session_enable_short_turn_refresh(call->ice_session, lc->short_turn_refresh);

	/* Gather local host candidates. */
	if (call->af == AF_INET6) {
		if (linphone_core_get_local_ip_for(AF_INET6, NULL, local_addr) < 0) {
			ms_error("Fail to get local IPv6");
			return -1;
		} else {
			linphone_core_add_local_ice_candidates(call, AF_INET6, local_addr, audio_cl, video_cl, text_cl);
		}
	}
	if (linphone_core_get_local_ip_for(AF_INET, NULL, local_addr) < 0) {
		if (call->af != AF_INET6) {
			ms_error("Fail to get local IPv4");
			return -1;
		}
	} else {
		linphone_core_add_local_ice_candidates(call, AF_INET, local_addr, audio_cl, video_cl, text_cl);
	}
	if ((ai != NULL) && (nat_policy != NULL)
		&& (linphone_nat_policy_stun_enabled(nat_policy) || linphone_nat_policy_turn_enabled(nat_policy))) {
		bool_t gathering_in_progress;
		ms_message("ICE: gathering candidate from [%s] using %s", server, linphone_nat_policy_turn_enabled(nat_policy) ? "TURN" : "STUN");
		/* Gather local srflx candidates. */
		ice_session_enable_turn(call->ice_session, linphone_nat_policy_turn_enabled(nat_policy));
		ice_session_set_stun_auth_requested_cb(call->ice_session, (MSStunAuthRequestedCb)stun_auth_requested_cb, call);
		gathering_in_progress = ice_session_gather_candidates(call->ice_session, ai->ai_addr, (socklen_t)ai->ai_addrlen);
		return (gathering_in_progress == FALSE) ? 0 : 1;
	} else {
		ms_message("ICE: bypass candidates gathering");
		ice_session_compute_candidates_foundations(call->ice_session);
		ice_session_eliminate_redundant_candidates(call->ice_session);
		ice_session_choose_default_candidates(call->ice_session);
	}
	return 0;
}

const char *linphone_ice_state_to_string(LinphoneIceState state){
	switch(state){
		case LinphoneIceStateFailed:
			return "IceStateFailed";
		case LinphoneIceStateHostConnection:
			return "IceStateHostConnection";
		case LinphoneIceStateInProgress:
			return "IceStateInProgress";
		case LinphoneIceStateNotActivated:
			return "IceStateNotActivated";
		case LinphoneIceStateReflexiveConnection:
			return "IceStateReflexiveConnection";
		case LinphoneIceStateRelayConnection:
			return "IceStateRelayConnection";
	}
	return "invalid";
}

void linphone_core_update_ice_state_in_call_stats(LinphoneCall *call)
{
	IceCheckList *audio_check_list;
	IceCheckList *video_check_list;
	IceCheckList *text_check_list;
	IceSessionState session_state;

	if (call->ice_session == NULL) return;
	audio_check_list = ice_session_check_list(call->ice_session, call->main_audio_stream_index);
	video_check_list = ice_session_check_list(call->ice_session, call->main_video_stream_index);
	text_check_list = ice_session_check_list(call->ice_session, call->main_text_stream_index);
	if ((audio_check_list == NULL) && (video_check_list == NULL) && (text_check_list == NULL)) return;

	session_state = ice_session_state(call->ice_session);
	if ((session_state == IS_Completed) || ((session_state == IS_Failed) && (ice_session_has_completed_check_list(call->ice_session) == TRUE))) {
		if (call->params->has_audio && (audio_check_list != NULL)) {
			if (ice_check_list_state(audio_check_list) == ICL_Completed) {
				switch (ice_check_list_selected_valid_candidate_type(audio_check_list)) {
					case ICT_HostCandidate:
						call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateHostConnection;
						break;
					case ICT_ServerReflexiveCandidate:
					case ICT_PeerReflexiveCandidate:
						call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateReflexiveConnection;
						break;
					case ICT_RelayedCandidate:
						call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateRelayConnection;
						break;
					case ICT_CandidateInvalid:
					case ICT_CandidateTypeMax:
						/*shall not happen*/
						break;
				}
			} else {
				call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateFailed;
			}
		}else call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateNotActivated;
		
		if (call->params->has_video && (video_check_list != NULL)) {
			if (ice_check_list_state(video_check_list) == ICL_Completed) {
				switch (ice_check_list_selected_valid_candidate_type(video_check_list)) {
					case ICT_HostCandidate:
						call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateHostConnection;
						break;
					case ICT_ServerReflexiveCandidate:
					case ICT_PeerReflexiveCandidate:
						call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateReflexiveConnection;
						break;
					case ICT_RelayedCandidate:
						call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateRelayConnection;
						break;
					case ICT_CandidateInvalid:
					case ICT_CandidateTypeMax:
						/*shall not happen*/
						break;
				}
			} else {
				call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateFailed;
			}
		}else call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateNotActivated;
		
		if (call->params->realtimetext_enabled && (text_check_list != NULL)) {
			if (ice_check_list_state(text_check_list) == ICL_Completed) {
				switch (ice_check_list_selected_valid_candidate_type(text_check_list)) {
					case ICT_HostCandidate:
						call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateHostConnection;
						break;
					case ICT_ServerReflexiveCandidate:
					case ICT_PeerReflexiveCandidate:
						call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateReflexiveConnection;
						break;
					case ICT_RelayedCandidate:
						call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateRelayConnection;
						break;
					case ICT_CandidateInvalid:
					case ICT_CandidateTypeMax:
						/*shall not happen*/
						break;
				}
			} else {
				call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateFailed;
			}
		}else call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateNotActivated;
	} else if (session_state == IS_Running) {
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateInProgress;
		if (call->params->has_video && (video_check_list != NULL)) {
			call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateInProgress;
		}
		if (call->params->realtimetext_enabled && (text_check_list != NULL)) {
			call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateInProgress;
		}
	} else {
		call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state = LinphoneIceStateFailed;
		if (call->params->has_video && (video_check_list != NULL)) {
			call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state = LinphoneIceStateFailed;
		}
		if (call->params->realtimetext_enabled && (text_check_list != NULL)) {
			call->stats[LINPHONE_CALL_STATS_TEXT].ice_state = LinphoneIceStateFailed;
		}
	}
	ms_message("Call [%p] New ICE state: audio: [%s]    video: [%s]    text: [%s]", call,
		   linphone_ice_state_to_string(call->stats[LINPHONE_CALL_STATS_AUDIO].ice_state), linphone_ice_state_to_string(call->stats[LINPHONE_CALL_STATS_VIDEO].ice_state), linphone_ice_state_to_string(call->stats[LINPHONE_CALL_STATS_TEXT].ice_state));
}

void linphone_call_stop_ice_for_inactive_streams(LinphoneCall *call, SalMediaDescription *desc) {
	int i;
	IceSession *session = call->ice_session;

	if (session == NULL) return;
	if (ice_session_state(session) == IS_Completed) return;

	for (i = 0; i < desc->nb_streams; i++) {
		IceCheckList *cl = ice_session_check_list(session, i);
		if (!sal_stream_description_active(&desc->streams[i]) && cl) {
			ice_session_remove_check_list(session, cl);
			clear_ice_check_list(call, cl);
		}
	}

	linphone_core_update_ice_state_in_call_stats(call);
}

void _update_local_media_description_from_ice(SalMediaDescription *desc, IceSession *session, bool_t use_nortpproxy) {
	IceCandidate *rtp_candidate = NULL;
	IceCandidate *rtcp_candidate = NULL;
	IceSessionState session_state = ice_session_state(session);
	int nb_candidates;
	int i;
	int j;
	bool_t result = FALSE;

	if (session_state == IS_Completed) {
		IceCheckList *first_cl = NULL;
		for (i = 0; i < desc->nb_streams; i++) {
			IceCheckList *cl = ice_session_check_list(session, i);
			if (cl != NULL) {
				first_cl = cl;
				break;
			}
		}
		if (first_cl != NULL) {
			result = ice_check_list_selected_valid_local_candidate(first_cl, &rtp_candidate, NULL);
		}
		if (result == TRUE) {
			strncpy(desc->addr, rtp_candidate->taddr.ip, sizeof(desc->addr));
		} else {
			ms_warning("If ICE has completed successfully, rtp_candidate should be set!");
		}
	}
	
	strncpy(desc->ice_pwd, ice_session_local_pwd(session), sizeof(desc->ice_pwd));
	strncpy(desc->ice_ufrag, ice_session_local_ufrag(session), sizeof(desc->ice_ufrag));
	for (i = 0; i < desc->nb_streams; i++) {
		SalStreamDescription *stream = &desc->streams[i];
		IceCheckList *cl = ice_session_check_list(session, i);
		nb_candidates = 0;
		rtp_candidate = rtcp_candidate = NULL;
		if (!sal_stream_description_active(stream) || (cl == NULL)) continue;
		if (ice_check_list_state(cl) == ICL_Completed) {
			if (use_nortpproxy) stream->set_nortpproxy = TRUE;
			result = ice_check_list_selected_valid_local_candidate(ice_session_check_list(session, i), &rtp_candidate, &rtcp_candidate);
		} else {
			stream->set_nortpproxy = FALSE;
			result = ice_check_list_default_local_candidate(ice_session_check_list(session, i), &rtp_candidate, &rtcp_candidate);
		}
		if (result == TRUE) {
			strncpy(stream->rtp_addr, rtp_candidate->taddr.ip, sizeof(stream->rtp_addr));
			strncpy(stream->rtcp_addr, rtcp_candidate->taddr.ip, sizeof(stream->rtcp_addr));
			stream->rtp_port = rtp_candidate->taddr.port;
			stream->rtcp_port = rtcp_candidate->taddr.port;
		} else {
			memset(stream->rtp_addr, 0, sizeof(stream->rtp_addr));
			memset(stream->rtcp_addr, 0, sizeof(stream->rtcp_addr));
		}
		if ((strlen(ice_check_list_local_pwd(cl)) != strlen(desc->ice_pwd)) || (strcmp(ice_check_list_local_pwd(cl), desc->ice_pwd)))
			strncpy(stream->ice_pwd, ice_check_list_local_pwd(cl), sizeof(stream->ice_pwd));
		else
			memset(stream->ice_pwd, 0, sizeof(stream->ice_pwd));
		if ((strlen(ice_check_list_local_ufrag(cl)) != strlen(desc->ice_ufrag)) || (strcmp(ice_check_list_local_ufrag(cl), desc->ice_ufrag)))
			strncpy(stream->ice_ufrag, ice_check_list_local_ufrag(cl), sizeof(stream->ice_ufrag));
		else
			memset(stream->ice_pwd, 0, sizeof(stream->ice_pwd));
		stream->ice_mismatch = ice_check_list_is_mismatch(cl);
		if ((ice_check_list_state(cl) == ICL_Running) || (ice_check_list_state(cl) == ICL_Completed)) {
			memset(stream->ice_candidates, 0, sizeof(stream->ice_candidates));
			for (j = 0; j < MIN((int)bctbx_list_size(cl->local_candidates), SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES); j++) {
				SalIceCandidate *sal_candidate = &stream->ice_candidates[nb_candidates];
				IceCandidate *ice_candidate = bctbx_list_nth_data(cl->local_candidates, j);
				const char *default_addr = NULL;
				int default_port = 0;
				if (ice_candidate->componentID == 1) {
					default_addr = stream->rtp_addr;
					default_port = stream->rtp_port;
				} else if (ice_candidate->componentID == 2) {
					default_addr = stream->rtcp_addr;
					default_port = stream->rtcp_port;
				} else continue;
				if (default_addr[0] == '\0') default_addr = desc->addr;
				/* Only include the candidates matching the default destination for each component of the stream if the state is Completed as specified in RFC5245 section 9.1.2.2. */
				if ((ice_check_list_state(cl) == ICL_Completed)
					&& !((ice_candidate->taddr.port == default_port) && (strlen(ice_candidate->taddr.ip) == strlen(default_addr)) && (strcmp(ice_candidate->taddr.ip, default_addr) == 0)))
					continue;
				strncpy(sal_candidate->foundation, ice_candidate->foundation, sizeof(sal_candidate->foundation));
				sal_candidate->componentID = ice_candidate->componentID;
				sal_candidate->priority = ice_candidate->priority;
				strncpy(sal_candidate->type, ice_candidate_type(ice_candidate), sizeof(sal_candidate->type));
				strncpy(sal_candidate->addr, ice_candidate->taddr.ip, sizeof(sal_candidate->addr));
				sal_candidate->port = ice_candidate->taddr.port;
				if ((ice_candidate->base != NULL) && (ice_candidate->base != ice_candidate)) {
					strncpy(sal_candidate->raddr, ice_candidate->base->taddr.ip, sizeof(sal_candidate->raddr));
					sal_candidate->rport = ice_candidate->base->taddr.port;
				}
				nb_candidates++;
			}
		}
		if ((ice_check_list_state(cl) == ICL_Completed) && (ice_session_role(session) == IR_Controlling)) {
			memset(stream->ice_remote_candidates, 0, sizeof(stream->ice_remote_candidates));
			if (ice_check_list_selected_valid_remote_candidate(cl, &rtp_candidate, &rtcp_candidate) == TRUE) {
				strncpy(stream->ice_remote_candidates[0].addr, rtp_candidate->taddr.ip, sizeof(stream->ice_remote_candidates[0].addr));
				stream->ice_remote_candidates[0].port = rtp_candidate->taddr.port;
				strncpy(stream->ice_remote_candidates[1].addr, rtcp_candidate->taddr.ip, sizeof(stream->ice_remote_candidates[1].addr));
				stream->ice_remote_candidates[1].port = rtcp_candidate->taddr.port;
			} else {
				ms_error("ice: Selected valid remote candidates should be present if the check list is in the Completed state");
			}
		} else {
			for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES; j++) {
				stream->ice_remote_candidates[j].addr[0] = '\0';
				stream->ice_remote_candidates[j].port = 0;
			}
		}
	}
}

static void get_default_addr_and_port(uint16_t componentID, const SalMediaDescription *md, const SalStreamDescription *stream, const char **addr, int *port)
{
	if (componentID == 1) {
		*addr = stream->rtp_addr;
		*port = stream->rtp_port;
	} else if (componentID == 2) {
		*addr = stream->rtcp_addr;
		*port = stream->rtcp_port;
	} else return;
	if ((*addr)[0] == '\0') *addr = md->addr;
}

static void clear_ice_check_list(LinphoneCall *call, IceCheckList *removed){
	if (call->audiostream && call->audiostream->ms.ice_check_list==removed)
		call->audiostream->ms.ice_check_list=NULL;
	if (call->videostream && call->videostream->ms.ice_check_list==removed)
		call->videostream->ms.ice_check_list=NULL;
	if (call->textstream && call->textstream->ms.ice_check_list==removed)
		call->textstream->ms.ice_check_list=NULL;
}

void linphone_call_clear_unused_ice_candidates(LinphoneCall *call, const SalMediaDescription *md){
	int i;
	
	if (!call->localdesc) return;
	for (i = 0; i < md->nb_streams; i++) {
		const SalStreamDescription *local_stream = &call->localdesc->streams[i];
		const SalStreamDescription *stream = &md->streams[i];
		IceCheckList *cl = ice_session_check_list(call->ice_session, i);
		if (!cl || !local_stream) continue;
		
		if (stream->rtcp_mux && local_stream->rtcp_mux){
			ice_check_list_remove_rtcp_candidates(cl);
		}
	}
}

bool_t linphone_core_media_description_contains_video_stream(const SalMediaDescription *md){
	int i;

	for (i = 0; md && i < md->nb_streams; i++) {
		if (md->streams[i].type == SalVideo && md->streams[i].rtp_port!=0)
			return TRUE;
	}
	return FALSE;
}

unsigned int linphone_core_get_audio_features(LinphoneCore *lc){
	unsigned int ret=0;
	const char *features=lp_config_get_string(lc->config,"sound","features",NULL);
	if (features){
		char tmp[256]={0};
		char name[256];
		char *p,*n;
		strncpy(tmp,features,sizeof(tmp)-1);
		for(p=tmp;*p!='\0';p++){
			if (*p==' ') continue;
			n=strchr(p,'|');
			if (n) *n='\0';
			sscanf(p,"%s",name);
			ms_message("Found audio feature %s",name);
			if (strcasecmp(name,"PLC")==0) ret|=AUDIO_STREAM_FEATURE_PLC;
			else if (strcasecmp(name,"EC")==0) ret|=AUDIO_STREAM_FEATURE_EC;
			else if (strcasecmp(name,"EQUALIZER")==0) ret|=AUDIO_STREAM_FEATURE_EQUALIZER;
			else if (strcasecmp(name,"VOL_SND")==0) ret|=AUDIO_STREAM_FEATURE_VOL_SND;
			else if (strcasecmp(name,"VOL_RCV")==0) ret|=AUDIO_STREAM_FEATURE_VOL_RCV;
			else if (strcasecmp(name,"DTMF")==0) ret|=AUDIO_STREAM_FEATURE_DTMF;
			else if (strcasecmp(name,"DTMF_ECHO")==0) ret|=AUDIO_STREAM_FEATURE_DTMF_ECHO;
			else if (strcasecmp(name,"MIXED_RECORDING")==0) ret|=AUDIO_STREAM_FEATURE_MIXED_RECORDING;
			else if (strcasecmp(name,"LOCAL_PLAYING")==0) ret|=AUDIO_STREAM_FEATURE_LOCAL_PLAYING;
			else if (strcasecmp(name,"REMOTE_PLAYING")==0) ret|=AUDIO_STREAM_FEATURE_REMOTE_PLAYING;
			else if (strcasecmp(name,"ALL")==0) ret|=AUDIO_STREAM_FEATURE_ALL;
			else if (strcasecmp(name,"NONE")==0) ret=0;
			else ms_error("Unsupported audio feature %s requested in config file.",name);
			if (!n) break;
			p=n;
		}
	}else ret=AUDIO_STREAM_FEATURE_ALL;

	if (ret==AUDIO_STREAM_FEATURE_ALL){
		/*since call recording is specified before creation of the stream in linphonecore,
		* it will be requested on demand. It is not necessary to include it all the time*/
		ret&=~AUDIO_STREAM_FEATURE_MIXED_RECORDING;
	}
	return ret;
}

bool_t linphone_core_tone_indications_enabled(LinphoneCore*lc){
	return lp_config_get_int(lc->config,"sound","tone_indications",1);
}

int linphone_core_get_local_ip_for(int type, const char *dest, char *result){
	return bctbx_get_local_ip_for(type, dest, 5060, result, LINPHONE_IPADDR_SIZE);
}

void linphone_core_get_local_ip(LinphoneCore *lc, int af, const char *dest, char *result) {
	if (af == AF_UNSPEC) {
		if (linphone_core_ipv6_enabled(lc)) {
			bool_t has_ipv6 = linphone_core_get_local_ip_for(AF_INET6, dest, result) == 0;
			if (strcmp(result, "::1") != 0)
				return; /*this machine has real ipv6 connectivity*/
			if ((linphone_core_get_local_ip_for(AF_INET, dest, result) == 0) && (strcmp(result, "127.0.0.1") != 0))
				return; /*this machine has only ipv4 connectivity*/
			if (has_ipv6) {
				/*this machine has only local loopback for both ipv4 and ipv6, so prefer ipv6*/
				strncpy(result, "::1", LINPHONE_IPADDR_SIZE);
				return;
			}
		}
		/*in all other cases use IPv4*/
		af = AF_INET;
	}
	linphone_core_get_local_ip_for(af, dest, result);
}

SalReason linphone_reason_to_sal(LinphoneReason reason){
	switch(reason){
		case LinphoneReasonNone:
			return SalReasonNone;
		case LinphoneReasonNoResponse:
			return SalReasonRequestTimeout;
		case LinphoneReasonForbidden:
			return SalReasonForbidden;
		case LinphoneReasonDeclined:
			return SalReasonDeclined;
		case LinphoneReasonNotFound:
			return SalReasonNotFound;
		case LinphoneReasonTemporarilyUnavailable:
			return SalReasonTemporarilyUnavailable;
		case LinphoneReasonBusy:
			return SalReasonBusy;
		case LinphoneReasonNotAcceptable:
			return SalReasonNotAcceptable;
		case LinphoneReasonIOError:
			return SalReasonServiceUnavailable;
		case LinphoneReasonDoNotDisturb:
			return SalReasonDoNotDisturb;
		case LinphoneReasonUnauthorized:
			return SalReasonUnauthorized;
		case LinphoneReasonUnsupportedContent:
			return SalReasonUnsupportedContent;
		case LinphoneReasonNoMatch:
			return SalReasonNoMatch;
		case LinphoneReasonMovedPermanently:
			return SalReasonMovedPermanently;
		case LinphoneReasonGone:
			return SalReasonGone;
		case LinphoneReasonAddressIncomplete:
			return SalReasonAddressIncomplete;
		case LinphoneReasonNotImplemented:
			return SalReasonNotImplemented;
		case LinphoneReasonBadGateway:
			return SalReasonBadGateway;
		case LinphoneReasonServerTimeout:
			return SalReasonServerTimeout;
		case LinphoneReasonNotAnswered:
			return SalReasonRequestTimeout;
		case LinphoneReasonUnknown:
			return SalReasonUnknown;
	}
	return SalReasonUnknown;
}

LinphoneReason linphone_reason_from_sal(SalReason r){
	LinphoneReason ret=LinphoneReasonNone;
	switch(r){
		case SalReasonNone:
			ret=LinphoneReasonNone;
			break;
		case SalReasonIOError:
			ret=LinphoneReasonIOError;
			break;
		case SalReasonUnknown:
		case SalReasonInternalError:
			ret=LinphoneReasonUnknown;
			break;
		case SalReasonBusy:
			ret=LinphoneReasonBusy;
			break;
		case SalReasonDeclined:
			ret=LinphoneReasonDeclined;
			break;
		case SalReasonDoNotDisturb:
			ret=LinphoneReasonDoNotDisturb;
			break;
		case SalReasonForbidden:
			ret=LinphoneReasonBadCredentials;
			break;
		case SalReasonNotAcceptable:
			ret=LinphoneReasonNotAcceptable;
			break;
		case SalReasonNotFound:
			ret=LinphoneReasonNotFound;
			break;
		case SalReasonRedirect:
			ret=LinphoneReasonNone;
			break;
		case SalReasonTemporarilyUnavailable:
			ret=LinphoneReasonTemporarilyUnavailable;
			break;
		case SalReasonServiceUnavailable:
			ret=LinphoneReasonIOError;
			break;
		case SalReasonRequestPending:
			ret=LinphoneReasonTemporarilyUnavailable; /*might not be exactly the perfect matching, but better than LinphoneReasonNone*/
			break;
		case SalReasonUnauthorized:
			ret=LinphoneReasonUnauthorized;
			break;
		case SalReasonUnsupportedContent:
			ret=LinphoneReasonUnsupportedContent;
		break;
		case SalReasonNoMatch:
			ret=LinphoneReasonNoMatch;
		break;
		case SalReasonRequestTimeout:
			ret=LinphoneReasonNotAnswered;
		break;
		case SalReasonMovedPermanently:
			ret=LinphoneReasonMovedPermanently;
		break;
		case SalReasonGone:
			ret=LinphoneReasonGone;
		break;
		case SalReasonAddressIncomplete:
			ret=LinphoneReasonAddressIncomplete;
		break;
		case SalReasonNotImplemented:
			ret=LinphoneReasonNotImplemented;
		break;
		case SalReasonBadGateway:
			ret=LinphoneReasonBadGateway;
		break;
		case SalReasonServerTimeout:
			ret=LinphoneReasonServerTimeout;
		break;
	}
	return ret;
}

/**
 * Set the name of the mediastreamer2 filter to be used for rendering video.
 * This is for advanced users of the library, mainly to workaround hardware/driver bugs.
 * @ingroup media_parameters
**/
void linphone_core_set_video_display_filter(LinphoneCore *lc, const char *filter_name){
	lp_config_set_string(lc->config,"video","displaytype",filter_name);
}

const char *linphone_core_get_video_display_filter(LinphoneCore *lc){
	return lp_config_get_string(lc->config,"video","displaytype",NULL);
}

/**
 * Queue a task into the main loop. The data pointer must remain valid until the task is completed.
 * task_fun must return BELLE_SIP_STOP when job is finished.
**/
void linphone_core_queue_task(LinphoneCore *lc, belle_sip_source_func_t task_fun, void *data, const char *task_description){
	belle_sip_source_t *s=sal_create_timer(lc->sal,task_fun,data, 20, task_description);
	belle_sip_object_unref(s);
}

static int get_unique_transport(LinphoneCore *lc, LinphoneTransportType *type, int *port){
	LCSipTransports tp;
	linphone_core_get_sip_transports(lc,&tp);
	if (tp.tcp_port==0 && tp.tls_port==0 && tp.udp_port!=0){
		*type=LinphoneTransportUdp;
		*port=tp.udp_port;
		return 0;
	}else if (tp.tcp_port==0 && tp.udp_port==0 && tp.tls_port!=0){
		*type=LinphoneTransportTls;
		*port=tp.tls_port;
		return 0;
	}else if (tp.tcp_port!=0 && tp.udp_port==0 && tp.tls_port==0){
		*type=LinphoneTransportTcp;
		*port=tp.tcp_port;
		return 0;
	}
	return -1;
}

static void linphone_core_migrate_proxy_config(LinphoneCore *lc, LinphoneTransportType type){
	const bctbx_list_t *elem;
	for(elem=linphone_core_get_proxy_config_list(lc);elem!=NULL;elem=elem->next){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		const char *proxy=linphone_proxy_config_get_addr(cfg);
		const char *route=linphone_proxy_config_get_route(cfg);
		LinphoneAddress *proxy_addr=linphone_address_new(proxy);
		LinphoneAddress *route_addr=NULL;
		char *tmp;
		if (route) route_addr=linphone_address_new(route);
		if (proxy_addr){
			linphone_address_set_transport(proxy_addr,type);
			tmp=linphone_address_as_string(proxy_addr);
			linphone_proxy_config_set_server_addr(cfg,tmp);
			ms_free(tmp);
			linphone_address_unref(proxy_addr);
		}
		if (route_addr){
			linphone_address_set_transport(route_addr,type);
			tmp=linphone_address_as_string(route_addr);
			linphone_proxy_config_set_route(cfg,tmp);
			ms_free(tmp);
			linphone_address_unref(route_addr);
		}
	}
}

int linphone_core_migrate_to_multi_transport(LinphoneCore *lc){
	if (!lp_config_get_int(lc->config,"sip","multi_transport_migration_done",0)){
		LinphoneTransportType tpt;
		int port;
		if (get_unique_transport(lc,&tpt,&port)==0){
			LCSipTransports newtp={0};
			if (lp_config_get_int(lc->config,"sip","sip_random_port",0))
				port=-1;
			ms_message("Core is using a single SIP transport, migrating proxy config and enabling multi-transport.");
			linphone_core_migrate_proxy_config(lc,tpt);
			newtp.udp_port=port;
			newtp.tcp_port=port;
			newtp.tls_port=LC_SIP_TRANSPORT_RANDOM;
			lp_config_set_string(lc->config, "sip","sip_random_port",NULL); /*remove*/
			linphone_core_set_sip_transports(lc,&newtp);
		}
		lp_config_set_int(lc->config,"sip","multi_transport_migration_done",1);
		return 1;
	}
	return 0;
}

LinphoneToneDescription * linphone_tone_description_new(LinphoneReason reason, LinphoneToneID id, const char *audiofile){
	LinphoneToneDescription *obj=ms_new0(LinphoneToneDescription,1);
	obj->reason=reason;
	obj->toneid=id;
	obj->audiofile=audiofile ? ms_strdup(audiofile) : NULL;
	return obj;
}

void linphone_tone_description_destroy(LinphoneToneDescription *obj){
	if (obj->audiofile) ms_free(obj->audiofile);
	ms_free(obj);
}

LinphoneToneDescription *linphone_core_get_call_error_tone(const LinphoneCore *lc, LinphoneReason reason){
	const bctbx_list_t *elem;
	for (elem=lc->tones;elem!=NULL;elem=elem->next){
		LinphoneToneDescription *tone=(LinphoneToneDescription*)elem->data;
		if (tone->reason==reason) return tone;
	}
	return NULL;
}

const char *linphone_core_get_tone_file(const LinphoneCore *lc, LinphoneToneID id){
	const bctbx_list_t *elem;
	for (elem=lc->tones;elem!=NULL;elem=elem->next){
		LinphoneToneDescription *tone=(LinphoneToneDescription*)elem->data;
		if (tone->toneid==id && tone->reason==LinphoneReasonNone && tone->audiofile!=NULL) return tone->audiofile;
	}
	return NULL;
}

void _linphone_core_set_tone(LinphoneCore *lc, LinphoneReason reason, LinphoneToneID id, const char *audiofile){
	LinphoneToneDescription *tone=linphone_core_get_call_error_tone(lc,reason);
	if (tone){
		lc->tones=bctbx_list_remove(lc->tones,tone);
		linphone_tone_description_destroy(tone);
	}
	tone=linphone_tone_description_new(reason,id,audiofile);
	lc->tones=bctbx_list_append(lc->tones,tone);
}

void linphone_core_set_call_error_tone(LinphoneCore *lc, LinphoneReason reason, const char *audiofile){
	_linphone_core_set_tone(lc,reason,LinphoneToneUndefined, audiofile);
}

void linphone_core_set_tone(LinphoneCore *lc, LinphoneToneID id, const char *audiofile){
	_linphone_core_set_tone(lc, LinphoneReasonNone, id, audiofile);
}

const MSCryptoSuite * linphone_core_get_srtp_crypto_suites(LinphoneCore *lc){
	const char *config= lp_config_get_string(lc->config, "sip", "srtp_crypto_suites", "AES_CM_128_HMAC_SHA1_80, AES_CM_128_HMAC_SHA1_32, AES_256_CM_HMAC_SHA1_80, AES_256_CM_HMAC_SHA1_32");
	char *tmp=ms_strdup(config);

	char *sep;
	char *pos;
	char *nextpos;
	char *params;
	int found=0;
	MSCryptoSuite *result=NULL;
	pos=tmp;
	do{
		sep=strchr(pos,',');
		if (!sep) {
			sep=pos+strlen(pos);
			nextpos=NULL;
		}else {
			*sep='\0';
			nextpos=sep+1;
		}
		while(*pos==' ') ++pos; /*strip leading spaces*/
		params=strchr(pos,' '); /*look for params that arrive after crypto suite name*/
		if (params){
			while(*params==' ') ++params; /*strip parameters leading space*/
		}
		if (sep-pos>0){
			MSCryptoSuiteNameParams np;
			MSCryptoSuite suite;
			np.name=pos;
			np.params=params;
			suite=ms_crypto_suite_build_from_name_params(&np);
			if (suite!=MS_CRYPTO_SUITE_INVALID){
				result=ms_realloc(result,(found+2)*sizeof(MSCryptoSuite));
				result[found]=suite;
				result[found+1]=MS_CRYPTO_SUITE_INVALID;
				found++;
				ms_message("Configured srtp crypto suite: %s %s",np.name,np.params ? np.params : "");
			}
		}
		pos=nextpos;
	}while(pos);
	ms_free(tmp);
	if (lc->rtp_conf.srtp_suites){
		ms_free(lc->rtp_conf.srtp_suites);
		lc->rtp_conf.srtp_suites=NULL;
	}
	lc->rtp_conf.srtp_suites=result;
	return result;
}

static char * seperate_string_list(char **str) {
	char *ret;

	if (str == NULL) return NULL;
	if (*str == NULL) return NULL;
	if (**str == '\0') return NULL;

	ret = *str;
	for ( ; **str!='\0' && **str!=' ' && **str!=','; (*str)++);
	if (**str == '\0') {
		return ret;
	} else {
		**str = '\0';
		do { (*str)++; } while (**str!='\0' && (**str==' ' || **str==','));
		return ret;
	}
}

MsZrtpCryptoTypesCount linphone_core_get_zrtp_key_agreement_suites(LinphoneCore *lc, MSZrtpKeyAgreement keyAgreements[MS_MAX_ZRTP_CRYPTO_TYPES]){
	char * zrtpConfig = (char*)lp_config_get_string(lc->config, "sip", "zrtp_key_agreements_suites", NULL);
	MsZrtpCryptoTypesCount key_agreements_count = 0;
	char * entry, * origPtr;
	if (zrtpConfig == NULL) {
	        return 0;
	}

	origPtr = ms_strdup(zrtpConfig);
	zrtpConfig = origPtr;
	while ((entry = seperate_string_list(&zrtpConfig))) {
		const MSZrtpKeyAgreement agreement = ms_zrtp_key_agreement_from_string(entry);
		if (agreement != MS_ZRTP_KEY_AGREEMENT_INVALID) {
			ms_message("Configured zrtp key agreement: '%s'", ms_zrtp_key_agreement_to_string(agreement));
			keyAgreements[key_agreements_count++] = agreement;
		}
	}

	ms_free(origPtr);
	return key_agreements_count;
}

MsZrtpCryptoTypesCount linphone_core_get_zrtp_cipher_suites(LinphoneCore *lc, MSZrtpCipher ciphers[MS_MAX_ZRTP_CRYPTO_TYPES]){
	char * zrtpConfig = (char*)lp_config_get_string(lc->config, "sip", "zrtp_cipher_suites", NULL);
	MsZrtpCryptoTypesCount cipher_count = 0;
	char * entry, * origPtr;
	if (zrtpConfig == NULL) {
	        return 0;
	}

	origPtr = ms_strdup(zrtpConfig);
	zrtpConfig = origPtr;
	while ((entry = seperate_string_list(&zrtpConfig))) {
		const MSZrtpCipher cipher = ms_zrtp_cipher_from_string(entry);
		if (cipher != MS_ZRTP_CIPHER_INVALID) {
			ms_message("Configured zrtp cipher: '%s'", ms_zrtp_cipher_to_string(cipher));
			ciphers[cipher_count++] = cipher;
		}
	}

	ms_free(origPtr);
	return cipher_count;
}

MsZrtpCryptoTypesCount linphone_core_get_zrtp_hash_suites(LinphoneCore *lc, MSZrtpHash hashes[MS_MAX_ZRTP_CRYPTO_TYPES]){
	char * zrtpConfig = (char*)lp_config_get_string(lc->config, "sip", "zrtp_hash_suites", NULL);
	MsZrtpCryptoTypesCount hash_count = 0;
	char * entry, * origPtr;
	if (zrtpConfig == NULL) {
        	return 0;
	}

	origPtr = ms_strdup(zrtpConfig);
	zrtpConfig = origPtr;
	while ((entry = seperate_string_list(&zrtpConfig))) {
		const MSZrtpHash hash = ms_zrtp_hash_from_string(entry);
		if (hash != MS_ZRTP_HASH_INVALID) {
			ms_message("Configured zrtp hash: '%s'", ms_zrtp_hash_to_string(hash));
			hashes[hash_count++] = hash;
		}
	}

	ms_free(origPtr);
	return hash_count;
}

MsZrtpCryptoTypesCount linphone_core_get_zrtp_auth_suites(LinphoneCore *lc, MSZrtpAuthTag authTags[MS_MAX_ZRTP_CRYPTO_TYPES]){
	char * zrtpConfig = (char*)lp_config_get_string(lc->config, "sip", "zrtp_auth_suites", NULL);
	MsZrtpCryptoTypesCount auth_tag_count = 0;
	char * entry, * origPtr;
	if (zrtpConfig == NULL) {
		return 0;
	}

	origPtr = ms_strdup(zrtpConfig);
	zrtpConfig = origPtr;
	while ((entry = seperate_string_list(&zrtpConfig))) {
		const MSZrtpAuthTag authTag = ms_zrtp_auth_tag_from_string(entry);
		if (authTag != MS_ZRTP_AUTHTAG_INVALID) {
			ms_message("Configured zrtp auth tag: '%s'", ms_zrtp_auth_tag_to_string(authTag));
			authTags[auth_tag_count++] = authTag;
		}
	}

	ms_free(origPtr);
	return auth_tag_count;
}

MsZrtpCryptoTypesCount linphone_core_get_zrtp_sas_suites(LinphoneCore *lc, MSZrtpSasType sasTypes[MS_MAX_ZRTP_CRYPTO_TYPES]){
	char * zrtpConfig = (char*)lp_config_get_string(lc->config, "sip", "zrtp_sas_suites", NULL);
	MsZrtpCryptoTypesCount sas_count = 0;
	char * entry, * origPtr;
	if (zrtpConfig == NULL) {
	        return 0;
	}

	origPtr = ms_strdup(zrtpConfig);
	zrtpConfig = origPtr;
	while ((entry = seperate_string_list(&zrtpConfig))) {
		const MSZrtpSasType type = ms_zrtp_sas_type_from_string(entry);
		if (type != MS_ZRTP_SAS_INVALID) {
			ms_message("Configured zrtp SAS type: '%s'", ms_zrtp_sas_type_to_string(type));
			sasTypes[sas_count++] = type;
		}
	}

	ms_free(origPtr);
	return sas_count;
}

const char ** linphone_core_get_supported_file_formats(LinphoneCore *core){
	static const char *mkv="mkv";
	static const char *wav="wav";
	if (core->supported_formats==NULL){
		core->supported_formats=ms_malloc0(3*sizeof(char*));
		core->supported_formats[0]=wav;
        if (ms_factory_lookup_filter_by_id(core->factory,MS_MKV_RECORDER_ID)){
			core->supported_formats[1]=mkv;
		}
	}
	return core->supported_formats;
}

bool_t linphone_core_file_format_supported(LinphoneCore *lc, const char *fmt){
	const char **formats=linphone_core_get_supported_file_formats(lc);
	for(;*formats!=NULL;++formats){
		if (strcasecmp(*formats,fmt)==0) return TRUE;
	}
	return FALSE;
}

bool_t linphone_core_symmetric_rtp_enabled(LinphoneCore*lc){
	return lp_config_get_int(lc->config,"rtp","symmetric",1);
}

int linphone_core_set_network_simulator_params(LinphoneCore *lc, const OrtpNetworkSimulatorParams *params){
	if (params!=&lc->net_conf.netsim_params)
		lc->net_conf.netsim_params=*params;
	/*TODO: should we make some sanity checks on the parameters here*/
	return 0;
}

const OrtpNetworkSimulatorParams *linphone_core_get_network_simulator_params(const LinphoneCore *lc){
	return &lc->net_conf.netsim_params;
}

static const char *_tunnel_mode_str[3] = { "disable", "enable", "auto" };

LinphoneTunnelMode linphone_tunnel_mode_from_string(const char *string) {
	if(string != NULL) {
		int i;
		for(i=0; i<3 && strcmp(string, _tunnel_mode_str[i]) != 0; i++);
		if(i<3) {
			return (LinphoneTunnelMode)i;
		} else {
			ms_error("Invalid tunnel mode '%s'", string);
			return LinphoneTunnelModeDisable;
		}
	} else {
		return LinphoneTunnelModeDisable;
	}
}

const char *linphone_tunnel_mode_to_string(LinphoneTunnelMode mode) {
	switch(mode){
		case LinphoneTunnelModeAuto:
			return "auto";
		case LinphoneTunnelModeDisable:
			return "disable";
		case LinphoneTunnelModeEnable:
			return "enable";
	}
	return "invalid";
}


typedef struct Hook{
	LinphoneCoreIterateHook fun;
	void *data;
}Hook;

void linphone_task_list_init(LinphoneTaskList *t){
	t->hooks = NULL;
}

static Hook *hook_new(LinphoneCoreIterateHook hook, void *hook_data){
	Hook *h=ms_new0(Hook,1);
	h->fun=hook;
	h->data=hook_data;
	return h;
}

static void hook_invoke(Hook *h){
	h->fun(h->data);
}

void linphone_task_list_add(LinphoneTaskList *t, LinphoneCoreIterateHook hook, void *hook_data){
	t->hooks = bctbx_list_append(t->hooks,hook_new(hook,hook_data));
}

void linphone_task_list_remove(LinphoneTaskList *t, LinphoneCoreIterateHook hook, void *hook_data){
	bctbx_list_t *elem;
	for(elem=t->hooks;elem!=NULL;elem=elem->next){
		Hook *h=(Hook*)elem->data;
		if (h->fun==hook && h->data==hook_data){
			t->hooks = bctbx_list_erase_link(t->hooks,elem);
			ms_free(h);
			return;
		}
	}
	ms_error("linphone_task_list_remove(): No such hook found.");
}

void linphone_task_list_run(LinphoneTaskList *t){
	bctbx_list_for_each(t->hooks,(void (*)(void*))hook_invoke);
}

void linphone_task_list_free(LinphoneTaskList *t){
	t->hooks = bctbx_list_free_with_data(t->hooks, (void (*)(void*))ms_free);
}

static bool_t _ice_params_found_in_remote_media_description(IceSession *ice_session, const SalMediaDescription *md) {
	const SalStreamDescription *stream;
	IceCheckList *cl = NULL;
	int i;
	bool_t ice_params_found = FALSE;
	if ((md->ice_pwd[0] != '\0') && (md->ice_ufrag[0] != '\0')) {
		ice_params_found=TRUE;
	} else {
		for (i = 0; i < md->nb_streams; i++) {
			stream = &md->streams[i];
			cl = ice_session_check_list(ice_session, i);
			if (cl) {
				if ((stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0')) {
					ice_params_found=TRUE;
				} else {
					ice_params_found=FALSE;
					break;
				}
			}
		}
	}
	return ice_params_found;
}

static bool_t _check_for_ice_restart_and_set_remote_credentials(IceSession *ice_session, const SalMediaDescription *md, bool_t is_offer) {
	const SalStreamDescription *stream;
	IceCheckList *cl = NULL;
	bool_t ice_restarted = FALSE;
	int i;

	if ((strcmp(md->addr, "0.0.0.0") == 0) || (strcmp(md->addr, "::0") == 0)) {
		ice_session_restart(ice_session, is_offer ? IR_Controlled : IR_Controlling);
		ice_restarted = TRUE;
	} else {
		for (i = 0; i < md->nb_streams; i++) {
			stream = &md->streams[i];
			cl = ice_session_check_list(ice_session, i);
			if (cl && (strcmp(stream->rtp_addr, "0.0.0.0") == 0)) {
				ice_session_restart(ice_session, is_offer ? IR_Controlled : IR_Controlling);
				ice_restarted = TRUE;
				break;
			}
		}
	}
	if ((ice_session_remote_ufrag(ice_session) == NULL) && (ice_session_remote_pwd(ice_session) == NULL)) {
		ice_session_set_remote_credentials(ice_session, md->ice_ufrag, md->ice_pwd);
	} else if (ice_session_remote_credentials_changed(ice_session, md->ice_ufrag, md->ice_pwd)) {
		if (ice_restarted == FALSE) {
			ice_session_restart(ice_session, is_offer ? IR_Controlled : IR_Controlling);
			ice_restarted = TRUE;
		}
		ice_session_set_remote_credentials(ice_session, md->ice_ufrag, md->ice_pwd);
	}
	for (i = 0; i < md->nb_streams; i++) {
		stream = &md->streams[i];
		cl = ice_session_check_list(ice_session, i);
		if (cl && (stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0')) {
			if (ice_check_list_remote_credentials_changed(cl, stream->ice_ufrag, stream->ice_pwd)) {
				if (ice_restarted == FALSE
						&& ice_check_list_get_remote_ufrag(cl)
						&& ice_check_list_get_remote_pwd(cl)) {
						/* restart only if remote ufrag/paswd was already set*/
					ice_session_restart(ice_session, is_offer ? IR_Controlled : IR_Controlling);
					ice_restarted = TRUE;
				}
				ice_check_list_set_remote_credentials(cl, stream->ice_ufrag, stream->ice_pwd);
				break;
			}
		}
	}
	return ice_restarted;
}

static void _create_ice_check_lists_and_parse_ice_attributes(LinphoneCall *call, const SalMediaDescription *md, bool_t ice_restarted) {
	const SalStreamDescription *stream;
	IceCheckList *cl = NULL;
	bool_t default_candidate = FALSE;
	const char *addr = NULL;
	int port = 0;
	int componentID = 0;
	int remote_family;
	int family;
	int i, j;

	for (i = 0; i < md->nb_streams; i++) {
		stream = &md->streams[i];
		cl = ice_session_check_list(call->ice_session, i);

		if (cl==NULL) continue;
		if (stream->ice_mismatch == TRUE) {
			ice_check_list_set_state(cl, ICL_Failed);
			continue;
		}
		if (stream->rtp_port == 0) {
			ice_session_remove_check_list(call->ice_session, cl);
			clear_ice_check_list(call,cl);
			continue;
		}

		if ((stream->ice_pwd[0] != '\0') && (stream->ice_ufrag[0] != '\0'))
			ice_check_list_set_remote_credentials(cl, stream->ice_ufrag, stream->ice_pwd);
		for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES; j++) {
			const SalIceCandidate *candidate = &stream->ice_candidates[j];
			default_candidate = FALSE;
			addr = NULL;
			port = 0;
			if (candidate->addr[0] == '\0') break;
			if ((candidate->componentID == 0) || (candidate->componentID > 2)) continue;
			get_default_addr_and_port(candidate->componentID, md, stream, &addr, &port);
			if (addr && (candidate->port == port) && (strlen(candidate->addr) == strlen(addr)) && (strcmp(candidate->addr, addr) == 0))
				default_candidate = TRUE;
			if (strchr(candidate->addr, ':') != NULL) family = AF_INET6;
			else family = AF_INET;
			ice_add_remote_candidate(cl, candidate->type, family, candidate->addr, candidate->port, candidate->componentID,
				candidate->priority, candidate->foundation, default_candidate);
		}
		if (ice_restarted == FALSE) {
			bool_t losing_pairs_added = FALSE;
			for (j = 0; j < SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES; j++) {
				const SalIceRemoteCandidate *remote_candidate = &stream->ice_remote_candidates[j];
				addr = NULL;
				port = 0;
				componentID = j + 1;
				if (remote_candidate->addr[0] == '\0') break;
				get_default_addr_and_port(componentID, md, stream, &addr, &port);
				if (j == 0) {
					/* If we receive a re-invite and we finished ICE processing on our side, use the candidates given by the remote. */
					ice_check_list_unselect_valid_pairs(cl);
				}
				if (strchr(remote_candidate->addr, ':') != NULL) remote_family = AF_INET6;
				else remote_family = AF_INET;
				if (strchr(addr, ':') != NULL) family = AF_INET6;
				else family = AF_INET;
				
				ice_add_losing_pair(cl, j + 1, remote_family, remote_candidate->addr, remote_candidate->port, family, addr, port);
				losing_pairs_added = TRUE;
			}
			if (losing_pairs_added == TRUE) ice_check_list_check_completed(cl);
		}
	}
}

static void _update_ice_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md, bool_t is_offer) {
	const SalStreamDescription *stream;
	IceCheckList *cl = NULL;
	bool_t ice_restarted = FALSE;
	int i;

	/* Check for ICE restart and set remote credentials. */
	ice_restarted = _check_for_ice_restart_and_set_remote_credentials(call->ice_session, md, is_offer);

	/* Create ICE check lists if needed and parse ICE attributes. */
	_create_ice_check_lists_and_parse_ice_attributes(call, md, ice_restarted);
	for (i = 0; i < md->nb_streams; i++) {
		stream = &md->streams[i];
		cl = ice_session_check_list(call->ice_session, i);
		if (!cl) continue;
			
		if (!sal_stream_description_active(stream)) {
			ice_session_remove_check_list_from_idx(call->ice_session, i);
			clear_ice_check_list(call, cl);
		}
	}
	linphone_call_clear_unused_ice_candidates(call, md);
	ice_session_check_mismatch(call->ice_session);
}

void linphone_call_update_ice_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md, bool_t is_offer){
	if (_ice_params_found_in_remote_media_description(call->ice_session, md) == TRUE) {
		_update_ice_from_remote_media_description(call, md, is_offer);
	} else {
		/* Response from remote does not contain mandatory ICE attributes, delete the session. */
		linphone_call_delete_ice_session(call);
		linphone_call_set_symmetric_rtp(call, linphone_core_symmetric_rtp_enabled(linphone_call_get_core(call)));
		return;
	}
	if (ice_session_nb_check_lists(call->ice_session) == 0) {
		linphone_call_delete_ice_session(call);
		linphone_call_set_symmetric_rtp(call, linphone_core_symmetric_rtp_enabled(linphone_call_get_core(call)));
	}
}
