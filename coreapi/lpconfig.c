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

#include "linphonecore.h"

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
#ifdef WINAPI_FAMILY_PHONE_APP
#include <stdlib.h>
#else
#include <Shlwapi.h>
#endif
#else
#include <libgen.h>
#endif

#ifdef WIN32
#define RENAME_REQUIRES_NONEXISTENT_NEW_PATH 1
#endif

#define lp_new0(type,n)	(type*)calloc(sizeof(type),n)

#include "lpconfig.h"


typedef struct _LpItem{
	char *key;
	char *value;
	int is_comment;
} LpItem;

typedef struct _LpSectionParam{
	char *key;
	char *value;
} LpSectionParam;

typedef struct _LpSection{
	char *name;
	MSList *items;
	MSList *params;
} LpSection;

struct _LpConfig{
	int refcnt;
	FILE *file;
	char *filename;
	char *tmpfilename;
	MSList *sections;
	int modified;
	int readonly;
};

LpItem * lp_item_new(const char *key, const char *value){
	LpItem *item=lp_new0(LpItem,1);
	item->key=ortp_strdup(key);
	item->value=ortp_strdup(value);
	return item;
}

LpItem * lp_comment_new(const char *comment){
	LpItem *item=lp_new0(LpItem,1);
	item->value=ortp_strdup(comment);
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
	ms_list_for_each(sec->items,lp_item_destroy);
	ms_list_for_each(sec->params,lp_section_param_destroy);
	ms_list_free(sec->items);
	free(sec);
}

void lp_section_add_item(LpSection *sec,LpItem *item){
	sec->items=ms_list_append(sec->items,(void *)item);
}

void lp_config_add_section(LpConfig *lpconfig, LpSection *section){
	lpconfig->sections=ms_list_append(lpconfig->sections,(void *)section);
}

void lp_config_add_section_param(LpSection *section, LpSectionParam *param){
	section->params = ms_list_append(section->params, (void *)param);
}

