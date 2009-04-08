/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

#include "mediastreamer2/mswebcam.h"
#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msfilter.h"

static MSWebCamManager *scm=NULL;

static MSWebCamManager * create_manager(){
	MSWebCamManager *obj=(MSWebCamManager *)ms_new(MSWebCamManager,1);
	obj->cams=NULL;
	obj->descs=NULL;
	return obj;
}

void ms_web_cam_manager_destroy(void){
	if (scm!=NULL){
		ms_list_for_each(scm->cams,(void (*)(void*))ms_web_cam_destroy);
		ms_list_free(scm->cams);
		ms_list_free(scm->descs);
	}
	ms_free(scm);
	scm=NULL;
}

MSWebCamManager * ms_web_cam_manager_get(void){
	if (scm==NULL) scm=create_manager();
	return scm;
}

MSWebCam * ms_web_cam_manager_get_cam(MSWebCamManager *m, const char *id){
	MSList *elem;
	for (elem=m->cams;elem!=NULL;elem=elem->next){
		MSWebCam *cam=(MSWebCam*)elem->data;
		if (id==NULL) return cam;
		if (strcmp(ms_web_cam_get_string_id(cam),id)==0)	return cam;
	}
	if (id!=NULL) ms_warning("no camera with id %s",id);
	return NULL;
}

MSWebCam * ms_web_cam_manager_get_default_cam(MSWebCamManager *m){
	if (m->cams!=NULL)
  		return (MSWebCam*)m->cams->data;
  	return NULL;
}

const MSList * ms_web_cam_manager_get_list(MSWebCamManager *m){
	return m->cams;
}

void ms_web_cam_manager_add_cam(MSWebCamManager *m, MSWebCam *c){
	ms_message("Webcam %s added",ms_web_cam_get_string_id(c));
	m->cams=ms_list_append(m->cams,c);
}

void ms_web_cam_manager_prepend_cam(MSWebCamManager *m, MSWebCam *c){
	ms_message("Webcam %s prepended",ms_web_cam_get_string_id(c));
	m->cams=ms_list_prepend(m->cams,c);
}

static void cam_detect(MSWebCamManager *m, MSWebCamDesc *desc){
	if (desc->detect!=NULL)
		desc->detect(m);
}

void ms_web_cam_manager_register_desc(MSWebCamManager *m, MSWebCamDesc *desc){
	m->descs=ms_list_append(m->descs,desc);
	cam_detect(m,desc);
}

void ms_web_cam_manager_reload(MSWebCamManager *m){
	MSList *elem;
	ms_list_for_each(m->cams,(void (*)(void*))ms_web_cam_destroy);
	ms_list_free(m->cams);
	m->cams=NULL;
	for(elem=m->descs;elem!=NULL;elem=elem->next)
		cam_detect(m,(MSWebCamDesc*)elem->data);
}

MSWebCam * ms_web_cam_new(MSWebCamDesc *desc){
	MSWebCam *obj=(MSWebCam *)ms_new(MSWebCam,1);
	obj->desc=desc;
	obj->name=NULL;
	obj->data=NULL;
	obj->id=NULL;
	if (desc->init!=NULL)
		desc->init(obj);
	return obj;
}

const char *ms_web_cam_get_driver_type(const MSWebCam *obj){
	return obj->desc->driver_type;
}

const char *ms_web_cam_get_name(const MSWebCam *obj){
	return obj->name;
}

const char *ms_web_cam_get_string_id(MSWebCam *obj){
	if (obj->id==NULL)	obj->id=ms_strdup_printf("%s: %s",obj->desc->driver_type,obj->name);
	return obj->id;
}

struct _MSFilter * ms_web_cam_create_reader(MSWebCam *obj){
	if (obj->desc->create_reader!=NULL)
		return obj->desc->create_reader(obj);
	else ms_warning("ms_web_cam_create_reader: unimplemented by %s wrapper",obj->desc->driver_type);
	return NULL;
}

void ms_web_cam_destroy(MSWebCam *obj){
	if (obj->desc->uninit!=NULL) obj->desc->uninit(obj);
	if (obj->name!=NULL) ms_free(obj->name);
	if (obj->id!=NULL)	ms_free(obj->id);
	ms_free(obj);
}
