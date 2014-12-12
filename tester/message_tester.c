/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2013  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"

#ifdef MSG_STORAGE_ENABLED
#include <sqlite3.h>
#endif


static char* message_external_body_url=NULL;

void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message) {
	stats* counters = get_stats(lc);
	counters->number_of_LinphoneMessageReceivedLegacy++;
}

void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage* message) {
	char* from=linphone_address_as_string(linphone_chat_message_get_from(message));
	stats* counters;
	const char *text=linphone_chat_message_get_text(message);
	const char *external_body_url=linphone_chat_message_get_external_body_url(message);
	ms_message("Message from [%s]  is [%s] , external URL [%s]",from?from:""
																,text?text:""
																,external_body_url?external_body_url:"");
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_LinphoneMessageReceived++;
	if (counters->last_received_chat_message) linphone_chat_message_unref(counters->last_received_chat_message);
	counters->last_received_chat_message=linphone_chat_message_ref(message);
	if (linphone_chat_message_get_file_transfer_information(message)) {
		counters->number_of_LinphoneMessageReceivedWithFile++;
	} else if (linphone_chat_message_get_external_body_url(message)) {
		counters->number_of_LinphoneMessageExtBodyReceived++;
		if (message_external_body_url) {
			CU_ASSERT_STRING_EQUAL(linphone_chat_message_get_external_body_url(message),message_external_body_url);
			message_external_body_url=NULL;
		}
	}
}

/**
 * function invoked when a file transfer is received.
 * */
void file_transfer_received(LinphoneChatMessage *message, const LinphoneContent* content, const LinphoneBuffer *buffer){
	FILE* file=NULL;
	char receive_file[256];
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(message);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	snprintf(receive_file,sizeof(receive_file), "%s/receive_file.dump", liblinphone_tester_writable_dir_prefix);
	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk, creating file*/
		file = fopen(receive_file,"wb");
		linphone_chat_message_set_user_data(message,(void*)file); /*store fd for next chunks*/
	}

	file = (FILE*)linphone_chat_message_get_user_data(message);

	if (linphone_buffer_is_empty(buffer)) { /* tranfer complete */
		stats* counters = get_stats(lc);
		counters->number_of_LinphoneMessageExtBodyReceived++;
		fclose(file);
	} else { /* store content on a file*/
		if (fwrite(linphone_buffer_get_content(buffer),linphone_buffer_get_size(buffer),1,file)==-1){
			ms_error("file_transfer_received(): write() failed: %s",strerror(errno));
		}
	}
}

static char big_file [128000]; /* a buffer to simulate a big file for the file transfer message test */

/*
 * function called when the file transfer is initiated. file content should be feed into object LinphoneContent
 * */
LinphoneBuffer * file_transfer_send(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t size){
	LinphoneBuffer *lb;
	size_t file_size;
	size_t size_to_send;
	FILE *file_to_send;
	uint8_t *buf;
	if (size == 0) return linphone_buffer_new(); /*end of file*/
	file_to_send = linphone_chat_message_get_user_data(message);
	fseek(file_to_send, 0, SEEK_END);
	file_size = ftell(file_to_send);
	fseek(file_to_send, offset, SEEK_SET);
	size_to_send = MIN(size, file_size - offset);
	buf = ms_malloc(size_to_send);
	if (fread(buf, size_to_send, 1, file_to_send)!=size_to_send){
		ms_error("fread error");
	}
	lb = linphone_buffer_new_from_data(buf, size_to_send);
	ms_free(buf);
	return lb;
}

LinphoneBuffer * memory_file_transfer_send(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t size){
	size_t size_to_send = MIN(size, sizeof(big_file) - offset);
	if (size == 0) return linphone_buffer_new(); /*end of file*/
	return linphone_buffer_new_from_data((uint8_t *)big_file + offset, size_to_send);
}

/**
 * function invoked to report file transfer progress.
 * */
