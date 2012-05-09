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

belle_sdp_session_description_t * media_description_to_sdp(const SalMediaDescription *desc) {
	belle_sdp_session_description_t* session_desc=belle_sdp_session_description_new();
	bool_t inet6;
	belle_sdp_origin_t* origin;
	belle_sdp_mime_parameter_t* mime_param;
	belle_sdp_media_description_t* media_desc;
	int i,j;
	MSList* pt_it;
	PayloadType* pt;
	char buffer[1024];

	if (strchr(desc->addr,':')!=NULL){
			inet6=1;
	}else inet6=0;
	belle_sdp_session_description_set_version(session_desc,belle_sdp_version_create(0));

	origin = belle_sdp_origin_create(desc->username
										,desc->session_id
										,desc->session_ver
										,"IN"
										, inet6 ? "IP6" :"IP4"
										,desc->addr);

	belle_sdp_session_description_set_origin(session_desc,origin);

	belle_sdp_session_description_set_session_name(session_desc,belle_sdp_session_name_create("Talk"));

	if(!sal_media_description_has_dir (desc,SalStreamSendOnly) && !sal_media_description_has_dir (desc,SalStreamInactive)) {
		belle_sdp_session_description_set_connection(session_desc
													,belle_sdp_connection_create("IN",inet6 ? "IP6" :"IP4",desc->addr));

	} else 	{
		belle_sdp_session_description_set_connection(session_desc
													,belle_sdp_connection_create("IN"
																				,inet6 ? "IP6" :"IP4"
																				,inet6 ? "::0" :"0.0.0.0"));

	}

	belle_sdp_session_description_set_time_description(session_desc,belle_sdp_time_description_create(0,0));

	if (desc->bandwidth>0) {
		belle_sdp_session_description_set_bandwidth(session_desc,"AS",desc->bandwidth);
	}

	for (i=0; i<desc->nstreams;i++) {
		media_desc = belle_sdp_media_description_create(sal_stream_type_to_string(desc->streams[i].type)
                										,desc->streams[i].port
                										,1
                										,sal_media_proto_to_string(desc->streams[i].proto)
                										,NULL);
		for (pt_it=desc->streams[i].payloads;pt_it!=NULL;pt_it=pt_it->next) {
			pt=(PayloadType*)pt_it->data;
			mime_param= belle_sdp_mime_parameter_create(pt->mime_type
														, pt->type
														, pt->clock_rate
														,desc->streams[i].type==SalAudio?1:-1);
			belle_sdp_mime_parameter_set_parameters(mime_param,pt->recv_fmtp);
			if (desc->streams[i].ptime>0) {
				belle_sdp_mime_parameter_set_ptime(mime_param,desc->streams[i].ptime);
			}
			belle_sdp_media_description_append_values_from_mime_parameter(media_desc,mime_param);
			belle_sip_object_unref(mime_param);
		}
		if (desc->streams[i].bandwidth>0)
			belle_sdp_media_description_set_bandwidth(media_desc,"AS",desc->streams[i].bandwidth);

		if (desc->streams[i].proto == SalProtoRtpSavp) {
				/* add crypto lines */
				for(j=0; j<SAL_CRYPTO_ALGO_MAX; j++) {

					switch (desc->streams[i].crypto[j].algo) {
						case AES_128_SHA1_80:
							snprintf(buffer, sizeof(buffer), "%d %s inline:%s",
									desc->streams[i].crypto[j].tag, "AES_CM_128_HMAC_SHA1_80", desc->streams[i].crypto[j].master_key);
							belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create("crypto",buffer));
							break;
						case AES_128_SHA1_32:
							snprintf(buffer, sizeof(buffer), "%d %s inline:%s",
									desc->streams[i].crypto[j].tag, "AES_CM_128_HMAC_SHA1_32", desc->streams[i].crypto[j].master_key);
							belle_sdp_media_description_add_attribute(media_desc,belle_sdp_attribute_create("crypto",buffer));
							break;
						case AES_128_NO_AUTH:
							ms_warning("Unsupported crypto suite: AES_128_NO_AUTH");
							break;
						case NO_CIPHER_SHA1_80:
							ms_warning("Unsupported crypto suite: NO_CIPHER_SHA1_80");
							break;
						default:
							j = SAL_CRYPTO_ALGO_MAX;
							/* no break */
					}
				}

			}
		belle_sdp_session_description_add_media_description(session_desc,media_desc);

	}
	return session_desc;

}


