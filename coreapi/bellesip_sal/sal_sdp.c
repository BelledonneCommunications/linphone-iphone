
/*
linphone
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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
#include "sal_impl.h"
#define keywordcmp(key,b) strncmp(key,b,sizeof(key))


static void add_ice_candidates(belle_sdp_media_description_t *md, const SalStreamDescription *desc){
	char buffer[1024];
	const SalIceCandidate *candidate;
	int nb;
	int i;

	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES; i++) {
		candidate = &desc->ice_candidates[i];
		if ((candidate->addr[0] == '\0') || (candidate->port == 0)) break;
		nb = snprintf(buffer, sizeof(buffer), "%s %u UDP %u %s %d typ %s",
			candidate->foundation, candidate->componentID, candidate->priority, candidate->addr, candidate->port, candidate->type);
		if (nb < 0) {
			ms_error("Cannot add ICE candidate attribute!");
			return;
		}
		if (candidate->raddr[0] != '\0') {
			nb = snprintf(buffer + nb, sizeof(buffer) - nb, " raddr %s rport %d", candidate->raddr, candidate->rport);
			if (nb < 0) {
				ms_error("Cannot add ICE candidate attribute!");
				return;
			}
		}
		belle_sdp_media_description_add_attribute(md,belle_sdp_attribute_create("candidate",buffer));
	}
}

static void add_ice_remote_candidates(belle_sdp_media_description_t *md, const SalStreamDescription *desc){
	char buffer[1024];
	char *ptr = buffer;
	const SalIceRemoteCandidate *candidate;
	int offset = 0;
	int i;

	buffer[0] = '\0';
	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES; i++) {
		candidate = &desc->ice_remote_candidates[i];
		if ((candidate->addr[0] != '\0') && (candidate->port != 0)) {
			offset = snprintf(ptr, buffer + sizeof(buffer) - ptr, "%s%d %s %d", (i > 0) ? " " : "", i + 1, candidate->addr, candidate->port);
			if (offset < 0) {
				ms_error("Cannot add ICE remote-candidates attribute!");
				return;
			}
			ptr += offset;
		}
	}
	if (buffer[0] != '\0') belle_sdp_media_description_add_attribute(md,belle_sdp_attribute_create("remote-candidates",buffer));
}

static bool_t is_rtcp_fb_trr_int_the_same_for_all_payloads(const SalStreamDescription *stream, uint16_t *trr_int) {
	bctbx_list_t *pt_it;
	bool_t first = TRUE;
	for (pt_it = stream->payloads; pt_it != NULL; pt_it = pt_it->next) {
		PayloadType *pt = (PayloadType *)pt_it->data;
		if (payload_type_get_flags(pt) & PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED) {
			if (first == TRUE) {
				*trr_int = payload_type_get_avpf_params(pt).trr_interval;
				first = FALSE;
			} else if (payload_type_get_avpf_params(pt).trr_interval != *trr_int) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

static void add_rtcp_fb_trr_int_attribute(belle_sdp_media_description_t *media_desc, int8_t id, uint16_t trr_int) {
	belle_sdp_rtcp_fb_attribute_t *attribute = belle_sdp_rtcp_fb_attribute_new();
	belle_sdp_rtcp_fb_attribute_set_id(attribute, id);
	belle_sdp_rtcp_fb_attribute_set_type(attribute, BELLE_SDP_RTCP_FB_TRR_INT);
	belle_sdp_rtcp_fb_attribute_set_trr_int(attribute, trr_int);
	belle_sdp_media_description_add_attribute(media_desc, BELLE_SDP_ATTRIBUTE(attribute));
}

static void add_rtcp_fb_ack_attribute(belle_sdp_media_description_t *media_desc, int8_t id, belle_sdp_rtcp_fb_val_param_t param) {
	belle_sdp_rtcp_fb_attribute_t *attribute = belle_sdp_rtcp_fb_attribute_new();
	belle_sdp_rtcp_fb_attribute_set_id(attribute, id);
	belle_sdp_rtcp_fb_attribute_set_type(attribute, BELLE_SDP_RTCP_FB_ACK);
	belle_sdp_rtcp_fb_attribute_set_param(attribute, param);
	belle_sdp_media_description_add_attribute(media_desc, BELLE_SDP_ATTRIBUTE(attribute));
}

static void add_rtcp_fb_nack_attribute(belle_sdp_media_description_t *media_desc, int8_t id, belle_sdp_rtcp_fb_val_param_t param) {
	belle_sdp_rtcp_fb_attribute_t *attribute = belle_sdp_rtcp_fb_attribute_new();
	belle_sdp_rtcp_fb_attribute_set_id(attribute, id);
	belle_sdp_rtcp_fb_attribute_set_type(attribute, BELLE_SDP_RTCP_FB_NACK);
	belle_sdp_rtcp_fb_attribute_set_param(attribute, param);
	belle_sdp_media_description_add_attribute(media_desc, BELLE_SDP_ATTRIBUTE(attribute));
}

static void add_rtcp_fb_ccm_attribute(belle_sdp_media_description_t *media_desc, int8_t id, belle_sdp_rtcp_fb_val_param_t param) {
	belle_sdp_rtcp_fb_attribute_t *attribute = belle_sdp_rtcp_fb_attribute_new();
	belle_sdp_rtcp_fb_attribute_set_id(attribute, id);
	belle_sdp_rtcp_fb_attribute_set_type(attribute, BELLE_SDP_RTCP_FB_CCM);
	belle_sdp_rtcp_fb_attribute_set_param(attribute, param);
	belle_sdp_media_description_add_attribute(media_desc, BELLE_SDP_ATTRIBUTE(attribute));
}

static void add_rtcp_fb_attributes(belle_sdp_media_description_t *media_desc, const SalMediaDescription *md, const SalStreamDescription *stream) {
	bctbx_list_t *pt_it;
	PayloadType *pt;
	PayloadTypeAvpfParams avpf_params;
	bool_t general_trr_int;
	uint16_t trr_int = 0;

	general_trr_int = is_rtcp_fb_trr_int_the_same_for_all_payloads(stream, &trr_int);
	if (general_trr_int == TRUE && trr_int != 0) {
		add_rtcp_fb_trr_int_attribute(media_desc, -1, trr_int);
	}
	if (stream->rtcp_fb.generic_nack_enabled == TRUE) {
		add_rtcp_fb_nack_attribute(media_desc, -1, BELLE_SDP_RTCP_FB_NONE);
	}
	if (stream->rtcp_fb.tmmbr_enabled == TRUE) {
		add_rtcp_fb_ccm_attribute(media_desc, -1, BELLE_SDP_RTCP_FB_TMMBR);
	}

	for (pt_it = stream->payloads; pt_it != NULL; pt_it = pt_it->next) {
		pt = (PayloadType *)pt_it->data;

		/* AVPF/SAVPF profile is used so enable AVPF for all paylad types. */
		payload_type_set_flag(pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
		avpf_params = payload_type_get_avpf_params(pt);

		/* Add trr-int if not set generally. */
		if (general_trr_int != TRUE && trr_int != 0) {
			add_rtcp_fb_trr_int_attribute(media_desc, payload_type_get_number(pt), avpf_params.trr_interval);
		}

		/* Add rtcp-fb attributes according to the AVPF features of the payload types. */
		if (avpf_params.features & PAYLOAD_TYPE_AVPF_PLI) {
			add_rtcp_fb_nack_attribute(media_desc, payload_type_get_number(pt), BELLE_SDP_RTCP_FB_PLI);
		}
		if (avpf_params.features & PAYLOAD_TYPE_AVPF_SLI) {
			add_rtcp_fb_nack_attribute(media_desc, payload_type_get_number(pt), BELLE_SDP_RTCP_FB_SLI);
		}
		if (avpf_params.features & PAYLOAD_TYPE_AVPF_RPSI) {
			if (avpf_params.rpsi_compatibility == TRUE) {
				add_rtcp_fb_nack_attribute(media_desc, payload_type_get_number(pt), BELLE_SDP_RTCP_FB_RPSI);
			} else {
				add_rtcp_fb_ack_attribute(media_desc, payload_type_get_number(pt), BELLE_SDP_RTCP_FB_RPSI);
			}
		}
		if (avpf_params.features & PAYLOAD_TYPE_AVPF_FIR) {
			add_rtcp_fb_ccm_attribute(media_desc, payload_type_get_number(pt), BELLE_SDP_RTCP_FB_FIR);
		}
	}
}

