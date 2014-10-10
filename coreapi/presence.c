/*
linphone
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

#include "linphonecore.h"
#include "private.h"
#include "lpconfig.h"
#include "linphonepresence.h"



extern const char *__policy_enum_to_str(LinphoneSubscribePolicy pol);



struct _LinphonePresenceNote {
	void *user_data;
	int refcnt;
	char *lang;
	char *content;
};

struct _LinphonePresenceService {
	void *user_data;
	int refcnt;
	char *id;
	LinphonePresenceBasicStatus status;
	char *contact;
	MSList *notes;				/**< A list of _LinphonePresenceNote structures. */
	time_t timestamp;
};

struct _LinphonePresenceActivity {
	void *user_data;
	int refcnt;
	LinphonePresenceActivityType type;
	char *description;
};

struct _LinphonePresencePerson {
	void *user_data;
	int refcnt;
	char *id;
	MSList *activities;		/**< A list of _LinphonePresenceActivity structures. */
	MSList *activities_notes;	/**< A list of _LinphonePresenceNote structures. */
	MSList *notes;			/**< A list of _LinphonePresenceNote structures. */
	time_t timestamp;
};

/**
 * Represents the presence model as defined in RFC 4479 and RFC 4480.
 * This model is not complete. For example, it does not handle devices.
 */
struct _LinphonePresenceModel {
	void *user_data;
	int refcnt;
	MSList *services;	/**< A list of _LinphonePresenceService structures. Also named tuples in the RFC. */
	MSList *persons;	/**< A list of _LinphonePresencePerson structures. */
	MSList *notes;		/**< A list of _LinphonePresenceNote structures. */
};


static const char *person_prefix = "/pidf:presence/dm:person";


/*****************************************************************************
 * PRIVATE FUNCTIONS                                                         *
 ****************************************************************************/
/*defined in http://www.w3.org/TR/REC-xml/*/
static char presence_id_valid_characters[] = "0123456789abcdefghijklmnopqrstuvwxyz-.";
/*NameStartChar (NameChar)**/
static char presence_id_valid_start_characters[] = ":_abcdefghijklmnopqrstuvwxyz";

static char * generate_presence_id(void) {
	char id[7];
	int i;
	id[0] = presence_id_valid_start_characters[ortp_random() % (sizeof(presence_id_valid_start_characters)-1)];
	for (i = 1; i < 6; i++) {
		id[i] = presence_id_valid_characters[ortp_random() % (sizeof(presence_id_valid_characters)-1)];
	}
	id[6] = '\0';

	return ms_strdup(id);
}

static const char * presence_basic_status_to_string(LinphonePresenceBasicStatus basic_status) {
	switch (basic_status) {
		case LinphonePresenceBasicStatusOpen:
			return "open";
		case LinphonePresenceBasicStatusClosed:
		default:
			return "closed";
	}
}

static void presence_note_delete(LinphonePresenceNote *note) {
	ms_free(note->content);
	if (note->lang != NULL) {
		ms_free(note->lang);
	}
	ms_free(note);
}

static LinphonePresenceService * presence_service_new(const char *id, LinphonePresenceBasicStatus status) {
	LinphonePresenceService *service = ms_new0(LinphonePresenceService, 1);
	service->refcnt = 1;
	if (id != NULL) {
		service->id = ms_strdup(id);
	}
	service->status = status;
	service->timestamp = time(NULL);
	return service;
}

