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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

static belle_sdp_media_description_t *stream_description_to_sdp ( const SalMediaDescription *md, const SalStreamDescription *stream ) {
	belle_sdp_mime_parameter_t* mime_param;
	belle_sdp_media_description_t* media_desc;
	int j;
	MSList* pt_it;
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
	
	media_desc = belle_sdp_media_description_create ( sal_stream_type_to_string ( stream->type )
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
					,stream->type==SalAudio?1:-1 );
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
		if (strchr(rtp_addr,':')!=NULL){
			inet6=TRUE;
		}else inet6=FALSE;
		belle_sdp_media_description_set_connection(media_desc,belle_sdp_connection_create("IN", inet6 ? "IP6" : "IP4", rtp_addr));
	}
	
	if ( stream->bandwidth>0 )
		belle_sdp_media_description_set_bandwidth ( media_desc,"AS",stream->bandwidth );

	if ( stream->proto == SalProtoRtpSavp ) {
		/* add crypto lines */
		for ( j=0; j<SAL_CRYPTO_ALGO_MAX; j++ ) {

			switch ( stream->crypto[j].algo ) {
				case AES_128_SHA1_80:
					snprintf ( buffer, sizeof ( buffer ), "%d %s inline:%s",
							   stream->crypto[j].tag, "AES_CM_128_HMAC_SHA1_80", stream->crypto[j].master_key );
					belle_sdp_media_description_add_attribute ( media_desc,belle_sdp_attribute_create ( "crypto",buffer ) );
					break;
				case AES_128_SHA1_32:
					snprintf ( buffer, sizeof ( buffer ), "%d %s inline:%s",
							   stream->crypto[j].tag, "AES_CM_128_HMAC_SHA1_32", stream->crypto[j].master_key );
					belle_sdp_media_description_add_attribute ( media_desc,belle_sdp_attribute_create ( "crypto",buffer ) );
					break;
				case AES_128_NO_AUTH:
					ms_warning ( "Unsupported crypto suite: AES_128_NO_AUTH" );
					break;
				case NO_CIPHER_SHA1_80:
					ms_warning ( "Unsupported crypto suite: NO_CIPHER_SHA1_80" );
					break;
				default:
					j = SAL_CRYPTO_ALGO_MAX;
					/* no break */
			}
		}
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
	if (stream->ice_completed == TRUE) {
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
	
	return media_desc;
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

	if ( (!sal_media_description_has_dir ( desc,SalStreamSendOnly ) && !sal_media_description_has_dir ( desc,SalStreamInactive )) 
		|| desc->ice_ufrag[0] != '\0' ) {
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
	
	if (desc->ice_completed == TRUE) belle_sdp_session_description_add_attribute(session_desc, belle_sdp_attribute_create("nortpproxy","yes"));
	if (desc->ice_pwd[0] != '\0') belle_sdp_session_description_add_attribute(session_desc, belle_sdp_attribute_create("ice-pwd",desc->ice_pwd));
	if (desc->ice_ufrag[0] != '\0') belle_sdp_session_description_add_attribute(session_desc, belle_sdp_attribute_create("ice-ufrag",desc->ice_ufrag));
	
	for ( i=0; i<desc->n_total_streams; i++ ) {
		belle_sdp_session_description_add_media_description ( session_desc,stream_description_to_sdp(desc,&desc->streams[i]));
	}
	return session_desc;
}


static void sdp_parse_payload_types(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	PayloadType *pt;
	belle_sip_list_t* mime_param_it=NULL;
	belle_sdp_mime_parameter_t* mime_param;
	belle_sip_list_t* mime_params=belle_sdp_media_description_build_mime_parameters ( media_desc );
	for ( mime_param_it=mime_params
						; mime_param_it!=NULL
			; mime_param_it=mime_param_it->next ) {
		mime_param=BELLE_SDP_MIME_PARAMETER ( mime_param_it->data )

		pt=payload_type_new();
		payload_type_set_number ( pt,belle_sdp_mime_parameter_get_media_format ( mime_param ) );
		pt->clock_rate=belle_sdp_mime_parameter_get_rate ( mime_param );
		pt->mime_type=ms_strdup ( belle_sdp_mime_parameter_get_type ( mime_param ) );
		pt->channels=belle_sdp_mime_parameter_get_channel_count ( mime_param );
		payload_type_set_send_fmtp ( pt,belle_sdp_mime_parameter_get_parameters ( mime_param ) );
		stream->payloads=ms_list_append ( stream->payloads,pt );
		stream->ptime=belle_sdp_mime_parameter_get_ptime ( mime_param );
		ms_message ( "Found payload %s/%i fmtp=%s",pt->mime_type,pt->clock_rate,
						pt->send_fmtp ? pt->send_fmtp : "" );
	}
	if ( mime_params ) belle_sip_list_free_with_data ( mime_params,belle_sip_object_unref );
}

static void sdp_parse_media_crypto_parameters(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	belle_sip_list_t *attribute_it;
	const belle_sdp_attribute_t *attribute;
	char tmp[256], tmp2[256];
	int valid_count = 0;
	int nb;

	memset ( &stream->crypto, 0, sizeof ( stream->crypto ) );
	for ( attribute_it=belle_sdp_media_description_get_attributes ( media_desc )
						; valid_count < SAL_CRYPTO_ALGO_MAX && attribute_it!=NULL;
			attribute_it=attribute_it->next ) {
		attribute=BELLE_SDP_ATTRIBUTE ( attribute_it->data );

		if ( keywordcmp ( "crypto",belle_sdp_attribute_get_name ( attribute ) ) ==0 && belle_sdp_attribute_get_value ( attribute ) !=NULL ) {
			nb = sscanf ( belle_sdp_attribute_get_value ( attribute ), "%d %256s inline:%256s",
							&stream->crypto[valid_count].tag,
							tmp,
							tmp2 );
			ms_message ( "Found valid crypto line (tag:%d algo:'%s' key:'%s'",
							stream->crypto[valid_count].tag,
							tmp,
							tmp2 );
			if ( nb == 3 ) {
				if ( keywordcmp ( "AES_CM_128_HMAC_SHA1_80",tmp ) == 0 )
					stream->crypto[valid_count].algo = AES_128_SHA1_80;
				else if ( keywordcmp ( "AES_CM_128_HMAC_SHA1_32",tmp ) == 0 )
					stream->crypto[valid_count].algo = AES_128_SHA1_32;
				else {
					ms_warning ( "Failed to parse crypto-algo: '%s'", tmp );
					stream->crypto[valid_count].algo = 0;
				}
				if ( stream->crypto[valid_count].algo ) {
					strncpy ( stream->crypto[valid_count].master_key, tmp2, 41 );
					stream->crypto[valid_count].master_key[40] = '\0';
					ms_message ( "Found valid crypto line (tag:%d algo:'%s' key:'%s'",
									stream->crypto[valid_count].tag,
									tmp,
									stream->crypto[valid_count].master_key );
					valid_count++;
				}
			} else {
				ms_warning ( "sdp has a strange a= line (%s) nb=%i",belle_sdp_attribute_get_value ( attribute ),nb );
			}
		}
	}
	ms_message ( "Found: %d valid crypto lines", valid_count );
}

static void sdp_parse_ice_media_parameters(belle_sdp_media_description_t *media_desc, SalStreamDescription *stream) {
	belle_sip_list_t *attribute_it;
	const belle_sdp_attribute_t *attribute;
	const char *att_name;
	const char *value;
	int nb_ice_candidates = 0;

	for (attribute_it = belle_sdp_media_description_get_attributes(media_desc); attribute_it != NULL; attribute_it=attribute_it->next) {
		attribute=(belle_sdp_attribute_t*)attribute_it->data;
		att_name = belle_sdp_attribute_get_name(attribute);
		value = belle_sdp_attribute_get_value(attribute);

		if ((keywordcmp("candidate", att_name) == 0) && (value != NULL)) {
			SalIceCandidate *candidate = &stream->ice_candidates[nb_ice_candidates];
			int nb = sscanf(value, "%s %u UDP %u %s %d typ %s raddr %s rport %d",
				candidate->foundation, &candidate->componentID, &candidate->priority, candidate->addr, &candidate->port,
				candidate->type, candidate->raddr, &candidate->rport);
			if ((nb == 6) || (nb == 8)) nb_ice_candidates++;
			else memset(candidate, 0, sizeof(*candidate));
		} else if ((keywordcmp("remote-candidates", att_name) == 0) && (value != NULL)) {
			SalIceRemoteCandidate candidate;
			unsigned int componentID;
			int offset;
			const char *ptr = value;
			const char *endptr = value + strlen(ptr);
			while (3 == sscanf(ptr, "%u %s %u%n", &componentID, candidate.addr, &candidate.port, &offset)) {
				if ((componentID > 0) && (componentID <= SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES)) {
					SalIceRemoteCandidate *remote_candidate = &stream->ice_remote_candidates[componentID - 1];
					strncpy(remote_candidate->addr, candidate.addr, sizeof(remote_candidate->addr));
					remote_candidate->port = candidate.port;
				}
				ptr += offset;
				if (ptr < endptr) {
					if (ptr[offset] == ' ') ptr += 1;
				} else break;
			}
		} else if ((keywordcmp("ice-ufrag", att_name) == 0) && (value != NULL)) {
			strncpy(stream->ice_ufrag, value, sizeof(stream->ice_ufrag));
		} else if ((keywordcmp("ice-pwd", att_name) == 0) && (value != NULL)) {
			strncpy(stream->ice_pwd, value, sizeof(stream->ice_pwd));
		} else if (keywordcmp("ice-mismatch", att_name) == 0) {
			stream->ice_mismatch = TRUE;
		}
	}
}

static SalStreamDescription * sdp_to_stream_description(SalMediaDescription *md, belle_sdp_media_description_t *media_desc) {
	SalStreamDescription *stream;
	belle_sdp_connection_t* cnx;
	belle_sdp_media_t* media;
	const belle_sdp_attribute_t* attribute;
	const char* value;
	const char *mtype,*proto;

	stream=&md->streams[md->n_total_streams];
	media=belle_sdp_media_description_get_media ( media_desc );

	memset ( stream,0,sizeof ( *stream ) );

	proto = belle_sdp_media_get_protocol ( media );
	stream->proto=SalProtoUnknown;
	if ( proto ) {
		if ( strcasecmp ( proto,"RTP/AVP" ) ==0 )
			stream->proto=SalProtoRtpAvp;
		else if ( strcasecmp ( proto,"RTP/SAVP" ) ==0 ) {
			stream->proto=SalProtoRtpSavp;
		}
	}
	if ( ( cnx=belle_sdp_media_description_get_connection ( media_desc ) ) && belle_sdp_connection_get_address ( cnx ) ) {
		strncpy ( stream->rtp_addr,belle_sdp_connection_get_address ( cnx ),sizeof ( stream->rtp_addr ) );
	}

	stream->rtp_port=belle_sdp_media_get_media_port ( media );

	if ( stream->rtp_port > 0 )
		md->n_active_streams++;

	mtype = belle_sdp_media_get_media_type ( media );
	if ( strcasecmp ( "audio", mtype ) == 0 ) {
		stream->type=SalAudio;
	} else if ( strcasecmp ( "video", mtype ) == 0 ) {
		stream->type=SalVideo;
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
			strncpy(stream->rtcp_addr, tmp, sizeof(stream->rtcp_addr));
		} else {
			ms_warning("sdp has a strange a=rtcp line (%s) nb=%i", value, nb);
		}
	}

	/* Read crypto lines if any */
	if ( stream->proto == SalProtoRtpSavp ) {
		sdp_parse_media_crypto_parameters(media_desc, stream);
	}

	/* Get ICE candidate attributes if any */
	sdp_parse_ice_media_parameters(media_desc, stream);

	md->n_total_streams++;
	return stream;
}


int sdp_to_media_description ( belle_sdp_session_description_t  *session_desc, SalMediaDescription *desc ) {
	belle_sdp_connection_t* cnx;
	belle_sip_list_t* media_desc_it;
	belle_sdp_media_description_t* media_desc;
	belle_sdp_session_name_t *sname;
	const char* value;
	
	desc->n_active_streams = 0;
	desc->n_total_streams = 0;
	desc->dir = SalStreamSendRecv;

	if ( ( cnx=belle_sdp_session_description_get_connection ( session_desc ) ) && belle_sdp_connection_get_address ( cnx ) ) {
		strncpy ( desc->addr,belle_sdp_connection_get_address ( cnx ),sizeof ( desc->addr ) );
	}
	if ( (sname=belle_sdp_session_description_get_session_name(session_desc)) && belle_sdp_session_name_get_value(sname) ){
		strncpy(desc->name,belle_sdp_session_name_get_value(sname),sizeof(desc->name));
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

	/* Get ICE remote ufrag and remote pwd, and ice_lite flag */
	value=belle_sdp_session_description_get_attribute_value(session_desc,"ice-ufrag");
	if (value) strncpy(desc->ice_ufrag, value, sizeof(desc->ice_ufrag));
	
	value=belle_sdp_session_description_get_attribute_value(session_desc,"ice-pwd");
	if (value) strncpy(desc->ice_pwd, value, sizeof(desc->ice_pwd));
	
	value=belle_sdp_session_description_get_attribute_value(session_desc,"ice-lite");
	if (value) desc->ice_lite = TRUE;
	
	for ( media_desc_it=belle_sdp_session_description_get_media_descriptions ( session_desc )
						; media_desc_it!=NULL
			; media_desc_it=media_desc_it->next ) {
		if (desc->n_total_streams==SAL_MEDIA_DESCRIPTION_MAX_STREAMS){
			ms_warning("Cannot convert mline at position [%i] from SDP to SalMediaDescription",desc->n_total_streams);
			break;
		}
		media_desc=BELLE_SDP_MEDIA_DESCRIPTION ( media_desc_it->data );
		sdp_to_stream_description(desc, media_desc);
	}
	return 0;
}
