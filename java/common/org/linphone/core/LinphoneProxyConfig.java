/*
LinphoneProxyConfig.java
Copyright (C) 2010  Belledonne Communications, Grenoble, France

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.core;
/**
 * The LinphoneProxyConfig object represents a proxy configuration to be used by the LinphoneCore object. Its fields must not be used directly in favour of the accessors methods. 
 * Once created and filled properly the LinphoneProxyConfig can be given to LinphoneCore with {@link LinphoneCore#addProxyConfig(LinphoneProxyConfig)}. This will automatically triggers the registration, if enabled.
 *<br>The proxy configuration are persistent to restarts because they are saved in the configuration file. As a consequence, after {@link LinphoneCoreFactory#createLinphoneCore(LinphoneCoreListener, String, String, Object)} there might already be a default proxy that can be examined with {@link LinphoneCore#getDefaultProxyConfig()} .
 *
 */
public interface LinphoneProxyConfig {
	
	/**
	 *Starts editing a proxy configuration.
	 *Because proxy configuration must be consistent, applications MUST call {@link #edit()} before doing any attempts to modify proxy configuration (such as identity, proxy address and so on). 
	 *Once the modifications are done, then the application must call {@link #done()} to commit the changes.
	 */
	public void edit();
	/**
	 * Commits modification made to the proxy configuration.
	 */
	public void done();
	/**
	 * Sets the user identity as a SIP address.
	 * @param identy This identity is normally formed with display name, username and domain, such as: Alice <sip:alice@example.net> The REGISTER messages will have from and to set to this identity.
	 */
	public void setIdentity(String identity) throws LinphoneCoreException;
	/**
	 *get  the SIP identity that belongs to this proxy configuration.
	 *
	 * @return The SIP identity is a SIP address (Display Name <sip:username> )
	 */
	public String getIdentity();
	/**
	 *Sets the proxy address
	 * Examples of valid sip proxy address are:
	 *<li>IP address: sip:87.98.157.38
	 *<li>IP address with port: sip:87.98.157.38:5062
	 *<li>hostnames : sip:sip.example.net
	 * @param proxyUri
	 * @throws LinphoneCoreException
	 */
	public void setProxy(String proxyUri) throws LinphoneCoreException;
	/**
	 * get the proxy's SIP address.
	 *  
	 */
	public String getProxy();
	/**
	 * Enable register for this proxy config.
	 * Register message is issued after call to {@link #done()}
	 * @param value
	 * @throws LinphoneCoreException
	 */	
	public void enableRegister(boolean value) throws LinphoneCoreException;
	/**
	 * @return true if registration to the proxy is enabled.
	 */
	public boolean registerEnabled();
	
	/**
	 * normalize a human readable phone number into a basic string. 888-444-222 becomes 888444222
	 * @param number
	 * @return
	 */
	public String normalizePhoneNumber(String number);
	/**
	 * Useful function to automatically add international prefix to e164 phone numbers
	 * @param prefix
	 */
	public void setDialPrefix(String prefix);
	/**
	 * * Sets whether liblinphone should replace "+" by "00" in dialed numbers (passed to
	 * {@link LinphoneCore#invite(String)}).
	 * @param value default value is false
	 */
	public void setDialEscapePlus(boolean value);
	
	/**
	 * get domain host name or ip
	 * @return may be null
	 */
	public String getDomain();
	/**
	 * 
	 * @return  a boolean indicating that the user is successfully registered on the proxy.
	 */
	public boolean isRegistered();
	/**
	 * Sets a SIP route. When a route is set, all outgoing calls will go to the route's destination if this proxy is the default one (see {@link LinphoneCore#getDefaultProxyConfig()} ).
	 * @param routeUri ex sip:git.linphone.org
	 * @throws LinphoneCoreException
	 */
	public void setRoute(String routeUri) throws LinphoneCoreException;
	/**
	 * 
	 * @return  the route set for this proxy configuration.
	 */
	public String getRoute();
	
}