static belle_sdp_attribute_t * create_rtcp_xr_attribute(const OrtpRtcpXrConfiguration *config) {
	belle_sdp_rtcp_xr_attribute_t *attribute = belle_sdp_rtcp_xr_attribute_new();
	if (config->rcvr_rtt_mode != OrtpRtcpXrRcvrRttNone) {
		if (config->rcvr_rtt_mode == OrtpRtcpXrRcvrRttAll) belle_sdp_rtcp_xr_attribute_set_rcvr_rtt_mode(attribute, "all");
		else if (config->rcvr_rtt_mode == OrtpRtcpXrRcvrRttSender) belle_sdp_rtcp_xr_attribute_set_rcvr_rtt_mode(attribute, "sender");
		belle_sdp_rtcp_xr_attribute_set_rcvr_rtt_max_size(attribute, config->rcvr_rtt_max_size);
	}
	belle_sdp_rtcp_xr_attribute_set_stat_summary(attribute, (config->stat_summary_enabled == TRUE));
	if (config->stat_summary_enabled == TRUE) {
		if (config->stat_summary_flags & OrtpRtcpXrStatSummaryLoss) belle_sdp_rtcp_xr_attribute_add_stat_summary_flag(attribute, "loss");
		if (config->stat_summary_flags & OrtpRtcpXrStatSummaryDup) belle_sdp_rtcp_xr_attribute_add_stat_summary_flag(attribute, "dup");
		if (config->stat_summary_flags & OrtpRtcpXrStatSummaryJitt) belle_sdp_rtcp_xr_attribute_add_stat_summary_flag(attribute, "jitt");
		if (config->stat_summary_flags & OrtpRtcpXrStatSummaryTTL) belle_sdp_rtcp_xr_attribute_add_stat_summary_flag(attribute, "TTL");
		if (config->stat_summary_flags & OrtpRtcpXrStatSummaryHL) belle_sdp_rtcp_xr_attribute_add_stat_summary_flag(attribute, "HL");
	}
	belle_sdp_rtcp_xr_attribute_set_voip_metrics(attribute, (config->voip_metrics_enabled == TRUE));
	return BELLE_SDP_ATTRIBUTE(attribute);
}

