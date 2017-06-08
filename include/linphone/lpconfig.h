/***************************************************************************
 *            lpconfig.h
 *
 *  Thu Mar 10 15:02:49 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon.morlat@linphone.org
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef LPCONFIG_H
#define LPCONFIG_H

#include "linphone/types.h"

/**
 * @addtogroup misc
 * @{
 */

/**
 * Safely downcast a belle_sip_object into LinphoneConfig
 */
#define LINPHONE_CONFIG(obj) BELLE_SIP_CAST(obj, LinphoneConfig);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Instantiates a LinphoneConfig object from a user config file.
 * The caller of this constructor owns a reference. linphone_config_unref() must be called when this object is no longer needed.
 * @ingroup misc
 * @param filename the filename of the config file to read to fill the instantiated LinphoneConfig
 * @see linphone_config_new_with_factory
 */
LINPHONE_PUBLIC LinphoneConfig * linphone_config_new(const char *filename);

/**
 * Instantiates a LinphoneConfig object from a user provided buffer.
 * The caller of this constructor owns a reference. linphone_config_unref() must be called when this object is no longer needed.
 * @ingroup misc
 * @param buffer the buffer from which the LinphoneConfig will be retrieved. We expect the buffer to be null-terminated.
 * @see linphone_config_new_with_factory
 * @see linphone_config_new
 */
LINPHONE_PUBLIC LinphoneConfig * linphone_config_new_from_buffer(const char *buffer);

/**
 * Instantiates a LinphoneConfig object from a user config file and a factory config file.
 * The caller of this constructor owns a reference. linphone_config_unref() must be called when this object is no longer needed.
 * @ingroup misc
 * @param config_filename the filename of the user config file to read to fill the instantiated LinphoneConfig
 * @param factory_config_filename the filename of the factory config file to read to fill the instantiated LinphoneConfig
 * @see linphone_config_new
 *
 * The user config file is read first to fill the LinphoneConfig and then the factory config file is read.
 * Therefore the configuration parameters defined in the user config file will be overwritten by the parameters
 * defined in the factory config file.
 */
LINPHONE_PUBLIC LinphoneConfig * linphone_config_new_with_factory(const char *config_filename, const char *factory_config_filename);

/**
 * Reads a user config file and fill the LinphoneConfig with the read config values.
 * @ingroup misc
 * @param lpconfig The LinphoneConfig object to fill with the content of the file
 * @param filename The filename of the config file to read to fill the LinphoneConfig
 */
LINPHONE_PUBLIC LinphoneStatus linphone_config_read_file(LinphoneConfig *lpconfig, const char *filename);

/**
 * Reads a xml config file and fill the LinphoneConfig with the read config dynamic values.
 * @ingroup misc
 * @param lpconfig The LinphoneConfig object to fill with the content of the file
 * @param filename The filename of the config file to read to fill the LinphoneConfig
 */
LINPHONE_PUBLIC const char* linphone_config_load_from_xml_file(LinphoneConfig *lpc, const char *filename);

/**
 * Reads a xml config string and fill the LinphoneConfig with the read config dynamic values.
 * @ingroup misc
 * @param lpconfig The LinphoneConfig object to fill with the content of the file
 * @param buffer The string of the config file to fill the LinphoneConfig
 * @return 0 in case of success
 */
LINPHONE_PUBLIC LinphoneStatus linphone_config_load_from_xml_string(LpConfig *lpc, const char *buffer);

/**
 * Retrieves a configuration item as a string, given its section, key, and default value.
 *
 * The default value string is returned if the config item isn't found.
**/
LINPHONE_PUBLIC const char *linphone_config_get_string(const LinphoneConfig *lpconfig, const char *section, const char *key, const char *default_string);

/**
 * Retrieves a configuration item as a list of strings, given its section, key, and default value.
 * The default value is returned if the config item is not found.
 * @param[in] lpconfig A #LinphoneConfig object
 * @param[in] section The section from which to retrieve a configuration item
 * @param[in] key The name of the configuration item to retrieve
 * @param[in] default_list \bctbx_list{const char *}
 * @return \bctbx_list{const char *}
 */
LINPHONE_PUBLIC bctbx_list_t * linphone_config_get_string_list(const LinphoneConfig *lpconfig, const char *section, const char *key, bctbx_list_t *default_list);

/**
 * Retrieves a configuration item as a range, given its section, key, and default min and max values.
 *
 * @return TRUE if the value is successfully parsed as a range, FALSE otherwise.
 * If FALSE is returned, min and max are filled respectively with default_min and default_max values.
 */
