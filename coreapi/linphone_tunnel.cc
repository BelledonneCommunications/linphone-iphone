/***************************************************************************
 *            linphone_tunnel.cc
 *
 *  Fri Dec 9, 2011
 *  Copyright  2011  Belledonne Communications
 *  Author: Guillaume Beraudo
 *  Email: guillaume dot beraudo at linphone dot org
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

#include "TunnelManager.hh"
#include "linphone_tunnel.h"
#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"

LinphoneTunnel* linphone_core_get_tunnel(LinphoneCore *lc){
	return lc->tunnel;
}

struct _LinphoneTunnel {
	belledonnecomm::TunnelManager *manager;
	MSList *config_list;
};

extern "C" LinphoneTunnel* linphone_core_tunnel_new(LinphoneCore *lc){
	LinphoneTunnel* tunnel = ms_new0(LinphoneTunnel, 1);
	tunnel->manager = new belledonnecomm::TunnelManager(lc);
	return tunnel;
}

static inline belledonnecomm::TunnelManager *bcTunnel(LinphoneTunnel *tunnel){
	return tunnel->manager;
}

static inline _LpConfig *config(LinphoneTunnel *tunnel){
	return tunnel->manager->getLinphoneCore()->config;
}

void linphone_tunnel_destroy(LinphoneTunnel *tunnel){
	delete tunnel->manager;
	ms_free(tunnel);
}

static char *linphone_tunnel_config_to_string(const LinphoneTunnelConfig *tunnel_config) {
	char *str = NULL;
	if(linphone_tunnel_config_get_remote_udp_mirror_port(tunnel_config) != -1) {
		str = ms_strdup_printf("%s:%d:%d:%d", 
			linphone_tunnel_config_get_host(tunnel_config),
			linphone_tunnel_config_get_port(tunnel_config),
			linphone_tunnel_config_get_remote_udp_mirror_port(tunnel_config),
			linphone_tunnel_config_get_delay(tunnel_config));
	} else {
		str = ms_strdup_printf("%s:%d",
			linphone_tunnel_config_get_host(tunnel_config),
			linphone_tunnel_config_get_port(tunnel_config));
	}
	return str;
}

static LinphoneTunnelConfig *linphone_tunnel_config_from_string(const char *str) {
	LinphoneTunnelConfig *tunnel_config = NULL;
	char * dstr = ms_strdup(str);
	const char *host = NULL;
	int port = -1;
	int remote_udp_mirror_port = -1;
	int delay = -1;
	int pos = 0;
	char *pch;
	pch = strtok(dstr, ":");
	while(pch != NULL) {
		switch(pos) {
		case 0:
			host = pch;
			break;
		case 1:
			port = atoi(pch);
			break;
		case 2:
			remote_udp_mirror_port = atoi(pch);
			break;
		case 3:
			delay = atoi(pch);
			break;	
		default:
			// Abort
			pos = 0;
			break;
			
		}
		++pos;
		pch = strtok(NULL, ":");
	}
	if(pos >= 2) {
		tunnel_config = linphone_tunnel_config_new();
		linphone_tunnel_config_set_host(tunnel_config, host);
		linphone_tunnel_config_set_port(tunnel_config, port);
	}
	if(pos >= 3) {
		linphone_tunnel_config_set_remote_udp_mirror_port(tunnel_config, remote_udp_mirror_port);
	}
	if(pos == 4) {
		linphone_tunnel_config_set_delay(tunnel_config, delay);
	}
	ms_free(dstr);	
	return tunnel_config;
}


static void linphone_tunnel_save_config(LinphoneTunnel *tunnel) {
	MSList *elem = tunnel->config_list;
	char *tmp = NULL, *old_tmp = NULL, *tc_str = NULL;
	while(elem != NULL) {
		LinphoneTunnelConfig *tunnel_config = (LinphoneTunnelConfig *)elem->data;
		tc_str = linphone_tunnel_config_to_string(tunnel_config);
		if(tmp != NULL) {
			old_tmp = tmp;
			tmp = ms_strdup_printf("%s %s", old_tmp, tc_str);
			ms_free(old_tmp);
			ms_free(tc_str);
		} else {
			tmp = tc_str;
		}
		elem = elem->next;
	}
	lp_config_set_string(config(tunnel), "tunnel", "server_addresses", tmp);
	if(tmp != NULL) {
		ms_free(tmp);
	}
}


static void linphone_tunnel_add_server_intern(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config) {
	if(linphone_tunnel_config_get_remote_udp_mirror_port(tunnel_config) == -1) {
		bcTunnel(tunnel)->addServer(linphone_tunnel_config_get_host(tunnel_config), 
			linphone_tunnel_config_get_port(tunnel_config));
	} else {
		bcTunnel(tunnel)->addServer(linphone_tunnel_config_get_host(tunnel_config), 
			linphone_tunnel_config_get_port(tunnel_config), 
			linphone_tunnel_config_get_remote_udp_mirror_port(tunnel_config), 
			linphone_tunnel_config_get_delay(tunnel_config));
	}
	tunnel->config_list = ms_list_append(tunnel->config_list, tunnel_config);
}


static void linphone_tunnel_load_config(LinphoneTunnel *tunnel){
	const char * confaddress = lp_config_get_string(config(tunnel), "tunnel", "server_addresses", NULL);
	char *tmp;
	const char *it;
	LinphoneTunnelConfig *tunnel_config;
	int adv;
	if(confaddress != NULL) {
		tmp = ms_strdup(confaddress);
		it = confaddress;
		while(confaddress[0] != '\0') {
			int ret = sscanf(it,"%s%n", tmp, &adv);
			if (ret >= 1){
				it += adv;
				tunnel_config = linphone_tunnel_config_from_string(tmp);
				if(tunnel_config != NULL) {
					linphone_tunnel_add_server_intern(tunnel, tunnel_config);
				} else {
					ms_error("Tunnel server address incorrectly specified from config file: %s", tmp);
				}
			} else break;
		}
		ms_free(tmp);
	}
}

static void linphone_tunnel_refresh_config(LinphoneTunnel *tunnel) {
	MSList *old_list = tunnel->config_list;
	tunnel->config_list = NULL;
	bcTunnel(tunnel)->cleanServers();
	while(old_list != NULL) {
		LinphoneTunnelConfig *tunnel_config = (LinphoneTunnelConfig *)old_list->data;
		linphone_tunnel_add_server_intern(tunnel, tunnel_config);
		old_list = old_list->next;
	}
}

void linphone_tunnel_add_server(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config) {
	linphone_tunnel_add_server_intern(tunnel, tunnel_config);
	linphone_tunnel_save_config(tunnel);
}

void linphone_tunnel_remove_server(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config) {
	MSList *elem = ms_list_find(tunnel->config_list, tunnel_config);
	if(elem != NULL) {
		tunnel->config_list = ms_list_remove(tunnel->config_list, tunnel_config);
		linphone_tunnel_config_destroy(tunnel_config);		
		linphone_tunnel_refresh_config(tunnel);
		linphone_tunnel_save_config(tunnel);
	}	
}

const MSList *linphone_tunnel_get_servers(LinphoneTunnel *tunnel){
	return tunnel->config_list;
}

void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel){
	bcTunnel(tunnel)->cleanServers();
	
	/* Free the list */
	ms_list_for_each(tunnel->config_list, (void (*)(void *))linphone_tunnel_config_destroy); 
	tunnel->config_list = ms_list_free(tunnel->config_list);
	
	linphone_tunnel_save_config(tunnel);
}

