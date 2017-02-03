/*
nat_policy.h
Copyright (C) 2010-2016 Belledonne Communications SARL

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

#ifndef LINPHONE_NAT_POLICY_H_
#define LINPHONE_NAT_POLICY_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup network_parameters
 * @{
 */

/**
 * Acquire a reference to the LinphoneNatPolicy object.
 * @param[in] policy LinphoneNatPolicy object.
 * @return The same LinphoneNatPolicy object.
**/
LINPHONE_PUBLIC LinphoneNatPolicy * linphone_nat_policy_ref(LinphoneNatPolicy *policy);

/**
 * Release reference to the LinphoneNatPolicy object.
 * @param[in] policy LinphoneNatPolicy object.
**/
LINPHONE_PUBLIC void linphone_nat_policy_unref(LinphoneNatPolicy *policy);

/**
 * Retrieve the user pointer associated with the LinphoneNatPolicy object.
 * @param[in] policy LinphoneNatPolicy object.
 * @return The user pointer associated with the LinphoneNatPolicy object.
**/
LINPHONE_PUBLIC void *linphone_nat_policy_get_user_data(const LinphoneNatPolicy *policy);

/**
 * Assign a user pointer to the LinphoneNatPolicy object.
 * @param[in] policy LinphoneNatPolicy object.
 * @param[in] ud The user pointer to associate with the LinphoneNatPolicy object.
**/
LINPHONE_PUBLIC void linphone_nat_policy_set_user_data(LinphoneNatPolicy *policy, void *ud);

/**
 * Clear a NAT policy (deactivate all protocols and unset the STUN server).
 * @param[in] policy LinphoneNatPolicy object.
 */
LINPHONE_PUBLIC void linphone_nat_policy_clear(LinphoneNatPolicy *policy);

/**
 * Tell whether STUN is enabled.
 * @param[in] policy LinphoneNatPolicy object
 * @return Boolean value telling whether STUN is enabled.
 */
LINPHONE_PUBLIC bool_t linphone_nat_policy_stun_enabled(const LinphoneNatPolicy *policy);

/**
 * Enable STUN.
 * If TURN is also enabled, TURN will be used instead of STUN.
 * @param[in] policy LinphoneNatPolicy object
 * @param[in] enable Boolean value telling whether to enable STUN.
 */
LINPHONE_PUBLIC void linphone_nat_policy_enable_stun(LinphoneNatPolicy *policy, bool_t enable);

/**
 * Tell whether TURN is enabled.
 * @param[in] policy LinphoneNatPolicy object
 * @return Boolean value telling whether TURN is enabled.
 */
LINPHONE_PUBLIC bool_t linphone_nat_policy_turn_enabled(const LinphoneNatPolicy *policy);

/**
 * Enable TURN.
 * If STUN is also enabled, it is ignored and TURN is used.
 * @param[in] policy LinphoneNatPolicy object
 * @param[in] enable Boolean value telling whether to enable TURN.
 */
LINPHONE_PUBLIC void linphone_nat_policy_enable_turn(LinphoneNatPolicy *policy, bool_t enable);

/**
 * Tell whether ICE is enabled.
 * @param[in] policy LinphoneNatPolicy object
 * @return Boolean value telling whether ICE is enabled.
 */
LINPHONE_PUBLIC bool_t linphone_nat_policy_ice_enabled(const LinphoneNatPolicy *policy);

/**
 * Enable ICE.
 * ICE can be enabled without STUN/TURN, in which case only the local candidates will be used.
 * @param[in] policy LinphoneNatPolicy object
 * @param[in] enable Boolean value telling whether to enable ICE.
 */
LINPHONE_PUBLIC void linphone_nat_policy_enable_ice(LinphoneNatPolicy *policy, bool_t enable);

/**
 * Tell whether uPnP is enabled.
 * @param[in] policy LinphoneNatPolicy object
 * @return Boolean value telling whether uPnP is enabled.
 */
LINPHONE_PUBLIC bool_t linphone_nat_policy_upnp_enabled(const LinphoneNatPolicy *policy);

/**
 * Enable uPnP.
 * This has the effect to disable every other policies (ICE, STUN and TURN).
 * @param[in] policy LinphoneNatPolicy object
 * @param[in] enable Boolean value telling whether to enable uPnP.
 */
LINPHONE_PUBLIC void linphone_nat_policy_enable_upnp(LinphoneNatPolicy *policy, bool_t enable);

/**
 * Get the STUN/TURN server to use with this NAT policy.
 * Used when STUN or TURN are enabled.
 * @param[in] policy LinphoneNatPolicy object
 * @return The STUN server used by this NAT policy.
 */
LINPHONE_PUBLIC const char * linphone_nat_policy_get_stun_server(const LinphoneNatPolicy *policy);

/**
 * Set the STUN/TURN server to use with this NAT policy.
 * Used when STUN or TURN are enabled.
 * @param[in] policy LinphoneNatPolicy object
 * @param[in] stun_server The STUN server to use with this NAT policy.
 */
LINPHONE_PUBLIC void linphone_nat_policy_set_stun_server(LinphoneNatPolicy *policy, const char *stun_server);

/**
 * Get the username used to authenticate with the STUN/TURN server.
 * The authentication will search for a LinphoneAuthInfo with this username.
 * If it is not set the username of the currently used LinphoneProxyConfig is used to search for a LinphoneAuthInfo.
 * @param[in] policy LinphoneNatPolicy object
 * @return The username used to authenticate with the STUN/TURN server.
 */
LINPHONE_PUBLIC const char * linphone_nat_policy_get_stun_server_username(const LinphoneNatPolicy *policy);

/**
 * Set the username used to authenticate with the STUN/TURN server.
 * The authentication will search for a LinphoneAuthInfo with this username.
 * If it is not set the username of the currently used LinphoneProxyConfig is used to search for a LinphoneAuthInfo.
 * @param[in] policy LinphoneNatPolicy object
 * @param[in] username The username used to authenticate with the STUN/TURN server.
 */
LINPHONE_PUBLIC void linphone_nat_policy_set_stun_server_username(LinphoneNatPolicy *policy, const char *username);

/**
 * Start a STUN server DNS resolution.
 * @param[in] policy LinphoneNatPolicy object
 */
LINPHONE_PUBLIC void linphone_nat_policy_resolve_stun_server(LinphoneNatPolicy *policy);

/**
 * Get the addrinfo representation of the STUN server address.
 * WARNING: This function may block for up to 1 second.
 * @param[in] policy LinphoneNatPolicy object
 * @return addrinfo representation of the STUN server address.
 */
LINPHONE_PUBLIC const struct addrinfo * linphone_nat_policy_get_stun_server_addrinfo(LinphoneNatPolicy *policy);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_NAT_POLICY_H_ */