LINPHONE_PUBLIC bool_t linphone_config_get_range(const LinphoneConfig *lpconfig, const char *section, const char *key, int *min, int *max, int default_min, int default_max);

/**
 * Retrieves a configuration item as an integer, given its section, key, and default value.
 *
 * The default integer value is returned if the config item isn't found.
**/
LINPHONE_PUBLIC int linphone_config_get_int(const LinphoneConfig *lpconfig,const char *section, const char *key, int default_value);

/**
 * Retrieves a configuration item as a 64 bit integer, given its section, key, and default value.
 *
 * The default integer value is returned if the config item isn't found.
**/
LINPHONE_PUBLIC int64_t linphone_config_get_int64(const LinphoneConfig *lpconfig,const char *section, const char *key, int64_t default_value);

/**
 * Retrieves a configuration item as a float, given its section, key, and default value.
 *
 * The default float value is returned if the config item isn't found.
**/
LINPHONE_PUBLIC float linphone_config_get_float(const LinphoneConfig *lpconfig,const char *section, const char *key, float default_value);

/**
 * Sets a string config item
**/
LINPHONE_PUBLIC void linphone_config_set_string(LinphoneConfig *lpconfig,const char *section, const char *key, const char *value);

/**
 * Sets a string list config item
 * @param[in] lpconfig A #LinphoneConfig object
 * @param[in] section The name of the section to put the configuration item into
 * @param[in] key The name of the configuration item to set
 * @param[in] value \bctbx_list{const char *} The value to set
 */
LINPHONE_PUBLIC void linphone_config_set_string_list(LinphoneConfig *lpconfig, const char *section, const char *key, const bctbx_list_t *value);

/**
 * Sets a range config item
 */
LINPHONE_PUBLIC void linphone_config_set_range(LinphoneConfig *lpconfig, const char *section, const char *key, int min_value, int max_value);

/**
 * Sets an integer config item
**/
LINPHONE_PUBLIC void linphone_config_set_int(LinphoneConfig *lpconfig,const char *section, const char *key, int value);

/**
 * Sets an integer config item, but store it as hexadecimal
**/
LINPHONE_PUBLIC void linphone_config_set_int_hex(LinphoneConfig *lpconfig,const char *section, const char *key, int value);

/**
 * Sets a 64 bits integer config item
**/
LINPHONE_PUBLIC void linphone_config_set_int64(LinphoneConfig *lpconfig,const char *section, const char *key, int64_t value);

/**
 * Sets a float config item
**/
LINPHONE_PUBLIC void linphone_config_set_float(LinphoneConfig *lpconfig,const char *section, const char *key, float value);

/**
 * Writes the config file to disk.
**/
LINPHONE_PUBLIC LinphoneStatus linphone_config_sync(LinphoneConfig *lpconfig);

/**
 * Returns 1 if a given section is present in the configuration.
**/
LINPHONE_PUBLIC int linphone_config_has_section(const LinphoneConfig *lpconfig, const char *section);

/**
 * Removes every pair of key,value in a section and remove the section.
**/
LINPHONE_PUBLIC void linphone_config_clean_section(LinphoneConfig *lpconfig, const char *section);

/**
 * Returns 1 if a given section  with a given key is present in the configuration.
 * @param[in] lpconfig The LinphoneConfig object
 * @param[in] section
 * @param[in] key
 **/
LINPHONE_PUBLIC int linphone_config_has_entry(const LinphoneConfig *lpconfig, const char *section, const char *key);

/**
 * Removes entries for key,value in a section.
 * @param[in] lpconfig The LinphoneConfig object
 * @param[in] section
 * @param[in] key
 **/
LINPHONE_PUBLIC void linphone_config_clean_entry(LinphoneConfig *lpconfig, const char *section, const char *key);

/**
 * Returns the list of sections' names in the LinphoneConfig.
 * @param[in] lpconfig The LinphoneConfig object
 * @return a null terminated static array of strings
**/
LINPHONE_PUBLIC const char** linphone_config_get_sections_names(LinphoneConfig *lpconfig);

/**
 * Call a function for each section present in the configuration.
**/
void linphone_config_for_each_section(const LinphoneConfig *lpconfig, void (*callback)(const char *section, void *ctx), void *ctx);

/**
 * Call a function for each entry present in a section configuration.
**/
void linphone_config_for_each_entry(const LinphoneConfig *lpconfig, const char *section, void (*callback)(const char *entry, void *ctx), void *ctx);

/*tells whether uncommited (with linphone_config_sync()) modifications exist*/
bool_t linphone_config_needs_commit(const LinphoneConfig *lpconfig);