static void stream_description_to_sdp ( belle_sdp_session_description_t *session_desc, const SalMediaDescription *md, const SalStreamDescription *stream ) {
	belle_sdp_mime_parameter_t* mime_param;
	belle_sdp_media_description_t* media_desc;
	int j;
	bctbx_list_t* pt_it;
	PayloadType* pt;
	char buffer[1024];
	char* dir=NULL;
	const char *rtp_addr;
	const char *rtcp_addr;
	int rtp_port;
	int rtcp_port;
	bool_t different_rtp_and_rtcp_addr;

	rtp_addr=stream->rtp_addr;
	rtcp_addr=stream->rtcp_addr;
	rtp_port=stream->rtp_port;
	rtcp_port=stream->rtcp_port;

	media_desc = belle_sdp_media_description_create ( sal_stream_description_get_type_as_string(stream)
				 ,stream->rtp_port
				 ,1
				 ,sal_media_proto_to_string ( stream->proto )
				 ,NULL );
	if (stream->payloads) {
		for ( pt_it=stream->payloads; pt_it!=NULL; pt_it=pt_it->next ) {
			pt= ( PayloadType* ) pt_it->data;
			mime_param= belle_sdp_mime_parameter_create ( pt->mime_type
					, payload_type_get_number ( pt )
					, pt->clock_rate
					, pt->channels>0 ? pt->channels : -1 );
			belle_sdp_mime_parameter_set_parameters ( mime_param,pt->recv_fmtp );
			if ( stream->ptime>0 ) {
				belle_sdp_mime_parameter_set_ptime ( mime_param,stream->ptime );
			}
			belle_sdp_media_description_append_values_from_mime_parameter ( media_desc,mime_param );
			belle_sip_object_unref ( mime_param );
		}
	} else {
		/* to comply with SDP we cannot have an empty payload type number list */
		/* as it happens only when mline is declined with a zero port, it does not matter to put whatever codec*/
		belle_sip_list_t* format = belle_sip_list_append(NULL,0);
		belle_sdp_media_set_media_formats(belle_sdp_media_description_get_media(media_desc),format);
	}
	/*only add a c= line within the stream description if address are differents*/
	if (rtp_addr[0]!='\0' && strcmp(rtp_addr,md->addr)!=0){
		bool_t inet6;
		belle_sdp_connection_t *connection;
		if (strchr(rtp_addr,':')!=NULL){
			inet6=TRUE;
		}else inet6=FALSE;
		connection = belle_sdp_connection_create("IN", inet6 ? "IP6" : "IP4", rtp_addr);
		if (ms_is_multicast(rtp_addr)) {
			/*remove session cline in case of multicast*/
			belle_sdp_session_description_set_connection(session_desc,NULL);
			if (inet6 == FALSE)
				belle_sdp_connection_set_ttl(connection,stream->ttl);
		}
		belle_sdp_media_description_set_connection(media_desc,connection);
	}

	if ( stream->bandwidth>0 )
		belle_sdp_media_description_set_bandwidth ( media_desc,"AS",stream->bandwidth );

	if (sal_stream_description_has_srtp(stream)) {
		/* add crypto lines */
		for ( j=0; j<SAL_CRYPTO_ALGO_MAX; j++ ) {
			MSCryptoSuiteNameParams desc;
			if (ms_crypto_suite_to_name_params(stream->crypto[j].algo,&desc)==0){
				if (desc.params)
					snprintf ( buffer, sizeof ( buffer )-1, "%d %s inline:%s %s", stream->crypto[j].tag, desc.name, stream->crypto[j].master_key,desc.params);
				else 
					snprintf ( buffer, sizeof ( buffer )-1, "%d %s inline:%s", stream->crypto[j].tag, desc.name, stream->crypto[j].master_key );
				
				belle_sdp_media_description_add_attribute( media_desc,belle_sdp_attribute_create ("crypto", buffer));
			}else break;
		}
	}

	/* insert DTLS session attribute if needed */
	if ((stream->proto == SalProtoUdpTlsRtpSavpf) || (stream->proto == SalProtoUdpTlsRtpSavp)) {
		char* ssrc_attribute = ms_strdup_printf("%u cname:%s",stream->rtp_ssrc,stream->rtcp_cname);
		if ((stream->dtls_role != SalDtlsRoleInvalid) && (strlen(stream->dtls_fingerprint)>0)) {
			switch(stream->dtls_role) {
				case SalDtlsRoleIsClient:
					belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create("setup","active"));
					break;
				case SalDtlsRoleIsServer:
					belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create("setup","passive"));
					break;
				case SalDtlsRoleUnset:
				default:
					belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create("setup","actpass"));
					break;
			}
			belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create("fingerprint",stream->dtls_fingerprint));
		}
		belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create("ssrc",ssrc_attribute));
		ms_free(ssrc_attribute);
	}

	/* insert zrtp-hash attribute if needed */
	if (stream->haveZrtpHash == 1) {
		belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create("zrtp-hash", (const char *)(stream->zrtphash)));
	}

	switch ( stream->dir ) {
		case SalStreamSendRecv:
			/*dir="sendrecv";*/
			dir=NULL;
			break;
		case SalStreamRecvOnly:
			dir="recvonly";
			break;
		case SalStreamSendOnly:
			dir="sendonly";
			break;
		case SalStreamInactive:
			dir="inactive";
			break;
	}
	if ( dir ) belle_sdp_media_description_add_attribute ( media_desc,belle_sdp_attribute_create ( dir,NULL ) );
	
	if (stream->rtcp_mux){
		belle_sdp_media_description_add_attribute(media_desc, belle_sdp_attribute_create ("rtcp-mux",NULL ) );
	}
	
	if (rtp_port != 0) {
		different_rtp_and_rtcp_addr = (rtcp_addr[0] != '\0') && (strcmp(rtp_addr, rtcp_addr) != 0);
		if ((rtcp_port != (rtp_port + 1)) || (different_rtp_and_rtcp_addr == TRUE)) {
			if (different_rtp_and_rtcp_addr == TRUE) {
				snprintf(buffer, sizeof(buffer), "%u IN IP4 %s", rtcp_port, rtcp_addr);
			} else {
				snprintf(buffer, sizeof(buffer), "%u",rtcp_port);
			}
			belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create ("rtcp",buffer));
		}
	}
	if (stream->set_nortpproxy == TRUE) {
		belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create ("nortpproxy","yes"));
	}
	if (stream->ice_mismatch == TRUE) {
		belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create ("ice-mismatch",NULL));
	} else {
		if (rtp_port != 0) {
			if (stream->ice_pwd[0] != '\0') 
				belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create ("ice-pwd",stream->ice_pwd));
			if (stream->ice_ufrag[0] != '\0')
				belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create ("ice-ufrag",stream->ice_ufrag));
			add_ice_candidates(media_desc,stream);
			add_ice_remote_candidates(media_desc,stream);
		}
	}

	if ((rtp_port != 0) && (sal_stream_description_has_avpf(stream) || sal_stream_description_has_implicit_avpf(stream))) {
		add_rtcp_fb_attributes(media_desc, md, stream);
	}

	if ((rtp_port != 0) && (stream->rtcp_xr.enabled == TRUE)) {
		char sastr[1024] = {0};
		char mastr[1024] = {0};
		size_t saoff = 0;
		size_t maoff = 0;
		const belle_sdp_attribute_t *session_attribute = belle_sdp_session_description_get_attribute(session_desc, "rtcp-xr");
		belle_sdp_attribute_t *media_attribute;
		if (session_attribute != NULL) {
			belle_sip_object_marshal((belle_sip_object_t*)session_attribute, sastr, sizeof(sastr), &saoff);
		}
		media_attribute = create_rtcp_xr_attribute(&stream->rtcp_xr);
		if (media_attribute != NULL) {
			belle_sip_object_marshal((belle_sip_object_t*)media_attribute, mastr, sizeof(mastr), &maoff);
		}
		if (strcmp(sastr, mastr) != 0) {
			belle_sdp_media_description_add_attribute(media_desc, media_attribute);
		} else {
			belle_sip_object_unref((belle_sip_object_t*)media_attribute);
		}
	}

	if (stream->custom_sdp_attributes) {
		belle_sdp_session_description_t *custom_desc = (belle_sdp_session_description_t *)stream->custom_sdp_attributes;
		belle_sip_list_t *l = belle_sdp_session_description_get_attributes(custom_desc);
		belle_sip_list_t *elem;
		for (elem = l; elem != NULL; elem = elem->next) {
			belle_sdp_media_description_add_attribute(media_desc, (belle_sdp_attribute_t *)elem->data);
		}
	}

	/*
	 * rfc5576
	 * 4.1.  The "ssrc" Media Attribute
	 * <ssrc-id> is the synchronization source (SSRC) ID of the
	 * source being described, interpreted as a 32-bit unsigned integer in
	 * network byte order and represented in decimal.*/


	belle_sdp_session_description_add_media_description(session_desc, media_desc);
}

