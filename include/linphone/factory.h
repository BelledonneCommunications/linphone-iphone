/*
linphone
Copyright (C) 2016 Belledonne Communications <info@belledonne-communications.com>

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

#ifndef LINPHONE_FACTORY_H
#define LINPHONE_FACTORY_H

#include "linphone/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup initializing
 * @{
 */

/**
 * Create the #LinphoneFactory if that has not been done and return
 * a pointer on it.
 * @return A pointer on the #LinphoneFactory
 */
LINPHONE_PUBLIC LinphoneFactory *linphone_factory_get(void);

/**
 * Clean the factory. This function is generally useless as the factory is unique per process, however
 * calling this function at the end avoid getting reports from belle-sip leak detector about memory leaked in linphone_factory_get().
 */
LINPHONE_PUBLIC void linphone_factory_clean(void);

/**
 * Instanciate a #LinphoneCore object.
 *
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param factory The #LinphoneFactory singleton.
 * @param cbs a #LinphoneCoreCbs object holding your application callbacks. A reference
 * will be taken on it until the destruciton of the core or the unregistration
 * with linphone_core_remove_cbs().
 * @param config_path a path to a config file. If it does not exists it will be created.
 *        The config file is used to store all settings, call logs, friends, proxies... so that all these settings
 *	       become persistent over the life of the LinphoneCore object.
 *	       It is allowed to set a NULL config file. In that case LinphoneCore will not store any settings.
 * @param factory_config_path a path to a read-only config file that can be used to
 *        to store hard-coded preference such as proxy settings or internal preferences.
 *        The settings in this factory file always override the one in the normal config file.
 *        It is OPTIONAL, use NULL if unneeded.
 * @see linphone_core_new_with_config
 */
LINPHONE_PUBLIC LinphoneCore *linphone_factory_create_core(const LinphoneFactory *factory, LinphoneCoreCbs *cbs,
						const char *config_path, const char *factory_config_path);


/**
 * Instanciate a #LinphoneCore object.
 *
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param factory The #LinphoneFactory singleton.
 * @param cbs a #LinphoneCoreCbs object holding your application callbacks. A reference
 * will be taken on it until the destruciton of the core or the unregistration
 * with linphone_core_remove_cbs().
 * @param config_path a path to a config file. If it does not exists it will be created.
 *        The config file is used to store all settings, call logs, friends, proxies... so that all these settings
 *	       become persistent over the life of the LinphoneCore object.
 *	       It is allowed to set a NULL config file. In that case LinphoneCore will not store any settings.
 * @param factory_config_path a path to a read-only config file that can be used to
 *        to store hard-coded preference such as proxy settings or internal preferences.
 *        The settings in this factory file always override the one in the normal config file.
 *        It is OPTIONAL, use NULL if unneeded.
 * @param user_data an application pointer associated with the returned core.
 * @param system_context a pointer to a system object required by the core to operate. Currently it is required to pass an android Context on android, pass NULL on other platforms.
 * @see linphone_core_new_with_config
 */
LINPHONE_PUBLIC LinphoneCore *linphone_factory_create_core_2(const LinphoneFactory *factory, LinphoneCoreCbs *cbs,
						const char *config_path, const char *factory_config_path, void *user_data, void *system_context);


/**
 * Instantiates a LinphoneCore object with a given LpConfig.
 *
 * @param factory The #LinphoneFactory singleton.
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param cbs a #LinphoneCoreCbs object holding your application callbacks. A reference
 * will be taken on it until the destruciton of the core or the unregistration
 * with linphone_core_remove_cbs().
 * @param config a pointer to an LpConfig object holding the configuration of the LinphoneCore to be instantiated.
 * @see linphone_core_new
 */
LINPHONE_PUBLIC LinphoneCore *linphone_factory_create_core_with_config(const LinphoneFactory *factory, LinphoneCoreCbs *cbs, LinphoneConfig *config);

/**
 * Instantiates a LinphoneCore object with a given LpConfig.
 *
 * @param factory The #LinphoneFactory singleton.
 * The LinphoneCore object is the primary handle for doing all phone actions.
 * It should be unique within your application.
 * @param cbs a #LinphoneCoreCbs object holding your application callbacks. A reference
 * will be taken on it until the destruciton of the core or the unregistration
 * with linphone_core_remove_cbs().
 * @param config a pointer to an LpConfig object holding the configuration of the LinphoneCore to be instantiated.
 * @param user_data an application pointer associated with the returned core.
 * @param system_context a pointer to a system object required by the core to operate. Currently it is required to pass an android Context on android, pass NULL on other platforms.
 * @see linphone_core_new
 */
