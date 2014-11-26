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

#ifdef MSG_STORAGE_ENABLED
#ifndef PRIu64
#define PRIu64 "I64u"
#endif

#include "sqlite3.h"

static ORTP_INLINE LinphoneChatMessage* get_transient_message(LinphoneChatRoom* cr, unsigned int storage_id){
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

/* DB layout:
 * | 0  | storage_id
 * | 1  | type
 * | 2  | subtype
 * | 3  | name
 * | 4  | encoding
 * | 5  | size
 * | 6  | data
 */
// Callback for sql request when getting linphone content
static int callback_content(void *data, int argc, char **argv, char **colName) {
	LinphoneChatMessage *message = (LinphoneChatMessage *)data;

	if (message->file_transfer_information) {
		linphone_content_unref(message->file_transfer_information);
		message->file_transfer_information = NULL;
	}
	message->file_transfer_information = linphone_content_new();
	if (argv[1]) linphone_content_set_type(message->file_transfer_information, argv[1]);
	if (argv[2]) linphone_content_set_subtype(message->file_transfer_information, argv[2]);
	if (argv[3]) linphone_content_set_name(message->file_transfer_information, argv[3]);
	if (argv[4]) linphone_content_set_encoding(message->file_transfer_information, argv[4]);
	linphone_content_set_size(message->file_transfer_information, (size_t)atoi(argv[5]));

	return 0;
}

static void fetch_content_from_database(sqlite3 *db, LinphoneChatMessage *message, int content_id) {
	char* errmsg = NULL;
	int ret;
	char * buf;

	buf = sqlite3_mprintf("SELECT * FROM content WHERE id = %i", content_id);
	ret = sqlite3_exec(db, buf, callback_content, message, &errmsg);
	if (ret != SQLITE_OK) {
		ms_error("Error in creation: %s.", errmsg);
		sqlite3_free(errmsg);
	}
	sqlite3_free(buf);
}

/* DB layout:
 * | 0  | storage_id
 * | 1  | localContact
 * | 2  | remoteContact
 * | 3  | direction flag
 * | 4  | message
 * | 5  | time (unused now, used to be string-based timestamp)
 * | 6  | read flag
 * | 7  | status
 * | 8  | external body url
 * | 9  | utc timestamp
 * | 10 | app data text
 * | 11 | linphone content
 */
static void create_chat_message(char **argv, void *data){
	LinphoneChatRoom *cr = (LinphoneChatRoom *)data;
	LinphoneAddress *from;
	LinphoneAddress *to;

	unsigned int storage_id = atoi(argv[0]);

	// check if the message exists in the transient list, in which case we should return that one.
	LinphoneChatMessage* new_message = get_transient_message(cr, storage_id);
	if( new_message == NULL ){
		new_message = linphone_chat_room_create_message(cr, argv[4]);

		if(atoi(argv[3])==LinphoneChatMessageIncoming){
			new_message->dir=LinphoneChatMessageIncoming;
			from=linphone_address_new(argv[2]);
			to=linphone_address_new(argv[1]);
		} else {
			new_message->dir=LinphoneChatMessageOutgoing;
			from=linphone_address_new(argv[1]);
			to=linphone_address_new(argv[2]);
		}
		linphone_chat_message_set_from(new_message,from);
		linphone_address_destroy(from);
		if (to){
			linphone_chat_message_set_to(new_message,to);
			linphone_address_destroy(to);
		}

		if( argv[9] != NULL ){
			new_message->time = (time_t)atol(argv[9]);
		} else {
			new_message->time = time(NULL);
		}

		new_message->is_read=atoi(argv[6]);
		new_message->state=atoi(argv[7]);
		new_message->storage_id=storage_id;
		new_message->external_body_url= argv[8] ? ms_strdup(argv[8])  : NULL;
		new_message->appdata          = argv[10]? ms_strdup(argv[10]) : NULL;

		if (argv[11] != NULL) {
			int id = atoi(argv[11]);
			if (id >= 0) {
				fetch_content_from_database(cr->lc->db, new_message, id);
			}
		}
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
		ms_error("Error in creation: %s.", errmsg);
		sqlite3_free(errmsg);
	}
}

int linphone_sql_request(sqlite3* db,const char *stmt){
	char* errmsg=NULL;
	int ret;
	ret=sqlite3_exec(db,stmt,NULL,NULL,&errmsg);
	if(ret != SQLITE_OK) {
		ms_error("linphone_sql_request: statement %s -> error sqlite3_exec(): %s.", stmt, errmsg);
		sqlite3_free(errmsg);
	}
	return ret;
}

// Process the request to fetch all chat contacts
void linphone_sql_request_all(sqlite3* db,const char *stmt, LinphoneCore* lc){
	char* errmsg=NULL;
	int ret;
	ret=sqlite3_exec(db,stmt,callback_all,lc,&errmsg);
	if(ret != SQLITE_OK) {
		ms_error("linphone_sql_request_all: error sqlite3_exec(): %s.", errmsg);
		sqlite3_free(errmsg);
	}
}

static int linphone_chat_message_store_content(LinphoneChatMessage *msg) {
	LinphoneCore *lc = linphone_chat_room_get_lc(msg->chat_room);
	int id = -1;
	if (lc->db) {
		LinphoneContent *content = msg->file_transfer_information;
		char *buf = sqlite3_mprintf("INSERT INTO content VALUES(NULL,%Q,%Q,%Q,%Q,%i,%Q);",
						linphone_content_get_type(content),
						linphone_content_get_subtype(content),
						linphone_content_get_name(content),
						linphone_content_get_encoding(content),
						linphone_content_get_size(content),
						NULL
					);
		linphone_sql_request(lc->db, buf);
		sqlite3_free(buf);
		id = (unsigned int) sqlite3_last_insert_rowid (lc->db);
	}
	return id;
}

unsigned int linphone_chat_message_store(LinphoneChatMessage *msg){
	LinphoneCore *lc=linphone_chat_room_get_lc(msg->chat_room);
	int id = 0;

	if (lc->db){
		int content_id = -1;
		char *peer;
		char *local_contact;
		char *buf;
		if (msg->file_transfer_information) {
			content_id = linphone_chat_message_store_content(msg);
		}

		peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(msg->chat_room));
		local_contact=linphone_address_as_string_uri_only(linphone_chat_message_get_local_address(msg));
		buf = sqlite3_mprintf("INSERT INTO history VALUES(NULL,%Q,%Q,%i,%Q,%Q,%i,%i,%Q,%lld,%Q,%i);",
						local_contact,
						peer,
						msg->dir,
						msg->message,
						"-1", /* use UTC field now */
						msg->is_read,
						msg->state,
						msg->external_body_url,
						(int64_t)msg->time,
						msg->appdata,
						content_id
					);
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
		char *buf=sqlite3_mprintf("UPDATE history SET status=%i WHERE (id = %i);",
								  msg->state,msg->storage_id);
		linphone_sql_request(lc->db,buf);
		sqlite3_free(buf);
	}
}

