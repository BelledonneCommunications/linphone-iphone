/*
auth_info.h
Copyright (C) 2016  Belledonne Communications SARL

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

#ifndef LINPHONE_AUTH_INFO_H
#define LINPHONE_AUTH_INFO_H

#include <mediastreamer2/mscommon.h>

#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

/**
 * @addtogroup authentication
 * @{
 */

/**
 * Object holding authentication information.
 *
 * @note The object's fields should not be accessed directly. Prefer using
 * the accessor methods.
 *
 * In most case, authentication information consists of a username and password.
 * Sometimes, a userid is required by proxy, and realm can be useful to discriminate
 * different SIP domains.
 *
 * Once created and filled, a LinphoneAuthInfo must be added to the LinphoneCore in
 * order to become known and used automatically when needed.
 * Use linphone_core_add_auth_info() for that purpose.
 *
 * The LinphoneCore object can take the initiative to request authentication information
 * when needed to the application through the auth_info_requested callback of the
 * LinphoneCoreVTable structure.
 *
 * The application can respond to this information request later using
 * linphone_core_add_auth_info(). This will unblock all pending authentication
 * transactions and retry them with authentication headers.
 *
**/
typedef struct _LinphoneAuthInfo LinphoneAuthInfo;

/**
 * Safely cast a belle_sip_object_t into LinphoneAuthInfo
 */
#define LINPHONE_AUTH_INFO(obj) BELLE_SIP_CAST(obj, LinphoneAuthInfo)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a #LinphoneAuthInfo object with supplied information.
 * The object can be created empty, that is with all arguments set to NULL.
 * Username, userid, password, realm and domain can be set later using specific methods.
 * At the end, username and passwd (or ha1) are required.
 * @param username The username that needs to be authenticated
 * @param userid The userid used for authenticating (use NULL if you don't know what it is)
 * @param passwd The password in clear text
 * @param ha1 The ha1-encrypted password if password is not given in clear text.
 * @param realm The authentication domain (which can be larger than the sip domain. Unfortunately many SIP servers don't use this parameter.
 * @param domain The SIP domain for which this authentication information is valid, if it has to be restricted for a single SIP domain.
 * @return A #LinphoneAuthInfo object. linphone_auth_info_destroy() must be used to destroy it when no longer needed. The LinphoneCore makes a copy of LinphoneAuthInfo
 * passed through linphone_core_add_auth_info().
**/
LINPHONE_PUBLIC LinphoneAuthInfo *linphone_auth_info_new(const char *username, const char *userid,
	const char *passwd, const char *ha1,const char *realm, const char *domain);

/**
 * Instantiates a new auth info with values from source.
 * @param[in] source The #LinphoneAuthInfo object to be cloned
 * @return The newly created #LinphoneAuthInfo object.
 */
LINPHONE_PUBLIC LinphoneAuthInfo *linphone_auth_info_clone(const LinphoneAuthInfo* source);

/**
 * Sets the password.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] passwd The password.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_passwd(LinphoneAuthInfo *info, const char *passwd);

/**
 * Sets the username.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] username The username.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_username(LinphoneAuthInfo *info, const char *username);

/**
 * Sets the userid.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] userid The userid.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_userid(LinphoneAuthInfo *info, const char *userid);

/**
 * Sets the realm.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] realm The realm.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_realm(LinphoneAuthInfo *info, const char *realm);

/**
 * Sets the domain for which this authentication is valid.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] domain The domain.
 * This should not be necessary because realm is supposed to be unique and sufficient.
 * However, many SIP servers don't set realm correctly, then domain has to be used to distinguish between several SIP account bearing the same username.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_domain(LinphoneAuthInfo *info, const char *domain);

/**
 * Sets the ha1.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] ha1 The ha1.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_ha1(LinphoneAuthInfo *info, const char *ha1);

/**
 * Sets the TLS certificate.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] tls_cert The TLS certificate.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_tls_cert(LinphoneAuthInfo *info, const char *tls_cert);

/**
 * Sets the TLS key.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] tls_key The TLS key.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_tls_key(LinphoneAuthInfo *info, const char *tls_key);

/**
 * Sets the TLS certificate path.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] tls_cert_path The TLS certificate path.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_tls_cert_path(LinphoneAuthInfo *info, const char *tls_cert_path);

/**
 * Sets the TLS key path.
 * @param[in] info The #LinphoneAuthInfo object
 * @param[in] tls_key_path The TLS key path.
**/
LINPHONE_PUBLIC void linphone_auth_info_set_tls_key_path(LinphoneAuthInfo *info, const char *tls_key_path);

/**
 * Gets the username.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The username.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_username(const LinphoneAuthInfo *info);

/**
 * Gets the password.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The password.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_passwd(const LinphoneAuthInfo *info);

/**
 * Gets the userid.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The userid.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_userid(const LinphoneAuthInfo *info);

/**
 * Gets the realm.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The realm.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_realm(const LinphoneAuthInfo *info);

/**
 * Gets the domain.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The domain.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_domain(const LinphoneAuthInfo *info);

/**
 * Gets the ha1.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The ha1.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_ha1(const LinphoneAuthInfo *info);

/**
 * Gets the TLS certificate.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The TLS certificate.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_tls_cert(const LinphoneAuthInfo *info);

/**
 * Gets the TLS key.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The TLS key.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_tls_key(const LinphoneAuthInfo *info);

/**
 * Gets the TLS certificate path.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The TLS certificate path.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_tls_cert_path(const LinphoneAuthInfo *info);

/**
 * Gets the TLS key path.
 * @param[in] info The #LinphoneAuthInfo object
 * @return The TLS key path.
 */
LINPHONE_PUBLIC const char *linphone_auth_info_get_tls_key_path(const LinphoneAuthInfo *info);

/* you don't need those function*/
LINPHONE_PUBLIC void linphone_auth_info_destroy(LinphoneAuthInfo *info);
LINPHONE_PUBLIC LinphoneAuthInfo * linphone_auth_info_new_from_config_file(LpConfig *config, int pos);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_AUTH_INFO_H */
