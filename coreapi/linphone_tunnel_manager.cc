/***************************************************************************
 *            linphone_tunnel_manager.cc
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


#include "linphone_tunnel_manager.h"
#include "TunnelManager.hh"
#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"

static inline belledonnecomm::TunnelManager *bcTunnel(LinphoneTunnelManager *tunnel){
	return (belledonnecomm::TunnelManager *)tunnel;
}

extern "C" LinphoneTunnelManager* linphone_core_tunnel_new(LinphoneCore *lc){
	LinphoneTunnelManager* tunnel= (LinphoneTunnelManager*) new belledonnecomm::TunnelManager(lc);
	return tunnel;
}

LinphoneTunnelManager* linphone_tunnel_get(LinphoneCore *lc){
	return lc->tunnel;
}

void linphone_tunnel_destroy(LinphoneTunnelManager *tunnel){
	delete bcTunnel(tunnel);
}

void linphone_tunnel_add_server(LinphoneTunnelManager *tunnel, const char *host, int port){
	bcTunnel(tunnel)->addServer(host, port);
}

void linphone_tunnel_add_server_and_mirror(LinphoneTunnelManager *tunnel, const char *host, int port, int remote_udp_mirror, int delay){
	bcTunnel(tunnel)->addServer(host, port, remote_udp_mirror, delay);
}

void linphone_tunnel_clean_servers(LinphoneTunnelManager *tunnel){
	bcTunnel(tunnel)->cleanServers();
}

void linphone_tunnel_enable(LinphoneTunnelManager *tunnel, bool_t enabled){
	bcTunnel(tunnel)->enable(enabled);
}

bool_t linphone_tunnel_enabled(LinphoneTunnelManager *tunnel){
	return bcTunnel(tunnel)->isEnabled();
}

void linphone_tunnel_enable_logs(LinphoneTunnelManager *tunnel, bool_t enabled){
	bcTunnel(tunnel)->enableLogs(enabled);
}

void linphone_tunnel_enable_logs_with_handler(LinphoneTunnelManager *tunnel, bool_t enabled, LogHandler logHandler){
	bcTunnel(tunnel)->enableLogs(enabled, logHandler);
}

void linphone_tunnel_set_http_proxy_auth_info(LinphoneTunnelManager *tunnel, const char* username,const char* passwd){
	bcTunnel(tunnel)->setHttpProxyAuthInfo(username, passwd);
}

void linphone_tunnel_reconnect(LinphoneTunnelManager *tunnel){
	bcTunnel(tunnel)->reconnect();
}

void linphone_tunnel_auto_detect(LinphoneTunnelManager *tunnel){
	bcTunnel(tunnel)->autoDetect();
}


static inline _LpConfig *config(LinphoneTunnelManager *tunnel){
	return ((belledonnecomm::TunnelManager *)tunnel)->getLinphoneCore()->config;
}

/**
 * Set tunnel server addresses. "host1:port1 host2:port2 host3:port3"
**/
void linphone_tunnel_set_server_addresses(LinphoneTunnelManager *tunnel, const char *addresses){
	lp_config_set_string(config(tunnel),"tunnel","server_addresses",addresses);
}

/**
 * Get tunnel server addresses. "host1:port1 host2:port2 host3:port3"
**/
const char *linphone_tunnel_get_server_addresses(LinphoneTunnelManager *tunnel){
	return lp_config_get_string(config(tunnel),"tunnel","server_addresses", NULL);
}

/**
 * Set tunnel state.
**/
void linphone_tunnel_set_state(LinphoneTunnelManager *tunnel, LinphoneTunnelState state){
	switch (state) {
		case LinphoneTunnelEnabled:
			lp_config_set_string(config(tunnel),"tunnel","tunnel_state","enabled");
			break;
		case LinphoneTunnelDisabled:
			lp_config_set_string(config(tunnel),"tunnel","tunnel_state","disabled");
			break;
		case LinphoneTunnelAuto:
			lp_config_set_string(config(tunnel),"tunnel","tunnel_state","auto");
			break;
	}
}

/**
 * Get tunnel state.
**/
LinphoneTunnelState linphone_tunnel_get_state(LinphoneTunnelManager *tunnel){
	const char *state=lp_config_get_string(config(tunnel),"tunnel","tunnel_state","disabled");
	if (0==strcmp("enabled", state)){
		return LinphoneTunnelEnabled;
	} else if (0==strcmp("auto", state)){
		return LinphoneTunnelAuto;
	} else {
		return LinphoneTunnelDisabled;
	}
}