void linphone_chat_message_store_appdata(LinphoneChatMessage* msg){
	LinphoneCore *lc=msg->chat_room->lc;
	if (lc->db){
		char *buf=sqlite3_mprintf("UPDATE history SET appdata=%Q WHERE id=%i;",
								  msg->appdata,msg->storage_id);
		linphone_sql_request(lc->db,buf);
		sqlite3_free(buf);
	}
}

void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	int read=1;
	char *peer;
	char *buf;

	if (lc->db==NULL) return ;

	peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	buf=sqlite3_mprintf("UPDATE history SET read=%i WHERE remoteContact = %Q;",
				   read,peer);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
	ms_free(peer);
}

void linphone_chat_room_update_url(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	char *buf;

	if (lc->db==NULL) return ;

	buf=sqlite3_mprintf("UPDATE history SET url=%Q WHERE id=%i;",msg->external_body_url,msg->storage_id);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
}

static int linphone_chat_room_get_messages_count(LinphoneChatRoom *cr, bool_t unread_only){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	int numrows=0;
	char *peer;
	char *buf;
	sqlite3_stmt *selectStatement;
	int returnValue;

	if (lc->db==NULL) return 0;

	peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	buf=sqlite3_mprintf("SELECT count(*) FROM history WHERE remoteContact = %Q %s;",peer,unread_only?"AND read = 0":"");
	returnValue = sqlite3_prepare_v2(lc->db,buf,-1,&selectStatement,NULL);
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

int linphone_chat_room_get_unread_messages_count(LinphoneChatRoom *cr){
	return linphone_chat_room_get_messages_count(cr, TRUE);
}

int linphone_chat_room_get_history_size(LinphoneChatRoom *cr){
	return linphone_chat_room_get_messages_count(cr, FALSE);
}

void linphone_chat_room_delete_message(LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	LinphoneCore *lc=cr->lc;
	char *buf;

	if (lc->db==NULL) return ;

	buf=sqlite3_mprintf("DELETE FROM history WHERE id = %i;", msg->storage_id);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
}

void linphone_chat_room_delete_history(LinphoneChatRoom *cr){
	LinphoneCore *lc=cr->lc;
	char *peer;
	char *buf;

	if (lc->db==NULL) return ;

	peer=linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));
	buf=sqlite3_mprintf("DELETE FROM history WHERE remoteContact = %Q;",peer);
	linphone_sql_request(lc->db,buf);
	sqlite3_free(buf);
	ms_free(peer);
}

