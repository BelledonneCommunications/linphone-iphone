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
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


#define XMLPARSING_BUFFER_LEN	2048
#define MAX_XPATH_LENGTH	256



extern const char *__policy_enum_to_str(LinphoneSubscribePolicy pol);



struct _LinphonePresenceNote {
	char *lang;
	char *content;
};

struct _LinphonePresenceService {
	char *id;
	LinphonePresenceBasicStatus status;
};

struct _LinphonePresenceActivity {
	LinphonePresenceActivity activity;
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

static struct _LinphonePresenceNote * presence_note_new(const char *content, const char *lang) {
	struct _LinphonePresenceNote * note = ms_new0(struct _LinphonePresenceNote, 1);
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

static struct _LinphonePresenceService * presence_service_new(const char *id, LinphonePresenceBasicStatus status) {
	struct _LinphonePresenceService *service = ms_new0(struct _LinphonePresenceService, 1);
	if (id != NULL) {
		service->id = ms_strdup(id);
	}
	service->status = status;
	return service;
}

static void presence_service_delete(struct _LinphonePresenceService *service) {
	if (service->id != NULL) {
		ms_free(service->id);
	}
	ms_free(service);
};

static struct _LinphonePresenceActivity * presence_activity_new(LinphonePresenceActivity activity, const char *description) {
	struct _LinphonePresenceActivity *act = ms_new0(struct _LinphonePresenceActivity, 1);
	act->activity = activity;
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

static struct _LinphonePresencePerson * presence_person_new(const char *id, const char *timestamp) {
	struct _LinphonePresencePerson *person = ms_new0(struct _LinphonePresencePerson, 1);
	if (id != NULL) {
		person->id = ms_strdup(id);
	}
	if (timestamp != NULL) {
		person->timestamp = parse_timestamp(timestamp);
		if (person->timestamp == ((time_t)-1))
			person->timestamp = time(NULL);
	} else {
		person->timestamp = time(NULL);
	}
	return person;
}

static void presence_person_delete(struct _LinphonePresencePerson *person) {
	if (person->id != NULL) {
		ms_free(person->id);
	}
	ms_list_for_each(person->activities, (MSIterateFunc)presence_activity_delete);
	ms_list_free(person->activities);
	ms_list_for_each(person->activities_notes, (MSIterateFunc)presence_note_delete);
	ms_list_free(person->activities_notes);
	ms_list_for_each(person->notes, (MSIterateFunc)presence_note_delete);
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

static void presence_model_add_service(LinphonePresenceModel *model, struct _LinphonePresenceService *service) {
	model->services = ms_list_append(model->services, service);
}

static void presence_model_add_person(LinphonePresenceModel *model, struct _LinphonePresencePerson *person) {
	model->persons = ms_list_append(model->persons, person);
}

static void presence_model_add_note(LinphonePresenceModel *model, struct _LinphonePresenceNote *note) {
	model->notes = ms_list_append(model->notes, note);
}

static void presence_model_find_open_basic_status(struct _LinphonePresenceService *service, LinphonePresenceBasicStatus *status) {
	if (service->status == LinphonePresenceBasicStatusOpen) {
		*status = LinphonePresenceBasicStatusOpen;
	}
}

LinphonePresenceModel * linphone_presence_model_new(void) {
	return ms_new0(LinphonePresenceModel, 1);
}

void linphone_presence_model_delete(LinphonePresenceModel *model) {
	ms_list_for_each(model->services, (MSIterateFunc)presence_service_delete);
	ms_list_free(model->services);
	ms_list_for_each(model->persons, (MSIterateFunc)presence_person_delete);
	ms_list_free(model->persons);
	ms_list_for_each(model->notes, (MSIterateFunc)presence_note_delete);
	ms_list_free(model->notes);
	ms_free(model);
}

/* Suppose that if at least one service is open, then the model is open. */
LinphonePresenceBasicStatus linphone_presence_model_get_basic_status(const LinphonePresenceModel *model) {
	LinphonePresenceBasicStatus status = LinphonePresenceBasicStatusClosed;
	ms_list_for_each2(model->services, (MSIterate2Func)presence_model_find_open_basic_status, &status);
	return status;
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
	LinphonePresenceActivity *activity;
	char **description;
};

static void presence_model_get_activity(const struct _LinphonePresencePerson *person, struct _get_activity_st *st) {
	struct _LinphonePresenceActivity *activity;
	unsigned int size = ms_list_size(person->activities);
	if (st->requested_idx < (st->current_idx + size)) {
		activity = (struct _LinphonePresenceActivity *)ms_list_nth_data(person->activities, st->requested_idx - st->current_idx);
		*st->activity = activity->activity;
		if (st->description != NULL) {
			*st->description = activity->description;
		}
	} else {
		st->current_idx += size;
	}
}

int linphone_presence_model_get_activity(const LinphonePresenceModel *model, unsigned int idx, LinphonePresenceActivity *activity, char **description) {
	struct _get_activity_st st;
	if ((activity == NULL) || (idx >= linphone_presence_model_nb_activities(model)))
		return -1;
	memset(&st, 0, sizeof(st));
	st.requested_idx = idx;
	st.activity = activity;
	*st.activity = LinphonePresenceActivityUnknown;
	if (description != NULL) {
		st.description = description;
	}
	ms_list_for_each2(model->persons, (MSIterate2Func)presence_model_get_activity, &st);
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

static int process_rfcxxxx_presence_notification(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	LinphonePresenceBasicStatus basic_status;
	struct _LinphonePresenceService *service;
	struct _LinphonePresencePerson *person;
	struct _LinphonePresenceActivity *activity = NULL;
	char *status_text = NULL;
	char *substatus_text = NULL;

	if (create_xml_xpath_context(xml_ctx) < 0)
		return -1;
	status_text = get_xml_text_content(xml_ctx, "/presence/atom/address/status/@status");
	if (status_text == NULL)
		return -1;
	substatus_text = get_xml_text_content(xml_ctx, "/presence/atom/address/msnsubstatus/@substatus");
	if (substatus_text == NULL) {
		free_xml_text_content(status_text);
		return -1;
	}

	if (strcmp(status_text, "open") == 0) {
		basic_status = LinphonePresenceBasicStatusOpen;
		if (strcmp(substatus_text, "berightback") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityInTransit, NULL);
		} else if (strcmp(substatus_text, "away") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityAway, NULL);
		} else if (strcmp(substatus_text, "outtolunch") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityMeal, NULL);
		}
	} else if (strcmp(status_text, "inuse") == 0) {
		basic_status = LinphonePresenceBasicStatusOpen;
		if (strcmp(substatus_text, "busy") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityBusy, NULL);
		} else if (strcmp(substatus_text, "onthephone") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityOnThePhone, NULL);
		}
	} else if (strcmp(status_text, "closed") == 0) {
		basic_status = LinphonePresenceBasicStatusClosed;
	}
	service = presence_service_new(NULL, basic_status);
	if (service != NULL) {
		presence_model_add_service(model, service);
	}
	if (activity != NULL) {
		person = presence_person_new(NULL, NULL);
		if (person != NULL) {
			presence_person_add_activity(person, activity);
			presence_model_add_person(model, person);
		}
	}
	free_xml_text_content(status_text);
	free_xml_text_content(substatus_text);