LINPHONE_PUBLIC void linphone_config_destroy(LinphoneConfig *cfg);

/**
 * Retrieves a default configuration item as an integer, given its section, key, and default value.
 * The default integer value is returned if the config item isn't found.
**/
LINPHONE_PUBLIC int linphone_config_get_default_int(const LinphoneConfig *lpconfig, const char *section, const char *key, int default_value);

/**
 * Retrieves a default configuration item as a 64 bit integer, given its section, key, and default value.
 * The default integer value is returned if the config item isn't found.
**/
LINPHONE_PUBLIC int64_t linphone_config_get_default_int64(const LinphoneConfig *lpconfig, const char *section, const char *key, int64_t default_value);

/**
 * Retrieves a default configuration item as a float, given its section, key, and default value.
 * The default float value is returned if the config item isn't found.
**/
LINPHONE_PUBLIC float linphone_config_get_default_float(const LinphoneConfig *lpconfig, const char *section, const char *key, float default_value);

/**
 * Retrieves a default configuration item as a string, given its section, key, and default value.
 * The default value string is returned if the config item isn't found.
**/
LINPHONE_PUBLIC const char* linphone_config_get_default_string(const LinphoneConfig *lpconfig, const char *section, const char *key, const char *default_value);

/**
 * Retrieves a section parameter item as a string, given its section and key.
 * The default value string is returned if the config item isn't found.
**/
LINPHONE_PUBLIC const char* linphone_config_get_section_param_string(const LinphoneConfig *lpconfig, const char *section, const char *key, const char *default_value);

/**
 * increment reference count
**/
LINPHONE_PUBLIC LinphoneConfig *linphone_config_ref(LinphoneConfig *lpconfig);

/**
 * Decrement reference count, which will eventually free the object.
**/
LINPHONE_PUBLIC void linphone_config_unref(LinphoneConfig *lpconfig);

/**
 * Write a string in a file placed relatively with the Linphone configuration file.
 * @param lpconfig LinphoneConfig instance used as a reference
 * @param filename Name of the file where to write data. The name is relative to the place of the config file
 * @param data String to write
 */
LINPHONE_PUBLIC void linphone_config_write_relative_file(const LinphoneConfig *lpconfig, const char *filename, const char *data);

/**
 * Read a string from a file placed beside the Linphone configuration file
 * @param lpconfig LinphoneConfig instance used as a reference
 * @param filename Name of the file where data will be read from. The name is relative to the place of the config file
 * @param data Buffer where read string will be stored
 * @param max_length Length of the buffer
 * @return 0 on success, -1 on failure
 * @donotwrap
 */
LINPHONE_PUBLIC LinphoneStatus linphone_config_read_relative_file(const LinphoneConfig *lpconfig, const char *filename, char *data, size_t max_length);

/**
 * @return TRUE if file exists relative to the to the current location
**/
LINPHONE_PUBLIC bool_t linphone_config_relative_file_exists(const LinphoneConfig *lpconfig, const char *filename);

/**
 * Dumps the LinphoneConfig as XML into a buffer
 * @param[in] lpconfig The LinphoneConfig object
 * @return The buffer that contains the XML dump
**/
LINPHONE_PUBLIC char* linphone_config_dump_as_xml(const LinphoneConfig *lpconfig);

/**
 * Dumps the LinphoneConfig as INI into a buffer
 * @param[in] lpconfig The LinphoneConfig object
 * @return The buffer that contains the config dump
**/
LINPHONE_PUBLIC char* linphone_config_dump(const LinphoneConfig *lpconfig);

/**
 * Retrieves the overwrite flag for a config item
**/
LINPHONE_PUBLIC bool_t linphone_config_get_overwrite_flag_for_entry(const LinphoneConfig *lpconfig, const char *section, const char *key);

/**
 * Sets the overwrite flag for a config item (used when dumping config as xml)
**/
LINPHONE_PUBLIC void linphone_config_set_overwrite_flag_for_entry(LinphoneConfig *lpconfig, const char *section, const char *key, bool_t value);

/**
 * Retrieves the overwrite flag for a config section
**/
LINPHONE_PUBLIC bool_t linphone_config_get_overwrite_flag_for_section(const LinphoneConfig *lpconfig, const char *section);

/**
 * Sets the overwrite flag for a config section (used when dumping config as xml)
**/
LINPHONE_PUBLIC void linphone_config_set_overwrite_flag_for_section(LinphoneConfig *lpconfig, const char *section, bool_t value);

