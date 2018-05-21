/*
 * c-address.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _L_C_ADDRESS_H_
#define _L_C_ADDRESS_H_

#include "linphone/api/c-types.h"

// =============================================================================

#ifdef __cplusplus
	extern "C" {
#endif // ifdef __cplusplus

/**
 * @addtogroup linphone_address
 * @{
 */

/**
 * Constructs a #LinphoneAddress object by parsing the user supplied address,
 * given as a string.
 **/
LINPHONE_PUBLIC LinphoneAddress *linphone_address_new (const char *address);

/**
 * Clones a #LinphoneAddress object.
 **/
LINPHONE_PUBLIC LinphoneAddress *linphone_address_clone (const LinphoneAddress *address);

/**
 * Increment reference count of #LinphoneAddress object.
 **/
LINPHONE_PUBLIC LinphoneAddress *linphone_address_ref (LinphoneAddress *address);

/**
 * Decrement reference count of #LinphoneAddress object. When dropped to zero, memory is freed.
 **/
LINPHONE_PUBLIC void linphone_address_unref (LinphoneAddress *address);

/**
 * Returns the address scheme, normally "sip".
 **/
LINPHONE_PUBLIC const char *linphone_address_get_scheme (const LinphoneAddress *address);

/**
 * Returns the display name.
 **/
LINPHONE_PUBLIC const char *linphone_address_get_display_name (const LinphoneAddress *address);

/**
 * Sets the display name.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_display_name (LinphoneAddress *address, const char *display_name);

/**
 * Returns the username.
 **/
LINPHONE_PUBLIC const char *linphone_address_get_username (const LinphoneAddress *address);

/**
 * Sets the username.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_username (LinphoneAddress *address, const char *username);

/**
 * Returns the domain name.
 **/
LINPHONE_PUBLIC const char *linphone_address_get_domain (const LinphoneAddress *address);

/**
 * Sets the domain.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_domain (LinphoneAddress *address, const char *domain);

/**
 * Get port number as an integer value, 0 if not present.
 */
LINPHONE_PUBLIC int linphone_address_get_port (const LinphoneAddress *address);

/**
 * Sets the port number.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_port (LinphoneAddress *address, int port);

/**
 * Get the transport.
 **/
LINPHONE_PUBLIC LinphoneTransportType linphone_address_get_transport (const LinphoneAddress *address);

/**
 * Set a transport.
 **/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_transport (LinphoneAddress *address, LinphoneTransportType transport);

/**
 * Returns true if address refers to a secure location (sips)
 **/
LINPHONE_PUBLIC bool_t linphone_address_get_secure (const LinphoneAddress *address);

/**
 * Make the address refer to a secure location (sips scheme)
 * @param[in] address A #LinphoneAddress object
 * @param[in] enabled TRUE if address is requested to be secure.
 **/
LINPHONE_PUBLIC void linphone_address_set_secure (LinphoneAddress *address, bool_t enabled);

/**
 * returns true if address is a routable sip address
 */
LINPHONE_PUBLIC bool_t linphone_address_is_sip (const LinphoneAddress *address);

/**
 * Get the value of the method parameter
 **/
LINPHONE_PUBLIC const char *linphone_address_get_method_param (const LinphoneAddress *address);

/**
 * Set the value of the method parameter
 **/
LINPHONE_PUBLIC void linphone_address_set_method_param (LinphoneAddress *address, const char *method_param);

/**
 * Get the password encoded in the address.
 * It is used for basic authentication (not recommended).
 * @param address the address
 * @return the password, if any, NULL otherwise.
 **/
LINPHONE_PUBLIC const char *linphone_address_get_password (const LinphoneAddress *address);

/**
 * Set the password encoded in the address.
 * It is used for basic authentication (not recommended).
 * @param address the #LinphoneAddress
 * @param password the password to set.
 **/
LINPHONE_PUBLIC void linphone_address_set_password (LinphoneAddress *address, const char *password);

/**
 * Removes address's tags and uri headers so that it is displayable to the user.
 **/
LINPHONE_PUBLIC void linphone_address_clean (LinphoneAddress *address);

/**
 * Returns the address as a string.
 * The returned char * must be freed by the application. Use ms_free().
 **/
LINPHONE_PUBLIC char *linphone_address_as_string (const LinphoneAddress *address);

/**
 * Returns the SIP uri only as a string, that is display name is removed.
 * The returned char * must be freed by the application. Use ms_free().
 **/
LINPHONE_PUBLIC char *linphone_address_as_string_uri_only (const LinphoneAddress *address);

