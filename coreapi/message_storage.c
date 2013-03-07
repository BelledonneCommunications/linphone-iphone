/*
message_storage.c
Copyright (C) 2012  Belledonne Communications, Grenoble, France

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

#include "private.h"
#include "linphonecore.h"

#ifdef WIN32

static inline char *my_ctime_r(const time_t *t, char *buf){
	strcpy(buf,ctime(t));
	return buf;
}

#else
#define my_ctime_r ctime_r
#endif

#ifdef MSG_STORAGE_ENABLED

#include "sqlite3.h"

static const char *days[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

#define CONFIG_FILE ".linphone-history.db"

char *linphone_message_storage_get_config_file(const char *filename){
	const int path_max=1024;
	char *config_file=(char *)malloc(path_max*sizeof(char));
	if (filename==NULL) filename=CONFIG_FILE;
	/*try accessing a local file first if exists*/
	if (access(CONFIG_FILE,F_OK)==0){
		snprintf(config_file,path_max,"%s",filename);
	}else{
#ifdef WIN32
		const char *appdata=getenv("APPDATA");
		if (appdata){
			snprintf(config_file,path_max,"%s\\%s",appdata,LINPHONE_CONFIG_DIR);
			CreateDirectory(config_file,NULL);
			snprintf(config_file,path_max,"%s\\%s\\%s",appdata,LINPHONE_CONFIG_DIR,filename);
		}
#else
		const char *home=getenv("HOME");
		if (home==NULL) home=".";
		snprintf(config_file,path_max,"%s/%s",home,filename);
#endif
	}
	return config_file;
}

void create_chat_message(char **argv, void *data){
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	LinphoneChatMessage* new_message = linphone_chat_room_create_message(cr,argv[4]);
	struct tm ret={0};
	char tmp1[80]={0};
	char tmp2[80]={0};
	
	if(atoi(argv[3])==INCOMING){
		linphone_chat_message_set_from(new_message,linphone_address_new(argv[2]));
	} else {
		linphone_chat_message_set_from(new_message,linphone_address_new(argv[1]));
	}

	if(argv[5]!=NULL){
		int i,j;
		sscanf(argv[5],"%3c %3c%d%d:%d:%d %d",tmp1,tmp2,&ret.tm_mday,
	             &ret.tm_hour,&ret.tm_min,&ret.tm_sec,&ret.tm_year);
		ret.tm_year-=1900;
		for(i=0;i<7;i++) { 
			if(strcmp(tmp1,days[i])==0) ret.tm_wday=i; 
		}
		for(j=0;j<12;j++) { 
			if(strcmp(tmp2,months[j])==0) ret.tm_mon=j; 
		}
	}
	new_message->time=argv[5]!=NULL ? mktime(&ret) : time(NULL);
	new_message->state=atoi(argv[7]);
	cr->messages_hist=ms_list_prepend(cr->messages_hist,(void *)new_message);
}

static int callback(void *data, int argc, char **argv, char **colName){
    create_chat_message(argv,data);
    return 0;
}

void linphone_sql_request_message(sqlite3 *db,const char *stmt,void *data){
	char* errmsg;
	int ret;
	ret=sqlite3_exec(db,stmt,callback,data,&errmsg);
	if(ret != SQLITE_OK) {
		printf("Error in creation: %s.\n", errmsg);
	}
}

void linphone_sql_request(sqlite3* db,const char *stmt){
	char* errmsg;
	int ret;
	ret=sqlite3_exec(db,stmt,0,0,&errmsg);
	if(ret != SQLITE_OK) {
		printf("Error in creation: %s.\n", errmsg);
	}
}

void linphone_core_set_history_message(LinphoneChatRoom *cr,const char *local_contact,const char *remote_contact, 
                     int direction, const char *message,const char *date, int read, int state){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	char *buf=sqlite3_mprintf("insert into history values(NULL,%Q,%Q,%i,%Q,%Q,%i,%i);",
					local_contact,remote_contact,direction,message,date,read,state);
	linphone_sql_request(lc->db,buf);
}

void linphone_core_set_message_state(LinphoneChatRoom *cr,const char *message, int state, time_t date){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	char time_str[26];
	char *buf=sqlite3_mprintf("update history set status=%i where message = %Q and time = %Q;",
	               state,message,my_ctime_r(&date,time_str));
	linphone_sql_request(lc->db,buf);
}

void linphone_core_set_messages_flag_read(LinphoneChatRoom *cr,const char *from, int read){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	char *buf=sqlite3_mprintf("update history set read=%i where remoteContact = %Q;",
	               read,from);
	linphone_sql_request(lc->db,buf);
}

MSList *linphone_chat_room_get_history(const char *to,LinphoneChatRoom *cr,int nb_message){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	cr->messages_hist = NULL;
	char *buf=sqlite3_mprintf("select * from history where remoteContact = %Q order by id DESC limit %i ;",to,nb_message);
	linphone_sql_request_message(lc->db,buf,(void *)cr);
	return cr->messages_hist;
}

void linphone_close_storage(sqlite3* db){
	sqlite3_close(db);
}

void linphone_create_table(sqlite3* db){
	char* errmsg;
	int ret;
	ret=sqlite3_exec(db,"CREATE TABLE if not exists history (id INTEGER PRIMARY KEY AUTOINCREMENT, localContact TEXT NOT NULL, remoteContact TEXT NOT NULL, direction INTEGER, message TEXT, time TEXT NOT NULL, read INTEGER, status INTEGER);",
	        0,0,&errmsg);
	if(ret != SQLITE_OK) {
		printf("Error in creation: %s.\n", errmsg);
	}
}

sqlite3 * linphone_message_storage_init(){
	int ret;
	char *errmsg;
	sqlite3 *db;
	char *filename;
	filename=linphone_message_storage_get_config_file(NULL);
	ret=sqlite3_open(filename,&db);
	if(ret != SQLITE_OK) {
		printf("Error in the opening: %s.\n", errmsg);
		sqlite3_close(db);
	}
	linphone_create_table(db);
	return db;
}
#else 

void linphone_core_set_history_message(LinphoneChatRoom *cr,const char *local_contact,const char *remote_contact, 
                     int direction, const char *message,const char *date, int read, int state){
}

void linphone_core_set_message_state(LinphoneChatRoom *cr,const char *message, int state, time_t date){
}

void linphone_core_set_messages_flag_read(LinphoneChatRoom *cr,const char *from, int read){
}

MSList *linphone_chat_room_get_history(const char *to,LinphoneChatRoom *cr,int nb_message){
	return NULL;
}
#endif