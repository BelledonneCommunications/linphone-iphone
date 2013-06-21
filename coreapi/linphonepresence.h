/*
linphonepresence.h
Copyright (C) 2010-2013  Belledonne Communications SARL

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

#ifndef LINPHONEPRESENCE_H_
#define LINPHONEPRESENCE_H_


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup buddy_list
 * @{
 */


/** Basic status as defined in section 4.1.4 of RFC 3863 */
typedef enum LinphonePresenceBasicStatus {
	/** This value means that the associated contact element, if any, is ready to accept communication. */
	LinphonePresenceBasicStatusOpen,

	/** This value means that the associated contact element, if any, is unable to accept communication. */
	LinphonePresenceBasicStatusClosed
} LinphonePresenceBasicStatus;

/** Activities as defined in section 3.2 of RFC 4480 */
typedef enum LinphonePresenceActivity {
	/** This value is not defined in the RFC, it corresponds to no activity with a basic status of "closed". */
	LinphonePresenceActivityOffline,

	/** This value is not defined in the RFC, it corresponds to no activity with a basic status of "open". */
	LinphonePresenceActivityOnline,

	/** The person has a calendar appointment, without specifying exactly of what type. This activity is
	 *  indicated if more detailed information is not available or the person chooses not to reveal more
	 * information. */
	LinphonePresenceActivityAppointment,

	/** The person is physically away from all interactive communication devices. */
	LinphonePresenceActivityAway,

	/** The person is eating the first meal of the day, usually eaten in the morning. */
	LinphonePresenceActivityBreakfast,

	/** The person is busy, without further details. */
	LinphonePresenceActivityBusy,

	/** The person is having his or her main meal of the day, eaten in the evening or at midday. */
	LinphonePresenceActivityDinner,

	/**  This is a scheduled national or local holiday. */
	LinphonePresenceActivityHoliday,

	/** The person is riding in a vehicle, such as a car, but not steering. */
	LinphonePresenceActivityInTransit,

	/** The person is looking for (paid) work. */
	LinphonePresenceActivityLookingForWork,

	/** The person is eating his or her midday meal. */
	LinphonePresenceActivityLunch,

	/** The person is scheduled for a meal, without specifying whether it is breakfast, lunch, or dinner,
	 *  or some other meal. */
	LinphonePresenceActivityMeal,

	/** The person is in an assembly or gathering of people, as for a business, social, or religious purpose.
	 *  A meeting is a sub-class of an appointment. */
	LinphonePresenceActivityMeeting,

	/** The person is talking on the telephone. */
	LinphonePresenceActivityOnThePhone,

	/** The person is engaged in an activity with no defined representation. A string describing the activity
	 *  in plain text SHOULD be provided. */
	LinphonePresenceActivityOther,

	/** A performance is a sub-class of an appointment and includes musical, theatrical, and cinematic
	 *  performances as well as lectures. It is distinguished from a meeting by the fact that the person
	 *  may either be lecturing or be in the audience, with a potentially large number of other people,
	 *  making interruptions particularly noticeable. */
	LinphonePresenceActivityPerformance,

	/** The person will not return for the foreseeable future, e.g., because it is no longer working for
	 *  the company. */
	LinphonePresenceActivityPermanentAbsence,

	/** The person is occupying himself or herself in amusement, sport, or other recreation. */
	LinphonePresenceActivityPlaying,

	/** The person is giving a presentation, lecture, or participating in a formal round-table discussion. */
	LinphonePresenceActivityPresentation,

	/** The person is visiting stores in search of goods or services. */
	LinphonePresenceActivityShopping,

	/** The person is sleeping.*/
	LinphonePresenceActivitySleeping,

	/** The person is observing an event, such as a sports event. */
	LinphonePresenceActivitySpectator,

	/** The person is controlling a vehicle, watercraft, or plane. */
	LinphonePresenceActivitySteering,

	/** The person is on a business or personal trip, but not necessarily in-transit. */
	LinphonePresenceActivityTravel,

	/** The person is watching television. */
	LinphonePresenceActivityTV,

	/** The activity of the person is unknown. */
	LinphonePresenceActivityUnknown,

	/** A period of time devoted to pleasure, rest, or relaxation. */
	LinphonePresenceActivityVacation,

	/** The person is engaged in, typically paid, labor, as part of a profession or job. */
	LinphonePresenceActivityWorking,

	/** The person is participating in religious rites. */
	LinphonePresenceActivityWorship
} LinphonePresenceActivityType;

