/*
linphone
Copyright (C) 2012  Belledonne Communications SARL

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

#include "upnp.h"
#include "private.h"
#include "lpconfig.h"

#define UPNP_ADD_MAX_RETRY 4
#define UPNP_REMOVE_MAX_RETRY 4
#define UPNP_SECTION_NAME "uPnP"

/*
 * uPnP Definitions
 */

typedef struct _UpnpPortBinding {
	ms_mutex_t mutex;
	LinphoneUpnpState state;
	upnp_igd_ip_protocol protocol;
	char local_addr[LINPHONE_IPADDR_SIZE];
	int local_port;
	char external_addr[LINPHONE_IPADDR_SIZE];
	int external_port;
	int retry;
	int ref;
	bool_t to_remove;
	bool_t to_add;
} UpnpPortBinding;

typedef struct _UpnpStream {
	UpnpPortBinding *rtp;
	UpnpPortBinding *rtcp;
	LinphoneUpnpState state;
} UpnpStream;

struct _UpnpSession {
	LinphoneCall *call;
	UpnpStream *audio;
	UpnpStream *video;
	LinphoneUpnpState state;
};

struct _UpnpContext {
	LinphoneCore *lc;
	upnp_igd_context *upnp_igd_ctxt;
	UpnpPortBinding *sip_tcp;
	UpnpPortBinding *sip_tls;
	UpnpPortBinding *sip_udp;
	LinphoneUpnpState state;
	MSList *removing_configs;
	MSList *adding_configs;
	MSList *pending_bindings;

	ms_mutex_t mutex;
	ms_cond_t empty_cond;

};


bool_t linphone_core_upnp_hook(void *data);
void linphone_core_upnp_refresh(UpnpContext *ctx);

UpnpPortBinding *linphone_upnp_port_binding_new();
UpnpPortBinding *linphone_upnp_port_binding_copy(const UpnpPortBinding *port);
bool_t linphone_upnp_port_binding_equal(const UpnpPortBinding *port1, const UpnpPortBinding *port2);
UpnpPortBinding *linphone_upnp_port_binding_equivalent_in_list(MSList *list, const UpnpPortBinding *port);
UpnpPortBinding *linphone_upnp_port_binding_retain(UpnpPortBinding *port);
void linphone_upnp_port_binding_log(int level, const char *msg, const UpnpPortBinding *port);
void linphone_upnp_port_binding_release(UpnpPortBinding *port);

MSList *linphone_upnp_config_list_port_bindings(struct _LpConfig *lpc);
void linphone_upnp_config_add_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port);
void linphone_upnp_config_remove_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port);

int linphone_upnp_context_send_remove_port_binding(UpnpContext *lupnp, UpnpPortBinding *port);
int linphone_upnp_context_send_add_port_binding(UpnpContext *lupnp, UpnpPortBinding *port);


/**
 * uPnP Callbacks
 */

/* Convert uPnP IGD logs to ortp logs */
void linphone_upnp_igd_print(void *cookie, upnp_igd_print_level level, const char *fmt, va_list list) {
	int ortp_level = ORTP_DEBUG;
	switch(level) {
	case UPNP_IGD_MESSAGE:
		ortp_level = ORTP_MESSAGE;
		break;
	case UPNP_IGD_WARNING:
		ortp_level = ORTP_DEBUG; // Too verbose otherwise
		break;
	case UPNP_IGD_ERROR:
		ortp_level = ORTP_DEBUG; // Too verbose otherwise
		break;
	default:
		break;
	}
	ortp_logv(ortp_level, fmt, list);
}

