/***************************************************************************
 *            lpconfig.c
 *
 *  Thu Mar 10 11:13:44 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon.morlat@linphone.org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define MAX_LEN 16384

#include "private.h"
#include "bctoolbox/vfs.h"
#include "belle-sip/object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if !defined(_WIN32_WCE)
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if _MSC_VER
#include <io.h>
#endif
#endif /*_WIN32_WCE*/

#ifdef _MSC_VER
#ifdef LINPHONE_WINDOWS_DESKTOP
#include <Shlwapi.h>
#else
#include <stdlib.h>
#endif
#else
#include <libgen.h>
#endif

#ifdef _WIN32
#define RENAME_REQUIRES_NONEXISTENT_NEW_PATH 1
#endif

#define lp_new0(type,n)	(type*)calloc(sizeof(type),n)

#include "linphone/lpconfig.h"
#include "lpc2xml.h"

typedef struct _LpItem{
	char *key;
	char *value;
	int is_comment;
	bool_t overwrite; // If set to true, will add overwrite=true when converted to xml
	bool_t skip; // If set to true, won't be dumped when converted to xml
} LpItem;

typedef struct _LpSectionParam{
	char *key;
	char *value;
} LpSectionParam;

typedef struct _LpSection{
	char *name;
	bctbx_list_t *items;
	bctbx_list_t *params;
	bool_t overwrite; // If set to true, will add overwrite=true to all items of this section when converted to xml
	bool_t skip; // If set to true, won't be dumped when converted to xml
} LpSection;

struct _LpConfig{
	belle_sip_object_t base;
	bctbx_vfs_file_t* pFile;
	char *filename;
	char *tmpfilename;
	bctbx_list_t *sections;
	int modified;
	int readonly;
	bctbx_vfs_t* g_bctbx_vfs;
};

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneConfig);
BELLE_SIP_DECLARE_VPTR(LinphoneConfig);


char* lp_realpath(const char* file, char* name) {
#if defined(_WIN32) || defined(__QNX__) || defined(ANDROID)
	return ms_strdup(file);
#else
	char * output = realpath(file, name);
	char * msoutput = ms_strdup(output);
	free(output);
	return msoutput;
#endif
}

LpItem * lp_item_new(const char *key, const char *value){
	LpItem *item=lp_new0(LpItem,1);
	item->key=ortp_strdup(key);
	item->value=ortp_strdup(value);
	return item;
}

LpItem * lp_comment_new(const char *comment){
	LpItem *item=lp_new0(LpItem,1);
	char* pos = NULL;
	item->value=ortp_strdup(comment);

	pos=strchr(item->value,'\r');
	if (pos==NULL)
		pos=strchr(item->value,'\n');

	if(pos) {
		*pos='\0'; /*replace the '\n' */
	}
	item->is_comment=TRUE;
	return item;
}

LpSectionParam *lp_section_param_new(const char *key, const char *value){
	LpSectionParam *param = lp_new0(LpSectionParam, 1);
	param->key = ortp_strdup(key);
	param->value = ortp_strdup(value);
	return param;
}

LpSection *lp_section_new(const char *name){
	LpSection *sec=lp_new0(LpSection,1);
	sec->name=ortp_strdup(name);
	return sec;
}

void lp_item_destroy(void *pitem){
	LpItem *item=(LpItem*)pitem;
	if (item->key) ortp_free(item->key);
	ortp_free(item->value);
	free(item);
}

void lp_section_param_destroy(void *section_param){
	LpSectionParam *param = (LpSectionParam*)section_param;
	ortp_free(param->key);
	ortp_free(param->value);
	free(param);
}

void lp_section_destroy(LpSection *sec){
	ortp_free(sec->name);
	bctbx_list_for_each(sec->items,lp_item_destroy);
	bctbx_list_for_each(sec->params,lp_section_param_destroy);
	bctbx_list_free(sec->items);
	free(sec);
}

void lp_section_add_item(LpSection *sec,LpItem *item){
	sec->items=bctbx_list_append(sec->items,(void *)item);
}

void linphone_config_add_section(LpConfig *lpconfig, LpSection *section){
	lpconfig->sections=bctbx_list_append(lpconfig->sections,(void *)section);
}

void linphone_config_add_section_param(LpSection *section, LpSectionParam *param){
	section->params = bctbx_list_append(section->params, (void *)param);
}

void linphone_config_remove_section(LpConfig *lpconfig, LpSection *section){
	lpconfig->sections=bctbx_list_remove(lpconfig->sections,(void *)section);
	lp_section_destroy(section);
}

