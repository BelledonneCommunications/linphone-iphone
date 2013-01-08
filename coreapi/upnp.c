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

#define UPNP_ADD_MAX_RETRY 4
#define UPNP_REMOVE_MAX_RETRY 4
#define UPNP_SECTION_NAME "uPnP"

/* Define private types */
typedef struct _LpItem{
	char *key;
	char *value;
} LpItem;

typedef struct _LpSection{
	char *name;
	MSList *items;
} LpSection;

typedef struct _LpConfig{
	FILE *file;
	char *filename;
	MSList *sections;
	int modified;
	int readonly;
} LpConfig;

/* Declare private functions */
LpSection *lp_config_find_section(LpConfig *lpconfig, const char *name);
void lp_section_remove_item(LpSection *sec, LpItem *item);
void lp_config_set_string(LpConfig *lpconfig,const char *section, const char *key, const char *value);

bool_t linphone_core_upnp_hook(void *data);

UpnpPortBinding *upnp_port_binding_new();
UpnpPortBinding *upnp_port_binding_copy(const UpnpPortBinding *port);
bool_t upnp_port_binding_equal(const UpnpPortBinding *port1, const UpnpPortBinding *port2);
UpnpPortBinding *upnp_port_binding_retain(UpnpPortBinding *port);
void upnp_port_binding_log(int level, const char *msg, const UpnpPortBinding *port);
void upnp_port_binding_release(UpnpPortBinding *port);

MSList *upnp_config_list_port_bindings(struct _LpConfig *lpc);
void upnp_config_add_port_binding(LinphoneCore *lc, const UpnpPortBinding *port);
void upnp_config_remove_port_binding(LinphoneCore *lc, const UpnpPortBinding *port);

int upnp_context_send_remove_port_binding(LinphoneCore *lc, UpnpPortBinding *port);
int upnp_context_send_add_port_binding(LinphoneCore *lc, UpnpPortBinding *port);


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
	LinphoneCore *lc = (LinphoneCore *)cookie;
	UpnpContext *lupnp = &lc->upnp;
	upnp_igd_port_mapping *mapping = NULL;
	UpnpPortBinding *port_mapping = NULL;
	const char *ip_address = NULL;
	const char *connection_status = NULL;
	bool_t nat_enabled = FALSE;
	ms_mutex_lock(&lupnp->mutex);

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
			if(lupnp->state != LinphoneUpnpStateOk) {
				lupnp->clean = TRUE; // Remove saved port mapping configurations
			}
			lupnp->state = LinphoneUpnpStateOk;
		}

		break;

	case UPNP_IGD_PORT_MAPPING_ADD_SUCCESS:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->external_port = mapping->remote_port;
		port_mapping->state = LinphoneUpnpStateOk;
		upnp_port_binding_log(ORTP_MESSAGE, "Added port binding", port_mapping);
		upnp_config_add_port_binding(lc, port_mapping);

		lupnp->pending_bindings = ms_list_remove(lupnp->pending_bindings, port_mapping);
		upnp_port_binding_release(port_mapping);
		break;

	case UPNP_IGD_PORT_MAPPING_ADD_FAILURE:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		if(upnp_context_send_add_port_binding(lc, port_mapping) != 0) {
			upnp_port_binding_log(ORTP_ERROR, "Can't add port binding", port_mapping);
		}

		lupnp->pending_bindings = ms_list_remove(lupnp->pending_bindings, port_mapping);
		upnp_port_binding_release(port_mapping);
		break;

	case UPNP_IGD_PORT_MAPPING_REMOVE_SUCCESS:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->state = LinphoneUpnpStateIdle;
		upnp_port_binding_log(ORTP_MESSAGE, "Removed port binding", port_mapping);
		upnp_config_remove_port_binding(lc, port_mapping);

		lupnp->pending_bindings = ms_list_remove(lupnp->pending_bindings, port_mapping);
		upnp_port_binding_release(port_mapping);
		break;

	case UPNP_IGD_PORT_MAPPING_REMOVE_FAILURE:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		if(upnp_context_send_remove_port_binding(lc, port_mapping) != 0) {
			upnp_port_binding_log(ORTP_ERROR, "Can't remove port binding", port_mapping);
			upnp_config_remove_port_binding(lc, port_mapping);
		}

		lupnp->pending_bindings = ms_list_remove(lupnp->pending_bindings, port_mapping);
		upnp_port_binding_release(port_mapping);
		break;

	default:
		break;
	}

	if(lupnp->pending_bindings == NULL) {
		if(lupnp->cleaning == TRUE) {
			lupnp->emit = TRUE; // Emit port bindings
			lupnp->cleaning = FALSE;
		}
		pthread_cond_signal(&lupnp->cond);
	}
	ms_mutex_unlock(&lupnp->mutex);
}