MSList *linphone_chat_room_get_history_range(LinphoneChatRoom *cr, int startm, int endm){
	LinphoneCore *lc=linphone_chat_room_get_lc(cr);
	MSList *ret;
	char *buf,*buf2;
	char *peer;
	uint64_t begin,end;
	int buf_max_size = 512;

	if (lc->db==NULL) return NULL;
	peer = linphone_address_as_string_uri_only(linphone_chat_room_get_peer_address(cr));

	cr->messages_hist = NULL;

	/*since we want to append query parameters depending on arguments given, we use malloc instead of sqlite3_mprintf*/
	buf=ms_malloc(buf_max_size);
	buf=sqlite3_snprintf(buf_max_size-1,buf,"SELECT * FROM history WHERE remoteContact = %Q ORDER BY id DESC",peer);


	if (startm<0) startm=0;

	if ((endm>0&&endm>=startm) || (startm == 0 && endm == 0) ){
		buf2=ms_strdup_printf("%s LIMIT %i ",buf,endm+1-startm);
		ms_free(buf);
		buf = buf2;
	}else if(startm>0){
		ms_message("%s(): end is lower than start (%d < %d). Assuming no end limit.",__FUNCTION__,endm,startm);
		buf2=ms_strdup_printf("%s LIMIT -1",buf);
		ms_free(buf);
		buf = buf2;
	}

	if (startm>0){
		buf2=ms_strdup_printf("%s OFFSET %i ",buf,startm);
		ms_free(buf);
		buf = buf2;
	}

	begin=ortp_get_cur_time_ms();
	linphone_sql_request_message(lc->db,buf,cr);
	end=ortp_get_cur_time_ms();
	ms_message("%s(): completed in %i ms",__FUNCTION__, (int)(end-begin));
	ms_free(buf);
	ret=cr->messages_hist;
	cr->messages_hist=NULL;
	ms_free(peer);
	return ret;
}