void lp_section_remove_item(LpSection *sec, LpItem *item){
	sec->items=bctbx_list_remove(sec->items,(void *)item);
	lp_item_destroy(item);
}

static bool_t is_first_char(const char *start, const char *pos){
	const char *p;
	for(p=start;p<pos;p++){
		if (*p!=' ') return FALSE;
	}
	return TRUE;
}

static int is_a_comment(const char *str){
	while (*str==' '){
		str++;
	}
	if (*str=='#') return 1;
	return 0;
}

LpSection *linphone_config_find_section(const LpConfig *lpconfig, const char *name){
	LpSection *sec;
	bctbx_list_t *elem;
	/*printf("Looking for section %s\n",name);*/
	for (elem=lpconfig->sections;elem!=NULL;elem=bctbx_list_next(elem)){
		sec=(LpSection*)elem->data;
		if (strcmp(sec->name,name)==0){
			/*printf("Section %s found\n",name);*/
			return sec;
		}
	}
	return NULL;
}

LpSectionParam *lp_section_find_param(const LpSection *sec, const char *key){
	bctbx_list_t *elem;
	LpSectionParam *param;
	for (elem = sec->params; elem != NULL; elem = bctbx_list_next(elem)){
		param = (LpSectionParam*)elem->data;
		if (strcmp(param->key, key) == 0) {
			return param;
		}
	}
	return NULL;
}

LpItem *lp_section_find_comment(const LpSection *sec, const char *comment){
	bctbx_list_t *elem;
	LpItem *item;
	/*printf("Looking for item %s\n",name);*/
	for (elem=sec->items;elem!=NULL;elem=bctbx_list_next(elem)){
		item=(LpItem*)elem->data;
		if (item->is_comment && strcmp(item->value,comment)==0) {
			/*printf("Item %s found\n",name);*/
			return item;
		}
	}
	return NULL;
}

LpItem *lp_section_find_item(const LpSection *sec, const char *name){
	bctbx_list_t *elem;
	LpItem *item;
	/*printf("Looking for item %s\n",name);*/
	for (elem=sec->items;elem!=NULL;elem=bctbx_list_next(elem)){
		item=(LpItem*)elem->data;
		if (!item->is_comment && strcmp(item->key,name)==0) {
			/*printf("Item %s found\n",name);*/
			return item;
		}
	}
	return NULL;
}

static LpSection* linphone_config_parse_line(LpConfig* lpconfig, const char* line, LpSection* cur) {
	LpSectionParam *params = NULL;
	char *pos1,*pos2;
	int nbs;
	size_t size=strlen(line)+1;
	char *secname=ms_malloc(size);
	char *key=ms_malloc(size);
	char *value=ms_malloc(size);
	LpItem *item;

	pos1=strchr(line,'[');
	if (pos1!=NULL && is_first_char(line,pos1) ){
		pos2=strchr(pos1,']');
		if (pos2!=NULL){
			secname[0]='\0';
			/* found section */
			*pos2='\0';
			nbs = sscanf(pos1+1, "%s", secname);
			if (nbs >= 1) {
				if (strlen(secname) > 0) {
					cur = linphone_config_find_section (lpconfig,secname);
					if (cur == NULL) {
						cur = lp_section_new(secname);
						linphone_config_add_section(lpconfig, cur);
					}

					if (pos2 > pos1 + 1 + strlen(secname)) {
						/* found at least one section param */
						pos2 = pos1 + 1 + strlen(secname) + 1; // Remove the white space after the secname
						pos1 = strchr(pos2, '=');
						while (pos1 != NULL) {
							/* for each section param */
							key[0] = '\0';
							value[0] = '\0';
							*pos1 = ' ';
							if (sscanf(pos2, "%s %s", key, value) == 2) {
								params = lp_section_param_new(key, value);
								linphone_config_add_section_param(cur, params);

								pos2 += strlen(key) + strlen(value) + 2; // Remove the = sign + the white space after each param
								pos1 = strchr(pos2, '=');
							} else {
								ms_warning("parse section params error !");
								pos1 = NULL;
							}
						}
					}
				}
			} else {
				ms_warning("parse error!");
			}
		}
	}else {
		if (is_a_comment(line)){
			if (cur){
				LpItem *comment=lp_comment_new(line);
				item=lp_section_find_comment(cur,comment->value);
				if (item!=NULL) {
					lp_section_remove_item(cur, item);
				}
				lp_section_add_item(cur,comment);
			}
		}else{
			pos1=strchr(line,'=');
			if (pos1!=NULL){
				key[0]='\0';

				*pos1='\0';
				if (sscanf(line,"%s",key)>0){

					pos1++;
					pos2=strchr(pos1,'\r');
					if (pos2==NULL)
						pos2=strchr(pos1,'\n');
					if (pos2==NULL) pos2=pos1+strlen(pos1);
					else {
						*pos2='\0'; /*replace the '\n' */
					}
					/* remove ending white spaces */
					for (; pos2>pos1 && pos2[-1]==' ';pos2--) pos2[-1]='\0';

					if (pos2-pos1>0){
						/* found a pair key,value */

						if (cur!=NULL){
							item=lp_section_find_item(cur,key);
							if (item==NULL){
								lp_section_add_item(cur,lp_item_new(key,pos1));
							}else{
								ortp_free(item->value);
								item->value=ortp_strdup(pos1);
							}
							/*ms_message("Found %s=%s",key,pos1);*/
						}else{
							ms_warning("found key,item but no sections");
						}
					}
				}
			}
		}
	}
	ms_free(key);
	ms_free(value);
	ms_free(secname);
	return cur;
}