belle_sdp_session_description_t * media_description_to_sdp ( const SalMediaDescription *desc ) {
	belle_sdp_session_description_t* session_desc=belle_sdp_session_description_new();
	bool_t inet6;
	belle_sdp_origin_t* origin;
	int i;

	if ( strchr ( desc->addr,':' ) !=NULL ) {
		inet6=1;
	} else inet6=0;
	belle_sdp_session_description_set_version ( session_desc,belle_sdp_version_create ( 0 ) );

	origin = belle_sdp_origin_create ( desc->username
									  ,desc->session_id
									  ,desc->session_ver
									  ,"IN"
									  , inet6 ? "IP6" :"IP4"
									  ,desc->addr );

	belle_sdp_session_description_set_origin ( session_desc,origin );

	belle_sdp_session_description_set_session_name ( session_desc,
		belle_sdp_session_name_create ( desc->name[0]!='\0' ? desc->name : "Talk" ) );

	if ( !sal_media_description_has_dir ( desc,SalStreamInactive ) || desc->ice_ufrag[0] != '\0' ) {
		/*in case of sendonly, setting of the IP on cnx we give a chance to receive stun packets*/
		belle_sdp_session_description_set_connection ( session_desc
				,belle_sdp_connection_create ( "IN",inet6 ? "IP6" :"IP4",desc->addr ) );

	} else 	{
		belle_sdp_session_description_set_connection ( session_desc
				,belle_sdp_connection_create ( "IN"
								,inet6 ? "IP6" :"IP4"
								,inet6 ? "::0" :"0.0.0.0" ) );

	}

	belle_sdp_session_description_set_time_description ( session_desc,belle_sdp_time_description_create ( 0,0 ) );

	if ( desc->bandwidth>0 ) {
		belle_sdp_session_description_set_bandwidth ( session_desc,"AS",desc->bandwidth );
	}
	
	if (desc->set_nortpproxy == TRUE) belle_sdp_session_description_add_attribute(session_desc, belle_sdp_attribute_create("nortpproxy","yes"));
	if (desc->ice_pwd[0] != '\0') belle_sdp_session_description_add_attribute(session_desc, belle_sdp_attribute_create("ice-pwd",desc->ice_pwd));
	if (desc->ice_ufrag[0] != '\0') belle_sdp_session_description_add_attribute(session_desc, belle_sdp_attribute_create("ice-ufrag",desc->ice_ufrag));

	if (desc->rtcp_xr.enabled == TRUE) {
		belle_sdp_session_description_add_attribute(session_desc, create_rtcp_xr_attribute(&desc->rtcp_xr));
	}
	if (desc->custom_sdp_attributes) {
		belle_sdp_session_description_t *custom_desc = (belle_sdp_session_description_t *)desc->custom_sdp_attributes;
		belle_sip_list_t *l = belle_sdp_session_description_get_attributes(custom_desc);
		belle_sip_list_t *elem;
		for (elem = l; elem != NULL; elem = elem->next) {
			belle_sdp_session_description_add_attribute(session_desc, (belle_sdp_attribute_t *)elem->data);
		}
	}

	for ( i=0; i<desc->nb_streams; i++ ) {
		stream_description_to_sdp(session_desc, desc, &desc->streams[i]);
	}
	return session_desc;
}


static void sdp_parse_payload_types(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	PayloadType *pt;
	PayloadTypeAvpfParams avpf_params;
	belle_sip_list_t* mime_param_it=NULL;
	belle_sdp_mime_parameter_t* mime_param;
	belle_sip_list_t* mime_params=belle_sdp_media_description_build_mime_parameters ( media_desc );
	memset(&avpf_params, 0, sizeof(avpf_params));
	for ( mime_param_it=mime_params
						; mime_param_it!=NULL
			; mime_param_it=mime_param_it->next ) {
		mime_param=BELLE_SDP_MIME_PARAMETER ( mime_param_it->data );

		pt=payload_type_new();
		payload_type_set_number ( pt,belle_sdp_mime_parameter_get_media_format ( mime_param ) );
		pt->clock_rate=belle_sdp_mime_parameter_get_rate ( mime_param );
		pt->mime_type=ms_strdup ( belle_sdp_mime_parameter_get_type ( mime_param ) );
		pt->channels=belle_sdp_mime_parameter_get_channel_count ( mime_param );
		payload_type_set_send_fmtp ( pt,belle_sdp_mime_parameter_get_parameters ( mime_param ) );
		payload_type_set_avpf_params(pt, avpf_params);
		stream->payloads=bctbx_list_append ( stream->payloads,pt );
		stream->ptime=belle_sdp_mime_parameter_get_ptime ( mime_param );
		ms_message ( "Found payload %s/%i fmtp=%s",pt->mime_type,pt->clock_rate,
						pt->send_fmtp ? pt->send_fmtp : "" );
	}
	if ( mime_params ) belle_sip_list_free_with_data ( mime_params,belle_sip_object_unref );
}

