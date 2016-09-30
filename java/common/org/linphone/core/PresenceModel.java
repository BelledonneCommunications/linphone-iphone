/*
PresenceModel.java
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

public interface PresenceModel {

	/**
	 * Gets the basic status of a presence model.
	 * @return The #BasicStatus of the #PresenceModel object.
	 */
	PresenceBasicStatus getBasicStatus();

	/**
	 * Sets the basic status of a presence model.
	 * @param basic_status The #BasicStatus to set for the #PresenceModel object.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int setBasicStatus(PresenceBasicStatus basic_status);

	/**
	 * Gets the timestamp of a presence model.
	 * @return The timestamp of the #LinphonePresenceModel object or -1 on error.
	 */
	long getTimestamp();

	/**
	 * Gets the contact of a presence model.
	 * @return A string containing the contact, or null if no contact is found.
	 */
	String getContact();

	/**
	 * Sets the contact of a presence model.
	 * @param contact The contact string to set.
	 */
	void setContact(String contact);

	/**
	 * Gets the first activity of a presence model (there is usually only one).
	 * @return A #PresenceActivity object if successful, null otherwise.
	 */
	PresenceActivity getActivity();

	/**
	 * Sets the activity of a presence model (limits to only one activity).
	 * @param activity The #PresenceActivityType to set for the model.
	 * @param description An additional description of the activity to set for the model. Can be null if no additional description is to be added.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 *
	 * WARNING: This method will modify the basic status of the model according to the activity being set.
	 * If you don't want the basic status to be modified automatically, you can use the combination of setBasicStatus(), clearActivities() and addActivity().
	 */
	int setActivity(PresenceActivityType activity, String description);

	/**
	 * Gets the number of activities included in the presence model.
	 * @return The number of activities included in the #PresenceModel object.
	 */
	long getNbActivities();

	/**
	 * Gets the nth activity of a presence model.
	 * @param idx The index of the activity to get (the first activity having the index 0).
	 * @return A #PresenceActivity object if successful, null otherwise.
	 */
	PresenceActivity getNthActivity(long idx);

	/**
	 * Adds an activity to a presence model.
	 * @param activity The #PresenceActivity to add to the model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addActivity(PresenceActivity activity);

	/**
	 * Clears the activities of a presence model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearActivities();

	/**
	 * Gets the first note of a presence model (there is usually only one).
	 * @param lang The language of the note to get. Can be null to get a note that has no language specified or to get the first note whatever language it is written into.
	 * @return A #PresenceNote object if successful, null otherwise.
	 */
	PresenceNote getNote(String lang);

	/**
	 * Adds a note to a presence model.
	 * @param note_content The note to be added to the presence model.
	 * @param lang The language of the note to be added. Can be null if no language is to be specified for the note.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 *
	 * Only one note for each language can be set, so e.g. setting a note for the 'fr' language if there is only one will replace the existing one.
	 */
	int addNote(String note_content, String lang);

	/**
	 * Clears all the notes of a presence model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearNotes();

	/**
	 * Gets the number of services included in the presence model.
	 * @return The number of services included in the #PresenceModel object.
	 */
	long getNbServices();

	/**
	 * Gets the nth service of a presence model.
	 * @param idx The index of the service to get (the first service having the index 0).
	 * @return A #PresenceService object if successful, null otherwise.
	 */
	PresenceService getNthService(long idx);

	/**
	 * Adds a service to a presence model.
	 * @param service The #PresenceService object to add to the model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addService(PresenceService service);

	/**
	 * Clears the services of a presence model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearServices();

	/**
	 * Gets the number of persons included in the presence model.
	 * @return The number of persons included in the #PresenceModel object.
	 */
	long getNbPersons();

	/**
	 * Gets the nth person of a presence model.
	 * @param idx The index of the person to get (the first person having the index 0).
	 * @return A pointer to a #PresencePerson object if successful, null otherwise.
	 */
	PresencePerson getNthPerson(long idx);

	/**
	 * Adds a person to a presence model.
	 * @param person The #PresencePerson object to add to the model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int addPerson(PresencePerson person);

	/**
	 * Clears the persons of a presence model.
	 * @return 0 if successful, a value &lt; 0 in case of error.
	 */
	int clearPersons();

}
