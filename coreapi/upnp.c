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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "upnp.h"
#include "private.h"
#include "linphone/lpconfig.h"
#include <ctype.h>

#define UPNP_STRINGIFY(x)     #x
#define UPNP_TOSTRING(x)      UPNP_STRINGIFY(x)

#define UPNP_ADD_MAX_RETRY    4
#define UPNP_REMOVE_MAX_RETRY 4
#define UPNP_SECTION_NAME     "uPnP"
#define UPNP_CORE_READY_CHECK 1
#define UPNP_CORE_RETRY_DELAY 10
#define UPNP_CALL_RETRY_DELAY 3
#define UPNP_UUID_LEN         128
#define UPNP_UUID_LEN_STR     UPNP_TOSTRING(UPNP_UUID_LEN)
/*
 * uPnP Definitions
 */

typedef struct _UpnpPortBinding {
	ms_mutex_t mutex;
	LinphoneUpnpState state;
	upnp_igd_ip_protocol protocol;
	char *device_id;
	char local_addr[LINPHONE_IPADDR_SIZE];
	int local_port;
	char external_addr[LINPHONE_IPADDR_SIZE];
	int external_port;
	int retry;
	int ref;
	bool_t to_remove;
	bool_t to_add;
	time_t last_update;
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
	bctbx_list_t *removing_configs;
	bctbx_list_t *adding_configs;
	bctbx_list_t *pending_bindings;

	ms_mutex_t mutex;
	ms_cond_t empty_cond;

	time_t last_ready_check;
	LinphoneUpnpState last_ready_state;
};


bool_t linphone_core_upnp_hook(void *data);
void linphone_upnp_update(UpnpContext *ctx);
bool_t linphone_upnp_is_blacklisted(UpnpContext *ctx);

UpnpPortBinding *linphone_upnp_port_binding_new(void);
UpnpPortBinding *linphone_upnp_port_binding_new_with_parameters(upnp_igd_ip_protocol protocol, int local_port, int external_port);
UpnpPortBinding *linphone_upnp_port_binding_new_or_collect(bctbx_list_t *list, upnp_igd_ip_protocol protocol, int local_port, int external_port);
UpnpPortBinding *linphone_upnp_port_binding_copy(const UpnpPortBinding *port);
void linphone_upnp_port_binding_set_device_id(UpnpPortBinding *port, const char * device_id);
bool_t linphone_upnp_port_binding_equal(const UpnpPortBinding *port1, const UpnpPortBinding *port2);
UpnpPortBinding *linphone_upnp_port_binding_equivalent_in_list(bctbx_list_t *list, const UpnpPortBinding *port);
UpnpPortBinding *linphone_upnp_port_binding_retain(UpnpPortBinding *port);
void linphone_upnp_update_port_binding(UpnpContext *lupnp, UpnpPortBinding **port_mapping, upnp_igd_ip_protocol protocol, int port, int retry_delay);
void linphone_upnp_port_binding_log(int level, const char *msg, const UpnpPortBinding *port);
void linphone_upnp_port_binding_release(UpnpPortBinding *port);
void linphone_upnp_update_config(UpnpContext *lupnp);
void linphone_upnp_update_proxy(UpnpContext *lupnp, bool_t force);

// Configuration
bctbx_list_t *linphone_upnp_config_list_port_bindings(struct _LpConfig *lpc, const char *device_id);
void linphone_upnp_config_add_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port);
void linphone_upnp_config_remove_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port);

// uPnP
int linphone_upnp_context_send_remove_port_binding(UpnpContext *lupnp, UpnpPortBinding *port, bool_t retry);
int linphone_upnp_context_send_add_port_binding(UpnpContext *lupnp, UpnpPortBinding *port, bool_t retry);

static int linphone_upnp_strncmpi(const char *str1, const char *str2, int len) {
	int i = 0;
	char char1, char2;
	while(i < len) {
		char1 = toupper(*str1);
		char2 = toupper(*str2);
		if(char1 == '\0' || char1 != char2) {
			return char1 - char2;
		}
		str1++;
		str2++;
		i++;
	}
	return 0;
}

static int linphone_upnp_str_min(const char *str1, const char *str2) {
	int len1 = strlen(str1);
	int len2 = strlen(str2);
	if(len1 > len2) {
		return len2;
	}
	return len1;
}

char * linphone_upnp_format_device_id(const char *device_id) {
	char *ret = NULL;
	char *tmp;
	char tchar;
	bool_t copy;
	if(device_id == NULL) {
		return ret;
	}
	ret = ms_new0(char, UPNP_UUID_LEN + 1);
	tmp = ret;
	if(linphone_upnp_strncmpi(device_id, "uuid:", linphone_upnp_str_min(device_id, "uuid:")) == 0) {
		device_id += strlen("uuid:");
	}
	while(*device_id != '\0' && tmp - ret < UPNP_UUID_LEN) {
		copy = FALSE;
		tchar = *device_id;
		if(tchar >= '0' && tchar <= '9')
			copy = TRUE;
		if(!copy && tchar >= 'A' && tchar <= 'Z')
			copy = TRUE;
		if(!copy && tchar >= 'a' && tchar <= 'z')
			copy = TRUE;
		if(copy) {
			*tmp = *device_id;
			tmp++;
		}
		device_id++;
	}
	*tmp = '\0';
	return ret;
}

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
	ortp_logv(ORTP_LOG_DOMAIN, ortp_level, fmt, list);
}