void linphone_config_parse(LpConfig *lpconfig, bctbx_vfs_file_t* pFile){
	char tmp[MAX_LEN]= {'\0'};
	LpSection* current_section = NULL;
	int size  =0;
	if (pFile==NULL) return;
	while(( size = bctbx_file_get_nxtline(pFile, tmp, MAX_LEN)) > 0){
		//tmp[size] = '\0';
		current_section = linphone_config_parse_line(lpconfig, tmp, current_section);
	}
}

LpConfig * linphone_config_new(const char *filename){
	return linphone_config_new_with_factory(filename, NULL);
}

static void _linphone_config_init_from_buffer(LinphoneConfig *conf, const char *buffer) {
	LpSection* current_section = NULL;

	char* ptr = ms_strdup(buffer);
	char* strtok_storage = NULL;
	char* line = strtok_r(ptr, "\n", &strtok_storage);

	while( line != NULL ){
		current_section = linphone_config_parse_line(conf,line,current_section);
		line = strtok_r(NULL, "\n", &strtok_storage);
	}

	ms_free(ptr);
}

LpConfig * linphone_config_new_from_buffer(const char *buffer){
	LpConfig* conf = belle_sip_object_new(LinphoneConfig);
	_linphone_config_init_from_buffer(conf, buffer);
	return conf;
}

static int _linphone_config_init_from_files(LinphoneConfig *lpconfig, const char *config_filename, const char *factory_config_filename) {
	lpconfig->g_bctbx_vfs = bctbx_vfs_get_default();
	
	if (config_filename!=NULL){
		if(ortp_file_exist(config_filename) == 0) {
			lpconfig->filename=lp_realpath(config_filename, NULL);
			if(lpconfig->filename == NULL) {
				ms_error("Could not find the real path of %s: %s", config_filename, strerror(errno));
				goto fail;
			}
		} else {
			lpconfig->filename = ms_strdup(config_filename);
		}
		lpconfig->tmpfilename=ortp_strdup_printf("%s.tmp",lpconfig->filename);
		ms_message("Using (r/w) config information from %s", lpconfig->filename);

#if !defined(_WIN32)
		{
			struct stat fileStat;
			if ((stat(lpconfig->filename,&fileStat) == 0) && (S_ISREG(fileStat.st_mode))) {
				/* make existing configuration files non-group/world-accessible */
				if (chmod(lpconfig->filename, S_IRUSR | S_IWUSR) == -1) {
					ms_warning("unable to correct permissions on "
						"configuration file: %s", strerror(errno));
				}
			}
		}
#endif /*_WIN32*/

		/*open with r+ to check if we can write on it later*/
		lpconfig->pFile = bctbx_file_open(lpconfig->g_bctbx_vfs,lpconfig->filename, "r+");
#ifdef RENAME_REQUIRES_NONEXISTENT_NEW_PATH
		if (lpconfig->pFile == NULL){
			lpconfig->pFile = bctbx_file_open(lpconfig->g_bctbx_vfs,lpconfig->tmpfilename, "r+");
			if (lpconfig->pFile == NULL){
				ms_warning("Could not open %s but %s works, app may have crashed during last sync.",lpconfig->filename,lpconfig->tmpfilename);
			}
		}
#endif
		if (lpconfig->pFile != NULL){
		    linphone_config_parse(lpconfig, lpconfig->pFile);
			bctbx_file_close(lpconfig->pFile);
			lpconfig->pFile = NULL;
			lpconfig->modified=0;
		}
	}
	if (factory_config_filename != NULL) {
		linphone_config_read_file(lpconfig, factory_config_filename);
	}
	return 0;

fail:
	return -1;
}

