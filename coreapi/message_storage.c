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

static inline LinphoneChatMessage* get_transient_message(LinphoneChatRoom* cr, unsigned int storage_id){
	MSList* transients = cr->transient_messages;
	LinphoneChatMessage* chat;
	while( transients ){
		chat = (LinphoneChatMessage*)transients->data;
		if(chat->storage_id == storage_id){
			return linphone_chat_message_ref(chat);
		}
		transients = transients->next;
	}
	return NULL;
}

static void create_chat_message(char **argv, void *data){
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	LinphoneAddress *from;

	unsigned int storage_id = atoi(argv[0]);

	// check if the message exists in the transient list, in which case we should return that one.
	LinphoneChatMessage* new_message = get_transient_message(cr, storage_id);
	if( new_message == NULL ){
		new_message = linphone_chat_room_create_message(cr, argv[4]);

		if(atoi(argv[3])==LinphoneChatMessageIncoming){
			new_message->dir=LinphoneChatMessageIncoming;
			from=linphone_address_new(argv[2]);
		} else {
			new_message->dir=LinphoneChatMessageOutgoing;
			from=linphone_address_new(argv[1]);
		}
		linphone_chat_message_set_from(new_message,from);
		linphone_address_destroy(from);

		if( argv[9] != NULL ){
			new_message->time = (time_t)atol(argv[9]);
		} else {
			new_message->time = time(NULL);
		}

		new_message->is_read=atoi(argv[6]);
		new_message->state=atoi(argv[7]);
		new_message->storage_id=storage_id;
		new_message->external_body_url=argv[8]?ms_strdup(argv[8]):NULL;
	}
	cr->messages_hist=ms_list_prepend(cr->messages_hist,new_message);
}

// Called when fetching all conversations from database
static int callback_all(void *data, int argc, char **argv, char **colName){
	LinphoneCore* lc = (LinphoneCore*) data;
	char* address = argv[0];
	linphone_core_get_or_create_chat_room(lc, address);
	return 0;
}

static int callback(void *data, int argc, char **argv, char **colName){
	create_chat_message(argv,data);
	return 0;
}

void linphone_sql_request_message(sqlite3 *db,const char *stmt,LinphoneChatRoom *cr){
	char* errmsg=NULL;
	int ret;
	ret=sqlite3_exec(db,stmt,callback,cr,&errmsg);
	if(ret != SQLITE_OK) {
		ms_error("Error in creation: %s.\n", errmsg);
		sqlite3_free(errmsg);
	}
}

void linphone_sql_request(sqlite3* db,const char *stmt){
	char* errmsg=NULL;
	int ret;
	ret=sqlite3_exec(db,stmt,NULL,NULL,&errmsg);
	if(ret != SQLITE_OK) {
		ms_error("linphone_sql_request: error sqlite3_exec(): %s.\n", errmsg);
		sqlite3_free(errmsg);
	}
}

// Process the request to fetch all chat contacts
void linphone_sql_request_all(sqlite3* db,const char *stmt, LinphoneCore* lc){
	char* errmsg=NULL;
	int ret;
	ret=sqlite3_exec(db,stmt,callback_all,lc,&errmsg);
	if(ret != SQLITE_OK) {
		ms_error("linphone_sql_request_all: error sqlite3_exec(): %s.\n", errmsg);
		sqlite3_free(errmsg);
	}
}

unsigned int linphone_chat_message_store(LinphoneChatMessage *msg){
	LinphoneCore *lc=linphone_chat_room_get_lc(msg->chat_room);
	int id=0;

	if (lc->db){
		char *peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(msg->chat_room));
		char *local_contact=linphone_address_as_string_uri_only(linphone_chat_message_get_local_address(msg));
		char *buf=sqlite3_mprintf("INSERT INTO history VALUES(NULL,%Q,%Q,%i,%Q,%Q,%i,%i,%Q,%i);",
						local_contact,
								  peer,
								  msg->dir,
								  msg->message,
								  "-1", /* use UTC field now */
								  msg->is_read,
								  msg->state,
								  msg->external_body_url,
								  msg->time);
		linphone_sql_request(lc->db,buf);
		sqlite3_free(buf);
		ms_free(local_contact);
		ms_free(peer);
		id = (unsigned int) sqlite3_last_insert_rowid (lc->db);
	}
	return id;
}