int sdp_to_media_description(belle_sdp_session_description_t  *session_desc, SalMediaDescription *desc) {
	/*
	typedef struct SalMediaDescription{
		int refcount;
		char addr[64];
		char username[64];
		int nstreams;
		int bandwidth;
		unsigned int session_ver;
		unsigned int session_id;
		SalStreamDescription streams[SAL_MEDIA_DESCRIPTION_MAX_STREAMS];
	} SalMediaDescription;
	 */
	belle_sdp_connection_t* cnx;
	belle_sip_list_t* media_desc_it;
	belle_sdp_media_description_t* media_desc;
	const char *mtype,*proto;
	SalStreamDescription *stream;
	belle_sdp_media_t* media;
	belle_sip_list_t* mime_param_it=NULL;
	belle_sdp_mime_parameter_t* mime_param;
	PayloadType *pt;
	belle_sip_list_t* attribute_it;
	belle_sdp_attribute_t* attribute;
	int valid_count = 0;
	char tmp[256], tmp2[256];
	int nb=0;

	desc->nstreams=0;

	if ((cnx=belle_sdp_session_description_get_connection(session_desc)) && belle_sdp_connection_get_address(cnx)) {
		strncpy(desc->addr,belle_sdp_connection_get_address(cnx),sizeof(desc->addr));
	}
	if (belle_sdp_session_description_get_bandwidth(session_desc,"AS") >0) {
		desc->bandwidth=belle_sdp_session_description_get_bandwidth(session_desc,"AS");
	}
	for(media_desc_it=belle_sdp_session_description_get_media_descriptions(session_desc)
			;media_desc_it!=NULL
			;media_desc_it=media_desc_it->next) {
		media_desc=BELLE_SDP_MEDIA_DESCRIPTION(media_desc_it->data);
		stream=&desc->streams[desc->nstreams];
		media=belle_sdp_media_description_get_media(media_desc);

		memset(stream,0,sizeof(*stream));

		proto = belle_sdp_media_get_protocol(media);
		stream->proto=SalProtoUnknown;
		if (proto){
			if (strcasecmp(proto,"RTP/AVP")==0)
				stream->proto=SalProtoRtpAvp;
			else if (strcasecmp(proto,"RTP/SAVP")==0){
				stream->proto=SalProtoRtpSavp;
			}
		}
		if ((cnx=belle_sdp_media_description_get_connection(media_desc)) && belle_sdp_connection_get_address(cnx)) {
			strncpy(stream->addr,belle_sdp_connection_get_address(cnx),sizeof(stream->addr));
		}

		stream->port=belle_sdp_media_get_media_port(media);

		mtype = belle_sdp_media_get_media_type(media);
		if (strcasecmp("audio", mtype) == 0){
			stream->type=SalAudio;
		}else if (strcasecmp("video", mtype) == 0){
			stream->type=SalVideo;
		}else {
			stream->type=SalOther;
			strncpy(stream->typeother,mtype,sizeof(stream->typeother)-1);
		}

		if (belle_sdp_media_description_get_bandwidth(media_desc,"AS") >0) {
			stream->bandwidth=belle_sdp_media_description_get_bandwidth(media_desc,"AS");
		}


		if (belle_sdp_media_description_get_attribute(media_desc,"sendrecv")) {
			stream->dir=SalStreamSendRecv;
		} else if (belle_sdp_media_description_get_attribute(media_desc,"sendonly")) {
			stream->dir=SalStreamSendOnly;
		} else if (belle_sdp_media_description_get_attribute(media_desc,"recvonly")) {
			stream->dir=SalStreamRecvOnly;
		} else if (belle_sdp_media_description_get_attribute(media_desc,"inactive")) {
			stream->dir=SalStreamInactive;
		} else {
			stream->dir=SalStreamSendRecv;
		}

		/* for each payload type */
		for(mime_param_it=belle_sdp_media_description_build_mime_parameters(media_desc)
				;mime_param_it!=NULL
				;mime_param_it=mime_param_it->next) {
			mime_param=BELLE_SDP_MIME_PARAMETER(mime_param_it->data)

			pt=payload_type_new();
			payload_type_set_number(pt,belle_sdp_mime_parameter_get_media_format(mime_param));
			pt->clock_rate=belle_sdp_mime_parameter_get_rate(mime_param);
			pt->mime_type=ms_strdup(belle_sdp_mime_parameter_get_type(mime_param));
			pt->channels=belle_sdp_mime_parameter_get_channel_count(mime_param);
			payload_type_set_send_fmtp(pt,belle_sdp_mime_parameter_get_parameters(mime_param));
			stream->payloads=ms_list_append(stream->payloads,pt);
			stream->ptime=belle_sdp_mime_parameter_get_ptime(mime_param);
			ms_message("Found payload %s/%i fmtp=%s",pt->mime_type,pt->clock_rate,
					pt->send_fmtp ? pt->send_fmtp : "");
		}
		if (mime_param_it) belle_sip_list_free_with_data(mime_param_it,belle_sip_object_unref);
		/* read crypto lines if any */
		if (stream->proto == SalProtoRtpSavp) {

			memset(&stream->crypto, 0, sizeof(stream->crypto));
			for (attribute_it=belle_sdp_media_description_get_attributes(media_desc)
					;valid_count < SAL_CRYPTO_ALGO_MAX && attribute_it!=NULL;
					attribute_it=attribute_it->next){
				attribute=BELLE_SDP_ATTRIBUTE(attribute_it->data);

				if (keywordcmp("crypto",belle_sdp_attribute_get_name(attribute))==0 && belle_sdp_attribute_get_value(attribute)!=NULL){
					 nb = sscanf(belle_sdp_attribute_get_value(attribute), "%d %256s inline:%256s",
							 &stream->crypto[valid_count].tag,
							 tmp,
							 tmp2);
							ms_message("Found valid crypto line (tag:%d algo:'%s' key:'%s'",
									stream->crypto[valid_count].tag,
									tmp,
									tmp2);
					if (nb == 3) {
						if (keywordcmp("AES_CM_128_HMAC_SHA1_80",tmp) == 0)
							stream->crypto[valid_count].algo = AES_128_SHA1_80;
						else if (keywordcmp("AES_CM_128_HMAC_SHA1_32",tmp) == 0)
							stream->crypto[valid_count].algo = AES_128_SHA1_32;
						else {
							ms_warning("Failed to parse crypto-algo: '%s'", tmp);
							stream->crypto[valid_count].algo = 0;
						}
						if (stream->crypto[valid_count].algo) {
							strncpy(stream->crypto[valid_count].master_key, tmp2, 41);
							stream->crypto[valid_count].master_key[40] = '\0';
							ms_message("Found valid crypto line (tag:%d algo:'%s' key:'%s'",
								stream->crypto[valid_count].tag,
								tmp,
								stream->crypto[valid_count].master_key);
							valid_count++;
						}
					} else {
						ms_warning("sdp has a strange a= line (%s) nb=%i",belle_sdp_attribute_get_value(attribute),nb);
					}
				}
			}
			ms_message("Found: %d valid crypto lines", valid_count);
		}
		desc->nstreams++;
	}

	return 0;
}




