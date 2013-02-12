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
	 * @param key 
	 */
	void setInt(String section, String key, int value);

	/**
	 * Synchronize LpConfig with file
	 */
	void sync();
}