static void presence_service_delete(LinphonePresenceService *service) {
	if (service->id != NULL) {
		ms_free(service->id);
	}
	if (service->contact != NULL) {
		ms_free(service->contact);
	}
	ms_list_for_each(service->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(service->notes);
	ms_free(service);
};

static void presence_service_set_timestamp(LinphonePresenceService *service, time_t timestamp) {
	service->timestamp = timestamp;
}

static void presence_service_add_note(LinphonePresenceService *service, LinphonePresenceNote *note) {
	service->notes = ms_list_append(service->notes, note);
}

static void presence_activity_delete(LinphonePresenceActivity *activity) {
	if (activity->description != NULL) {
		ms_free(activity->description);
	}
	ms_free(activity);
}

static time_t parse_timestamp(const char *timestamp) {
	struct tm ret;
	time_t seconds;

	memset(&ret, 0, sizeof(ret));
	sscanf(timestamp, "%d-%d-%dT%d:%d:%d",
	       &ret.tm_year, &ret.tm_mon, &ret.tm_mday, &ret.tm_hour, &ret.tm_min, &ret.tm_sec);
	ret.tm_mon--;
	ret.tm_year -= 1900;
	ret.tm_isdst = 0;
	seconds = mktime(&ret);
	if (seconds == (time_t)-1) {
		ms_error("mktime() failed: %s", strerror(errno));
		return (time_t)-1;
	}
	return seconds - timezone;
}

char * linphone_timestamp_to_rfc3339_string(time_t timestamp) {
	char timestamp_str[22];
	struct tm *ret;
#ifndef WIN32
	struct tm gmt;
	ret = gmtime_r(&timestamp,&gmt);
#else
	ret = gmtime(&timestamp);
#endif
	snprintf(timestamp_str, sizeof(timestamp_str), "%4d-%02d-%02dT%02d:%02d:%02dZ",
		 ret->tm_year + 1900, ret->tm_mon + 1, ret->tm_mday, ret->tm_hour, ret->tm_min, ret->tm_sec);
	return ms_strdup(timestamp_str);
}

static LinphonePresencePerson * presence_person_new(const char *id,  time_t timestamp) {
	LinphonePresencePerson *person = ms_new0(LinphonePresencePerson, 1);
	person->refcnt = 1;
	if (id != NULL) {
		person->id = ms_strdup(id);
	}
	if (person->timestamp == ((time_t)-1))
		person->timestamp = time(NULL);
	else
		person->timestamp = timestamp;
	return person;
}

static void presence_person_delete(LinphonePresencePerson *person) {
	if (person->id != NULL) {
		ms_free(person->id);
	}
	ms_list_for_each(person->activities, (MSIterateFunc)linphone_presence_activity_unref);
	ms_list_free(person->activities);
	ms_list_for_each(person->activities_notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->activities_notes);
	ms_list_for_each(person->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->notes);
	ms_free(person);
}

static void presence_person_add_activities_note(LinphonePresencePerson *person, LinphonePresenceNote *note) {
	person->activities_notes = ms_list_append(person->activities_notes, note);
}

static void presence_person_add_note(LinphonePresencePerson *person, LinphonePresenceNote *note) {
	person->notes = ms_list_append(person->notes, note);
}

static void presence_model_add_person(LinphonePresenceModel *model, LinphonePresencePerson *person) {
	model->persons = ms_list_append(model->persons, person);
}

static void presence_model_add_note(LinphonePresenceModel *model, LinphonePresenceNote *note) {
	model->notes = ms_list_append(model->notes, note);
}

static void presence_model_find_open_basic_status(LinphonePresenceService *service, LinphonePresenceBasicStatus *status) {
	if (service->status == LinphonePresenceBasicStatusOpen) {
		*status = LinphonePresenceBasicStatusOpen;
	}
}

static void presence_model_delete(LinphonePresenceModel *model) {
	if (model == NULL) return;

	ms_list_for_each(model->services, (MSIterateFunc)linphone_presence_service_unref);
	ms_list_free(model->services);
	ms_list_for_each(model->persons, (MSIterateFunc)linphone_presence_person_unref);
	ms_list_free(model->persons);
	ms_list_for_each(model->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(model->notes);
	ms_free(model);
}



/*****************************************************************************
 * HELPER FUNCTIONS TO EASE ACCESS IN MOST SIMPLER CASES                     *
 ****************************************************************************/

LinphonePresenceModel * linphone_presence_model_new_with_activity(LinphonePresenceActivityType acttype, const char *description) {
	LinphonePresenceModel *model = linphone_presence_model_new();
	if (model != NULL) {
		linphone_presence_model_set_activity(model, acttype, description);
	}
	return model;
}

LinphonePresenceModel * linphone_presence_model_new_with_activity_and_note(LinphonePresenceActivityType acttype, const char *description, const char *note, const char *lang) {
	LinphonePresenceModel *model = linphone_presence_model_new();
	if (model != NULL) {
		linphone_presence_model_set_activity(model, acttype, description);
		linphone_presence_model_add_note(model, note, lang);
	}
	return model;
}

/* Suppose that if at least one service is open, then the model is open. */
LinphonePresenceBasicStatus linphone_presence_model_get_basic_status(const LinphonePresenceModel *model) {
	LinphonePresenceBasicStatus status = LinphonePresenceBasicStatusClosed;
	if (model != NULL) {
		ms_list_for_each2(model->services, (MSIterate2Func)presence_model_find_open_basic_status, &status);
	}
	return status;
}

int linphone_presence_model_set_basic_status(LinphonePresenceModel *model, LinphonePresenceBasicStatus basic_status) {
	LinphonePresenceService *service;
	int err = 0;

	if (model == NULL) return -1;

	linphone_presence_model_clear_services(model);
	service = linphone_presence_service_new(NULL, basic_status, NULL);
	if (service == NULL) return -1;

	err = linphone_presence_model_add_service(model, service);
	linphone_presence_service_unref(service);
	return err;
}

static void presence_service_find_newer_timestamp(LinphonePresenceService *service, time_t *timestamp) {
	if (service->timestamp > *timestamp)
		*timestamp = service->timestamp;
}

static void presence_person_find_newer_timestamp(LinphonePresencePerson *person, time_t *timestamp) {
	if (person->timestamp > *timestamp)
		*timestamp = person->timestamp;
}

time_t linphone_presence_model_get_timestamp(const LinphonePresenceModel *model) {
	time_t timestamp = (time_t)-1;

	if (model == NULL)
		return timestamp;

	ms_list_for_each2(model->services, (MSIterate2Func)presence_service_find_newer_timestamp, &timestamp);
	ms_list_for_each2(model->persons, (MSIterate2Func)presence_person_find_newer_timestamp, &timestamp);

	return timestamp;
}

static void presence_model_find_contact(LinphonePresenceService *service, char **contact) {
	if ((service->contact != NULL) && (*contact == NULL))
		*contact = service->contact;
}

char * linphone_presence_model_get_contact(const LinphonePresenceModel *model) {
	char *contact = NULL;
	ms_list_for_each2(model->services, (MSIterate2Func)presence_model_find_contact, &contact);
	if (contact == NULL) return NULL;
	return ms_strdup(contact);
}

int linphone_presence_model_set_contact(LinphonePresenceModel *model, const char *contact) {
	LinphonePresenceService *service;

	if (model == NULL) return -1;

	service = linphone_presence_model_get_nth_service(model, 0);
	if (service == NULL) {
		service = linphone_presence_service_new(NULL, LinphonePresenceBasicStatusClosed, NULL);
		if (service == NULL) return -1;
		linphone_presence_model_add_service(model, service);
	}
	return linphone_presence_service_set_contact(service, contact);
}

static void presence_model_count_activities(const LinphonePresencePerson *person, unsigned int *nb) {
	*nb += ms_list_size(person->activities);
}

struct _get_activity_st {
	unsigned int requested_idx;
	unsigned int current_idx;
	LinphonePresenceActivity *activity;
};

static void presence_model_get_activity(const LinphonePresencePerson *person, struct _get_activity_st *st) {
	unsigned int size = ms_list_size(person->activities);
	if (st->requested_idx < (st->current_idx + size)) {
		st->activity = (LinphonePresenceActivity *)ms_list_nth_data(person->activities, st->requested_idx - st->current_idx);
	} else {
		st->current_idx += size;
	}
}

LinphonePresenceActivity * linphone_presence_model_get_activity(const LinphonePresenceModel *model) {
	return linphone_presence_model_get_nth_activity(model, 0);
}

int linphone_presence_model_set_activity(LinphonePresenceModel *model, LinphonePresenceActivityType acttype, const char *description) {
	LinphonePresenceBasicStatus basic_status = LinphonePresenceBasicStatusOpen;
	LinphonePresenceActivity *activity;
	int err = 0;

	if (model == NULL) return -1;

	switch (acttype) {
		case LinphonePresenceActivityAppointment:
		case LinphonePresenceActivityBusy:
		case LinphonePresenceActivityMeeting:
		case LinphonePresenceActivityPermanentAbsence:
		case LinphonePresenceActivityOffline:
		case LinphonePresenceActivityWorship:
			basic_status = LinphonePresenceBasicStatusClosed;
			break;
		default:
			basic_status = LinphonePresenceBasicStatusOpen;
			break;
	}
	if (linphone_presence_model_set_basic_status(model, basic_status) < 0) return -1;
	linphone_presence_model_clear_activities(model);
	activity = linphone_presence_activity_new(acttype, description);
	if (activity == NULL) return -1;
	err = linphone_presence_model_add_activity(model, activity);
	linphone_presence_activity_unref(activity);
	return err;
}

unsigned int linphone_presence_model_get_nb_activities(const LinphonePresenceModel *model) {
	unsigned int nb = 0;
	ms_list_for_each2(model->persons, (MSIterate2Func)presence_model_count_activities, &nb);
	return nb;
}

LinphonePresenceActivity * linphone_presence_model_get_nth_activity(const LinphonePresenceModel *model, unsigned int idx) {
	struct _get_activity_st st;

	if ((model == NULL) || (idx >= linphone_presence_model_get_nb_activities(model)))
		return NULL;

	memset(&st, 0, sizeof(st));
	st.requested_idx = idx;
	ms_list_for_each2(model->persons, (MSIterate2Func)presence_model_get_activity, &st);

	return st.activity;
}

int linphone_presence_model_add_activity(LinphonePresenceModel *model, LinphonePresenceActivity *activity) {
	char *id = NULL;
	LinphonePresencePerson *person = NULL;

	if ((model == NULL) || (activity == NULL)) return -1;

	if (ms_list_size(model->persons) == 0) {
		/* There is no person in the presence model, add one. */
		id = generate_presence_id();
		person = presence_person_new(id, time(NULL));
		if (id != NULL) ms_free(id);
		if (person == NULL)
			return -1;

		presence_model_add_person(model, person);
	} else {
		/* Add the activity to the first person in the model. */
		person = (LinphonePresencePerson *)ms_list_nth_data(model->persons, 0);
	}

	linphone_presence_person_add_activity(person, activity);
	return 0;
}

int linphone_presence_model_clear_activities(LinphonePresenceModel *model) {
	if (model == NULL) return -1;

	ms_list_for_each(model->persons, (MSIterateFunc)linphone_presence_person_clear_activities);
	return 0;
}

struct _find_note_st {
	const char *lang;
	LinphonePresenceNote *note;
};

static LinphonePresenceNote * find_presence_note_in_list(MSList *list, const char *lang) {
	int nb;
	int i;

	nb = ms_list_size(list);
	for (i = 0; i < nb; i++) {
		LinphonePresenceNote *note = (LinphonePresenceNote *)ms_list_nth_data(list, i);
		if (lang == NULL) {
			if (note->lang == NULL) {
				return note;
			}
		} else {
			if ((note->lang != NULL) && (strcmp(lang, note->lang) == 0)) {
				return note;
			}
		}
	}

	return NULL;
}

static void find_presence_person_note(LinphonePresencePerson *person, struct _find_note_st *st) {
	/* First look for the note in the activities notes... */
	st->note = find_presence_note_in_list(person->activities_notes, st->lang);
	if (st->note != NULL) return;

	/* ... then look in the person notes. */
	st->note = find_presence_note_in_list(person->notes, st->lang);
}

static void find_presence_service_note(LinphonePresenceService *service, struct _find_note_st *st) {
	st->note = find_presence_note_in_list(service->notes, st->lang);
}

static LinphonePresenceNote * get_first_presence_note_in_list(MSList *list) {
	return (LinphonePresenceNote *)ms_list_nth_data(list, 0);
}

static void get_first_presence_person_note(LinphonePresencePerson *person, struct _find_note_st *st) {
	st->note = get_first_presence_note_in_list(person->activities_notes);
	if (st->note != NULL) return;
	st->note = get_first_presence_note_in_list(person->notes);
}

static void get_first_presence_service_note(LinphonePresenceService *service, struct _find_note_st *st) {
	st->note = get_first_presence_note_in_list(service->notes);
}

LinphonePresenceNote * linphone_presence_model_get_note(const LinphonePresenceModel *model, const char *lang) {
	struct _find_note_st st;

	if (model == NULL) return NULL;

	st.note = NULL;
	if (lang != NULL) {
		/* First try to find a note in the specified language exactly. */
		st.lang = lang;
		ms_list_for_each2(model->persons, (MSIterate2Func)find_presence_person_note, &st);
		if (st.note == NULL) {
			ms_list_for_each2(model->services, (MSIterate2Func)find_presence_service_note, &st);
		}
		if (st.note == NULL) {
			st.note = find_presence_note_in_list(model->notes, lang);
		}
	}

	if (st.note == NULL) {
		/* No notes in the specified language has been found, try to find one without language. */
		st.lang = NULL;
		ms_list_for_each2(model->persons, (MSIterate2Func)find_presence_person_note, &st);
		if (st.note == NULL) {
			ms_list_for_each2(model->services, (MSIterate2Func)find_presence_service_note, &st);
		}
		if (st.note == NULL) {
			st.note = find_presence_note_in_list(model->notes, NULL);
		}
	}

	if (st.note == NULL) {
		/* Still no result, so get the first note even if it is not in the specified language. */
		ms_list_for_each2(model->persons, (MSIterate2Func)get_first_presence_person_note, &st);
		if (st.note == NULL) {
			ms_list_for_each2(model->services, (MSIterate2Func)get_first_presence_service_note, &st);
		}
		if (st.note == NULL) {
			st.note = get_first_presence_note_in_list(model->notes);
		}
	}

	return st.note;
}

int linphone_presence_model_add_note(LinphonePresenceModel *model, const char *note_content, const char *lang) {
	LinphonePresenceService *service;
	LinphonePresenceNote *note;

	if ((model == NULL) || (note_content == NULL))
		return -1;

	/* Will put the note in the first service. */
	service = ms_list_nth_data(model->services, 0);
	if (service == NULL) {
		/* If no service exists, create one. */
		service = presence_service_new(generate_presence_id(), LinphonePresenceBasicStatusClosed);
	}
	if (service == NULL)
		return -1;

	/* Search for an existing note in the specified language. */
	note = find_presence_note_in_list(service->notes, lang);
	if (note == NULL) {
		note = linphone_presence_note_new(note_content, lang);
	} else {
		linphone_presence_note_set_content(note, note_content);
	}
	if (note == NULL)
		return -1;

	presence_service_add_note(service, note);

	return 0;
}

static void clear_presence_person_notes(LinphonePresencePerson *person) {
	ms_list_for_each(person->activities_notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->activities_notes);
	person->activities_notes = NULL;
	ms_list_for_each(person->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->notes);
	person->notes = NULL;
}

static void clear_presence_service_notes(LinphonePresenceService *service) {
	ms_list_for_each(service->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(service->notes);
	service->notes = NULL;
}

int linphone_presence_model_clear_notes(LinphonePresenceModel *model) {
	if (model == NULL)
		return -1;

	ms_list_for_each(model->persons, (MSIterateFunc)clear_presence_person_notes);
	ms_list_for_each(model->services, (MSIterateFunc)clear_presence_service_notes);
	ms_list_for_each(model->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(model->notes);
	model->notes = NULL;

	return 0;
}

/*****************************************************************************
 * PRESENCE MODEL FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES             *
 ****************************************************************************/

LinphonePresenceModel * linphone_presence_model_new(void) {
	LinphonePresenceModel *model = ms_new0(LinphonePresenceModel, 1);
	model->refcnt = 1;
	return model;
}

unsigned int linphone_presence_model_get_nb_services(const LinphonePresenceModel *model) {
	return ms_list_size(model->services);
}

LinphonePresenceService * linphone_presence_model_get_nth_service(const LinphonePresenceModel *model, unsigned int idx) {
	if ((model == NULL) || (idx >= linphone_presence_model_get_nb_services(model)))
		return NULL;

	return (LinphonePresenceService *)ms_list_nth_data(model->services, idx);
}

int linphone_presence_model_add_service(LinphonePresenceModel *model, LinphonePresenceService *service) {
	if ((model == NULL) || (service == NULL)) return -1;
	model->services = ms_list_append(model->services, linphone_presence_service_ref(service));
	return 0;
}

int linphone_presence_model_clear_services(LinphonePresenceModel *model) {
	if (model == NULL) return -1;

	ms_list_for_each(model->services, (MSIterateFunc)linphone_presence_service_unref);
	ms_list_free(model->services);
	model->services = NULL;
	return 0;
}

unsigned int linphone_presence_model_get_nb_persons(const LinphonePresenceModel *model) {
	return ms_list_size(model->persons);
}

LinphonePresencePerson * linphone_presence_model_get_nth_person(const LinphonePresenceModel *model, unsigned int idx) {
	if ((model == NULL) || (idx >= linphone_presence_model_get_nb_persons(model)))
		return NULL;

	return (LinphonePresencePerson *)ms_list_nth_data(model->persons, idx);
}

int linphone_presence_model_add_person(LinphonePresenceModel *model, LinphonePresencePerson *person) {
	if ((model == NULL) || (person == NULL)) return -1;
	model->persons = ms_list_append(model->persons, linphone_presence_person_ref(person));
	return 0;
}

int linphone_presence_model_clear_persons(LinphonePresenceModel *model) {
	if (model == NULL) return -1;

	ms_list_for_each(model->persons, (MSIterateFunc)linphone_presence_person_unref);
	ms_list_free(model->persons);
	model->persons = NULL;
	return 0;
}



/*****************************************************************************
 * PRESENCE SERVICE FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES           *
 ****************************************************************************/

LinphonePresenceService * linphone_presence_service_new(const char *id, LinphonePresenceBasicStatus basic_status, const char *contact) {
	LinphonePresenceService *service;
	char *service_id;
	if (id == NULL)
		service_id = generate_presence_id();
	else
		service_id = ms_strdup(id);
	service = presence_service_new(service_id, basic_status);
	linphone_presence_service_set_contact(service, contact);
	if (service_id != NULL)
		ms_free(service_id);
	return service;
}

char * linphone_presence_service_get_id(const LinphonePresenceService *service) {
	if (service == NULL) return NULL;
	return ms_strdup(service->id);
}

int linphone_presence_service_set_id(LinphonePresenceService *service, const char *id) {
	if (service == NULL) return -1;
	if (service->id != NULL)
		ms_free(service->id);
	if (id == NULL)
		service->id = generate_presence_id();
	else
		service->id = ms_strdup(id);
	return 0;
}

LinphonePresenceBasicStatus linphone_presence_service_get_basic_status(const LinphonePresenceService *service) {
	if (service == NULL) return LinphonePresenceBasicStatusClosed;
	return service->status;
}

int linphone_presence_service_set_basic_status(LinphonePresenceService *service, LinphonePresenceBasicStatus basic_status) {
	if (service == NULL) return -1;
	service->status = basic_status;
	return 0;
}

char * linphone_presence_service_get_contact(const LinphonePresenceService *service) {
	if (service->contact == NULL) return NULL;
	return ms_strdup(service->contact);
}

int linphone_presence_service_set_contact(LinphonePresenceService *service, const char *contact) {
	if (service == NULL) return -1;
	if (service->contact != NULL)
		ms_free(service->contact);
	if (contact != NULL)
		service->contact = ms_strdup(contact);
	else
		service->contact = NULL;
	return 0;
}

unsigned int linphone_presence_service_get_nb_notes(const LinphonePresenceService *service) {
	return ms_list_size(service->notes);
}

LinphonePresenceNote * linphone_presence_service_get_nth_note(const LinphonePresenceService *service, unsigned int idx) {
	if ((service == NULL) || (idx >= linphone_presence_service_get_nb_notes(service)))
		return NULL;

	return (LinphonePresenceNote *)ms_list_nth_data(service->notes, idx);
}

int linphone_presence_service_add_note(LinphonePresenceService *service, LinphonePresenceNote *note) {
	if ((service == NULL) || (note == NULL)) return -1;
	service->notes = ms_list_append(service->notes, linphone_presence_note_ref(note));
	return 0;
}

int linphone_presence_service_clear_notes(LinphonePresenceService *service) {
	if (service == NULL) return -1;

	ms_list_for_each(service->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(service->notes);
	service->notes = NULL;
	return 0;
}



/*****************************************************************************
 * PRESENCE PERSON FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES            *
 ****************************************************************************/

LinphonePresencePerson * linphone_presence_person_new(const char *id) {
	return presence_person_new(id, time(NULL));
}

char * linphone_presence_person_get_id(const LinphonePresencePerson *person) {
	if (person == NULL) return NULL;
	return ms_strdup(person->id);
}

int linphone_presence_person_set_id(LinphonePresencePerson *person, const char *id) {
	if (person == NULL) return -1;
	if (person->id != NULL)
		ms_free(person->id);
	if (id == NULL)
		person->id = generate_presence_id();
	else
		person->id = ms_strdup(id);
	return 0;
}

unsigned int linphone_presence_person_get_nb_activities(const LinphonePresencePerson *person) {
	if (person == NULL) return 0;
	return ms_list_size(person->activities);
}

LinphonePresenceActivity * linphone_presence_person_get_nth_activity(const LinphonePresencePerson *person, unsigned int idx) {
	if ((person == NULL) || (idx >= linphone_presence_person_get_nb_activities(person)))
		return NULL;
	return (LinphonePresenceActivity *)ms_list_nth_data(person->activities, idx);
}

int linphone_presence_person_add_activity(LinphonePresencePerson *person, LinphonePresenceActivity *activity) {
	if ((person == NULL) || (activity == NULL)) return -1;
	person->activities = ms_list_append(person->activities, linphone_presence_activity_ref(activity));
	return 0;
}

int linphone_presence_person_clear_activities(LinphonePresencePerson *person) {
	if (person == NULL) return -1;
	ms_list_for_each(person->activities, (MSIterateFunc)linphone_presence_activity_unref);
	ms_list_free(person->activities);
	person->activities = NULL;
	return 0;
}

unsigned int linphone_presence_person_get_nb_notes(const LinphonePresencePerson *person) {
	if (person == NULL) return 0;
	return ms_list_size(person->notes);
}

LinphonePresenceNote * linphone_presence_person_get_nth_note(const LinphonePresencePerson *person, unsigned int idx) {
	if ((person == NULL) || (idx >= linphone_presence_person_get_nb_notes(person)))
		return NULL;
	return (LinphonePresenceNote *)ms_list_nth_data(person->notes, idx);
}

int linphone_presence_person_add_note(LinphonePresencePerson *person, LinphonePresenceNote *note) {
	if ((person == NULL) || (note == NULL)) return -1;
	person->notes = ms_list_append(person->notes, linphone_presence_note_ref(note));
	return 0;
}

int linphone_presence_person_clear_notes(LinphonePresencePerson *person) {
	if (person == NULL) return -1;
	ms_list_for_each(person->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->notes);
	person->notes = NULL;
	return 0;
}

unsigned int linphone_presence_person_get_nb_activities_notes(const LinphonePresencePerson *person) {
	if (person == NULL) return 0;
	return ms_list_size(person->activities_notes);
}

LinphonePresenceNote * linphone_presence_person_get_nth_activities_note(const LinphonePresencePerson *person, unsigned int idx) {
	if ((person == NULL) || (idx >= linphone_presence_person_get_nb_activities_notes(person)))
		return NULL;
	return (LinphonePresenceNote *)ms_list_nth_data(person->activities_notes, idx);
}

int linphone_presence_person_add_activities_note(LinphonePresencePerson *person, LinphonePresenceNote *note) {
	if ((person == NULL) || (note == NULL)) return -1;
	person->notes = ms_list_append(person->activities_notes, linphone_presence_note_ref(note));
	return 0;
}

int linphone_presence_person_clear_activities_notes(LinphonePresencePerson *person) {
	if (person == NULL) return -1;
	ms_list_for_each(person->activities_notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->activities_notes);
	person->activities_notes = NULL;
	return 0;
}


/*****************************************************************************
 * PRESENCE ACTIVITY FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES          *
 ****************************************************************************/

struct _presence_activity_name_map {
	const char *name;
	LinphonePresenceActivityType type;
};

static struct _presence_activity_name_map activity_map[] = {
	{ "appointment", LinphonePresenceActivityAppointment },
	{ "away", LinphonePresenceActivityAway },
	{ "breakfast", LinphonePresenceActivityBreakfast },
	{ "busy", LinphonePresenceActivityBusy },
	{ "dinner", LinphonePresenceActivityDinner },
	{ "holiday", LinphonePresenceActivityHoliday },
	{ "in-transit", LinphonePresenceActivityInTransit },
	{ "looking-for-work", LinphonePresenceActivityLookingForWork },
	{ "lunch", LinphonePresenceActivityLunch },
	{ "meal", LinphonePresenceActivityMeal },
	{ "meeting", LinphonePresenceActivityMeeting },
	{ "on-the-phone", LinphonePresenceActivityOnThePhone },
	{ "other", LinphonePresenceActivityOther },
	{ "performance", LinphonePresenceActivityPerformance },
	{ "permanent-absence", LinphonePresenceActivityPermanentAbsence },
	{ "playing", LinphonePresenceActivityPlaying },
	{ "presentation", LinphonePresenceActivityPresentation },
	{ "shopping", LinphonePresenceActivityShopping },
	{ "sleeping", LinphonePresenceActivitySleeping },
	{ "spectator", LinphonePresenceActivitySpectator },
	{ "steering", LinphonePresenceActivitySteering },
	{ "travel", LinphonePresenceActivityTravel },
	{ "tv", LinphonePresenceActivityTV },
	{ "unknown", LinphonePresenceActivityUnknown },
	{ "vacation", LinphonePresenceActivityVacation },
	{ "working", LinphonePresenceActivityWorking },
	{ "worship", LinphonePresenceActivityWorship }
};

static int activity_name_to_presence_activity_type(const char *name, LinphonePresenceActivityType *acttype) {
	unsigned int i;
	for (i = 0; i < (sizeof(activity_map) / sizeof(activity_map[0])); i++) {
		if (strcmp(name, activity_map[i].name) == 0) {
			*acttype = activity_map[i].type;
			return 0;
		}
	}
	return -1;
}

static const char * presence_activity_type_to_string(LinphonePresenceActivityType acttype) {
	unsigned int i;
	for (i = 0; i < (sizeof(activity_map) / sizeof(activity_map[0])); i++) {
		if (acttype == activity_map[i].type) {
			return activity_map[i].name;
		}
	}
	return NULL;
}

LinphonePresenceActivity * linphone_presence_activity_new(LinphonePresenceActivityType acttype, const char *description) {
	LinphonePresenceActivity *act = ms_new0(LinphonePresenceActivity, 1);
	act->refcnt = 1;
	act->type = acttype;
	if (description != NULL) {
		act->description = ms_strdup(description);
	}
	return act;
}

char * linphone_presence_activity_to_string(const LinphonePresenceActivity *activity) {
	LinphonePresenceActivityType acttype = linphone_presence_activity_get_type(activity);
	const char *description = linphone_presence_activity_get_description(activity);
	const char *acttype_str;

	if (acttype == LinphonePresenceActivityOffline)
		acttype_str = "offline";
	else if (acttype == LinphonePresenceActivityOnline)
		acttype_str = "online";
	else
		acttype_str = presence_activity_type_to_string(acttype);

	return ms_strdup_printf("%s%s%s", acttype_str,
				(description == NULL) ? "" : ": ",
				(description == NULL) ? "" : description);
}

LinphonePresenceActivityType linphone_presence_activity_get_type(const LinphonePresenceActivity *activity) {
	if (activity == NULL)
		return LinphonePresenceActivityOffline;
	return activity->type;
}

int linphone_presence_activity_set_type(LinphonePresenceActivity *activity, LinphonePresenceActivityType acttype) {
	if (activity == NULL) return -1;
	activity->type = acttype;
	return 0;
}

const char * linphone_presence_activity_get_description(const LinphonePresenceActivity *activity) {
	if (activity == NULL)
		return NULL;
	return activity->description;
}

int linphone_presence_activity_set_description(LinphonePresenceActivity *activity, const char *description) {
	if (activity == NULL) return -1;
	if (activity->description != NULL)
		ms_free(activity->description);
	if (description != NULL)
		activity->description = ms_strdup(description);
	else
		activity->description = NULL;
	return 0;
}



/*****************************************************************************
 * PRESENCE NOTE FUNCTIONS TO GET ACCESS TO ALL FUNCTIONALITIES              *
 ****************************************************************************/

LinphonePresenceNote * linphone_presence_note_new(const char *content, const char *lang) {
	LinphonePresenceNote *note;

	if (content == NULL) return NULL;
	note = ms_new0(LinphonePresenceNote, 1);
	note->refcnt = 1;
	note->content = ms_strdup(content);
	if (lang != NULL) {
		note->lang = ms_strdup(lang);
	}
	return note;
}

const char * linphone_presence_note_get_content(const LinphonePresenceNote *note) {
	if (note == NULL)
		return NULL;
	return note->content;
}

int linphone_presence_note_set_content(LinphonePresenceNote *note, const char *content) {
	if (content == NULL) return -1;
	if (note->content != NULL) {
		ms_free(note->content);
	}
	note->content = ms_strdup(content);
	return 0;
}

const char * linphone_presence_note_get_lang(const LinphonePresenceNote *note) {
	if (note == NULL)
		return NULL;
	return note->lang;
}

int linphone_presence_note_set_lang(LinphonePresenceNote *note, const char *lang) {
	if (note->lang != NULL) {
		ms_free(note->lang);
		note->lang = NULL;
	}
	if (lang != NULL) {
		note->lang = ms_strdup(lang);
	}
	return 0;
}



/*****************************************************************************
 * PRESENCE INTERNAL FUNCTIONS FOR WRAPPERS IN OTHER PROGRAMMING LANGUAGES   *
 ****************************************************************************/

LinphonePresenceModel * linphone_presence_model_ref(LinphonePresenceModel *model) {
	model->refcnt++;
	return model;
}

LinphonePresenceModel * linphone_presence_model_unref(LinphonePresenceModel *model) {
	model->refcnt--;
	if (model->refcnt == 0) {
		presence_model_delete(model);
		return NULL;
	}
	return model;
}

void linphone_presence_model_set_user_data(LinphonePresenceModel *model, void *user_data) {
	model->user_data = user_data;
}

void * linphone_presence_model_get_user_data(const LinphonePresenceModel *model) {
	return model->user_data;
}

LinphonePresenceService * linphone_presence_service_ref(LinphonePresenceService *service) {
	service->refcnt++;
	return service;
}

LinphonePresenceService * linphone_presence_service_unref(LinphonePresenceService *service) {
	service->refcnt--;
	if (service->refcnt == 0) {
		presence_service_delete(service);
		return NULL;
	}
	return service;
}

void linphone_presence_service_set_user_data(LinphonePresenceService *service, void *user_data) {
	service->user_data = user_data;
}

void * linphone_presence_service_get_user_data(const LinphonePresenceService *service) {
	return service->user_data;
}

LinphonePresencePerson * linphone_presence_person_ref(LinphonePresencePerson *person) {
	person->refcnt++;
	return person;
}

LinphonePresencePerson * linphone_presence_person_unref(LinphonePresencePerson *person) {
	person->refcnt--;
	if (person->refcnt == 0) {
		presence_person_delete(person);
		return NULL;
	}
	return person;
}

void linphone_presence_person_set_user_data(LinphonePresencePerson *person, void *user_data) {
	person->user_data = user_data;
}

void * linphone_presence_person_get_user_data(const LinphonePresencePerson *person) {
	return person->user_data;
}

LinphonePresenceActivity * linphone_presence_activity_ref(LinphonePresenceActivity *activity) {
	activity->refcnt++;
	return activity;
}

LinphonePresenceActivity * linphone_presence_activity_unref(LinphonePresenceActivity *activity) {
	activity->refcnt--;
	if (activity->refcnt == 0) {
		presence_activity_delete(activity);
		return NULL;
	}
	return activity;
}

void linphone_presence_activity_set_user_data(LinphonePresenceActivity *activity, void *user_data) {
	activity->user_data = user_data;
}

void * linphone_presence_activity_get_user_data(const LinphonePresenceActivity *activity) {
	return activity->user_data;
}

LinphonePresenceNote * linphone_presence_note_ref(LinphonePresenceNote *note) {
	note->refcnt++;
	return note;
}

LinphonePresenceNote * linphone_presence_note_unref(LinphonePresenceNote *note) {
	note->refcnt--;
	if (note->refcnt == 0) {
		presence_note_delete(note);
		return NULL;
	}
	return note;
}

void linphone_presence_note_set_user_data(LinphonePresenceNote *note, void *user_data) {
	note->user_data = user_data;
}

void * linphone_presence_note_get_user_data(const LinphonePresenceNote *note) {
	return note->user_data;
}



/*****************************************************************************
 * XML PRESENCE INTERNAL HANDLING                                            *
 ****************************************************************************/

static const char *service_prefix = "/pidf:presence/pidf:tuple";

static int process_pidf_xml_presence_service_notes(xmlparsing_context_t *xml_ctx, LinphonePresenceService *service, unsigned int service_idx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr note_object;
	LinphonePresenceNote *note;
	const char *note_str;
	const char *lang;
	int i;

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:note", service_prefix, service_idx);
	note_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:note[%i]", service_prefix, service_idx, i);
			note_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:note[%i]/@xml:lang", service_prefix, service_idx, i);
			lang = linphone_get_xml_text_content(xml_ctx, xpath_str);

			note = linphone_presence_note_new(note_str, lang);
			presence_service_add_note(service, note);
			if (lang != NULL) linphone_free_xml_text_content(lang);
			linphone_free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	return 0;
}

static int process_pidf_xml_presence_services(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr service_object;
	LinphonePresenceService *service;
	const char *basic_status_str;
	const char *service_id_str;
	const char *timestamp_str;
	const char *contact_str;
	LinphonePresenceBasicStatus basic_status;
	int i;

	service_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, service_prefix);
	if ((service_object != NULL) && (service_object->nodesetval != NULL)) {
		for (i = 1; i <= service_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:status/pidf:basic", service_prefix, i);
			basic_status_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (basic_status_str == NULL)
				continue;

			if (strcmp(basic_status_str, "open") == 0) {
				basic_status = LinphonePresenceBasicStatusOpen;
			} else if (strcmp(basic_status_str, "closed") == 0) {
				basic_status = LinphonePresenceBasicStatusClosed;
			} else {
				/* Invalid value for basic status. */
				linphone_free_xml_text_content(basic_status_str);
				return -1;
			}

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:timestamp", service_prefix, i);
			timestamp_str = linphone_get_xml_text_content(xml_ctx, xpath_str);

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:contact", service_prefix, i);
			contact_str = linphone_get_xml_text_content(xml_ctx, xpath_str);

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/@id", service_prefix, i);
			service_id_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			service = presence_service_new(service_id_str, basic_status);
			if (service != NULL) {
				if (timestamp_str != NULL) {
					presence_service_set_timestamp(service, parse_timestamp(timestamp_str));
					linphone_free_xml_text_content(timestamp_str);
				}
				if (contact_str != NULL) {
					linphone_presence_service_set_contact(service, contact_str);
					linphone_free_xml_text_content(contact_str);
				}
				process_pidf_xml_presence_service_notes(xml_ctx, service, i);
				linphone_presence_model_add_service(model, service);
			}
			linphone_free_xml_text_content(basic_status_str);
			if (service_id_str != NULL) linphone_free_xml_text_content(service_id_str);
		}
	}
	if (service_object != NULL) xmlXPathFreeObject(service_object);

	return 0;
}

static bool_t is_valid_activity_name(const char *name) {
	unsigned int i;
	for (i = 0; i < (sizeof(activity_map) / sizeof(activity_map[0])); i++) {
		if (strcmp(name, activity_map[i].name) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static int process_pidf_xml_presence_person_activities(xmlparsing_context_t *xml_ctx, LinphonePresencePerson *person, unsigned int person_idx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr activities_nodes_object;
	xmlXPathObjectPtr activities_object;
	xmlNodePtr activity_node;
	LinphonePresenceActivity *activity;
	const char *description;
	int i, j;
	int err = 0;

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities", person_prefix, person_idx);
	activities_nodes_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((activities_nodes_object != NULL) && (activities_nodes_object->nodesetval != NULL)) {
		for (i = 1; i <= activities_nodes_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities[%i]/rpid:*", person_prefix, person_idx, i);
			activities_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
			if ((activities_object != NULL) && (activities_object->nodesetval != NULL)) {
				for (j = 0; j < activities_object->nodesetval->nodeNr; j++) {
					activity_node = activities_object->nodesetval->nodeTab[j];
					if ((activity_node->name != NULL) && (is_valid_activity_name((const char *)activity_node->name) == TRUE)) {
						LinphonePresenceActivityType acttype;
						description = (const char *)xmlNodeGetContent(activity_node);
						if ((description != NULL) && (description[0] == '\0')) {
							linphone_free_xml_text_content(description);
							description = NULL;
						}
						err = activity_name_to_presence_activity_type((const char *)activity_node->name, &acttype);
						if (err < 0) break;
						activity = linphone_presence_activity_new(acttype, description);
						linphone_presence_person_add_activity(person, activity);
						if (description != NULL) linphone_free_xml_text_content(description);
					}
				}
			}
			if (activities_object != NULL) xmlXPathFreeObject(activities_object);
			if (err < 0) break;
		}
	}
	if (activities_nodes_object != NULL) xmlXPathFreeObject(activities_nodes_object);

	return err;
}

static int process_pidf_xml_presence_person_notes(xmlparsing_context_t *xml_ctx, LinphonePresencePerson *person, unsigned int person_idx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr note_object;
	LinphonePresenceNote *note;
	const char *note_str;
	const char *lang;
	int i;

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities/rpid:note", person_prefix, person_idx);
	note_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities/rpid:note[%i]", person_prefix, person_idx, i);
			note_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities/rpid:note[%i]/@xml:lang", person_prefix, person_idx, i);
			lang = linphone_get_xml_text_content(xml_ctx, xpath_str);

			note = linphone_presence_note_new(note_str, lang);
			presence_person_add_activities_note(person, note);
			if (lang != NULL) linphone_free_xml_text_content(lang);
			linphone_free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/dm:note", person_prefix, person_idx);
	note_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/dm:note[%i]", person_prefix, person_idx, i);
			note_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/dm:note[%i]/@xml:lang", person_prefix, person_idx, i);
			lang = linphone_get_xml_text_content(xml_ctx, xpath_str);

			note = linphone_presence_note_new(note_str, lang);
			presence_person_add_note(person, note);
			if (lang != NULL) linphone_free_xml_text_content(lang);
			linphone_free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	return 0;
}

static int process_pidf_xml_presence_persons(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr person_object;
	LinphonePresencePerson *person;
	const char *person_id_str;
	const char *person_timestamp_str;
	time_t timestamp;
	int i;
	int err = 0;

	person_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, person_prefix);
	if ((person_object != NULL) && (person_object->nodesetval != NULL)) {
		for (i = 1; i <= person_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/@id", person_prefix, i);
			person_id_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:timestamp", person_prefix, i);
			person_timestamp_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (person_timestamp_str == NULL)
				timestamp = time(NULL);
			else
				timestamp = parse_timestamp(person_timestamp_str);
			person = presence_person_new(person_id_str, timestamp);
			if (person != NULL) {
				err = process_pidf_xml_presence_person_activities(xml_ctx, person, i);
				if (err == 0) {
					err = process_pidf_xml_presence_person_notes(xml_ctx, person, i);
				}
				if (err == 0) {
					presence_model_add_person(model, person);
				} else {
					linphone_presence_person_unref(person);
					break;
				}
			}
			if (person_id_str != NULL) linphone_free_xml_text_content(person_id_str);
			if (person_timestamp_str != NULL) linphone_free_xml_text_content(person_timestamp_str);
		}
	}
	if (person_object != NULL) xmlXPathFreeObject(person_object);

	if (err < 0) {
		/* Remove all the persons added since there was an error. */
		ms_list_for_each(model->persons, (MSIterateFunc)linphone_presence_person_unref);
	}
	return err;
}

static int process_pidf_xml_presence_notes(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr note_object;
	LinphonePresenceNote *note;
	const char *note_str;
	const char *lang;
	int i;

	note_object = linphone_get_xml_xpath_object_for_node_list(xml_ctx, "/pidf:presence/pidf:note");
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "/pidf:presence/pidf:note[%i]", i);
			note_str = linphone_get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "/pidf:presence/pidf:note[%i]/@xml:lang", i);
			lang = linphone_get_xml_text_content(xml_ctx, xpath_str);

			note = linphone_presence_note_new(note_str, lang);
			presence_model_add_note(model, note);
			if (lang != NULL) linphone_free_xml_text_content(lang);
			linphone_free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	return 0;
}

static LinphonePresenceModel * process_pidf_xml_presence_notification(xmlparsing_context_t *xml_ctx) {
	LinphonePresenceModel *model = NULL;
	int err;

	if (linphone_create_xml_xpath_context(xml_ctx) < 0)
		return NULL;

	model = linphone_presence_model_new();
	xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"pidf", (const xmlChar *)"urn:ietf:params:xml:ns:pidf");
	xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"dm", (const xmlChar *)"urn:ietf:params:xml:ns:pidf:data-model");
	xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar *)"rpid", (const xmlChar *)"urn:ietf:params:xml:ns:pidf:rpid");
	err = process_pidf_xml_presence_services(xml_ctx, model);
	if (err == 0) {
		err = process_pidf_xml_presence_persons(xml_ctx, model);
	}
	if (err == 0) {
		err = process_pidf_xml_presence_notes(xml_ctx, model);
	}

	if (err < 0) {
		linphone_presence_model_unref(model);
		model = NULL;
	}

	return model;
}




