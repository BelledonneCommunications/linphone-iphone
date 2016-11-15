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

#include "linphone/tunnel.h"
#include "linphone/core.h"
#include "private.h"
#include "linphone/lpconfig.h"


LinphoneTunnel* linphone_core_get_tunnel(const LinphoneCore *lc){
	return lc->tunnel;
}

/*stubs to avoid to have #ifdef TUNNEL_ENABLED in upper layers*/

void linphone_tunnel_destroy(LinphoneTunnel *tunnel){
}


void linphone_tunnel_add_server(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config){
	ms_warning("linphone_tunnel_add_server() - stubbed, no implementation");
}

void linphone_tunnel_remove_server(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config){
	ms_warning("linphone_tunnel_remove_server() - stubbed, no implementation");
}

const MSList *linphone_tunnel_get_servers(const LinphoneTunnel *tunnel){
	ms_warning("linphone_tunnel_get_servers() - stubbed, no implementation");
	return NULL;
}

void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel){
	ms_warning("linphone_tunnel_clean_servers() - stubbed, no implementation");
}

void linphone_tunnel_set_mode(LinphoneTunnel *tunnel, LinphoneTunnelMode mode) {
	ms_warning("linphone_tunnel_set_mode() - stubbed, no implementation");
}

LinphoneTunnelMode linphone_tunnel_get_mode(const LinphoneTunnel *tunnel){
	ms_warning("linphone_tunnel_get_mode() - stubbed, no implementation");
	return LinphoneTunnelModeDisable;
}

bool_t linphone_tunnel_connected(const LinphoneTunnel *tunnel){
	return FALSE;
}

bool_t linphone_tunnel_get_activated(const LinphoneTunnel *tunnel){
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

void linphone_tunnel_configure(LinphoneTunnel *tunnel){
	ms_warning("linphone_tunnel_configure() - stubbed, no implementation");
}

void linphone_tunnel_enable_sip(LinphoneTunnel *tunnel, bool_t enable) {
	ms_warning("linphone_tunnel_enable_sip() - stubbed, no implementation");
}
bool_t linphone_tunnel_sip_enabled(const LinphoneTunnel *tunnel) { return FALSE; }

/* Deprecated functions */
void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled) {}
bool_t linphone_tunnel_enabled(const LinphoneTunnel *tunnel) { return FALSE; }
void linphone_tunnel_auto_detect(LinphoneTunnel *tunnel) {}
bool_t linphone_tunnel_auto_detect_enabled(LinphoneTunnel *tunnel) { return FALSE; }
void linphone_tunnel_simulate_udp_loss(LinphoneTunnel *tunnel, bool_t enabled) {}
