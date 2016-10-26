/*
PresenceService.java
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

public interface PresenceService {

	/**
	 * Gets the id of a presence service.
	 * @return A string containing the id.
	 */
	String getId();

	/**
	 * Sets the id of a presence service.
	 * @param id The id string to set. Can be null to generate it automatically.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setId(String id);

	/**
	 * Gets the basic status of a presence service.
	 * @return The #PresenceBasicStatus of the #PresenceService object.
	 */
	PresenceBasicStatus getBasicStatus();

	/**
	 * Sets the basic status of a presence service.
	 * @param status The #PresenceBasicStatus to set for the #PresenceService object.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setBasicStatus(PresenceBasicStatus status);

	/**
	 * Gets the contact of a presence service.
	 * @return A string containing the contact, or null if no contact is found.
	 */
	String getContact();

	 /**
	 * Sets the contact of a presence service.
	 * @param contact The contact string to set.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setContact(String contact);

	/**
	 * Gets the number of notes included in the presence service.
	 * @return The number of notes included in the #PresenceService object.
	 */
	long getNbNotes();

	/**
	 * Gets the nth note of a presence service.
	 * @param idx The index of the note to get (the first note having the index 0).
	 * @return A pointer to a #PresenceNote object if successful, null otherwise.
	 */
	PresenceNote getNthNote(long idx);

	/**
	 * Adds a note to a presence service.
	 * @param note The #PresenceNote object to add to the service.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addNote(PresenceNote note);

	/**
	 * Clears the notes of a presence service.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearNotes();

	/**
	 * Gets the native pointer for this object.
	 */
	long getNativePtr();

}