void linphone_core_add_subscriber(LinphoneCore *lc, const char *subscriber, SalOp *op){
	LinphoneFriend *fl=linphone_friend_new_with_address(subscriber);
	if (fl==NULL) return ;
	fl->insub=op;
	linphone_friend_set_inc_subscribe_policy(fl,LinphoneSPAccept);
	fl->inc_subscribe_pending=TRUE;
	lc->subscribers=ms_list_append(lc->subscribers,(void *)fl);
	{
		char *tmp=linphone_address_as_string(fl->uri);
		linphone_core_notify_new_subscription_requested(lc,fl,tmp);
		ms_free(tmp);
	}
}

void linphone_core_reject_subscriber(LinphoneCore *lc, LinphoneFriend *lf){
	linphone_friend_set_inc_subscribe_policy(lf,LinphoneSPDeny);
}

void linphone_core_notify_all_friends(LinphoneCore *lc, LinphonePresenceModel *presence){
	MSList *elem;
	LinphonePresenceActivity *activity = linphone_presence_model_get_activity(presence);
	char *activity_str = linphone_presence_activity_to_string(activity);
	ms_message("Notifying all friends that we are [%s]", activity_str);
	if (activity_str != NULL) ms_free(activity_str);
	for(elem=lc->friends;elem!=NULL;elem=elem->next){
		LinphoneFriend *lf=(LinphoneFriend *)elem->data;
		if (lf->insub){
			linphone_friend_notify(lf,presence);
		}
	}
}

