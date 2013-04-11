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

#include <ortp/port.h>

/**
 * The LpConfig object is used to manipulate a configuration file.
 * 
 * @ingroup misc
 * The format of the configuration file is a .ini like format:
 * - sections are defined in []
 * - each section contains a sequence of key=value pairs.
 *
 * Example:
 * @code
 * [sound]
 * echocanceler=1
 * playback_dev=ALSA: Default device
 *
 * [video]
 * enabled=1
 * @endcode
**/
typedef struct _LpConfig LpConfig;

#ifdef __cplusplus
extern "C" {
#endif


#define LP_CONFIG_DEFAULT_STRING(config, name, default) \
	(config) ? (lp_config_get_string(config, "default_values", name, default)) : (default)

#define LP_CONFIG_DEFAULT_INT(config, name, default) \
	(config) ? (lp_config_get_int(config, "default_values", name, default)) : (default)

#define LP_CONFIG_DEFAULT_INT64(config, name, default) \
	(config) ? (lp_config_get_int64(config, "default_values", name, default)) : (default)

#define LP_CONFIG_DEFAULT_FLOAT(config, name, default) \
	(config) ? (lp_config_get_float(config, "default_values", name, default)) : (default)


/**
 * Instantiates a LpConfig object from a user config file.
 *
 * @ingroup misc
 * @param filename the filename of the config file to read to fill the instantiated LpConfig
 * @see lp_config_new_with_factory
 */
LpConfig * lp_config_new(const char *filename);

/**
 * Instantiates a LpConfig object from a user config file and a factory config file.
 *
 * @ingroup misc
 * @param config_filename the filename of the user config file to read to fill the instantiated LpConfig
 * @param factory_config_filename the filename of the factory config file to read to fill the instantiated LpConfig
 * @see lp_config_new
 *
 * The user config file is read first to fill the LpConfig and then the factory config file is read.
 * Therefore the configuration parameters defined in the user config file will be overwritten by the parameters
 * defined in the factory config file.
 */
LpConfig * lp_config_new_with_factory(const char *config_filename, const char *factory_config_filename);

int lp_config_read_file(LpConfig *lpconfig, const char *filename);
/**
 * Retrieves a configuration item as a string, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default value string is returned if the config item isn't found.
**/
const char *lp_config_get_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_string);
int lp_config_read_file(LpConfig *lpconfig, const char *filename);
/**
 * Retrieves a configuration item as a range, given its section, key, and default min and max values.
 *
 * @ingroup misc
 * @return TRUE if the value is successfully parsed as a range, FALSE otherwise.
 * If FALSE is returned, min and max are filled respectively with default_min and default_max values.
 */
bool_t lp_config_get_range(const LpConfig *lpconfig, const char *section, const char *key, int *min, int *max, int default_min, int default_max);
/**
 * Retrieves a configuration item as an integer, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default integer value is returned if the config item isn't found.
**/
int lp_config_get_int(const LpConfig *lpconfig,const char *section, const char *key, int default_value);

/**
 * Retrieves a configuration item as a 64 bit integer, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default integer value is returned if the config item isn't found.
**/
int64_t lp_config_get_int64(const LpConfig *lpconfig,const char *section, const char *key, int64_t default_value);


int lp_config_read_file(LpConfig *lpconfig, const char *filename);
/**
 * Retrieves a configuration item as a float, given its section, key, and default value.
 * 
 * @ingroup misc
 * The default float value is returned if the config item isn't found.
**/
float lp_config_get_float(const LpConfig *lpconfig,const char *section, const char *key, float default_value);
/**
 * Sets a string config item 
 *
 * @ingroup misc
**/
void lp_config_set_string(LpConfig *lpconfig,const char *section, const char *key, const char *value);
/**
 * Sets a range config item
 *
 * @ingroup misc
 */
void lp_config_set_range(LpConfig *lpconfig, const char *section, const char *key, int min_value, int max_value);
/**
 * Sets an integer config item
 *
 * @ingroup misc
**/
void lp_config_set_int(LpConfig *lpconfig,const char *section, const char *key, int value);

/**
 * Sets an integer config item, but store it as hexadecimal
 *
 * @ingroup misc
**/
void lp_config_set_int_hex(LpConfig *lpconfig,const char *section, const char *key, int value);

/**
 * Sets a 64 bits integer config item
 *
 * @ingroup misc
**/
void lp_config_set_int64(LpConfig *lpconfig,const char *section, const char *key, int64_t value);

/**
 * Sets a float config item
 *
 * @ingroup misc
**/
void lp_config_set_float(LpConfig *lpconfig,const char *section, const char *key, float value);	
/**
 * Writes the config file to disk.
 * 
 * @ingroup misc
**/
int lp_config_sync(LpConfig *lpconfig);
/**
 * Returns 1 if a given section is present in the configuration.
 *
 * @ingroup misc
**/
int lp_config_has_section(const LpConfig *lpconfig, const char *section);
/**
 * Removes every pair of key,value in a section and remove the section.
 *
 * @ingroup misc
**/
void lp_config_clean_section(LpConfig *lpconfig, const char *section);
/**
 * Call a function for each section present in the configuration.
 *
 * @ingroup misc
**/
void lp_config_for_each_section(const LpConfig *lpconfig, void (*callback)(const char *section, void *ctx), void *ctx);
/**
 * Call a function for each entry present in a section configuration.
 *
 * @ingroup misc
**/
void lp_config_for_each_entry(const LpConfig *lpconfig, const char *section, void (*callback)(const char *entry, void *ctx), void *ctx);

/*tells whether uncommited (with lp_config_sync()) modifications exist*/
int lp_config_needs_commit(const LpConfig *lpconfig);
void lp_config_destroy(LpConfig *cfg);
	
#ifdef __cplusplus
}
#endif

#endif