LpConfig *linphone_config_new_with_factory(const char *config_filename, const char *factory_config_filename) {
	LpConfig *lpconfig=belle_sip_object_new(LinphoneConfig);
	if (_linphone_config_init_from_files(lpconfig, config_filename, factory_config_filename) == 0) {
		return lpconfig;
	} else {
		ms_free(lpconfig);
		return NULL;
	}
}

int linphone_config_read_file(LpConfig *lpconfig, const char *filename){
	char* path = lp_realpath(filename, NULL);
	bctbx_vfs_file_t* pFile = bctbx_file_open(lpconfig->g_bctbx_vfs, path, "r");
	if (pFile != NULL){
		ms_message("Reading config information from %s", path);
		linphone_config_parse(lpconfig, pFile);
		bctbx_file_close(pFile);
		ms_free(path);
		return 0;
	}
	ms_warning("Fail to open file %s",path);
	ms_free(path);
	return -1;
}

void lp_item_set_value(LpItem *item, const char *value){
	char *prev_value=item->value;
	item->value=ortp_strdup(value);
	ortp_free(prev_value);
}


static void _linphone_config_uninit(LpConfig *lpconfig){
	if (lpconfig->filename!=NULL) ortp_free(lpconfig->filename);
	if (lpconfig->tmpfilename) ortp_free(lpconfig->tmpfilename);
	bctbx_list_for_each(lpconfig->sections,(void (*)(void*))lp_section_destroy);
	bctbx_list_free(lpconfig->sections);
}

LpConfig *linphone_config_ref(LpConfig *lpconfig){
	return (LinphoneConfig *)belle_sip_object_ref(BELLE_SIP_OBJECT(lpconfig));
}

void linphone_config_unref(LpConfig *lpconfig){
	belle_sip_object_unref(BELLE_SIP_OBJECT(lpconfig));
}

void linphone_config_destroy(LpConfig *lpconfig){
	linphone_config_unref(lpconfig);
}

const char *linphone_config_get_section_param_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_value){
	LpSection *sec;
	LpSectionParam *param;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL) {
		param = lp_section_find_param(sec, key);
		if (param != NULL) return param->value;
	}
	return default_value;
}

const char *linphone_config_get_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_string){
	LpSection *sec;
	LpItem *item;
	sec=linphone_config_find_section(lpconfig,section);
	if (sec!=NULL){
		item=lp_section_find_item(sec,key);
		if (item!=NULL) return item->value;
	}
	return default_string;
}

bctbx_list_t * linphone_config_get_string_list(const LpConfig *lpconfig, const char *section, const char *key, bctbx_list_t *default_list) {
	LpItem *item;
	LpSection *sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL) {
		item = lp_section_find_item(sec, key);
		if (item != NULL) {
			bctbx_list_t *l = NULL;
			char *str;
			char *ptr;
			str = ptr = ms_strdup(item->value);
			while (ptr != NULL) {
				char *next = strstr(ptr, ",");
				if (next != NULL) {
					*(next++) = '\0';
				}
				l = bctbx_list_append(l, ms_strdup(ptr));
				ptr = next;
			}
			ms_free(str);
			return l;
		}
	}
	return default_list;
}

bool_t linphone_config_get_range(const LpConfig *lpconfig, const char *section, const char *key, int *min, int *max, int default_min, int default_max) {
	const char *str = linphone_config_get_string(lpconfig, section, key, NULL);
	if (str != NULL) {
		char *minusptr = strchr(str, '-');
		if ((minusptr == NULL) || (minusptr == str)) {
			*min = default_min;
			*max = default_max;
			return FALSE;
		}
		*min = atoi(str);
		*max = atoi(minusptr + 1);
		return TRUE;
	} else {
		*min = default_min;
		*max = default_max;
		return TRUE;
	}
}


int linphone_config_get_int(const LpConfig *lpconfig,const char *section, const char *key, int default_value){
	const char *str=linphone_config_get_string(lpconfig,section,key,NULL);
	if (str!=NULL) {
		int ret=0;

		if (strstr(str,"0x")==str){
			sscanf(str,"%x",&ret);
		}else
			sscanf(str,"%i",&ret);
		return ret;
	}
	else return default_value;
}