void linphone_upnp_igd_callback(void *cookie, upnp_igd_event event, void *arg) {
	UpnpContext *lupnp = (UpnpContext *)cookie;
	upnp_igd_port_mapping *mapping = NULL;
	UpnpPortBinding *port_mapping = NULL;
	const char *ip_address = NULL;
	const char *connection_status = NULL;
	bool_t nat_enabled = FALSE;
	LinphoneUpnpState old_state;

	if(lupnp == NULL || lupnp->upnp_igd_ctxt == NULL) {
		ms_error("uPnP IGD: Invalid context in callback");
		return;
	}

	ms_mutex_lock(&lupnp->mutex);
	old_state = lupnp->state;

	switch(event) {
	case UPNP_IGD_EXTERNAL_IPADDRESS_CHANGED:
	case UPNP_IGD_NAT_ENABLED_CHANGED:
	case UPNP_IGD_CONNECTION_STATUS_CHANGED:
		ip_address = upnp_igd_get_external_ipaddress(lupnp->upnp_igd_ctxt);
		connection_status = upnp_igd_get_connection_status(lupnp->upnp_igd_ctxt);
		nat_enabled = upnp_igd_get_nat_enabled(lupnp->upnp_igd_ctxt);

		if(ip_address == NULL || connection_status == NULL) {
			ms_message("uPnP IGD: Pending");
			lupnp->state = LinphoneUpnpStatePending;
		} else if(strcasecmp(connection_status, "Connected")  || !nat_enabled) {
			ms_message("uPnP IGD: Not Available");
			lupnp->state = LinphoneUpnpStateNotAvailable;
		} else {
			ms_message("uPnP IGD: Connected");
			lupnp->state = LinphoneUpnpStateOk;
			if(old_state != LinphoneUpnpStateOk) {
				linphone_core_upnp_refresh(lupnp);
			}
		}

		break;

	case UPNP_IGD_PORT_MAPPING_ADD_SUCCESS:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->external_port = mapping->remote_port;
		port_mapping->state = LinphoneUpnpStateOk;
		linphone_upnp_port_binding_log(ORTP_MESSAGE, "Added port binding", port_mapping);
		linphone_upnp_config_add_port_binding(lupnp, port_mapping);

		break;

	case UPNP_IGD_PORT_MAPPING_ADD_FAILURE:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->external_port = -1; //Force random external port
		if(linphone_upnp_context_send_add_port_binding(lupnp, port_mapping) != 0) {
			linphone_upnp_port_binding_log(ORTP_ERROR, "Can't add port binding", port_mapping);
		}

		break;

	case UPNP_IGD_PORT_MAPPING_REMOVE_SUCCESS:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->state = LinphoneUpnpStateIdle;
		linphone_upnp_port_binding_log(ORTP_MESSAGE, "Removed port binding", port_mapping);
		linphone_upnp_config_remove_port_binding(lupnp, port_mapping);

		break;

	case UPNP_IGD_PORT_MAPPING_REMOVE_FAILURE:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		if(linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping) != 0) {
			linphone_upnp_port_binding_log(ORTP_ERROR, "Can't remove port binding", port_mapping);
			linphone_upnp_config_remove_port_binding(lupnp, port_mapping);
		}

		break;

	default:
		break;
	}

	if(port_mapping != NULL) {
		/*
		 * Execute delayed actions
		 */
		if(port_mapping->to_remove) {
			if(port_mapping->state == LinphoneUpnpStateOk) {
				port_mapping->to_remove = FALSE;
				linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping);
			} else if(port_mapping->state == LinphoneUpnpStateKo) {
				port_mapping->to_remove = FALSE;
			}
		}
		if(port_mapping->to_add) {
			if(port_mapping->state == LinphoneUpnpStateIdle || port_mapping->state == LinphoneUpnpStateKo) {
				port_mapping->to_add = FALSE;
				linphone_upnp_context_send_add_port_binding(lupnp, port_mapping);
			}
		}

		lupnp->pending_bindings = ms_list_remove(lupnp->pending_bindings, port_mapping);
		linphone_upnp_port_binding_release(port_mapping);
	}

	/*
	 * If there is no pending binding emit a signal
	 */
	if(lupnp->pending_bindings == NULL) {
		pthread_cond_signal(&lupnp->empty_cond);
	}
	ms_mutex_unlock(&lupnp->mutex);
}


/**
 * uPnP Context
 */

UpnpContext* linphone_upnp_context_new(LinphoneCore *lc) {
	LCSipTransports transport;
	UpnpContext *lupnp = (UpnpContext *)ms_new0(UpnpContext,1);
	const char *ip_address;

	ms_mutex_init(&lupnp->mutex, NULL);
	ms_cond_init(&lupnp->empty_cond, NULL);

	lupnp->lc = lc;
	lupnp->pending_bindings = NULL;
	lupnp->adding_configs = NULL;
	lupnp->removing_configs = NULL;
	lupnp->state = LinphoneUpnpStateIdle;
	ms_message("uPnP IGD: New %p for core %p", lupnp, lc);

	linphone_core_get_sip_transports(lc, &transport);
	if(transport.udp_port != 0) {
		lupnp->sip_udp = linphone_upnp_port_binding_new();
		lupnp->sip_udp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
		lupnp->sip_udp->local_port = transport.udp_port;
		lupnp->sip_udp->external_port = transport.udp_port;
	} else {
		lupnp->sip_udp = NULL;
	}
	if(transport.tcp_port != 0) {
		lupnp->sip_tcp = linphone_upnp_port_binding_new();
		lupnp->sip_tcp->protocol = UPNP_IGD_IP_PROTOCOL_TCP;
		lupnp->sip_tcp->local_port = transport.tcp_port;
		lupnp->sip_tcp->external_port = transport.tcp_port;
	} else {
		lupnp->sip_tcp = NULL;
	}
	if(transport.tls_port != 0) {
		lupnp->sip_tls = linphone_upnp_port_binding_new();
		lupnp->sip_tls->protocol = UPNP_IGD_IP_PROTOCOL_TCP;
		lupnp->sip_tls->local_port = transport.tls_port;
		lupnp->sip_tls->external_port = transport.tls_port;
	} else {
		lupnp->sip_tls = NULL;
	}

	linphone_core_add_iterate_hook(lc, linphone_core_upnp_hook, lupnp);

	lupnp->upnp_igd_ctxt = NULL;
	lupnp->upnp_igd_ctxt = upnp_igd_create(linphone_upnp_igd_callback, linphone_upnp_igd_print, lupnp);
	if(lupnp->upnp_igd_ctxt == NULL) {
		lupnp->state = LinphoneUpnpStateKo;
		ms_error("Can't create uPnP IGD context");
		return NULL;
	}

	ip_address = upnp_igd_get_local_ipaddress(lupnp->upnp_igd_ctxt);
	if(lupnp->sip_udp != NULL) {
		strncpy(lupnp->sip_udp->local_addr, ip_address, sizeof(lupnp->sip_udp->local_addr));
	}
	if(lupnp->sip_tcp != NULL) {
		strncpy(lupnp->sip_tcp->local_addr, ip_address, sizeof(lupnp->sip_tcp->local_addr));
	}
	if(lupnp->sip_tls != NULL) {
		strncpy(lupnp->sip_tls->local_addr, ip_address, sizeof(lupnp->sip_tls->local_addr));
	}

	lupnp->state = LinphoneUpnpStatePending;
	upnp_igd_start(lupnp->upnp_igd_ctxt);

	return lupnp;
}

