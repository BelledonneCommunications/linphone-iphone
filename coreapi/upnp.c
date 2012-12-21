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

#define UPNP_MAX_RETRY 4

UpnpPortBinding *upnp_port_binding_new();
UpnpPortBinding * upnp_port_binding_retain(UpnpPortBinding *port);
void upnp_port_binding_release(UpnpPortBinding *port);

int upnp_context_send_remove_port_binding(LinphoneCore *lc, UpnpPortBinding *port);
int upnp_context_send_add_port_binding(LinphoneCore *lc, UpnpPortBinding *port);

/* Convert uPnP IGD logs to ortp logs */
void linphone_upnp_igd_print(void *cookie, upnp_igd_print_level level, const char *fmt, va_list list) {
	int ortp_level = ORTP_DEBUG;
	switch(level) {
	case UPNP_IGD_MESSAGE:
		ortp_level = ORTP_MESSAGE;
		break;
	case UPNP_IGD_WARNING:
		ortp_level = ORTP_WARNING;
		break;
	case UPNP_IGD_ERROR:
		ortp_level = ORTP_ERROR;
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
			lupnp->state = UPNP_Pending;
		} else if(strcasecmp(connection_status, "Connected")  || !nat_enabled) {
			lupnp->state = UPNP_Ko;
		} else {
			// Emit add port binding
			// Emit remove old port binding
			lupnp->state = UPNP_Ok;
		}

		break;

	case UPNP_IGD_PORT_MAPPING_ADD_SUCCESS:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->remote_port = mapping->remote_port;
		port_mapping->state = UPNP_Ok;
		// TODO: SAVE IN CONFIG THE PORT
		upnp_port_binding_release(port_mapping);
		break;

	case UPNP_IGD_PORT_MAPPING_ADD_FAILURE:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		upnp_context_send_add_port_binding(lc, port_mapping);
		upnp_port_binding_release(port_mapping);
		break;

	case UPNP_IGD_PORT_MAPPING_REMOVE_SUCCESS:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		port_mapping->remote_port = -1;
		port_mapping->state = UPNP_Idle;
		// TODO: REMOVE FROM CONFIG THE PORT
		upnp_port_binding_release(port_mapping);
		break;

	case UPNP_IGD_PORT_MAPPING_REMOVE_FAILURE:
		mapping = (upnp_igd_port_mapping *) arg;
		port_mapping = (UpnpPortBinding*) mapping->cookie;
		upnp_context_send_remove_port_binding(lc, port_mapping);
		// TODO: REMOVE FROM CONFIG THE PORT (DON'T TRY ANYMORE)
		upnp_port_binding_release(port_mapping);
		break;

	default:
		break;
	}

	ms_mutex_unlock(&lupnp->mutex);
}

int upnp_context_send_add_port_binding(LinphoneCore *lc, UpnpPortBinding *port) {
	UpnpContext *lupnp = &lc->upnp;
	upnp_igd_port_mapping mapping;
	char * local_host = NULL;
	int ret;
	if(port->state == UPNP_Idle) {
		port->remote_port = -1;
		port->retry = 0;
		port->state = UPNP_Pending;
	}
	if(port->retry >= UPNP_MAX_RETRY) {
		ret = -1;
	} else {
		local_host = upnp_igd_get_local_ipaddress(lupnp->upnp_igd_ctxt);
		mapping.cookie = upnp_port_binding_retain(port);
		mapping.local_port = port->local_port;
		mapping.local_host = local_host;
		mapping.remote_port = rand()%1024 + 1024;
		mapping.remote_host = "";
		mapping.description = PACKAGE_NAME;
		mapping.protocol = port->protocol;

		port->retry++;
		ret = upnp_igd_add_port_mapping(lupnp->upnp_igd_ctxt, &mapping);
		if(local_host != NULL) {
			free(local_host);
		}
	}
	if(ret != 0) {
		port->state = UPNP_Ko;
	}
	return ret;
}

int upnp_context_send_remove_port_binding(LinphoneCore *lc, UpnpPortBinding *port) {
	UpnpContext *lupnp = &lc->upnp;
	upnp_igd_port_mapping mapping;
	int ret;
	if(port->state == UPNP_Idle) {
		port->retry = 0;
		port->state = UPNP_Pending;
	}
	if(port->retry >= UPNP_MAX_RETRY) {
		ret = -1;
	} else {
		mapping.cookie = upnp_port_binding_retain(port);
		mapping.remote_port = port->remote_port;
		mapping.remote_host = "";
		mapping.protocol = port->protocol;
		port->retry++;
		ret = upnp_igd_delete_port_mapping(lupnp->upnp_igd_ctxt, &mapping);
	}
	if(ret != 0) {
		port->state = UPNP_Ko;
	}
	return ret;
}

