/***************************************************************************
 *            tunnel.h
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

#ifndef LINPHONETUNNEL_H
#define LINPHONETUNNEL_H

#include "linphone/core.h"


/**
 * @addtogroup tunnel
 * @{
**/

	/**
	 * Linphone tunnel aims is to bypass IP traffic blocking due to aggressive firewalls which typically only authorize TCP traffic with destination port 443.
	 * <br> Its principle is tunneling all SIP and/or RTP traffic through a single secure https connection up to a detunnelizer server.
	 * <br> This set of methods enhance  LinphoneCore functionalities in order to provide an easy to use API to
	 * \li provision tunnel servers IP addresses and ports. This functionality is an option not implemented under GPL. Availability can be check at runtime using function #linphone_core_tunnel_available
	 * \li start/stop the tunneling service
	 * \li perform auto-detection whether tunneling is required, based on a test of sending/receiving a flow of UDP packets.
	 *
	 * It takes in charge automatically the SIP registration procedure when connecting or disconnecting to a tunnel server.
	 * No other action on LinphoneCore is required to enable full operation in tunnel mode.
	 *
	 * <br> Provision is done using object #LinphoneTunnelConfig created by function #linphone_tunnel_config_new(). Functions #linphone_tunnel_config_set_host
	 *  and #linphone_tunnel_config_set_port allow to point to tunnel server IP/port. Once set, use function #linphone_tunnel_add_server to provision a tunnel server.
	 *  <br> Finally  tunnel mode configuration is achieved by function #linphone_tunnel_set_mode.
	 *  <br> Tunnel connection status can be checked using function #linphone_tunnel_connected.
	 *
	 * Bellow pseudo code that can be use to configure, enable, check state and disable tunnel functionality:
	 *
	 * \code
	LinphoneTunnel *tunnel = linphone_core_get_tunnel(linphone_core);
	LinphoneTunnelConfig *config=linphone_tunnel_config_new(); //instantiate tunnel configuration
	linphone_tunnel_config_set_host(config, "tunnel.linphone.org"); //set tunnel server host address
	linphone_tunnel_config_set_port(config, 443); //set tunnel server port
	linphone_tunnel_add_server(tunnel, config); //provision tunnel config
	linphone_tunnel_set_mode(tunnel, LinphoneTunnelModeEnable); //activate the tunnel unconditional

	while (!linphone_tunnel_connected(tunnel)) { //wait for tunnel to be ready
		linphone_core_iterate(linphone_core); //schedule core main loop
		ms_sleep(100); //wait 100ms
	}

	LinphoneCall *call = linphone_core_invite(linphone_core,"sip:foo@example.org"); //place an outgoing call
	linphone_call_ref(call); //acquire a reference on the call to avoid deletion after completion
	//...
	linphone_core_terminate_call(linphone_core,call);

	while (linphone_call_get_state(call) != LinphoneCallReleased) { //wait for call to be in release state
		linphone_core_iterate(linphone_core); //schedule core main loop
		ms_sleep(100); //wait 100ms
	}

	linphone_tunnel_set_mode(tunnel, LinphoneTunnelModeDisable); //deactivate tunnel
	linphone_call_unref(call); //release reference on the call

	\endcode
*
*	**/

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _LinphoneTunnelConfig LinphoneTunnelConfig;

/**
 * Enum describing the tunnel modes.
**/
typedef enum _LinphoneTunnelMode {
	LinphoneTunnelModeDisable,	/**< The tunnel is disabled. */
	LinphoneTunnelModeEnable,	/**< The tunnel is enabled. */
	LinphoneTunnelModeAuto	/**< The tunnel is enabled automatically if it is required. */
} LinphoneTunnelMode;

/**
 * Convert a string into LinphoneTunnelMode enum
 * @param string String to convert
 * @return An LinphoneTunnelMode enum. If the passed string is NULL or
 * does not match with any mode, the LinphoneTunnelModeDisable is returned.
 */
