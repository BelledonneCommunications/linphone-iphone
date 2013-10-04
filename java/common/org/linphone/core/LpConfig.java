/*
LPConfig.java
Copyright (C) 2013  Belledonne Communications, Grenoble, France

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
 * The LpConfig object is used to manipulate a configuration file.
 * 
 * <pre>
 * The format of the configuration file is a .ini like format:
 * - sections are defined in []
 * - each section contains a sequence of key=value pairs.
 * 
 * Example:
 * [sound]
 * echocanceler=1
 * playback_dev=ALSA: Default device
 *
 * [video]
 * enabled=1
 * </pre>
 * }
 * @author Guillaume Beraudo
 */
public interface LpConfig {

	/**
	 * Sets an integer config item
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param value the value of the setting
	 */
	void setInt(String section, String key, int value);
	
	/**
	 * Sets an float config item
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param value the value of the setting
	 */
	void setFloat(String section, String key, float value);

	/**
	 * Sets an boolean config item
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param value the value of the setting
	 */
	void setBool(String section, String key, boolean value);

	/**
	 * Sets an string config item
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param value the value of the setting
	 */
	void setString(String section, String key, String value);

	/**
	 * Sets an integer range config item
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param min the min of the range
	 * @param max the max of the range
	 */
	void setIntRange(String section, String key, int min, int max);
	
	/**
	 * Gets a int from the config
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param defaultValue the default value if not set
	 * @return the value of the setting or the default value if not set
	 */
	int getInt(String section, String key, int defaultValue);
	
	/**
	 * Gets a float from the config
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param defaultValue the default value if not set
	 * @return the value of the setting or the default value if not set
	 */
	float getFloat(String section, String key, float defaultValue);
	
	/**
	 * Gets a boolean from the config
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param defaultValue the default value if not set
	 * @return the value of the setting or the default value if not set
	 */
	boolean getBool(String section, String key, boolean defaultValue);
	
	/**
	 * Gets a string from the config
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param defaultValue the default value if not set
	 * @return the value of the setting or the default value if not set
	 */
	String getString(String section, String key, String defaultValue);
	
	/**
	 * Gets a int range from the config
	 * @param section the section in the lpconfig
	 * @param key the name of the setting
	 * @param defaultValue the default value if not set
	 * @return the value of the setting or the default value if not set
	 */
	int[] getIntRange(String section, String key, int defaultMin, int defaultMax);

	/**
	 * Synchronize LpConfig with file
	 */
	void sync();
}
