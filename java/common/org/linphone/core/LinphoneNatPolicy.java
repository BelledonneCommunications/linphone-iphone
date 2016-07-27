/*
LinphoneNatPolicy.java
Copyright (C) 2016  Belledonne Communications, Grenoble, France

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

package org.linphone.core;

/**
 * Policy to use to pass through NATs/firewalls.
 *
 */
public interface LinphoneNatPolicy {
	/**
	 * Clear a NAT policy (deactivate all protocols and unset the STUN server).
	 */
	void clear();

	/**
	 * Tell whether STUN is enabled.
	 * @return Boolean value telling whether STUN is enabled.
	 */
	boolean stunEnabled();

	/**
	 * Enable STUN.
	 * If TURN is also enabled, TURN will be used instead of STUN.
	 * @param enable Boolean value telling whether to enable STUN.
	 */
	void enableStun(boolean enable);

	/**
	 * Tell whether TURN is enabled.
	 * @return Boolean value telling whether TURN is enabled.
	 */
	boolean turnEnabled();

	/**
	 * Enable TURN.
	 * If STUN is also enabled, it is ignored and TURN is used.
	 * @param enable Boolean value telling whether to enable TURN.
	 */
	void enableTurn(boolean enable);

	/**
	 * Tell whether ICE is enabled.
	 * @return Boolean value telling whether ICE is enabled.
	 */
	boolean iceEnabled();

	/**
	 * Enable ICE.
	 * ICE can be enabled without STUN/TURN, in which case only the local candidates will be used.
	 * @param enable Boolean value telling whether to enable ICE.
	 */
	void enableIce(boolean enable);

	/**
	 * Tell whether uPnP is enabled.
	 * @return Boolean value telling whether uPnP is enabled.
	 */
	boolean upnpEnabled();

	/**
	 * Enable uPnP.
	 * This has the effect to disable every other policies (ICE, STUN and TURN).
	 * @param enable Boolean value telling whether to enable uPnP.
	 */
	void enableUpnp(boolean enable);

	/**
	 * Get the STUN/TURN server to use with this NAT policy.
	 * Used when STUN or TURN are enabled.
	 * @return The STUN server used by this NAT policy.
	 */
	String getStunServer();

	/**
	 * Set the STUN/TURN server to use with this NAT policy.
	 * Used when STUN or TURN are enabled.
	 * @param stun_server The STUN server to use with this NAT policy.
	 */
	void setStunServer(String stun_server);

	/**
	 * Get the username used to authenticate with the STUN/TURN server.
	 * The authentication will search for a LinphoneAuthInfo with this username.
	 * If it is not set the username of the currently used LinphoneProxyConfig is used to search for a LinphoneAuthInfo.
	 * @return The username used to authenticate with the STUN/TURN server.
	 */
	String getStunServerUsername();

	/**
	 * Set the username used to authenticate with the STUN/TURN server.
	 * The authentication will search for a LinphoneAuthInfo with this username.
	 * If it is not set the username of the currently used LinphoneProxyConfig is used to search for a LinphoneAuthInfo.
	 * @param username The username used to authenticate with the STUN/TURN server.
	 */
	void setStunServerUsername(String username);
}
