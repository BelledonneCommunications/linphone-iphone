/*
linphone
Copyright (C) 2010-2014 Belledonne Communications SARL

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



static void linphone_content_destroy(LinphoneContent *content) {
	if (content->owned_fields == TRUE) {
		if (content->lcp.type) belle_sip_free(content->lcp.type);
		if (content->lcp.subtype) belle_sip_free(content->lcp.subtype);
		if (content->lcp.data) belle_sip_free(content->lcp.data);
		if (content->lcp.encoding) belle_sip_free(content->lcp.encoding);
		if (content->lcp.name) belle_sip_free(content->lcp.name);
	}
}

static void linphone_content_clone(LinphoneContent *obj, const LinphoneContent *ref) {
	linphone_content_set_type(obj, linphone_content_get_type(ref));
	linphone_content_set_subtype(obj, linphone_content_get_subtype(ref));
	linphone_content_set_encoding(obj, linphone_content_get_encoding(ref));
	linphone_content_set_name(obj, linphone_content_get_name(ref));
	if (linphone_content_get_buffer(ref) != NULL) {
		linphone_content_set_buffer(obj, linphone_content_get_buffer(ref), linphone_content_get_size(ref));
	} else {
		linphone_content_set_size(obj, linphone_content_get_size(ref));
	}
}


BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneContent);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneContent, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_content_destroy,
	(belle_sip_object_clone_t)linphone_content_clone,
	NULL, // marshal
	TRUE
);


LinphoneContent * linphone_core_create_content(LinphoneCore *lc) {
	return linphone_content_new();
}

LinphoneContent * linphone_content_ref(LinphoneContent *content) {
	belle_sip_object_ref(content);
	return content;
}

void linphone_content_unref(LinphoneContent *content) {
	belle_sip_object_unref(content);
}

void *linphone_content_get_user_data(const LinphoneContent *content) {
	return content->user_data;
}

void linphone_content_set_user_data(LinphoneContent *content, void *ud) {
	content->user_data = ud;
}

const char * linphone_content_get_type(const LinphoneContent *content) {
	return content->lcp.type;
}

void linphone_content_set_type(LinphoneContent *content, const char *type) {
	if (content->lcp.type != NULL) {
		belle_sip_free(content->lcp.type);
		content->lcp.type = NULL;
	}
	if (type != NULL) {
		content->lcp.type = belle_sip_strdup(type);
	}
}

const char * linphone_content_get_subtype(const LinphoneContent *content) {
	return content->lcp.subtype;
}

void linphone_content_set_subtype(LinphoneContent *content, const char *subtype) {
	if (content->lcp.subtype != NULL) {
		belle_sip_free(content->lcp.subtype);
		content->lcp.subtype = NULL;
	}
	if (subtype != NULL) {
		content->lcp.subtype = belle_sip_strdup(subtype);
	}
}

void * linphone_content_get_buffer(const LinphoneContent *content) {
	return content->lcp.data;
}

void linphone_content_set_buffer(LinphoneContent *content, const void *buffer, size_t size) {
	content->lcp.size = size;
	content->lcp.data = belle_sip_malloc(size + 1);
	memcpy(content->lcp.data, buffer, size);
	((char *)content->lcp.data)[size] = '\0';
}

char * linphone_content_get_string_buffer(const LinphoneContent *content) {
	return (char *)content->lcp.data;
}

void linphone_content_set_string_buffer(LinphoneContent *content, const char *buffer) {
	content->lcp.size = strlen(buffer);
	content->lcp.data = belle_sip_strdup(buffer);
}

size_t linphone_content_get_size(const LinphoneContent *content) {
	return content->lcp.size;
}

void linphone_content_set_size(LinphoneContent *content, size_t size) {
	content->lcp.size = size;
}

const char * linphone_content_get_encoding(const LinphoneContent *content) {
	return content->lcp.encoding;
}

void linphone_content_set_encoding(LinphoneContent *content, const char *encoding) {
	if (content->lcp.encoding != NULL) {
		belle_sip_free(content->lcp.encoding);
		content->lcp.encoding = NULL;
	}
	if (encoding != NULL) {
		content->lcp.encoding = belle_sip_strdup(encoding);
	}
}

const char * linphone_content_get_name(const LinphoneContent *content) {
	return content->lcp.name;
}

void linphone_content_set_name(LinphoneContent *content, const char *name) {
	if (content->lcp.name != NULL) {
		belle_sip_free(content->lcp.name);
		content->lcp.name = NULL;
	}
	if (name != NULL) {
		content->lcp.name = belle_sip_strdup(name);
	}
}



LinphoneContent * linphone_content_new(void) {
	LinphoneContent *content = belle_sip_object_new(LinphoneContent);
	belle_sip_object_ref(content);
	content->owned_fields = TRUE;
	return content;
}

LinphoneContent * linphone_content_copy(const LinphoneContent *ref) {
	return (LinphoneContent *)belle_sip_object_ref(belle_sip_object_clone(BELLE_SIP_OBJECT(ref)));
}

LinphoneContent * linphone_content_from_sal_body(const SalBody *ref) {
	if (ref && ref->type) {
		LinphoneContent *content = linphone_content_new();
		linphone_content_set_type(content, ref->type);
		linphone_content_set_subtype(content, ref->subtype);
		linphone_content_set_encoding(content, ref->encoding);
		if (ref->data != NULL) {
			linphone_content_set_buffer(content, ref->data, ref->size);
		} else {
			linphone_content_set_size(content, ref->size);
		}
		return content;
	}
	return NULL;
}

SalBody *sal_body_from_content(SalBody *body, const LinphoneContent *content) {
	if (content && linphone_content_get_type(content)) {
		body->type = linphone_content_get_type(content);
		body->subtype = linphone_content_get_subtype(content);
		body->data = linphone_content_get_buffer(content);
		body->size = linphone_content_get_size(content);
		body->encoding = linphone_content_get_encoding(content);
		return body;
	}
	return NULL;
}



LinphoneContent * linphone_content_private_to_linphone_content(const LinphoneContentPrivate *lcp) {
	LinphoneContent *content = belle_sip_object_new(LinphoneContent);
	memcpy(&content->lcp, lcp, sizeof(LinphoneContentPrivate));
	content->owned_fields = FALSE;
	return content;
}

LinphoneContentPrivate * linphone_content_to_linphone_content_private(const LinphoneContent *content) {
	return (LinphoneContentPrivate *)&content->lcp;
}
