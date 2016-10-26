/*
PresencePerson.java
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

package org.linphone.core;

public interface PresencePerson {

	/**
	 * Gets the id of a presence person.
	 * @return A string containing the id.
	 */
	String getId();

	/**
	 * Sets the id of a presence person.
	 * @param id The id string to set. Can be null to generate it automatically.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setId(String id);

	/**
	 * Gets the number of activities included in the presence person.
	 * @return The number of activities included in the #PresencePerson object.
	 */
	long getNbActivities();

	/**
	 * Gets the nth activity of a presence person.
	 * @param idx The index of the activity to get (the first activity having the index 0).
	 * @return A #PresenceActivity object if successful, null otherwise.
	 */
	PresenceActivity getNthActivity(long idx);

	/**
	 * Adds an activity to a presence person.
	 * @param activity The #PresenceActivity object to add to the person.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addActivity(PresenceActivity activity);

	/**
	 * Clears the activities of a presence person.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearActivities();

	/**
	 * Gets the number of notes included in the presence person.
	 * @return The number of notes included in the #PresencePerson object.
	 */
	long getNbNotes();

	/**
	 * Gets the nth note of a presence person.
	 * @param idx The index of the note to get (the first note having the index 0).
	 * @return A pointer to a #PresenceNote object if successful, null otherwise.
	 */
	PresenceNote getNthNote(long idx);

	/**
	 * Adds a note to a presence person.
	 * @param note The #PresenceNote object to add to the person.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addNote(PresenceNote note);

	/**
	 * Clears the notes of a presence person.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearNotes();

	/**
	 * Gets the number of activities notes included in the presence person.
	 * @return The number of activities notes included in the #PresencePerson object.
	 */
	long getNbActivitiesNotes();

	/**
	 * Gets the nth activities note of a presence person.
	 * @param idx The index of the activities note to get (the first note having the index 0).
	 * @return A pointer to a #PresenceNote object if successful, null otherwise.
	 */
	PresenceNote getNthActivitiesNote(long idx);

	/**
	 * Adds an activities note to a presence person.
	 * @param note The #PresenceNote object to add to the person.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addActivitiesNote(PresenceNote note);

	/**
	 * Clears the activities notes of a presence person.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearActivitesNotes();

	/**
	 * Gets the native pointer for this object.
	 */
	long getNativePtr();

}