LINPHONE_PUBLIC LinphoneTunnelMode linphone_tunnel_mode_from_string(const char *string);

/**
 * Convert a tunnel mode enum into string
 * @param mode Enum to convert
 * @return "disable", "enable" or "auto"
 */
LINPHONE_PUBLIC const char *linphone_tunnel_mode_to_string(LinphoneTunnelMode mode);

/**
 * Create a new tunnel configuration
 */
LINPHONE_PUBLIC LinphoneTunnelConfig *linphone_tunnel_config_new(void);

/**
 * Set the IP address or hostname of the tunnel server.
 * @param tunnel LinphoneTunnelConfig object
 * @param host The tunnel server IP address or hostname
 */
LINPHONE_PUBLIC void linphone_tunnel_config_set_host(LinphoneTunnelConfig *tunnel, const char *host);

/**
 * Get the IP address or hostname of the tunnel server.
 * @param tunnel LinphoneTunnelConfig object
 * @return The tunnel server IP address or hostname
 */
LINPHONE_PUBLIC const char *linphone_tunnel_config_get_host(const LinphoneTunnelConfig *tunnel);

/**
 * Set tls port of server.
 * @param tunnel LinphoneTunnelConfig object
 * @param port The tunnel server TLS port, recommended value is 443
 */
LINPHONE_PUBLIC void linphone_tunnel_config_set_port(LinphoneTunnelConfig *tunnel, int port);

/**
 * Get the TLS port of the tunnel server.
 * @param tunnel LinphoneTunnelConfig object
 * @return The TLS port of the tunnel server
 */
LINPHONE_PUBLIC int linphone_tunnel_config_get_port(const LinphoneTunnelConfig *tunnel);

/**
 * Set the remote port on the tunnel server side used to test UDP reachability.
 * This is used when the mode is set auto, to detect whether the tunnel has to be enabled or not.
 * @param tunnel LinphoneTunnelConfig object
 * @param remote_udp_mirror_port The remote port on the tunnel server side used to test UDP reachability, set to -1 to disable the feature
 */
LINPHONE_PUBLIC void linphone_tunnel_config_set_remote_udp_mirror_port(LinphoneTunnelConfig *tunnel, int remote_udp_mirror_port);

/**
 * Get the remote port on the tunnel server side used to test UDP reachability.
 * This is used when the mode is set auto, to detect whether the tunnel has to be enabled or not.
 * @param tunnel LinphoneTunnelConfig object
 * @return The remote port on the tunnel server side used to test UDP reachability
 */
LINPHONE_PUBLIC int linphone_tunnel_config_get_remote_udp_mirror_port(const LinphoneTunnelConfig *tunnel);

/**
 * Set the UDP packet round trip delay in ms for a tunnel configuration.
 * @param tunnel LinphoneTunnelConfig object
 * @param delay The UDP packet round trip delay in ms considered as acceptable (recommended value is 1000 ms).
 */
LINPHONE_PUBLIC void linphone_tunnel_config_set_delay(LinphoneTunnelConfig *tunnel, int delay);

/**
 * Get the UDP packet round trip delay in ms for a tunnel configuration.
 * @param tunnel LinphoneTunnelConfig object
 * @return The UDP packet round trip delay in ms.
 */
LINPHONE_PUBLIC int linphone_tunnel_config_get_delay(const LinphoneTunnelConfig *tunnel);

/**
 * Increment the refcount of LinphoneTunnelConfig object.
 * @param cfg the LinphoneTunnelConfig object.
 * @return the same cfg object.
**/
LINPHONE_PUBLIC LinphoneTunnelConfig * linphone_tunnel_config_ref(LinphoneTunnelConfig *cfg);


/**
 * Decrement the refcount of LinphoneTunnelConfig object.
 * @param cfg the LinphoneTunnelConfig object.
**/
LINPHONE_PUBLIC void linphone_tunnel_config_unref(LinphoneTunnelConfig *cfg);
/**
 * Destroy a tunnel configuration
 * @param tunnel LinphoneTunnelConfig object
 * @deprecated use linphone_tunnel_config_unref().
 */
