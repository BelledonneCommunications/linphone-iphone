/*
PresenceActivity.java
Copyright (C) 2010-2013  Belledonne Communications, Grenoble, France

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

public interface PresenceActivity {

	/**
	 * @brief Gets the string representation of a presence activity.
	 * @return A String representing the given activity.
	 */
	String toString();

	/**
	 * @brief Gets the activity type of a presence activity.
	 * @return The #PresenceActivityType of the activity.
	 */
	PresenceActivityType getType();

	/**
	 * @brief Sets the type of activity of a presence activity.
	 * @param[in] acttype The activity type to set for the activity.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setType(PresenceActivityType type);

	/**
	 * @brief Gets the description of a presence activity.
	 * @return A String containing the description of the presence activity, or null if no description is specified.
	 */
	String getDescription();

	/**
	 * @brief Sets the description of a presence activity.
	 * @param[in] description An additional description of the activity. Can be null if no additional description is to be added.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setDescription(String description);

	/**
	 * @brief Gets the native pointer for this object.
	 */
	long getNativePtr();

}
