/*
Copyright (C) 2010-2015 Belledonne Communications SARL

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

#ifndef LINPHONE_PROXY_CONFIG_H
#define LINPHONE_PROXY_CONFIG_H

/**
 * @addtogroup proxies
 * @{
**/

/**
 * Creates an empty proxy config.
 * @deprecated, use #linphone_core_create_proxy_config instead
**/
LINPHONE_PUBLIC LinphoneProxyConfig *linphone_proxy_config_new(void);

/**
 * Acquire a reference to the proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The same proxy config.
**/
LINPHONE_PUBLIC LinphoneProxyConfig *linphone_proxy_config_ref(LinphoneProxyConfig *cfg);

/**
 * Release reference to the proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
**/
LINPHONE_PUBLIC void linphone_proxy_config_unref(LinphoneProxyConfig *cfg);

/**
 * Retrieve the user pointer associated with the proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The user pointer associated with the proxy config.
**/
LINPHONE_PUBLIC void *linphone_proxy_config_get_user_data(const LinphoneProxyConfig *cfg);

/**
 * Assign a user pointer to the proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] ud The user pointer to associate with the proxy config.
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_user_data(LinphoneProxyConfig *cfg, void *ud);

/**
 * Sets the proxy address
 *
 * Examples of valid sip proxy address are:
 * - IP address: sip:87.98.157.38
 * - IP address with port: sip:87.98.157.38:5062
 * - hostnames : sip:sip.example.net
**/
LINPHONE_PUBLIC int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *cfg, const char *server_addr);

/**
 * @deprecated Use linphone_proxy_config_set_identity_address()
**/
LINPHONE_PUBLIC	int linphone_proxy_config_set_identity(LinphoneProxyConfig *cfg, const char *identity);

/**
 * Sets the user identity as a SIP address.
 *
 * This identity is normally formed with display name, username and domain, such
 * as:
 * Alice <sip:alice@example.net>
 * The REGISTER messages will have from and to set to this identity.
 *
**/
LINPHONE_PUBLIC int linphone_proxy_config_set_identity_address(LinphoneProxyConfig *cfg, const LinphoneAddress *identity);

/**
 * Sets a SIP route.
 * When a route is set, all outgoing calls will go to the route's destination if this proxy
 * is the default one (see linphone_core_set_default_proxy() ).
 * @return -1 if route is invalid, 0 otherwise.
**/
LINPHONE_PUBLIC int linphone_proxy_config_set_route(LinphoneProxyConfig *cfg, const char *route);

/**
 * Sets the registration expiration time in seconds.
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_expires(LinphoneProxyConfig *cfg, int expires);

#define linphone_proxy_config_expires linphone_proxy_config_set_expires

/**
 * Indicates  either or not, REGISTRATION must be issued for this #LinphoneProxyConfig .
 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param val if true, registration will be engaged
 */
LINPHONE_PUBLIC void linphone_proxy_config_enable_register(LinphoneProxyConfig *cfg, bool_t val);

#define linphone_proxy_config_enableregister linphone_proxy_config_enable_register

/**
 * Starts editing a proxy configuration.
 *
 * Because proxy configuration must be consistent, applications MUST
 * call linphone_proxy_config_edit() before doing any attempts to modify
 * proxy configuration (such as identity, proxy address and so on).
 * Once the modifications are done, then the application must call
 * linphone_proxy_config_done() to commit the changes.
**/
LINPHONE_PUBLIC void linphone_proxy_config_edit(LinphoneProxyConfig *cfg);

/**
 * Commits modification made to the proxy configuration.
**/
LINPHONE_PUBLIC int linphone_proxy_config_done(LinphoneProxyConfig *cfg);

/**
 * Indicates  either or not, PUBLISH must be issued for this #LinphoneProxyConfig .
 * <br> In case this #LinphoneProxyConfig has been added to #LinphoneCore, follows the linphone_proxy_config_edit() rule.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param val if true, publish will be engaged
 *
 */
LINPHONE_PUBLIC void linphone_proxy_config_enable_publish(LinphoneProxyConfig *cfg, bool_t val);

/**
 * Set the publish expiration time in second.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param expires in second
 * */
LINPHONE_PUBLIC void linphone_proxy_config_set_publish_expires(LinphoneProxyConfig *cfg, int expires);

/**
 * get the publish expiration time in second. Default value is the registration expiration value.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return expires in second
 * */
LINPHONE_PUBLIC int linphone_proxy_config_get_publish_expires(const LinphoneProxyConfig *cfg);