int64_t linphone_config_get_int64(const LpConfig *lpconfig,const char *section, const char *key, int64_t default_value){
	const char *str=linphone_config_get_string(lpconfig,section,key,NULL);
	if (str!=NULL) {
#ifdef _WIN32
		return (int64_t)_atoi64(str);
#else
		return atoll(str);
#endif
	}
	else return default_value;
}

float linphone_config_get_float(const LpConfig *lpconfig,const char *section, const char *key, float default_value){
	const char *str=linphone_config_get_string(lpconfig,section,key,NULL);
	float ret=default_value;
	if (str==NULL) return default_value;
	sscanf(str,"%f",&ret);
	return ret;
}

bool_t linphone_config_get_overwrite_flag_for_entry(const LpConfig *lpconfig, const char *section, const char *key) {
	LpSection *sec;
	LpItem *item;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL){
		item = lp_section_find_item(sec, key);
		if (item != NULL) return item->overwrite;
	}
	return FALSE;
}

bool_t linphone_config_get_overwrite_flag_for_section(const LpConfig *lpconfig, const char *section) {
	LpSection *sec;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL){
		return sec->overwrite;
	}
	return FALSE;
}

bool_t linphone_config_get_skip_flag_for_entry(const LpConfig *lpconfig, const char *section, const char *key) {
	LpSection *sec;
	LpItem *item;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL){
		item = lp_section_find_item(sec, key);
		if (item != NULL) return item->skip;
	}
	return FALSE;
}

bool_t linphone_config_get_skip_flag_for_section(const LpConfig *lpconfig, const char *section) {
	LpSection *sec;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL){
		return sec->skip;
	}
	return FALSE;
}

void linphone_config_set_string(LpConfig *lpconfig,const char *section, const char *key, const char *value){
	LpItem *item;
	LpSection *sec=linphone_config_find_section(lpconfig,section);
	if (sec!=NULL){
		item=lp_section_find_item(sec,key);
		if (item!=NULL){
			if (value!=NULL && value[0] != '\0')
				lp_item_set_value(item,value);
			else lp_section_remove_item(sec,item);
		}else{
			if (value!=NULL && value[0] != '\0')
				lp_section_add_item(sec,lp_item_new(key,value));
		}
	}else if (value!=NULL && value[0] != '\0'){
		sec=lp_section_new(section);
		linphone_config_add_section(lpconfig,sec);
		lp_section_add_item(sec,lp_item_new(key,value));
	}
	lpconfig->modified++;
}

void linphone_config_set_string_list(LpConfig *lpconfig, const char *section, const char *key, const bctbx_list_t *value) {
	char *strvalue = NULL;
	char *tmp = NULL;
	const bctbx_list_t *elem;
	for (elem = value; elem != NULL; elem = elem->next) {
		if (strvalue) {
			tmp = ms_strdup_printf("%s,%s", strvalue, (const char *)elem->data);
			ms_free(strvalue);
			strvalue = tmp;
		}
		else strvalue = ms_strdup((const char *)elem->data);
	}
	linphone_config_set_string(lpconfig, section, key, strvalue);
	if (strvalue) ms_free(strvalue);
}

void linphone_config_set_range(LpConfig *lpconfig, const char *section, const char *key, int min_value, int max_value) {
	char tmp[30];
	snprintf(tmp, sizeof(tmp), "%i-%i", min_value, max_value);
	linphone_config_set_string(lpconfig, section, key, tmp);
}

void linphone_config_set_int(LpConfig *lpconfig,const char *section, const char *key, int value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"%i",value);
	linphone_config_set_string(lpconfig,section,key,tmp);
}

void linphone_config_set_int_hex(LpConfig *lpconfig,const char *section, const char *key, int value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"0x%x",value);
	linphone_config_set_string(lpconfig,section,key,tmp);
}

void linphone_config_set_int64(LpConfig *lpconfig,const char *section, const char *key, int64_t value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"%lli",(long long)value);
	linphone_config_set_string(lpconfig,section,key,tmp);
}


void linphone_config_set_float(LpConfig *lpconfig,const char *section, const char *key, float value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"%f",value);
	linphone_config_set_string(lpconfig,section,key,tmp);
}

void linphone_config_set_overwrite_flag_for_entry(LpConfig *lpconfig, const char *section, const char *key, bool_t value) {
	LpSection *sec;
	LpItem *item;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL) {
		item = lp_section_find_item(sec, key);
		if (item != NULL) item->overwrite = value;
	}
}