LINPHONE_PUBLIC void linphone_tunnel_config_destroy(LinphoneTunnelConfig *tunnel);

/**
 * Store a user data in the tunnel config object
 * @param cfg the tunnel config
 * @param ud the user data
**/
LINPHONE_PUBLIC void linphone_tunnel_config_set_user_data(LinphoneTunnelConfig *cfg, void *ud);

/**
 * Retrieve user data from the tunnel config
 * @param cfg the tunnel config
 * @return the user data
**/
LINPHONE_PUBLIC void *linphone_tunnel_config_get_user_data(LinphoneTunnelConfig *cfg);

/**
 * Add a tunnel server configuration.
 * @param tunnel LinphoneTunnel object
 * @param tunnel_config LinphoneTunnelConfig object
 */
LINPHONE_PUBLIC void linphone_tunnel_add_server(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config);

/**
 * Remove a tunnel server configuration.
 * @param tunnel LinphoneTunnel object
 * @param tunnel_config LinphoneTunnelConfig object
 */
LINPHONE_PUBLIC void linphone_tunnel_remove_server(LinphoneTunnel *tunnel, LinphoneTunnelConfig *tunnel_config);

/**
 * Get added servers
 * @param tunnel LinphoneTunnel object
 * @return \bctbx_list{LinphoneTunnelConfig}
 */
LINPHONE_PUBLIC const bctbx_list_t *linphone_tunnel_get_servers(const LinphoneTunnel *tunnel);

/**
 * Remove all tunnel server addresses previously entered with linphone_tunnel_add_server()
 * @param tunnel LinphoneTunnel object
**/
LINPHONE_PUBLIC void linphone_tunnel_clean_servers(LinphoneTunnel *tunnel);

/**
 * Set the tunnel mode.
 * The tunnel mode can be 'enable', 'disable' or 'auto'
 * If the mode is set to 'auto', the tunnel manager will try to established an RTP session
 * with the tunnel server on the UdpMirrorPort. If the connection fail, the tunnel is automatically
 * activated whereas the tunnel is automatically disabled if the connection succeed.
 * @param tunnel LinphoneTunnel object
 * @param mode The desired LinphoneTunnelMode
**/
LINPHONE_PUBLIC void linphone_tunnel_set_mode(LinphoneTunnel *tunnel, LinphoneTunnelMode mode);

/**
 * Get the tunnel mode
 * @param tunnel LinphoneTunnel object
 * @return The current LinphoneTunnelMode
**/
LINPHONE_PUBLIC LinphoneTunnelMode linphone_tunnel_get_mode(const LinphoneTunnel *tunnel);

/**
 * Returns whether the tunnel is activated. If mode is set to auto, this gives indication whether the automatic detection determined
 * that tunnel was necessary or not.
 * @param tunnel the tunnel
 * @return TRUE if tunnel is in use, FALSE otherwise.
**/
LINPHONE_PUBLIC bool_t linphone_tunnel_get_activated(const LinphoneTunnel *tunnel);


/**
 * Check whether the tunnel is connected
 * @param tunnel LinphoneTunnel object
 * @return A boolean value telling if the tunnel is connected
**/
LINPHONE_PUBLIC bool_t linphone_tunnel_connected(const LinphoneTunnel *tunnel);

/**
 * Force reconnection to the tunnel server.
 * This method is useful when the device switches from wifi to Edge/3G or vice versa. In most cases the tunnel client socket
 * won't be notified promptly that its connection is now zombie, so it is recommended to call this method that will cause
 * the lost connection to be closed and new connection to be issued.
 * @param tunnel LinphoneTunnel object
**/
LINPHONE_PUBLIC void linphone_tunnel_reconnect(LinphoneTunnel *tunnel);

/**
 * Set whether SIP packets must be directly sent to a UA or pass through the tunnel
 * @param tunnel LinphoneTunnel object
 * @param enable If true, SIP packets shall pass through the tunnel
 */
