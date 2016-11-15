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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone/core.h"
#include "private.h"



static void linphone_buffer_destroy(LinphoneBuffer *buffer) {
	if (buffer->content) belle_sip_free(buffer->content);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneBuffer);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneBuffer, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_buffer_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);


LinphoneBuffer * linphone_buffer_new(void) {
	LinphoneBuffer *buffer = belle_sip_object_new(LinphoneBuffer);
	belle_sip_object_ref(buffer);
	return buffer;
}

LinphoneBuffer * linphone_buffer_new_from_data(const uint8_t *data, size_t size) {
	LinphoneBuffer *buffer = linphone_buffer_new();
	linphone_buffer_set_content(buffer, data, size);
	return buffer;
}

LinphoneBuffer * linphone_buffer_new_from_string(const char *data) {
	LinphoneBuffer *buffer = linphone_buffer_new();
	linphone_buffer_set_string_content(buffer, data);
	return buffer;
}

LinphoneBuffer * linphone_buffer_ref(LinphoneBuffer *buffer) {
	belle_sip_object_ref(buffer);
	return buffer;
}

void linphone_buffer_unref(LinphoneBuffer *buffer) {
	belle_sip_object_unref(buffer);
}

void *linphone_buffer_get_user_data(const LinphoneBuffer *buffer) {
	return buffer->user_data;
}

void linphone_buffer_set_user_data(LinphoneBuffer *buffer, void *ud) {
	buffer->user_data = ud;
}

const uint8_t * linphone_buffer_get_content(const LinphoneBuffer *buffer) {
	return buffer->content;
}

void linphone_buffer_set_content(LinphoneBuffer *buffer, const uint8_t *content, size_t size) {
	buffer->size = size;
	if (buffer->content) belle_sip_free(buffer->content);
	buffer->content = belle_sip_malloc(size + 1);
	memcpy(buffer->content, content, size);
    ((char *)buffer->content)[size] = '\0';
}

const char * linphone_buffer_get_string_content(const LinphoneBuffer *buffer) {
	return (const char *)buffer->content;
}

void linphone_buffer_set_string_content(LinphoneBuffer *buffer, const char *content) {
	buffer->size = strlen(content);
	if (buffer->content) belle_sip_free(buffer->content);
	buffer->content = (uint8_t *)belle_sip_strdup(content);
}

size_t linphone_buffer_get_size(const LinphoneBuffer *buffer) {
	return buffer->size;
}

void linphone_buffer_set_size(LinphoneBuffer *buffer, size_t size) {
	buffer->size = size;
}

bool_t linphone_buffer_is_empty(const LinphoneBuffer *buffer) {
	return (buffer->size == 0) ? TRUE : FALSE;
}