static void sdp_parse_media_crypto_parameters(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	belle_sip_list_t *attribute_it;
	belle_sdp_attribute_t *attribute;
	char tmp[256], tmp2[256], parameters[256]={0};
	int valid_count = 0;
	int nb;

	memset ( &stream->crypto, 0, sizeof ( stream->crypto ) );
	for ( attribute_it=belle_sdp_media_description_get_attributes ( media_desc )
						; valid_count < SAL_CRYPTO_ALGO_MAX && attribute_it!=NULL;
			attribute_it=attribute_it->next ) {
		attribute=BELLE_SDP_ATTRIBUTE ( attribute_it->data );

		if ( keywordcmp ( "crypto",belle_sdp_attribute_get_name ( attribute ) ) ==0 && belle_sdp_attribute_get_value ( attribute ) !=NULL ) {
			nb = sscanf ( belle_sdp_attribute_get_value ( attribute ), "%d %256s inline:%256s %256s",
							&stream->crypto[valid_count].tag,
							tmp,
							tmp2, parameters );
			
			if ( nb >= 3 ) {
				MSCryptoSuite cs;
				MSCryptoSuiteNameParams np;

				np.name=tmp;
				np.params=parameters[0]!='\0' ? parameters : NULL;
				cs=ms_crypto_suite_build_from_name_params(&np);
				if (cs==MS_CRYPTO_SUITE_INVALID){
					ms_warning ( "Failed to parse crypto-algo: '%s'", tmp );
					stream->crypto[valid_count].algo = 0;
				}else{
					char *sep;
					strncpy ( stream->crypto[valid_count].master_key, tmp2, sizeof(stream->crypto[valid_count].master_key)-1 );
					sep=strchr(stream->crypto[valid_count].master_key,'|');
					if (sep) *sep='\0';
					stream->crypto[valid_count].algo = cs;
					ms_message ( "Found valid crypto line (tag:%d algo:'%s' key:'%s'",
									stream->crypto[valid_count].tag,
									tmp,
									stream->crypto[valid_count].master_key );
					valid_count++;
				}
			}else{
				ms_warning ( "sdp has a strange a= line (%s) nb=%i",belle_sdp_attribute_get_value ( attribute ),nb );
			}
		}
	}
	ms_message("Found: %d valid crypto lines", valid_count );
}

static void sdp_parse_media_ice_parameters(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	belle_sip_list_t *attribute_it;
	belle_sdp_attribute_t *attribute;
	const char *att_name;
	const char *value;
	int nb_ice_candidates = 0;

	for (attribute_it = belle_sdp_media_description_get_attributes(media_desc); attribute_it != NULL; attribute_it=attribute_it->next) {
		attribute=BELLE_SDP_ATTRIBUTE(attribute_it->data);
		att_name = belle_sdp_attribute_get_name(attribute);
		value = belle_sdp_attribute_get_value(attribute);

		if ((nb_ice_candidates < (int)(sizeof(stream->ice_candidates)/sizeof(SalIceCandidate)))
				&& (keywordcmp("candidate", att_name) == 0)
				&& (value != NULL)) {
			SalIceCandidate *candidate = &stream->ice_candidates[nb_ice_candidates];
			char proto[4];
			int nb = sscanf(value, "%s %u %3s %u %s %d typ %s raddr %s rport %d",
				candidate->foundation, &candidate->componentID, proto, &candidate->priority, candidate->addr, &candidate->port,
				candidate->type, candidate->raddr, &candidate->rport);
			if (strcasecmp("udp",proto)==0 && ((nb == 7) || (nb == 9))) nb_ice_candidates++;
			else {
				ms_error("ice: Failed parsing a=candidate SDP attribute");
				memset(candidate, 0, sizeof(*candidate));
			}
		} else if ((keywordcmp("remote-candidates", att_name) == 0) && (value != NULL)) {
			SalIceRemoteCandidate candidate;
			unsigned int componentID;
			int offset;
			const char *ptr = value;
			const char *endptr = value + strlen(ptr);
			while (3 == sscanf(ptr, "%u %s %u%n", &componentID, candidate.addr, &candidate.port, &offset)) {
				if ((componentID > 0) && (componentID <= SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES)) {
					SalIceRemoteCandidate *remote_candidate = &stream->ice_remote_candidates[componentID - 1];
					strncpy(remote_candidate->addr, candidate.addr, sizeof(remote_candidate->addr)-1);
					remote_candidate->port = candidate.port;
				}
				ptr += offset;
				if (ptr < endptr) {
					if (ptr[offset] == ' ') ptr += 1;
				} else break;
			}
		} else if ((keywordcmp("ice-ufrag", att_name) == 0) && (value != NULL)) {
			strncpy(stream->ice_ufrag, value, sizeof(stream->ice_ufrag)-1);
		} else if ((keywordcmp("ice-pwd", att_name) == 0) && (value != NULL)) {
			strncpy(stream->ice_pwd, value, sizeof(stream->ice_pwd) -1);
		} else if (keywordcmp("ice-mismatch", att_name) == 0) {
			stream->ice_mismatch = TRUE;
		}
	}
}

static void enable_avpf_for_stream(SalStreamDescription *stream) {
	bctbx_list_t *pt_it;
	for (pt_it = stream->payloads; pt_it != NULL; pt_it = pt_it->next) {
		PayloadType *pt = (PayloadType *)pt_it->data;
		payload_type_set_flag(pt, PAYLOAD_TYPE_RTCP_FEEDBACK_ENABLED);
	}
}

