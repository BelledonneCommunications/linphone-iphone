/***************************************************************************
 *            linphone_tunnel_config.c
 *
 *  Copyright  2012  Belledonne Communications
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

#include "linphone_tunnel.h"

struct _LinphoneTunnelConfig {
	char *host;
	int port;
	int remote_udp_mirror_port;
	int delay;	
};

LinphoneTunnelConfig *linphone_tunnel_config_new() {
	LinphoneTunnelConfig *ltc = ms_new0(LinphoneTunnelConfig,1);
	ltc->remote_udp_mirror_port = 12345;
	ltc->delay = 1000;
	return ltc;
}

void linphone_tunnel_config_set_host(LinphoneTunnelConfig *tunnel, const char *host) {
	if(tunnel->host != NULL) {
		ms_free(tunnel->host);
		tunnel->host = NULL;
	}
	if(host != NULL && strlen(host)) {
		tunnel->host = ms_strdup(host);
	}
}

const char *linphone_tunnel_config_get_host(const LinphoneTunnelConfig *tunnel) {
	return tunnel->host;
}

void linphone_tunnel_config_set_port(LinphoneTunnelConfig *tunnel, int port) {
	tunnel->port = port;
}

int linphone_tunnel_config_get_port(const LinphoneTunnelConfig *tunnel) {
	return tunnel->port;
}

void linphone_tunnel_config_set_remote_udp_mirror_port(LinphoneTunnelConfig *tunnel, int remote_udp_mirror_port) {
	tunnel->remote_udp_mirror_port = remote_udp_mirror_port;
}

int linphone_tunnel_config_get_remote_udp_mirror_port(const LinphoneTunnelConfig *tunnel) {
	return tunnel->remote_udp_mirror_port;
}

void linphone_tunnel_config_set_delay(LinphoneTunnelConfig *tunnel, int delay) {
	tunnel->delay = delay;
}

int linphone_tunnel_config_get_delay(const LinphoneTunnelConfig *tunnel) {
	return tunnel->delay;
}

void linphone_tunnel_config_destroy(LinphoneTunnelConfig *tunnel) {
	if(tunnel->host != NULL) {
		ms_free(tunnel->host);
	}
	ms_free(tunnel);
}