void linphone_chat_message_store_state(LinphoneChatMessage *msg){
	LinphoneCore *lc=msg->chat_room->lc;
	if (lc->db){
		char *buf=sqlite3_mprintf("UPDATE history SET status=%i WHERE message = %Q AND utc = %i;",
			msg->state,msg->message,msg->time);
		linphone_sql_request(lc->db,buf);
		sqlite3_free(buf);


	}

	if( msg->state == LinphoneChatMessageStateDelivered
			|| msg->state == LinphoneChatMessageStateNotDelivered ){
		// message is not transient anymore, we can remove it from our transient list:
		msg->chat_room->transient_messages = ms_list_remove(msg->chat_room->transient_messages, msg);
		linphone_chat_message_unref(msg);
	}
}

void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	int read=1;

	if (lc->db==NULL) return ;

	char *peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	char *buf=sqlite3_mprintf("UPDATE history SET read=%i WHERE remoteContact = %Q;",
				   read,peer);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
	ms_free(peer);
}

void linphone_chat_room_update_url(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);

	if (lc->db==NULL) return ;

	char *buf=sqlite3_mprintf("UPDATE history SET url=%Q WHERE id=%i;",msg->external_body_url,msg->storage_id);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
}

int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	int numrows=0;

	if (lc->db==NULL) return 0;

	char *peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	char *buf=sqlite3_mprintf("SELECT count(*) FROM history WHERE remoteContact = %Q AND read = 0;",peer);
	sqlite3_stmt *selectStatement;
	int returnValue = sqlite3_prepare_v2(lc->db,buf,-1,&selectStatement,NULL);
	if (returnValue == SQLITE_OK){
		if(sqlite3_step(selectStatement) == SQLITE_ROW){
			numrows= sqlite3_column_int(selectStatement, 0);
		}
	}
	sqlite3_finalize(selectStatement);
	sqlite3_free(buf);
	ms_free(peer);
	return numrows;
}

void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	LinphoneCore *lc=cr->lc;

	if (lc->db==NULL) return ;

	char *buf=sqlite3_mprintf("DELETE FROM history WHERE id = %i;", msg->storage_id);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
}

void linphone_chat_room_delete_history(LinphoneChatRoom *cr){
	LinphoneCore *lc=cr->lc;

	if (lc->db==NULL) return ;

	char *peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	char *buf=sqlite3_mprintf("DELETE FROM history WHERE remoteContact = %Q;",peer);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
	ms_free(peer);
}

MSList *linphone_chat_room_get_history(LinphoneChatRoom *cr,int nb_message){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	MSList *ret;
	char *buf;
	char *peer;

	if (lc->db==NULL) return NULL;
	peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	cr->messages_hist = NULL;
	if (nb_message > 0)
		buf=sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q ORDER BY id DESC LIMIT %i ;",peer,nb_message);
	else
		buf=sqlite3_mprintf("SELECT * FROM history WHERE remoteContact = %Q ORDER BY id DESC;",peer);
	linphone_sql_request_message(lc->db,buf,cr);
	sqlite3_free(buf);
	ret=cr->messages_hist;
	cr->messages_hist=NULL;
	ms_free(peer);
	return ret;
}

void linphone_close_storage(sqlite3* db){
	sqlite3_close(db);
}

void linphone_create_table(sqlite3* db){
	char* errmsg=NULL;
	int ret;
	ret=sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS history ("
							 "id            INTEGER PRIMARY KEY AUTOINCREMENT,"
							 "localContact  TEXT NOT NULL,"
							 "remoteContact TEXT NOT NULL,"
							 "direction     INTEGER,"
							 "message       TEXT,"
							 "time          TEXT NOT NULL,"
							 "read          INTEGER,"
							 "status        INTEGER"
						");",
			0,0,&errmsg);
	if(ret != SQLITE_OK) {
		ms_error("Error in creation: %s.\n", errmsg);
		sqlite3_free(errmsg);
	}
}