/**
 * uPnP Context
 */

int upnp_context_init(LinphoneCore *lc) {
	LCSipTransports transport;
	UpnpContext *lupnp = &lc->upnp;
	const char *ip_address;

	ms_mutex_init(&lupnp->mutex, NULL);
	ms_cond_init(&lupnp->cond, NULL);

	lupnp->pending_bindings = NULL;
	lupnp->adding_configs = NULL;
	lupnp->removing_configs = NULL;
	lupnp->clean = FALSE;
	lupnp->cleaning = FALSE;
	lupnp->emit = FALSE;
	lupnp->state = LinphoneUpnpStateIdle;
	ms_message("uPnP IGD: Init");

	linphone_core_get_sip_transports(lc, &transport);
	if(transport.udp_port != 0) {
		lupnp->sip_udp = upnp_port_binding_new();
		lupnp->sip_udp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
		lupnp->sip_udp->local_port = transport.udp_port;
		lupnp->sip_udp->external_port = transport.udp_port;
	} else {
		lupnp->sip_udp = NULL;
	}
	if(transport.tcp_port != 0) {
		lupnp->sip_tcp = upnp_port_binding_new();
		lupnp->sip_tcp->protocol = UPNP_IGD_IP_PROTOCOL_TCP;
		lupnp->sip_tcp->local_port = transport.tcp_port;
		lupnp->sip_tcp->external_port = transport.tcp_port;
	} else {
		lupnp->sip_tcp = NULL;
	}
	if(transport.tls_port != 0) {
		lupnp->sip_tls = upnp_port_binding_new();
		lupnp->sip_tls->protocol = UPNP_IGD_IP_PROTOCOL_TCP;
		lupnp->sip_tls->local_port = transport.tls_port;
		lupnp->sip_tls->external_port = transport.tls_port;
	} else {
		lupnp->sip_tls = NULL;
	}

	linphone_core_add_iterate_hook(lc, linphone_core_upnp_hook, lc);

	lupnp->upnp_igd_ctxt = NULL;
	lupnp->upnp_igd_ctxt = upnp_igd_create(linphone_upnp_igd_callback, linphone_upnp_igd_print, lc);
	if(lupnp->upnp_igd_ctxt == NULL) {
		lupnp->state = LinphoneUpnpStateKo;
		ms_error("Can't create uPnP IGD context");
		return -1;
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
	return 0;
}

void upnp_context_uninit(LinphoneCore *lc) {
	UpnpContext *lupnp = &lc->upnp;

	/*
	 * Not need, all hooks are removed before
	 * linphone_core_remove_iterate_hook(lc, linphone_core_upnp_hook, lc);
     */

	/* Send port binding removes */
	if(lupnp->sip_udp != NULL) {
		upnp_context_send_remove_port_binding(lc, lupnp->sip_udp);
		lupnp->sip_udp = NULL;
	}
	if(lupnp->sip_tcp != NULL) {
		upnp_context_send_remove_port_binding(lc, lupnp->sip_tcp);
		lupnp->sip_tcp = NULL;
	}
	if(lupnp->sip_tls != NULL) {
		upnp_context_send_remove_port_binding(lc, lupnp->sip_tls);
		lupnp->sip_tcp = NULL;
	}

	/* Wait all pending bindings are done */
	ms_message("uPnP IGD: Wait all pending port bindings ...");
	ms_mutex_lock(&lupnp->mutex);
	ms_cond_wait(&lupnp->cond, &lupnp->mutex);
	ms_mutex_unlock(&lupnp->mutex);

	if(lupnp->upnp_igd_ctxt != NULL) {
		upnp_igd_destroy(lupnp->upnp_igd_ctxt);
	}

	/* Run one time the hook for configuration update */
	linphone_core_upnp_hook(lc);

	/* Release port bindings */
	if(lupnp->sip_udp != NULL) {
		upnp_port_binding_release(lupnp->sip_udp);
		lupnp->sip_udp = NULL;
	}
	if(lupnp->sip_tcp != NULL) {
		upnp_port_binding_release(lupnp->sip_tcp);
		lupnp->sip_tcp = NULL;
	}
	if(lupnp->sip_tls != NULL) {
		upnp_port_binding_release(lupnp->sip_tls);
		lupnp->sip_tcp = NULL;
	}

	/* Release lists */
	ms_list_for_each(lupnp->adding_configs,(void (*)(void*))upnp_port_binding_release);
	lupnp->adding_configs = ms_list_free(lupnp->adding_configs);
	ms_list_for_each(lupnp->removing_configs,(void (*)(void*))upnp_port_binding_release);
	lupnp->removing_configs = ms_list_free(lupnp->removing_configs);
	ms_list_for_each(lupnp->pending_bindings,(void (*)(void*))upnp_port_binding_release);
	lupnp->pending_bindings = ms_list_free(lupnp->pending_bindings);

	ms_mutex_destroy(&lupnp->mutex);
	ms_cond_destroy(&lupnp->cond);

	ms_message("uPnP IGD: Uninit");
}

int upnp_context_send_add_port_binding(LinphoneCore *lc, UpnpPortBinding *port) {
	UpnpContext *lupnp = &lc->upnp;
	upnp_igd_port_mapping mapping;
	int ret;
	if(port->state == LinphoneUpnpStateIdle) {
		port->retry = 0;
		port->state = LinphoneUpnpStateAdding;
	} else if(port->state != LinphoneUpnpStateAdding) {
		ms_error("uPnP: try to add a port binding in wrong state: %d", port->state);
		return -2;
	}

	if(port->retry >= UPNP_ADD_MAX_RETRY) {
		ret = -1;
	} else {
		mapping.cookie = upnp_port_binding_retain(port);
		lupnp->pending_bindings = ms_list_append(lupnp->pending_bindings, mapping.cookie);

		mapping.local_port = port->local_port;
		mapping.local_host = port->local_addr;
		if(port->external_port == -1)
			mapping.remote_port = rand()%(0xffff - 1024) + 1024; // TODO: use better method
		else
			mapping.remote_port = port->external_port;
		mapping.remote_host = "";
		mapping.description = PACKAGE_NAME;
		mapping.protocol = port->protocol;

		port->retry++;
		upnp_port_binding_log(ORTP_DEBUG, "Adding port binding...", port);
		ret = upnp_igd_add_port_mapping(lupnp->upnp_igd_ctxt, &mapping);
	}
	if(ret != 0) {
		port->state = LinphoneUpnpStateKo;
	}
	return ret;
}

int upnp_context_send_remove_port_binding(LinphoneCore *lc, UpnpPortBinding *port) {
	UpnpContext *lupnp = &lc->upnp;
	upnp_igd_port_mapping mapping;
	int ret;
	if(port->state == LinphoneUpnpStateOk) {
		port->retry = 0;
		port->state = LinphoneUpnpStateRemoving;
	} else if(port->state != LinphoneUpnpStateRemoving) {
		ms_error("uPnP: try to remove a port binding in wrong state: %d", port->state);
		return -2;
	}

	if(port->retry >= UPNP_REMOVE_MAX_RETRY) {
		ret = -1;
	} else {
		mapping.cookie = upnp_port_binding_retain(port);
		lupnp->pending_bindings = ms_list_append(lupnp->pending_bindings, mapping.cookie);

		mapping.remote_port = port->external_port;
		mapping.remote_host = "";
		mapping.protocol = port->protocol;
		port->retry++;
		upnp_port_binding_log(ORTP_DEBUG, "Removing port binding...", port);
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
	UpnpContext *lupnp = &lc->upnp;
	int ret = -1;
	const char *local_addr, *external_addr;

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
		if(call->upnp_session->audio->rtp->state == LinphoneUpnpStateIdle && audio) {
			// Add audio port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->audio->rtp);
		} else if(call->upnp_session->audio->rtp->state == LinphoneUpnpStateOk && !audio) {
			// Remove audio port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->audio->rtp);
		}
		if(call->upnp_session->audio->rtcp->state == LinphoneUpnpStateIdle && audio) {
			// Add audio port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->audio->rtcp);
		} else if(call->upnp_session->audio->rtcp->state == LinphoneUpnpStateOk && !audio) {
			// Remove audio port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->audio->rtcp);
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
		if(call->upnp_session->video->rtp->state == LinphoneUpnpStateIdle && video) {
			// Add video port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->video->rtp);
		} else if(call->upnp_session->video->rtp->state == LinphoneUpnpStateOk && !video) {
			// Remove video port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->video->rtp);
		}
		if(call->upnp_session->video->rtcp->state == LinphoneUpnpStateIdle && video) {
			// Add video port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->video->rtcp);
		} else if(call->upnp_session->video->rtcp->state == LinphoneUpnpStateOk && !video) {
			// Remove video port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->video->rtcp);
		}
	}

	ms_mutex_unlock(&lupnp->mutex);

	/*
	 * Update uPnP call state
	 */
	upnp_call_process(call);

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

int upnp_call_process(LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	UpnpContext *lupnp = &lc->upnp;
	int ret = -1;
	UpnpState oldState;

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

bool_t linphone_core_upnp_hook(void *data) {
	char key[64];
	MSList *list = NULL;
	MSList *item;
	UpnpPortBinding *port_mapping;
	LinphoneCore *lc = (LinphoneCore *)data;
	LinphoneCall *call;
	UpnpContext *lupnp = &lc->upnp;
	ms_mutex_lock(&lupnp->mutex);

	if(lupnp->clean && !lupnp->cleaning) {
		lupnp->clean = FALSE;
		// Remove old mapping
		list = upnp_config_list_port_bindings(lc->config);
		if(list == NULL) {
			lupnp->emit = TRUE;
		} else {
			lupnp->cleaning = TRUE;
			for(item = list;item != NULL; item = item->next) {
				port_mapping = (UpnpPortBinding *)item->data;
				upnp_context_send_remove_port_binding(lc, port_mapping);
			}
			ms_list_for_each(list,(void (*)(void*))upnp_port_binding_release);
			list = ms_list_free(list);
		}
	}

	if(lupnp->emit) {
		lupnp->emit = FALSE;

		/* Force port bindings */
		if(lupnp->sip_udp != NULL) {
			lupnp->sip_udp->state = LinphoneUpnpStateIdle;
			upnp_context_send_add_port_binding(lc, lupnp->sip_udp);
		}
		if(lupnp->sip_tcp != NULL) {
			lupnp->sip_udp->state = LinphoneUpnpStateIdle;
			upnp_context_send_add_port_binding(lc, lupnp->sip_tcp);
		}
		if(lupnp->sip_tls != NULL) {
			lupnp->sip_udp->state = LinphoneUpnpStateIdle;
			upnp_context_send_add_port_binding(lc, lupnp->sip_tls);
		}
		list = lc->calls;
		while(list != NULL) {
			call = (LinphoneCall *)list->data;
			call->upnp_session->audio->rtp->state = LinphoneUpnpStateIdle;
			call->upnp_session->audio->rtcp->state = LinphoneUpnpStateIdle;
			call->upnp_session->video->rtp->state = LinphoneUpnpStateIdle;
			call->upnp_session->video->rtcp->state = LinphoneUpnpStateIdle;
			linphone_core_update_upnp_audio_video(call, call->audiostream!=NULL, call->videostream!=NULL);
			list = list->next;
		}
	}

	/* Add configs */
	for(item = lupnp->adding_configs;item!=NULL;item=item->next) {
		port_mapping = (UpnpPortBinding *)item->data;
		snprintf(key, sizeof(key), "%s-%d-%d",
					(port_mapping->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
							port_mapping->external_port,
							port_mapping->local_port);
		lp_config_set_string(lc->config, UPNP_SECTION_NAME, key, "uPnP");
		upnp_port_binding_log(ORTP_DEBUG, "Configuration: Added port binding", port_mapping);
	}
	ms_list_for_each(lupnp->adding_configs,(void (*)(void*))upnp_port_binding_release);
	lupnp->adding_configs = ms_list_free(lupnp->adding_configs);

	/* Remove configs */
	for(item = lupnp->removing_configs;item!=NULL;item=item->next) {
		port_mapping = (UpnpPortBinding *)item->data;
		snprintf(key, sizeof(key), "%s-%d-%d",
					(port_mapping->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
							port_mapping->external_port,
							port_mapping->local_port);
		lp_config_set_string(lc->config, UPNP_SECTION_NAME, key, NULL);
		upnp_port_binding_log(ORTP_DEBUG, "Configuration: Removed port binding", port_mapping);
	}
	ms_list_for_each(lupnp->removing_configs,(void (*)(void*))upnp_port_binding_release);
	lupnp->removing_configs = ms_list_free(lupnp->removing_configs);

	ms_mutex_unlock(&lupnp->mutex);
	return TRUE;
}

void linphone_core_update_local_media_description_from_upnp(SalMediaDescription *desc, UpnpSession *session) {
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
}


/*
 * uPnP Port Binding
 */

UpnpPortBinding *upnp_port_binding_new() {
	UpnpPortBinding *port = NULL;
	port = ms_new0(UpnpPortBinding,1);
	ms_mutex_init(&port->mutex, NULL);
	port->state = LinphoneUpnpStateIdle;
	port->local_addr[0] = '\0';
	port->local_port = -1;
	port->external_addr[0] = '\0';
	port->external_port = -1;
	port->ref = 1;
	return port;
}

UpnpPortBinding *upnp_port_binding_copy(const UpnpPortBinding *port) {
	UpnpPortBinding *new_port = NULL;
	new_port = ms_new0(UpnpPortBinding,1);
	memcpy(new_port, port, sizeof(UpnpPortBinding));
	ms_mutex_init(&new_port->mutex, NULL);
	new_port->ref = 1;
	return new_port;
}

void upnp_port_binding_log(int level, const char *msg, const UpnpPortBinding *port) {
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

bool_t upnp_port_binding_equal(const UpnpPortBinding *port1, const UpnpPortBinding *port2) {
	return port1->protocol == port2->protocol &&
			port1->local_port == port2->local_port &&
			port1->external_port == port2->external_port;
}

UpnpPortBinding *upnp_port_binding_retain(UpnpPortBinding *port) {
	ms_mutex_lock(&port->mutex);
	port->ref++;
	ms_mutex_unlock(&port->mutex);
	return port;
}

void upnp_port_binding_release(UpnpPortBinding *port) {
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

UpnpStream* upnp_stream_new() {
	UpnpStream *stream = ms_new0(UpnpStream,1);
	stream->state = LinphoneUpnpStateIdle;
	stream->rtp = upnp_port_binding_new();
	stream->rtp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	stream->rtcp = upnp_port_binding_new();
	stream->rtcp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	return stream;
}

void upnp_stream_destroy(UpnpStream* stream) {
	upnp_port_binding_release(stream->rtp);
	stream->rtp = NULL;
	upnp_port_binding_release(stream->rtcp);
	stream->rtcp = NULL;
	ms_free(stream);
}


/*
 * uPnP Session
 */

UpnpSession* upnp_session_new() {
	UpnpSession *session = ms_new0(UpnpSession,1);
	session->state = LinphoneUpnpStateIdle;
	session->audio = upnp_stream_new();
	session->video = upnp_stream_new();
	return session;
}

void upnp_session_destroy(LinphoneCall* call) {
	LinphoneCore *lc = call->core;

	/* Remove bindings */
	if(call->upnp_session->audio->rtp->state != LinphoneUpnpStateKo && call->upnp_session->audio->rtp->state != LinphoneUpnpStateIdle) {
		upnp_context_send_remove_port_binding(lc, call->upnp_session->audio->rtp);
	}
	if(call->upnp_session->audio->rtcp->state != LinphoneUpnpStateKo && call->upnp_session->audio->rtcp->state != LinphoneUpnpStateIdle) {
		upnp_context_send_remove_port_binding(lc, call->upnp_session->audio->rtcp);
	}
	if(call->upnp_session->video->rtp->state != LinphoneUpnpStateKo && call->upnp_session->video->rtp->state != LinphoneUpnpStateIdle) {
		upnp_context_send_remove_port_binding(lc, call->upnp_session->video->rtp);
	}
	if(call->upnp_session->video->rtcp->state != LinphoneUpnpStateKo && call->upnp_session->video->rtcp->state != LinphoneUpnpStateIdle) {
		upnp_context_send_remove_port_binding(lc, call->upnp_session->video->rtcp);
	}

	upnp_stream_destroy(call->upnp_session->audio);
	upnp_stream_destroy(call->upnp_session->video);
	ms_free(call->upnp_session);
	call->upnp_session = NULL;
}


/*
 * uPnP Config
 */

MSList *upnp_config_list_port_bindings(struct _LpConfig *lpc) {
	char protocol_str[4]; // TCP or UDP
	upnp_igd_ip_protocol protocol;
	int external_port;
	int local_port;
	MSList *retList = NULL;
	UpnpPortBinding *port;
	bool_t valid;
	MSList *elem;
	LpItem *item;
	LpSection *sec=lp_config_find_section(lpc, UPNP_SECTION_NAME);
	if(sec == NULL)
		return retList;

	elem = sec->items;
	while(elem != NULL) {
		item=(LpItem*)elem->data;
		valid = TRUE;
		if(sscanf(item->key, "%3s-%i-%i", protocol_str, &external_port, &local_port) == 3) {
			if(strcasecmp(protocol_str, "TCP") == 0) {
				protocol = UPNP_IGD_IP_PROTOCOL_TCP;
			} else if(strcasecmp(protocol_str, "UDP") == 0) {
				protocol = UPNP_IGD_IP_PROTOCOL_UDP;
			} else {
				valid = FALSE;
			}
			if(valid) {
				port = upnp_port_binding_new();
				port->state = LinphoneUpnpStateOk;
				port->protocol = protocol;
				port->external_port = external_port;
				port->local_port = local_port;
				retList = ms_list_append(retList, port);
			}
		} else {
			valid = FALSE;
		}
		elem = ms_list_next(elem);
		if(!valid) {
			ms_warning("uPnP configuration invalid line: %s", item->key);
			lp_section_remove_item(sec, item);
		}
	}

	return retList;
}

void upnp_config_add_port_binding(LinphoneCore *lc, const UpnpPortBinding *port) {
	UpnpContext *lupnp = &lc->upnp;
	MSList *list;
	UpnpPortBinding *list_port;

	list = lupnp->removing_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(upnp_port_binding_equal(list_port, port) == TRUE) {
			lupnp->removing_configs = ms_list_remove(lupnp->removing_configs, list_port);
			upnp_port_binding_release(list_port);
			return;
		}
		list = ms_list_next(list);
	}

	list = lupnp->adding_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(upnp_port_binding_equal(list_port, port) == TRUE) {
			return;
		}
		list = ms_list_next(list);
	}

	list_port = upnp_port_binding_copy(port);
	lupnp->adding_configs = ms_list_append(lupnp->adding_configs, list_port);
}

void upnp_config_remove_port_binding(LinphoneCore *lc, const UpnpPortBinding *port) {
	UpnpContext *lupnp = &lc->upnp;
	MSList *list;
	UpnpPortBinding *list_port;

	list = lupnp->adding_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(upnp_port_binding_equal(list_port, port) == TRUE) {
			lupnp->adding_configs = ms_list_remove(lupnp->adding_configs, list_port);
			upnp_port_binding_release(list_port);
			return;
		}
		list = ms_list_next(list);
	}

	list = lupnp->removing_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(upnp_port_binding_equal(list_port, port) == TRUE) {
			return;
		}
		list = ms_list_next(list);
	}

	list_port = upnp_port_binding_copy(port);
	lupnp->removing_configs = ms_list_append(lupnp->removing_configs, list_port);
}