/**
 * Compare two #LinphoneAddress ignoring tags and headers, basically just domain, username, and port.
 * @param[in] address1 #LinphoneAddress object
 * @param[in] address2 #LinphoneAddress object
 * @return Boolean value telling if the #LinphoneAddress objects are equal.
 * @see linphone_address_equal()
 **/
LINPHONE_PUBLIC bool_t linphone_address_weak_equal (const LinphoneAddress *address1, const LinphoneAddress *address2);

/**
 * Compare two #LinphoneAddress taking the tags and headers into account.
 * @param[in] address1 #LinphoneAddress object
 * @param[in] address2 #LinphoneAddress object
 * @return Boolean value telling if the #LinphoneAddress objects are equal.
 * @see linphone_address_weak_equal()
 */
LINPHONE_PUBLIC bool_t linphone_address_equal (const LinphoneAddress *address1, const LinphoneAddress *address2);

/**
 * Get the header encoded in the address.
 * @param header_name the header name
**/
LINPHONE_PUBLIC const char *linphone_address_get_header (const LinphoneAddress *address, const char *header_name);

/**
 * Set a header into the address.
 * Headers appear in the URI with '?', such as \<sip:test@linphone.org?SomeHeader=SomeValue\>.
 * @param address the address
 * @param header_name the header name
 * @param header_value the header value
 **/
LINPHONE_PUBLIC void linphone_address_set_header (LinphoneAddress *address, const char *header_name, const char *header_value);

/**
 * Tell whether a parameter is present in the address
 * @param[in] address #LinphoneAddress object
 * @param[in] param_name The name of the parameter
 * @return A boolean value telling whether the parameter is present in the address
 */
LINPHONE_PUBLIC bool_t linphone_address_has_param (const LinphoneAddress *address, const char *param_name);

/**
 * Get the value of a parameter of the address
 * @param[in] address #LinphoneAddress object
 * @param[in] param_name The name of the parameter
 * @return The value of the parameter
 */
LINPHONE_PUBLIC const char *linphone_address_get_param (const LinphoneAddress *address, const char *param_name);

/**
 * Set the value of a parameter of the address
 * @param[in] address #LinphoneAddress object
 * @param[in] param_name The name of the parameter
 * @param[in] param_value The new value of the parameter
 */
LINPHONE_PUBLIC void linphone_address_set_param (LinphoneAddress *address, const char *param_name, const char *param_value);

LINPHONE_PUBLIC void linphone_address_set_params (LinphoneAddress *address, const char *params);

/**
 * Tell whether a parameter is present in the URI of the address
 * @param[in] address #LinphoneAddress object
 * @param[in] uri_param_name The name of the parameter
 * @return A boolean value telling whether the parameter is present in the URI of the address
 */
LINPHONE_PUBLIC bool_t linphone_address_has_uri_param (const LinphoneAddress *address, const char *uri_param_name);

/**
 * Get the value of a parameter of the URI of the address
 * @param[in] address #LinphoneAddress object
 * @param[in] uri_param_name The name of the parameter
 * @return The value of the parameter
 */
LINPHONE_PUBLIC const char *linphone_address_get_uri_param (const LinphoneAddress *address, const char *uri_param_name);

/**
 * Set the value of a parameter of the URI of the address
 * @param[in] address #LinphoneAddress object
 * @param[in] uri_param_name The name of the parameter
 * @param[in] uri_param_value The new value of the parameter
 */
LINPHONE_PUBLIC void linphone_address_set_uri_param (LinphoneAddress *address, const char *uri_param_name, const char *uri_param_value);

LINPHONE_PUBLIC void linphone_address_set_uri_params (LinphoneAddress *address, const char *params);

/**
 * Removes the value of a parameter of the URI of the address
 * @param[in] address #LinphoneAddress object
 * @param[in] uri_param_name The name of the parameter
 */
LINPHONE_PUBLIC void linphone_address_remove_uri_param (LinphoneAddress *address, const char *uri_param_name);

/**
 * Destroys a #LinphoneAddress object (actually calls linphone_address_unref()).
 * @deprecated Use linphone_address_unref() instead
 * @donotwrap
 **/
LINPHONE_DEPRECATED LINPHONE_PUBLIC void linphone_address_destroy (LinphoneAddress *address);

/**
 * Returns true if address refers to a secure location (sips)
 * @deprecated use linphone_address_get_secure()
 * @donotwrap
 **/
LINPHONE_DEPRECATED LINPHONE_PUBLIC bool_t linphone_address_is_secure (const LinphoneAddress *address);

/**
 * @}
 */

#ifdef __cplusplus
	}
#endif // ifdef __cplusplus

#endif // ifndef _L_C_ADDRESS_H_
