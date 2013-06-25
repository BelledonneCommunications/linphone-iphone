/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)

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

#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


#define XMLPARSING_BUFFER_LEN	2048
#define MAX_XPATH_LENGTH	256



extern const char *__policy_enum_to_str(LinphoneSubscribePolicy pol);



struct _LinphonePresenceNote {
	void *user_data;
	int refcnt;
	char *lang;
	char *content;
};

struct _LinphonePresenceService {
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

typedef struct _xmlparsing_context {
	xmlDoc *doc;
	xmlXPathContextPtr xpath_ctx;
	char errorBuffer[XMLPARSING_BUFFER_LEN];
	char warningBuffer[XMLPARSING_BUFFER_LEN];
} xmlparsing_context_t;



static xmlparsing_context_t * xmlparsing_context_new() {
	xmlparsing_context_t *xmlCtx = (xmlparsing_context_t *)malloc(sizeof(xmlparsing_context_t));
	if (xmlCtx != NULL) {
		xmlCtx->doc = NULL;
		xmlCtx->xpath_ctx = NULL;
		xmlCtx->errorBuffer[0] = '\0';
		xmlCtx->warningBuffer[0] = '\0';
	}
	return xmlCtx;
}

static void xmlparsing_context_destroy(xmlparsing_context_t *ctx) {
	if (ctx->doc != NULL) {
		xmlFreeDoc(ctx->doc);
		ctx->doc = NULL;
	}
	if (ctx->xpath_ctx != NULL) {
		xmlXPathFreeContext(ctx->xpath_ctx);
		ctx->xpath_ctx = NULL;
	}
	free(ctx);
}

static void xmlparsing_genericxml_error(void *ctx, const char *fmt, ...) {
	xmlparsing_context_t *xmlCtx = (xmlparsing_context_t *)ctx;
	int sl = strlen(xmlCtx->errorBuffer);
	va_list args;
	va_start(args, fmt);
	vsnprintf(xmlCtx->errorBuffer + sl, XMLPARSING_BUFFER_LEN - sl, fmt, args);
	va_end(args);
}

static char presence_id_valid_characters[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static char * generate_presence_id(void) {
	char id[7];
	int i;

	for (i = 0; i < 6; i++) {
		id[i] = presence_id_valid_characters[random() % sizeof(presence_id_valid_characters)];
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

static struct _LinphonePresenceNote * presence_note_new(const char *content, const char *lang) {
	struct _LinphonePresenceNote * note = ms_new0(struct _LinphonePresenceNote, 1);
	note->refcnt = 1;
	note->content = ms_strdup(content);
	if (lang != NULL) {
		note->lang = ms_strdup(lang);
	}
	return note;
}

static void presence_note_delete(struct _LinphonePresenceNote *note) {
	ms_free(note->content);
	if (note->lang != NULL) {
		ms_free(note->lang);
	}
	ms_free(note);
}

static void presence_note_set_content(struct _LinphonePresenceNote *note, const char *content) {
	if (note->content != NULL) {
		ms_free(note->content);
	}
	note->content = ms_strdup(content);
}

static struct _LinphonePresenceService * presence_service_new(const char *id, LinphonePresenceBasicStatus status) {
	struct _LinphonePresenceService *service = ms_new0(struct _LinphonePresenceService, 1);
	if (id != NULL) {
		service->id = ms_strdup(id);
	}
	service->status = status;
	service->timestamp = time(NULL);
	return service;
}

static void presence_service_delete(struct _LinphonePresenceService *service) {
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

static void presence_service_set_timestamp(struct _LinphonePresenceService *service, time_t timestamp) {
	service->timestamp = timestamp;
}

static void presence_service_set_contact(struct _LinphonePresenceService *service, const char *contact) {
	service->contact = ms_strdup(contact);
}

static void presence_service_add_note(struct _LinphonePresenceService *service, struct _LinphonePresenceNote *note) {
	service->notes = ms_list_append(service->notes, note);
}

static struct _LinphonePresenceActivity * presence_activity_new(LinphonePresenceActivityType acttype, const char *description) {
	struct _LinphonePresenceActivity *act = ms_new0(struct _LinphonePresenceActivity, 1);
	act->refcnt = 1;
	act->type = acttype;
	if (description != NULL) {
		act->description = ms_strdup(description);
	}
	return act;
}

static void presence_activity_delete(struct _LinphonePresenceActivity *activity) {
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

static char * timestamp_to_string(time_t timestamp) {
	char timestamp_str[22];
	struct tm *ret;
#ifndef WIN32
	struct tm gmt;
	ret = gmtime_r(&timestamp,&gmt);
#else
	ret = gmtime(&curtime);
#endif
	snprintf(timestamp_str, sizeof(timestamp_str), "%4d-%02d-%02dT%02d:%02d:%02dZ",
		 ret->tm_year + 1900, ret->tm_mon + 1, ret->tm_mday, ret->tm_hour, ret->tm_min, ret->tm_sec);
	return ms_strdup(timestamp_str);
}

static struct _LinphonePresencePerson * presence_person_new(const char *id,  time_t timestamp) {
	struct _LinphonePresencePerson *person = ms_new0(struct _LinphonePresencePerson, 1);
	if (id != NULL) {
		person->id = ms_strdup(id);
	}
	if (person->timestamp == ((time_t)-1))
		person->timestamp = time(NULL);
	else
		person->timestamp = timestamp;
	return person;
}

static void presence_person_delete(struct _LinphonePresencePerson *person) {
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

static void presence_person_add_activity(struct _LinphonePresencePerson *person, struct _LinphonePresenceActivity *activity) {
	person->activities = ms_list_append(person->activities, activity);
}

static void presence_person_add_activities_note(struct _LinphonePresencePerson *person, struct _LinphonePresenceNote *note) {
	person->activities_notes = ms_list_append(person->activities_notes, note);
}

static void presence_person_add_note(struct _LinphonePresencePerson *person, struct _LinphonePresenceNote *note) {
	person->notes = ms_list_append(person->notes, note);
}

static void presence_person_clear_activities(struct _LinphonePresencePerson *person) {
	ms_list_for_each(person->activities, (MSIterateFunc)linphone_presence_activity_unref);
	ms_list_free(person->activities);
	person->activities = NULL;
}

static void presence_model_add_service(LinphonePresenceModel *model, struct _LinphonePresenceService *service) {
	model->services = ms_list_append(model->services, service);
}

static void presence_model_add_person(LinphonePresenceModel *model, struct _LinphonePresencePerson *person) {
	model->persons = ms_list_append(model->persons, person);
}

static void presence_model_add_note(LinphonePresenceModel *model, struct _LinphonePresenceNote *note) {
	model->notes = ms_list_append(model->notes, note);
}

static int presence_model_set_basic_status(LinphonePresenceModel *model, LinphonePresenceBasicStatus basic_status) {
	struct _LinphonePresenceService *service;
	char *id;
	if (ms_list_size(model->services) > 0) {
		ms_list_for_each(model->services, (MSIterateFunc)presence_service_delete);
		ms_list_free(model->services);
		model->services = NULL;
	}
	id = generate_presence_id();
	service = presence_service_new(id, basic_status);
	ms_free(id);
	if (service == NULL) return -1;
	presence_model_add_service(model, service);
	return 0;
}

static void presence_model_clear_activities(LinphonePresenceModel *model) {
	ms_list_for_each(model->persons, (MSIterateFunc)presence_person_clear_activities);
}

static int presence_model_add_activity(LinphonePresenceModel *model, LinphonePresenceActivityType acttype, const char *description) {
	char *id = NULL;
	struct _LinphonePresencePerson *person = NULL;
	struct _LinphonePresenceActivity *act = NULL;

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
		person = (struct _LinphonePresencePerson *)ms_list_nth_data(model->persons, 0);
	}
	act = presence_activity_new(acttype, description);
	if (act == NULL)
		return -1;
	presence_person_add_activity(person, act);

	return 0;
}

static void presence_model_find_open_basic_status(struct _LinphonePresenceService *service, LinphonePresenceBasicStatus *status) {
	if (service->status == LinphonePresenceBasicStatusOpen) {
		*status = LinphonePresenceBasicStatusOpen;
	}
}

static bool_t presence_service_equals(const struct _LinphonePresenceService *s1, const struct _LinphonePresenceService *s2) {
	if (s1->status != s2->status)
		return FALSE;
	return TRUE;
}

static bool_t presence_note_equals(const struct _LinphonePresenceNote *n1, const struct _LinphonePresenceNote *n2) {
	if (((n1->lang == NULL) && (n2->lang != NULL))
		|| ((n1->lang != NULL) && (n2->lang == NULL)))
		return FALSE;

	if (strcmp(n1->content, n2->content) != 0)
		return FALSE;
	if ((n1->lang != NULL) && (n2->lang != NULL)) {
		if (strcmp(n1->lang, n2->lang) != 0)
			return FALSE;
	}

	return TRUE;
}

static bool_t presence_activity_equals(const struct _LinphonePresenceActivity *a1, const struct _LinphonePresenceActivity *a2) {
	if (((a1->description == NULL) && (a2->description != NULL))
		|| ((a1->description != NULL) && (a2->description == NULL)))
		return FALSE;

	if (a1->type != a2->type)
		return FALSE;

	if ((a1->description != NULL) && (a2->description != NULL)) {
		if (strcmp(a1->description, a2->description) != 0)
			return FALSE;
	}

	return TRUE;
}

static bool_t presence_person_equals(const struct _LinphonePresencePerson *p1, const struct _LinphonePresencePerson *p2) {
	int nb;
	int i;

	if ((ms_list_size(p1->activities) != ms_list_size(p2->activities))
		|| (ms_list_size(p1->activities_notes) != ms_list_size(p2->activities_notes))
		|| (ms_list_size(p1->notes) != ms_list_size(p2->notes)))
		return FALSE;

	nb = ms_list_size(p1->activities);
	for (i = 0; i < nb; i++) {
		if (presence_activity_equals(ms_list_nth_data(p1->activities, i), ms_list_nth_data(p2->activities, i)) == FALSE)
			return FALSE;
	}

	nb = ms_list_size(p1->activities_notes);
	for (i = 0; i < nb; i++) {
		if (presence_note_equals(ms_list_nth_data(p1->activities_notes, i), ms_list_nth_data(p2->activities_notes, i)) == FALSE)
			return FALSE;
	}

	nb = ms_list_size(p1->notes);
	for (i = 0; i < nb; i++) {
		if (presence_note_equals(ms_list_nth_data(p1->notes, i), ms_list_nth_data(p2->notes, i)) == FALSE)
			return FALSE;
	}

	return TRUE;
}

LinphonePresenceModel * linphone_presence_model_new(void) {
	LinphonePresenceModel *model = ms_new0(LinphonePresenceModel, 1);
	model->refcnt = 1;
	return model;
}

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

static void presence_model_delete(LinphonePresenceModel *model) {
	if (model == NULL) return;

	ms_list_for_each(model->services, (MSIterateFunc)presence_service_delete);
	ms_list_free(model->services);
	ms_list_for_each(model->persons, (MSIterateFunc)presence_person_delete);
	ms_list_free(model->persons);
	ms_list_for_each(model->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(model->notes);
	ms_free(model);
}

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

void * linphone_presence_model_get_user_data(LinphonePresenceModel *model) {
	return model->user_data;
}

bool_t linphone_presence_model_equals(const LinphonePresenceModel *m1, const LinphonePresenceModel *m2) {
	LinphonePresenceActivity *activity = NULL;
	int nb;
	int i;

	/* If the two pointers are the same, the presence model are equals. */
	if (m1 == m2)
		return TRUE;

	/* A null activity is equal to an activity with no activity but a basic status of Closed. */
	if (m1 == NULL) {
		activity = linphone_presence_model_get_activity(m2);
		if (linphone_presence_activity_get_type(activity) != LinphonePresenceActivityOffline)
			return FALSE;
		return TRUE;
	}
	if (m2 == NULL) {
		activity = linphone_presence_model_get_activity(m2);
		if (linphone_presence_activity_get_type(activity) != LinphonePresenceActivityOffline)
			return FALSE;
		return TRUE;
	}

	if ((ms_list_size(m1->services) != ms_list_size(m2->services))
		|| (ms_list_size(m1->persons) != ms_list_size(m2->persons))
		|| (ms_list_size(m1->notes) != ms_list_size(m2->notes)))
		return FALSE;

	nb = ms_list_size(m1->services);
	for (i = 0; i < nb; i++) {
		if (presence_service_equals(ms_list_nth_data(m1->services, i), ms_list_nth_data(m2->services, i)) == FALSE)
			return FALSE;
	}

	nb = ms_list_size(m1->persons);
	for (i = 0; i < nb; i++) {
		if (presence_person_equals(ms_list_nth_data(m1->persons, i), ms_list_nth_data(m2->persons, i)) == FALSE)
			return FALSE;
	}

	nb = ms_list_size(m1->notes);
	for (i = 0; i < nb; i++) {
		if (presence_note_equals(ms_list_nth_data(m1->notes, i), ms_list_nth_data(m2->notes, i)) == FALSE)
			return FALSE;
	}

	return TRUE;
}

/* Suppose that if at least one service is open, then the model is open. */
LinphonePresenceBasicStatus linphone_presence_model_get_basic_status(const LinphonePresenceModel *model) {
	LinphonePresenceBasicStatus status = LinphonePresenceBasicStatusClosed;
	if (model != NULL) {
		ms_list_for_each2(model->services, (MSIterate2Func)presence_model_find_open_basic_status, &status);
	}
	return status;
}

static void presence_model_find_contact(struct _LinphonePresenceService *service, char **contact) {
	if ((service->contact != NULL) && (*contact == NULL))
		*contact = service->contact;
}

char * linphone_presence_model_get_contact(const LinphonePresenceModel *model) {
	char *contact = NULL;
	ms_list_for_each2(model->services, (MSIterate2Func)presence_model_find_contact, &contact);
	if (contact == NULL) return NULL;
	return ms_strdup(contact);
}

void linphone_presence_model_set_contact(LinphonePresenceModel *model, const char *contact) {
	struct _LinphonePresenceService *service;
	if (model != NULL) {
		service = (struct _LinphonePresenceService *)ms_list_nth_data(model->services, 0);
		if (service != NULL) {
			if (service->contact != NULL)
				ms_free(service->contact);
			if (contact != NULL)
				service->contact = ms_strdup(contact);
			else
				service->contact = NULL;
		}
	}
}

static void presence_model_count_activities(const struct _LinphonePresencePerson *person, unsigned int *nb) {
	*nb += ms_list_size(person->activities);
}

unsigned int linphone_presence_model_nb_activities(const LinphonePresenceModel *model) {
	unsigned int nb = 0;
	ms_list_for_each2(model->persons, (MSIterate2Func)presence_model_count_activities, &nb);
	return nb;
}

struct _get_activity_st {
	unsigned int requested_idx;
	unsigned int current_idx;
	struct _LinphonePresenceActivity *activity;
};

static void presence_model_get_activity(const struct _LinphonePresencePerson *person, struct _get_activity_st *st) {
	unsigned int size = ms_list_size(person->activities);
	if (st->requested_idx < (st->current_idx + size)) {
		st->activity = (struct _LinphonePresenceActivity *)ms_list_nth_data(person->activities, st->requested_idx - st->current_idx);
	} else {
		st->current_idx += size;
	}
}

LinphonePresenceActivity * linphone_presence_model_get_nth_activity(const LinphonePresenceModel *model, unsigned int idx) {
	struct _get_activity_st st;

	if ((model == NULL) || (idx >= linphone_presence_model_nb_activities(model)))
		return NULL;

	memset(&st, 0, sizeof(st));
	st.requested_idx = idx;
	ms_list_for_each2(model->persons, (MSIterate2Func)presence_model_get_activity, &st);

	return st.activity;
}

LinphonePresenceActivity * linphone_presence_model_get_activity(const LinphonePresenceModel *model) {
	return linphone_presence_model_get_nth_activity(model, 0);
}

int linphone_presence_model_set_activity(LinphonePresenceModel *model, LinphonePresenceActivityType acttype, const char *description) {
	LinphonePresenceBasicStatus basic_status = LinphonePresenceBasicStatusOpen;

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
	if (presence_model_set_basic_status(model, basic_status) < 0)
		return -1;
	presence_model_clear_activities(model);
	if (presence_model_add_activity(model, acttype, description) < 0)
		return -1;

	return 0;
}

struct _find_note_st {
	const char *lang;
	struct _LinphonePresenceNote *note;
};

static struct _LinphonePresenceNote * find_presence_note_in_list(MSList *list, const char *lang) {
	int nb;
	int i;

	nb = ms_list_size(list);
	for (i = 0; i < nb; i++) {
		struct _LinphonePresenceNote *note = (struct _LinphonePresenceNote *)ms_list_nth_data(list, i);
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

static void find_presence_person_note(struct _LinphonePresencePerson *person, struct _find_note_st *st) {
	/* First look for the note in the activities notes... */
	st->note = find_presence_note_in_list(person->activities_notes, st->lang);
	if (st->note != NULL) return;

	/* ... then look in the person notes. */
	st->note = find_presence_note_in_list(person->notes, st->lang);
}

static void find_presence_service_note(struct _LinphonePresenceService *service, struct _find_note_st *st) {
	st->note = find_presence_note_in_list(service->notes, st->lang);
}

static struct _LinphonePresenceNote * get_first_presence_note_in_list(MSList *list) {
	return (struct _LinphonePresenceNote *)ms_list_nth_data(list, 0);
}

static void get_first_presence_person_note(struct _LinphonePresencePerson *person, struct _find_note_st *st) {
	st->note = get_first_presence_note_in_list(person->activities_notes);
	if (st->note != NULL) return;
	st->note = get_first_presence_note_in_list(person->notes);
}

static void get_first_presence_service_note(struct _LinphonePresenceService *service, struct _find_note_st *st) {
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
	struct _LinphonePresenceService *service;
	struct _LinphonePresenceNote *note;

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
		note = presence_note_new(note_content, lang);
	} else {
		presence_note_set_content(note, note_content);
	}
	if (note == NULL)
		return -1;

	presence_service_add_note(service, note);

	return 0;
}

static void clear_presence_person_notes(struct _LinphonePresencePerson *person) {
	ms_list_for_each(person->activities_notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->activities_notes);
	person->activities_notes = NULL;
	ms_list_for_each(person->notes, (MSIterateFunc)linphone_presence_note_unref);
	ms_list_free(person->notes);
	person->notes = NULL;
}

static void clear_presence_service_notes(struct _LinphonePresenceService *service) {
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

static int create_xml_xpath_context(xmlparsing_context_t *xml_ctx) {
	if (xml_ctx->xpath_ctx != NULL) {
		xmlXPathFreeContext(xml_ctx->xpath_ctx);
	}
	xml_ctx->xpath_ctx = xmlXPathNewContext(xml_ctx->doc);
	if (xml_ctx->xpath_ctx == NULL) return -1;
	return 0;
}

static char * get_xml_text_content(xmlparsing_context_t *xml_ctx, const char *xpath_expression) {
	xmlXPathObjectPtr xpath_obj;
	xmlChar *text = NULL;
	int i;

	xpath_obj = xmlXPathEvalExpression((const xmlChar *)xpath_expression, xml_ctx->xpath_ctx);
	if (xpath_obj != NULL) {
		if (xpath_obj->nodesetval != NULL) {
			xmlNodeSetPtr nodes = xpath_obj->nodesetval;
			for (i = 0; i < nodes->nodeNr; i++) {
				xmlNodePtr node = nodes->nodeTab[i];
				if (node->children != NULL) {
					text = xmlNodeListGetString(xml_ctx->doc, node->children, 1);
				}
			}
		}
		xmlXPathFreeObject(xpath_obj);
	}

	return (char *)text;
}

static void free_xml_text_content(const char *text) {
	xmlFree((xmlChar *)text);
}

static xmlXPathObjectPtr get_xml_xpath_object_for_node_list(xmlparsing_context_t *xml_ctx, const char *xpath_expression) {
	return xmlXPathEvalExpression((const xmlChar *)xpath_expression, xml_ctx->xpath_ctx);
}

static const char *service_prefix = "/pidf:presence/pidf:tuple";

static int process_pidf_xml_presence_service_notes(xmlparsing_context_t *xml_ctx, struct _LinphonePresenceService *service, unsigned int service_idx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr note_object;
	struct _LinphonePresenceNote *note;
	const char *note_str;
	const char *lang;
	int i;

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:note", service_prefix, service_idx);
	note_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:note[%i]", service_prefix, service_idx, i);
			note_str = get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:note[%i]/@xml:lang", service_prefix, service_idx, i);
			lang = get_xml_text_content(xml_ctx, xpath_str);

			note = presence_note_new(note_str, lang);
			presence_service_add_note(service, note);
			if (lang != NULL) free_xml_text_content(lang);
			free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	return 0;
}

static int process_pidf_xml_presence_services(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr service_object;
	struct _LinphonePresenceService *service;
	const char *basic_status_str;
	const char *service_id_str;
	const char *timestamp_str;
	const char *contact_str;
	LinphonePresenceBasicStatus basic_status;
	int i;

	service_object = get_xml_xpath_object_for_node_list(xml_ctx, service_prefix);
	if ((service_object != NULL) && (service_object->nodesetval != NULL)) {
		for (i = 1; i <= service_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:status/pidf:basic", service_prefix, i);
			basic_status_str = get_xml_text_content(xml_ctx, xpath_str);
			if (basic_status_str == NULL)
				continue;

			if (strcmp(basic_status_str, "open") == 0) {
				basic_status = LinphonePresenceBasicStatusOpen;
			} else if (strcmp(basic_status_str, "closed") == 0) {
				basic_status = LinphonePresenceBasicStatusClosed;
			} else {
				/* Invalid value for basic status. */
				free_xml_text_content(basic_status_str);
				return -1;
			}

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:timestamp", service_prefix, i);
			timestamp_str = get_xml_text_content(xml_ctx, xpath_str);

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:contact", service_prefix, i);
			contact_str = get_xml_text_content(xml_ctx, xpath_str);

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/@id", service_prefix, i);
			service_id_str = get_xml_text_content(xml_ctx, xpath_str);
			service = presence_service_new(service_id_str, basic_status);
			if (service != NULL) {
				if (timestamp_str != NULL) {
					presence_service_set_timestamp(service, parse_timestamp(timestamp_str));
					free_xml_text_content(timestamp_str);
				}
				if (contact_str != NULL) {
					presence_service_set_contact(service, contact_str);
					free_xml_text_content(contact_str);
				}
				process_pidf_xml_presence_service_notes(xml_ctx, service, i);
				presence_model_add_service(model, service);
			}
			free_xml_text_content(basic_status_str);
			if (service_id_str != NULL) free_xml_text_content(service_id_str);
		}
	}
	if (service_object != NULL) xmlXPathFreeObject(service_object);

	return 0;
}

static const char *person_prefix = "/pidf:presence/dm:person";

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

void * linphone_presence_activity_get_user_data(LinphonePresenceActivity *activity) {
	return activity->user_data;
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

const char * linphone_presence_activity_get_description(const LinphonePresenceActivity *activity) {
	if (activity == NULL)
		return NULL;
	return activity->description;
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

void * linphone_presence_note_get_user_data(LinphonePresenceNote *note) {
	return note->user_data;
}

const char * linphone_presence_note_get_content(const LinphonePresenceNote *note) {
	if (note == NULL)
		return NULL;
	return note->content;
}

const char * linphone_presence_note_get_lang(const LinphonePresenceNote *note) {
	if (note == NULL)
		return NULL;
	return note->lang;
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

static int process_pidf_xml_presence_person_activities(xmlparsing_context_t *xml_ctx, struct _LinphonePresencePerson *person, unsigned int person_idx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr activities_nodes_object;
	xmlXPathObjectPtr activities_object;
	xmlNodePtr activity_node;
	struct _LinphonePresenceActivity *activity;
	const char *description;
	int i, j;
	int err = 0;

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities", person_prefix, person_idx);
	activities_nodes_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((activities_nodes_object != NULL) && (activities_nodes_object->nodesetval != NULL)) {
		for (i = 1; i <= activities_nodes_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities[%i]/rpid:*", person_prefix, person_idx, i);
			activities_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
			if ((activities_object != NULL) && (activities_object->nodesetval != NULL)) {
				for (j = 0; j < activities_object->nodesetval->nodeNr; j++) {
					activity_node = activities_object->nodesetval->nodeTab[j];
					if ((activity_node->name != NULL) && (is_valid_activity_name((const char *)activity_node->name) == TRUE)) {
						LinphonePresenceActivityType acttype;
						description = (const char *)xmlNodeGetContent(activity_node);
						if ((description != NULL) && (description[0] == '\0')) {
							free_xml_text_content(description);
							description = NULL;
						}
						err = activity_name_to_presence_activity_type((const char *)activity_node->name, &acttype);
						if (err < 0) break;
						activity = presence_activity_new(acttype, description);
						presence_person_add_activity(person, activity);
						if (description != NULL) free_xml_text_content(description);
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

static int process_pidf_xml_presence_person_notes(xmlparsing_context_t *xml_ctx, struct _LinphonePresencePerson *person, unsigned int person_idx) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr note_object;
	struct _LinphonePresenceNote *note;
	const char *note_str;
	const char *lang;
	int i;

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities/rpid:note", person_prefix, person_idx);
	note_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities/rpid:note[%i]", person_prefix, person_idx, i);
			note_str = get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities/rpid:note[%i]/@xml:lang", person_prefix, person_idx, i);
			lang = get_xml_text_content(xml_ctx, xpath_str);

			note = presence_note_new(note_str, lang);
			presence_person_add_activities_note(person, note);
			if (lang != NULL) free_xml_text_content(lang);
			free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/dm:note", person_prefix, person_idx);
	note_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/dm:note[%i]", person_prefix, person_idx, i);
			note_str = get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/dm:note[%i]/@xml:lang", person_prefix, person_idx, i);
			lang = get_xml_text_content(xml_ctx, xpath_str);

			note = presence_note_new(note_str, lang);
			presence_person_add_note(person, note);
			if (lang != NULL) free_xml_text_content(lang);
			free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	return 0;
}

static int process_pidf_xml_presence_persons(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr person_object;
	struct _LinphonePresencePerson *person;
	const char *person_id_str;
	const char *person_timestamp_str;
	time_t timestamp;
	int i;
	int err = 0;

	person_object = get_xml_xpath_object_for_node_list(xml_ctx, person_prefix);
	if ((person_object != NULL) && (person_object->nodesetval != NULL)) {
		for (i = 1; i <= person_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/@id", person_prefix, i);
			person_id_str = get_xml_text_content(xml_ctx, xpath_str);
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/pidf:timestamp", person_prefix, i);
			person_timestamp_str = get_xml_text_content(xml_ctx, xpath_str);
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
					presence_person_delete(person);
					break;
				}
			}
			if (person_id_str != NULL) free_xml_text_content(person_id_str);
			if (person_timestamp_str != NULL) free_xml_text_content(person_timestamp_str);
		}
	}
	if (person_object != NULL) xmlXPathFreeObject(person_object);

	if (err < 0) {
		/* Remove all the persons added since there was an error. */
		ms_list_for_each(model->persons, (MSIterateFunc)presence_person_delete);
	}
	return err;
}

static int process_pidf_xml_presence_notes(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr note_object;
	struct _LinphonePresenceNote *note;
	const char *note_str;
	const char *lang;
	int i;

	note_object = get_xml_xpath_object_for_node_list(xml_ctx, "/pidf:presence/pidf:note");
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "/pidf:presence/pidf:note[%i]", i);
			note_str = get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "/pidf:presence/pidf:note[%i]/@xml:lang", i);
			lang = get_xml_text_content(xml_ctx, xpath_str);

			note = presence_note_new(note_str, lang);
			presence_model_add_note(model, note);
			if (lang != NULL) free_xml_text_content(lang);
			free_xml_text_content(note_str);
		}
	}
	if (note_object != NULL) xmlXPathFreeObject(note_object);

	return 0;
}

static LinphonePresenceModel * process_pidf_xml_presence_notification(xmlparsing_context_t *xml_ctx) {
	LinphonePresenceModel *model = NULL;
	int err;

	if (create_xml_xpath_context(xml_ctx) < 0)
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
	LinphoneFriend *fl=linphone_friend_new_with_addr(subscriber);
	if (fl==NULL) return ;
	fl->insub=op;
	linphone_friend_set_inc_subscribe_policy(fl,LinphoneSPAccept);
	fl->inc_subscribe_pending=TRUE;
	lc->subscribers=ms_list_append(lc->subscribers,(void *)fl);
	if (lc->vtable.new_subscription_request!=NULL) {
		char *tmp=linphone_address_as_string(fl->uri);
		lc->vtable.new_subscription_request(lc,fl,tmp);
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
			if (sal_op_get_contact(cfg->op)) {
				sal_op_set_contact (op,sal_op_get_contact(cfg->op));
				ms_message("Contact for next subscribe answer has been fixed using proxy "/*to %s",fixed_contact*/);
			}
		}
	}
	
	/* check if we answer to this subscription */
	if (linphone_find_friend_by_addr(lc->friends,uri,&lf)!=NULL){
		lf->insub=op;
		lf->inc_subscribe_pending=TRUE;
		sal_subscribe_accept(op);
		linphone_friend_done(lf);	/*this will do all necessary actions */
	}else{
		/* check if this subscriber is in our black list */
		if (linphone_find_friend_by_addr(lc->subscribers,uri,&lf)){
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
		xml_ctx = xmlparsing_context_new();
		xmlSetGenericErrorFunc(xml_ctx, xmlparsing_genericxml_error);
		xml_ctx->doc = xmlReadDoc((const unsigned char*)body, 0, NULL, 0);
		if (xml_ctx->doc != NULL) {
			model = process_pidf_xml_presence_notification(xml_ctx);
		} else {
			ms_warning("Wrongly formatted presence XML: %s", xml_ctx->errorBuffer);
		}
		xmlparsing_context_destroy(xml_ctx);
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
			presence_model_add_activity(model, acttype, NULL);
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

static int write_xml_presence_note(xmlTextWriterPtr writer, struct _LinphonePresenceNote *note, const char *ns) {
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

static void write_xml_presence_note_obj(struct _LinphonePresenceNote *note, struct _presence_note_obj_st *st) {
	int err = write_xml_presence_note(st->writer, note, st->ns);
	if (err < 0) *st->err = err;
}

static int write_xml_presence_timestamp(xmlTextWriterPtr writer, time_t timestamp) {
	int err;
	char *timestamp_str = timestamp_to_string(timestamp);
	err = xmlTextWriterWriteElement(writer, (const xmlChar *)"timestamp", (const xmlChar *)timestamp_str);
	if (timestamp_str) ms_free(timestamp_str);
	return err;
}

static int write_xml_presence_service(xmlTextWriterPtr writer, struct _LinphonePresenceService *service, const char *contact) {
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
		err = xmlTextWriterWriteString(writer, (const xmlChar *)contact);
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

static bool_t is_valid_activity(struct _LinphonePresenceActivity *activity) {
	if ((activity->type == LinphonePresenceActivityOffline) || (activity->type == LinphonePresenceActivityOnline))
		return FALSE;
	return TRUE;
}

static int write_xml_presence_activity(xmlTextWriterPtr writer, struct _LinphonePresenceActivity *activity) {
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

static void write_xml_presence_activity_obj(struct _LinphonePresenceActivity *activity, struct _presence_activity_obj_st *st) {
	int err = write_xml_presence_activity(st->writer, activity);
	if (err < 0) *st->err = err;
}

static void person_has_valid_activity(struct _LinphonePresenceActivity *activity, bool_t *has_valid_activities) {
	if (is_valid_activity(activity) == TRUE) *has_valid_activities = TRUE;
}

static bool_t person_has_valid_activities(struct _LinphonePresencePerson *person) {
	bool_t has_valid_activities = FALSE;
	ms_list_for_each2(person->activities, (MSIterate2Func)person_has_valid_activity, &has_valid_activities);
	return has_valid_activities;
}

static int write_xml_presence_person(xmlTextWriterPtr writer, struct _LinphonePresencePerson *person) {
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

static void write_xml_presence_service_obj(struct _LinphonePresenceService *service, struct _presence_service_obj_st *st) {
	int err = write_xml_presence_service(st->writer, service, st->contact);
	if (err < 0) *st->err = err;
}

static void write_xml_presence_person_obj(struct _LinphonePresencePerson *person, struct _presence_person_obj_st *st) {
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
			struct _presence_service_obj_st st;
			st.writer = writer;
			st.contact = contact;
			st.err = &err;
			ms_list_for_each2(model->services, (MSIterate2Func)write_xml_presence_service_obj, &st);
		}
	}
	if ((err >= 0) && (model != NULL)) {
		struct _presence_person_obj_st st;
		st.writer = writer;
		st.err = &err;
		ms_list_for_each2(model->persons, (MSIterate2Func)write_xml_presence_person_obj, &st);
	}
	if ((err >= 0) && (model != NULL)) {
		struct _presence_note_obj_st st;
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

	xmlFreeTextWriter(writer);
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		*content = ms_strdup((char *)buf->content);
	}
	xmlBufferFree(buf);
}

void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model){
	char *tmp;
	LinphoneFriend *lf;
	LinphoneAddress *friend=NULL;
	LinphonePresenceModel *presence = (LinphonePresenceModel *)model;

	lf=linphone_find_friend_by_out_subscribe(lc->friends,op);
	if (lf==NULL && lp_config_get_int(lc->config,"sip","allow_out_of_subscribe_presence",0)){
		const SalAddress *addr=sal_op_get_from_address(op);
		lf=NULL;
		linphone_find_friend_by_addr(lc->friends,(LinphoneAddress*)addr,&lf);
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
		if (lc->vtable.notify_presence_recv)
			lc->vtable.notify_presence_recv(lc,(LinphoneFriend*)lf);
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