/**
 * Sets whether liblinphone should replace "+" by international calling prefix in dialed numbers (passed to
 * #linphone_core_invite ).
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val);

/**
 * Sets a dialing prefix to be automatically prepended when inviting a number with
 * linphone_core_invite();
 * This dialing prefix shall usually be the country code of the country where the user is living, without "+".
 *
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix);

 /**
 * Indicates whether quality statistics during call should be stored and sent to a collector according to RFC 6035.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] enable True to sotre quality statistics and sent them to the collector, false to disable it.
 */
LINPHONE_PUBLIC void linphone_proxy_config_enable_quality_reporting(LinphoneProxyConfig *cfg, bool_t enable);

/**
 * Indicates whether quality statistics during call should be stored and sent to a collector according to RFC 6035.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return True if quality repotring is enabled, false otherwise.
 */
LINPHONE_PUBLIC bool_t linphone_proxy_config_quality_reporting_enabled(LinphoneProxyConfig *cfg);

 /**
 * Set the route of the collector end-point when using quality reporting. This SIP address
 * should be used on server-side to process packets directly before discarding packets. Collector address
 * should be a non existing account and will not receive any messages.
 * If NULL, reports will be send to the proxy domain.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] collector route of the collector end-point, if NULL PUBLISH will be sent to the proxy domain.
 */
LINPHONE_PUBLIC void linphone_proxy_config_set_quality_reporting_collector(LinphoneProxyConfig *cfg, const char *collector);

 /**
 * Get the route of the collector end-point when using quality reporting. This SIP address
 * should be used on server-side to process packets directly before discarding packets. Collector address
 * should be a non existing account and will not receive any messages.
 * If NULL, reports will be send to the proxy domain.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The SIP address of the collector end-point.
 */
LINPHONE_PUBLIC const char *linphone_proxy_config_get_quality_reporting_collector(const LinphoneProxyConfig *cfg);

/**
 * Set the interval between 2 interval reports sending when using quality reporting. If call exceed interval size, an
 * interval report will be sent to the collector. On call termination, a session report will be sent
 * for the remaining period. Value must be 0 (disabled) or positive.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] interval The interval in seconds, 0 means interval reports are disabled.
 */
LINPHONE_PUBLIC void linphone_proxy_config_set_quality_reporting_interval(LinphoneProxyConfig *cfg, int interval);

/**
 * Get the interval between interval reports when using quality reporting.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The interval in seconds, 0 means interval reports are disabled.
 */

LINPHONE_PUBLIC int linphone_proxy_config_get_quality_reporting_interval(LinphoneProxyConfig *cfg);

/**
 * Get the registration state of the given proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The registration state of the proxy config.
**/
LINPHONE_PUBLIC LinphoneRegistrationState linphone_proxy_config_get_state(const LinphoneProxyConfig *cfg);

/**
 * @return a boolean indicating that the user is sucessfully registered on the proxy.
 * @deprecated Use linphone_proxy_config_get_state() instead.
**/
LINPHONE_PUBLIC bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *cfg);

/**
 * Get the domain name of the given proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The domain name of the proxy config.
**/
LINPHONE_PUBLIC const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg);

/**
 * Get the realm of the given proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The realm of the proxy config.
**/
LINPHONE_PUBLIC const char *linphone_proxy_config_get_realm(const LinphoneProxyConfig *cfg);

/**
 * Set the realm of the given proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] realm New realm value.
 * @return The realm of the proxy config.
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_realm(LinphoneProxyConfig *cfg, const char * realm);

/**
 * @return the route set for this proxy configuration.
**/
LINPHONE_PUBLIC const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *cfg);

/**
 * @return the SIP identity that belongs to this proxy configuration.
**/
LINPHONE_PUBLIC const LinphoneAddress *linphone_proxy_config_get_identity_address(const LinphoneProxyConfig *cfg);

/**
 * @deprecated use linphone_proxy_config_get_identity_address()
**/
LINPHONE_PUBLIC const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *cfg);

/**
 * @return TRUE if PUBLISH request is enabled for this proxy.
**/
LINPHONE_PUBLIC bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *cfg);

/**
 * @return the proxy's SIP address.
**/
LINPHONE_PUBLIC	const char *linphone_proxy_config_get_server_addr(const LinphoneProxyConfig *cfg);

#define linphone_proxy_config_get_addr linphone_proxy_config_get_server_addr