	return 0;
}

static int process_msoldpres_presence_notification(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	LinphonePresenceBasicStatus basic_status;
	struct _LinphonePresenceService *service;
	struct _LinphonePresencePerson *person;
	struct _LinphonePresenceActivity *activity = NULL;
	char *status_text = NULL;
	char *substatus_text = NULL;

	if (create_xml_xpath_context(xml_ctx) < 0)
		return -1;
	status_text = get_xml_text_content(xml_ctx, "/presence/atom/address/status/@status");
	if (status_text == NULL)
		return -1;
	substatus_text = get_xml_text_content(xml_ctx, "/presence/atom/address/msnsubstatus/@substatus");
	if (substatus_text == NULL) {
		free_xml_text_content(status_text);
		return -1;
	}

	if (strcmp(status_text, "open") == 0) {
		basic_status = LinphonePresenceBasicStatusOpen;
	} else if (strcmp(status_text, "inuse") == 0) {
		basic_status = LinphonePresenceBasicStatusOpen;
		if (strcmp(substatus_text, "busy") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityBusy, NULL);
		} else if (strcmp(substatus_text, "onthephone") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityOnThePhone, NULL);
		}
	} else if (strcmp(status_text, "inactive") == 0) {
		basic_status = LinphonePresenceBasicStatusOpen;
		if (strcmp(substatus_text, "berightback") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityInTransit, NULL);
		} else if (strcmp(substatus_text, "idle") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityAway, NULL);
		} else if (strcmp(substatus_text, "outtolunch") == 0) {
			activity = presence_activity_new(LinphonePresenceActivityMeal, NULL);
		}
	} else if (strcmp(status_text, "closed") == 0) {
		basic_status = LinphonePresenceBasicStatusClosed;
	}
	service = presence_service_new(NULL, basic_status);
	if (service != NULL) {
		presence_model_add_service(model, service);
	}
	if (activity != NULL) {
		person = presence_person_new(NULL, NULL);
		if (person != NULL) {
			presence_person_add_activity(person, activity);
			presence_model_add_person(model, person);
		}
	}

	return 0;
}

