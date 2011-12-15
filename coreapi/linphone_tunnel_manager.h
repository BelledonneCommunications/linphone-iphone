/***************************************************************************
 *            linphone_tunnel_manager.h
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
 
#ifndef LINPHONETUNNELMANAGER_H
#define LINPHONETUNNELMANAGER_H

#include "linphonecore.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LinphoneTunnelManager LinphoneTunnelManager;

LinphoneTunnelManager *linphone_tunnel_get(LinphoneCore *lc);
void linphone_tunnel_destroy(LinphoneTunnelManager *tunnel);
void linphone_tunnel_add_server(LinphoneTunnelManager *tunnel, const char *host, int port);
void linphone_tunnel_add_server_and_mirror(LinphoneTunnelManager *tunnel, const char *host, int port, int remote_udp_mirror, int delay);
void linphone_tunnel_clean_servers(LinphoneTunnelManager *tunnel);
void linphone_tunnel_enable(LinphoneTunnelManager *tunnel, bool_t enabled);
bool_t linphone_tunnel_enabled(LinphoneTunnelManager *tunnel);
void linphone_tunnel_enable_logs(LinphoneTunnelManager *tunnel, bool_t enabled);
void linphone_tunnel_auto_detect(LinphoneTunnelManager *tunnel);



/**
 * LinphoneTunnelState describes the tunnel activation states.
 */
typedef enum _LinphoneTunnelState{
	LinphoneTunnelDisabled, /**<The tunnel is always on */
	LinphoneTunnelEnabled, /**<The tunnel is always off */
	LinphoneTunnelAuto /**<The tunnel is active if needed */
}LinphoneTunnelState;

/**
 * Set tunnel addresses.
**/
void linphone_tunnel_set_server_addresses(LinphoneTunnelManager *tunnel, const char *lists);

/**
 * Get tunnel addresses.
**/
const char *linphone_tunnel_get_server_addresses(LinphoneTunnelManager *tunnel);

/**
 * Set tunnel state.
**/
void linphone_tunnel_set_state(LinphoneTunnelManager *tunnel, LinphoneTunnelState state);

/**
 * Get tunnel state.
**/
LinphoneTunnelState linphone_tunnel_get_state(LinphoneTunnelManager *tunnel);


#ifdef __cplusplus
}
#endif


#endif