/**
 * @return the duration of registration.
**/
LINPHONE_PUBLIC int linphone_proxy_config_get_expires(const LinphoneProxyConfig *cfg);

/**
 * @return TRUE if registration to the proxy is enabled.
**/
LINPHONE_PUBLIC bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *cfg);

/**
 * Refresh a proxy registration.
 * This is useful if for example you resuming from suspend, thus IP address may have changed.
**/
LINPHONE_PUBLIC void linphone_proxy_config_refresh_register(LinphoneProxyConfig *cfg);

/**
 * Prevent a proxy config from refreshing its registration.
 * This is useful to let registrations to expire naturally (or) when the application wants to keep control on when
 * refreshes are sent.
 * However, linphone_core_set_network_reachable(lc,TRUE) will always request the proxy configs to refresh their registrations.
 * The refreshing operations can be resumed with linphone_proxy_config_refresh_register().
 * @param[in] cfg #LinphoneProxyConfig object.
**/
LINPHONE_PUBLIC void linphone_proxy_config_pause_register(LinphoneProxyConfig *cfg);

LINPHONE_PUBLIC const LinphoneAddress* linphone_proxy_config_get_contact(const LinphoneProxyConfig *cfg);

/**
 * @return previously set contact parameters.
**/
LINPHONE_PUBLIC const char *linphone_proxy_config_get_contact_parameters(const LinphoneProxyConfig *cfg);

/**
 * Set optional contact parameters that will be added to the contact information sent in the registration.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param contact_params a string contaning the additional parameters in text form, like "myparam=something;myparam2=something_else"
 *
 * The main use case for this function is provide the proxy additional information regarding the user agent, like for example unique identifier or apple push id.
 * As an example, the contact address in the SIP register sent will look like <sip:joe@15.128.128.93:50421>;apple-push-id=43143-DFE23F-2323-FA2232.
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_contact_parameters(LinphoneProxyConfig *cfg, const char *contact_params);

/**
 * Set optional contact parameters that will be added to the contact information sent in the registration, inside the URI.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param contact_uri_params a string containing the additional parameters in text form, like "myparam=something;myparam2=something_else"
 *
 * The main use case for this function is provide the proxy additional information regarding the user agent, like for example unique identifier or apple push id.
 * As an example, the contact address in the SIP register sent will look like <sip:joe@15.128.128.93:50421;apple-push-id=43143-DFE23F-2323-FA2232>.
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_contact_uri_parameters(LinphoneProxyConfig *cfg, const char *contact_uri_params);

/**
 * @return previously set contact URI parameters.
**/
LINPHONE_PUBLIC const char* linphone_proxy_config_get_contact_uri_parameters(const LinphoneProxyConfig *cfg);

/**
 * Get the #LinphoneCore object to which is associated the #LinphoneProxyConfig.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The #LinphoneCore object to which is associated the #LinphoneProxyConfig.
**/
LINPHONE_PUBLIC LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *cfg);

/**
 * @return whether liblinphone should replace "+" by "00" in dialed numbers (passed to
 * #linphone_core_invite ).
 *
**/
LINPHONE_PUBLIC bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg);

/**
 * @return dialing prefix.
**/
LINPHONE_PUBLIC	const char * linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg);

/**
 * Get the reason why registration failed when the proxy config state is LinphoneRegistrationFailed.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The reason why registration failed for this proxy config.
**/
LINPHONE_PUBLIC LinphoneReason linphone_proxy_config_get_error(const LinphoneProxyConfig *cfg);

/**
 * Get detailed information why registration failed when the proxy config state is LinphoneRegistrationFailed.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The details why registration failed for this proxy config.
**/
LINPHONE_PUBLIC const LinphoneErrorInfo *linphone_proxy_config_get_error_info(const LinphoneProxyConfig *cfg);

/**
 * Get the transport from either service route, route or addr.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The transport as a string (I.E udp, tcp, tls, dtls)
**/
LINPHONE_PUBLIC const char* linphone_proxy_config_get_transport(const LinphoneProxyConfig *cfg);

/**
 * Destroys a proxy config.
 * @deprecated
 *
 * @note: LinphoneProxyConfig that have been removed from LinphoneCore with
 * linphone_core_remove_proxy_config() must not be freed.
**/
LINPHONE_PUBLIC void linphone_proxy_config_destroy(LinphoneProxyConfig *cfg);