int upnp_call_process(LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	UpnpContext *lupnp = &lc->upnp;
	int ret = -1;

	ms_mutex_lock(&lupnp->mutex);
	// Don't handle when the call
	if(lupnp->state != UPNP_Ko && call->upnp_session != NULL) {
		ret = 0;

		/*
		 * Audio part
		 */
		call->upnp_session->audio_rtp->local_port = call->audio_port;
		call->upnp_session->audio_rtcp->local_port = call->audio_port+1;
		if(call->upnp_session->audio_rtp->state == UPNP_Idle && call->audiostream != NULL) {
			// Add audio port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->audio_rtp);
		} else if(call->upnp_session->audio_rtp->state == UPNP_Ok && call->audiostream == NULL) {
			// Remove audio port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->audio_rtp);
		}
		if(call->upnp_session->audio_rtcp->state == UPNP_Idle && call->audiostream != NULL) {
			// Add audio port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->audio_rtcp);
		} else if(call->upnp_session->audio_rtcp->state == UPNP_Ok && call->audiostream == NULL) {
			// Remove audio port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->audio_rtcp);
		}

		/*
		 * Video part
		 */
		call->upnp_session->video_rtp->local_port = call->video_port;
		call->upnp_session->video_rtcp->local_port = call->video_port+1;
		if(call->upnp_session->video_rtp->state == UPNP_Idle && call->videostream != NULL) {
			// Add video port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->video_rtp);
		} else if(call->upnp_session->video_rtp->state == UPNP_Ok && call->videostream == NULL) {
			// Remove video port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->video_rtp);
		}
		if(call->upnp_session->video_rtcp->state == UPNP_Idle && call->videostream != NULL) {
			// Add video port binding
			upnp_context_send_add_port_binding(lc, call->upnp_session->video_rtcp);
		} else if(call->upnp_session->video_rtcp->state == UPNP_Ok && call->videostream == NULL) {
			// Remove video port binding
			upnp_context_send_remove_port_binding(lc, call->upnp_session->video_rtcp);
		}
	}
	ms_mutex_unlock(&lupnp->mutex);
	return ret;
}

int linphone_core_update_upnp(LinphoneCore *lc, LinphoneCall *call) {
	return upnp_call_process(call);
}

int upnp_context_init(LinphoneCore *lc) {
	LCSipTransports transport;
	UpnpContext *lupnp = &lc->upnp;
	ms_mutex_init(&lupnp->mutex, NULL);
	lupnp->state = UPNP_Idle;

	linphone_core_get_sip_transports(lc, &transport);
	if(transport.udp_port != 0) {
		lupnp->sip_udp = upnp_port_binding_new();
		lupnp->sip_udp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	} else {
		lupnp->sip_udp = NULL;
	}
	if(transport.tcp_port != 0) {
		lupnp->sip_tcp = upnp_port_binding_new();
		lupnp->sip_tcp->protocol = UPNP_IGD_IP_PROTOCOL_TCP;
	} else {
		lupnp->sip_tcp = NULL;
	}
	if(transport.tls_port != 0) {
		lupnp->sip_tls = upnp_port_binding_new();
		lupnp->sip_tls->protocol = UPNP_IGD_IP_PROTOCOL_TCP;
	} else {
		lupnp->sip_tls = NULL;
	}
	lupnp->upnp_igd_ctxt = NULL;
	lupnp->upnp_igd_ctxt = upnp_igd_create(linphone_upnp_igd_callback, linphone_upnp_igd_print, lc);
	if(lupnp->upnp_igd_ctxt == NULL ) {
		lupnp->state = UPNP_Ko;
		ms_error("Can't create uPnP IGD context");
		return -1;
	}
	lupnp->state = UPNP_Pending;
	return 0;
}

void upnp_context_uninit(LinphoneCore *lc) {
	// Emit remove port (sip & saved)
	UpnpContext *lupnp = &lc->upnp;
	if(lupnp->sip_udp != NULL) {
		upnp_port_binding_release(lupnp->sip_udp);
	}
	if(lupnp->sip_tcp != NULL) {
		upnp_port_binding_release(lupnp->sip_tcp);
	}
	if(lupnp->sip_tls != NULL) {
		upnp_port_binding_release(lupnp->sip_tls);
	}
	if(lupnp->upnp_igd_ctxt != NULL) {
		upnp_igd_destroy(lupnp->upnp_igd_ctxt);
	}
	ms_mutex_destroy(&lupnp->mutex);
}

UpnpPortBinding *upnp_port_binding_new() {
	UpnpPortBinding *port = NULL;
	port = ms_new0(UpnpPortBinding,1);
	ms_mutex_init(&port->mutex, NULL);
	port->state = UPNP_Idle;
	port->local_port = -1;
	port->remote_port = -1;
	port->ref = 1;
	return port;
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

UpnpSession* upnp_session_new() {
	UpnpSession *session = ms_new0(UpnpSession,1);
	session->state = UPNP_Idle;
	session->audio_rtp = upnp_port_binding_new();
	session->audio_rtp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	session->audio_rtcp = upnp_port_binding_new();
	session->audio_rtcp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	session->video_rtp = upnp_port_binding_new();
	session->video_rtp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	session->video_rtcp = upnp_port_binding_new();
	session->video_rtcp->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	return NULL;
}

void upnp_session_destroy(UpnpSession* session) {
	upnp_port_binding_release(session->audio_rtp);
	upnp_port_binding_release(session->audio_rtcp);
	upnp_port_binding_release(session->video_rtp);
	upnp_port_binding_release(session->video_rtp);
	// TODO: send remove
	ms_free(session);
}
