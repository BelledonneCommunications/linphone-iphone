
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

#include "linphone/lpconfig.h"
#include "linphone/wrapper_utils.h"
#include "mediastreamer2/mediastream.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

#include "nat/stun-client.h"
#include "utils/payload-type-handler.h"

#include "c-wrapper/c-wrapper.h"

// TODO: From coreapi. Remove me later.
#include "private.h"

void linphone_core_update_allocated_audio_bandwidth(LinphoneCore *lc){
	const bctbx_list_t *elem;
	int maxbw=LinphonePrivate::PayloadTypeHandler::getMinBandwidth(linphone_core_get_download_bandwidth(lc),
					linphone_core_get_upload_bandwidth(lc));
	int max_codec_bitrate=0;

	for(elem=linphone_core_get_audio_codecs(lc);elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		if (payload_type_enabled(pt)){
			int pt_bitrate=LinphonePrivate::PayloadTypeHandler::getAudioPayloadTypeBandwidth(pt,maxbw);
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

bool_t linphone_core_is_payload_type_usable_for_bandwidth(const LinphoneCore *lc, const PayloadType *pt, int bandwidth_limit){
	double codec_band;
	const int video_enablement_limit = 99;
	bool_t ret=FALSE;

	switch (pt->type){
		case PAYLOAD_AUDIO_CONTINUOUS:
		case PAYLOAD_AUDIO_PACKETIZED:
			codec_band=LinphonePrivate::PayloadTypeHandler::getAudioPayloadTypeBandwidth(pt,bandwidth_limit);
			ret=LinphonePrivate::PayloadTypeHandler::bandwidthIsGreater(bandwidth_limit,(int)codec_band);
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

bool_t lp_spawn_command_line_sync(const char *command, char **result,int *command_ret){
#if !defined(_WIN32_WCE) && !defined(LINPHONE_WINDOWS_UNIVERSAL)
	FILE *f=popen(command,"r");
	if (f!=NULL){
		int err;
		*result=reinterpret_cast<char *>(ms_malloc(4096));
		err=(int)fread(*result,1,4096-1,f);
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

int linphone_parse_host_port(const char *input, char *host, size_t hostlen, int *port){
	char tmphost[NI_MAXHOST]={0};

	if ((sscanf(input, "[%64[^]]]:%d", tmphost, port) == 2) || (sscanf(input, "[%64[^]]]", tmphost) == 1)) {

	} else {
		const char *p1 = strchr(input, ':');
		const char *p2 = strrchr(input, ':');
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
	memcpy(ss,res->ai_addr,(size_t)res->ai_addrlen);
	*socklen=(socklen_t)res->ai_addrlen;
	freeaddrinfo(res);
	return 0;
}

/* this functions runs a simple stun test and return the number of milliseconds to complete the tests, or -1 if the test were failed.*/
int linphone_run_stun_tests(LinphoneCore *lc, int audioPort, int videoPort, int textPort,
	char *audioCandidateAddr, int *audioCandidatePort, char *videoCandidateAddr, int *videoCandidatePort, char *textCandidateAddr, int *textCandidatePort) {
	LinphonePrivate::StunClient *client = new LinphonePrivate::StunClient(L_GET_CPP_PTR_FROM_C_OBJECT(lc));
	int ret = client->run(audioPort, videoPort, textPort);
	strncpy(audioCandidateAddr, client->getAudioCandidate().address.c_str(), LINPHONE_IPADDR_SIZE);
	*audioCandidatePort = client->getAudioCandidate().port;
	strncpy(videoCandidateAddr, client->getVideoCandidate().address.c_str(), LINPHONE_IPADDR_SIZE);
	*videoCandidatePort = client->getVideoCandidate().port;
	strncpy(textCandidateAddr, client->getTextCandidate().address.c_str(), LINPHONE_IPADDR_SIZE);
	*textCandidatePort = client->getTextCandidate().port;
	delete client;
	return ret;
}

int linphone_core_get_edge_bw(LinphoneCore *lc){
	int edge_bw=lp_config_get_int(lc->config,"net","edge_bw",20);
	return edge_bw;
}

int linphone_core_get_edge_ptime(LinphoneCore *lc){
	int edge_ptime=lp_config_get_int(lc->config,"net","edge_ptime",100);
	return edge_ptime;
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
		ret&=(unsigned int) ~AUDIO_STREAM_FEATURE_MIXED_RECORDING;
	}
	return ret;
}

bool_t linphone_core_tone_indications_enabled(LinphoneCore*lc){
	return !!lp_config_get_int(lc->config,"sound","tone_indications",1);
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

void linphone_core_set_echo_canceller_filter_name(LinphoneCore *lc, const char *filtername) {
	lp_config_set_string(lc->config, "sound", "ec_filter", filtername);
	if (filtername != NULL) {
		ms_factory_set_echo_canceller_filter_name(lc->factory, filtername);
	}
}

const char * linphone_core_get_echo_canceller_filter_name(const LinphoneCore *lc) {
	return lp_config_get_string(lc->config, "sound", "ec_filter", NULL);
}

/**
 * Queue a task into the main loop. The data pointer must remain valid until the task is completed.
 * task_fun must return BELLE_SIP_STOP when job is finished.
**/
void linphone_core_queue_task(LinphoneCore *lc, belle_sip_source_func_t task_fun, void *data, const char *task_description){
	belle_sip_source_t *s=lc->sal->createTimer(task_fun,data, 20, task_description);
	belle_sip_object_unref(s);
}

static int get_unique_transport(LinphoneCore *lc, LinphoneTransportType *type, int *port){
	LinphoneSipTransports tp;
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

LinphoneStatus linphone_core_migrate_to_multi_transport(LinphoneCore *lc){
	if (!lp_config_get_int(lc->config,"sip","multi_transport_migration_done",0)){
		LinphoneTransportType tpt;
		int port;
		if (get_unique_transport(lc,&tpt,&port)==0){
			LinphoneSipTransports newtp={0};
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

static LinphoneToneDescription *linphone_core_lookup_tone(const LinphoneCore *lc, LinphoneReason reason, LinphoneToneID id){
	const bctbx_list_t *elem;
	for (elem=lc->tones;elem!=NULL;elem=elem->next){
		LinphoneToneDescription *tone=(LinphoneToneDescription*)elem->data;
		if (reason == LinphoneReasonNone){
			if (tone->toneid == id && tone->reason == LinphoneReasonNone) return tone;
		}else{
			if (tone->reason==reason) return tone;
		}
	}
	return NULL;
}

LinphoneToneDescription *linphone_core_get_call_error_tone(const LinphoneCore *lc, LinphoneReason reason){
	return linphone_core_lookup_tone(lc, reason, LinphoneToneUndefined);
}

const char *linphone_core_get_tone_file(const LinphoneCore *lc, LinphoneToneID id){
	LinphoneToneDescription *tone = linphone_core_lookup_tone(lc, LinphoneReasonNone, id);
	return tone ? tone->audiofile : NULL;
}

void _linphone_core_set_tone(LinphoneCore *lc, LinphoneReason reason, LinphoneToneID id, const char *audiofile){
	LinphoneToneDescription *tone = linphone_core_lookup_tone(lc,reason, id);
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
	unsigned long found=0;
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
				result=reinterpret_cast<MSCryptoSuite *>(ms_realloc(result,(found+2)*sizeof(MSCryptoSuite)));
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
		core->supported_formats=reinterpret_cast<const char **>(ms_malloc0(3*sizeof(char*)));
		core->supported_formats[0]=wav;
        if (ms_factory_lookup_filter_by_id(core->factory,MS_MKV_RECORDER_ID)){
			core->supported_formats[1]=mkv;
		}
	}
	return core->supported_formats;
}

bctbx_list_t * linphone_core_get_supported_file_formats_list(LinphoneCore *core){
	bctbx_list_t *file_formats = NULL;
	file_formats = bctbx_list_append(file_formats, ms_strdup("wav"));
	if (ms_factory_lookup_filter_by_id(core->factory,MS_MKV_RECORDER_ID)){
		file_formats = bctbx_list_append(file_formats, ms_strdup("mkv"));
	}
	return file_formats;
}

bool_t linphone_core_file_format_supported(LinphoneCore *lc, const char *fmt){
	const char **formats=linphone_core_get_supported_file_formats(lc);
	for(;*formats!=NULL;++formats){
		if (strcasecmp(*formats,fmt)==0) return TRUE;
	}
	return FALSE;
}

bool_t linphone_core_symmetric_rtp_enabled(LinphoneCore*lc){
	/* Clients don't really need rtp symmetric, unless they have a public IP address and want
	 * to interoperate with natted client. This case is not frequent with client apps.
	 */
	return !!lp_config_get_int(lc->config,"rtp","symmetric",0);
}

LinphoneStatus linphone_core_set_network_simulator_params(LinphoneCore *lc, const OrtpNetworkSimulatorParams *params){
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

void linphone_core_report_call_log(LinphoneCore *lc, LinphoneCallLog *call_log){
	bool_t call_logs_sqlite_db_found = FALSE;

	// TODO: This is a workaround that has to be removed ASAP
	// Do not add calls made to the conference factory in the history
	const char *conference_factory_uri = nullptr;
	LinphoneProxyConfig *proxy = linphone_core_lookup_known_proxy(lc, call_log->to);
	if (proxy)
		conference_factory_uri = linphone_proxy_config_get_conference_factory_uri(proxy);
	if (conference_factory_uri) {
		LinphoneAddress *conference_factory_addr = linphone_address_new(conference_factory_uri);
		if (linphone_address_weak_equal(call_log->to, conference_factory_addr)) {
			linphone_address_unref(conference_factory_addr);
			return;
		}
		linphone_address_unref(conference_factory_addr);
	}
	const char *usernameFrom = linphone_address_get_username(call_log->from);
	const char *usernameTo = linphone_address_get_username(call_log->to);
	if ((usernameFrom && (strstr(usernameFrom, "chatroom-") == usernameFrom))
		|| (usernameTo && (strstr(usernameTo, "chatroom-") == usernameTo))
	)
		return;
	// End of workaround

#ifdef SQLITE_STORAGE_ENABLED
	if (lc->logs_db) {
		call_logs_sqlite_db_found = TRUE;
		linphone_core_store_call_log(lc, call_log);
	}
#endif
	if (!call_logs_sqlite_db_found) {
		lc->call_logs=bctbx_list_prepend(lc->call_logs,linphone_call_log_ref(call_log));
		if (bctbx_list_size(lc->call_logs)>(size_t)lc->max_call_logs){
			bctbx_list_t *elem,*prevelem=NULL;
			/*find the last element*/
			for(elem=lc->call_logs;elem!=NULL;elem=elem->next){
				prevelem = elem;
			}
			elem = prevelem;
			linphone_call_log_unref((LinphoneCallLog*)elem->data);
			lc->call_logs = bctbx_list_erase_link(lc->call_logs,elem);
		}
		call_logs_write_to_config_file(lc);
	}

	linphone_core_notify_call_log_updated(lc,call_log);
}

void linphone_core_report_early_failed_call(LinphoneCore *lc, LinphoneCallDir dir, LinphoneAddress *from, LinphoneAddress *to, LinphoneErrorInfo *ei){
	LinphoneCallLog *l = linphone_call_log_new(dir, from, to);
	l->error_info = ei;
	l->status = LinphoneCallEarlyAborted;
	linphone_core_report_call_log(lc, l);
	linphone_call_log_unref(l);
}

/* Functions to mainpulate the LinphoneRange structure */

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneRange);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneRange, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

LinphoneRange *linphone_range_new() {
	LinphoneRange *range = belle_sip_object_new(LinphoneRange);
	range->min = 0;
	range->max = 0;
	return range;
}

LinphoneRange* linphone_range_ref(LinphoneRange* range) {
	return (LinphoneRange*) belle_sip_object_ref(range);
}

void linphone_range_unref(LinphoneRange* range) {
	belle_sip_object_unref(range);
}

void *linphone_range_get_user_data(const LinphoneRange *range) {
	return range->user_data;
}

void linphone_range_set_user_data(LinphoneRange *range, void *data) {
	range->user_data = data;
}

int linphone_range_get_min(const LinphoneRange *range) {
	return range->min;
}

int linphone_range_get_max(const LinphoneRange *range) {
	return range->max;
}

void linphone_range_set_min(LinphoneRange *range, int min) {
	range->min = min;
}

void linphone_range_set_max(LinphoneRange *range, int max) {
	range->max = max;
}



LinphoneHeaders * linphone_headers_ref(LinphoneHeaders *obj){
	sal_custom_header_ref((SalCustomHeader*)obj);
	return obj;
}


void linphone_headers_unref(LinphoneHeaders *obj){
	sal_custom_header_unref((SalCustomHeader*)obj);
}


const char* linphone_headers_get_value(LinphoneHeaders *obj, const char *header_name){
	return sal_custom_header_find((SalCustomHeader*)obj, header_name);
}

void linphone_headers_add(LinphoneHeaders *obj, const char *name, const char *value){
	sal_custom_header_append((SalCustomHeader*)obj, name, value);
}

void linphone_headers_remove(LinphoneHeaders *obj, const char *name){
	sal_custom_header_remove((SalCustomHeader*)obj, name);
}