static const char *service_prefix = "/pidf:presence/pidf:tuple";

static int process_pidf_xml_presence_services(xmlparsing_context_t *xml_ctx, LinphonePresenceModel *model) {
	char xpath_str[MAX_XPATH_LENGTH];
	xmlXPathObjectPtr service_object;
	struct _LinphonePresenceService *service;
	const char *basic_status_str;
	const char *service_id_str;
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

			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/@id", service_prefix, i);
			service_id_str = get_xml_text_content(xml_ctx, xpath_str);
			service = presence_service_new(service_id_str, basic_status);
			if (service != NULL) {
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
	LinphonePresenceActivity activity;
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

static int activity_name_to_linphone_presence_activity(const char *name, LinphonePresenceActivity *activity) {
	unsigned int i;
	for (i = 0; i < (sizeof(activity_map) / sizeof(activity_map[0])); i++) {
		if (strcmp(name, activity_map[i].name) == 0) {
			*activity = activity_map[i].activity;
			return 0;
		}
	}
	return -1;
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
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:activities[%i]/*", person_prefix, person_idx, i);
			activities_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
			if ((activities_object != NULL) && (activities_object->nodesetval != NULL)) {
				for (j = 0; j < activities_object->nodesetval->nodeNr; j++) {
					activity_node = activities_object->nodesetval->nodeTab[j];
					if ((activity_node->name != NULL)
						&& (activity_node->ns != NULL)
						&& (activity_node->ns->prefix != NULL)
						&& (strcmp((const char *)activity_node->ns->prefix, "rpid") == 0)) {
						LinphonePresenceActivity linphone_activity;
						description = (const char *)xmlNodeGetContent(activity_node);
						if ((description != NULL) && (description[0] == '\0')) {
							free_xml_text_content(description);
							description = NULL;
						}
						err = activity_name_to_linphone_presence_activity((const char *)activity_node->name, &linphone_activity);
						if (err < 0) break;
						activity = presence_activity_new(linphone_activity, description);
						presence_person_add_activity(person, activity);
						if (description != NULL) free_xml_text_content(description);
					}
				}
			}
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

	snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:note", person_prefix, person_idx);
	note_object = get_xml_xpath_object_for_node_list(xml_ctx, xpath_str);
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:note[%i]", person_prefix, person_idx, i);
			note_str = get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/rpid:note[%i]/@xml:lang", person_prefix, person_idx, i);
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
	int i;
	int err = 0;

	person_object = get_xml_xpath_object_for_node_list(xml_ctx, person_prefix);
	if ((person_object != NULL) && (person_object->nodesetval != NULL)) {
		for (i = 1; i <= person_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/@id", person_prefix, i);
			person_id_str = get_xml_text_content(xml_ctx, xpath_str);
			snprintf(xpath_str, sizeof(xpath_str), "%s[%i]/timestamp", person_prefix, i);
			person_timestamp_str = get_xml_text_content(xml_ctx, xpath_str);
			person = presence_person_new(person_id_str, person_timestamp_str);
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

	note_object = get_xml_xpath_object_for_node_list(xml_ctx, "/pidf:presence/rpid:note");
	if ((note_object != NULL) && (note_object->nodesetval != NULL)) {
		for (i = 1; i <= note_object->nodesetval->nodeNr; i++) {
			snprintf(xpath_str, sizeof(xpath_str), "/pidf:presence/rpid:note[%i]", i);
			note_str = get_xml_text_content(xml_ctx, xpath_str);
			if (note_str == NULL) continue;
			snprintf(xpath_str, sizeof(xpath_str), "/pidf:presence/rpid:note[%i]/@xml:lang", i);
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
		linphone_presence_model_delete(model);
		model = NULL;
	}

	return model;
}

static LinphonePresenceModel * process_xpidf_xml_presence_notification(xmlparsing_context_t *xml_ctx) {
	LinphonePresenceModel *model = NULL;
	int err = -1;
	xmlDtdPtr dtd = xmlGetIntSubset(xml_ctx->doc);

	if (dtd != NULL) {
		if (strcmp((const char *)dtd->name, "presence") == 0) {
			model = linphone_presence_model_new();
			if ((strcmp((const char *)dtd->SystemID, "xpidf.dtd") == 0)
				&& (strcmp((const char *)dtd->ExternalID, "-//IETF//DTD RFCxxxx XPIDF 1.0//EN") == 0)) {
				err = process_rfcxxxx_presence_notification(xml_ctx, model);
			} else if (strcmp((const char *)dtd->SystemID, "http://schemas.microsoft.com/2002/09/sip/presence") == 0) {
				err = process_msoldpres_presence_notification(xml_ctx, model);
			}
		}
	}

	if ((err < 0) && (model != NULL)) {
		linphone_presence_model_delete(model);
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

void linphone_core_notify_all_friends(LinphoneCore *lc, LinphoneOnlineStatus os){
	MSList *elem;
	ms_message("Notifying all friends that we are in status %i",os);
	for(elem=lc->friends;elem!=NULL;elem=elem->next){
		LinphoneFriend *lf=(LinphoneFriend *)elem->data;
		if (lf->insub){
			linphone_friend_notify(lf,os);
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
	if (linphone_find_friend(lc->friends,uri,&lf)!=NULL){
		lf->insub=op;
		lf->inc_subscribe_pending=TRUE;
		sal_subscribe_accept(op);
		linphone_friend_done(lf);	/*this will do all necessary actions */
	}else{
		/* check if this subscriber is in our black list */
		if (linphone_find_friend(lc->subscribers,uri,&lf)){
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
	bool_t pidf_xml = FALSE;
	bool_t xpidf_xml = FALSE;
	LinphonePresenceModel *model = NULL;

	if (strcmp(content_type, "application") != 0) {
		*result = NULL;
		return;
	}

	pidf_xml = (strcmp(content_subtype, "pidf+xml") == 0);
	xpidf_xml = (strcmp(content_subtype, "xpidf+xml") == 0);
	if (pidf_xml || xpidf_xml) {
		xml_ctx = xmlparsing_context_new();
		xmlSetGenericErrorFunc(xml_ctx, xmlparsing_genericxml_error);
		xml_ctx->doc = xmlReadDoc((const unsigned char*)body, 0, NULL, 0);
		if (xml_ctx->doc != NULL) {
			if (pidf_xml)
				model = process_pidf_xml_presence_notification(xml_ctx);
			if (xpidf_xml)
				model = process_xpidf_xml_presence_notification(xml_ctx);
		} else {
			ms_warning("Wrongly formatted presence XML: %s", xml_ctx->errorBuffer);
		}
		xmlparsing_context_destroy(xml_ctx);
	}

	*result = (SalPresenceModel *)model;
}

void linphone_notify_recv(LinphoneCore *lc, SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model){
	char *tmp;
	LinphoneFriend *lf;
	LinphoneAddress *friend=NULL;

	lf=linphone_find_friend_by_out_subscribe(lc->friends,op);
	if (lf!=NULL){
		friend=lf->uri;
		tmp=linphone_address_as_string(friend);
		linphone_friend_set_presence(lf, (LinphonePresenceModel *)model);
		lf->subscribe_active=TRUE;
		if (lc->vtable.notify_presence_recv)
			lc->vtable.notify_presence_recv(lc,(LinphoneFriend*)lf);
		ms_free(tmp);
	}else{
		ms_message("But this person is not part of our friend list, so we don't care.");
		linphone_presence_model_delete((LinphonePresenceModel *)model);
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