void linphone_subscription_new(LinphoneCore *lc, SalOp *op, const char *from){
	LinphoneFriend *lf=NULL;
	char *tmp;
	LinphoneAddress *uri;
	LinphoneProxyConfig *cfg;

	uri=linphone_address_new(from);
	linphone_address_clean(uri);
	tmp=linphone_address_as_string(uri);
	ms_message("Receiving new subscription from %s.",from);

	cfg=linphone_core_lookup_known_proxy(lc,uri);
	if (cfg!=NULL){
		if (cfg->op){
			if (sal_op_get_contact_address(cfg->op)) {
				sal_op_set_contact_address (op,sal_op_get_contact_address(cfg->op));
				ms_message("Contact for next subscribe answer has been fixed using proxy "/*to %s",fixed_contact*/);
			}
		}
	}

	/* check if we answer to this subscription */
	if (linphone_find_friend_by_address(lc->friends,uri,&lf)!=NULL){
		lf->insub=op;
		lf->inc_subscribe_pending=TRUE;
		sal_subscribe_accept(op);
		linphone_friend_done(lf);	/*this will do all necessary actions */
	}else{
		/* check if this subscriber is in our black list */
		if (linphone_find_friend_by_address(lc->subscribers,uri,&lf)){
			if (lf->pol==LinphoneSPDeny){
				ms_message("Rejecting %s because we already rejected it once.",from);
				sal_subscribe_decline(op,SalReasonDeclined);
			}
			else {
				/* else it is in wait for approval state, because otherwise it is in the friend list.*/
				ms_message("New subscriber found in friend list, in %s state.",__policy_enum_to_str(lf->pol));
			}
		}else {
			sal_subscribe_accept(op);
			linphone_core_add_subscriber(lc,tmp,op);
		}
	}
	linphone_address_destroy(uri);
	ms_free(tmp);
}