MSList *linphone_chat_room_get_history(LinphoneChatRoom *cr,int nb_message){
	return linphone_chat_room_get_history_range(cr, 0, nb_message-1);
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


static int migrate_messages_timestamp(void* data,int argc, char** argv, char** column_names) {
	time_t new_time = parse_time_from_db(argv[1]);
	if( new_time ){
		/* replace 'time' by -1 and set 'utc' to the timestamp */
		char *buf =	sqlite3_mprintf("UPDATE history SET utc=%lld,time='-1' WHERE id=%i;", (int64_t)new_time, atoi(argv[0]));
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
	uint64_t begin=ortp_get_cur_time_ms();

	linphone_sql_request(db,"BEGIN TRANSACTION");

	ret = sqlite3_exec(db,"SELECT id,time,direction FROM history WHERE time != '-1';", migrate_messages_timestamp, db, &errmsg);
	if( ret != SQLITE_OK ){
		ms_warning("Error migrating outgoing messages: %s.\n", errmsg);
		sqlite3_free(errmsg);
		linphone_sql_request(db, "ROLLBACK");
	} else {
		uint64_t end;
		linphone_sql_request(db, "COMMIT");
		end=ortp_get_cur_time_ms();
		ms_message("Migrated message timestamps to UTC in %lu ms",(unsigned long)(end-begin));
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
		ms_debug("Table history updated successfully for URL.");
	}

	// for UTC timestamp storage
	ret = sqlite3_exec(db, "ALTER TABLE history ADD COLUMN utc INTEGER;", NULL,NULL,&errmsg);
	if( ret != SQLITE_OK ){
		ms_message("Table already up to date: %s.", errmsg);
		sqlite3_free(errmsg);
	} else {
		ms_debug("Table history updated successfully for UTC.");
		// migrate from old text-based timestamps to unix time-based timestamps
		linphone_migrate_timestamps(db);
	}

	// new field for app-specific storage
	ret=sqlite3_exec(db,"ALTER TABLE history ADD COLUMN appdata TEXT;",NULL,NULL,&errmsg);
	if(ret != SQLITE_OK) {
		ms_message("Table already up to date: %s.", errmsg);
		sqlite3_free(errmsg);
	} else {
		ms_debug("Table history updated successfully for app-specific data.");
	}

	// new field for linphone content storage
	ret=sqlite3_exec(db,"ALTER TABLE history ADD COLUMN content INTEGER;",NULL,NULL,&errmsg);
	if(ret != SQLITE_OK) {
		ms_message("Table already up to date: %s.", errmsg);
		sqlite3_free(errmsg);
	} else {
		ms_debug("Table history updated successfully for content data.");
		ret = sqlite3_exec(db,"CREATE TABLE IF NOT EXISTS content ("
							"id INTEGER PRIMARY KEY AUTOINCREMENT,"
							"type TEXT,"
							"subtype TEXT,"
							"name TEXT,"
							"encoding TEXT,"
							"size INTEGER,"
							"data BLOB"
						");",
			0,0,&errmsg);
		if(ret != SQLITE_OK) {
			ms_error("Error in creation: %s.\n", errmsg);
			sqlite3_free(errmsg);
		} else {
			ms_debug("Table content successfully created.");
		}
	}
}

void linphone_message_storage_init_chat_rooms(LinphoneCore *lc) {
	char *buf;

	if (lc->db==NULL) return;
	buf=sqlite3_mprintf("SELECT remoteContact FROM history GROUP BY remoteContact;");
	linphone_sql_request_all(lc->db,buf,lc);
	sqlite3_free(buf);
}

static void _linphone_message_storage_profile(void*data,const char*statement, sqlite3_uint64 duration){
	ms_warning("SQL statement '%s' took %" PRIu64 " microseconds", statement, (uint64_t)(duration / 1000LL) );
}

static void linphone_message_storage_activate_debug(sqlite3* db, bool_t debug){
	if( debug  ){
		sqlite3_profile(db, _linphone_message_storage_profile, NULL );
	} else {
		sqlite3_profile(db, NULL, NULL );
	}
}

void linphone_core_message_storage_set_debug(LinphoneCore *lc, bool_t debug){

	lc->debug_storage = debug;

	if( lc->db ){
		linphone_message_storage_activate_debug(lc->db, debug);
	}
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

	linphone_message_storage_activate_debug(db, lc->debug_storage);

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

void linphone_chat_message_store_appdata(LinphoneChatMessage *msg){
}

void linphone_chat_room_mark_as_read(LinphoneChatRoom *cr){
}

MSList *linphone_chat_room_get_history(LinphoneChatRoom *cr,int nb_message){
	return NULL;
}

LINPHONE_PUBLIC MSList *linphone_chat_room_get_history_range(LinphoneChatRoom *cr, int begin, int end){
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

int linphone_chat_room_get_history_size(LinphoneChatRoom *cr){
	return 0;
}

#endif
