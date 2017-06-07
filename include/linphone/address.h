/*
address.h
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

#ifndef LINPHONE_ADDRESS_H
#define LINPHONE_ADDRESS_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup linphone_address
 * @{
 */

/**
 * Constructs a LinphoneAddress object by parsing the user supplied address,
 * given as a string.
**/
LINPHONE_PUBLIC LinphoneAddress * linphone_address_new(const char *addr);

/**
 * Clones a LinphoneAddress object.
**/
LINPHONE_PUBLIC LinphoneAddress * linphone_address_clone(const LinphoneAddress *addr);

/**
 * Increment reference count of LinphoneAddress object.
**/
LINPHONE_PUBLIC LinphoneAddress * linphone_address_ref(LinphoneAddress *addr);

/**
 * Decrement reference count of LinphoneAddress object. When dropped to zero, memory is freed.
**/
LINPHONE_PUBLIC void linphone_address_unref(LinphoneAddress *addr);

/**
 * Returns the address scheme, normally "sip".
**/
LINPHONE_PUBLIC const char *linphone_address_get_scheme(const LinphoneAddress *u);

/**
 * Returns the display name.
**/
LINPHONE_PUBLIC const char *linphone_address_get_display_name(const LinphoneAddress* u);

/**
 * Returns the username.
**/
LINPHONE_PUBLIC const char *linphone_address_get_username(const LinphoneAddress *u);

/**
 * Returns the domain name.
**/
LINPHONE_PUBLIC const char *linphone_address_get_domain(const LinphoneAddress *u);

/**
 * Get port number as an integer value, 0 if not present.
 */
LINPHONE_PUBLIC int linphone_address_get_port(const LinphoneAddress *u);

/**
 * Sets the display name.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_display_name(LinphoneAddress *u, const char *display_name);

/**
 * Sets the username.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_username(LinphoneAddress *uri, const char *username);

/**
 * Sets the domain.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_domain(LinphoneAddress *uri, const char *host);

/**
 * Sets the port number.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_port(LinphoneAddress *uri, int port);

/**
 * Set a transport.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_address_set_transport(LinphoneAddress *uri,LinphoneTransportType type);

/**
 * Removes address's tags and uri headers so that it is displayable to the user.
**/
LINPHONE_PUBLIC void linphone_address_clean(LinphoneAddress *uri);

/**
 * Returns true if address refers to a secure location (sips)
 * @deprecated use linphone_address_get_secure()
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC bool_t linphone_address_is_secure(const LinphoneAddress *addr);

/**
 * Returns true if address refers to a secure location (sips)
**/
LINPHONE_PUBLIC bool_t linphone_address_get_secure(const LinphoneAddress *addr);

/**
 * Make the address refer to a secure location (sips scheme)
 * @param[in] addr A #LinphoneAddress object
 * @param[in] enabled TRUE if address is requested to be secure.
**/
LINPHONE_PUBLIC void linphone_address_set_secure(LinphoneAddress *addr, bool_t enabled);

/**
 * returns true if address is a routable sip address
 */
LINPHONE_PUBLIC bool_t linphone_address_is_sip(const LinphoneAddress *uri);

/**
 * Get the transport.
**/
LINPHONE_PUBLIC LinphoneTransportType linphone_address_get_transport(const LinphoneAddress *uri);

/**
 * Get the value of the method parameter
**/
LINPHONE_PUBLIC const char *linphone_address_get_method_param(const LinphoneAddress *addr);

/**
 * Set the value of the method parameter
**/
LINPHONE_PUBLIC void linphone_address_set_method_param(LinphoneAddress *addr, const char *method);

/**
 * Returns the address as a string.
 * The returned char * must be freed by the application. Use ms_free().
**/
LINPHONE_PUBLIC	char *linphone_address_as_string(const LinphoneAddress *u);

/**
 * Returns the SIP uri only as a string, that is display name is removed.
 * The returned char * must be freed by the application. Use ms_free().
**/
LINPHONE_PUBLIC	char *linphone_address_as_string_uri_only(const LinphoneAddress *u);

/**
 * Compare two LinphoneAddress ignoring tags and headers, basically just domain, username, and port.
 * @param[in] a1 LinphoneAddress object
 * @param[in] a2 LinphoneAddress object
 * @return Boolean value telling if the LinphoneAddress objects are equal.
 * @see linphone_address_equal()
**/
LINPHONE_PUBLIC	bool_t linphone_address_weak_equal(const LinphoneAddress *a1, const LinphoneAddress *a2);

/**
 * Compare two LinphoneAddress taking the tags and headers into account.
 * @param[in] a1 LinphoneAddress object
 * @param[in] a2 LinphoneAddress object
 * @return Boolean value telling if the LinphoneAddress objects are equal.
 * @see linphone_address_weak_equal()
 */
LINPHONE_PUBLIC bool_t linphone_address_equal(const LinphoneAddress *a1, const LinphoneAddress *a2);

/**
 * Set the password encoded in the address.
 * It is used for basic authentication (not recommended).
 * @param addr the LinphoneAddress
 * @param passwd the password to set.
**/
LINPHONE_PUBLIC void linphone_address_set_password(LinphoneAddress *addr, const char *passwd);

/**
 * Get the password encoded in the address.
 * It is used for basic authentication (not recommended).
 * @param addr the address
 * @return the password, if any, NULL otherwise.
**/
LINPHONE_PUBLIC const char *linphone_address_get_password(const LinphoneAddress *addr);

/**
 * Set a header into the address.
 * Headers appear in the URI with '?', such as \<sip:test@linphone.org?SomeHeader=SomeValue\>.
 * @param addr the address
 * @param header_name the header name
 * @param header_value the header value
**/
LINPHONE_PUBLIC void linphone_address_set_header(LinphoneAddress *addr, const char *header_name, const char *header_value);

/**
 * Get the header encoded in the address.
 * @param addr the address
**/
LINPHONE_PUBLIC const char * linphone_address_get_header(const LinphoneAddress *addr, const char *name);

LINPHONE_PUBLIC bool_t linphone_address_has_param(const LinphoneAddress *addr, const char *name);

LINPHONE_PUBLIC const char * linphone_address_get_param(const LinphoneAddress *addr, const char *name);

LINPHONE_PUBLIC void linphone_address_set_param(LinphoneAddress *addr, const char *name, const char *value);

LINPHONE_PUBLIC void linphone_address_set_params(LinphoneAddress *addr, const char *params);

LINPHONE_PUBLIC void linphone_address_set_uri_param(LinphoneAddress *addr, const char *name, const char *value);

LINPHONE_PUBLIC void linphone_address_set_uri_params(LinphoneAddress *addr, const char *params);

LINPHONE_PUBLIC bool_t linphone_address_has_uri_param(const LinphoneAddress *addr, const char *name);

LINPHONE_PUBLIC const char * linphone_address_get_uri_param(const LinphoneAddress *addr, const char *name);

/**
 * Destroys a LinphoneAddress object (actually calls linphone_address_unref()).
 * @deprecated Use linphone_address_unref() instead
 * @donotwrap
**/
LINPHONE_DEPRECATED LINPHONE_PUBLIC	void linphone_address_destroy(LinphoneAddress *u);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_ADDRESS_H */