void linphone_notify_parse_presence(SalOp *op, const char *content_type, const char *content_subtype, const char *body, SalPresenceModel **result) {
	xmlparsing_context_t *xml_ctx;
	LinphonePresenceModel *model = NULL;

	if (strcmp(content_type, "application") != 0) {
		*result = NULL;
		return;
	}

	if (strcmp(content_subtype, "pidf+xml") == 0) {
		xml_ctx = linphone_xmlparsing_context_new();
		xmlSetGenericErrorFunc(xml_ctx, linphone_xmlparsing_genericxml_error);
		xml_ctx->doc = xmlReadDoc((const unsigned char*)body, 0, NULL, 0);
		if (xml_ctx->doc != NULL) {
			model = process_pidf_xml_presence_notification(xml_ctx);
		} else {
			ms_warning("Wrongly formatted presence XML: %s", xml_ctx->errorBuffer);
		}
		linphone_xmlparsing_context_destroy(xml_ctx);
	} else {
		ms_error("Unknown content type '%s/%s' for presence", content_type, content_subtype);
	}

	/* If no activities are present in the model, add a dummy activity so that linphone_presence_activity_get_type() returns
	 * the expected result. */
	if (model != NULL) {
		LinphonePresenceActivity *activity = linphone_presence_model_get_activity(model);
		if (activity == NULL) {
			LinphonePresenceBasicStatus basic_status = linphone_presence_model_get_basic_status(model);
			LinphonePresenceActivityType acttype;
			switch (basic_status) {
				case LinphonePresenceBasicStatusOpen:
					acttype = LinphonePresenceActivityOnline;
					break;
				case LinphonePresenceBasicStatusClosed:
				default:
					acttype = LinphonePresenceActivityOffline;
					break;
			}
			activity = linphone_presence_activity_new(acttype, NULL);
			linphone_presence_model_add_activity(model, activity);
		}
	}

	*result = (SalPresenceModel *)model;
}