static void apply_rtcp_fb_attribute_to_payload(belle_sdp_rtcp_fb_attribute_t *fb_attribute, SalStreamDescription *stream, PayloadType *pt) {
	PayloadTypeAvpfParams avpf_params = payload_type_get_avpf_params(pt);
	switch (belle_sdp_rtcp_fb_attribute_get_type(fb_attribute)) {
		case BELLE_SDP_RTCP_FB_ACK:
			if (belle_sdp_rtcp_fb_attribute_get_param(fb_attribute) == BELLE_SDP_RTCP_FB_RPSI) {
				avpf_params.features |= PAYLOAD_TYPE_AVPF_RPSI;
			}
			break;
		case BELLE_SDP_RTCP_FB_NACK:
			switch (belle_sdp_rtcp_fb_attribute_get_param(fb_attribute)) {
				case BELLE_SDP_RTCP_FB_PLI:
					avpf_params.features |= PAYLOAD_TYPE_AVPF_PLI;
					break;
				case BELLE_SDP_RTCP_FB_SLI:
					avpf_params.features |= PAYLOAD_TYPE_AVPF_SLI;
					break;
				case BELLE_SDP_RTCP_FB_RPSI:
					/* Linphone uses positive feeback for RPSI. However first versions handling
					 * AVPF wrongly declared RPSI as negative feedback, so this is kept for compatibility
					 * with these versions but will probably be removed at some point in time. */
					avpf_params.features |= PAYLOAD_TYPE_AVPF_RPSI;
					avpf_params.rpsi_compatibility = TRUE;
					break;
				case BELLE_SDP_RTCP_FB_NONE:
					stream->rtcp_fb.generic_nack_enabled = TRUE;
					break;
				default:
					break;
			}
			break;
		case BELLE_SDP_RTCP_FB_TRR_INT:
			avpf_params.trr_interval = belle_sdp_rtcp_fb_attribute_get_trr_int(fb_attribute);
			break;
		case BELLE_SDP_RTCP_FB_CCM:
			switch (belle_sdp_rtcp_fb_attribute_get_param(fb_attribute)) {
				case BELLE_SDP_RTCP_FB_FIR:
					avpf_params.features |= PAYLOAD_TYPE_AVPF_FIR;
					break;
				case BELLE_SDP_RTCP_FB_TMMBR:
					stream->rtcp_fb.tmmbr_enabled = TRUE;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	payload_type_set_avpf_params(pt, avpf_params);
}

static bool_t sdp_parse_rtcp_fb_parameters(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	belle_sip_list_t *it;
	belle_sdp_attribute_t *attribute;
	belle_sdp_rtcp_fb_attribute_t *fb_attribute;
	bctbx_list_t *pt_it;
	PayloadType *pt;
	int8_t pt_num;
	bool_t retval = FALSE;
    
	/* Handle rtcp-fb attributes that concern all payload types. */
	for (it = belle_sdp_media_description_get_attributes(media_desc); it != NULL; it = it->next) {
		attribute = BELLE_SDP_ATTRIBUTE(it->data);
		if (keywordcmp("rtcp-fb", belle_sdp_attribute_get_name(attribute)) == 0) {
			fb_attribute = BELLE_SDP_RTCP_FB_ATTRIBUTE(attribute);
			if (belle_sdp_rtcp_fb_attribute_get_id(fb_attribute) == -1) {
				for (pt_it = stream->payloads; pt_it != NULL; pt_it = pt_it->next) {
					pt = (PayloadType *)pt_it->data;
					apply_rtcp_fb_attribute_to_payload(fb_attribute, stream, pt);
					retval = TRUE;
				}
			}
		}
	}

	/* Handle rtcp-fb attributes that are specefic to a payload type. */
	for (it = belle_sdp_media_description_get_attributes(media_desc); it != NULL; it = it->next) {
		attribute = BELLE_SDP_ATTRIBUTE(it->data);
		if (keywordcmp("rtcp-fb", belle_sdp_attribute_get_name(attribute)) == 0) {
			fb_attribute = BELLE_SDP_RTCP_FB_ATTRIBUTE(attribute);
			pt_num = belle_sdp_rtcp_fb_attribute_get_id(fb_attribute);
			for (pt_it = stream->payloads; pt_it != NULL; pt_it = pt_it->next) {
				pt = (PayloadType *)pt_it->data;
				retval = TRUE;
				if (payload_type_get_number(pt) == (int)pt_num) {
					apply_rtcp_fb_attribute_to_payload(fb_attribute, stream, pt);
				}
			}
		}
	}
	return retval;
}

static void sal_init_rtcp_xr_description(OrtpRtcpXrConfiguration *config) {
	config->enabled = FALSE;
	config->rcvr_rtt_mode = OrtpRtcpXrRcvrRttNone;
	config->rcvr_rtt_max_size = -1;
	config->stat_summary_flags = 0;
	config->voip_metrics_enabled = FALSE;
}

static void sdp_parse_rtcp_xr_parameters(const belle_sdp_attribute_t *attribute, OrtpRtcpXrConfiguration *config) {
	if (attribute != NULL) {
		const belle_sdp_rtcp_xr_attribute_t *xr_attr;
		const char *rcvr_rtt_mode;
		sal_init_rtcp_xr_description(config);
		xr_attr = BELLE_SDP_RTCP_XR_ATTRIBUTE(attribute);
		rcvr_rtt_mode = belle_sdp_rtcp_xr_attribute_get_rcvr_rtt_mode(xr_attr);
		if (rcvr_rtt_mode != NULL) {
			if (strcasecmp(rcvr_rtt_mode, "all") == 0) {
				config->rcvr_rtt_mode = OrtpRtcpXrRcvrRttAll;
			} else if (strcasecmp(rcvr_rtt_mode, "sender") == 0) {
				config->rcvr_rtt_mode = OrtpRtcpXrRcvrRttSender;
			}
			config->rcvr_rtt_max_size = belle_sdp_rtcp_xr_attribute_get_rcvr_rtt_max_size(xr_attr);
		}
		config->stat_summary_enabled = (belle_sdp_rtcp_xr_attribute_has_stat_summary(xr_attr) != 0);
		if (config->stat_summary_enabled) {
			const belle_sip_list_t *stat_summary_flag_it;
			for (stat_summary_flag_it = belle_sdp_rtcp_xr_attribute_get_stat_summary_flags(xr_attr); stat_summary_flag_it != NULL; stat_summary_flag_it = stat_summary_flag_it->next ) {
				const char *flag = (const char *)stat_summary_flag_it->data;
				if (flag != NULL) {
					if (strcasecmp(flag, "loss") == 0) config->stat_summary_flags |= OrtpRtcpXrStatSummaryLoss;
					else if (strcasecmp(flag, "dup") == 0) config->stat_summary_flags |= OrtpRtcpXrStatSummaryDup;
					else if (strcasecmp(flag, "jitt") == 0) config->stat_summary_flags |= OrtpRtcpXrStatSummaryJitt;
					else if (strcasecmp(flag, "TTL") == 0) config->stat_summary_flags |= OrtpRtcpXrStatSummaryTTL;
					else if (strcasecmp(flag, "HL") == 0) config->stat_summary_flags |= OrtpRtcpXrStatSummaryHL;
				}
			}
		}
		config->voip_metrics_enabled = (belle_sdp_rtcp_xr_attribute_has_voip_metrics(xr_attr) != 0);
		config->enabled = TRUE;
	}
}

static void sdp_parse_session_rtcp_xr_parameters(belle_sdp_session_description_t *session_desc, OrtpRtcpXrConfiguration *config) {
	const belle_sdp_attribute_t *attribute = belle_sdp_session_description_get_attribute(session_desc, "rtcp-xr");
	sdp_parse_rtcp_xr_parameters(attribute, config);
}

static void sdp_parse_media_rtcp_xr_parameters(belle_sdp_media_description_t *media_desc, OrtpRtcpXrConfiguration *config) {
	const belle_sdp_attribute_t *attribute = belle_sdp_media_description_get_attribute(media_desc, "rtcp-xr");
	sdp_parse_rtcp_xr_parameters(attribute, config);
}

static SalStreamDescription * sdp_to_stream_description(SalMediaDescription *md, belle_sdp_media_description_t *media_desc) {
	SalStreamDescription *stream;
	belle_sdp_connection_t* cnx;
	belle_sdp_media_t* media;
	belle_sdp_attribute_t* attribute;
	belle_sip_list_t *custom_attribute_it;
	const char* value;
	const char *mtype,*proto;
	bool_t has_avpf_attributes;

	stream=&md->streams[md->nb_streams];
	media=belle_sdp_media_description_get_media ( media_desc );

	proto = belle_sdp_media_get_protocol ( media );
	stream->proto=SalProtoOther;
	if ( proto ) {
		if (strcasecmp(proto, "RTP/AVP") == 0) {
			stream->proto = SalProtoRtpAvp;
		} else if (strcasecmp(proto, "RTP/SAVP") == 0) {
			stream->proto = SalProtoRtpSavp;
		} else if (strcasecmp(proto, "RTP/AVPF") == 0) {
			stream->proto = SalProtoRtpAvpf;
		} else if (strcasecmp(proto, "RTP/SAVPF") == 0) {
			stream->proto = SalProtoRtpSavpf;
		} else if (strcasecmp(proto, "UDP/TLS/RTP/SAVP") == 0) {
			stream->proto = SalProtoUdpTlsRtpSavp;
		} else if (strcasecmp(proto, "UDP/TLS/RTP/SAVPF") == 0) {
			stream->proto = SalProtoUdpTlsRtpSavpf;
		} else {
			strncpy(stream->proto_other,proto,sizeof(stream->proto_other)-1);
		}
	}
	if ( ( cnx=belle_sdp_media_description_get_connection ( media_desc ) ) && belle_sdp_connection_get_address ( cnx ) ) {
		strncpy ( stream->rtp_addr,belle_sdp_connection_get_address ( cnx ), sizeof ( stream->rtp_addr ) -1 );
		stream->ttl=belle_sdp_connection_get_ttl(cnx);
	}

	stream->rtp_port=belle_sdp_media_get_media_port ( media );

	mtype = belle_sdp_media_get_media_type ( media );
	if ( strcasecmp ( "audio", mtype ) == 0 ) {
		stream->type=SalAudio;
	} else if ( strcasecmp ( "video", mtype ) == 0 ) {
		stream->type=SalVideo;
	} else if ( strcasecmp ( "text", mtype ) == 0 ) {
		stream->type=SalText;
	} else {
		stream->type=SalOther;
		strncpy ( stream->typeother,mtype,sizeof ( stream->typeother )-1 );
	}

	if ( belle_sdp_media_description_get_bandwidth ( media_desc,"AS" ) >0 ) {
		stream->bandwidth=belle_sdp_media_description_get_bandwidth ( media_desc,"AS" );
	}

	if ( belle_sdp_media_description_get_attribute ( media_desc,"sendrecv" ) ) {
		stream->dir=SalStreamSendRecv;
	} else if ( belle_sdp_media_description_get_attribute ( media_desc,"sendonly" ) ) {
		stream->dir=SalStreamSendOnly;
	} else if ( belle_sdp_media_description_get_attribute ( media_desc,"recvonly" ) ) {
		stream->dir=SalStreamRecvOnly;
	} else if ( belle_sdp_media_description_get_attribute ( media_desc,"inactive" ) ) {
		stream->dir=SalStreamInactive;
	} else {
		stream->dir=md->dir; /*takes default value if not present*/
	}

	stream->rtcp_mux = belle_sdp_media_description_get_attribute(media_desc, "rtcp-mux") != NULL;
	
	/* Get media payload types */
	sdp_parse_payload_types(media_desc, stream);

	/* Get media specific RTCP attribute */
	stream->rtcp_port = stream->rtp_port + 1;
	snprintf(stream->rtcp_addr, sizeof(stream->rtcp_addr), "%s", stream->rtp_addr);
	attribute=belle_sdp_media_description_get_attribute(media_desc,"rtcp");
	if (attribute && (value=belle_sdp_attribute_get_value(attribute))!=NULL){
		char tmp[256];
		int nb = sscanf(value, "%d IN IP4 %s", &stream->rtcp_port, tmp);
		if (nb == 1) {
			/* SDP rtcp attribute only contains the port */
		} else if (nb == 2) {
			strncpy(stream->rtcp_addr, tmp, sizeof(stream->rtcp_addr)-1);
		} else {
			ms_warning("sdp has a strange a=rtcp line (%s) nb=%i", value, nb);
		}
	}

	/* Read DTLS specific attributes : check is some are found in the stream description otherwise copy the session description one(which are at least set to Invalid) */
	if (((stream->proto == SalProtoUdpTlsRtpSavpf) || (stream->proto == SalProtoUdpTlsRtpSavp))) {
		attribute=belle_sdp_media_description_get_attribute(media_desc,"setup");
		if (attribute && (value=belle_sdp_attribute_get_value(attribute))!=NULL){
			if (strncmp(value, "actpass", 7) == 0) {
				stream->dtls_role = SalDtlsRoleUnset;
			} else if (strncmp(value, "active", 6) == 0) {
				stream->dtls_role = SalDtlsRoleIsClient;
			} else if (strncmp(value, "passive", 7) == 0) {
				stream->dtls_role = SalDtlsRoleIsServer;
			}
		}
		if (stream->dtls_role != SalDtlsRoleInvalid && (attribute=belle_sdp_media_description_get_attribute(media_desc,"fingerprint"))) {
			strncpy(stream->dtls_fingerprint, belle_sdp_attribute_get_value(attribute),sizeof(stream->dtls_fingerprint));
		}
	}

	/* Read crypto lines if any */
	if (sal_stream_description_has_srtp(stream)) {
		sdp_parse_media_crypto_parameters(media_desc, stream);
	}

	/* Read zrtp-hash attribute */
	if ((attribute=belle_sdp_media_description_get_attribute(media_desc,"zrtp-hash"))!=NULL) {
		if ((value=belle_sdp_attribute_get_value(attribute))!=NULL) {
			strncpy((char *)(stream->zrtphash), belle_sdp_attribute_get_value(attribute),sizeof(stream->zrtphash));
			stream->haveZrtpHash = 1;
		}
	}

	/* Get ICE candidate attributes if any */
	sdp_parse_media_ice_parameters(media_desc, stream);
    
	has_avpf_attributes = sdp_parse_rtcp_fb_parameters(media_desc, stream);

	/* Get RTCP-FB attributes if any */
	if (sal_stream_description_has_avpf(stream)) {
		enable_avpf_for_stream(stream);
	}else if (has_avpf_attributes ){
		enable_avpf_for_stream(stream);
		stream->implicit_rtcp_fb = TRUE;
	}

	/* Get RTCP-XR attributes if any */
	stream->rtcp_xr = md->rtcp_xr;	// Use session parameters if no stream parameters are defined
	sdp_parse_media_rtcp_xr_parameters(media_desc, &stream->rtcp_xr);

	/* Get the custom attributes */
	for (custom_attribute_it = belle_sdp_media_description_get_attributes(media_desc); custom_attribute_it != NULL; custom_attribute_it = custom_attribute_it->next) {
		belle_sdp_attribute_t *attr = (belle_sdp_attribute_t *)custom_attribute_it->data;
		stream->custom_sdp_attributes = sal_custom_sdp_attribute_append(stream->custom_sdp_attributes, belle_sdp_attribute_get_name(attr), belle_sdp_attribute_get_value(attr));
	}

	md->nb_streams++;
	return stream;
}


int sdp_to_media_description ( belle_sdp_session_description_t  *session_desc, SalMediaDescription *desc ) {
	belle_sdp_connection_t* cnx;
	belle_sip_list_t* media_desc_it;
	belle_sdp_media_description_t* media_desc;
	belle_sdp_session_name_t *sname;
	belle_sip_list_t *custom_attribute_it;
	const char* value;
	SalDtlsRole session_role=SalDtlsRoleInvalid;
	int i;
	
	desc->nb_streams = 0;
	desc->dir = SalStreamSendRecv;

	if ( ( cnx=belle_sdp_session_description_get_connection ( session_desc ) ) && belle_sdp_connection_get_address ( cnx ) ) {
		strncpy ( desc->addr,belle_sdp_connection_get_address ( cnx ),sizeof ( desc->addr ) -1  );
	}
	if ( (sname=belle_sdp_session_description_get_session_name(session_desc)) && belle_sdp_session_name_get_value(sname) ){
		strncpy(desc->name,belle_sdp_session_name_get_value(sname),sizeof(desc->name) - 1);
	}
	
	if ( belle_sdp_session_description_get_bandwidth ( session_desc,"AS" ) >0 ) {
		desc->bandwidth=belle_sdp_session_description_get_bandwidth ( session_desc,"AS" );
	}
	
	/*in some very rare case, session attribute may set stream dir*/
	if ( belle_sdp_session_description_get_attribute ( session_desc,"sendrecv" ) ) {
		desc->dir=SalStreamSendRecv;
	} else if ( belle_sdp_session_description_get_attribute ( session_desc,"sendonly" ) ) {
		desc->dir=SalStreamSendOnly;
	} else if ( belle_sdp_session_description_get_attribute ( session_desc,"recvonly" ) ) {
		desc->dir=SalStreamRecvOnly;
	} else if ( belle_sdp_session_description_get_attribute ( session_desc,"inactive" ) ) {
		desc->dir=SalStreamInactive;
	}

	/*DTLS attributes can be defined at session level.*/
	value=belle_sdp_session_description_get_attribute_value(session_desc,"setup");
	if (value){
		if (strncmp(value, "actpass", 7) == 0) {
			session_role = SalDtlsRoleUnset;
		} else if (strncmp(value, "active", 6) == 0) {
			session_role = SalDtlsRoleIsClient;
		} else if (strncmp(value, "passive", 7) == 0) {
			session_role = SalDtlsRoleIsServer;
		}
	}
	value=belle_sdp_session_description_get_attribute_value(session_desc,"fingerprint");
	/*copy dtls attributes to every streams, might be overwritten stream by stream*/
	for (i=0;i<SAL_MEDIA_DESCRIPTION_MAX_STREAMS;i++) {
		if (value)
			strncpy(desc->streams[i].dtls_fingerprint, value, sizeof(desc->streams[i].dtls_fingerprint));
		desc->streams[i].dtls_role=session_role; /*set or reset value*/
	}

	/* Get ICE remote ufrag and remote pwd, and ice_lite flag */
	value=belle_sdp_session_description_get_attribute_value(session_desc,"ice-ufrag");
	if (value) strncpy(desc->ice_ufrag, value, sizeof(desc->ice_ufrag) - 1);
	
	value=belle_sdp_session_description_get_attribute_value(session_desc,"ice-pwd");
	if (value) strncpy(desc->ice_pwd, value, sizeof(desc->ice_pwd)-1);
	
	value=belle_sdp_session_description_get_attribute_value(session_desc,"ice-lite");
	if (value) desc->ice_lite = TRUE;

	/* Get session RTCP-XR attributes if any */
	sdp_parse_session_rtcp_xr_parameters(session_desc, &desc->rtcp_xr);

	/* Get the custom attributes */
	for (custom_attribute_it = belle_sdp_session_description_get_attributes(session_desc); custom_attribute_it != NULL; custom_attribute_it = custom_attribute_it->next) {
		belle_sdp_attribute_t *attr = (belle_sdp_attribute_t *)custom_attribute_it->data;
		desc->custom_sdp_attributes = sal_custom_sdp_attribute_append(desc->custom_sdp_attributes, belle_sdp_attribute_get_name(attr), belle_sdp_attribute_get_value(attr));
	}

	for ( media_desc_it=belle_sdp_session_description_get_media_descriptions ( session_desc )
						; media_desc_it!=NULL
			; media_desc_it=media_desc_it->next ) {
		if (desc->nb_streams==SAL_MEDIA_DESCRIPTION_MAX_STREAMS){
			ms_warning("Cannot convert mline at position [%i] from SDP to SalMediaDescription",desc->nb_streams);
			break;
		}
		media_desc=BELLE_SDP_MEDIA_DESCRIPTION ( media_desc_it->data );
		sdp_to_stream_description(desc, media_desc);
	}
	return 0;
}