void linphone_upnp_igd_callback(void *cookie, upnp_igd_event event, void *arg) {
	UpnpContext *lupnp = (UpnpContext *)cookie;
	upnp_igd_port_mapping *mapping = NULL;
	UpnpPortBinding *port_mapping = NULL;
	const char *ip_address = NULL;
	const char *connection_status = NULL;
	bool_t nat_enabled = FALSE;
	bool_t blacklisted = FALSE;
	LinphoneUpnpState old_state;

	if(lupnp == NULL || lupnp->upnp_igd_ctxt == NULL) {
		ms_error("uPnP IGD: Invalid context in callback");
		return;
	}

	ms_mutex_lock(&lupnp->mutex);
	old_state = lupnp->state;

	switch(event) {
	case UPNP_IGD_DEVICE_ADDED:
	case UPNP_IGD_DEVICE_REMOVED:
	case UPNP_IGD_EXTERNAL_IPADDRESS_CHANGED:
	case UPNP_IGD_NAT_ENABLED_CHANGED:
	case UPNP_IGD_CONNECTION_STATUS_CHANGED:
		ip_address = upnp_igd_get_external_ipaddress(lupnp->upnp_igd_ctxt);
		connection_status = upnp_igd_get_connection_status(lupnp->upnp_igd_ctxt);
		nat_enabled = upnp_igd_get_nat_enabled(lupnp->upnp_igd_ctxt);
		blacklisted = linphone_upnp_is_blacklisted(lupnp);

		if(ip_address == NULL || connection_status == NULL) {
			ms_message("uPnP IGD: Pending");
			lupnp->state = LinphoneUpnpStatePending;
		} else if(strcasecmp(connection_status, "Connected")  || !nat_enabled) {
			ms_message("uPnP IGD: Not Available");
			lupnp->state = LinphoneUpnpStateNotAvailable;
		} else if(blacklisted) {
			ms_message("uPnP IGD: Router is blacklisted");
			lupnp->state = LinphoneUpnpStateBlacklisted;
		} else {
			ms_message("uPnP IGD: Connected");
			lupnp->state = LinphoneUpnpStateOk;
			if(old_state != LinphoneUpnpStateOk) {
				linphone_upnp_update(lupnp);
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
		if(linphone_upnp_context_send_add_port_binding(lupnp, port_mapping, TRUE) != 0) {
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
		if(linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping, TRUE) != 0) {
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
				linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping, FALSE);
			} else if(port_mapping->state == LinphoneUpnpStateKo) {
				port_mapping->to_remove = FALSE;
			}
		}
		if(port_mapping->to_add) {
			if(port_mapping->state == LinphoneUpnpStateIdle || port_mapping->state == LinphoneUpnpStateKo) {
				port_mapping->to_add = FALSE;
				linphone_upnp_context_send_add_port_binding(lupnp, port_mapping, FALSE);
			}
		}

		lupnp->pending_bindings = bctbx_list_remove(lupnp->pending_bindings, port_mapping);
		linphone_upnp_port_binding_release(port_mapping);
	}

	/*
	 * If there is no pending binding emit a signal
	 */
	if(lupnp->pending_bindings == NULL) {
		ms_cond_signal(&lupnp->empty_cond);
	}
	ms_mutex_unlock(&lupnp->mutex);
}


/**
 * uPnP Context
 */

UpnpContext* linphone_upnp_context_new(LinphoneCore *lc) {
	UpnpContext *lupnp = (UpnpContext *)ms_new0(UpnpContext,1);
	char address[LINPHONE_IPADDR_SIZE];
	const char*upnp_binding_address=address;
	if (linphone_core_get_local_ip_for(lc->sip_conf.ipv6_enabled ? AF_INET6 : AF_INET,NULL,address)) {
		ms_warning("Linphone core [%p] cannot guess local address for upnp, let's choice the lib",lc);
		upnp_binding_address=NULL;
	}
	ms_mutex_init(&lupnp->mutex, NULL);
	ms_cond_init(&lupnp->empty_cond, NULL);

	lupnp->last_ready_check = 0;
	lupnp->last_ready_state = LinphoneUpnpStateIdle;

	lupnp->lc = lc;
	lupnp->pending_bindings = NULL;
	lupnp->adding_configs = NULL;
	lupnp->removing_configs = NULL;
	lupnp->state = LinphoneUpnpStateIdle;
	ms_message("uPnP IGD: New %p for core %p bound to %s", lupnp, lc,upnp_binding_address);

	// Init ports
	lupnp->sip_udp = NULL;
	lupnp->sip_tcp = NULL;
	lupnp->sip_tls = NULL;

	linphone_core_add_iterate_hook(lc, linphone_core_upnp_hook, lupnp);

	lupnp->upnp_igd_ctxt = NULL;
	lupnp->upnp_igd_ctxt = upnp_igd_create(linphone_upnp_igd_callback, linphone_upnp_igd_print, address, lupnp);
	if(lupnp->upnp_igd_ctxt == NULL) {
		lupnp->state = LinphoneUpnpStateKo;
		ms_error("Can't create uPnP IGD context");
		return NULL;
	}

	lupnp->state = LinphoneUpnpStatePending;
	upnp_igd_start(lupnp->upnp_igd_ctxt);

	return lupnp;
}