void linphone_upnp_context_destroy(UpnpContext *lupnp) {
	/*
	 * Not need, all hooks are removed before
	 * linphone_core_remove_iterate_hook(lc, linphone_core_upnp_hook, lc);
     */

	/* Send port binding removes */
	if(lupnp->sip_udp != NULL) {
		linphone_upnp_context_send_remove_port_binding(lupnp, lupnp->sip_udp);
		lupnp->sip_udp = NULL;
	}
	if(lupnp->sip_tcp != NULL) {
		linphone_upnp_context_send_remove_port_binding(lupnp, lupnp->sip_tcp);
		lupnp->sip_tcp = NULL;
	}
	if(lupnp->sip_tls != NULL) {
		linphone_upnp_context_send_remove_port_binding(lupnp, lupnp->sip_tls);
		lupnp->sip_tcp = NULL;
	}

	/* Wait all pending bindings are done */
	ms_message("uPnP IGD: Wait all pending port bindings ...");
	ms_mutex_lock(&lupnp->mutex);
	ms_cond_wait(&lupnp->empty_cond, &lupnp->mutex);
	ms_mutex_unlock(&lupnp->mutex);

	if(lupnp->upnp_igd_ctxt != NULL) {
		upnp_igd_destroy(lupnp->upnp_igd_ctxt);
	}

	/* Run one time the hook for configuration update */
	linphone_core_upnp_hook(lupnp);

	/* Release port bindings */
	if(lupnp->sip_udp != NULL) {
		linphone_upnp_port_binding_release(lupnp->sip_udp);
		lupnp->sip_udp = NULL;
	}
	if(lupnp->sip_tcp != NULL) {
		linphone_upnp_port_binding_release(lupnp->sip_tcp);
		lupnp->sip_tcp = NULL;
	}
	if(lupnp->sip_tls != NULL) {
		linphone_upnp_port_binding_release(lupnp->sip_tls);
		lupnp->sip_tcp = NULL;
	}

	/* Release lists */
	ms_list_for_each(lupnp->adding_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->adding_configs = ms_list_free(lupnp->adding_configs);
	ms_list_for_each(lupnp->removing_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->removing_configs = ms_list_free(lupnp->removing_configs);
	ms_list_for_each(lupnp->pending_bindings,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->pending_bindings = ms_list_free(lupnp->pending_bindings);

	ms_mutex_destroy(&lupnp->mutex);
	ms_cond_destroy(&lupnp->empty_cond);

	ms_message("uPnP IGD: destroy %p", lupnp);
	ms_free(lupnp);
}

LinphoneUpnpState linphone_upnp_context_get_state(UpnpContext *ctx) {
	return ctx->state;
}

const char* linphone_upnp_context_get_external_ipaddress(UpnpContext *ctx) {
	return upnp_igd_get_external_ipaddress(ctx->upnp_igd_ctxt);
}

int linphone_upnp_context_send_add_port_binding(UpnpContext *lupnp, UpnpPortBinding *port) {
	upnp_igd_port_mapping mapping;
	char description[128];
	int ret;

	// Compute port binding state
	if(port->state != LinphoneUpnpStateAdding) {
		port->to_remove = FALSE;
		switch(port->state) {
			case LinphoneUpnpStateKo:
			case LinphoneUpnpStateIdle: {
				port->retry = 0;
				port->state = LinphoneUpnpStateAdding;
			}
			break;
			case LinphoneUpnpStateRemoving: {
				port->to_add = TRUE;
				return 0;
			}
			break;
			default:
				return 0;
		}
	}

	if(port->retry >= UPNP_ADD_MAX_RETRY) {
		ret = -1;
	} else {
		mapping.cookie = linphone_upnp_port_binding_retain(port);
		lupnp->pending_bindings = ms_list_append(lupnp->pending_bindings, mapping.cookie);

		mapping.local_port = port->local_port;
		mapping.local_host = port->local_addr;
		if(port->external_port == -1)
			mapping.remote_port = rand()%(0xffff - 1024) + 1024;
		else
			mapping.remote_port = port->external_port;
		mapping.remote_host = "";
		snprintf(description, 128, "%s %s at %s:%d",
				PACKAGE_NAME,
				(port->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP": "UDP",
				port->local_addr, port->local_port);
		mapping.description = description;
		mapping.protocol = port->protocol;

		port->retry++;
		linphone_upnp_port_binding_log(ORTP_MESSAGE, "Try to add port binding", port);
		ret = upnp_igd_add_port_mapping(lupnp->upnp_igd_ctxt, &mapping);
	}
	if(ret != 0) {
		port->state = LinphoneUpnpStateKo;
	}
	return ret;
}

int linphone_upnp_context_send_remove_port_binding(UpnpContext *lupnp, UpnpPortBinding *port) {
	upnp_igd_port_mapping mapping;
	int ret;

	// Compute port binding state
	if(port->state != LinphoneUpnpStateRemoving) {
		port->to_add = FALSE;
		switch(port->state) {
			case LinphoneUpnpStateOk: {
				port->retry = 0;
				port->state = LinphoneUpnpStateRemoving;
			}
			break;
			case LinphoneUpnpStateAdding: {
				port->to_remove = TRUE;
				return 0;
			}
			break;
			default:
				return 0;
		}
	}

	if(port->retry >= UPNP_REMOVE_MAX_RETRY) {
		ret = -1;
	} else {
		mapping.cookie = linphone_upnp_port_binding_retain(port);
		lupnp->pending_bindings = ms_list_append(lupnp->pending_bindings, mapping.cookie);

		mapping.remote_port = port->external_port;
		mapping.remote_host = "";
		mapping.protocol = port->protocol;
		port->retry++;
		linphone_upnp_port_binding_log(ORTP_MESSAGE, "Try to remove port binding", port);
		ret = upnp_igd_delete_port_mapping(lupnp->upnp_igd_ctxt, &mapping);
	}
	if(ret != 0) {
		port->state = LinphoneUpnpStateKo;
	}
	return ret;
}

/*
 * uPnP Core interfaces
 */

int linphone_core_update_upnp_audio_video(LinphoneCall *call, bool_t audio, bool_t video) {
	LinphoneCore *lc = call->core;
	UpnpContext *lupnp = lc->upnp;
	int ret = -1;
	const char *local_addr, *external_addr;

	if(lupnp == NULL) {
		return ret;
	}

	ms_mutex_lock(&lupnp->mutex);
	// Don't handle when the call
	if(lupnp->state == LinphoneUpnpStateOk && call->upnp_session != NULL) {
		ret = 0;
		local_addr = upnp_igd_get_local_ipaddress(lupnp->upnp_igd_ctxt);
		external_addr = upnp_igd_get_external_ipaddress(lupnp->upnp_igd_ctxt);

		/*
		 * Audio part
		 */
		strncpy(call->upnp_session->audio->rtp->local_addr, local_addr, LINPHONE_IPADDR_SIZE);
		strncpy(call->upnp_session->audio->rtp->external_addr, external_addr, LINPHONE_IPADDR_SIZE);
		call->upnp_session->audio->rtp->local_port = call->audio_port;
		if(call->upnp_session->audio->rtp->external_port == -1) {
			call->upnp_session->audio->rtp->external_port = call->audio_port;
		}
		strncpy(call->upnp_session->audio->rtcp->local_addr, local_addr, LINPHONE_IPADDR_SIZE);
		strncpy(call->upnp_session->audio->rtcp->external_addr, external_addr, LINPHONE_IPADDR_SIZE);
		call->upnp_session->audio->rtcp->local_port = call->audio_port+1;
		if(call->upnp_session->audio->rtcp->external_port == -1) {
			call->upnp_session->audio->rtcp->external_port = call->audio_port+1;
		}
		if(audio) {
			// Add audio port binding
			linphone_upnp_context_send_add_port_binding(lupnp, call->upnp_session->audio->rtp);
			linphone_upnp_context_send_add_port_binding(lupnp, call->upnp_session->audio->rtcp);
		} else {
			// Remove audio port binding
			linphone_upnp_context_send_remove_port_binding(lupnp, call->upnp_session->audio->rtp);
			linphone_upnp_context_send_remove_port_binding(lupnp, call->upnp_session->audio->rtcp);
		}

		/*
		 * Video part
		 */
		strncpy(call->upnp_session->video->rtp->local_addr, local_addr, LINPHONE_IPADDR_SIZE);
		strncpy(call->upnp_session->video->rtp->external_addr, external_addr, LINPHONE_IPADDR_SIZE);
		call->upnp_session->video->rtp->local_port = call->video_port;
		if(call->upnp_session->video->rtp->external_port == -1) {
			call->upnp_session->video->rtp->external_port = call->video_port;
		}
		strncpy(call->upnp_session->video->rtcp->local_addr, local_addr, LINPHONE_IPADDR_SIZE);
		strncpy(call->upnp_session->video->rtcp->external_addr, external_addr, LINPHONE_IPADDR_SIZE);
		call->upnp_session->video->rtcp->local_port = call->video_port+1;
		if(call->upnp_session->video->rtcp->external_port == -1) {
			call->upnp_session->video->rtcp->external_port = call->video_port+1;
		}
		if(video) {
			// Add video port binding
			linphone_upnp_context_send_add_port_binding(lupnp, call->upnp_session->video->rtp);
			linphone_upnp_context_send_add_port_binding(lupnp, call->upnp_session->video->rtcp);
		} else {
			// Remove video port binding
			linphone_upnp_context_send_remove_port_binding(lupnp, call->upnp_session->video->rtp);
			linphone_upnp_context_send_remove_port_binding(lupnp, call->upnp_session->video->rtcp);
		}
	}

	ms_mutex_unlock(&lupnp->mutex);

	/*
	 * Update uPnP call state
	 */
	linphone_upnp_call_process(call);

	return ret;
}


int linphone_core_update_upnp_from_remote_media_description(LinphoneCall *call, const SalMediaDescription *md) {
	bool_t audio = FALSE;
	bool_t video = FALSE;
	int i;
	const SalStreamDescription *stream;

	for (i = 0; i < md->nstreams; i++) {
		stream = &md->streams[i];
		if(stream->type == SalAudio) {
			audio = TRUE;
		} else if(stream->type == SalVideo) {
			video = TRUE;
		}
	}

	return linphone_core_update_upnp_audio_video(call, audio, video);
}

int linphone_core_update_upnp(LinphoneCore *lc, LinphoneCall *call) {
	return linphone_core_update_upnp_audio_video(call, call->audiostream!=NULL, call->videostream!=NULL);
}

int linphone_upnp_call_process(LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	UpnpContext *lupnp = lc->upnp;
	int ret = -1;
	LinphoneUpnpState oldState;

	if(lupnp == NULL) {
		return ret;
	}

	ms_mutex_lock(&lupnp->mutex);

	// Don't handle when the call
	if(lupnp->state == LinphoneUpnpStateOk && call->upnp_session != NULL) {
		ret = 0;

		/*
		 * Update Audio state
		 */
		if((call->upnp_session->audio->rtp->state == LinphoneUpnpStateOk || call->upnp_session->audio->rtp->state == LinphoneUpnpStateIdle) &&
				(call->upnp_session->audio->rtcp->state == LinphoneUpnpStateOk || call->upnp_session->audio->rtcp->state == LinphoneUpnpStateIdle)) {
			call->upnp_session->audio->state = LinphoneUpnpStateOk;
		} else if(call->upnp_session->audio->rtp->state == LinphoneUpnpStateAdding ||
				call->upnp_session->audio->rtp->state == LinphoneUpnpStateRemoving ||
				call->upnp_session->audio->rtcp->state == LinphoneUpnpStateAdding ||
				call->upnp_session->audio->rtcp->state == LinphoneUpnpStateRemoving) {
			call->upnp_session->audio->state = LinphoneUpnpStatePending;
		} else if(call->upnp_session->audio->rtcp->state == LinphoneUpnpStateKo ||
				call->upnp_session->audio->rtp->state == LinphoneUpnpStateKo) {
			call->upnp_session->audio->state = LinphoneUpnpStateKo;
		} else {
			call->upnp_session->audio->state = LinphoneUpnpStateIdle;
		}

		/*
		 * Update Video state
		 */
		if((call->upnp_session->video->rtp->state == LinphoneUpnpStateOk || call->upnp_session->video->rtp->state == LinphoneUpnpStateIdle) &&
				(call->upnp_session->video->rtcp->state == LinphoneUpnpStateOk || call->upnp_session->video->rtcp->state == LinphoneUpnpStateIdle)) {
			call->upnp_session->video->state = LinphoneUpnpStateOk;
		} else if(call->upnp_session->video->rtp->state == LinphoneUpnpStateAdding ||
				call->upnp_session->video->rtp->state == LinphoneUpnpStateRemoving ||
				call->upnp_session->video->rtcp->state == LinphoneUpnpStateAdding ||
				call->upnp_session->video->rtcp->state == LinphoneUpnpStateRemoving) {
			call->upnp_session->video->state = LinphoneUpnpStatePending;
		} else if(call->upnp_session->video->rtcp->state == LinphoneUpnpStateKo ||
				call->upnp_session->video->rtp->state == LinphoneUpnpStateKo) {
			call->upnp_session->video->state = LinphoneUpnpStateKo;
		} else {
			call->upnp_session->video->state = LinphoneUpnpStateIdle;
		}

		/*
		 * Update session state
		 */
		oldState = call->upnp_session->state;
		if(call->upnp_session->audio->state == LinphoneUpnpStateOk &&
			call->upnp_session->video->state == LinphoneUpnpStateOk) {
			call->upnp_session->state = LinphoneUpnpStateOk;
		} else if(call->upnp_session->audio->state == LinphoneUpnpStatePending ||
				call->upnp_session->video->state == LinphoneUpnpStatePending) {
			call->upnp_session->state = LinphoneUpnpStatePending;
		} else if(call->upnp_session->audio->state == LinphoneUpnpStateKo ||
				call->upnp_session->video->state == LinphoneUpnpStateKo) {
			call->upnp_session->state = LinphoneUpnpStateKo;
		} else {
			call->upnp_session->state = LinphoneUpnpStateIdle;
		}

		/* When change is done proceed update */
		if(oldState != LinphoneUpnpStateOk && oldState != LinphoneUpnpStateKo &&
				(call->upnp_session->state == LinphoneUpnpStateOk || call->upnp_session->state == LinphoneUpnpStateKo)) {
			if(call->upnp_session->state == LinphoneUpnpStateOk)
				ms_message("uPnP IGD: uPnP for Call %p is ok", call);
			else
				ms_message("uPnP IGD: uPnP for Call %p is ko", call);

			switch (call->state) {
				case LinphoneCallUpdating:
					linphone_core_start_update_call(lc, call);
					break;
				case LinphoneCallUpdatedByRemote:
					linphone_core_start_accept_call_update(lc, call);
					break;
				case LinphoneCallOutgoingInit:
					linphone_core_proceed_with_invite_if_ready(lc, call, NULL);
					break;
				case LinphoneCallIdle:
					linphone_core_notify_incoming_call(lc, call);
					break;
				default:
					break;
			}
		}
	}

	ms_mutex_unlock(&lupnp->mutex);
	return ret;
}

void linphone_core_upnp_refresh(UpnpContext *lupnp) {
	MSList *global_list = NULL;
	MSList *list = NULL;
	MSList *item;
	LinphoneCall *call;
	UpnpPortBinding *port_mapping, *port_mapping2;

	ms_message("uPnP IGD: Refresh mappings");

	/* Remove context port bindings */
	if(lupnp->sip_udp != NULL) {
		global_list = ms_list_append(global_list, lupnp->sip_udp);
	}
	if(lupnp->sip_tcp != NULL) {
		global_list = ms_list_append(global_list, lupnp->sip_tcp);
	}
	if(lupnp->sip_tls != NULL) {
		global_list = ms_list_append(global_list, lupnp->sip_tls);
	}

	/* Remove call port bindings */
	list = lupnp->lc->calls;
	while(list != NULL) {
		call = (LinphoneCall *)list->data;
		if(call->upnp_session != NULL) {
			global_list = ms_list_append(global_list, call->upnp_session->audio->rtp);
			global_list = ms_list_append(global_list, call->upnp_session->audio->rtcp);
			global_list = ms_list_append(global_list, call->upnp_session->video->rtp);
			global_list = ms_list_append(global_list, call->upnp_session->video->rtcp);
		}
		list = list->next;
	}

	// Remove port binding configurations
	list = linphone_upnp_config_list_port_bindings(lupnp->lc->config);
	for(item = list;item != NULL; item = item->next) {
			port_mapping = (UpnpPortBinding *)item->data;
			port_mapping2 = linphone_upnp_port_binding_equivalent_in_list(global_list, port_mapping);
			if(port_mapping2 == NULL) {
				linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping);
			} else if(port_mapping2->state == LinphoneUpnpStateIdle){
				/* Force to remove */
				port_mapping2->state = LinphoneUpnpStateOk;
			}
	}
	ms_list_for_each(list, (void (*)(void*))linphone_upnp_port_binding_release);
	list = ms_list_free(list);


	// (Re)Add removed port bindings
	list = global_list;
	while(list != NULL) {
		port_mapping = (UpnpPortBinding *)list->data;
		linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping);
		linphone_upnp_context_send_add_port_binding(lupnp, port_mapping);
		list = list->next;
	}
	global_list = ms_list_free(global_list);
}

bool_t linphone_core_upnp_hook(void *data) {
	char key[64];
	MSList *item;
	UpnpPortBinding *port_mapping;
	UpnpContext *lupnp = (UpnpContext *)data;
	ms_mutex_lock(&lupnp->mutex);

	/* Add configs */
	for(item = lupnp->adding_configs;item!=NULL;item=item->next) {
		port_mapping = (UpnpPortBinding *)item->data;
		snprintf(key, sizeof(key), "%s-%d-%d",
					(port_mapping->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
							port_mapping->external_port,
							port_mapping->local_port);
		lp_config_set_string(lupnp->lc->config, UPNP_SECTION_NAME, key, "uPnP");
		linphone_upnp_port_binding_log(ORTP_DEBUG, "Configuration: Added port binding", port_mapping);
	}
	ms_list_for_each(lupnp->adding_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->adding_configs = ms_list_free(lupnp->adding_configs);

	/* Remove configs */
	for(item = lupnp->removing_configs;item!=NULL;item=item->next) {
		port_mapping = (UpnpPortBinding *)item->data;
		snprintf(key, sizeof(key), "%s-%d-%d",
					(port_mapping->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
							port_mapping->external_port,
							port_mapping->local_port);
		lp_config_set_string(lupnp->lc->config, UPNP_SECTION_NAME, key, NULL);
		linphone_upnp_port_binding_log(ORTP_DEBUG, "Configuration: Removed port binding", port_mapping);
	}
	ms_list_for_each(lupnp->removing_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->removing_configs = ms_list_free(lupnp->removing_configs);

	ms_mutex_unlock(&lupnp->mutex);
	return TRUE;
}

int linphone_core_update_local_media_description_from_upnp(SalMediaDescription *desc, UpnpSession *session) {
	int i;
	SalStreamDescription *stream;
	UpnpStream *upnpStream;

	for (i = 0; i < desc->nstreams; i++) {
		stream = &desc->streams[i];
		upnpStream = NULL;
		if(stream->type == SalAudio) {
			upnpStream = session->audio;
		} else if(stream->type == SalVideo) {
			upnpStream = session->video;
		}
		if(upnpStream != NULL) {
			if(upnpStream->rtp->state == LinphoneUpnpStateOk) {
				strncpy(stream->rtp_addr, upnpStream->rtp->external_addr, LINPHONE_IPADDR_SIZE);
				stream->rtp_port = upnpStream->rtp->external_port;
			}
			if(upnpStream->rtcp->state == LinphoneUpnpStateOk) {
				strncpy(stream->rtcp_addr, upnpStream->rtcp->external_addr, LINPHONE_IPADDR_SIZE);
				stream->rtcp_port = upnpStream->rtcp->external_port;
			}
		}
	}
	return 0;
}


/*
 * uPnP Port Binding
 */

UpnpPortBinding *linphone_upnp_port_binding_new() {
	UpnpPortBinding *port = NULL;
	port = ms_new0(UpnpPortBinding,1);
	ms_mutex_init(&port->mutex, NULL);
	port->state = LinphoneUpnpStateIdle;
	port->local_addr[0] = '\0';
	port->local_port = -1;
	port->external_addr[0] = '\0';
	port->external_port = -1;
	port->to_remove = FALSE;
	port->to_add = FALSE;
	port->ref = 1;
	return port;
}

UpnpPortBinding *linphone_upnp_port_binding_copy(const UpnpPortBinding *port) {
	UpnpPortBinding *new_port = NULL;
	new_port = ms_new0(UpnpPortBinding,1);
	memcpy(new_port, port, sizeof(UpnpPortBinding));
	ms_mutex_init(&new_port->mutex, NULL);
	new_port->ref = 1;
	return new_port;
}

void linphone_upnp_port_binding_log(int level, const char *msg, const UpnpPortBinding *port) {
	if(strlen(port->local_addr)) {
		ortp_log(level, "uPnP IGD: %s %s|%d->%s:%d", msg,
							(port->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
									port->external_port,
									port->local_addr,
									port->local_port);
	} else {
		ortp_log(level, "uPnP IGD: %s %s|%d->%d", msg,
							(port->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
									port->external_port,
									port->local_port);
	}
}

bool_t linphone_upnp_port_binding_equal(const UpnpPortBinding *port1, const UpnpPortBinding *port2) {
	return port1->protocol == port2->protocol &&
			port1->local_port == port2->local_port &&
			port1->external_port == port2->external_port;
}

UpnpPortBinding *linphone_upnp_port_binding_equivalent_in_list(MSList *list, const UpnpPortBinding *port) {
	UpnpPortBinding *port_mapping;
	while(list != NULL) {
		port_mapping = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(port, port_mapping)) {
			return port_mapping;
		}
		list = list->next;
	}

	return NULL;
}

UpnpPortBinding *linphone_upnp_port_binding_retain(UpnpPortBinding *port) {
	ms_mutex_lock(&port->mutex);
	port->ref++;
	ms_mutex_unlock(&port->mutex);
	return port;
}

void linphone_upnp_port_binding_release(UpnpPortBinding *port) {
	ms_mutex_lock(&port->mutex);
	if(--port->ref == 0) {
		ms_mutex_unlock(&port->mutex);
		ms_mutex_destroy(&port->mutex);
		ms_free(port);
		return;
	}
	ms_mutex_unlock(&port->mutex);
}


/*
 * uPnP Stream
 */

UpnpStream* linphone_upnp_stream_new() {
	UpnpStream *stream = ms_new0(UpnpStream,1);
	stream->state = LinphoneUpnpStateIdle;
	stream->rtp = linphone_upnp_port_binding_new();
	stream->rtp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	stream->rtcp = linphone_upnp_port_binding_new();
	stream->rtcp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	return stream;
}

void linphone_upnp_stream_destroy(UpnpStream* stream) {
	linphone_upnp_port_binding_release(stream->rtp);
	stream->rtp = NULL;
	linphone_upnp_port_binding_release(stream->rtcp);
	stream->rtcp = NULL;
	ms_free(stream);
}


/*
 * uPnP Session
 */

UpnpSession* linphone_upnp_session_new(LinphoneCall* call) {
	UpnpSession *session = ms_new0(UpnpSession,1);
	session->call = call;
	session->state = LinphoneUpnpStateIdle;
	session->audio = linphone_upnp_stream_new();
	session->video = linphone_upnp_stream_new();
	return session;
}

void linphone_upnp_session_destroy(UpnpSession *session) {
	LinphoneCore *lc = session->call->core;

	if(lc->upnp != NULL) {
		/* Remove bindings */
		linphone_upnp_context_send_remove_port_binding(lc->upnp, session->audio->rtp);
		linphone_upnp_context_send_remove_port_binding(lc->upnp, session->audio->rtcp);
		linphone_upnp_context_send_remove_port_binding(lc->upnp, session->video->rtp);
		linphone_upnp_context_send_remove_port_binding(lc->upnp, session->video->rtcp);
	}

	linphone_upnp_stream_destroy(session->audio);
	linphone_upnp_stream_destroy(session->video);
	ms_free(session);
}

LinphoneUpnpState linphone_upnp_session_get_state(UpnpSession *session) {
	return session->state;
}

/*
 * uPnP Config
 */

struct linphone_upnp_config_list_port_bindings_struct {
	struct _LpConfig *lpc;
	MSList *retList;
};

static void linphone_upnp_config_list_port_bindings_cb(const char *entry, struct linphone_upnp_config_list_port_bindings_struct *cookie) {
	char protocol_str[4]; // TCP or UDP
	upnp_igd_ip_protocol protocol;
	int external_port;
	int local_port;
	bool_t valid = TRUE;
	UpnpPortBinding *port;
	if(sscanf(entry, "%3s-%i-%i", protocol_str, &external_port, &local_port) == 3) {
		if(strcasecmp(protocol_str, "TCP") == 0) {
			protocol = UPNP_IGD_IP_PROTOCOL_TCP;
		} else if(strcasecmp(protocol_str, "UDP") == 0) {
			protocol = UPNP_IGD_IP_PROTOCOL_UDP;
		} else {
			valid = FALSE;
		}
		if(valid) {
			port = linphone_upnp_port_binding_new();
			port->state = LinphoneUpnpStateOk;
			port->protocol = protocol;
			port->external_port = external_port;
			port->local_port = local_port;
			cookie->retList = ms_list_append(cookie->retList, port);
		}
	} else {
		valid = FALSE;
	}
	if(!valid) {
		ms_warning("uPnP configuration invalid line: %s", entry);
	}
}

MSList *linphone_upnp_config_list_port_bindings(struct _LpConfig *lpc) {
	struct linphone_upnp_config_list_port_bindings_struct cookie = {lpc, NULL};
	lp_config_for_each_entry(lpc, UPNP_SECTION_NAME, (void(*)(const char *, void*))linphone_upnp_config_list_port_bindings_cb, &cookie);
	return cookie.retList;
}

void linphone_upnp_config_add_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port) {
	MSList *list;
	UpnpPortBinding *list_port;

	list = lupnp->removing_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			lupnp->removing_configs = ms_list_remove(lupnp->removing_configs, list_port);
			linphone_upnp_port_binding_release(list_port);
			return;
		}
		list = ms_list_next(list);
	}

	list = lupnp->adding_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			return;
		}
		list = ms_list_next(list);
	}

	list_port = linphone_upnp_port_binding_copy(port);
	lupnp->adding_configs = ms_list_append(lupnp->adding_configs, list_port);
}

void linphone_upnp_config_remove_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port) {
	MSList *list;
	UpnpPortBinding *list_port;

	list = lupnp->adding_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			lupnp->adding_configs = ms_list_remove(lupnp->adding_configs, list_port);
			linphone_upnp_port_binding_release(list_port);
			return;
		}
		list = ms_list_next(list);
	}

	list = lupnp->removing_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			return;
		}
		list = ms_list_next(list);
	}

	list_port = linphone_upnp_port_binding_copy(port);
	lupnp->removing_configs = ms_list_append(lupnp->removing_configs, list_port);
}