struct _presence_service_obj_st {
	xmlTextWriterPtr writer;
	const char *contact;
	int *err;
};

struct _presence_person_obj_st {
	xmlTextWriterPtr writer;
	int *err;
};

struct _presence_activity_obj_st {
	xmlTextWriterPtr writer;
	int *err;
};

struct _presence_note_obj_st {
	xmlTextWriterPtr writer;
	const char *ns;
	int *err;
};

static int write_xml_presence_note(xmlTextWriterPtr writer, LinphonePresenceNote *note, const char *ns) {
	int err;
	if (ns == NULL) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"note");
	} else {
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)ns, (const xmlChar *)"note", NULL);
	}
	if ((err >= 0) && (note->lang != NULL)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xml", (const xmlChar *)"lang", NULL, (const xmlChar *)note->lang);
	}
	if (err >= 0) {
		err = xmlTextWriterWriteString(writer, (const xmlChar *)note->content);
	}
	if (err >= 0) {
		err = xmlTextWriterEndElement(writer);
	}
	return err;
}

static void write_xml_presence_note_obj(LinphonePresenceNote *note, struct _presence_note_obj_st *st) {
	int err = write_xml_presence_note(st->writer, note, st->ns);
	if (err < 0) *st->err = err;
}

static int write_xml_presence_timestamp(xmlTextWriterPtr writer, time_t timestamp) {
	int err;
	char *timestamp_str = linphone_timestamp_to_rfc3339_string(timestamp);
	err = xmlTextWriterWriteElement(writer, (const xmlChar *)"timestamp", (const xmlChar *)timestamp_str);
	if (timestamp_str) ms_free(timestamp_str);
	return err;
}