void linphone_upnp_context_destroy(UpnpContext *lupnp) {
	linphone_core_remove_iterate_hook(lupnp->lc, linphone_core_upnp_hook, lupnp);

	ms_mutex_lock(&lupnp->mutex);

	if(lupnp->lc->sip_network_reachable) {
		/* Send port binding removes */
		if(lupnp->sip_udp != NULL) {
			linphone_upnp_context_send_remove_port_binding(lupnp, lupnp->sip_udp, TRUE);
		}
		if(lupnp->sip_tcp != NULL) {
			linphone_upnp_context_send_remove_port_binding(lupnp, lupnp->sip_tcp, TRUE);
		}
		if(lupnp->sip_tls != NULL) {
			linphone_upnp_context_send_remove_port_binding(lupnp, lupnp->sip_tls, TRUE);
		}
	}

	/* Wait all pending bindings are done */
	if(lupnp->pending_bindings != NULL) {
		ms_message("uPnP IGD: Wait all pending port bindings ...");
		ms_cond_wait(&lupnp->empty_cond, &lupnp->mutex);
	}
	ms_mutex_unlock(&lupnp->mutex);

	if(lupnp->upnp_igd_ctxt != NULL) {
		upnp_igd_destroy(lupnp->upnp_igd_ctxt);
		lupnp->upnp_igd_ctxt = NULL;
	}

	/* No more multi threading here */

	/* Run one more time configuration update and proxy */
	linphone_upnp_update_config(lupnp);
	linphone_upnp_update_proxy(lupnp, TRUE);

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
	bctbx_list_for_each(lupnp->adding_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->adding_configs = bctbx_list_free(lupnp->adding_configs);
	bctbx_list_for_each(lupnp->removing_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->removing_configs = bctbx_list_free(lupnp->removing_configs);
	bctbx_list_for_each(lupnp->pending_bindings,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->pending_bindings = bctbx_list_free(lupnp->pending_bindings);

	ms_mutex_destroy(&lupnp->mutex);
	ms_cond_destroy(&lupnp->empty_cond);

	ms_message("uPnP IGD: destroy %p", lupnp);
	ms_free(lupnp);
}

LinphoneUpnpState linphone_upnp_context_get_state(UpnpContext *lupnp) {
	LinphoneUpnpState state = LinphoneUpnpStateKo;
	if(lupnp != NULL) {
		ms_mutex_lock(&lupnp->mutex);
		state = lupnp->state;
		ms_mutex_unlock(&lupnp->mutex);
	}
	return state;
}

bool_t _linphone_upnp_context_is_ready_for_register(UpnpContext *lupnp) {
	bool_t ready = TRUE;

	// 1 Check global uPnP state
	ready = (lupnp->state == LinphoneUpnpStateOk);

	// 2 Check external ip address
	if(ready) {
		if (upnp_igd_get_external_ipaddress(lupnp->upnp_igd_ctxt) == NULL) {
			ready = FALSE;
		}
	}

	// 3 Check sip ports bindings
	if(ready) {
		if(lupnp->sip_udp != NULL) {
			if(lupnp->sip_udp->state != LinphoneUpnpStateOk) {
				ready = FALSE;
			}
		} else if(lupnp->sip_tcp != NULL) {
			if(lupnp->sip_tcp->state != LinphoneUpnpStateOk) {
				ready = FALSE;
			}
		} else if(lupnp->sip_tls != NULL) {
			if(lupnp->sip_tls->state != LinphoneUpnpStateOk) {
				ready = FALSE;
			}
		} else {
			ready = FALSE;
		}
	}

	return ready;
}

bool_t linphone_upnp_context_is_ready_for_register(UpnpContext *lupnp) {
	bool_t ready = FALSE;
	if(lupnp != NULL) {
		ms_mutex_lock(&lupnp->mutex);
		ready = _linphone_upnp_context_is_ready_for_register(lupnp);
		ms_mutex_unlock(&lupnp->mutex);
	}
	return ready;
}

int linphone_upnp_context_get_external_port(UpnpContext *lupnp) {
	int port = -1;
	if(lupnp != NULL) {
		ms_mutex_lock(&lupnp->mutex);

		if(lupnp->sip_udp != NULL) {
			if(lupnp->sip_udp->state == LinphoneUpnpStateOk) {
				port = lupnp->sip_udp->external_port;
			}
		} else if(lupnp->sip_tcp != NULL) {
			if(lupnp->sip_tcp->state == LinphoneUpnpStateOk) {
				port = lupnp->sip_tcp->external_port;
			}
		} else if(lupnp->sip_tls != NULL) {
			if(lupnp->sip_tls->state == LinphoneUpnpStateOk) {
				port = lupnp->sip_tls->external_port;
			}
		}

		ms_mutex_unlock(&lupnp->mutex);
	}
	return port;
}

bool_t linphone_upnp_is_blacklisted(UpnpContext *lupnp) {
	const char * device_model_name = upnp_igd_get_device_model_name(lupnp->upnp_igd_ctxt);
	const char * device_model_number = upnp_igd_get_device_model_number(lupnp->upnp_igd_ctxt);
	const char * blacklist = lp_config_get_string(lupnp->lc->config, "net", "upnp_blacklist", NULL);
	bool_t blacklisted = FALSE;
	char *str;
	char *pch;
	char *model_name;
	char *model_number;

	// Sanity checks
	if(device_model_name == NULL || device_model_number == NULL || blacklist == NULL) {
		return FALSE;
	}

	// Find in the list
	str = strdup(blacklist);
	pch = strtok(str, ";");
	while (pch != NULL && !blacklisted) {
		// Extract model name & number
		model_name = pch;
		model_number = strstr(pch, ",");
		if(model_number != NULL) {
			*(model_number++) = '\0';
		}

		// Compare with current device
		if(strcmp(model_name, device_model_name) == 0) {
			if(model_number == NULL || strcmp(model_number, device_model_number) == 0) {
				blacklisted = TRUE;
			}
		}
		pch = strtok(NULL, ";");
	}
	free(str);

	return blacklisted;
}

void linphone_upnp_refresh(UpnpContext * lupnp) {
	upnp_igd_refresh(lupnp->upnp_igd_ctxt);
}

const char* linphone_upnp_context_get_external_ipaddress(UpnpContext *lupnp) {
	const char* addr = NULL;
	if(lupnp != NULL) {
		ms_mutex_lock(&lupnp->mutex);
		addr = upnp_igd_get_external_ipaddress(lupnp->upnp_igd_ctxt);
		ms_mutex_unlock(&lupnp->mutex);
	}
	return addr;
}

int linphone_upnp_context_send_add_port_binding(UpnpContext *lupnp, UpnpPortBinding *port, bool_t retry) {
	upnp_igd_port_mapping mapping;
	char description[128];
	int ret;

	if(lupnp->state != LinphoneUpnpStateOk) {
		return -2;
	}

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

	// No retry if specified
	if(port->retry != 0 && !retry) {
		return -1;
	}

	if(port->retry >= UPNP_ADD_MAX_RETRY) {
		ret = -1;
	} else {
		linphone_upnp_port_binding_set_device_id(port, upnp_igd_get_device_id(lupnp->upnp_igd_ctxt));
		mapping.cookie = linphone_upnp_port_binding_retain(port);
		lupnp->pending_bindings = bctbx_list_append(lupnp->pending_bindings, mapping.cookie);

		mapping.local_port = port->local_port;
		mapping.local_host = port->local_addr;
		if(port->external_port == -1)
			port->external_port = rand()%(0xffff - 1024) + 1024;
		mapping.remote_port = port->external_port;
		mapping.remote_host = "";
		snprintf(description, 128, "%s %s at %s:%d",
				"Linphone",
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

int linphone_upnp_context_send_remove_port_binding(UpnpContext *lupnp, UpnpPortBinding *port, bool_t retry) {
	upnp_igd_port_mapping mapping;
	int ret;

	if(lupnp->state != LinphoneUpnpStateOk) {
		return -2;
	}

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

	// No retry if specified
	if(port->retry != 0 && !retry) {
		return 1;
	}

	if(port->retry >= UPNP_REMOVE_MAX_RETRY) {
		ret = -1;
	} else {
		linphone_upnp_port_binding_set_device_id(port, upnp_igd_get_device_id(lupnp->upnp_igd_ctxt));
		mapping.cookie = linphone_upnp_port_binding_retain(port);
		lupnp->pending_bindings = bctbx_list_append(lupnp->pending_bindings, mapping.cookie);

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

	if(lupnp == NULL) {
		return ret;
	}

	ms_mutex_lock(&lupnp->mutex);

	// Don't handle when the call
	if(lupnp->state == LinphoneUpnpStateOk && call->upnp_session != NULL) {
		ret = 0;

		/*
		 * Audio part
		 */
		linphone_upnp_update_port_binding(lupnp, &call->upnp_session->audio->rtp,
			UPNP_IGD_IP_PROTOCOL_UDP, (audio)? call->media_ports[call->main_audio_stream_index].rtp_port:0, UPNP_CALL_RETRY_DELAY);

		linphone_upnp_update_port_binding(lupnp, &call->upnp_session->audio->rtcp,
			UPNP_IGD_IP_PROTOCOL_UDP, (audio)? call->media_ports[call->main_audio_stream_index].rtcp_port:0, UPNP_CALL_RETRY_DELAY);

		/*
		 * Video part
		 */
		linphone_upnp_update_port_binding(lupnp, &call->upnp_session->video->rtp,
			UPNP_IGD_IP_PROTOCOL_UDP, (video)? call->media_ports[call->main_video_stream_index].rtp_port:0, UPNP_CALL_RETRY_DELAY);

		linphone_upnp_update_port_binding(lupnp, &call->upnp_session->video->rtcp,
			UPNP_IGD_IP_PROTOCOL_UDP, (video)? call->media_ports[call->main_video_stream_index].rtcp_port:0, UPNP_CALL_RETRY_DELAY);
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

	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		stream = &md->streams[i];
		if (!sal_stream_description_active(stream)) continue;
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

void linphone_core_update_upnp_state_in_call_stats(LinphoneCall *call) {
	call->stats[LINPHONE_CALL_STATS_AUDIO].upnp_state = call->upnp_session->audio->state;
	call->stats[LINPHONE_CALL_STATS_VIDEO].upnp_state = call->upnp_session->video->state;
}

void linphone_upnp_update_stream_state(UpnpStream *stream) {
	if((stream->rtp == NULL || stream->rtp->state == LinphoneUpnpStateOk || stream->rtp->state == LinphoneUpnpStateIdle) &&
	   (stream->rtcp == NULL || stream->rtcp->state == LinphoneUpnpStateOk || stream->rtcp->state == LinphoneUpnpStateIdle)) {
		stream->state = LinphoneUpnpStateOk;
	} else if((stream->rtp != NULL &&
	   (stream->rtp->state == LinphoneUpnpStateAdding || stream->rtp->state == LinphoneUpnpStateRemoving)) ||
		  (stream->rtcp != NULL &&
			 (stream->rtcp->state == LinphoneUpnpStateAdding || stream->rtcp->state == LinphoneUpnpStateRemoving))) {
		stream->state = LinphoneUpnpStatePending;
	} else if((stream->rtp != NULL && stream->rtp->state == LinphoneUpnpStateKo) ||
			(stream->rtcp != NULL && stream->rtcp->state == LinphoneUpnpStateKo)) {
		stream->state = LinphoneUpnpStateKo;
	} else {
		ms_error("Invalid stream %p state", stream);
	}
}

int linphone_upnp_call_process(LinphoneCall *call) {
	LinphoneCore *lc = call->core;
	UpnpContext *lupnp = lc->upnp;
	int ret = -1;
	LinphoneUpnpState oldState = 0, newState = 0;

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
		linphone_upnp_update_stream_state(call->upnp_session->audio);

		/*
		 * Update Video state
		 */
		linphone_upnp_update_stream_state(call->upnp_session->video);

		/*
		 * Update stat
		 */
		linphone_core_update_upnp_state_in_call_stats(call);

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
		newState = call->upnp_session->state;
	}

	ms_mutex_unlock(&lupnp->mutex);

	/* When change is done proceed update */
	if(oldState != LinphoneUpnpStateOk && oldState != LinphoneUpnpStateKo &&
			(newState == LinphoneUpnpStateOk || newState == LinphoneUpnpStateKo)) {
		if(call->upnp_session->state == LinphoneUpnpStateOk)
			ms_message("uPnP IGD: uPnP for Call %p is ok", call);
		else
			ms_message("uPnP IGD: uPnP for Call %p is ko", call);

		switch (call->state) {
			case LinphoneCallUpdating:
				linphone_core_start_update_call(lc, call);
				break;
			case LinphoneCallUpdatedByRemote:
				linphone_core_start_accept_call_update(lc, call,call->prevstate,linphone_call_state_to_string(call->prevstate));
				break;
			case LinphoneCallOutgoingInit:
				linphone_core_proceed_with_invite_if_ready(lc, call, NULL);
				break;
			case LinphoneCallIdle:
				linphone_call_update_local_media_description_from_ice_or_upnp(call);
				sal_call_set_local_media_description(call->op,call->localdesc);
				linphone_core_notify_incoming_call(lc, call);
				break;
			default:
				break;
		}
	}

	return ret;
}

static const char *linphone_core_upnp_get_charptr_null(const char *str) {
	if(str != NULL) {
		return str;
	}
	return "(Null)";
}

void linphone_upnp_update(UpnpContext *lupnp) {
	bctbx_list_t *global_list = NULL;
	bctbx_list_t *list = NULL;
	bctbx_list_t *item;
	LinphoneCall *call;
	UpnpPortBinding *port_mapping, *port_mapping2;

	ms_message("uPnP IGD: Name:%s", linphone_core_upnp_get_charptr_null(upnp_igd_get_device_name(lupnp->upnp_igd_ctxt)));
	ms_message("uPnP IGD: Device:%s %s",
				   linphone_core_upnp_get_charptr_null(upnp_igd_get_device_model_name(lupnp->upnp_igd_ctxt)),
			   linphone_core_upnp_get_charptr_null(upnp_igd_get_device_model_number(lupnp->upnp_igd_ctxt)));
	ms_message("uPnP IGD: Refresh mappings");

	if(lupnp->sip_udp != NULL) {
		global_list = bctbx_list_append(global_list, lupnp->sip_udp);
	}
	if(lupnp->sip_tcp != NULL) {
		global_list = bctbx_list_append(global_list, lupnp->sip_tcp);
	}
	if(lupnp->sip_tls != NULL) {
		global_list = bctbx_list_append(global_list, lupnp->sip_tls);
	}

	list = lupnp->lc->calls;
	while(list != NULL) {
		call = (LinphoneCall *)list->data;
		if(call->upnp_session != NULL) {
			if(call->upnp_session->audio->rtp != NULL) {
				global_list = bctbx_list_append(global_list, call->upnp_session->audio->rtp);
			}
			if(call->upnp_session->audio->rtcp != NULL) {
				global_list = bctbx_list_append(global_list, call->upnp_session->audio->rtcp);
			}
			if(call->upnp_session->video->rtp != NULL) {
				global_list = bctbx_list_append(global_list, call->upnp_session->video->rtp);
			}
			if(call->upnp_session->video->rtcp != NULL) {
				global_list = bctbx_list_append(global_list, call->upnp_session->video->rtcp);
			}
		}
		list = list->next;
	}

	list = linphone_upnp_config_list_port_bindings(lupnp->lc->config, upnp_igd_get_device_id(lupnp->upnp_igd_ctxt));
	for(item = list;item != NULL; item = item->next) {
			port_mapping = (UpnpPortBinding *)item->data;
			port_mapping2 = linphone_upnp_port_binding_equivalent_in_list(global_list, port_mapping);
			if(port_mapping2 == NULL) {
				linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping, TRUE);
			} else if(port_mapping2->state == LinphoneUpnpStateIdle){
				/* Force to remove */
				port_mapping2->state = LinphoneUpnpStateOk;
			}
	}
	bctbx_list_for_each(list, (void (*)(void*))linphone_upnp_port_binding_release);
	list = bctbx_list_free(list);


	// (Re)Add removed port bindings
	list = global_list;
	while(list != NULL) {
		port_mapping = (UpnpPortBinding *)list->data;
		linphone_upnp_context_send_remove_port_binding(lupnp, port_mapping, TRUE);
		linphone_upnp_context_send_add_port_binding(lupnp, port_mapping, TRUE);
		list = list->next;
	}
	global_list = bctbx_list_free(global_list);
}

void linphone_upnp_update_port_binding(UpnpContext *lupnp, UpnpPortBinding **port_mapping, upnp_igd_ip_protocol protocol, int port, int retry_delay) {
	const char *local_addr, *external_addr;
	time_t now = time(NULL);
	if(port != 0) {
		if(*port_mapping != NULL) {
			if(port != (*port_mapping)->local_port) {
				linphone_upnp_context_send_remove_port_binding(lupnp, *port_mapping, FALSE);
				*port_mapping = NULL;
			}
		}
		if(*port_mapping == NULL) {
			*port_mapping = linphone_upnp_port_binding_new_or_collect(lupnp->pending_bindings, protocol, port, port);
		}

		// Get addresses
		local_addr = upnp_igd_get_local_ipaddress(lupnp->upnp_igd_ctxt);
		external_addr = upnp_igd_get_external_ipaddress(lupnp->upnp_igd_ctxt);

		// Force binding update on local address change
		if(local_addr != NULL) {
			if(strncmp((*port_mapping)->local_addr, local_addr, sizeof((*port_mapping)->local_addr))) {
				linphone_upnp_context_send_remove_port_binding(lupnp, *port_mapping, FALSE);
				strncpy((*port_mapping)->local_addr, local_addr, sizeof((*port_mapping)->local_addr));
			}
		}
		if(external_addr != NULL) {
			strncpy((*port_mapping)->external_addr, external_addr, sizeof((*port_mapping)->external_addr));
		}

		// Add (if not already done) the binding
		if(now - (*port_mapping)->last_update >= retry_delay) {
			(*port_mapping)->last_update = now;
			linphone_upnp_context_send_add_port_binding(lupnp, *port_mapping, FALSE);
		}
	} else {
		if(*port_mapping != NULL) {
			linphone_upnp_context_send_remove_port_binding(lupnp, *port_mapping, FALSE);
			*port_mapping = NULL;
		}
	}
}

void linphone_upnp_update_config(UpnpContext* lupnp) {
	char key[64];
	const bctbx_list_t *item;
	UpnpPortBinding *port_mapping;

	/* Add configs */
	for(item = lupnp->adding_configs;item!=NULL;item=item->next) {
		port_mapping = (UpnpPortBinding *)item->data;
		snprintf(key, sizeof(key), "%s-%s-%d-%d",
					port_mapping->device_id,
					(port_mapping->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
					port_mapping->external_port,
					port_mapping->local_port);
		lp_config_set_string(lupnp->lc->config, UPNP_SECTION_NAME, key, "uPnP");
		linphone_upnp_port_binding_log(ORTP_DEBUG, "Configuration: Added port binding", port_mapping);
	}
	bctbx_list_for_each(lupnp->adding_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->adding_configs = bctbx_list_free(lupnp->adding_configs);

	/* Remove configs */
	for(item = lupnp->removing_configs;item!=NULL;item=item->next) {
		port_mapping = (UpnpPortBinding *)item->data;
		snprintf(key, sizeof(key), "%s-%s-%d-%d",
					port_mapping->device_id,
					(port_mapping->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
					port_mapping->external_port,
					port_mapping->local_port);
		lp_config_set_string(lupnp->lc->config, UPNP_SECTION_NAME, key, NULL);
		linphone_upnp_port_binding_log(ORTP_DEBUG, "Configuration: Removed port binding", port_mapping);
	}
	bctbx_list_for_each(lupnp->removing_configs,(void (*)(void*))linphone_upnp_port_binding_release);
	lupnp->removing_configs = bctbx_list_free(lupnp->removing_configs);
}

void linphone_upnp_update_proxy(UpnpContext* lupnp, bool_t force) {
	LinphoneUpnpState ready_state;
	const bctbx_list_t *item;
	time_t now = (force)? (lupnp->last_ready_check + UPNP_CORE_READY_CHECK) : time(NULL);

	/* Refresh registers if we are ready */
	if(now - lupnp->last_ready_check >= UPNP_CORE_READY_CHECK) {
		lupnp->last_ready_check = now;
		ready_state = (_linphone_upnp_context_is_ready_for_register(lupnp))? LinphoneUpnpStateOk: LinphoneUpnpStateKo;
		if(ready_state != lupnp->last_ready_state) {
			for(item=linphone_core_get_proxy_config_list(lupnp->lc);item!=NULL;item=item->next) {
				LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)item->data;
				if (linphone_proxy_config_register_enabled(cfg)) {
					if (ready_state != LinphoneUpnpStateOk) {
						// Only reset ithe registration if we require that upnp should be ok
						if(lupnp->lc->sip_conf.register_only_when_upnp_is_ok) {
							linphone_proxy_config_set_state(cfg, LinphoneRegistrationNone, "Registration impossible (uPnP not ready)");
						} else {
							cfg->commit=TRUE;
						}
					} else {
						cfg->commit=TRUE;
					}
				}
			}
			lupnp->last_ready_state = ready_state;
		}
	}
}

bool_t linphone_core_upnp_hook(void *data) {
	LCSipTransports transport;
	UpnpContext *lupnp = (UpnpContext *)data;

	ms_mutex_lock(&lupnp->mutex);

	/* Update ports */
	if(lupnp->state == LinphoneUpnpStateOk) {
		linphone_core_get_sip_transports(lupnp->lc, &transport);
		linphone_upnp_update_port_binding(lupnp, &lupnp->sip_udp, UPNP_IGD_IP_PROTOCOL_UDP, transport.udp_port, UPNP_CORE_RETRY_DELAY);
		linphone_upnp_update_port_binding(lupnp, &lupnp->sip_tcp, UPNP_IGD_IP_PROTOCOL_TCP, transport.tcp_port, UPNP_CORE_RETRY_DELAY);
		linphone_upnp_update_port_binding(lupnp, &lupnp->sip_tls, UPNP_IGD_IP_PROTOCOL_TCP, transport.tls_port, UPNP_CORE_RETRY_DELAY);
	}

	linphone_upnp_update_proxy(lupnp, FALSE);
	linphone_upnp_update_config(lupnp);

	ms_mutex_unlock(&lupnp->mutex);
	return TRUE;
}

int linphone_core_update_local_media_description_from_upnp(SalMediaDescription *desc, UpnpSession *session) {
	int i;
	SalStreamDescription *stream;
	UpnpStream *upnpStream;

	for (i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		stream = &desc->streams[i];
		if (!sal_stream_description_active(stream)) continue;
		upnpStream = NULL;
		if(stream->type == SalAudio) {
			upnpStream = session->audio;
		} else if(stream->type == SalVideo) {
			upnpStream = session->video;
		}
		if(upnpStream != NULL) {
			if(upnpStream->rtp != NULL && upnpStream->rtp->state == LinphoneUpnpStateOk) {
				strncpy(stream->rtp_addr, upnpStream->rtp->external_addr, LINPHONE_IPADDR_SIZE);
				stream->rtp_port = upnpStream->rtp->external_port;
			}
			if(upnpStream->rtcp != NULL && upnpStream->rtcp->state == LinphoneUpnpStateOk) {
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

UpnpPortBinding *linphone_upnp_port_binding_new(void) {
	UpnpPortBinding *port = NULL;
	port = ms_new0(UpnpPortBinding,1);
	ms_mutex_init(&port->mutex, NULL);
	port->state = LinphoneUpnpStateIdle;
	port->protocol = UPNP_IGD_IP_PROTOCOL_UDP;
	port->device_id = NULL;
	port->local_addr[0] = '\0';
	port->local_port = -1;
	port->external_addr[0] = '\0';
	port->external_port = -1;
	port->to_remove = FALSE;
	port->to_add = FALSE;
	port->ref = 1;
	port->last_update = 0;
	return port;
}

UpnpPortBinding *linphone_upnp_port_binding_new_with_parameters(upnp_igd_ip_protocol protocol, int local_port, int external_port) {
	UpnpPortBinding *port_binding = linphone_upnp_port_binding_new();
	port_binding->protocol = protocol;
	port_binding->local_port = local_port;
	port_binding->external_port = external_port;
	return port_binding;
}

UpnpPortBinding *linphone_upnp_port_binding_new_or_collect(bctbx_list_t *list, upnp_igd_ip_protocol protocol, int local_port, int external_port) {
	UpnpPortBinding *tmp_binding;
	UpnpPortBinding *end_binding;

	// Seek an binding with same protocol and local port
	end_binding = linphone_upnp_port_binding_new_with_parameters(protocol, local_port, -1);
	tmp_binding = linphone_upnp_port_binding_equivalent_in_list(list, end_binding);

	// Must be not attached to any struct
	if(tmp_binding != NULL && tmp_binding->ref == 1) {
		linphone_upnp_port_binding_release(end_binding);
		end_binding = linphone_upnp_port_binding_retain(tmp_binding);
	} else {
		end_binding->external_port = external_port;
	}
	return end_binding;
}

UpnpPortBinding *linphone_upnp_port_binding_copy(const UpnpPortBinding *port) {
	UpnpPortBinding *new_port = NULL;
	new_port = ms_new0(UpnpPortBinding,1);
	memcpy(new_port, port, sizeof(UpnpPortBinding));
	new_port->device_id = NULL;
	linphone_upnp_port_binding_set_device_id(new_port, port->device_id);
	ms_mutex_init(&new_port->mutex, NULL);
	new_port->ref = 1;
	return new_port;
}

void linphone_upnp_port_binding_set_device_id(UpnpPortBinding *port, const char *device_id) {
	char *formated_device_id = linphone_upnp_format_device_id(device_id);
	if(formated_device_id != NULL && port->device_id != NULL) {
		if(strcmp(formated_device_id, port->device_id) == 0) {
			ms_free(formated_device_id);
			return;
		}
	}
	if(port->device_id != NULL) {
		ms_free(port->device_id);
	}
	port->device_id = formated_device_id;
}

void linphone_upnp_port_binding_log(int level, const char *msg, const UpnpPortBinding *port) {
	if(strlen(port->local_addr)) {
		ortp_log(level, "uPnP IGD: %s %s|%d->%s:%d (retry %d)", msg,
							(port->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
									port->external_port,
									port->local_addr,
									port->local_port,
									port->retry - 1);
	} else {
		ortp_log(level, "uPnP IGD: %s %s|%d->%d (retry %d)", msg,
							(port->protocol == UPNP_IGD_IP_PROTOCOL_TCP)? "TCP":"UDP",
									port->external_port,
									port->local_port,
									port->retry - 1);
	}
}

// Return true if the binding are equivalent. (Note external_port == -1 means "don't care")
bool_t linphone_upnp_port_binding_equal(const UpnpPortBinding *port1, const UpnpPortBinding *port2) {
	return port1->protocol == port2->protocol &&
		   port1->local_port == port2->local_port &&
		   (port1->external_port == -1 || port2->external_port == -1 || port1->external_port == port2->external_port);
}

UpnpPortBinding *linphone_upnp_port_binding_equivalent_in_list(bctbx_list_t *list, const UpnpPortBinding *port) {
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
		if(port->device_id != NULL) {
			ms_free(port->device_id);
		}
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

UpnpStream* linphone_upnp_stream_new(void) {
	UpnpStream *stream = ms_new0(UpnpStream,1);
	stream->state = LinphoneUpnpStateIdle;
	stream->rtp = NULL;
	stream->rtcp = NULL;
	return stream;
}

void linphone_upnp_stream_destroy(UpnpStream* stream) {
	if(stream->rtp != NULL) {
		linphone_upnp_port_binding_release(stream->rtp);
		stream->rtp = NULL;
	}
	if(stream->rtcp != NULL) {
		linphone_upnp_port_binding_release(stream->rtcp);
		stream->rtcp = NULL;
	}
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
		if(session->audio->rtp != NULL) {
			linphone_upnp_context_send_remove_port_binding(lc->upnp, session->audio->rtp, TRUE);
		}
		if(session->audio->rtcp != NULL) {
			linphone_upnp_context_send_remove_port_binding(lc->upnp, session->audio->rtcp, TRUE);
		}
		if(session->video->rtp != NULL) {
			linphone_upnp_context_send_remove_port_binding(lc->upnp, session->video->rtp, TRUE);
		}
		if(session->video->rtcp != NULL) {
			linphone_upnp_context_send_remove_port_binding(lc->upnp, session->video->rtcp, TRUE);
		}
	}

	session->call->stats[LINPHONE_CALL_STATS_AUDIO].upnp_state = LinphoneUpnpStateKo;
	session->call->stats[LINPHONE_CALL_STATS_VIDEO].upnp_state = LinphoneUpnpStateKo;

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
	bctbx_list_t *retList;
	const char *device_id;
};

static void linphone_upnp_config_list_port_bindings_cb(const char *entry, struct linphone_upnp_config_list_port_bindings_struct *cookie) {
	char device_id[UPNP_UUID_LEN + 1];
	char protocol_str[4]; // TCP or UDP
	upnp_igd_ip_protocol protocol;
	int external_port;
	int local_port;
	int ret;
	bool_t valid = TRUE;
	UpnpPortBinding *port;

	ret = sscanf(entry, "%"UPNP_UUID_LEN_STR"[^-]-%3s-%i-%i", device_id, protocol_str, &external_port, &local_port);
	if(ret == 4) {
		// Handle only wanted device bindings
		if(device_id != NULL && strcmp(cookie->device_id, device_id) != 0) {
			return;
		}
		if(linphone_upnp_strncmpi(protocol_str, "TCP", 3) == 0) {
			protocol = UPNP_IGD_IP_PROTOCOL_TCP;
		} else if(linphone_upnp_strncmpi(protocol_str, "UDP", 3) == 0) {
			protocol = UPNP_IGD_IP_PROTOCOL_UDP;
		} else {
			valid = FALSE;
		}
		if(valid) {
			port = linphone_upnp_port_binding_new();
			linphone_upnp_port_binding_set_device_id(port, device_id);
			port->state = LinphoneUpnpStateOk;
			port->protocol = protocol;
			port->external_port = external_port;
			port->local_port = local_port;
			cookie->retList = bctbx_list_append(cookie->retList, port);
		}
	} else {
		valid = FALSE;
	}
	if(!valid) {
		ms_warning("uPnP configuration invalid line: %s", entry);
	}
}

bctbx_list_t *linphone_upnp_config_list_port_bindings(struct _LpConfig *lpc, const char *device_id) {
	char *formated_device_id = linphone_upnp_format_device_id(device_id);
	struct linphone_upnp_config_list_port_bindings_struct cookie = {lpc, NULL, formated_device_id};
	lp_config_for_each_entry(lpc, UPNP_SECTION_NAME, (void(*)(const char *, void*))linphone_upnp_config_list_port_bindings_cb, &cookie);
	ms_free(formated_device_id);
	return cookie.retList;
}

void linphone_upnp_config_add_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port) {
	bctbx_list_t *list;
	UpnpPortBinding *list_port;

	if(port->device_id == NULL) {
		ms_error("Can't remove port binding without device_id");
		return;
	}

	list = lupnp->removing_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			lupnp->removing_configs = bctbx_list_remove(lupnp->removing_configs, list_port);
			linphone_upnp_port_binding_release(list_port);
			return;
		}
		list = bctbx_list_next(list);
	}

	list = lupnp->adding_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			return;
		}
		list = bctbx_list_next(list);
	}

	list_port = linphone_upnp_port_binding_copy(port);
	lupnp->adding_configs = bctbx_list_append(lupnp->adding_configs, list_port);
}

void linphone_upnp_config_remove_port_binding(UpnpContext *lupnp, const UpnpPortBinding *port) {
	bctbx_list_t *list;
	UpnpPortBinding *list_port;

	if(port->device_id == NULL) {
		ms_error("Can't remove port binding without device_id");
		return;
	}

	list = lupnp->adding_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			lupnp->adding_configs = bctbx_list_remove(lupnp->adding_configs, list_port);
			linphone_upnp_port_binding_release(list_port);
			return;
		}
		list = bctbx_list_next(list);
	}

	list = lupnp->removing_configs;
	while(list != NULL) {
		list_port = (UpnpPortBinding *)list->data;
		if(linphone_upnp_port_binding_equal(list_port, port) == TRUE) {
			return;
		}
		list = bctbx_list_next(list);
	}

	list_port = linphone_upnp_port_binding_copy(port);
	lupnp->removing_configs = bctbx_list_append(lupnp->removing_configs, list_port);
}