void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled){
	lp_config_set_int(config(tunnel),"tunnel","enabled",(int)enabled);
	bcTunnel(tunnel)->enable(enabled);
}

bool_t linphone_tunnel_enabled(LinphoneTunnel *tunnel){
	return bcTunnel(tunnel)->isEnabled();
}

bool_t linphone_tunnel_connected(LinphoneTunnel *tunnel){
	return bcTunnel(tunnel)->isReady();
}

static OrtpLogFunc tunnelOrtpLogHandler=NULL;

/*
#define TUNNEL_DEBUG (1)
#define TUNNEL_INFO  (1<<1)
#define TUNNEL_NOTICE (1<<2)
#define TUNNEL_WARN  (1<<3)
#define TUNNEL_ERROR (1<<4)
#define TUNNEL_ALERT (1<<5)
#define TUNNEL_FATAL (1<<6)
*/

static void tunnelLogHandler(int level, const char *fmt, va_list l){
	if (tunnelOrtpLogHandler){
		OrtpLogLevel ortp_level=ORTP_DEBUG;
		switch(level){
			case TUNNEL_DEBUG:
				ortp_level=ORTP_DEBUG;
			break;
			case TUNNEL_INFO:
				ortp_level=ORTP_MESSAGE;
			break;
			case TUNNEL_NOTICE:
				ortp_level=ORTP_MESSAGE;
			break;
			case TUNNEL_WARN:
				ortp_level=ORTP_WARNING;
			break;
			case TUNNEL_ERROR:
				ortp_level=ORTP_ERROR;
			break;
			case TUNNEL_ALERT:
				ortp_level=ORTP_ERROR;
			break;
			case TUNNEL_FATAL:
				ortp_level=ORTP_FATAL;
			break;
			default:
				ms_fatal("Unexepcted tunnel log %i: %s",level,fmt);
			break;
		}
		tunnelOrtpLogHandler(ortp_level,fmt,l);
	}
}