LINPHONE_PUBLIC LinphoneCore *linphone_factory_create_core_with_config_2(const LinphoneFactory *factory, LinphoneCoreCbs *cbs, LinphoneConfig *config, void *user_data, void *system_context);

/**
 * Instanciate a #LinphoneCoreCbs object.
 * @return a new #LinphoneCoreCbs.
 */
LINPHONE_PUBLIC LinphoneCoreCbs *linphone_factory_create_core_cbs(const LinphoneFactory *factory);

/**
 * Parse a string holding a SIP URI and create the according #LinphoneAddress object.
 * @param factory The #LinphoneFactory singleton.
 * @param addr A string holding the SIP URI to parse.
 * @return A new #LinphoneAddress.
 */
LINPHONE_PUBLIC LinphoneAddress *linphone_factory_create_address(const LinphoneFactory *factory, const char *addr);

/**
 * Creates a #LinphoneAuthInfo object.
 * The object can be created empty, that is with all arguments set to NULL.
 * Username, userid, password, realm and domain can be set later using specific methods.
 * At the end, username and passwd (or ha1) are required.
 * @param factory The #LinphoneFactory singleton.
 * @param username The username that needs to be authenticated
 * @param userid The userid used for authenticating (use NULL if you don't know what it is)
 * @param passwd The password in clear text
 * @param ha1 The ha1-encrypted password if password is not given in clear text.
 * @param realm The authentication domain (which can be larger than the sip domain. Unfortunately many SIP servers don't use this parameter.
 * @param domain The SIP domain for which this authentication information is valid, if it has to be restricted for a single SIP domain.
 * @return A #LinphoneAuthInfo object. linphone_auth_info_destroy() must be used to destroy it when no longer needed. The LinphoneCore makes a copy of LinphoneAuthInfo
 * passed through linphone_core_add_auth_info().
 */
LINPHONE_PUBLIC LinphoneAuthInfo *linphone_factory_create_auth_info(const LinphoneFactory *factory, const char *username, const char *userid, const char *passwd, const char *ha1, const char *realm, const char *domain);

/**
 * Create a LinphoneCallCbs object that holds callbacks for events happening on a call.
 * @param[in] factory LinphoneFactory singletion object
 * @return A new LinphoneCallCbs object
 */
LINPHONE_PUBLIC LinphoneCallCbs * linphone_factory_create_call_cbs(const LinphoneFactory *factory);

/**
 * Create an empty #LinphoneVcard.
 * @return a new #LinphoneVcard.
 * @ingroup initializing
 */
LINPHONE_PUBLIC LinphoneVcard *linphone_factory_create_vcard(LinphoneFactory *factory);

/**
 * Create a LinphoneVideoDefinition from a given width and height
 * @param[in] factory LinphoneFactory singleton object
 * @param[in] width The width of the created video definition
 * @param[in] height The height of the created video definition
 * @return A new LinphoneVideoDefinition object
 */
LINPHONE_PUBLIC LinphoneVideoDefinition * linphone_factory_create_video_definition(const LinphoneFactory *factory, unsigned int width, unsigned int height);

/**
 * Create a LinphoneVideoDefinition from a given standard definition name
 * @param[in] factory LinphoneFactory singleton object
 * @param[in] name The standard definition name of the video definition to create
 * @return A new LinphoneVideoDefinition object
 */
LINPHONE_PUBLIC LinphoneVideoDefinition * linphone_factory_create_video_definition_from_name(const LinphoneFactory *factory, const char *name);

/**
 * Get the list of standard video definitions supported by Linphone.
 * @param[in] factory LinphoneFactory singleton object
 * @return \bctbx_list{LinphoneVideoDefinition}
 */
LINPHONE_PUBLIC const bctbx_list_t * linphone_factory_get_supported_video_definitions(const LinphoneFactory *factory);

/**
 * Get the top directory where the resources are located.
 * @param[in] factory LinphoneFactory object
 * @return The path to the top directory where the resources are located
 */
LINPHONE_PUBLIC const char * linphone_factory_get_top_resources_dir(const LinphoneFactory *factory);