void file_transfer_progress_indication(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(message);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	const LinphoneAddress* from_address = linphone_chat_message_get_from(message);
	const LinphoneAddress* to_address = linphone_chat_message_get_to(message);
	char *address = linphone_chat_message_is_outgoing(message)?linphone_address_as_string(to_address):linphone_address_as_string(from_address);
	stats* counters = get_stats(lc);
	int progress = (int)((offset * 100)/total);
	ms_message(" File transfer  [%d%%] %s of type [%s/%s] %s [%s] \n", progress
																	,(linphone_chat_message_is_outgoing(message)?"sent":"received")
																	, linphone_content_get_type(content)
																	, linphone_content_get_subtype(content)
																	,(linphone_chat_message_is_outgoing(message)?"to":"from")
																	, address);
	counters->progress_of_LinphoneFileTransfer = progress;
	free(address);
}

void is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
	stats *counters = get_stats(lc);
	if (room->remote_is_composing == LinphoneIsComposingActive) {
		counters->number_of_LinphoneIsComposingActiveReceived++;
	} else {
		counters->number_of_LinphoneIsComposingIdleReceived++;
	}
}

void liblinphone_tester_chat_message_state_change(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud) {
	liblinphone_tester_chat_message_msg_state_changed(msg, state);
}

void liblinphone_tester_chat_message_msg_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	stats* counters = get_stats(lc);
	ms_message("Message [%s] [%s]",linphone_chat_message_get_text(msg), linphone_chat_message_state_to_string(state));
	switch (state) {
	case LinphoneChatMessageStateDelivered:
		counters->number_of_LinphoneMessageDelivered++;
		break;
	case LinphoneChatMessageStateNotDelivered:
		counters->number_of_LinphoneMessageNotDelivered++;
		break;
	case LinphoneChatMessageStateInProgress:
		counters->number_of_LinphoneMessageInProgress++;
		break;
	case LinphoneChatMessageStateFileTransferError:
		counters->number_of_LinphoneMessageNotDelivered++;
		break;
	default:
		ms_error("Unexpected state [%s] for message [%p]",linphone_chat_message_state_to_string(state), msg);
	}
}

static void text_message(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_within_dialog(void) {
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	lp_config_set_int(pauline->lc->config,"sip","chat_use_call_dialogs",1);

	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	CU_ASSERT_TRUE(call(marie,pauline));

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static LinphoneAuthInfo* text_message_with_credential_from_auth_cb_auth_info;
static void text_message_with_credential_from_auth_cb_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
	ms_message("text_message_with_credential_from_auth_cb:Auth info requested  for user id [%s] at realm [%s]\n"
						,username
						,realm);
	linphone_core_add_auth_info(lc,text_message_with_credential_from_auth_cb_auth_info); /*add stored authentication info to LinphoneCore*/
}


static void text_message_with_credential_from_auth_cb(void) {
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneCoreVTable* vtable = linphone_core_v_table_new();
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	text_message_with_credential_from_auth_cb_auth_info=linphone_auth_info_clone((LinphoneAuthInfo*)(linphone_core_get_auth_info_list(marie->lc)->data));

	/*to force cb to be called*/
	linphone_core_clear_all_auth_info(marie->lc);
	vtable->auth_info_requested=text_message_with_credential_from_auth_cb_auth_info_requested;
	linphone_core_add_listener(marie->lc, vtable);

	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_privacy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneProxyConfig* pauline_proxy;
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	/*test proxy config privacy*/
	linphone_core_get_default_proxy(pauline->lc,&pauline_proxy);
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_compatibility_mode(void) {
	char route[256];
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneProxyConfig* proxy;
	LinphoneAddress* proxy_address;
	char*tmp;
	LCSipTransports transport;
	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room;

	linphone_core_get_default_proxy(marie->lc,&proxy);
	CU_ASSERT_PTR_NOT_NULL (proxy);
	proxy_address=linphone_address_new(linphone_proxy_config_get_addr(proxy));
	linphone_address_clean(proxy_address);
	tmp=linphone_address_as_string_uri_only(proxy_address);
	linphone_proxy_config_set_server_addr(proxy,tmp);
	sprintf(route,"sip:%s",test_route);
	linphone_proxy_config_set_route(proxy,route);
	ms_free(tmp);
	linphone_address_destroy(proxy_address);
	linphone_core_get_sip_transports(marie->lc,&transport);
	transport.udp_port=0;
	transport.tls_port=0;
	transport.dtls_port=0;
	/*only keep tcp*/
	linphone_core_set_sip_transports(marie->lc,&transport);
	marie->stat.number_of_LinphoneRegistrationOk=0;

	CU_ASSERT_TRUE (wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphoneRegistrationOk,1));

	chat_room = linphone_core_create_chat_room(marie->lc,to);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceivedLegacy,1);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_ack(void) {
	int leaked_objects;
	int begin;
	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	{
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
		char* to = linphone_address_as_string(marie->identity);
		LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
		LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_room_send_chat_message(chat_room,message);
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
		CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);

		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline);
	}
	leaked_objects=belle_sip_object_get_object_count()-begin;
	CU_ASSERT_TRUE(leaked_objects==0);
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}

