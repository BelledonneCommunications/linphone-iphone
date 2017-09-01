/*
linphone
Copyright (C) 2010-2017 Belledonne Communications SARL

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

#include "linphone/factory.h"
#include "linphone/video_definition.h"

#include "private.h"


static void linphone_video_definition_destroy(LinphoneVideoDefinition *vdef) {
	if (vdef->name) bctbx_free(vdef->name);
}

static void _linphone_video_definition_clone(LinphoneVideoDefinition *obj, const LinphoneVideoDefinition *orig){
	obj->name = bctbx_strdup(orig->name);
	obj->width = orig->width;
	obj->height = orig->height;
}

belle_sip_error_code _linphone_video_definition_marshal(const LinphoneVideoDefinition* obj, char* buff, size_t buff_size, size_t *offset){
	return belle_sip_snprintf(buff,buff_size,offset,"%s",obj->name);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneVideoDefinition);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneVideoDefinition, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_video_definition_destroy,
	(belle_sip_object_clone_t)_linphone_video_definition_clone, // clone
	(belle_sip_object_marshal_t)_linphone_video_definition_marshal, // marshal
	FALSE
);


LinphoneVideoDefinition * linphone_video_definition_new(unsigned int width, unsigned int height, const char *name) {
	LinphoneVideoDefinition *vdef = belle_sip_object_new(LinphoneVideoDefinition);
	vdef->width = width;
	vdef->height = height;
	if (name == NULL) {
		vdef->name = bctbx_strdup_printf("%ux%u", width, height);
	} else {
		vdef->name = bctbx_strdup(name);
	}
	return vdef;
}

LinphoneVideoDefinition * linphone_video_definition_ref(LinphoneVideoDefinition *vdef) {
	belle_sip_object_ref(vdef);
	return vdef;
}

void linphone_video_definition_unref(LinphoneVideoDefinition *vdef) {
	belle_sip_object_unref(vdef);
}

void *linphone_video_definition_get_user_data(const LinphoneVideoDefinition *vdef) {
	return vdef->user_data;
}

void linphone_video_definition_set_user_data(LinphoneVideoDefinition *vdef, void *ud) {
	vdef->user_data = ud;
}

LinphoneVideoDefinition * linphone_video_definition_clone(const LinphoneVideoDefinition *vdef) {
	return (LinphoneVideoDefinition*)belle_sip_object_clone((belle_sip_object_t*)vdef);
}

unsigned int linphone_video_definition_get_width(const LinphoneVideoDefinition *vdef) {
	return vdef->width;
}

void linphone_video_definition_set_width(LinphoneVideoDefinition *vdef, unsigned int width) {
	vdef->width = width;
}

unsigned int linphone_video_definition_get_height(const LinphoneVideoDefinition *vdef) {
	return vdef->height;
}

void linphone_video_definition_set_height(LinphoneVideoDefinition *vdef, unsigned int height) {
	vdef->height = height;
}

void linphone_video_definition_set_definition(LinphoneVideoDefinition *vdef, unsigned int width, unsigned int height) {
	vdef->width = width;
	vdef->height = height;
}

const char * linphone_video_definition_get_name(const LinphoneVideoDefinition *vdef) {
	return vdef->name;
}

void linphone_video_definition_set_name(LinphoneVideoDefinition *vdef, const char *name) {
	if (vdef->name != NULL) bctbx_free(vdef->name);
	vdef->name = bctbx_strdup(name);
}

bool_t linphone_video_definition_equals(const LinphoneVideoDefinition *vdef1, const LinphoneVideoDefinition *vdef2) {
	return ((vdef1 != NULL && vdef2 != NULL)
		&& (((vdef1->width == vdef2->width) && (vdef1->height == vdef2->height))
		|| ((vdef1->width == vdef2->height) && (vdef1->height == vdef2->width))));
}

bool_t linphone_video_definition_strict_equals(const LinphoneVideoDefinition *vdef1, const LinphoneVideoDefinition *vdef2) {
	return (vdef1->width == vdef2->width) && (vdef1->height == vdef2->height);
}

bool_t linphone_video_definition_is_undefined(const LinphoneVideoDefinition *vdef) {
	return (vdef->width == 0) || (vdef->height == 0);
}