/**
 * Set the top directory where the resources are located.
 * If you only define this top directory, the other resources directory will automatically be derived form this one.
 * @param[in] factory LinphoneFactory object
 * @param[in] path The path to the top directory where the resources are located
 */
LINPHONE_PUBLIC void linphone_factory_set_top_resources_dir(LinphoneFactory *factory, const char *path);

/**
 * Get the directory where the data resources are located.
 * @param[in] factory LinphoneFactory object
 * @return The path to the directory where the data resources are located
 */
LINPHONE_PUBLIC const char * linphone_factory_get_data_resources_dir(LinphoneFactory *factory);

/**
 * Set the directory where the data resources are located.
 * @param[in] factory LinphoneFactory object
 * @param[in] path The path where the data resources are located
 */
LINPHONE_PUBLIC void linphone_factory_set_data_resources_dir(LinphoneFactory *factory, const char *path);

/**
 * Get the directory where the sound resources are located.
 * @param[in] factory LinphoneFactory object
 * @return The path to the directory where the sound resources are located
 */
LINPHONE_PUBLIC const char * linphone_factory_get_sound_resources_dir(LinphoneFactory *factory);

/**
 * Set the directory where the sound resources are located.
 * @param[in] factory LinphoneFactory object
 * @param[in] path The path where the sound resources are located
 */
LINPHONE_PUBLIC void linphone_factory_set_sound_resources_dir(LinphoneFactory *factory, const char *path);

/**
 * Get the directory where the ring resources are located.
 * @param[in] factory LinphoneFactory object
 * @return The path to the directory where the ring resources are located
 */
LINPHONE_PUBLIC const char * linphone_factory_get_ring_resources_dir(LinphoneFactory *factory);

/**
 * Set the directory where the ring resources are located.
 * @param[in] factory LinphoneFactory object
 * @param[in] path The path where the ring resources are located
 */
LINPHONE_PUBLIC void linphone_factory_set_ring_resources_dir(LinphoneFactory *factory, const char *path);

/**
 * Get the directory where the image resources are located.
 * @param[in] factory LinphoneFactory object
 * @return The path to the directory where the image resources are located
 */
LINPHONE_PUBLIC const char * linphone_factory_get_image_resources_dir(LinphoneFactory *factory);

/**
 * Set the directory where the image resources are located.
 * @param[in] factory LinphoneFactory object
 * @param[in] path The path where the image resources are located
 */
LINPHONE_PUBLIC void linphone_factory_set_image_resources_dir(LinphoneFactory *factory, const char *path);

/**
 * Get the directory where the mediastreamer2 plugins are located.
 * @param[in] factory LinphoneFactory object
 * @return The path to the directory where the mediastreamer2 plugins are located, or NULL if it has not been set
 */
LINPHONE_PUBLIC const char * linphone_factory_get_msplugins_dir(LinphoneFactory *factory);

/**
 * Set the directory where the mediastreamer2 plugins are located.
 * @param[in] factory LinphoneFactory object
 * @param[in] path The path to the directory where the mediastreamer2 plugins are located
 */
LINPHONE_PUBLIC void linphone_factory_set_msplugins_dir(LinphoneFactory *factory, const char *path);

/**
 * Creates an object LinphoneErrorInfo.
 * @param[in] factory LinphoneFactory object
 * @return  LinphoneErrorInfo object.
 */
LINPHONE_PUBLIC  LinphoneErrorInfo *linphone_factory_create_error_info(LinphoneFactory *factory);

/**
 * Creates an object LinphoneRange.
 * @param[in] factory LinphoneFactory object
 * @return  LinphoneRange object.
 */
LINPHONE_PUBLIC LinphoneRange *linphone_factory_create_range(LinphoneFactory *factory);

/**
 * Creates an object LinphoneTransports.
 * @param[in] factory LinphoneFactory object
 * @return  LinphoneTransports object.
 */
LINPHONE_PUBLIC LinphoneTransports *linphone_factory_create_transports(LinphoneFactory *factory);

/**
 * Creates an object LinphoneVideoActivationPolicy.
 * @param[in] factory LinphoneFactory object
 * @return  LinphoneVideoActivationPolicy object.
 */
LINPHONE_PUBLIC LinphoneVideoActivationPolicy *linphone_factory_create_video_activation_policy(LinphoneFactory *factory);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif // LINPHONE_FACTORY_H