void linphone_tunnel_enable_logs_with_handler(LinphoneTunnel *tunnel, bool_t enabled, OrtpLogFunc logHandler){
	tunnelOrtpLogHandler=logHandler;
	bcTunnel(tunnel)->enableLogs(enabled, tunnelLogHandler);
}

void linphone_tunnel_set_http_proxy_auth_info(LinphoneTunnel *tunnel, const char* username,const char* passwd){
	bcTunnel(tunnel)->setHttpProxyAuthInfo(username, passwd);
}

void linphone_tunnel_set_http_proxy(LinphoneTunnel*tunnel, const char *host, int port, const char* username,const char* passwd){
	bcTunnel(tunnel)->setHttpProxy(host, port, username, passwd);
	lp_config_set_string(config(tunnel),"tunnel","http_proxy_host",host);
	lp_config_set_int(config(tunnel),"tunnel","http_proxy_port",port);
	lp_config_set_string(config(tunnel),"tunnel","http_proxy_username",username);
	lp_config_set_string(config(tunnel),"tunnel","http_proxy_password",passwd);
}

void linphone_tunnel_get_http_proxy(LinphoneTunnel*tunnel,const char **host, int *port, const char **username, const char **passwd){
	if (host) *host=lp_config_get_string(config(tunnel),"tunnel","http_proxy_host",NULL);
	if (port) *port=lp_config_get_int(config(tunnel),"tunnel","http_proxy_port",0);
	if (username) *username=lp_config_get_string(config(tunnel),"tunnel","http_proxy_username",NULL);
	if (passwd) *passwd=lp_config_get_string(config(tunnel),"tunnel","http_proxy_password",NULL);
}

void linphone_tunnel_reconnect(LinphoneTunnel *tunnel){
	bcTunnel(tunnel)->reconnect();
}

void linphone_tunnel_auto_detect(LinphoneTunnel *tunnel){
	bcTunnel(tunnel)->autoDetect();
}

static void my_ortp_logv(OrtpLogLevel level, const char *fmt, va_list args){
	ortp_logv(level,fmt,args);
}

/**
 * Startup tunnel using configuration.
 * Called internally from linphonecore at startup.
 */
void linphone_tunnel_configure(LinphoneTunnel *tunnel){
	bool_t enabled=(bool_t)lp_config_get_int(config(tunnel),"tunnel","enabled",FALSE);
	linphone_tunnel_enable_logs_with_handler(tunnel,TRUE,my_ortp_logv);
	linphone_tunnel_load_config(tunnel);
	linphone_tunnel_enable(tunnel, enabled);
}