static void text_message_with_external_body(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);
	linphone_chat_message_set_external_body_url(message,message_external_body_url="http://www.linphone.org");
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,message);

	/* check transient message list: the message should be in it, and should be the only one */
	CU_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 1);
	CU_ASSERT_EQUAL(ms_list_nth_data(chat_room->transient_messages,0), message);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);

	CU_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 0);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static bool_t compare_files(const char *path1, const char *path2) {
	bool_t res;
	size_t size1;
	size_t size2;
	uint8_t *buf1;
	uint8_t *buf2;
	FILE *f1 = fopen(path1, "rb");
	FILE *f2 = fopen(path2, "rb");
	fseek(f1, 0, SEEK_END);
	size1 = ftell(f1);
	fseek(f1, 0, SEEK_SET);
	fseek(f2, 0, SEEK_END);
	size2 = ftell(f2);
	fseek(f2, 0, SEEK_SET);
	if (size1 != size2) {
		fclose(f1);
		fclose(f2);
		return FALSE;
	}
	buf1 = ms_malloc(size1);
	buf2 = ms_malloc(size2);
	if (fread(buf1, size1, 1, f1)!=size1){
		ms_error("fread() error");
	}
	if (fread(buf2, size2, 1, f2)!=size2){
		ms_error("fread() error");
	}
	fclose(f1);
	fclose(f2);
	res = (memcmp(buf1, buf2, size1) == 0) ? TRUE : FALSE;
	ms_free(buf1);
	ms_free(buf2);
	return res;
}

static void file_transfer_message(void) {
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* message;
	LinphoneChatMessageCbs *cbs;
	LinphoneContent* content;
	FILE *file_to_send = NULL;
	size_t file_size;
	char *send_filepath = ms_strdup_printf("%s/images/nowebcamCIF.jpg", liblinphone_tester_file_prefix);
	char *receive_filepath = ms_strdup_printf("%s/receive_file.dump", liblinphone_tester_writable_dir_prefix);
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	file_to_send = fopen(send_filepath, "rb");
	fseek(file_to_send, 0, SEEK_END);
	file_size = ftell(file_to_send);
	fseek(file_to_send, 0, SEEK_SET);

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);
	/* create a file transfer message */
	content = linphone_core_create_content(pauline->lc);
	linphone_content_set_type(content,"image");
	linphone_content_set_subtype(content,"jpeg");
	linphone_content_set_size(content,file_size); /*total size to be transfered*/
	linphone_content_set_name(content,"nowebcamCIF.jpg");
	message = linphone_chat_room_create_file_transfer_message(chat_room, content);
	linphone_chat_message_set_user_data(message, file_to_send);
	cbs = linphone_chat_message_get_callbacks(message);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, file_transfer_send);
	linphone_chat_room_send_chat_message(chat_room,message);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
	fclose(file_to_send);
	if (marie->stat.last_received_chat_message ) {
		cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
		linphone_chat_message_download_file(marie->stat.last_received_chat_message);
	}
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageExtBodyReceived,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);
	CU_ASSERT_TRUE(compare_files(send_filepath, receive_filepath));

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(send_filepath);
	ms_free(receive_filepath);
}