LINPHONE_PUBLIC void linphone_proxy_config_set_sip_setup(LinphoneProxyConfig *cfg, const char *type);
LINPHONE_PUBLIC SipSetupContext *linphone_proxy_config_get_sip_setup_context(LinphoneProxyConfig *cfg);
LINPHONE_PUBLIC SipSetup *linphone_proxy_config_get_sip_setup(LinphoneProxyConfig *cfg);

/**
 * Detect if the given input is a phone number or not.
 * @param proxy #LinphoneProxyConfig argument, unused yet but may contain useful data. Can be NULL.
 * @param username string to parse.
 * @return TRUE if input is a phone number, FALSE otherwise.
**/
LINPHONE_PUBLIC bool_t linphone_proxy_config_is_phone_number(LinphoneProxyConfig *proxy, const char *username);

/**
 * See linphone_proxy_config_normalize_phone_number
 * @param proxy #LinphoneProxyConfig object containing country code and/or escape symbol. If NULL passed, will use default configuration.
 * @param username the string to parse
 * @param result the newly normalized number
 * @param result_len the size of the normalized number \a result
 * @return TRUE if a phone number was recognized, FALSE otherwise.
 * @deprecated use linphone_proxy_config_normalize_phone_number()
 */
LINPHONE_PUBLIC LINPHONE_DEPRECATED bool_t linphone_proxy_config_normalize_number(LinphoneProxyConfig *proxy, const char *username, char *result, size_t result_len);

/**
 * Normalize a human readable phone number into a basic string. 888-444-222 becomes 888444222
 * or +33888444222 depending on the #LinphoneProxyConfig object.
 * This function will always generate a normalized username if input is a phone number.
 * @param proxy #LinphoneProxyConfig object containing country code and/or escape symbol. If NULL passed, will use default configuration.
 * @param username the string to parse
 * @return NULL if input is an invalid phone number, normalized phone number from username input otherwise.
*/
LINPHONE_PUBLIC char* linphone_proxy_config_normalize_phone_number(LinphoneProxyConfig *proxy, const char *username);

/**
 * Normalize a human readable sip uri into a fully qualified LinphoneAddress.
 * A sip address should look like DisplayName \<sip:username\@domain:port\> .
 * Basically this function performs the following tasks
 * - if a phone number is entered, prepend country prefix and eventually escape the '+' by 00 of the proxy config.
 * - if no domain part is supplied, append the domain name of the proxy config. Returns NULL if no proxy is provided at this point.
 * - if no sip: is present, prepend it.
 *
 * The result is a syntactically correct SIP address.
 * @param proxy #LinphoneProxyConfig object containing country code, escape symbol and/or domain name. Can be NULL if domain is already provided.
 * @param username the string to parse
 * @return NULL if invalid input, normalized sip address otherwise.
*/
LINPHONE_PUBLIC LinphoneAddress* linphone_proxy_config_normalize_sip_uri(LinphoneProxyConfig *proxy, const char *username);

/**
 * Set default privacy policy for all calls routed through this proxy.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param privacy LinphonePrivacy to configure privacy
 * */
LINPHONE_PUBLIC void linphone_proxy_config_set_privacy(LinphoneProxyConfig *cfg, LinphonePrivacyMask privacy);

/**
 * Get default privacy policy for all calls routed through this proxy.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return Privacy mode
 * */
LINPHONE_PUBLIC LinphonePrivacyMask linphone_proxy_config_get_privacy(const LinphoneProxyConfig *cfg);

/**
 * Set the http file transfer server to be used for content type application/vnd.gsma.rcs-ft-http+xml
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param server_url URL of the file server like https://file.linphone.org/upload.php
 * @warning That function isn't implemented yet.
 * */
LINPHONE_PUBLIC void linphone_proxy_config_set_file_transfer_server(LinphoneProxyConfig *cfg, const char * server_url);

/**
 * Get the http file transfer server to be used for content type application/vnd.gsma.rcs-ft-http+xml
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return URL of the file server like https://file.linphone.org/upload.php
 * @warning That function isn't implemented yet.
 * */
LINPHONE_PUBLIC const char* linphone_proxy_config_get_file_transfer_server(const LinphoneProxyConfig *cfg);

/**
 * Indicates whether AVPF/SAVPF must be used for calls using this proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] enable True to enable AVPF/SAVF, false to disable it.
 * @deprecated use linphone_proxy_config_set_avpf_mode()
 */
LINPHONE_PUBLIC void linphone_proxy_config_enable_avpf(LinphoneProxyConfig *cfg, bool_t enable);