void linphone_config_set_overwrite_flag_for_section(LpConfig *lpconfig, const char *section, bool_t value) {
	LpSection *sec;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL) {
		sec->overwrite = value;
	}
}

void linphone_config_set_skip_flag_for_entry(LpConfig *lpconfig, const char *section, const char *key, bool_t value) {
	LpSection *sec;
	LpItem *item;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL) {
		item = lp_section_find_item(sec, key);
		if (item != NULL) item->skip = value;
	}
}

void linphone_config_set_skip_flag_for_section(LpConfig *lpconfig, const char *section, bool_t value) {
	LpSection *sec;
	sec = linphone_config_find_section(lpconfig, section);
	if (sec != NULL) {
		sec->skip = value;
	}
}

void lp_item_write(LpItem *item, LpConfig *lpconfig){
	int ret =-1 ;
	if (item->is_comment){	
		ret =bctbx_file_fprintf(lpconfig->pFile, 0, "%s\n",item->value);

	}
	else if (item->value && item->value[0] != '\0' ){
		ret =bctbx_file_fprintf(lpconfig->pFile, 0, "%s=%s\n",item->key,item->value);
	}
	
	else {
		ms_warning("Not writing item %s to file, it is empty", item->key);
	}
	if (ret < 0){
		ms_error("lp_item_write : not writing item to file" );
	}
}

void lp_section_param_write(LpSectionParam *param, LpConfig *lpconfig){
	if( param->value && param->value[0] != '\0') {
		bctbx_file_fprintf(lpconfig->pFile, 0, " %s=%s", param->key, param->value);

	} else {
		ms_warning("Not writing param %s to file, it is empty", param->key);
	}
}

void lp_section_write(LpSection *sec,LpConfig *lpconfig){

	if (bctbx_file_fprintf(lpconfig->pFile, 0, "[%s",sec->name) < 0) ms_error("lp_section_write : write error on %s", sec->name);
	bctbx_list_for_each2(sec->params, (void (*)(void*, void*))lp_section_param_write, (void *)lpconfig);

	if (bctbx_file_fprintf(lpconfig->pFile, 0, "]\n")< 0) ms_error("lp_section_write : write error ");
	bctbx_list_for_each2(sec->items, (void (*)(void*, void*))lp_item_write, (void *)lpconfig);

	if (bctbx_file_fprintf(lpconfig->pFile, 0, "\n")< 0) ms_error("lp_section_write : write error");
	
}

int linphone_config_sync(LpConfig *lpconfig){
	bctbx_vfs_file_t *pFile = NULL;
	if (lpconfig->filename==NULL) return -1;
	if (lpconfig->readonly) return 0;

#ifndef _WIN32
	/* don't create group/world-accessible files */
	(void) umask(S_IRWXG | S_IRWXO);
#endif
	pFile  = bctbx_file_open(lpconfig->g_bctbx_vfs,lpconfig->tmpfilename, "w");
	lpconfig->pFile = pFile;
	if (pFile == NULL){
		ms_warning("Could not write %s ! Maybe it is read-only. Configuration will not be saved.",lpconfig->filename);
		lpconfig->readonly=1;
		return -1;
	}
	
	bctbx_list_for_each2(lpconfig->sections,(void (*)(void *,void*))lp_section_write,(void *)lpconfig);
	bctbx_file_close(pFile);

#ifdef RENAME_REQUIRES_NONEXISTENT_NEW_PATH
	/* On windows, rename() does not accept that the newpath is an existing file, while it is accepted on Unix.
	 * As a result, we are forced to first delete the linphonerc file, and then rename.*/
	if (remove(lpconfig->filename)!=0){
		ms_error("Cannot remove %s: %s",lpconfig->filename, strerror(errno));
	}
#endif
	if (rename(lpconfig->tmpfilename,lpconfig->filename)!=0){
		ms_error("Cannot rename %s into %s: %s",lpconfig->tmpfilename,lpconfig->filename,strerror(errno));
	}
	lpconfig->modified=0;
	return 0;
}

int linphone_config_has_section(const LpConfig *lpconfig, const char *section){
	if (linphone_config_find_section(lpconfig,section)!=NULL) return 1;
	return 0;
}

