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

	/** Basic status as defined in section 4.1.4 of RFC 3863 */
	public enum BasicStatus {
		/** This value means that the associated contact element, if any, is ready to accept communication. */
		Open(0),
		/** This value means that the associated contact element, if any, is unable to accept communication. */
		Closed(1),
		Invalid(2);

		protected final int mValue;

		private BasicStatus(int value) {
			mValue = value;
		}

		public int toInt() {
			return mValue;
		}

		static protected BasicStatus fromInt(int value) {
			switch (value) {
			case 0: return Open;
			case 1: return Closed;
			default: return Invalid;
			}
		}
	}



	/**
	 * @brief Gets the basic status of a presence model.
	 * @return The #BasicStatus of the #PresenceModel object.
	 */
	BasicStatus getBasicStatus();

	/**
	 * @brief Gets the number of activities included in the presence model.
	 * @return The number of activities included in the #PresenceModel object.
	 */
	long nbActivities();

	/**
	 * @brief Gets the nth activity of a presence model.
	 * @param idx The index of the activity to get (the first activity having the index 0).
	 * @return A #PresenceActivity object if successful, null otherwise.
	 */
	PresenceActivity getNthActivity(long idx);

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
	 */
	int setActivity(PresenceActivityType activity, String description);

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

}