/**
 * Structure holding the information about the presence of a person.
 */
struct _LinphonePresenceModel;

/**
 * Presence model type holding information about the presence of a person.
 */
typedef struct _LinphonePresenceModel LinphonePresenceModel;

/**
 * Structure holding the information about a presence activity.
 */
struct _LinphonePresenceActivity;

/**
 * Presence activity type holding information about a presence activity.
 */
typedef struct _LinphonePresenceActivity LinphonePresenceActivity;

/**
 * Structure holding the information about a presence note.
 */
struct _LinphonePresenceNote;

/**
 * Presence note type holding information about a presence note.
 */
typedef struct _LinphonePresenceNote LinphonePresenceNote;



/**
 * @brief Creates a default presence model.
 * @returns The created presence model, NULL on error.
 * @see linphone_presence_model_new_with_activity
 * @see linphone_presence_model_new_with_activity_and_note
 *
 * The created presence model is considered 'offline'.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new(void);

/**
 * @brief Creates a presence model specifying an activity.
 * @param[in] activity The activity to set for the created presence model.
 * @param[in] description An additional description of the activity (mainly useful for the 'other' activity). Set it to NULL to not add a description.
 * @returns The created presence model, or NULL if an error occured.
 * @see linphone_presence_model_new
 * @see linphone_presence_model_new_with_activity_and_note
 *
 * The created presence model has the activity specified in the parameters.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new_with_activity(LinphonePresenceActivityType activity, const char *description);

/**
 * @brief Creates a presence model specifying an activity and adding a note.
 * @param[in] activity The activity to set for the created presence model.
 * @param[in] description An additional description of the activity (mainly useful for the 'other' activity). Set it to NULL to not add a description.
 * @param[in] note An additional note giving additional information about the contact presence.
 * @param[in] lang The language the note is written in. It can be set to NULL in order to not specify the language of the note.
 * @returns The created presence model, or NULL if an error occured.
 * @see linphone_presence_model_new_with_activity
 * @see linphone_presence_model_new_with_activity_and_note
 *
 * The created presence model has the activity and the note specified in the parameters.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new_with_activity_and_note(LinphonePresenceActivityType activity, const char *description, const char *note, const char *lang);

/**
 * @brief Deletes a presence model.
 * @param[in] model The #LinphonePresenceModel object to delete.
 */
LINPHONE_PUBLIC void linphone_presence_model_delete(LinphonePresenceModel *model);

/**
 * @brief Compares two presence models.
 * @param[in] m1 The first #LinphonePresenceModel object.
 * @param[in] m2 The second #LinphonePresenceModel object.
 * @return TRUE if the #LinphonePresenceModel objects are equals, FALSE otherwise.
 */
LINPHONE_PUBLIC bool_t linphone_presence_model_equals(const LinphonePresenceModel *m1, const LinphonePresenceModel *m2);

/**
 * @brief Gets the basic status of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the basic status from.
 * @return The #LinphonePresenceBasicStatus of the #LinphonePresenceModel object given as parameter.
 */
LINPHONE_PUBLIC LinphonePresenceBasicStatus linphone_presence_model_get_basic_status(const LinphonePresenceModel *model);