/* same than previous but with a 160 characters file */
#define SMALL_FILE_SIZE 160
static void small_file_transfer_message(void) {
	int i;
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* message;
	LinphoneChatMessageCbs *cbs;
	LinphoneContent* content;
	const char* big_file_content="big file"; /* setting dummy file content to something */
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	for (i=0;i<SMALL_FILE_SIZE;i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[SMALL_FILE_SIZE - 1]=*"E";

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);
	/* create a file transfer message */
	content = linphone_core_create_content(pauline->lc);
	linphone_content_set_type(content,"text");
	linphone_content_set_subtype(content,"plain");
	linphone_content_set_size(content,SMALL_FILE_SIZE); /*total size to be transfered*/
	linphone_content_set_name(content,"bigfile.txt");
	message = linphone_chat_room_create_file_transfer_message(chat_room, content);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	cbs = linphone_chat_message_get_callbacks(message);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, memory_file_transfer_send);
	linphone_chat_room_send_chat_message(chat_room,message);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
	if (marie->stat.last_received_chat_message ) {
		cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
		linphone_chat_message_download_file(marie->stat.last_received_chat_message);
	}
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageExtBodyReceived,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void file_transfer_message_io_error_upload(void) {
	int i;
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* message;
	LinphoneChatMessageCbs *cbs;
	LinphoneContent* content;
	const char* big_file_content="big file"; /* setting dummy file content to something */
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	/* setting dummy file content to something */
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);

	/* create a file transfer message */
	content = linphone_core_create_content(pauline->lc);
	linphone_content_set_type(content,"text");
	linphone_content_set_subtype(content,"plain");
	linphone_content_set_size(content,sizeof(big_file)); /*total size to be transfered*/
	linphone_content_set_name(content,"bigfile.txt");
	message = linphone_chat_room_create_file_transfer_message(chat_room, content);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	cbs = linphone_chat_message_get_callbacks(message);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, memory_file_transfer_send);
	linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
	linphone_chat_room_send_chat_message(chat_room,message);

	/*wait for file to be 25% uploaded and simultate a network error*/
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.progress_of_LinphoneFileTransfer,25));
	sal_set_send_error(pauline->lc->sal, -1);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageNotDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,0);

	sal_set_send_error(pauline->lc->sal, 0);

	linphone_core_refresh_registers(pauline->lc); /*to make sure registration is back in registered and so it can be later unregistered*/
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneRegistrationOk,pauline->stat.number_of_LinphoneRegistrationOk+1));

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


#ifdef TEST_IS_BUGGED_NO_CALL_TO_IO_ERROR_CALLBACK
static void file_transfer_message_io_error_download(void) {
	int i;
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* message;
	LinphoneContent content;
	const char* big_file_content="big file"; /* setting dummy file content to something */
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	/* setting dummy file content to something */
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);

	/* create a file transfer message */
	memset(&content,0,sizeof(content));
	content.type="text";
	content.subtype="plain";
	content.size=sizeof(big_file); /*total size to be transfered*/
	content.name = "bigfile.txt";
	message = linphone_chat_room_create_file_transfer_message(chat_room, &content);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);

	/* wait for marie to receive pauline's message */
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));


	if (marie->stat.last_received_chat_message ) { /* get last message and use it to download file */
		linphone_chat_message_start_file_download(marie->stat.last_received_chat_message, liblinphone_tester_chat_message_state_change, marie->lc);
		/* wait for file to be 50% downloaded */
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.progress_of_LinphoneFileTransfer, 50));
		/* and simulate network error */
		sal_set_recv_error(marie->lc->sal, -1);
	}

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageNotDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,0);

	sal_set_recv_error(marie->lc->sal, 0);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
