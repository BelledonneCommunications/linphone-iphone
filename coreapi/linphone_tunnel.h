/***************************************************************************
 *            linphone_tunnel.h
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

/*
 * Linphone VoIP tunnel extension API
**/

#ifdef __cplusplus
extern "C"
{
#endif


void linphone_tunnel_add_server(LinphoneTunnel *tunnel, const char *host, int port);
void linphone_tunnel_add_server_and_mirror(LinphoneTunnel *tunnel, const char *host, int port, int remote_udp_mirror, int delay);
/*returns a string of space separated list of host:port of tunnel server addresses*/
char *linphone_tunnel_get_servers(LinphoneTunnel *tunnel);
void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel);
void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled);
bool_t linphone_tunnel_enabled(LinphoneTunnel *tunnel);
void linphone_tunnel_reconnect(LinphoneTunnel *tunnel);
void linphone_tunnel_auto_detect(LinphoneTunnel *tunnel);
void linphone_tunnel_set_http_proxy(LinphoneTunnel *tunnel, const char *host, int port, const char* username,const char* passwd);
void linphone_tunnel_set_http_proxy_auth_info(LinphoneTunnel*tunnel, const char* username,const char* passwd);


#ifdef __cplusplus
}
#endif


#endif