void lp_config_remove_section(LpConfig *lpconfig, LpSection *section){
	lpconfig->sections=ms_list_remove(lpconfig->sections,(void *)section);
	lp_section_destroy(section);
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

LpSection *lp_config_find_section(const LpConfig *lpconfig, const char *name){
	LpSection *sec;
	MSList *elem;
	/*printf("Looking for section %s\n",name);*/
	for (elem=lpconfig->sections;elem!=NULL;elem=ms_list_next(elem)){
		sec=(LpSection*)elem->data;
		if (strcmp(sec->name,name)==0){
			/*printf("Section %s found\n",name);*/
			return sec;
		}
	}
	return NULL;
}

LpSectionParam *lp_section_find_param(const LpSection *sec, const char *key){
	MSList *elem;
	LpSectionParam *param;
	for (elem = sec->params; elem != NULL; elem = ms_list_next(elem)){
		param = (LpSectionParam*)elem->data;
		if (strcmp(param->key, key) == 0) {
			return param;
		}
	}
	return NULL;
}

LpItem *lp_section_find_item(const LpSection *sec, const char *name){
	MSList *elem;
	LpItem *item;
	/*printf("Looking for item %s\n",name);*/
	for (elem=sec->items;elem!=NULL;elem=ms_list_next(elem)){
		item=(LpItem*)elem->data;
		if (!item->is_comment && strcmp(item->key,name)==0) {
			/*printf("Item %s found\n",name);*/
			return item;
		}
	}
	return NULL;
}

static LpSection* lp_config_parse_line(LpConfig* lpconfig, const char* line, LpSection* cur) {
	LpSectionParam *params = NULL;
	char *pos1,*pos2;
	int nbs;
	int size=strlen(line)+1;
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
					cur = lp_config_find_section (lpconfig,secname);
					if (cur == NULL) {
						cur = lp_section_new(secname);
						lp_config_add_section(lpconfig, cur);
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
								lp_config_add_section_param(cur, params);

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

void lp_config_parse(LpConfig *lpconfig, FILE *file){
	char tmp[MAX_LEN]= {'\0'};
	LpSection* current_section = NULL;

	if (file==NULL) return;

	while(fgets(tmp,MAX_LEN,file)!=NULL){
		tmp[sizeof(tmp) -1] = '\0';
		current_section = lp_config_parse_line(lpconfig, tmp, current_section);
	}
}

LpConfig * lp_config_new(const char *filename){
	return lp_config_new_with_factory(filename, NULL);
}

LpConfig * lp_config_new_from_buffer(const char *buffer){
	LpConfig* conf = lp_new0(LpConfig,1);
	LpSection* current_section = NULL;

	char* ptr = ms_strdup(buffer);
	char* strtok_storage = NULL;
	char* line = strtok_r(ptr, "\n", &strtok_storage);

	conf->refcnt=1;

	while( line != NULL ){
		current_section = lp_config_parse_line(conf,line,current_section);
		line = strtok_r(NULL, "\n", &strtok_storage);
	}

	ms_free(ptr);

	return conf;
}

LpConfig *lp_config_new_with_factory(const char *config_filename, const char *factory_config_filename) {
	LpConfig *lpconfig=lp_new0(LpConfig,1);
	lpconfig->refcnt=1;
	if (config_filename!=NULL){
		ms_message("Using (r/w) config information from %s", config_filename);
		lpconfig->filename=ortp_strdup(config_filename);
		lpconfig->tmpfilename=ortp_strdup_printf("%s.tmp",config_filename);
		
#if !defined(WIN32)
		{
			struct stat fileStat;
			if ((stat(config_filename,&fileStat) == 0) && (S_ISREG(fileStat.st_mode))) {
				/* make existing configuration files non-group/world-accessible */
				if (chmod(config_filename, S_IRUSR | S_IWUSR) == -1) {
					ms_warning("unable to correct permissions on "
						"configuration file: %s", strerror(errno));
				}
			}
		}
#endif /*WIN32*/
		/*open with r+ to check if we can write on it later*/
		lpconfig->file=fopen(lpconfig->filename,"r+");
#ifdef RENAME_REQUIRES_NONEXISTENT_NEW_PATH
		if (lpconfig->file==NULL){
			lpconfig->file=fopen(lpconfig->tmpfilename,"r+");
			if (lpconfig->file){
				ms_warning("Could not open %s but %s works, app may have crashed during last sync.",lpconfig->filename,lpconfig->tmpfilename);
			}
		}
#endif
		if (lpconfig->file!=NULL){
			lp_config_parse(lpconfig,lpconfig->file);
			fclose(lpconfig->file);

			lpconfig->file=NULL;
			lpconfig->modified=0;
		}
	}
	if (factory_config_filename != NULL) {
		lp_config_read_file(lpconfig, factory_config_filename);
	}
	return lpconfig;
}

int lp_config_read_file(LpConfig *lpconfig, const char *filename){
	FILE* f=fopen(filename,"r");
	if (f!=NULL){
		ms_message("Reading config information from %s", filename);
		lp_config_parse(lpconfig,f);
		fclose(f);
		return 0;
	}
	ms_warning("Fail to open file %s",filename);
	return -1;
}

void lp_item_set_value(LpItem *item, const char *value){
	char *prev_value=item->value;
	item->value=ortp_strdup(value);
	ortp_free(prev_value);
}


static void _lp_config_destroy(LpConfig *lpconfig){
	if (lpconfig->filename!=NULL) ortp_free(lpconfig->filename);
	if (lpconfig->tmpfilename) ortp_free(lpconfig->tmpfilename);
	ms_list_for_each(lpconfig->sections,(void (*)(void*))lp_section_destroy);
	ms_list_free(lpconfig->sections);
	free(lpconfig);
}

LpConfig *lp_config_ref(LpConfig *lpconfig){
	lpconfig->refcnt++;
	return lpconfig;
}

void lp_config_unref(LpConfig *lpconfig){
	lpconfig->refcnt--;
	if (lpconfig->refcnt==0)
		_lp_config_destroy(lpconfig);
}

void lp_config_destroy(LpConfig *lpconfig){
	lp_config_unref(lpconfig);
}

void lp_section_remove_item(LpSection *sec, LpItem *item){
	sec->items=ms_list_remove(sec->items,(void *)item);
	lp_item_destroy(item);
}

const char *lp_config_get_section_param_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_value){
	LpSection *sec;
	LpSectionParam *param;
	sec = lp_config_find_section(lpconfig, section);
	if (sec != NULL) {
		param = lp_section_find_param(sec, key);
		if (param != NULL) return param->value;
	}
	return default_value;
}

const char *lp_config_get_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_string){
	LpSection *sec;
	LpItem *item;
	sec=lp_config_find_section(lpconfig,section);
	if (sec!=NULL){
		item=lp_section_find_item(sec,key);
		if (item!=NULL) return item->value;
	}
	return default_string;
}

bool_t lp_config_get_range(const LpConfig *lpconfig, const char *section, const char *key, int *min, int *max, int default_min, int default_max) {
	const char *str = lp_config_get_string(lpconfig, section, key, NULL);
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


int lp_config_get_int(const LpConfig *lpconfig,const char *section, const char *key, int default_value){
	const char *str=lp_config_get_string(lpconfig,section,key,NULL);
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

int64_t lp_config_get_int64(const LpConfig *lpconfig,const char *section, const char *key, int64_t default_value){
	const char *str=lp_config_get_string(lpconfig,section,key,NULL);
	if (str!=NULL) {
#ifdef WIN32
		return (int64_t)_atoi64(str);
#else
		return atoll(str);
#endif
	}
	else return default_value;
}

float lp_config_get_float(const LpConfig *lpconfig,const char *section, const char *key, float default_value){
	const char *str=lp_config_get_string(lpconfig,section,key,NULL);
	float ret=default_value;
	if (str==NULL) return default_value;
	sscanf(str,"%f",&ret);
	return ret;
}

void lp_config_set_string(LpConfig *lpconfig,const char *section, const char *key, const char *value){
	LpItem *item;
	LpSection *sec=lp_config_find_section(lpconfig,section);
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
		lp_config_add_section(lpconfig,sec);
		lp_section_add_item(sec,lp_item_new(key,value));
	}
	lpconfig->modified++;
}

void lp_config_set_range(LpConfig *lpconfig, const char *section, const char *key, int min_value, int max_value) {
	char tmp[30];
	snprintf(tmp, sizeof(tmp), "%i-%i", min_value, max_value);
	lp_config_set_string(lpconfig, section, key, tmp);
}

void lp_config_set_int(LpConfig *lpconfig,const char *section, const char *key, int value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"%i",value);
	lp_config_set_string(lpconfig,section,key,tmp);
}

void lp_config_set_int_hex(LpConfig *lpconfig,const char *section, const char *key, int value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"0x%x",value);
	lp_config_set_string(lpconfig,section,key,tmp);
}

