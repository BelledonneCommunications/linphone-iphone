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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

extern void __register_ffmpeg_encoders_if_possible(void);

#include "mediastreamer2/mscommon.h"
#include "mediastreamer2/msfilter.h"

#include "alldescs.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/mswebcam.h"

#if !defined(_WIN32_WCE)
#include <sys/types.h>
#endif
#ifndef WIN32
#include <dirent.h>
#else
#ifndef PACKAGE_PLUGINS_DIR
#if defined(WIN32) || defined(_WIN32_WCE)
#define PACKAGE_PLUGINS_DIR "plugins\\"
#else
#define PACKAGE_PLUGINS_DIR "."
#endif
#endif
#endif
#ifdef HAVE_DLOPEN
#include <dlfcn.h>
#endif

#if defined(__APPLE__) && !defined(__GNUC__)
#import <Cocoa/Cocoa.h>
#include <Foundation/Foundation.h> 
#endif

MSList *ms_list_new(void *data){
	MSList *new_elem=(MSList *)ms_new(MSList,1);
	new_elem->prev=new_elem->next=NULL;
	new_elem->data=data;
	return new_elem;
}

MSList * ms_list_append(MSList *elem, void * data){
	MSList *new_elem=ms_list_new(data);
	MSList *it=elem;
	if (elem==NULL) return new_elem;
	while (it->next!=NULL) it=ms_list_next(it);
	it->next=new_elem;
	new_elem->prev=it;
	return elem;
}

MSList * ms_list_prepend(MSList *elem, void *data){
	MSList *new_elem=ms_list_new(data);
	if (elem!=NULL) {
		new_elem->next=elem;
		elem->prev=new_elem;
	}
	return new_elem;
}


MSList * ms_list_concat(MSList *first, MSList *second){
	MSList *it=first;
	if (it==NULL) return second;
	while(it->next!=NULL) it=ms_list_next(it);
	it->next=second;
	second->prev=it;
	return first;
}

MSList * ms_list_free(MSList *list){
	MSList *elem = list;
	MSList *tmp;
	if (list==NULL) return NULL;
	while(elem->next!=NULL) {
		tmp = elem;
		elem = elem->next;
		ms_free(tmp);
	}
	ms_free(elem);
	return NULL;
}

MSList * ms_list_remove(MSList *first, void *data){
	MSList *it;
	it=ms_list_find(first,data);
	if (it) return ms_list_remove_link(first,it);
	else {
		ms_warning("ms_list_remove: no element with %p data was in the list", data);
		return first;
	}
}

int ms_list_size(const MSList *first){
	int n=0;
	while(first!=NULL){
		++n;
		first=first->next;
	}
	return n;
}

void ms_list_for_each(const MSList *list, void (*func)(void *)){
	for(;list!=NULL;list=list->next){
		func(list->data);
	}
}

void ms_list_for_each2(const MSList *list, void (*func)(void *, void *), void *user_data){
	for(;list!=NULL;list=list->next){
		func(list->data,user_data);
	}
}

MSList *ms_list_remove_link(MSList *list, MSList *elem){
	MSList *ret;
	if (elem==list){
		ret=elem->next;
		elem->prev=NULL;
		elem->next=NULL;
		if (ret!=NULL) ret->prev=NULL;
		ms_free(elem);
		return ret;
	}
	elem->prev->next=elem->next;
	if (elem->next!=NULL) elem->next->prev=elem->prev;
	elem->next=NULL;
	elem->prev=NULL;
	ms_free(elem);
	return list;
}

MSList *ms_list_find(MSList *list, void *data){
	for(;list!=NULL;list=list->next){
		if (list->data==data) return list;
	}
	return NULL;
}

MSList *ms_list_find_custom(MSList *list, int (*compare_func)(const void *, const void*), void *user_data){
	for(;list!=NULL;list=list->next){
		if (compare_func(list->data,user_data)==0) return list;
	}
	return NULL;
}

void * ms_list_nth_data(const MSList *list, int index){
	int i;
	for(i=0;list!=NULL;list=list->next,++i){
		if (i==index) return list->data;
	}
	ms_error("ms_list_nth_data: no such index in list.");
	return NULL;
}