#endif

static void file_transfer_message_upload_cancelled(void) {
	int i;
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* message;
	LinphoneChatMessageCbs *cbs;
	LinphoneContent* content;
	const char* big_file_content="big file"; /* setting dummy file content to something */
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	/* setting dummy file content to something */
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);

	/* create a file transfer message */
	content = linphone_core_create_content(pauline->lc);
	linphone_content_set_type(content,"text");
	linphone_content_set_subtype(content,"plain");
	linphone_content_set_size(content,sizeof(big_file)); /*total size to be transfered*/
	linphone_content_set_name(content,"bigfile.txt");
	message = linphone_chat_room_create_file_transfer_message(chat_room, content);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	cbs = linphone_chat_message_get_callbacks(message);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, memory_file_transfer_send);
	linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
	linphone_chat_room_send_chat_message(chat_room,message);

	/*wait for file to be 50% uploaded and cancel the transfer */
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.progress_of_LinphoneFileTransfer, 50));
	linphone_chat_message_cancel_file_transfer(message);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageNotDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,0);

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void file_transfer_message_download_cancelled(void) {
#if 0
	int i;
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* message;
	LinphoneContent content;
	const char* big_file_content="big file"; /* setting dummy file content to something */
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	/* setting dummy file content to something */
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);

	/* create a file transfer message */
	memset(&content,0,sizeof(content));
	content.type="text";
	content.subtype="plain";
	content.size=sizeof(big_file); /*total size to be transfered*/
	content.name = "bigfile.txt";
	message = linphone_chat_room_create_file_transfer_message(chat_room, &content);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);

	/* wait for marie to receive pauline's message */
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));


	if (marie->stat.last_received_chat_message ) { /* get last message and use it to download file */
		linphone_chat_message_start_file_download(marie->stat.last_received_chat_message, liblinphone_tester_chat_message_state_change, marie->lc);
		/* wait for file to be 50% downloaded */
		CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.progress_of_LinphoneFileTransfer, 50));
		/* and cancel the transfer */
		linphone_chat_message_cancel_file_transfer(marie->stat.last_received_chat_message);
	}

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,0);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageNotDelivered,1);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
#endif
	ms_error("Test skipped");
}