LINPHONE_PUBLIC void linphone_tunnel_enable_sip(LinphoneTunnel *tunnel, bool_t enable);

/**
 * Check whether tunnel is set to transport SIP packets
 * @param tunnel LinphoneTunnel object
 * @return A boolean value telling whether SIP packets shall pass through the tunnel
 */
LINPHONE_PUBLIC bool_t linphone_tunnel_sip_enabled(const LinphoneTunnel *tunnel);

/**
 * Set an optional http proxy to go through when connecting to tunnel server.
 * @param tunnel LinphoneTunnel object
 * @param host http proxy host
 * @param port http proxy port
 * @param username Optional http proxy username if the proxy request authentication. Currently only basic authentication is supported. Use NULL if not needed.
 * @param passwd Optional http proxy password. Use NULL if not needed.
 **/
LINPHONE_PUBLIC void linphone_tunnel_set_http_proxy(LinphoneTunnel *tunnel, const char *host, int port, const char* username,const char* passwd);

/**
 * Retrieve optional http proxy configuration previously set with linphone_tunnel_set_http_proxy().
 * @param tunnel LinphoneTunnel object
 * @param host http proxy host
 * @param port http proxy port
 * @param username Optional http proxy username if the proxy request authentication. Currently only basic authentication is supported. Use NULL if not needed.
 * @param passwd Optional http proxy password. Use NULL if not needed.
 **/
LINPHONE_PUBLIC void linphone_tunnel_get_http_proxy(LinphoneTunnel*tunnel,const char **host, int *port, const char **username, const char **passwd);

/**
 * Set authentication info for the http proxy
 * @param tunnel LinphoneTunnel object
 * @param username User name
 * @param passwd Password
 */
LINPHONE_PUBLIC void linphone_tunnel_set_http_proxy_auth_info(LinphoneTunnel*tunnel, const char* username,const char* passwd);


/**
 * Sets whether tunneling of SIP and RTP is required.
 * @param tunnel object
 * @param enabled If true enter in tunneled mode, if false exits from tunneled mode.
 * The TunnelManager takes care of refreshing SIP registration when switching on or off the tunneled mode.
 * @deprecated Replaced by linphone_tunnel_set_mode()
**/
LINPHONE_PUBLIC void linphone_tunnel_enable(LinphoneTunnel *tunnel, bool_t enabled);

/**
 * Check whether tunnel is enabled
 * @param tunnel Tunnel object
 * @return Returns a boolean indicating whether tunneled operation is enabled.
 * @deprecated Replaced by linphone_tunnel_get_mode()
**/
LINPHONE_PUBLIC bool_t linphone_tunnel_enabled(const LinphoneTunnel *tunnel);

/**
 * Start tunnel need detection.
 * @param  tunnel object
 * In auto detect mode, the tunnel manager try to establish a real time rtp communication with the tunnel server on  specified port.
 * <br>In case of success, the tunnel is automatically turned off. Otherwise, if no udp communication is feasible, tunnel mode is turned on.
 * <br> Call this method each time to run the auto detection algorithm
 * @deprecated Replaced by linphone_tunnel_set_mode(LinphoneTunnelModeAuto)
 */
LINPHONE_PUBLIC void linphone_tunnel_auto_detect(LinphoneTunnel *tunnel);

/**
 * Tell whether tunnel auto detection is enabled.
 * @param[in] tunnel LinphoneTunnel object.
 * @return TRUE if auto detection is enabled, FALSE otherwise.
 * @deprecated Replaced by linphone_tunnel_get_mode()
 */
LINPHONE_PUBLIC bool_t linphone_tunnel_auto_detect_enabled(LinphoneTunnel *tunnel);

LINPHONE_PUBLIC void linphone_tunnel_simulate_udp_loss(LinphoneTunnel *tunnel, bool_t enabled);

/**
 * @}
**/

#ifdef __cplusplus
}
#endif


#endif //LINPHONETUNNEL_H