/**
 * @brief Gets the number of activities included in the presence model.
 * @param[in] model The #LinphonePresenceModel object to get the number of activities from.
 * @return The number of activities included in the #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_model_nb_activities(const LinphonePresenceModel *model);

/**
 * @brief Gets the nth activity of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the activity from.
 * @param[in] idx The index of the activity to get (the first activity having the index 0).
 * @return A pointer to a #LinphonePresenceActivity object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_model_get_nth_activity(const LinphonePresenceModel *model, unsigned int idx);

/**
 * @brief Gets the first activity of a presence model (there is usually only one).
 * @param[in] model The #LinphonePresenceModel object to get the activity from.
 * @return A #LinphonePresenceActivity object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_model_get_activity(const LinphonePresenceModel *model);

/**
 * @brief Sets the activity of a presence model (limits to only one activity).
 * @param[in] model The #LinphonePresenceModel object for which to set the activity.
 * @param[in] activity The #LinphonePresenceActivityType to set for the model.
 * @param[in] description An additional description of the activity to set for the model. Can be NULL if no additional description is to be added.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_set_activity(LinphonePresenceModel *model, LinphonePresenceActivityType activity, const char *description);

/**
 * @brief Gets the first note of a presence model (there is usually only one).
 * @param[in] model The #LinphonePresenceModel object to get the note from.
 * @param[in] lang The language of the note to get. Can be NULL to a note that has no language specified or to get the first note whatever language it is written into.
 * @return A pointer to a #LinphonePresenceNote object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_model_get_note(const LinphonePresenceModel *model, const char *lang);

/**
 * @brief Adds a note to a presence model.
 * @param[in] model The #LinphonePresenceModel object to add a note to.
 * @param[in] note_content The note to be added to the presence model.
 * @param[in] lang The language of the note to be added. Can be NULL if no language is to be specified for the note.
 * @return 0 if successful, a value < 0 in case of error.
 *
 * Only one note for each language can be set, so e.g. setting a note for the 'fr' language if there is only one will replace the existing one.
 */
LINPHONE_PUBLIC int linphone_presence_model_add_note(LinphonePresenceModel *model, const char *note_content, const char *lang);

/**
 * @brief Clears all the notes of a presence model.
 * @param[in] model The #LinphonePresenceModel for which to clear notes.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_clear_notes(LinphonePresenceModel *model);

/**
 * @brief Gets the string representation of a presence activity.
 * @param[in] activity A pointer to the #LinphonePresenceActivity object for which to get a string representation.
 * @return A pointer a dynamically allocated string representing the given activity.
 *
 * The returned string is to be freed by calling ms_free().
 */
LINPHONE_PUBLIC char * linphone_presence_activity_to_string(const LinphonePresenceActivity * activity);

/**
 * @brief Gets the activity type of a presence activity.
 * @param[in] activity A pointer to the #LinphonePresenceActivity for which to get the type.
 * @return The #LinphonePresenceActivityType of the activity.
 */
LINPHONE_PUBLIC LinphonePresenceActivityType linphone_presence_activity_get_type(const LinphonePresenceActivity *activity);

/**
 * @brief Gets the description of a presence activity.
 * @param[in] activity A pointer to the #LinphonePresenceActivity for which to get the description.
 * @return A pointer to the description string of the presence activity, or NULL if no description is specified.
 */
LINPHONE_PUBLIC const char * linphone_presence_activity_get_description(const LinphonePresenceActivity *activity);

/**
 * @brief Gets the content of a presence note.
 * @param[in] note A pointer to the #LinphonePresenceNote for which to get the content.
 * @return A pointer to the content of the presence note.
 */
LINPHONE_PUBLIC const char * linphone_presence_note_get_content(const LinphonePresenceNote *note);

/**
 * @brief Gets the language of a presence note.
 * @param[in] note A pointer to the #LinphonePresenceNote for which to get the language.
 * @return A pointer to the language string of the presence note, or NULL if no language is specified.
 */
LINPHONE_PUBLIC const char * linphone_presence_note_get_lang(const LinphonePresenceNote *note);


/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONEPRESENCE_H_ */