void lp_config_set_int64(LpConfig *lpconfig,const char *section, const char *key, int64_t value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"%lli",(long long)value);
	lp_config_set_string(lpconfig,section,key,tmp);
}


void lp_config_set_float(LpConfig *lpconfig,const char *section, const char *key, float value){
	char tmp[30];
	snprintf(tmp,sizeof(tmp),"%f",value);
	lp_config_set_string(lpconfig,section,key,tmp);
}

void lp_item_write(LpItem *item, FILE *file){
	if (item->is_comment)
		fprintf(file,"%s",item->value);
	else if (item->value && item->value[0] != '\0' )
		fprintf(file,"%s=%s\n",item->key,item->value);
	else {
		ms_warning("Not writing item %s to file, it is empty", item->key);
	}
}

void lp_section_param_write(LpSectionParam *param, FILE *file){
	if( param->value && param->value[0] != '\0') {
		fprintf(file, " %s=%s", param->key, param->value);
	} else {
		ms_warning("Not writing param %s to file, it is empty", param->key);
	}
}

void lp_section_write(LpSection *sec, FILE *file){
	fprintf(file, "[%s",sec->name);
	ms_list_for_each2(sec->params, (void (*)(void*, void*))lp_section_param_write, (void *)file);
	fprintf(file, "]\n");
	ms_list_for_each2(sec->items, (void (*)(void*, void*))lp_item_write, (void *)file);
	fprintf(file, "\n");
}