void linphone_config_for_each_section(const LpConfig *lpconfig, void (*callback)(const char *section, void *ctx), void *ctx) {
	LpSection *sec;
	bctbx_list_t *elem;
	for (elem=lpconfig->sections;elem!=NULL;elem=bctbx_list_next(elem)){
		sec=(LpSection*)elem->data;
		callback(sec->name, ctx);
	}
}

void linphone_config_for_each_entry(const LpConfig *lpconfig, const char *section, void (*callback)(const char *entry, void *ctx), void *ctx) {
	LpItem *item;
	bctbx_list_t *elem;
	LpSection *sec=linphone_config_find_section(lpconfig,section);
	if (sec!=NULL){
		for (elem=sec->items;elem!=NULL;elem=bctbx_list_next(elem)){
			item=(LpItem*)elem->data;
			if (!item->is_comment)
				callback(item->key, ctx);
		}
	}
}

void linphone_config_clean_section(LpConfig *lpconfig, const char *section){
	LpSection *sec=linphone_config_find_section(lpconfig,section);
	if (sec!=NULL){
		linphone_config_remove_section(lpconfig,sec);
	}
	lpconfig->modified++;
}

int linphone_config_needs_commit(const LpConfig *lpconfig){
	return lpconfig->modified>0;
}

static const char *DEFAULT_VALUES_SUFFIX = "_default_values";

int linphone_config_get_default_int(const LpConfig *lpconfig, const char *section, const char *key, int default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return linphone_config_get_int(lpconfig, default_section, key, default_value);
}

int64_t linphone_config_get_default_int64(const LpConfig *lpconfig, const char *section, const char *key, int64_t default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return linphone_config_get_int64(lpconfig, default_section, key, default_value);
}

float linphone_config_get_default_float(const LpConfig *lpconfig, const char *section, const char *key, float default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return linphone_config_get_float(lpconfig, default_section, key, default_value);
}

const char* linphone_config_get_default_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return linphone_config_get_string(lpconfig, default_section, key, default_value);
}

/*
 * WARNING: this function is very dangerous.
 * Read carefuly the folowing notices:
 * 1. The 'path' parameter may be modify by
 *    the function. Be care to keep a copy of
 *    the original string.
 * 2. The return pointer may points on a part of
 *    'path'. So, be care to not free the string
 *    pointed by 'path' before the last used of
 *    the returned pointer.
 * 3. Do not feed it after midnight
 */
static const char *_linphone_config_dirname(char *path) {
#ifdef _MSC_VER
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	static char dirname[_MAX_DRIVE + _MAX_DIR];
	_splitpath(path, drive, dir, fname, ext);
	snprintf(dirname, sizeof(dirname), "%s%s", drive, dir);
	return dirname;
#else
	return dirname(path);
#endif
}

bool_t linphone_config_relative_file_exists(const LpConfig *lpconfig, const char *filename) {
	bctbx_vfs_file_t *pFile;
	if (lpconfig->filename == NULL) {
		return FALSE;
	} else {
		char *conf_path = ms_strdup(lpconfig->filename);
		const char *dir = _linphone_config_dirname(conf_path);
		char *filepath = ms_strdup_printf("%s/%s", dir, filename);
		char *realfilepath = lp_realpath(filepath, NULL);

		ms_free(conf_path);
		ms_free(filepath);

		if(realfilepath == NULL) return FALSE;

		pFile = bctbx_file_open(lpconfig->g_bctbx_vfs,realfilepath, "r");
		ms_free(realfilepath);
		if (pFile != NULL) {
			bctbx_file_close(pFile);
		}
		return pFile != NULL;
	}
}

void linphone_config_write_relative_file(const LpConfig *lpconfig, const char *filename, const char *data) {
	char *dup_config_file = NULL;
	const char *dir = NULL;
	char *filepath = NULL;
	char *realfilepath = NULL;
	bctbx_vfs_file_t *pFile;

	if (lpconfig->filename == NULL) return;

	if(strlen(data) == 0) {
		ms_warning("%s has not been created because there is no data to write", filename);
		return;
	}

	dup_config_file = ms_strdup(lpconfig->filename);
	dir = _linphone_config_dirname(dup_config_file);
	filepath = ms_strdup_printf("%s/%s", dir, filename);
	realfilepath = lp_realpath(filepath, NULL);
	if(realfilepath == NULL) {
		ms_error("Could not resolv %s: %s", filepath, strerror(errno));
		goto end;
	}

	pFile = bctbx_file_open(lpconfig->g_bctbx_vfs,realfilepath, "w");
	if(pFile == NULL) {
		ms_error("Could not open %s for write", realfilepath);
		goto end;
	}
	bctbx_file_fprintf(pFile, 0, "%s",data);
	bctbx_file_close(pFile);

end:
	ms_free(dup_config_file);
	ms_free(filepath);
	if(realfilepath) ms_free(realfilepath);
}

