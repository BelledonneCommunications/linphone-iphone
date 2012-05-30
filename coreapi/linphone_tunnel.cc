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

#ifdef TUNNEL_ENABLED
#include "TunnelManager.hh"
#endif
#include "linphone_tunnel.h"
#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"


LinphoneTunnel* linphone_core_get_tunnel(LinphoneCore *lc){
	return lc->tunnel;
}

#ifdef TUNNEL_ENABLED

static inline belledonnecomm::TunnelManager *bcTunnel(LinphoneTunnel *tunnel){
	return (belledonnecomm::TunnelManager *)tunnel;
}

static inline _LpConfig *config(LinphoneTunnel *tunnel){
	return ((belledonnecomm::TunnelManager *)tunnel)->getLinphoneCore()->config;
}

extern "C" LinphoneTunnel* linphone_core_tunnel_new(LinphoneCore *lc){
	LinphoneTunnel* tunnel= (LinphoneTunnel*) new belledonnecomm::TunnelManager(lc);
	return tunnel;
}

void linphone_tunnel_destroy(LinphoneTunnel *tunnel){
	delete bcTunnel(tunnel);
}

static void add_server_to_config(LinphoneTunnel *tunnel, const char *host, int port){
	const char *orig=lp_config_get_string(config(tunnel),"tunnel","server_addresses", NULL);
	char *tmp;
	if (orig){
		tmp=ms_strdup_printf("%s %s:%i",orig,host,port);
	}else tmp=ms_strdup_printf("%s:%i",host, port);
	lp_config_set_string(config(tunnel),"tunnel","server_addresses",tmp);
	ms_free(tmp);
}

void linphone_tunnel_add_server(LinphoneTunnel *tunnel, const char *host, int port){
	bcTunnel(tunnel)->addServer(host, port);
	add_server_to_config(tunnel,host,port);
}

void linphone_tunnel_add_server_and_mirror(LinphoneTunnel *tunnel, const char *host, int port, int remote_udp_mirror, int delay){
	bcTunnel(tunnel)->addServer(host, port, remote_udp_mirror, delay);
	/*FIXME, udp-mirror feature not saved in config*/
	add_server_to_config(tunnel,host,port);
}

char *linphone_tunnel_get_servers(LinphoneTunnel *tunnel){
	const char *tmp=lp_config_get_string(config(tunnel),"tunnel","server_addresses",NULL);
	if (tmp) return ms_strdup(tmp);
	return NULL;
}

void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel){
	bcTunnel(tunnel)->cleanServers();
	lp_config_set_string(config(tunnel),"tunnel","server_addresses",NULL);
}

void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled){
	lp_config_set_int(config(tunnel),"tunnel","enabled",(int)enabled);
	bcTunnel(tunnel)->enable(enabled);
}

bool_t linphone_tunnel_enabled(LinphoneTunnel *tunnel){
	return bcTunnel(tunnel)->isEnabled();
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

static void tunnel_add_servers_from_config(LinphoneTunnel *tunnel, char* confaddress){
	char *str1;
	for(str1=confaddress;;str1=NULL){
		char *port;
		char *address=strtok(str1," "); // Not thread safe
		if (!address) break;
		port=strchr(address, ':');
		if (!port) ms_fatal("Bad tunnel address %s",confaddress);
		*port++='\0';
		linphone_tunnel_add_server(tunnel, address, atoi(port));
	}
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
	const char* addresses=lp_config_get_string(config(tunnel),"tunnel","server_addresses", NULL);
	char *copy=addresses ? ms_strdup(addresses) : NULL;
	linphone_tunnel_enable_logs_with_handler(tunnel,TRUE,my_ortp_logv);
	linphone_tunnel_clean_servers(tunnel);
	if (copy){
		tunnel_add_servers_from_config(tunnel,copy);
		ms_free(copy);
	}
	linphone_tunnel_enable(tunnel, enabled);
}

#else

/*stubs to avoid to have #ifdef TUNNEL_ENABLED in upper layers*/

void linphone_tunnel_destroy(LinphoneTunnel *tunnel){
}


void linphone_tunnel_add_server(LinphoneTunnel *tunnel, const char *host, int port){
}

void linphone_tunnel_add_server_and_mirror(LinphoneTunnel *tunnel, const char *host, int port, int remote_udp_mirror, int delay){
}

char *linphone_tunnel_get_servers(LinphoneTunnel *tunnel){
	return NULL;
}

void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel){
}

void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled){
}

bool_t linphone_tunnel_enabled(LinphoneTunnel *tunnel){
	return FALSE;
}


void linphone_tunnel_enable_logs_with_handler(LinphoneTunnel *tunnel, bool_t enabled, OrtpLogFunc logHandler){
}

void linphone_tunnel_set_http_proxy_auth_info(LinphoneTunnel *tunnel, const char* username,const char* passwd){
}

void linphone_tunnel_set_http_proxy(LinphoneTunnel*tunnel, const char *host, int port, const char* username,const char* passwd){
}

void linphone_tunnel_get_http_proxy(LinphoneTunnel*tunnel,const char **host, int *port, const char **username, const char **passwd){
}

void linphone_tunnel_reconnect(LinphoneTunnel *tunnel){
}

void linphone_tunnel_auto_detect(LinphoneTunnel *tunnel){
}

void linphone_tunnel_configure(LinphoneTunnel *tunnel){
}


#endif