static void text_message_with_send_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(marie->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	/*simultate a network error*/
	sal_set_send_error(marie->lc->sal, -1);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,message);

	/* check transient message list: the message should be in it, and should be the only one */
	CU_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 1);
	CU_ASSERT_EQUAL(ms_list_nth_data(chat_room->transient_messages,0), message);


	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	/*CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageInProgress,1);*/
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0);

	/* the message should have been discarded from transient list after an error */
	CU_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 0);

	sal_set_send_error(marie->lc->sal, 0);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_denied(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(marie->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(message);

	/*pauline doesn't want to be disturbed*/
	linphone_core_disable_chat(pauline->lc,LinphoneReasonDoNotDisturb);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,message);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static const char *info_content="<somexml>blabla</somexml>";

void info_message_received(LinphoneCore *lc, LinphoneCall* call, const LinphoneInfoMessage *msg){
	stats* counters = get_stats(lc);

	if (counters->last_received_info_message) {
		linphone_info_message_destroy(counters->last_received_info_message);
	}
	counters->last_received_info_message=linphone_info_message_copy(msg);
	counters->number_of_inforeceived++;
}



static void info_message_with_args(bool_t with_content) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	LinphoneInfoMessage *info;
	const LinphoneContent *content;
	const char *hvalue;

	CU_ASSERT_TRUE(call(pauline,marie));

	info=linphone_core_create_info_message(marie->lc);
	linphone_info_message_add_header(info,"Weather","still bad");
	if (with_content) {
		LinphoneContent* ct=linphone_core_create_content(marie->lc);
		linphone_content_set_type(ct,"application");
		linphone_content_set_subtype(ct,"somexml");
		linphone_content_set_buffer(ct,info_content,strlen(info_content));
		linphone_info_message_set_content(info,ct);
		linphone_content_unref(ct);
	}
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_call_send_info_message(linphone_core_get_current_call(marie->lc),info);
	linphone_info_message_destroy(info);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_inforeceived,1));

	CU_ASSERT_PTR_NOT_NULL(pauline->stat.last_received_info_message);
	hvalue=linphone_info_message_get_header(pauline->stat.last_received_info_message, "Weather");
	content=linphone_info_message_get_content(pauline->stat.last_received_info_message);

	CU_ASSERT_PTR_NOT_NULL(hvalue);
	if (hvalue)
		CU_ASSERT_TRUE(strcmp(hvalue,"still bad")==0);

	if (with_content){
		CU_ASSERT_PTR_NOT_NULL(content);
		if (content) {
			CU_ASSERT_PTR_NOT_NULL(linphone_content_get_buffer(content));
			CU_ASSERT_PTR_NOT_NULL(linphone_content_get_type(content));
			CU_ASSERT_PTR_NOT_NULL(linphone_content_get_subtype(content));
			if (linphone_content_get_type(content)) CU_ASSERT_TRUE(strcmp(linphone_content_get_type(content),"application")==0);
			if (linphone_content_get_subtype(content)) CU_ASSERT_TRUE(strcmp(linphone_content_get_subtype(content),"somexml")==0);
			if (linphone_content_get_buffer(content))CU_ASSERT_TRUE(strcmp((const char*)linphone_content_get_buffer(content),info_content)==0);
			CU_ASSERT_EQUAL(linphone_content_get_size(content),strlen(info_content));
		}
	}
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void info_message(){
	info_message_with_args(FALSE);
}

static void info_message_with_body(){
	info_message_with_args(TRUE);
}

static void is_composing_notification(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc, to);
	int dummy = 0;

	ms_free(to);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_compose(chat_room);
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /*just to sleep while iterating*/
	linphone_chat_room_send_message(chat_room, "Composing a message");
	CU_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, 1));
	CU_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingIdleReceived, 2));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

#ifdef MSG_STORAGE_ENABLED

/*
 * Copy file "from" to file "to".
 * Destination file is truncated if existing.
 * Return 0 on success, positive value on error.
 */
static int
message_tester_copy_file(const char *from, const char *to)
{
	FILE *in, *out;
	char buf[256];
	size_t n;

	/* Open "from" file for reading */
	in=fopen(from, "r");
	if ( in == NULL )
	{
		ms_error("Can't open %s for reading: %s\n",from,strerror(errno));
		return 1;
	}

	/* Open "to" file for writing (will truncate existing files) */
	out=fopen(to, "w");
	if ( out == NULL )
	{
		ms_error("Can't open %s for writing: %s\n",to,strerror(errno));
		fclose(in);
		return 2;
	}

	/* Copy data from "in" to "out" */
	while ( (n=fread(buf, 1, sizeof buf, in)) > 0 )
	{
		if ( ! fwrite(buf, 1, n, out) )
		{
			ms_error("Could not write in %s: %s\n",to,strerror(errno));
			fclose(in);
			fclose(out);
			return 3;
		}
	}

	fclose(in);
	fclose(out);

	return 0;
}

static int check_no_strange_time(void* data,int argc, char** argv,char** cNames) {
	CU_ASSERT_EQUAL(argc, 1);
	CU_ASSERT_STRING_EQUAL(cNames[0], "COUNT(*)"); // count of non updated messages should be 0
	CU_ASSERT_STRING_EQUAL(argv[0], "0"); // count of non updated messages should be 0
	return 0;
}