/**
 * Retrieves the skip flag for a config item
**/
LINPHONE_PUBLIC bool_t linphone_config_get_skip_flag_for_entry(const LinphoneConfig *lpconfig, const char *section, const char *key);

/**
 * Sets the skip flag for a config item (used when dumping config as xml)
**/
LINPHONE_PUBLIC void linphone_config_set_skip_flag_for_entry(LinphoneConfig *lpconfig, const char *section, const char *key, bool_t value);

/**
 * Retrieves the skip flag for a config section
**/
LINPHONE_PUBLIC bool_t linphone_config_get_skip_flag_for_section(const LinphoneConfig *lpconfig, const char *section);

/**
 * Sets the skip flag for a config section (used when dumping config as xml)
**/
LINPHONE_PUBLIC void linphone_config_set_skip_flag_for_section(LinphoneConfig *lpconfig, const char *section, bool_t value);

/**
 * Converts a config section into a dictionary.
 * @return a dictionary with all the keys from a section, or NULL if the section doesn't exist
 */
LINPHONE_PUBLIC LinphoneDictionary * lp_config_section_to_dict( const LpConfig* lpconfig, const char* section );

/**
 * Loads a dictionary into a section of the lpconfig. If the section doesn't exist it is created.
 * Overwrites existing keys, creates non-existing keys.
 */
LINPHONE_PUBLIC void lp_config_load_dict_to_section( LpConfig* lpconfig, const char* section, const LinphoneDictionary* dict);

#ifdef __cplusplus
}
#endif

// Define old function names for backward compatibility
#define lp_config_new linphone_config_new
#define lp_config_new_from_buffer linphone_config_new_from_buffer
#define lp_config_new_with_factory linphone_config_new_with_factory
#define lp_config_read_file linphone_config_read_file
#define lp_config_get_string linphone_config_get_string
#define lp_config_get_string_list linphone_config_get_string_list
#define lp_config_get_range linphone_config_get_range
#define lp_config_get_int linphone_config_get_int
#define lp_config_get_int64 linphone_config_get_int64
#define lp_config_get_float linphone_config_get_float
#define lp_config_set_string linphone_config_set_string
#define lp_config_set_string_list linphone_config_set_string_list
#define lp_config_set_range linphone_config_set_range
#define lp_config_set_int linphone_config_set_int
#define lp_config_set_int_hex linphone_config_set_int_hex
#define lp_config_set_int64 linphone_config_set_int64
#define lp_config_set_float linphone_config_set_float
#define lp_config_sync linphone_config_sync
#define lp_config_has_section linphone_config_has_section
#define lp_config_clean_section linphone_config_clean_section
#define lp_config_has_entry linphone_config_has_entry
#define lp_config_clean_entry linphone_config_clean_entry
#define lp_config_get_sections_names linphone_config_get_sections_names
#define lp_config_for_each_section linphone_config_for_each_section
#define lp_config_for_each_entry linphone_config_for_each_entry
#define lp_config_needs_commit linphone_config_needs_commit
#define lp_config_destroy linphone_config_destroy
#define lp_config_get_default_int linphone_config_get_default_int
#define lp_config_get_default_int64 linphone_config_get_default_int64
#define lp_config_get_default_float linphone_config_get_default_float
#define lp_config_get_default_string linphone_config_get_default_string
#define lp_config_get_section_param_string linphone_config_get_section_param_string
#define lp_config_ref linphone_config_ref
#define lp_config_unref linphone_config_unref
#define lp_config_write_relative_file linphone_config_write_relative_file
#define lp_config_read_relative_file linphone_config_read_relative_file
#define lp_config_relative_file_exists linphone_config_relative_file_exists
#define lp_config_dump_as_xml linphone_config_dump_as_xml
#define lp_config_dump linphone_config_dump
#define lp_config_get_overwrite_flag_for_entry linphone_config_get_overwrite_flag_for_entry
#define lp_config_set_overwrite_flag_for_entry linphone_config_set_overwrite_flag_for_entry
#define lp_config_get_overwrite_flag_for_section linphone_config_get_overwrite_flag_for_section
#define lp_config_set_overwrite_flag_for_section linphone_config_set_overwrite_flag_for_section
#define lp_config_get_skip_flag_for_entry linphone_config_get_skip_flag_for_entry
#define lp_config_set_skip_flag_for_entry linphone_config_set_skip_flag_for_entry
#define lp_config_get_skip_flag_for_section linphone_config_get_skip_flag_for_section
#define lp_config_set_skip_flag_for_section linphone_config_set_skip_flag_for_section

/**
 * @}
 */

#endif