/**
 * Indicates whether AVPF/SAVPF is being used for calls using this proxy config.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return True if AVPF/SAVPF is enabled, false otherwise.
 * @deprecated use linphone_proxy_config_set_avpf_mode()
 */
LINPHONE_PUBLIC bool_t linphone_proxy_config_avpf_enabled(LinphoneProxyConfig *cfg);

/**
 * Set the interval between regular RTCP reports when using AVPF/SAVPF.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] interval The interval in seconds (between 0 and 5 seconds).
 */
LINPHONE_PUBLIC void linphone_proxy_config_set_avpf_rr_interval(LinphoneProxyConfig *cfg, uint8_t interval);

/**
 * Get the interval between regular RTCP reports when using AVPF/SAVPF.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The interval in seconds.
 */
LINPHONE_PUBLIC uint8_t linphone_proxy_config_get_avpf_rr_interval(const LinphoneProxyConfig *cfg);

/**
 * Get enablement status of RTCP feedback (also known as AVPF profile).
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return the enablement mode, which can be LinphoneAVPFDefault (use LinphoneCore's mode), LinphoneAVPFEnabled (avpf is enabled), or LinphoneAVPFDisabled (disabled).
**/
LINPHONE_PUBLIC LinphoneAVPFMode linphone_proxy_config_get_avpf_mode(const LinphoneProxyConfig *cfg);

/**
 * Enable the use of RTCP feedback (also known as AVPF profile).
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] mode the enablement mode, which can be LinphoneAVPFDefault (use LinphoneCore's mode), LinphoneAVPFEnabled (avpf is enabled), or LinphoneAVPFDisabled (disabled).
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_avpf_mode(LinphoneProxyConfig *cfg, LinphoneAVPFMode mode);

/**
 * Obtain the value of a header sent by the server in last answer to REGISTER.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param header_name the header name for which to fetch corresponding value
 * @return the value of the queried header.
**/
LINPHONE_PUBLIC const char *linphone_proxy_config_get_custom_header(LinphoneProxyConfig *cfg, const char *header_name);

/**
 * Set the value of a custom header sent to the server in REGISTERs request.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param header_name the header name
 * @param header_value the header's value
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_custom_header(LinphoneProxyConfig *cfg, const char *header_name, const char *header_value);

/**
 * Find authentication info matching proxy config, if any, similarly to linphone_core_find_auth_info.
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return a #LinphoneAuthInfo matching proxy config criteria if possible, NULL if nothing can be found.
**/
LINPHONE_PUBLIC const LinphoneAuthInfo* linphone_proxy_config_find_auth_info(const LinphoneProxyConfig *cfg);


/**
 * Get the persistent reference key associated to the proxy config.
 *
 * The reference key can be for example an id to an external database.
 * It is stored in the config file, thus can survive to process exits/restarts.
 *
 * @param[in] cfg #LinphoneProxyConfig object.
 * @return The reference key string that has been associated to the proxy config, or NULL if none has been associated.
**/
LINPHONE_PUBLIC const char * linphone_proxy_config_get_ref_key(const LinphoneProxyConfig *cfg);

/**
 * Associate a persistent reference key to the proxy config.
 *
 * The reference key can be for example an id to an external database.
 * It is stored in the config file, thus can survive to process exits/restarts.
 *
 * @param[in] cfg #LinphoneProxyConfig object.
 * @param[in] refkey The reference key string to associate to the proxy config.
**/
LINPHONE_PUBLIC void linphone_proxy_config_set_ref_key(LinphoneProxyConfig *cfg, const char *refkey);

/**
 * Get The policy that is used to pass through NATs/firewalls when using this proxy config.
 * If it is set to NULL, the default NAT policy from the core will be used instead.
 * @param[in] cfg #LinphoneProxyConfig object
 * @return LinphoneNatPolicy object in use.
 * @see linphone_core_get_nat_policy()
 */
LINPHONE_PUBLIC LinphoneNatPolicy * linphone_proxy_config_get_nat_policy(const LinphoneProxyConfig *cfg);

/**
 * Set the policy to use to pass through NATs/firewalls when using this proxy config.
 * If it is set to NULL, the default NAT policy from the core will be used instead.
 * @param[in] cfg #LinphoneProxyConfig object
 * @param[in] policy LinphoneNatPolicy object
 * @see linphone_core_set_nat_policy()
 */
LINPHONE_PUBLIC void linphone_proxy_config_set_nat_policy(LinphoneProxyConfig *cfg, LinphoneNatPolicy *policy);

/**
 * @}
 */

#endif