static void message_storage_migration() {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	char src_db[256];
	char tmp_db[256];
	MSList* chatrooms;
	snprintf(src_db,sizeof(src_db), "%s/messages.db", liblinphone_tester_file_prefix);
	snprintf(tmp_db,sizeof(tmp_db), "%s/tmp.db", liblinphone_tester_writable_dir_prefix);

	CU_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 0);

	// enable to test the performances of the migration step
	//linphone_core_message_storage_set_debug(marie->lc, TRUE);

	// the messages.db has 10000 dummy messages with the very first DB scheme.
	// This will test the migration procedure
	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	chatrooms = linphone_core_get_chat_rooms(marie->lc);
	CU_ASSERT(ms_list_size(chatrooms) > 0);

	// check that all messages have been migrated to the UTC time storage
	CU_ASSERT(sqlite3_exec(marie->lc->db, "SELECT COUNT(*) FROM history WHERE time != '-1';", check_no_strange_time, NULL, NULL) == SQLITE_OK );

	linphone_core_manager_destroy(marie);
	remove(tmp_db);
}

static void history_message_count_helper(LinphoneChatRoom* chatroom, int x, int y, int expected ){
	MSList* messages = linphone_chat_room_get_history_range(chatroom, x, y);
	int size = ms_list_size(messages);
	if( expected != size ){
		ms_warning("History retrieved from %d to %d returned %d records, but expected %d", x, y, size, expected);
	}
	CU_ASSERT_EQUAL(size, expected);

	ms_list_free_with_data(messages, (void (*)(void *))linphone_chat_message_unref);

}

static void history_range_full_test(){
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *jehan_addr = linphone_address_new("<sip:Jehan@sip.linphone.org>");
	LinphoneChatRoom *chatroom;
	char src_db[256];
	char tmp_db[256];
	snprintf(src_db,sizeof(src_db), "%s/messages.db", liblinphone_tester_file_prefix);
	snprintf(tmp_db,sizeof(tmp_db), "%s/tmp.db", liblinphone_tester_writable_dir_prefix);

	CU_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 0);

	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	chatroom = linphone_core_get_chat_room(marie->lc, jehan_addr);
	CU_ASSERT_PTR_NOT_NULL(chatroom);
	if (chatroom){
		// We have 20 tests to perform to fully qualify the function, here they are:
		history_message_count_helper(chatroom, 0, 0, 1);
		history_message_count_helper(chatroom, -1, 0, 1);
		history_message_count_helper(chatroom, 0, -1, 1270);
		history_message_count_helper(chatroom, 1, 3, 3);
		history_message_count_helper(chatroom, 3, 1, 1270-3);
		history_message_count_helper(chatroom, 10, 10, 1);
		history_message_count_helper(chatroom, -1, -1, 1270);
		history_message_count_helper(chatroom, -1, -2, 1270);
		history_message_count_helper(chatroom, -2, -1, 1270);
		history_message_count_helper(chatroom, 3, -1, 1270-3);
		history_message_count_helper(chatroom, 1, -3, 1270-1);
		history_message_count_helper(chatroom, 2, -2, 1270-2);
		history_message_count_helper(chatroom, 2, 0, 1270-2);
		history_message_count_helper(chatroom, 0, 2, 3);
		history_message_count_helper(chatroom, -1, 3, 4);
		history_message_count_helper(chatroom, -2, 2, 3);
		history_message_count_helper(chatroom, -3, 1, 2);
	}
	linphone_core_manager_destroy(marie);
	linphone_address_destroy(jehan_addr);
	remove(tmp_db);
}