static int write_xml_presence_service(xmlTextWriterPtr writer, LinphonePresenceService *service, const char *contact) {
	int err = xmlTextWriterStartElement(writer, (const xmlChar *)"tuple");
	if (err >= 0) {
		if ((service == NULL) || (service->id == NULL)) {
			char *text = generate_presence_id();
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"id", (const xmlChar *)text);
			if (text != NULL) ms_free(text);
		} else {
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"id", (const xmlChar *)service->id);
		}
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"status");
	}
	if (err >= 0) {
		LinphonePresenceBasicStatus basic_status = LinphonePresenceBasicStatusClosed;
		if (service != NULL) basic_status = service->status;
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"basic", (const xmlChar *)presence_basic_status_to_string(basic_status));
	}
	if (err >= 0) {
		/* Close the "status" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"contact");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"priority", (const xmlChar *)"0.8");
	}
	if (err >= 0) {
		const char *contact_str;
		if ((service == NULL) || (service->contact == NULL))
			contact_str = contact;
		else
			contact_str = service->contact;
		err = xmlTextWriterWriteString(writer, (const xmlChar *)contact_str);
	}
	if (err >= 0) {
		/* Close the "contact" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if ((err >= 0) && (service != NULL) && (service->notes != NULL)) {
		struct _presence_note_obj_st st;
		st.writer = writer;
		st.ns = NULL;
		st.err = &err;
		ms_list_for_each2(service->notes, (MSIterate2Func)write_xml_presence_note_obj, &st);
	}
	if (err >= 0) {
		if (service == NULL)
			err = write_xml_presence_timestamp(writer, time(NULL));
		else
			err = write_xml_presence_timestamp(writer, service->timestamp);
	}
	if (err >= 0) {
		/* Close the "tuple" element. */
		err = xmlTextWriterEndElement(writer);
	}
	return err;
}

static bool_t is_valid_activity(LinphonePresenceActivity *activity) {
	if ((activity->type == LinphonePresenceActivityOffline) || (activity->type == LinphonePresenceActivityOnline))
		return FALSE;
	return TRUE;
}

static int write_xml_presence_activity(xmlTextWriterPtr writer, LinphonePresenceActivity *activity) {
	int err;

	if (is_valid_activity(activity) == FALSE) return 0;

	err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"rpid",
					      (const xmlChar *)presence_activity_type_to_string(activity->type), NULL);
	if ((err >= 0) && (activity->description != NULL)) {
		err = xmlTextWriterWriteString(writer, (const xmlChar *)activity->description);
	}
	if (err >= 0) {
		err = xmlTextWriterEndElement(writer);
	}
	return err;
}

static void write_xml_presence_activity_obj(LinphonePresenceActivity *activity, struct _presence_activity_obj_st *st) {
	int err = write_xml_presence_activity(st->writer, activity);
	if (err < 0) *st->err = err;
}