static const char *days[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char *months[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
static time_t parse_time_from_db( const char* time ){
	/* messages used to be stored in the DB by using string-based time */
	struct tm ret={0};
	char tmp1[80]={0};
	char tmp2[80]={0};
	int i,j;
	time_t parsed = 0;

	if( sscanf(time,"%3c %3c%d%d:%d:%d %d",tmp1,tmp2,&ret.tm_mday,
			   &ret.tm_hour,&ret.tm_min,&ret.tm_sec,&ret.tm_year) == 7 ){
		ret.tm_year-=1900;
		for(i=0;i<7;i++) {
			if(strcmp(tmp1,days[i])==0) ret.tm_wday=i;
		}
		for(j=0;j<12;j++) {
			if(strcmp(tmp2,months[j])==0) ret.tm_mon=j;
		}
		ret.tm_isdst=-1;
		parsed = mktime(&ret);
	}
	return parsed;
}


static int migrate_messages(void* data,int argc, char** argv, char** column_names) {
	time_t new_time = parse_time_from_db(argv[1]);
	if( new_time ){
		/* replace 'time' by -1 and set 'utc' to the timestamp */
		char *buf =	sqlite3_mprintf("UPDATE history SET utc=%i,time='-1' WHERE id=%i", new_time, atoi(argv[0]));
		if( buf) {
			linphone_sql_request((sqlite3*)data, buf);
			sqlite3_free(buf);
		}
	} else {
		ms_warning("Cannot parse time %s from id %s", argv[1], argv[0]);
	}
	return 0;
}

static void linphone_migrate_timestamps(sqlite3* db){
	int ret;
	char* errmsg = NULL;

	ret = sqlite3_exec(db,"SELECT id,time,direction FROM history WHERE time != '-1'", migrate_messages, db, &errmsg);
	if( ret != SQLITE_OK ){
		ms_warning("Error migrating outgoing messages: %s.\n", errmsg);
		sqlite3_free(errmsg);
	} else {
		ms_message("Migrated message timestamps to UTC");
	}
}

void linphone_update_table(sqlite3* db) {
	char* errmsg=NULL;
	int ret;

	// for image url storage
	ret=sqlite3_exec(db,"ALTER TABLE history ADD COLUMN url TEXT;",NULL,NULL,&errmsg);
	if(ret != SQLITE_OK) {
		ms_message("Table already up to date: %s.", errmsg);
		sqlite3_free(errmsg);
	} else {
		ms_debug("Table updated successfully for URL.");
	}

	// for UTC timestamp storage
	ret = sqlite3_exec(db, "ALTER TABLE history ADD COLUMN utc INTEGER;", NULL,NULL,&errmsg);
	if( ret != SQLITE_OK ){
		ms_message("Table already up to date: %s.", errmsg);
		sqlite3_free(errmsg);
	} else {
		ms_debug("Table updated successfully for UTC.");
		// migrate from old text-based timestamps to unix time-based timestamps
		linphone_migrate_timestamps(db);
	}
}

void linphone_message_storage_init_chat_rooms(LinphoneCore *lc) {
	char *buf;

	if (lc->db==NULL) return;
	buf=sqlite3_mprintf("SELECT remoteContact FROM history GROUP BY remoteContact;");
	linphone_sql_request_all(lc->db,buf,lc);
	sqlite3_free(buf);
}

void linphone_core_message_storage_init(LinphoneCore *lc){
	int ret;
	const char *errmsg;
	sqlite3 *db;

	linphone_core_message_storage_close(lc);

	ret=sqlite3_open(lc->chat_db_file,&db);
	if(ret != SQLITE_OK) {
		errmsg=sqlite3_errmsg(db);
		ms_error("Error in the opening: %s.\n", errmsg);
		sqlite3_close(db);
	}
	linphone_create_table(db);
	linphone_update_table(db);
	lc->db=db;

	// Create a chatroom for each contact in the chat history
	linphone_message_storage_init_chat_rooms(lc);
}

void linphone_core_message_storage_close(LinphoneCore *lc){
	if (lc->db){
		sqlite3_close(lc->db);
		lc->db=NULL;
	}
}

#else

unsigned int linphone_chat_message_store(LinphoneChatMessage *cr){
	return 0;
}

void linphone_chat_message_store_state(LinphoneChatMessage *cr){
}

void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr){
}

MSList *linphone_chat_room_get_history(LinphoneChatRoom *cr,int nb_message){
	return NULL;
}

void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
}

void linphone_chat_room_delete_history(LinphoneChatRoom *cr){
}

void linphone_message_storage_init_chat_rooms(LinphoneCore *lc) {
}

void linphone_core_message_storage_init(LinphoneCore *lc){
}

void linphone_core_message_storage_close(LinphoneCore *lc){
}

void linphone_chat_room_update_url(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
}

int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr){
	return 0;
}

#endif