int linphone_config_read_relative_file(const LpConfig *lpconfig, const char *filename, char *data, size_t max_length) {
	char *dup_config_file = NULL;
	const char *dir = NULL;
	char *filepath = NULL;
	bctbx_vfs_file_t* pFile = NULL;

	char* realfilepath = NULL;

	if (lpconfig->filename == NULL) return -1;

	dup_config_file = ms_strdup(lpconfig->filename);
	dir = _linphone_config_dirname(dup_config_file);
	filepath = ms_strdup_printf("%s/%s", dir, filename);
	realfilepath = lp_realpath(filepath, NULL);
	if(realfilepath == NULL) {
		ms_error("Could not resolv %s: %s", filepath, strerror(errno));
		goto err;
	}

	pFile = bctbx_file_open(lpconfig->g_bctbx_vfs,realfilepath,"r");
	if (pFile == NULL) {
		ms_error("Could not open %s for read.", realfilepath);
		goto err;
	}

	if(bctbx_file_read(pFile, data, 1, (off_t)max_length) < 0){
		ms_error("%s could not be loaded.", realfilepath);
		goto err;
		
	}

	bctbx_file_close(pFile);

	ms_free(dup_config_file);
	ms_free(filepath);
	ms_free(realfilepath);
	return 0;

err:
	ms_free(dup_config_file);
	ms_free(filepath);
	if(realfilepath) ms_free(realfilepath);
	return -1;
}

const char** linphone_config_get_sections_names(LpConfig *lpconfig) {
	const char **sections_names;
	const bctbx_list_t *sections = lpconfig->sections;
	size_t ndev;
	int i;

	ndev = bctbx_list_size(sections);
	sections_names = ms_malloc((ndev + 1) * sizeof(const char *));

	for (i = 0; sections != NULL; sections = sections->next, i++) {
		LpSection *section = (LpSection *)sections->data;
		sections_names[i] = ms_strdup(section->name);
	}

	sections_names[ndev] = NULL;
	return sections_names;
}

char* linphone_config_dump_as_xml(const LpConfig *lpconfig) {
	char *buffer;

	lpc2xml_context *ctx = lpc2xml_context_new(NULL, NULL);
	lpc2xml_set_lpc(ctx, lpconfig);
	lpc2xml_convert_string(ctx, &buffer);
	lpc2xml_context_destroy(ctx);

	return buffer;
}

struct _entry_data {
	const LpConfig *conf;
	const char *section;
	char** buffer;
};

static void dump_entry(const char *entry, void *data) {
	struct _entry_data *d = (struct _entry_data *) data;
	const char *value = linphone_config_get_string(d->conf, d->section, entry, "");
	*d->buffer = ms_strcat_printf(*d->buffer, "\t%s=%s\n", entry, value);
}

static void dump_section(const char *section, void *data) {
	struct _entry_data *d = (struct _entry_data *) data;
	d->section = section;
	*d->buffer = ms_strcat_printf(*d->buffer, "[%s]\n", section);
	linphone_config_for_each_entry(d->conf, section, dump_entry, d);
}

char* linphone_config_dump(const LpConfig *lpconfig) {
	char* buffer = NULL;
	struct _entry_data d = { lpconfig, NULL, &buffer };
	linphone_config_for_each_section(lpconfig, dump_section, &d);

	return buffer;
}

void linphone_config_clean_entry(LpConfig *lpconfig, const char *section, const char *key) {
	LpSection *sec;
	LpItem *item;
	sec=linphone_config_find_section(lpconfig,section);
	if (sec!=NULL){
		item=lp_section_find_item(sec,key);
		if (item!=NULL)
			lp_section_remove_item(sec,item);
	}
	return ;
}
int linphone_config_has_entry(const LpConfig *lpconfig, const char *section, const char *key) {
	LpSection *sec;
	sec=linphone_config_find_section(lpconfig,section);
	if (sec!=NULL){
		return lp_section_find_item(sec,key) != NULL;
	} else
		return FALSE;
	
}

BELLE_SIP_INSTANCIATE_VPTR(
	LinphoneConfig,
	belle_sip_object_t,
	_linphone_config_uninit, // uninit
	NULL, // copy
	NULL, // marshal
	FALSE
);