static void person_has_valid_activity(LinphonePresenceActivity *activity, bool_t *has_valid_activities) {
	if (is_valid_activity(activity) == TRUE) *has_valid_activities = TRUE;
}

static bool_t person_has_valid_activities(LinphonePresencePerson *person) {
	bool_t has_valid_activities = FALSE;
	ms_list_for_each2(person->activities, (MSIterate2Func)person_has_valid_activity, &has_valid_activities);
	return has_valid_activities;
}

static int write_xml_presence_person(xmlTextWriterPtr writer, LinphonePresencePerson *person) {
	int err;

	if ((person_has_valid_activities(person) == FALSE) && (person->notes == NULL)) return 0;

	err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"dm", (const xmlChar *)"person", NULL);
	if (err >= 0) {
		if (person->id == NULL) {
			char *text = generate_presence_id();
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"id", (const xmlChar *)text);
			if (text != NULL) ms_free(text);
		} else {
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"id", (const xmlChar *)person->id);
		}
	}
	if ((err >= 0) && ((person->activities_notes != NULL) || (person_has_valid_activities(person) == TRUE))) {
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"rpid", (const xmlChar *)"activities", NULL);
		if ((err >= 0) && (person->activities_notes != NULL)) {
			struct _presence_note_obj_st st;
			st.writer = writer;
			st.ns = "rpid";
			st.err = &err;
			ms_list_for_each2(person->activities_notes, (MSIterate2Func)write_xml_presence_note_obj, &st);
		}
		if ((err >= 0) && (person->activities != NULL)) {
			struct _presence_activity_obj_st st;
			st.writer = writer;
			st.err = &err;
			ms_list_for_each2(person->activities, (MSIterate2Func)write_xml_presence_activity_obj, &st);
		}
		if (err >= 0) {
			/* Close the "activities" element. */
			err = xmlTextWriterEndElement(writer);
		}
	}
	if ((err >= 0) && (person->notes != NULL)) {
		struct _presence_note_obj_st st;
		st.writer = writer;
		st.ns = "dm";
		st.err = &err;
		ms_list_for_each2(person->notes, (MSIterate2Func)write_xml_presence_note_obj, &st);
	}
	if (err >= 0) {
		write_xml_presence_timestamp(writer, person->timestamp);
	}
	if (err >= 0) {
		/* Close the "person" element. */
		err = xmlTextWriterEndElement(writer);
	}
	return err;
}

static void write_xml_presence_service_obj(LinphonePresenceService *service, struct _presence_service_obj_st *st) {
	int err = write_xml_presence_service(st->writer, service, st->contact);
	if (err < 0) *st->err = err;
}

static void write_xml_presence_person_obj(LinphonePresencePerson *person, struct _presence_person_obj_st *st) {
	int err = write_xml_presence_person(st->writer, person);
	if (err < 0) *st->err = err;
}

void linphone_notify_convert_presence_to_xml(SalOp *op, SalPresenceModel *presence, const char *contact, char **content) {
	LinphonePresenceModel *model;
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;

	if ((contact == NULL) || (content == NULL)) return;

	model = (LinphonePresenceModel *)presence;
	buf = xmlBufferCreate();
	if (buf == NULL) {
		ms_error("Error creating the XML buffer");
		return;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ms_error("Error creating the XML writer");
		return;
	}

	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", NULL);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, NULL, (const xmlChar *)"presence", (const xmlChar *)"urn:ietf:params:xml:ns:pidf");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"dm",
						    NULL, (const xmlChar *)"urn:ietf:params:xml:ns:pidf:data-model");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"rpid",
						    NULL, (const xmlChar *)"urn:ietf:params:xml:ns:pidf:rpid");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"entity", (const xmlChar *)contact);
	}
	if (err >= 0) {
		if ((model == NULL) || (model->services == NULL)) {
			err = write_xml_presence_service(writer, NULL, contact);
		} else {
			struct _presence_service_obj_st st={0};
			st.writer = writer;
			st.contact = contact;
			st.err = &err;
			ms_list_for_each2(model->services, (MSIterate2Func)write_xml_presence_service_obj, &st);
		}
	}
	if ((err >= 0) && (model != NULL)) {
		struct _presence_person_obj_st st={0};
		st.writer = writer;
		st.err = &err;
		ms_list_for_each2(model->persons, (MSIterate2Func)write_xml_presence_person_obj, &st);
	}
	if ((err >= 0) && (model != NULL)) {
		struct _presence_note_obj_st st={0};
		st.writer = writer;
		st.ns = NULL;
		st.err = &err;
		ms_list_for_each2(model->notes, (MSIterate2Func)write_xml_presence_note_obj, &st);
	}
	if (err >= 0) {
		/* Close the "presence" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		*content = ms_strdup((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
}

void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model){
	char *tmp;
	LinphoneFriend *lf;
	LinphoneAddress *friend=NULL;
	LinphonePresenceModel *presence = model ? (LinphonePresenceModel *)model:linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline, NULL);

	lf=linphone_find_friend_by_out_subscribe(lc->friends,op);
	if (lf==NULL && lp_config_get_int(lc->config,"sip","allow_out_of_subscribe_presence",0)){
		const SalAddress *addr=sal_op_get_from_address(op);
		lf=NULL;
		linphone_find_friend_by_address(lc->friends,(LinphoneAddress*)addr,&lf);
	}
	if (lf!=NULL){
		LinphonePresenceActivity *activity = NULL;
		char *activity_str;
		friend=lf->uri;
		tmp=linphone_address_as_string(friend);
		activity = linphone_presence_model_get_activity(presence);
		activity_str = linphone_presence_activity_to_string(activity);
		ms_message("We are notified that [%s] has presence [%s]", tmp, activity_str);
		if (activity_str != NULL) ms_free(activity_str);
		if (lf->presence != NULL) {
			linphone_presence_model_unref(lf->presence);
		}
		lf->presence = presence;
		lf->subscribe_active=TRUE;
		linphone_core_notify_notify_presence_received(lc,(LinphoneFriend*)lf);
		ms_free(tmp);
	}else{
		ms_message("But this person is not part of our friend list, so we don't care.");
		linphone_presence_model_unref(presence);
	}
	if (ss==SalSubscribeTerminated){
		sal_op_release(op);
		if (lf){
			lf->outsub=NULL;
			lf->subscribe_active=FALSE;
		}
	}
}

void linphone_subscription_closed(LinphoneCore *lc, SalOp *op){
	LinphoneFriend *lf;
	lf=linphone_find_friend_by_inc_subscribe(lc->friends,op);
	sal_op_release(op);
	if (lf!=NULL){
		lf->insub=NULL;
	}else{
		ms_warning("Receiving unsuscribe for unknown in-subscribtion from %s", sal_op_get_from(op));
	}
}

LinphonePresenceActivity * linphone_core_create_presence_activity(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description) {
	return linphone_presence_activity_new(acttype, description);
}

LinphonePresenceModel * linphone_core_create_presence_model(LinphoneCore *lc) {
	return linphone_presence_model_new();
}

LinphonePresenceModel * linphone_core_create_presence_model_with_activity(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description) {
	return linphone_presence_model_new_with_activity(acttype, description);
}

LinphonePresenceModel * linphone_core_create_presence_model_with_activity_and_note(LinphoneCore *lc, LinphonePresenceActivityType acttype, const char *description, const char *note, const char *lang) {
	return linphone_presence_model_new_with_activity_and_note(acttype, description, note, lang);
}

LinphonePresenceNote * linphone_core_create_presence_note(LinphoneCore *lc, const char *content, const char *lang) {
	return linphone_presence_note_new(content, lang);
}

LinphonePresencePerson * linphone_core_create_presence_person(LinphoneCore *lc, const char *id) {
	return linphone_presence_person_new(id);
}

LinphonePresenceService * linphone_core_create_presence_service(LinphoneCore *lc, const char *id, LinphonePresenceBasicStatus basic_status, const char *contact) {
	return linphone_presence_service_new(id, basic_status, contact);
}