int ms_list_position(const MSList *list, MSList *elem){
	int i;
	for(i=0;list!=NULL;list=list->next,++i){
		if (elem==list) return i;
	}
	ms_error("ms_list_position: no such element in list.");
	return -1;
}

int ms_list_index(const MSList *list, void *data){
	int i;
	for(i=0;list!=NULL;list=list->next,++i){
		if (data==list->data) return i;
	}
	ms_error("ms_list_index: no such element in list.");
	return -1;
}

MSList *ms_list_insert_sorted(MSList *list, void *data, int (*compare_func)(const void *, const void*)){
	MSList *it,*previt=NULL;
	MSList *nelem;
	MSList *ret=list;
	if (list==NULL) return ms_list_append(list,data);
	else{
		nelem=ms_list_new(data);
		for(it=list;it!=NULL;it=it->next){
			previt=it;
			if (compare_func(data,it->data)<=0){
				nelem->prev=it->prev;
				nelem->next=it;
				if (it->prev!=NULL)
					it->prev->next=nelem;
				else{
					ret=nelem;
				}
				it->prev=nelem;
				return ret;
			}
		}
		previt->next=nelem;
		nelem->prev=previt;
	}
	return ret;
}

MSList *ms_list_insert(MSList *list, MSList *before, void *data){
	MSList *elem;
	if (list==NULL || before==NULL) return ms_list_append(list,data);
	for(elem=list;elem!=NULL;elem=ms_list_next(elem)){
		if (elem==before){
			if (elem->prev==NULL)
				return ms_list_prepend(list,data);
			else{
				MSList *nelem=ms_list_new(data);
				nelem->prev=elem->prev;
				nelem->next=elem;
				elem->prev->next=nelem;
				elem->prev=nelem;
			}
		}
	}
	return list;
}

MSList *ms_list_copy(const MSList *list){
	MSList *copy=NULL;
	const MSList *iter;
	for(iter=list;iter!=NULL;iter=ms_list_next(iter)){
		copy=ms_list_append(copy,iter->data);
	}
	return copy;
}


#ifdef __APPLE__
#define PLUGINS_EXT ".dylib"
#else
#define PLUGINS_EXT ".so"
#endif

typedef void (*init_func_t)(void);

