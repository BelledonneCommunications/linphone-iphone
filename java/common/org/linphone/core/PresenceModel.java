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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

package org.linphone.core;

public interface PresenceModel {

	/**
	 * @brief Gets the basic status of a presence model.
	 * @return The #BasicStatus of the #PresenceModel object.
	 */
	PresenceBasicStatus getBasicStatus();

	/**
	 * @brief Sets the basic status of a presence model.
	 * @param[in] basic_status The #BasicStatus to set for the #PresenceModel object.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int setBasicStatus(PresenceBasicStatus basic_status);

	/**
	 * @brief Gets the timestamp of a presence model.
	 * @return The timestamp of the #LinphonePresenceModel object or -1 on error.
	 */
	long getTimestamp();

	/**
	 * @brief Gets the contact of a presence model.
	 * @return A string containing the contact, or null if no contact is found.
	 */
	String getContact();

	/**
	 * @brief Sets the contact of a presence model.
	 * @param contact The contact string to set.
	 */
	void setContact(String contact);

	/**
	 * @brief Gets the first activity of a presence model (there is usually only one).
	 * @return A #PresenceActivity object if successful, null otherwise.
	 */
	PresenceActivity getActivity();

	/**
	 * @brief Sets the activity of a presence model (limits to only one activity).
	 * @param[in] activity The #PresenceActivityType to set for the model.
	 * @param[in] description An additional description of the activity to set for the model. Can be null if no additional description is to be added.
	 * @return 0 if successful, a value < 0 in case of error.
	 *
	 * WARNING: This method will modify the basic status of the model according to the activity being set.
	 * If you don't want the basic status to be modified automatically, you can use the combination of setBasicStatus(), clearActivities() and addActivity().
	 */
	int setActivity(PresenceActivityType activity, String description);

	/**
	 * @brief Gets the number of activities included in the presence model.
	 * @return The number of activities included in the #PresenceModel object.
	 */
	long getNbActivities();

	/**
	 * @brief Gets the nth activity of a presence model.
	 * @param idx The index of the activity to get (the first activity having the index 0).
	 * @return A #PresenceActivity object if successful, null otherwise.
	 */
	PresenceActivity getNthActivity(long idx);

	/**
	 * @brief Adds an activity to a presence model.
	 * @param[in] activity The #PresenceActivity to add to the model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int addActivity(PresenceActivity activity);

	/**
	 * @brief Clears the activities of a presence model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int clearActivities();

	/**
	 * @brief Gets the first note of a presence model (there is usually only one).
	 * @param[in] lang The language of the note to get. Can be null to get a note that has no language specified or to get the first note whatever language it is written into.
	 * @return A #PresenceNote object if successful, null otherwise.
	 */
	PresenceNote getNote(String lang);

	/**
	 * @brief Adds a note to a presence model.
	 * @param[in] note_content The note to be added to the presence model.
	 * @param[in] lang The language of the note to be added. Can be null if no language is to be specified for the note.
	 * @return 0 if successful, a value < 0 in case of error.
	 *
	 * Only one note for each language can be set, so e.g. setting a note for the 'fr' language if there is only one will replace the existing one.
	 */
	int addNote(String note_content, String lang);

	/**
	 * @brief Clears all the notes of a presence model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int clearNotes();

	/**
	 * @brief Gets the number of services included in the presence model.
	 * @return The number of services included in the #PresenceModel object.
	 */
	long getNbServices();

	/**
	 * @brief Gets the nth service of a presence model.
	 * @param[in] idx The index of the service to get (the first service having the index 0).
	 * @return A #PresenceService object if successful, null otherwise.
	 */
	PresenceService getNthService(long idx);

	/**
	 * @brief Adds a service to a presence model.
	 * @param[in] service The #PresenceService object to add to the model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int addService(PresenceService service);

	/**
	 * @brief Clears the services of a presence model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int clearServices();

	/**
	 * @brief Gets the number of persons included in the presence model.
	 * @return The number of persons included in the #PresenceModel object.
	 */
	long getNbPersons();

	/**
	 * @brief Gets the nth person of a presence model.
	 * @param[in] idx The index of the person to get (the first person having the index 0).
	 * @return A pointer to a #PresencePerson object if successful, null otherwise.
	 */
	PresencePerson getNthPerson(long idx);

	/**
	 * @brief Adds a person to a presence model.
	 * @param[in] person The #PresencePerson object to add to the model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int addPerson(PresencePerson person);

	/**
	 * @brief Clears the persons of a presence model.
	 * @return 0 if successful, a value < 0 in case of error.
	 */
	int clearPersons();

}