static void history_messages_count() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *jehan_addr = linphone_address_new("<sip:Jehan@sip.linphone.org>");
	LinphoneChatRoom *chatroom;
	MSList *messages;
	char src_db[256];
	char tmp_db[256];
	snprintf(src_db,sizeof(src_db), "%s/messages.db", liblinphone_tester_file_prefix);
	snprintf(tmp_db,sizeof(tmp_db), "%s/tmp.db", liblinphone_tester_writable_dir_prefix);

	CU_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 0);

	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	chatroom = linphone_core_get_chat_room(marie->lc, jehan_addr);
	CU_ASSERT_PTR_NOT_NULL(chatroom);
	if (chatroom){
		messages=linphone_chat_room_get_history(chatroom,10);
		CU_ASSERT_EQUAL(ms_list_size(messages), 10);
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		messages=linphone_chat_room_get_history(chatroom,1);
		CU_ASSERT_EQUAL(ms_list_size(messages), 1);
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		messages=linphone_chat_room_get_history(chatroom,0);
		CU_ASSERT_EQUAL(linphone_chat_room_get_history_size(chatroom), 1270);
		CU_ASSERT_EQUAL(ms_list_size(messages), 1270);
		/*check the second most recent message*/
		CU_ASSERT_STRING_EQUAL(linphone_chat_message_get_text((LinphoneChatMessage *)messages->next->data), "Fore and aft follow each other.");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test offset+limit: retrieve the 42th latest message only and check its content*/
		messages=linphone_chat_room_get_history_range(chatroom, 42, 42);
		CU_ASSERT_EQUAL(ms_list_size(messages), 1);
		CU_ASSERT_STRING_EQUAL(linphone_chat_message_get_text((LinphoneChatMessage *)messages->data), "If you open yourself to the Tao is intangible and evasive, yet prefers to keep us at the mercy of the kingdom, then all of the streams of hundreds of valleys because of its limitless possibilities.");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test offset without limit*/
		messages = linphone_chat_room_get_history_range(chatroom, 1265, -1);
		CU_ASSERT_EQUAL(ms_list_size(messages), 1270-1265);
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test limit without offset*/
		messages = linphone_chat_room_get_history_range(chatroom, 0, 5);
		CU_ASSERT_EQUAL(ms_list_size(messages), 6);
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test invalid start*/
		messages = linphone_chat_room_get_history_range(chatroom, 1265, 1260);
		CU_ASSERT_EQUAL(ms_list_size(messages), 1270-1265);
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);
	}
	linphone_core_manager_destroy(marie);
	linphone_address_destroy(jehan_addr);
	remove(tmp_db);
}


#endif

test_t message_tests[] = {
	{ "Text message", text_message },
	{ "Text message within call's dialog", text_message_within_dialog},
	{ "Text message with credentials from auth info cb", text_message_with_credential_from_auth_cb},
	{ "Text message with privacy", text_message_with_privacy },
	{ "Text message compatibility mode", text_message_compatibility_mode },
	{ "Text message with ack", text_message_with_ack },
	{ "Text message with send error", text_message_with_send_error },
	{ "Text message with external body", text_message_with_external_body },
	{ "File transfer message", file_transfer_message },
	{ "Small File transfer message", small_file_transfer_message},
	{ "File transfer message with io error at upload", file_transfer_message_io_error_upload },
/*	{ "File transfer message with io error at download", file_transfer_message_io_error_download },*/
	{ "File transfer message upload cancelled", file_transfer_message_upload_cancelled },
	{ "File transfer message download cancelled", file_transfer_message_download_cancelled },
	{ "Text message denied", text_message_denied },
	{ "Info message", info_message },
	{ "Info message with body", info_message_with_body },
	{ "IsComposing notification", is_composing_notification }
#ifdef MSG_STORAGE_ENABLED
	,{ "Database migration", message_storage_migration }
	,{ "History count", history_messages_count }
	,{ "History range", history_range_full_test }
#endif
};

test_suite_t message_test_suite = {
	"Message",
	NULL,
	NULL,
	sizeof(message_tests) / sizeof(message_tests[0]),
	message_tests
};