int ms_load_plugins(const char *dir){
	int num=0;
#if defined(WIN32) && !defined(_WIN32_WCE)
	WIN32_FIND_DATA FileData; 
	HANDLE hSearch; 
	char szDirPath[1024]; 
	char szPluginFile[1024]; 
	BOOL fFinished = FALSE;
	const char *tmp=getenv("DEBUG");
	BOOL debug=(tmp!=NULL && atoi(tmp)==1);
	snprintf(szDirPath, sizeof(szDirPath), "%s", dir);

	// Start searching for .dll files in the current directory.

	snprintf(szDirPath, sizeof(szDirPath), "%s\\*.dll", dir);
	hSearch = FindFirstFile(szDirPath, &FileData);
	if (hSearch == INVALID_HANDLE_VALUE)
	{
		ms_message("no plugin (*.dll) found in %s.", szDirPath);
		return 0;
	}
	snprintf(szDirPath, sizeof(szDirPath), "%s", dir);

	while (!fFinished) 
	{
		/* load library */
		HINSTANCE os_handle;
		UINT em;
		if (!debug) em = SetErrorMode (SEM_FAILCRITICALERRORS);
		
		snprintf(szPluginFile, sizeof(szPluginFile), "%s\\%s", szDirPath, FileData.cFileName);
		os_handle = LoadLibraryEx (szPluginFile, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (os_handle==NULL)
		{
			ms_message("Fail to load plugin %s with altered search path: error %i",szPluginFile,GetLastError());
			os_handle = LoadLibraryEx (szPluginFile, NULL, 0);
		}
		if (!debug) SetErrorMode (em);
		if (os_handle==NULL)
			ms_error("Fail to load plugin %s", szPluginFile); 
		else{
			init_func_t initroutine;
			char szPluginName[256]; 
			char szMethodName[256];
			char *minus;
			snprintf(szPluginName, 256, "%s", FileData.cFileName);
			/*on mingw, dll names might be libsomething-3.dll. We must skip the -X.dll stuff*/
			minus=strchr(szPluginName,'-');
			if (minus) *minus='\0';
			else szPluginName[strlen(szPluginName)-4]='\0'; /*remove .dll*/
			snprintf(szMethodName, 256, "%s_init", szPluginName);
			initroutine = (init_func_t) GetProcAddress (os_handle, szMethodName);
				if (initroutine!=NULL){
					initroutine();
					ms_message("Plugin loaded (%s)", szPluginFile);
					num++;
				}else{
					ms_warning("Could not locate init routine of plugin %s. Should be %s",
					szPluginFile, szMethodName);
				}
		}
		if (!FindNextFile(hSearch, &FileData)) {
			if (GetLastError() == ERROR_NO_MORE_FILES){ 
				fFinished = TRUE; 
			} 
			else 
			{ 
				ms_error("couldn't find next plugin dll."); 
				fFinished = TRUE; 
			} 
		}
	} 
	/* Close the search handle. */
	FindClose(hSearch);

#elif HAVE_DLOPEN
	DIR *ds;
	struct dirent *de;
	char *fullpath;
	ds=opendir(dir);	
	if (ds==NULL){
		ms_message("Cannot open directory %s: %s",dir,strerror(errno));
		return -1;
	}
	while( (de=readdir(ds))!=NULL){
		if ((de->d_type==DT_REG && strstr(de->d_name,PLUGINS_EXT)!=NULL)
		    || (de->d_type==DT_UNKNOWN && strstr(de->d_name,PLUGINS_EXT)==de->d_name+strlen(de->d_name)-strlen(PLUGINS_EXT))) {
			void *handle;
			fullpath=ms_strdup_printf("%s/%s",dir,de->d_name);
			ms_message("Loading plugin %s...",fullpath);
			
			if ( (handle=dlopen(fullpath,RTLD_NOW))==NULL){
				ms_warning("Fail to load plugin %s : %s",fullpath,dlerror());
			}else {
				char *initroutine_name=ms_malloc0(strlen(de->d_name)+10);
				char *p;
				void *initroutine=NULL;
				strcpy(initroutine_name,de->d_name);
				p=strstr(initroutine_name,PLUGINS_EXT);
				if (p!=NULL){
					strcpy(p,"_init");
					initroutine=dlsym(handle,initroutine_name);
				}

#ifdef __APPLE__
				if (initroutine==NULL){
					/* on macosx: library name are libxxxx.1.2.3.dylib */
					/* -> MUST remove the .1.2.3 */
					p=strstr(initroutine_name,".");
					if (p!=NULL)
					{
						strcpy(p,"_init");
						initroutine=dlsym(handle,initroutine_name);
					}
				}
#endif

				if (initroutine!=NULL){
					init_func_t func=(init_func_t)initroutine;
					func();
					ms_message("Plugin loaded (%s)", fullpath);
					num++;
				}else{
					ms_warning("Could not locate init routine of plugin %s",de->d_name);
				}
				ms_free(initroutine_name);
			}
			ms_free(fullpath);
		}
	}
	closedir(ds);
#else
	ms_warning("no loadable plugin support: plugins cannot be loaded.");
	num=-1;
#endif
	return num;
}


#ifdef __ALSA_ENABLED__
extern MSSndCardDesc alsa_card_desc;
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
extern MSSndCardDesc oss_card_desc;
#endif

#ifdef __ARTS_ENABLED__
extern MSSndCardDesc arts_card_desc;
#endif

#ifdef WIN32
extern MSSndCardDesc winsnd_card_desc;
#endif

#ifdef __DIRECTSOUND_ENABLED__
extern MSSndCardDesc winsndds_card_desc;
#endif

#ifdef __MACSND_ENABLED__
extern MSSndCardDesc ca_card_desc;
#endif

#ifdef __PORTAUDIO_ENABLED__
extern MSSndCardDesc pasnd_card_desc;
#endif

#ifdef __MAC_AQ_ENABLED__
extern MSSndCardDesc aq_card_desc;
#endif

static MSSndCardDesc * ms_snd_card_descs[]={
#ifdef __ALSA_ENABLED__
	&alsa_card_desc,
#endif
#ifdef HAVE_SYS_SOUNDCARD_H
	&oss_card_desc,
#endif
#ifdef __ARTS_ENABLED__
	&arts_card_desc,
#endif
#ifdef WIN32
	&winsnd_card_desc,
#endif
#ifdef __DIRECTSOUND_ENABLED__
	&winsndds_card_desc,
#endif
#ifdef __PORTAUDIO_ENABLED__
	&pasnd_card_desc,
#endif
#ifdef __MACSND_ENABLED__
	&ca_card_desc,
#endif
#ifdef __MAC_AQ_ENABLED__
	&aq_card_desc,
#endif
	NULL
};

#ifdef VIDEO_ENABLED

#ifdef __linux
extern MSWebCamDesc v4l_desc;
#endif

#ifdef HAVE_LINUX_VIDEODEV2_H
extern MSWebCamDesc v4l2_card_desc;
#endif

#ifdef WIN32
extern MSWebCamDesc ms_vfw_cam_desc;
#endif
#if defined(WIN32) && defined(HAVE_DIRECTSHOW)
extern MSWebCamDesc ms_directx_cam_desc;
#endif

#ifdef __MINGW32__
extern MSWebCamDesc ms_dshow_cam_desc;
#endif

#ifdef __APPLE__
extern MSWebCamDesc ms_v4m_cam_desc;
#endif

extern MSWebCamDesc static_image_desc;
extern MSWebCamDesc mire_desc;

static MSWebCamDesc * ms_web_cam_descs[]={
#ifdef HAVE_LINUX_VIDEODEV2_H
	&v4l2_card_desc,
#endif
#ifdef __linux
	&v4l_desc,
#endif
#if defined(WIN32) && defined(HAVE_DIRECTSHOW)
	&ms_directx_cam_desc,
#endif
#if defined(WIN32) && defined(HAVE_VFW)
	&ms_vfw_cam_desc,
#endif
#ifdef __MINGW32__
	&ms_dshow_cam_desc,
#endif
#ifdef __APPLE__
	&ms_v4m_cam_desc,
#endif

	&mire_desc,
	&static_image_desc,
	NULL
};

#endif

void ms_init(){
	int i;
	MSSndCardManager *cm;

#if !defined(_WIN32_WCE)
	if (getenv("MEDIASTREAMER_DEBUG")!=NULL){
		ortp_set_log_level_mask(ORTP_DEBUG|ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	}
#endif
	ms_message("Registering all filters...");
	/* register builtin MSFilter's */
	for (i=0;ms_filter_descs[i]!=NULL;i++){
		ms_filter_register(ms_filter_descs[i]);
	}
	ms_message("Registering all soundcard handlers");
	cm=ms_snd_card_manager_get();
	for (i=0;ms_snd_card_descs[i]!=NULL;i++){
		ms_snd_card_manager_register_desc(cm,ms_snd_card_descs[i]);
	}

#ifdef VIDEO_ENABLED
	ms_message("Registering all webcam handlers");
	{
		MSWebCamManager *wm;
		wm=ms_web_cam_manager_get();
		for (i=0;ms_web_cam_descs[i]!=NULL;i++){
			ms_web_cam_manager_register_desc(wm,ms_web_cam_descs[i]);
		}
	}
	__register_ffmpeg_encoders_if_possible();
#endif
	ms_message("Loading plugins");
	ms_load_plugins(PACKAGE_PLUGINS_DIR);
	ms_message("ms_init() done");
}

void ms_exit(){
	ms_filter_unregister_all();
	ms_snd_card_manager_destroy();
#ifdef VIDEO_ENABLED
	ms_web_cam_manager_destroy();
#endif
}

void ms_sleep(int seconds){
#ifdef WIN32
	Sleep(seconds*1000);
#else
	struct timespec ts,rem;
	int err;
	ts.tv_sec=seconds;
	ts.tv_nsec=0;
	do {
		err=nanosleep(&ts,&rem);
		ts=rem;
	}while(err==-1 && errno==EINTR);
#endif
}

#define DEFAULT_MAX_PAYLOAD_SIZE 1440

static int max_payload_size=DEFAULT_MAX_PAYLOAD_SIZE;

int ms_get_payload_max_size(){
	return max_payload_size;
}

void ms_set_payload_max_size(int size){
	if (size<=0) size=DEFAULT_MAX_PAYLOAD_SIZE;
	max_payload_size=size;
}
