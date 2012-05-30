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

/**
 * @addtogroup tunnel
 * @{
**/

	/**
	 * This set of methods enhance  LinphoneCore functionalities in order to provide an easy to use API to
	 * - provision tunnel servers ip addresses and ports. This functionality is an option not implemented under GPL.
	 * - start/stop the tunneling service
	 * - perform auto-detection whether tunneling is required, based on a test of sending/receiving a flow of UDP packets.
	 *
	 * It takes in charge automatically the SIP registration procedure when connecting or disconnecting to a tunnel server.
	 * No other action on LinphoneCore is required to enable full operation in tunnel mode.
	**/

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Add a tunnel server. At least one should be provided to be able to connect.
 * When several addresses are provided, the tunnel client may try each of them until it gets connected.
 * @param  tunnel object
 * @param host server ip address
 * @param port tunnel server tls port, recommended value is 443
 */
void linphone_tunnel_add_server(LinphoneTunnel *tunnel, const char *host, int port);
/**
 *Add tunnel server with auto detection capabilities
 *
 * @param  tunnel object
 * @param host tunnel server ip address
 * @param port tunnel server tls port, recommended value is 443
 * @param remote_udp_mirror remote port on the tunnel server side  used to test udp reachability
 * @param delay udp packet round trip delay in ms considered as acceptable. recommended value is 1000 ms.
 */
void linphone_tunnel_add_server_and_mirror(LinphoneTunnel *tunnel, const char *host, int port, int remote_udp_mirror, int delay);
/**
 * @param  tunnel object
 * returns a string of space separated list of host:port of tunnel server addresses
 * */
char *linphone_tunnel_get_servers(LinphoneTunnel *tunnel);
/**
 * @param  tunnel object
 * Removes all tunnel server address previously entered with addServer()
**/
void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel);
/**
 * Sets whether tunneling of SIP and RTP is required.
 * @param  tunnel object
 * @param enabled If true enter in tunneled mode, if false exits from tunneled mode.
 * The TunnelManager takes care of refreshing SIP registration when switching on or off the tunneled mode.
 *
**/
void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled);
/**
 * @param  tunnel object
 * Returns a boolean indicating whether tunneled operation is enabled.
**/
bool_t linphone_tunnel_enabled(LinphoneTunnel *tunnel);
/**
 * @param  tunnel object
 * Forces reconnection to the tunnel server.
 * This method is useful when the device switches from wifi to Edge/3G or vice versa. In most cases the tunnel client socket
 * won't be notified promptly that its connection is now zombie, so it is recommended to call this method that will cause
 * the lost connection to be closed and new connection to be issued.
**/
void linphone_tunnel_reconnect(LinphoneTunnel *tunnel);
/**
 * @param  tunnel object
 * In auto detect mode, the tunnel manager try to establish a real time rtp cummunication with the tunnel server on  specified port.
 *<br>In case of success, the tunnel is automatically turned off. Otherwise, if no udp commmunication is feasible, tunnel mode is turned on.
 *<br> Call this method each time to run the auto detection algorithm
 */
void linphone_tunnel_auto_detect(LinphoneTunnel *tunnel);
void linphone_tunnel_set_http_proxy(LinphoneTunnel *tunnel, const char *host, int port, const char* username,const char* passwd);
void linphone_tunnel_set_http_proxy_auth_info(LinphoneTunnel*tunnel, const char* username,const char* passwd);
void linphone_tunnel_get_http_proxy(LinphoneTunnel*tunnel,const char **host, int *port, const char **username, const char **passwd);

void linphone_tunnel_enable_logs(LinphoneTunnel *tunnel, bool_t enabled);

/**
 * @}
**/

#ifdef __cplusplus
}
#endif


#endif