int lp_config_sync(LpConfig *lpconfig){
	FILE *file;
	if (lpconfig->filename==NULL) return -1;
	if (lpconfig->readonly) return 0;
#ifndef WIN32
	/* don't create group/world-accessible files */
	(void) umask(S_IRWXG | S_IRWXO);
#endif
	file=fopen(lpconfig->tmpfilename,"w");
	if (file==NULL){
		ms_warning("Could not write %s ! Maybe it is read-only. Configuration will not be saved.",lpconfig->filename);
		lpconfig->readonly=1;
		return -1;
	}
	ms_list_for_each2(lpconfig->sections,(void (*)(void *,void*))lp_section_write,(void *)file);
	fclose(file);
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

int lp_config_has_section(const LpConfig *lpconfig, const char *section){
	if (lp_config_find_section(lpconfig,section)!=NULL) return 1;
	return 0;
}

void lp_config_for_each_section(const LpConfig *lpconfig, void (*callback)(const char *section, void *ctx), void *ctx) {
	LpSection *sec;
	MSList *elem;
	for (elem=lpconfig->sections;elem!=NULL;elem=ms_list_next(elem)){
		sec=(LpSection*)elem->data;
		callback(sec->name, ctx);
	}
}

void lp_config_for_each_entry(const LpConfig *lpconfig, const char *section, void (*callback)(const char *entry, void *ctx), void *ctx) {
	LpItem *item;
	MSList *elem;
	LpSection *sec=lp_config_find_section(lpconfig,section);
	if (sec!=NULL){
		for (elem=sec->items;elem!=NULL;elem=ms_list_next(elem)){
			item=(LpItem*)elem->data;
			if (!item->is_comment)
				callback(item->key, ctx);
		}
	}
}

void lp_config_clean_section(LpConfig *lpconfig, const char *section){
	LpSection *sec=lp_config_find_section(lpconfig,section);
	if (sec!=NULL){
		lp_config_remove_section(lpconfig,sec);
	}
	lpconfig->modified++;
}

int lp_config_needs_commit(const LpConfig *lpconfig){
	return lpconfig->modified>0;
}

static const char *DEFAULT_VALUES_SUFFIX = "_default_values";

int lp_config_get_default_int(const LpConfig *lpconfig, const char *section, const char *key, int default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return lp_config_get_int(lpconfig, default_section, key, default_value);
}

int64_t lp_config_get_default_int64(const LpConfig *lpconfig, const char *section, const char *key, int64_t default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return lp_config_get_int64(lpconfig, default_section, key, default_value);
}

float lp_config_get_default_float(const LpConfig *lpconfig, const char *section, const char *key, float default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return lp_config_get_float(lpconfig, default_section, key, default_value);
}

const char* lp_config_get_default_string(const LpConfig *lpconfig, const char *section, const char *key, const char *default_value) {
	char default_section[MAX_LEN];
	strcpy(default_section, section);
	strcat(default_section, DEFAULT_VALUES_SUFFIX);

	return lp_config_get_string(lpconfig, default_section, key, default_value);
}

static char *_lp_config_dirname(char *path) {
#ifdef _MSC_VER
#ifdef WINAPI_FAMILY_PHONE_APP
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath(path, drive, dir, fname, ext);
	return ms_strdup_printf("%s%s", drive, dir);
#else
	char *dir = ms_strdup(path);
	PathRemoveFileSpec(dir);
	return dir;
#endif
#else
	char *tmp = ms_strdup(path);
	char *dir = ms_strdup(dirname(tmp));
	ms_free(tmp);
	return dir;
#endif
}

void lp_config_write_relative_file(const LpConfig *lpconfig, const char *filename, const char *data) {
	if(strlen(data) > 0) {
		char *dir = _lp_config_dirname(lpconfig->filename);
		char *filepath = ms_strdup_printf("%s/%s", dir, filename);
		FILE *file = fopen(filepath, "w");
		if(file != NULL) {
			fprintf(file, "%s", data);
			fclose(file);
		} else {
			ms_error("Could not open %s for write", filepath);
		}
		ms_free(dir);
		ms_free(filepath);
	} else {
		ms_warning("%s has not been created because there is no data to write", filename);
	}
}

int lp_config_read_relative_file(const LpConfig *lpconfig, const char *filename, char *data, size_t max_length) {
	char *dir = _lp_config_dirname(lpconfig->filename);
	char *filepath = ms_strdup_printf("%s/%s", dir, filename);
	FILE *file = fopen(filepath, "r");
	if(file != NULL) {
		if(fgets(data, max_length, file) == NULL) {
			ms_error("%s could not be loaded. %s", filepath, strerror(errno));
			goto err;
		}
		fclose(file);
	} else {
		ms_error("Could not open %s for read. %s", filepath, strerror(errno));
		goto err;
	}
	ms_free(dir);
	ms_free(filepath);
	return 0;
	
err:
	ms_free(dir);
	ms_free(filepath);
	return -1;
}
