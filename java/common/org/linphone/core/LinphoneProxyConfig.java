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
	public LinphoneProxyConfig edit();
	/**
	 * Commits modification made to the proxy configuration.
	 */
	public void done();
	/**
	 * Sets the user identity as a SIP address.
	 * @param identity This identity is normally formed with display name, username and domain, such as: Alice <sip:alice@example.net> The REGISTER messages will have from and to set to this identity.
	 */
	public void setIdentity(String identity) throws LinphoneCoreException;
	/**
	 *get  the SIP identity that belongs to this proxy configuration.
	 *
	 * @return The SIP identity is a SIP address (Display Name <sip:username> )
	 */
	public String getIdentity();
	/**
	 * Sets the address of the proxy configuration
	 * @param address
	 */
	public void setAddress(LinphoneAddress address) throws LinphoneCoreException;
	/**
	 *get linphoneAddress that belongs to this proxy configuration.
	 *
	 * @return LinphoneAddress
	 */
	public LinphoneAddress getAddress();
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
	 */
	public LinphoneProxyConfig enableRegister(boolean value);
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
	 * Normalize a human readable sip uri into a fully qualified LinphoneAddress.
	 * A sip address should look like DisplayName \<sip:username\@domain:port\> .
	 * @param username the string to parse
	 * @return NULL if invalid input, normalized sip address otherwise.
	 */
	public LinphoneAddress normalizeSipUri(String username);
	/**
	 * Useful function to automatically add international prefix to e164 phone numbers
	 * @param prefix
	 */
	public void setDialPrefix(String prefix);

	/**
	 * Returns the automatically added international prefix to e164 phone numbers
	 */
	public String getDialPrefix();

	/**
	 * * Sets whether liblinphone should replace "+" by "00" in dialed numbers (passed to
	 * {@link LinphoneCore#invite(String)}).
	 * @param value default value is false
	 */
	public void setDialEscapePlus(boolean value);

	/**
	 * Whether liblinphone should replace "+" by "00" in dialed numbers (passed to
	 * {@link LinphoneCore#invite(String)}).
	 */
	public boolean getDialEscapePlus();

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
	/**
	 * Indicates  either or not, PUBLISH must be issued for this #LinphoneProxyConfig .
	 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
	 * @param enable if true, publish will be engaged
	 *
	 */
	public void enablePublish(boolean enable);
	/**
	 * returns publish state for this proxy config (see {@link #enablePublish(boolean)} )
	 */
	public boolean publishEnabled();


	LinphoneCore.RegistrationState getState();

	/**
	 * Sets the registration expiration time.
	 * @param delay expiration time in seconds
	 */
	void setExpires(int delay);

	/**
	 * Gets the registration expiration time.
	 * @return delay expiration time in seconds.
	 */
	int getExpires();

	/**
	 * Set the privacy for all calls or chat sessions using the identity exposed by this LinphoneProxyConfig
	 * @param privacy_mask a or'd int of values defined in interface {@link org.linphone.core.Privacy}
	 */
	void setPrivacy(int privacy_mask);

	/**
	 * Get the privacy mask requested for this proxy config.
	 * @return the privacy mask as defined in interface {@link org.linphone.core.Privacy}
	 */
	int getPrivacy();

	/**
	 * Indicates whether AVPF/SAVPF must be used for calls using this proxy config.
	 * @param enable True to enable AVPF/SAVF, false to disable it.
	 */
	void enableAvpf(boolean enable);

	/**
	 * Whether AVPF is used for calls through this proxy.
	 * @return
	 */
	boolean avpfEnabled();

	/**
	 * Set the interval between regular RTCP reports when using AVPF/SAVPF.
	 * @param interval The interval in seconds (between 0 and 5 seconds).
	 */
	void setAvpfRRInterval(int interval);

	/**
	 * Get the interval between regular RTCP reports when using AVPF/SAVPF.
	 * @return The interval in seconds.
	 */
	int getAvpfRRInterval();

	/**
	 * Indicates whether quality reporting must be used for calls using this proxy config.
	 * @param enable True to enable quality reporting, false to disable it.
	 */
	void enableQualityReporting(boolean enable);


	/**
	 * Whether quality reporting is used for calls through this proxy.
	 * @return
	 */
	boolean qualityReportingEnabled();

	/**
	 * Set the interval between quality interval reports during a call when using quality reporting.
	 * @param interval The interval in seconds (should be greater than 120 seconds to avoid too much).
	 */
	void setQualityReportingInterval(int interval);

	/**
	 * Get the interval between quality interval reports during a call when using quality reporting.
	 * @return The interval in seconds.
	 */
	int getQualityReportingInterval();

	/**
	 * Set the collector SIP URI to collect reports when using quality reporting.
	 * @param collector The collector SIP URI which should be configured server side too.
	 */
	void setQualityReportingCollector(String collector);

	/**
	 * Get the collector SIP URI collecting reports when using quality reporting.
	 * @return The SIP URI collector address.
	 */
	String getQualityReportingCollector();

	/**
	 * Set the outbound proxy realm. It is used in digest authentication to avoid
	 * re-authentication if a previous token has already been provided.
	 * @param realm The new outbound proxy realm.
	 */
	void setRealm(String realm);

	/**
	 * Get the outbound proxy realm.
	 * @return The outbound proxy realm.
	 */
	String getRealm();

	/**
	 * Set optional contact parameters that will be added to the contact information sent in the registration.
	 * @param contact_params a string containing the additional parameters in text form, like "myparam=something;myparam2=something_else"
	 *
	 * The main use case for this function is provide the proxy additional information regarding the user agent, like for example unique identifier or android push id.
	 * As an example, the contact address in the SIP register sent will look like <sip:joe@15.128.128.93:50421>;android-push-id=43143-DFE23F-2323-FA2232.
	**/
	public void setContactParameters(String contact_params);

	/**
	 * Get the contact's parameters.
	 * @return
	 */
	public String getContactParameters();

	/**
	 * Set optional contact parameters that will be added to the contact information sent in the registration, inside the URI.
	 * @param params a string containing the additional parameters in text form, like "myparam=something;myparam2=something_else"
	 *
	 * The main use case for this function is provide the proxy additional information regarding the user agent, like for example unique identifier or apple push id.
	 * As an example, the contact address in the SIP register sent will look like <sip:joe@15.128.128.93:50421;apple-push-id=43143-DFE23F-2323-FA2232>.
	**/
	public void setContactUriParameters(String params);

	/**
	 * Get the contact's URI parameters.
	 * @return
	 */
	public String getContactUriParameters();

	/**
	 * Return the international prefix for the given country
	 * @param iso code
	 */
	public int lookupCCCFromIso(String iso);

	/**
	 * Return the international prefix for the given country
	 * @param e164 phone number
	 */
	public int lookupCCCFromE164(String e164);

	/**
	 * Return reason error code.
	 * @return reason code.
	 */
	public Reason getError();

	/**
	 * Get full error information about last error occured on the proxy config.
	 * @return an ErrorInfo.
	 */
	public ErrorInfo getErrorInfo();

	/**
	 * Set the publish expiration time in second.
	 * @param expires in second
	 */
	public void setPublishExpires(int expires);
	/**
	 * @return the publish expiration time in second. Default value is the registration expiration value.
	 */
	public int getPublishExpires();

	/**
	 *  attached a user data to a proxy config
	 **/
	void setUserData(Object obj);

	/**
	 * Detect if the given input is a phone number or not.
	 * @param username string to parse.
	 * @return TRUE if input is a phone number, FALSE otherwise.
	 **/
	boolean isPhoneNumber(String username);

	/**
	 * Returns user data from a proxy config. return null if any
	 * @return an Object.
	 */
	Object getUserData();
}
