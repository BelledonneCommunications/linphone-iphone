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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
typedef enum LinphonePresenceActivityType {
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
 * Structure holding the information about a presence service.
 */
struct _LinphonePresenceService;

/**
 * Structure holding the information about a presence person.
 */
struct _LinphonePresencePerson;

/**
 * Presence person holding information about a presence person.
 */
typedef struct _LinphonePresencePerson LinphonePresencePerson;

/**
 * Presence service type holding information about a presence service.
 */
typedef struct _LinphonePresenceService LinphonePresenceService;

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



/*****************************************************************************
 * HELPER FUNCTIONS TO EASE ACCESS IN MOST SIMPLER CASES                     *
 ****************************************************************************/

/**
 * Creates a presence model specifying an activity.
 * @param[in] activity The activity to set for the created presence model.
 * @param[in] description An additional description of the activity (mainly useful for the 'other' activity). Set it to NULL to not add a description.
 * @return The created presence model, or NULL if an error occured.
 * @see linphone_presence_model_new
 * @see linphone_presence_model_new_with_activity_and_note
 *
 * The created presence model has the activity specified in the parameters.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new_with_activity(LinphonePresenceActivityType activity, const char *description);

/**
 * Creates a presence model specifying an activity and adding a note.
 * @param[in] activity The activity to set for the created presence model.
 * @param[in] description An additional description of the activity (mainly useful for the 'other' activity). Set it to NULL to not add a description.
 * @param[in] note An additional note giving additional information about the contact presence.
 * @param[in] lang The language the note is written in. It can be set to NULL in order to not specify the language of the note.
 * @return The created presence model, or NULL if an error occured.
 * @see linphone_presence_model_new_with_activity
 * @see linphone_presence_model_new_with_activity_and_note
 *
 * The created presence model has the activity and the note specified in the parameters.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new_with_activity_and_note(LinphonePresenceActivityType activity, const char *description, const char *note, const char *lang);

/**
 * Gets the basic status of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the basic status from.
 * @return The #LinphonePresenceBasicStatus of the #LinphonePresenceModel object given as parameter.
 */
LINPHONE_PUBLIC LinphonePresenceBasicStatus linphone_presence_model_get_basic_status(const LinphonePresenceModel *model);

/**
 *  Sets the basic status of a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to set the basic status.
 * @param[in] basic_status The #LinphonePresenceBasicStatus to set for the #LinphonePresenceModel object.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_set_basic_status(LinphonePresenceModel *model, LinphonePresenceBasicStatus basic_status);

/**
 *  Gets the timestamp of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the timestamp from.
 * @return The timestamp of the #LinphonePresenceModel object or -1 on error.
 */
LINPHONE_PUBLIC time_t linphone_presence_model_get_timestamp(const LinphonePresenceModel *model);

/**
 * Gets the contact of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the contact from.
 * @return A pointer to a dynamically allocated string containing the contact, or NULL if no contact is found.
 *
 * The returned string is to be freed by calling ms_free().
 */
LINPHONE_PUBLIC char * linphone_presence_model_get_contact(const LinphonePresenceModel *model);

/**
 * Sets the contact of a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to set the contact.
 * @param[in] contact The contact string to set.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_set_contact(LinphonePresenceModel *model, const char *contact);

/**
 * Sets the presentity of a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to set the contact.
 * @param[in] presentity The presentity address to set (presentity is copied).
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_set_presentity(LinphonePresenceModel *model, const LinphoneAddress *presentity);
/**
 * Gets the presentity of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the contact from.
 * @return A pointer to a const LinphoneAddress, or NULL if no contact is found.
 *
 */
LINPHONE_PUBLIC const LinphoneAddress * linphone_presence_model_get_presentity(const LinphonePresenceModel *model);
	
/**
 * Gets the first activity of a presence model (there is usually only one).
 * @param[in] model The #LinphonePresenceModel object to get the activity from.
 * @return A #LinphonePresenceActivity object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_model_get_activity(const LinphonePresenceModel *model);

/**
 * Sets the activity of a presence model (limits to only one activity).
 * @param[in] model The #LinphonePresenceModel object for which to set the activity.
 * @param[in] activity The #LinphonePresenceActivityType to set for the model.
 * @param[in] description An additional description of the activity to set for the model. Can be NULL if no additional description is to be added.
 * @return 0 if successful, a value < 0 in case of error.
 *
 * WARNING: This function will modify the basic status of the model according to the activity being set.
 * If you don't want the basic status to be modified automatically, you can use the combination of linphone_presence_model_set_basic_status(),
 * linphone_presence_model_clear_activities() and linphone_presence_model_add_activity().
 */
LINPHONE_PUBLIC int linphone_presence_model_set_activity(LinphonePresenceModel *model, LinphonePresenceActivityType activity, const char *description);

/**
 * Gets the number of activities included in the presence model.
 * @param[in] model The #LinphonePresenceModel object to get the number of activities from.
 * @return The number of activities included in the #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_model_get_nb_activities(const LinphonePresenceModel *model);

/**
 * Gets the nth activity of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the activity from.
 * @param[in] idx The index of the activity to get (the first activity having the index 0).
 * @return A pointer to a #LinphonePresenceActivity object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_model_get_nth_activity(const LinphonePresenceModel *model, unsigned int idx);

/**
 * Adds an activity to a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to add an activity.
 * @param[in] activity The #LinphonePresenceActivity object to add to the model.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_add_activity(LinphonePresenceModel *model, LinphonePresenceActivity *activity);

/**
 * Clears the activities of a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to clear the activities.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_clear_activities(LinphonePresenceModel *model);

/**
 * Gets the first note of a presence model (there is usually only one).
 * @param[in] model The #LinphonePresenceModel object to get the note from.
 * @param[in] lang The language of the note to get. Can be NULL to get a note that has no language specified or to get the first note whatever language it is written into.
 * @return A pointer to a #LinphonePresenceNote object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_model_get_note(const LinphonePresenceModel *model, const char *lang);

/**
 * Adds a note to a presence model.
 * @param[in] model The #LinphonePresenceModel object to add a note to.
 * @param[in] note_content The note to be added to the presence model.
 * @param[in] lang The language of the note to be added. Can be NULL if no language is to be specified for the note.
 * @return 0 if successful, a value < 0 in case of error.
 *
 * Only one note for each language can be set, so e.g. setting a note for the 'fr' language if there is only one will replace the existing one.
 */
LINPHONE_PUBLIC int linphone_presence_model_add_note(LinphonePresenceModel *model, const char *note_content, const char *lang);

/**
 * Clears all the notes of a presence model.
 * @param[in] model The #LinphonePresenceModel for which to clear notes.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_clear_notes(LinphonePresenceModel *model);


/*****************************************************************************
 * PRESENCE MODEL FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES             *
 ****************************************************************************/

/**
 * Creates a default presence model.
 * @return The created presence model, NULL on error.
 * @see linphone_presence_model_new_with_activity
 * @see linphone_presence_model_new_with_activity_and_note
 *
 * The created presence model is considered 'offline'.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_new(void);

/**
 * Gets the number of services included in the presence model.
 * @param[in] model The #LinphonePresenceModel object to get the number of services from.
 * @return The number of services included in the #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_model_get_nb_services(const LinphonePresenceModel *model);

/**
 * Gets the nth service of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the service from.
 * @param[in] idx The index of the service to get (the first service having the index 0).
 * @return A pointer to a #LinphonePresenceService object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceService * linphone_presence_model_get_nth_service(const LinphonePresenceModel *model, unsigned int idx);

/**
 * Adds a service to a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to add a service.
 * @param[in] service The #LinphonePresenceService object to add to the model.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_add_service(LinphonePresenceModel *model, LinphonePresenceService *service);

/**
 * Clears the services of a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to clear the services.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_clear_services(LinphonePresenceModel *model);

/**
 * Gets the number of persons included in the presence model.
 * @param[in] model The #LinphonePresenceModel object to get the number of persons from.
 * @return The number of persons included in the #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_model_get_nb_persons(const LinphonePresenceModel *model);

/**
 * Gets the nth person of a presence model.
 * @param[in] model The #LinphonePresenceModel object to get the person from.
 * @param[in] idx The index of the person to get (the first person having the index 0).
 * @return A pointer to a #LinphonePresencePerson object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresencePerson * linphone_presence_model_get_nth_person(const LinphonePresenceModel *model, unsigned int idx);

/**
 * Adds a person to a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to add a person.
 * @param[in] person The #LinphonePresencePerson object to add to the model.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_add_person(LinphonePresenceModel *model, LinphonePresencePerson *person);

/**
 * Clears the persons of a presence model.
 * @param[in] model The #LinphonePresenceModel object for which to clear the persons.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_model_clear_persons(LinphonePresenceModel *model);


/*****************************************************************************
 * PRESENCE SERVICE FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES           *
 ****************************************************************************/

/**
 * Creates a presence service.
 * @param[in] id The id of the presence service to be created. Can be NULL to generate it automatically.
 * @param[in] basic_status The #LinphonePresenceBasicStatus to set for the #LinphonePresenceService object.
 * @param[in] contact The contact string to set.
 * @return The created presence service, NULL on error.
 *
 * The created presence service has the basic status 'closed'.
 */
LINPHONE_PUBLIC LinphonePresenceService * linphone_presence_service_new(const char *id, LinphonePresenceBasicStatus basic_status, const char *contact);

/**
 * Gets the id of a presence service.
 * @param[in] service The #LinphonePresenceService object to get the id from.
 * @return A pointer to a dynamically allocated string containing the id, or NULL in case of error.
 *
 * The returned string is to be freed by calling ms_free().
 */
LINPHONE_PUBLIC char * linphone_presence_service_get_id(const LinphonePresenceService *service);

/**
 * Sets the id of a presence service.
 * @param[in] service The #LinphonePresenceService object for which to set the id.
 * @param[in] id The id string to set. Can be NULL to generate it automatically.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_service_set_id(LinphonePresenceService *service, const char *id);

/**
 * Gets the basic status of a presence service.
 * @param[in] service The #LinphonePresenceService object to get the basic status from.
 * @return The #LinphonePresenceBasicStatus of the #LinphonePresenceService object given as parameter.
 */
LINPHONE_PUBLIC LinphonePresenceBasicStatus linphone_presence_service_get_basic_status(const LinphonePresenceService *service);

/**
 * Sets the basic status of a presence service.
 * @param[in] service The #LinphonePresenceService object for which to set the basic status.
 * @param[in] basic_status The #LinphonePresenceBasicStatus to set for the #LinphonePresenceService object.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_service_set_basic_status(LinphonePresenceService *service, LinphonePresenceBasicStatus basic_status);

/**
 * Gets the contact of a presence service.
 * @param[in] service The #LinphonePresenceService object to get the contact from.
 * @return A pointer to a dynamically allocated string containing the contact, or NULL if no contact is found.
 *
 * The returned string is to be freed by calling ms_free().
 */
LINPHONE_PUBLIC char * linphone_presence_service_get_contact(const LinphonePresenceService *service);

/**
 * Sets the contact of a presence service.
 * @param[in] service The #LinphonePresenceService object for which to set the contact.
 * @param[in] contact The contact string to set.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_service_set_contact(LinphonePresenceService *service, const char *contact);

/**
 * Gets the number of notes included in the presence service.
 * @param[in] service The #LinphonePresenceService object to get the number of notes from.
 * @return The number of notes included in the #LinphonePresenceService object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_service_get_nb_notes(const LinphonePresenceService *service);

/**
 * Gets the nth note of a presence service.
 * @param[in] service The #LinphonePresenceService object to get the note from.
 * @param[in] idx The index of the note to get (the first note having the index 0).
 * @return A pointer to a #LinphonePresenceNote object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_service_get_nth_note(const LinphonePresenceService *service, unsigned int idx);

/**
 * Adds a note to a presence service.
 * @param[in] service The #LinphonePresenceService object for which to add a note.
 * @param[in] note The #LinphonePresenceNote object to add to the service.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_service_add_note(LinphonePresenceService *service, LinphonePresenceNote *note);

/**
 * Clears the notes of a presence service.
 * @param[in] service The #LinphonePresenceService object for which to clear the notes.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_service_clear_notes(LinphonePresenceService *service);


/*****************************************************************************
 * PRESENCE PERSON FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES            *
 ****************************************************************************/

/**
 * Creates a presence person.
 * @param[in] id The id of the presence person to be created. Can be NULL to generate it automatically.
 * @return The created presence person, NULL on error.
 */
LINPHONE_PUBLIC LinphonePresencePerson * linphone_presence_person_new(const char *id);

/**
 * Gets the id of a presence person.
 * @param[in] person The #LinphonePresencePerson object to get the id from.
 * @return A pointer to a dynamically allocated string containing the id, or NULL in case of error.
 *
 * The returned string is to be freed by calling ms_free().
 */
LINPHONE_PUBLIC char * linphone_presence_person_get_id(const LinphonePresencePerson *person);

/**
 * Sets the id of a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to set the id.
 * @param[in] id The id string to set. Can be NULL to generate it automatically.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_set_id(LinphonePresencePerson *person, const char *id);

/**
 * Gets the number of activities included in the presence person.
 * @param[in] person The #LinphonePresencePerson object to get the number of activities from.
 * @return The number of activities included in the #LinphonePresencePerson object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_person_get_nb_activities(const LinphonePresencePerson *person);

/**
 * Gets the nth activity of a presence person.
 * @param[in] person The #LinphonePresencePerson object to get the activity from.
 * @param[in] idx The index of the activity to get (the first activity having the index 0).
 * @return A pointer to a #LinphonePresenceActivity object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_person_get_nth_activity(const LinphonePresencePerson *person, unsigned int idx);

/**
 * Adds an activity to a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to add an activity.
 * @param[in] activity The #LinphonePresenceActivity object to add to the person.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_add_activity(LinphonePresencePerson *person, LinphonePresenceActivity *activity);

/**
 * Clears the activities of a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to clear the activities.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_clear_activities(LinphonePresencePerson *person);

/**
 * Gets the number of notes included in the presence person.
 * @param[in] person The #LinphonePresencePerson object to get the number of notes from.
 * @return The number of notes included in the #LinphonePresencePerson object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_person_get_nb_notes(const LinphonePresencePerson *person);

/**
 * Gets the nth note of a presence person.
 * @param[in] person The #LinphonePresencePerson object to get the note from.
 * @param[in] idx The index of the note to get (the first note having the index 0).
 * @return A pointer to a #LinphonePresenceNote object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_person_get_nth_note(const LinphonePresencePerson *person, unsigned int idx);

/**
 * Adds a note to a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to add a note.
 * @param[in] note The #LinphonePresenceNote object to add to the person.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_add_note(LinphonePresencePerson *person, LinphonePresenceNote *note);

/**
 * Clears the notes of a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to clear the notes.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_clear_notes(LinphonePresencePerson *person);

/**
 * Gets the number of activities notes included in the presence person.
 * @param[in] person The #LinphonePresencePerson object to get the number of activities notes from.
 * @return The number of activities notes included in the #LinphonePresencePerson object.
 */
LINPHONE_PUBLIC unsigned int linphone_presence_person_get_nb_activities_notes(const LinphonePresencePerson *person);

/**
 * Gets the nth activities note of a presence person.
 * @param[in] person The #LinphonePresencePerson object to get the activities note from.
 * @param[in] idx The index of the activities note to get (the first note having the index 0).
 * @return A pointer to a #LinphonePresenceNote object if successful, NULL otherwise.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_person_get_nth_activities_note(const LinphonePresencePerson *person, unsigned int idx);

/**
 * Adds an activities note to a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to add an activities note.
 * @param[in] note The #LinphonePresenceNote object to add to the person.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_add_activities_note(LinphonePresencePerson *person, LinphonePresenceNote *note);

/**
 * Clears the activities notes of a presence person.
 * @param[in] person The #LinphonePresencePerson object for which to clear the activities notes.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_person_clear_activities_notes(LinphonePresencePerson *person);


/*****************************************************************************
 * PRESENCE ACTIVITY FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES          *
 ****************************************************************************/

/**
 * Creates a presence activity.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @return The created presence activity, NULL on error.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_activity_new(LinphonePresenceActivityType acttype, const char *description);

/**
 * Gets the string representation of a presence activity.
 * @param[in] activity A pointer to the #LinphonePresenceActivity object for which to get a string representation.
 * @return A pointer a dynamically allocated string representing the given activity.
 *
 * The returned string is to be freed by calling ms_free().
 */
LINPHONE_PUBLIC char * linphone_presence_activity_to_string(const LinphonePresenceActivity * activity);

/**
 * Gets the activity type of a presence activity.
 * @param[in] activity A pointer to the #LinphonePresenceActivity for which to get the type.
 * @return The #LinphonePresenceActivityType of the activity.
 */
LINPHONE_PUBLIC LinphonePresenceActivityType linphone_presence_activity_get_type(const LinphonePresenceActivity *activity);

/**
 * Sets the type of activity of a presence activity.
 * @param[in] activity The #LinphonePresenceActivity for which to set for the activity type.
 * @param[in] acttype The activity type to set for the activity.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_activity_set_type(LinphonePresenceActivity *activity, LinphonePresenceActivityType acttype);

/**
 * Gets the description of a presence activity.
 * @param[in] activity A pointer to the #LinphonePresenceActivity for which to get the description.
 * @return A pointer to the description string of the presence activity, or NULL if no description is specified.
 */
LINPHONE_PUBLIC const char * linphone_presence_activity_get_description(const LinphonePresenceActivity *activity);

/**
 * Sets the description of a presence activity.
 * @param[in] activity The #LinphonePresenceActivity object for which to set the description.
 * @param[in] description An additional description of the activity. Can be NULL if no additional description is to be added.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_activity_set_description(LinphonePresenceActivity *activity, const char *description);


/*****************************************************************************
 * PRESENCE NOTE FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES              *
 ****************************************************************************/

/**
 * Creates a presence note.
 * @param[in] content The content of the note to be created.
 * @param[in] lang The language of the note to be created. Can be NULL if no language is to be specified for the note.
 * @return The created presence note, NULL on error.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_note_new(const char *content, const char *lang);

/**
 * Gets the content of a presence note.
 * @param[in] note A pointer to the #LinphonePresenceNote for which to get the content.
 * @return A pointer to the content of the presence note.
 */
LINPHONE_PUBLIC const char * linphone_presence_note_get_content(const LinphonePresenceNote *note);

/**
 * Sets the content of a presence note.
 * @param[in] note The #LinphonePresenceNote object for which to set the content.
 * @param[in] content The content of the note.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_note_set_content(LinphonePresenceNote *note, const char *content);

/**
 * Gets the language of a presence note.
 * @param[in] note A pointer to the #LinphonePresenceNote for which to get the language.
 * @return A pointer to the language string of the presence note, or NULL if no language is specified.
 */
LINPHONE_PUBLIC const char * linphone_presence_note_get_lang(const LinphonePresenceNote *note);

/**
 * Sets the language of a presence note.
 * @param[in] note The #LinphonePresenceNote object for which to set the language.
 * @param[in] lang The language of the note.
 * @return 0 if successful, a value < 0 in case of error.
 */
LINPHONE_PUBLIC int linphone_presence_note_set_lang(LinphonePresenceNote *note, const char *lang);


/*****************************************************************************
 * PRESENCE INTERNAL FUNCTIONS FOR WRAPPERS IN OTHER PROGRAMMING LANGUAGES   *
 ****************************************************************************/

/**
 * Increase the reference count of the #LinphonePresenceModel object.
 * @param[in] model The #LinphonePresenceModel object for which the reference count is to be increased.
 * @return The #LinphonePresenceModel object with the increased reference count.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_ref(LinphonePresenceModel *model);

/**
 * Decrease the reference count of the #LinphonePresenceModel object and destroy it if it reaches 0.
 * @param[in] model The #LinphonePresenceModel object for which the reference count is to be decreased.
 * @return The #LinphonePresenceModel object if the reference count is still positive, NULL if the object has been destroyed.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_presence_model_unref(LinphonePresenceModel *model);

/**
 * Sets the user data of a #LinphonePresenceModel object.
 * @param[in] model The #LinphonePresenceModel object for which to set the user data.
 * @param[in] user_data A pointer to the user data to set.
 */
LINPHONE_PUBLIC void linphone_presence_model_set_user_data(LinphonePresenceModel *model, void *user_data);

/**
 * Gets the user data of a #LinphonePresenceModel object.
 * @param[in] model The #LinphonePresenceModel object for which to get the user data.
 * @return A pointer to the user data.
 */
LINPHONE_PUBLIC void * linphone_presence_model_get_user_data(const LinphonePresenceModel *model);

/**
 * Increase the reference count of the #LinphonePresenceService object.
 * @param[in] service The #LinphonePresenceService object for which the reference count is to be increased.
 * @return The #LinphonePresenceService object with the increased reference count.
 */
LINPHONE_PUBLIC LinphonePresenceService * linphone_presence_service_ref(LinphonePresenceService *service);

/**
 * Decrease the reference count of the #LinphonePresenceService object and destroy it if it reaches 0.
 * @param[in] service The #LinphonePresenceService object for which the reference count is to be decreased.
 * @return The #LinphonePresenceService object if the reference count is still positive, NULL if the object has been destroyed.
 */
LINPHONE_PUBLIC LinphonePresenceService * linphone_presence_service_unref(LinphonePresenceService *service);

/**
 * Sets the user data of a #LinphonePresenceService object.
 * @param[in] service The #LinphonePresenceService object for which to set the user data.
 * @param[in] user_data A pointer to the user data to set.
 */
LINPHONE_PUBLIC void linphone_presence_service_set_user_data(LinphonePresenceService *service, void *user_data);

/**
 * Gets the user data of a #LinphonePresenceService object.
 * @param[in] service The #LinphonePresenceService object for which to get the user data.
 * @return A pointer to the user data.
 */
LINPHONE_PUBLIC void * linphone_presence_service_get_user_data(const LinphonePresenceService *service);

/**
 * Increase the reference count of the #LinphonePresencePerson object.
 * @param[in] person The #LinphonePresencePerson object for which the reference count is to be increased.
 * @return The #LinphonePresencePerson object with the increased reference count.
 */
LINPHONE_PUBLIC LinphonePresencePerson * linphone_presence_person_ref(LinphonePresencePerson *person);

/**
 * Decrease the reference count of the #LinphonePresencePerson object and destroy it if it reaches 0.
 * @param[in] person The #LinphonePresencePerson object for which the reference count is to be decreased.
 * @return The #LinphonePresencePerson object if the reference count is still positive, NULL if the object has been destroyed.
 */
LINPHONE_PUBLIC LinphonePresencePerson * linphone_presence_person_unref(LinphonePresencePerson *person);

/**
 * Sets the user data of a #LinphonePresencePerson object.
 * @param[in] person The #LinphonePresencePerson object for which to set the user data.
 * @param[in] user_data A pointer to the user data to set.
 */
LINPHONE_PUBLIC void linphone_presence_person_set_user_data(LinphonePresencePerson *person, void *user_data);

/**
 * Gets the user data of a #LinphonePresencePerson object.
 * @param[in] person The #LinphonePresencePerson object for which to get the user data.
 * @return A pointer to the user data.
 */
LINPHONE_PUBLIC void * linphone_presence_person_get_user_data(const LinphonePresencePerson *person);

/**
 * Increase the reference count of the #LinphonePresenceActivity object.
 * @param[in] activity The #LinphonePresenceActivity object for which the reference count is to be increased.
 * @return The #LinphonePresenceActivity object with the increased reference count.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_activity_ref(LinphonePresenceActivity *activity);

/**
 * Decrease the reference count of the #LinphonePresenceActivity object and destroy it if it reaches 0.
 * @param[in] activity The #LinphonePresenceActivity object for which the reference count is to be decreased.
 * @return The #LinphonePresenceActivity object if the reference count is still positive, NULL if the object has been destroyed.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_presence_activity_unref(LinphonePresenceActivity *activity);

/**
 * Sets the user data of a #LinphonePresenceActivity object.
 * @param[in] activity The #LinphonePresenceActivity object for which to set the user data.
 * @param[in] user_data A pointer to the user data to set.
 */
LINPHONE_PUBLIC void linphone_presence_activity_set_user_data(LinphonePresenceActivity *activity, void *user_data);

/**
 * Gets the user data of a #LinphonePresenceActivity object.
 * @param[in] activity The #LinphonePresenceActivity object for which to get the user data.
 * @return A pointer to the user data.
 */
LINPHONE_PUBLIC void * linphone_presence_activity_get_user_data(const LinphonePresenceActivity *activity);

/**
 * Increase the reference count of the #LinphonePresenceNote object.
 * @param[in] note The #LinphonePresenceNote object for which the reference count is to be increased.
 * @return The #LinphonePresenceNote object with the increased reference count.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_note_ref(LinphonePresenceNote *note);

/**
 * Decrease the reference count of the #LinphonePresenceNote object and destroy it if it reaches 0.
 * @param[in] note The #LinphonePresenceNote object for which the reference count is to be decreased.
 * @return The #LinphonePresenceNote object if the reference count is still positive, NULL if the object has been destroyed.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_presence_note_unref(LinphonePresenceNote *note);

/**
 * Sets the user data of a #LinphonePresenceNote object.
 * @param[in] note The #LinphonePresenceNote object for which to set the user data.
 * @param[in] user_data A pointer to the user data to set.
 */
LINPHONE_PUBLIC void linphone_presence_note_set_user_data(LinphonePresenceNote *note, void *user_data);

/**
 * Gets the user data of a #LinphonePresenceNote object.
 * @param[in] note The #LinphonePresenceNote object for which to get the user data.
 * @return A pointer to the user data.
 */
LINPHONE_PUBLIC void * linphone_presence_note_get_user_data(const LinphonePresenceNote *note);


/*****************************************************************************
 * LINPHONE CORE FUNCTIONS RELATED TO PRESENCE                               *
 ****************************************************************************/

/**
 * Create a LinphonePresenceActivity with the given type and description.
 * @param[in] lc #LinphoneCore object.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @return The created #LinphonePresenceActivity object.
 */
LINPHONE_PUBLIC LinphonePresenceActivity * linphone_core_create_presence_activity(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description);

/**
 * Create a default LinphonePresenceModel.
 * @param[in] lc #LinphoneCore object.
 * @return The created #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_create_presence_model(LinphoneCore *lc);

/**
 * Create a LinphonePresenceModel with the given activity type and activity description.
 * @param[in] lc #LinphoneCore object.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity of the created model.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @return The created #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_create_presence_model_with_activity(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description);

/**
 * Create a LinphonePresenceModel with the given activity type, activity description, note content and note language.
 * @param[in] lc #LinphoneCore object.
 * @param[in] acttype The #LinphonePresenceActivityType to set for the activity of the created model.
 * @param[in] description An additional description of the activity to set for the activity. Can be NULL if no additional description is to be added.
 * @param[in] note The content of the note to be added to the created model.
 * @param[in] lang The language of the note to be added to the created model.
 * @return The created #LinphonePresenceModel object.
 */
LINPHONE_PUBLIC LinphonePresenceModel * linphone_core_create_presence_model_with_activity_and_note(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description, const char *note, const char *lang);

/**
 * Create a LinphonePresenceNote with the given content and language.
 * @param[in] lc #LinphoneCore object.
 * @param[in] content The content of the note to be created.
 * @param[in] lang The language of the note to be created.
 * @return The created #LinphonePresenceNote object.
 */
LINPHONE_PUBLIC LinphonePresenceNote * linphone_core_create_presence_note(LinphoneCore *lc, const char *content, const char *lang);

/**
 * Create a LinphonePresencePerson with the given id.
 * @param[in] lc #LinphoneCore object
 * @param[in] id The id of the person to be created.
 * @return The created #LinphonePresencePerson object.
 */
LINPHONE_PUBLIC LinphonePresencePerson * linphone_core_create_presence_person(LinphoneCore *lc, const char *id);

/**
 * Create a LinphonePresenceService with the given id, basic status and contact.
 * @param[in] lc #LinphoneCore object.
 * @param[in] id The id of the service to be created.
 * @param[in] basic_status The basic status of the service to be created.
 * @param[in] contact A string containing a contact information corresponding to the service to be created.
 * @return The created #LinphonePresenceService object.
 */
LINPHONE_PUBLIC LinphonePresenceService * linphone_core_create_presence_service(LinphoneCore *lc, const char *id, LinphonePresenceBasicStatus basic_status, const char *contact);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONEPRESENCE_H_ */
