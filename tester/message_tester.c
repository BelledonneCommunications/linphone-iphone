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



#include "linphonecore.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "lime.h"

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
			BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_external_body_url(message),message_external_body_url);
			message_external_body_url=NULL;
		}
	}
}

/**
 * function invoked when a file transfer is received.
 * */
void file_transfer_received(LinphoneChatMessage *message, const LinphoneContent* content, const LinphoneBuffer *buffer){
	FILE* file=NULL;
	char *receive_file = bc_tester_file("receive_file.dump");
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(message);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk, creating file*/
		file = fopen(receive_file,"wb");
		linphone_chat_message_set_user_data(message,(void*)file); /*store fd for next chunks*/
	}
	ms_free(receive_file);
	file = (FILE*)linphone_chat_message_get_user_data(message);

	if (linphone_buffer_is_empty(buffer)) { /* tranfer complete */
		stats* counters = get_stats(lc);
		counters->number_of_LinphoneFileTransferDownloadSuccessful++;
		fclose(file);
	} else { /* store content on a file*/
		if (fwrite(linphone_buffer_get_content(buffer),linphone_buffer_get_size(buffer),1,file)==-1){
			ms_error("file_transfer_received(): write() failed: %s",strerror(errno));
		}
	}
}

char big_file[128000]; /* a buffer to simulate a big file for the file transfer message test */

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
	const char *text = linphone_chat_message_get_text(msg);
	if (!text) text = "<no text>";
	ms_message("Message [%s] [%s]",text, linphone_chat_message_state_to_string(state));
	switch (state) {
		case LinphoneChatMessageStateIdle:
			return;
		case LinphoneChatMessageStateDelivered:
			counters->number_of_LinphoneMessageDelivered++;
			return;
		case LinphoneChatMessageStateNotDelivered:
			counters->number_of_LinphoneMessageNotDelivered++;
			return;
		case LinphoneChatMessageStateInProgress:
			counters->number_of_LinphoneMessageInProgress++;
			return;
		case LinphoneChatMessageStateFileTransferError:
			counters->number_of_LinphoneMessageNotDelivered++;
			return;
		case LinphoneChatMessageStateFileTransferDone:
			counters->number_of_LinphoneMessageFileTransferDone++;
			return;
	}
	ms_error("Unexpected state [%s] for message [%p]",linphone_chat_message_state_to_string(state), msg);
}

static void text_message(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	char* to;
	LinphoneChatRoom* chat_room;

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
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1, int, "%d");

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_within_dialog(void) {
	char* to;
	LinphoneChatRoom* chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
	BC_ASSERT_TRUE(call(marie,pauline));

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

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
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	text_message_with_credential_from_auth_cb_auth_info=linphone_auth_info_clone((LinphoneAuthInfo*)(linphone_core_get_auth_info_list(pauline->lc)->data));

	/*to force cb to be called*/
	linphone_core_clear_all_auth_info(pauline->lc);
	vtable->auth_info_requested=text_message_with_credential_from_auth_cb_auth_info_requested;
	linphone_core_add_listener(pauline->lc, vtable);

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
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1, int, "%d");

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_privacy(void) {
	char *to;
	LinphoneChatRoom* chat_room;

	LinphoneProxyConfig* pauline_proxy;
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	/*test proxy config privacy*/
	linphone_core_get_default_proxy(pauline->lc,&pauline_proxy);
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1, int, "%d");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_compatibility_mode(void) {
	char route[256];
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneProxyConfig* proxy;
	LinphoneAddress* proxy_address;
	char*tmp;
	LCSipTransports transport;
	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room;

	linphone_core_get_default_proxy(marie->lc,&proxy);
	BC_ASSERT_PTR_NOT_NULL (proxy);
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

	BC_ASSERT_TRUE (wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphoneRegistrationOk,1));

	chat_room = linphone_core_create_chat_room(marie->lc,to);
	{
		int dummy=0;
		wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);
	}
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceivedLegacy,1, int, "%d");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_ack(void) {
	int leaked_objects;
	int begin;
	LinphoneCoreManager* marie;
	LinphoneCoreManager* pauline;

	belle_sip_object_enable_leak_detector(TRUE);
	begin=belle_sip_object_get_object_count();

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_tcp_rc");

	{
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
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
		ms_free(to);
		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(pauline);
	}
	leaked_objects=belle_sip_object_get_object_count()-begin;
	BC_ASSERT_EQUAL(leaked_objects, 0, int, "%d");
	if (leaked_objects>0){
		belle_sip_object_dump_active_objects();
	}
}

static void text_message_with_external_body(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	char *to = linphone_address_as_string(marie->identity);
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
	BC_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 1, int, "%d");
	BC_ASSERT_PTR_EQUAL(ms_list_nth_data(chat_room->transient_messages,0), message);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1, int, "%d");

	BC_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 0, int, "%d");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	ms_free(to);
}

bool_t compare_files(const char *path1, const char *path2) {
	bool_t res;
	size_t size1;
	size_t size2;
	uint8_t *buf1;
	uint8_t *buf2;

	buf1 = (uint8_t*)ms_load_path_content(path1, &size1);
	buf2 = (uint8_t*)ms_load_path_content(path2, &size2);
	res = buf1 && buf2 && (size1 == size2) && (memcmp(buf1, buf2, size1) == 0);
	ms_free(buf1);
	ms_free(buf2);
	return res;
}

static void file_transfer_message(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		char* to;
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* message;
		LinphoneChatMessageCbs *cbs;
		LinphoneContent* content;
		FILE *file_to_send = NULL;
		size_t file_size;
		char *send_filepath = bc_tester_res("images/nowebcamCIF.jpg");
		char *receive_filepath = bc_tester_file("receive_file.dump");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
		fclose(file_to_send);
		if (marie->stat.last_received_chat_message ) {
			cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
			linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
			linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
			linphone_chat_message_download_file(marie->stat.last_received_chat_message);
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1, int, "%d");
		BC_ASSERT_TRUE(compare_files(send_filepath, receive_filepath));

		linphone_content_unref(content);
		linphone_core_manager_destroy(pauline);
		ms_free(send_filepath);
		ms_free(receive_filepath);
		linphone_core_manager_destroy(marie);
	}
}

/* same than previous but with a 160 characters file */
#define SMALL_FILE_SIZE 160
static void small_file_transfer_message(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		int i;
		char* to;
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* message;
		LinphoneChatMessageCbs *cbs;
		LinphoneContent* content;
		const char* big_file_content="big file"; /* setting dummy file content to something */
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
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
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
		if (marie->stat.last_received_chat_message ) {
			cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
			linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
			linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
			linphone_chat_message_download_file(marie->stat.last_received_chat_message);
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1, int, "%d");

		linphone_content_unref(content);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

#ifdef HAVE_LIME

static FILE* fopen_from_write_dir(const char * name, const char * mode) {
	char *filepath = bc_tester_res("%s", bc_tester_writable_dir_prefix,name);
	FILE * file = fopen(filepath,mode);
	ms_free(filepath);
	return file;
}

static void lime_file_transfer_message_base(bool_t encrypt_file) {
	int i;
	char *to;
	FILE *ZIDCacheMarieFD, *ZIDCachePaulineFD;
	LinphoneCoreManager *marie, *pauline;
	LinphoneChatRoom *chat_room;
	LinphoneContent *content;
	LinphoneChatMessage *message;
	LinphoneChatMessageCbs *cbs;
	char *pauline_id, *marie_id;
	char *filepath;

	/* setting dummy file content to something */
	const char* big_file_content="big file";
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_tcp_rc");
	reset_counters(&marie->stat);
	reset_counters(&pauline->stat);

	/* make sure lime is enabled */
	linphone_core_enable_lime(marie->lc, 1);
	linphone_core_enable_lime(pauline->lc, 1);
	if (!encrypt_file) {
		LpConfig *pauline_lp = linphone_core_get_config(pauline->lc);
		lp_config_set_int(pauline_lp,"sip","lime_for_file_sharing",0);
	}

	/* set the zid caches files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri */
	ZIDCacheMarieFD = fopen_from_write_dir("tmpZIDCacheMarie.xml", "wb");
	ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "wb");
	pauline_id = linphone_address_as_string_uri_only(pauline->identity);
	marie_id = linphone_address_as_string_uri_only(marie->identity);
	fprintf(ZIDCacheMarieFD, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>ef7692d0792a67491ae2d44e</selfZID><peer><ZID>005dbe0399643d953a2202dd</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</sndKey><rcvKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</rcvKey><sndSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000078</sndIndex><rcvIndex>000001cf</rcvIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778csal_set_uuid(lc->sal, account->instance_id);bdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>72d80ab1cad243cf45634980c1d02cfb2df81ce0dd5dfcf1ebeacfc5345a9176</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000000f</sndIndex><rcvIndex>00000000</rcvIndex></peer></cache>", pauline_id, pauline_id);
	fprintf(ZIDCachePaulineFD, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>005dbe0399643d953a2202dd</selfZID><peer><ZID>ef7692d0792a67491ae2d44e</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><rcvKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</rcvKey><sndKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</sndKey><rcvSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvIndex>00000078</rcvIndex><sndIndex>000001cf</sndIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>81e6e6362c34dc974263d1f77cbb9a8d6d6a718330994379099a8fa19fb12faa</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000002e</sndIndex><rcvIndex>00000000</rcvIndex><pvs>01</pvs></peer></cache>", marie_id, marie_id);
	fclose(ZIDCacheMarieFD);
	fclose(ZIDCachePaulineFD);
	ms_free(marie_id);
	ms_free(pauline_id);

	filepath = bc_tester_res("%s", bc_tester_writable_dir_prefix,"tmpZIDCacheMarie.xml");
	linphone_core_set_zrtp_secrets_file(marie->lc, filepath);
	ms_free(filepath);

	filepath = bc_tester_res("%s", bc_tester_writable_dir_prefix,"tmpZIDCachePauline.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
	ms_free(filepath);

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
	linphone_content_set_size(content,sizeof(big_file)); /*total size to be transfered*/
	linphone_content_set_name(content,"big_file.txt");

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
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
	if (marie->stat.last_received_chat_message ) {
		cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
		linphone_chat_message_download_file(marie->stat.last_received_chat_message);
	}
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1));

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1, int, "%d");

	linphone_content_unref(content);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);

}
static  void lime_file_transfer_message() {
	lime_file_transfer_message_base(TRUE);
}

static  void lime_file_transfer_message_without_encryption() {
	lime_file_transfer_message_base(FALSE);
}

static void printHex(char *title, uint8_t *data, uint32_t length) {
	int i;
	char debug_string_buffer[2048];
	char *debug_string = debug_string_buffer;
	sprintf (debug_string, "%s : ", title);
	debug_string += strlen(title)+3;
	for (i=0; i<length; i++) {
		sprintf (debug_string, "0x%02x, ", data[i]);
		debug_string+=6;
	}
	*debug_string = '\0';
	ms_message("%s", debug_string_buffer);
}

#define PLAIN_TEXT_TEST_MESSAGE "Ceci est un fabuleux message de test à encrypter"
static void lime_unit(void) {
	int retval;
	size_t size;
	uint8_t *cacheBufferString;
	xmlDocPtr cacheBufferAlice;
	xmlDocPtr cacheBufferBob;
	uint8_t *multipartMessage = NULL;
	uint8_t *decryptedMessage = NULL;
	xmlChar *xmlStringOutput;
	int xmlStringLength;
	limeURIKeys_t associatedKeys;
	int i;
	limeKey_t associatedKey;
	uint8_t targetZID[12] = {0x00, 0x5d, 0xbe, 0x03, 0x99, 0x64, 0x3d, 0x95, 0x3a, 0x22, 0x02, 0xdd};
	uint8_t senderZID[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0};
	uint8_t encryptedMessage[1024];
	uint8_t plainMessage[1024];
	uint8_t receiverZID[12];
	xmlDocPtr cacheBuffer;
	FILE *CACHE;

	/**** Low level tests using on cache file to extract keys, encrypt/decrypt ****/
	/**** use functions that are not directly used by external entities ****/

	/* create and load cache file */
	CACHE = fopen_from_write_dir("ZIDCache.xml", "wb");
	fprintf (CACHE, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>ef7692d0792a67491ae2d44e</selfZID><peer><ZID>005dbe0399643d953a2202dd</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>pipo1@pipo.com</uri><sndKey>963c57bb28e62068d2df23e8f9b771932d3c57bb28e62068d2df23e8f9b77193</sndKey><rcvKey>05d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>02ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000069</sndIndex><rcvIndex>000001e8</rcvIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>pipo1@pipo.com</uri><sndKey>123456789012345678901234567890123456765431262068d2df23e8f9b77193</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000001</sndIndex><rcvIndex>00000000</rcvIndex><pvs>01</pvs></peer></cache>");
	fclose(CACHE);
	CACHE = fopen_from_write_dir("ZIDCache.xml", "rb+");
	cacheBufferString = (uint8_t*) ms_load_file_content(CACHE, &size);
	*(cacheBufferString+size) = '\0';
	fclose(CACHE);
	/* parse it to an xmlDoc */
	cacheBuffer = xmlParseDoc(cacheBufferString);
	ms_free(cacheBufferString);

	/* get data from cache : sender */
	associatedKeys.peerURI = (uint8_t *)malloc(15);
	memcpy(associatedKeys.peerURI, "pipo1@pipo.com", 15);
	associatedKeys.associatedZIDNumber  = 0;
	retval = lime_getCachedSndKeysByURI(cacheBuffer, &associatedKeys);
	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");
	BC_ASSERT_EQUAL_FATAL(associatedKeys.associatedZIDNumber, 2, int, "%d"); /* there are 2 keys associated to pipo1@pipo.com address in the cache above*/
	ms_message("Get cached key by URI, for sender, return %d keys", associatedKeys.associatedZIDNumber);

	for (i=0; i<associatedKeys.associatedZIDNumber; i++) {
		printHex("ZID", associatedKeys.peerKeys[i]->peerZID, 12);
		printHex("key", associatedKeys.peerKeys[i]->key, 32);
		printHex("sessionID", associatedKeys.peerKeys[i]->sessionId, 32);
		ms_message("session index %d\n", associatedKeys.peerKeys[i]->sessionIndex);
	}

	/* get data from cache : receiver */
	memcpy(associatedKey.peerZID, targetZID, 12);
	retval = lime_getCachedRcvKeyByZid(cacheBuffer, &associatedKey);
	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");
	printHex("Got receiver key for ZID", targetZID, 12);
	printHex("Key", associatedKey.key, 32);
	printHex("sessionID", associatedKey.sessionId, 32);
	ms_message("session index %d\n", associatedKey.sessionIndex);

	/* encrypt/decrypt a message */
	lime_encryptMessage(associatedKeys.peerKeys[0], (uint8_t *)PLAIN_TEXT_TEST_MESSAGE, strlen(PLAIN_TEXT_TEST_MESSAGE), senderZID, encryptedMessage);
	printHex("Ciphered", encryptedMessage, strlen((char *)encryptedMessage));
	/* invert sender and receiverZID to decrypt/authenticate */
	memcpy(receiverZID, associatedKeys.peerKeys[0]->peerZID, 12);
	memcpy(associatedKeys.peerKeys[0]->peerZID, senderZID, 12);
	retval = lime_decryptMessage(associatedKeys.peerKeys[0], encryptedMessage, strlen(PLAIN_TEXT_TEST_MESSAGE)+16, receiverZID, plainMessage);
	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");
	BC_ASSERT_STRING_EQUAL((char *)plainMessage, (char *)PLAIN_TEXT_TEST_MESSAGE);
	ms_message("Decrypt and auth returned %d\nPlain text is %s\n", retval, plainMessage);

	/* update receiver data */
	associatedKey.sessionIndex++;
	associatedKey.key[0]++;
	associatedKey.sessionId[0]++;
	retval = lime_setCachedKey(cacheBuffer, &associatedKey, LIME_RECEIVER);
	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");

	/* update sender data */
	associatedKeys.peerKeys[0]->sessionIndex++;
	associatedKeys.peerKeys[0]->key[0]++;
	associatedKeys.peerKeys[0]->sessionId[0]++;
	retval = lime_setCachedKey(cacheBuffer, associatedKeys.peerKeys[0], LIME_SENDER);
	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");

	/* free memory */
	lime_freeKeys(associatedKeys);

	/* write the file */
	/* dump the xml document into a string */
	xmlDocDumpFormatMemoryEnc(cacheBuffer, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
	/* write it to the file */
	CACHE = fopen_from_write_dir("ZIDCache.xml", "w+");
	fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
	xmlFree(xmlStringOutput);
	fclose(CACHE);
	xmlFreeDoc(cacheBuffer);

	/**** Higher level tests using 2 caches to encrypt/decrypt a message ****/
	/* Create Alice cache file and then load it */
	CACHE = fopen_from_write_dir("ZIDCacheAlice.xml", "wb");
	fprintf(CACHE, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>ef7692d0792a67491ae2d44e</selfZID><peer><ZID>005dbe0399643d953a2202dd</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>sip:pauline@sip.example.org</uri><sndKey>9111ebeb52e50edcc6fcb3eea1a2d3ae3c2c75d3668923e83c59d0f472455150</sndKey><rcvKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</rcvKey><sndSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000080</sndIndex><rcvIndex>000001cf</rcvIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>sip:pauline@sip.example.org</uri><sndKey>72d80ab1cad243cf45634980c1d02cfb2df81ce0dd5dfcf1ebeacfc5345a9176</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000000f</sndIndex><rcvIndex>00000000</rcvIndex></peer></cache>");
	fclose(CACHE);
	CACHE = fopen_from_write_dir("ZIDCacheAlice.xml", "rb+");
	cacheBufferString = (uint8_t *)ms_load_file_content(CACHE, &size);
	*(cacheBufferString+size) = '\0';
	fclose(CACHE);
	/* parse it to an xmlDoc */
	cacheBufferAlice = xmlParseDoc(cacheBufferString);
	ms_free(cacheBufferString);

	/* Create Bob cache file and then load it */
	CACHE = fopen_from_write_dir("ZIDCacheBob.xml", "wb");
	fprintf(CACHE, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>005dbe0399643d953a2202dd</selfZID><peer><ZID>ef7692d0792a67491ae2d44e</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>sip:marie@sip.example.org</uri><rcvKey>9111ebeb52e50edcc6fcb3eea1a2d3ae3c2c75d3668923e83c59d0f472455150</rcvKey><sndKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</sndKey><rcvSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvIndex>00000080</rcvIndex><sndIndex>000001cf</sndIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>sip:marie@sip.example.org</uri><sndKey>81e6e6362c34dc974263d1f77cbb9a8d6d6a718330994379099a8fa19fb12faa</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000002e</sndIndex><rcvIndex>00000000</rcvIndex><pvs>01</pvs></peer></cache>");
	fclose(CACHE);
	CACHE = fopen_from_write_dir("ZIDCacheBob.xml", "rb+");
	cacheBufferString = (uint8_t *)ms_load_file_content(CACHE, &size);
	*(cacheBufferString+size) = '\0';
	fclose(CACHE);
	/* parse it to an xmlDoc */
	cacheBufferBob = xmlParseDoc(cacheBufferString);
	ms_free(cacheBufferString);



	/* encrypt a message */
	retval = lime_createMultipartMessage(cacheBufferAlice, (uint8_t *)PLAIN_TEXT_TEST_MESSAGE, (uint8_t *)"sip:pauline@sip.example.org", &multipartMessage);

	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");
	if (retval == 0) {
		ms_message("Encrypted message created is %s", multipartMessage);
	}

	/* decrypt the multipart message */
	retval = lime_decryptMultipartMessage(cacheBufferBob, multipartMessage, &decryptedMessage);

	BC_ASSERT_EQUAL_FATAL(retval, 0, int, "%d");
	if (retval == 0) {
		BC_ASSERT_STRING_EQUAL((char *)decryptedMessage, (char *)PLAIN_TEXT_TEST_MESSAGE);
		ms_message("Succesfully decrypted message is %s", decryptedMessage);
	}
	free(multipartMessage);
	free(decryptedMessage);

	/* update ZID files */
	/* dump the xml document into a string */
	xmlDocDumpFormatMemoryEnc(cacheBufferAlice, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
	/* write it to the file */
	CACHE = fopen_from_write_dir("ZIDCacheAlice.xml", "wb+");
	fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
	xmlFree(xmlStringOutput);
	fclose(CACHE);

	xmlDocDumpFormatMemoryEnc(cacheBufferBob, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
	/* write it to the file */
	CACHE = fopen_from_write_dir("ZIDCacheBob.xml", "wb+");
	fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
	xmlFree(xmlStringOutput);
	fclose(CACHE);


	xmlFreeDoc(cacheBufferAlice);
	xmlFreeDoc(cacheBufferBob);
}

static void lime_text_message(void) {
	char* to;
	FILE *ZIDCacheMarieFD, *ZIDCachePaulineFD;
	LinphoneChatRoom* chat_room;
	char* filepath;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	/* make sure lime is enabled */
	linphone_core_enable_lime(marie->lc, 1);
	linphone_core_enable_lime(pauline->lc, 1);

	/* set the zid caches files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri */
	ZIDCacheMarieFD = fopen_from_write_dir("tmpZIDCacheMarie.xml", "w");
	ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "w");
	fprintf(ZIDCacheMarieFD, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>ef7692d0792a67491ae2d44e</selfZID><peer><ZID>005dbe0399643d953a2202dd</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</sndKey><rcvKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</rcvKey><sndSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000078</sndIndex><rcvIndex>000001cf</rcvIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>72d80ab1cad243cf45634980c1d02cfb2df81ce0dd5dfcf1ebeacfc5345a9176</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000000f</sndIndex><rcvIndex>00000000</rcvIndex></peer></cache>", linphone_address_as_string_uri_only(pauline->identity), linphone_address_as_string_uri_only(pauline->identity));
	fprintf(ZIDCachePaulineFD, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>005dbe0399643d953a2202dd</selfZID><peer><ZID>ef7692d0792a67491ae2d44e</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><rcvKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</rcvKey><sndKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</sndKey><rcvSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvIndex>00000078</rcvIndex><sndIndex>000001cf</sndIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>81e6e6362c34dc974263d1f77cbb9a8d6d6a718330994379099a8fa19fb12faa</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000002e</sndIndex><rcvIndex>00000000</rcvIndex><pvs>01</pvs></peer></cache>", linphone_address_as_string_uri_only(marie->identity), linphone_address_as_string_uri_only(marie->identity));
	fclose(ZIDCacheMarieFD);
	fclose(ZIDCachePaulineFD);

	filepath = bc_tester_res("%s", bc_tester_writable_dir_prefix,"tmpZIDCacheMarie.xml");
	linphone_core_set_zrtp_secrets_file(marie->lc, filepath);
	ms_free(filepath);

	filepath = bc_tester_res("%s", bc_tester_writable_dir_prefix,"tmpZIDCachePauline.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
	ms_free(filepath);

	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedLegacy,1));

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
	/* TODO : check the message arrived correctly deciphered */

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
#endif /* HAVE_LIME */

static void file_transfer_message_io_error_upload(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		int i;
		char* to;
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* message;
		LinphoneChatMessageCbs *cbs;
		LinphoneContent* content;
		const char* big_file_content="big file"; /* setting dummy file content to something */
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.progress_of_LinphoneFileTransfer,25));
		sal_set_send_error(pauline->lc->sal, -1);

		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");

		sal_set_send_error(pauline->lc->sal, 0);

		linphone_core_refresh_registers(pauline->lc); /*to make sure registration is back in registered and so it can be later unregistered*/
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneRegistrationOk,pauline->stat.number_of_LinphoneRegistrationOk+1));

		linphone_content_unref(content);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}


#ifdef TEST_IS_BUGGED_NO_CALL_TO_IO_ERROR_CALLBACK
static void file_transfer_message_io_error_download(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		int i;
		char* to;
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* message;
		LinphoneContent content;
		const char* big_file_content="big file"; /* setting dummy file content to something */
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
		ms_free(to);

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
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));


		if (marie->stat.last_received_chat_message ) { /* get last message and use it to download file */
			linphone_chat_message_start_file_download(marie->stat.last_received_chat_message, liblinphone_tester_chat_message_state_change, marie->lc);
			/* wait for file to be 50% downloaded */
			BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.progress_of_LinphoneFileTransfer, 50));
			/* and simulate network error */
			sal_set_recv_error(marie->lc->sal, -1);
		}

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");

		sal_set_recv_error(marie->lc->sal, 0);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}
#endif

static void file_transfer_message_upload_cancelled(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		int i;
		char* to;
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* message;
		LinphoneChatMessageCbs *cbs;
		LinphoneContent* content;
		const char* big_file_content="big file"; /* setting dummy file content to something */
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.progress_of_LinphoneFileTransfer, 50));
		linphone_chat_message_cancel_file_transfer(message);

		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");

		linphone_content_unref(content);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
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
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));


	if (marie->stat.last_received_chat_message ) { /* get last message and use it to download file */
		linphone_chat_message_start_file_download(marie->stat.last_received_chat_message, liblinphone_tester_chat_message_state_change, marie->lc);
		/* wait for file to be 50% downloaded */
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.progress_of_LinphoneFileTransfer, 50));
		/* and cancel the transfer */
		linphone_chat_message_cancel_file_transfer(marie->stat.last_received_chat_message);
	}

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
#endif
	ms_error("Test skipped");
}

static void file_transfer_using_external_body_url(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
		char *to;
		LinphoneChatMessageCbs *cbs;
		LinphoneChatRoom *chat_room;
		LinphoneChatMessage *message;
		LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
		reset_counters(&marie->stat);
		reset_counters(&pauline->stat);

		/* make sure lime is disabled */
		linphone_core_enable_lime(marie->lc, FALSE);
		linphone_core_enable_lime(pauline->lc, FALSE);

		/* create a chatroom on pauline's side */
		to = linphone_address_as_string(marie->identity);
		chat_room = linphone_core_create_chat_room(pauline->lc,to);

		message = linphone_chat_room_create_message(chat_room, NULL);

		cbs = linphone_chat_message_get_callbacks(message);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);

		linphone_chat_message_set_external_body_url(message, "https://www.linphone.org:444//tmp/54ec58280ace9_c30709218df8eaba61d1.jpg");
		linphone_chat_room_send_chat_message(chat_room, message);

		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
		if (marie->stat.last_received_chat_message) {
			linphone_chat_message_download_file(marie->stat.last_received_chat_message);
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageExtBodyReceived, 1));
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageInProgress, 1));
		ms_free(to);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

static void file_transfer_message_two_messages() {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		char* to;
		LinphoneChatRoom* pauline_room;
		LinphoneChatMessage* message;
		LinphoneChatMessage* message2;
		LinphoneChatMessageCbs *cbs;
		LinphoneContent* content;
		FILE *file_to_send = NULL;
		size_t file_size;
		char *send_filepath = bc_tester_res("images/nowebcamCIF.jpg");
		char *receive_filepath = bc_tester_file("receive_file.dump");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

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
		pauline_room = linphone_core_create_chat_room(pauline->lc,to);
		ms_free(to);
		/* create a file transfer message */
		content = linphone_core_create_content(pauline->lc);
		linphone_content_set_type(content,"image");
		linphone_content_set_subtype(content,"jpeg");
		linphone_content_set_size(content,file_size); /*total size to be transfered*/
		linphone_content_set_name(content,"nowebcamCIF.jpg");
		message = linphone_chat_room_create_file_transfer_message(pauline_room, content);
		linphone_chat_message_set_user_data(message, file_to_send);
		message2 = linphone_chat_room_create_file_transfer_message(pauline_room, content);
		linphone_chat_message_set_user_data(message2, file_to_send);
		{
			int dummy=0;
			wait_for_until(marie->lc,pauline->lc,&dummy,1,100); /*just to have time to purge message stored in the server*/
			reset_counters(&marie->stat);
			reset_counters(&pauline->stat);
		}
		cbs = linphone_chat_message_get_callbacks(message);
		linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_send(cbs, file_transfer_send);
		cbs = linphone_chat_message_get_callbacks(message2);
		linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_send(cbs, file_transfer_send);

		linphone_chat_room_send_chat_message(pauline_room,message);
		linphone_chat_room_send_chat_message(pauline_room,message2);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
		message = linphone_chat_message_clone(marie->stat.last_received_chat_message);
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,2));
		message2 = marie->stat.last_received_chat_message;
		fclose(file_to_send);

		BC_ASSERT_EQUAL(ms_list_size(linphone_core_get_chat_rooms(marie->lc)), 1, int, "%d");

		cbs = linphone_chat_message_get_callbacks(message);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
		linphone_chat_message_download_file(message);

		cbs = linphone_chat_message_get_callbacks(message2);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
		linphone_chat_message_download_file(message2);

		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,2));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,2, int, "%d");
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,2, int, "%d");
		BC_ASSERT_TRUE(compare_files(send_filepath, receive_filepath));

		linphone_chat_message_unref(message);
		linphone_content_unref(content);
		linphone_core_manager_destroy(pauline);
		ms_free(send_filepath);
		ms_free(receive_filepath);
		linphone_core_manager_destroy(marie);
	}
}

static void text_message_with_send_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	char *to = linphone_address_as_string(pauline->identity);
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
	BC_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 1, int, "%d");
	BC_ASSERT_PTR_EQUAL(ms_list_nth_data(chat_room->transient_messages,0), message);


	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	/*BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageInProgress,1, int, "%d");*/
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0, int, "%d");

	/* the message should have been discarded from transient list after an error */
	BC_ASSERT_EQUAL(ms_list_size(chat_room->transient_messages), 0, int, "%d");

	sal_set_send_error(marie->lc->sal, 0);

	/*give a chance to register again to allow linphone_core_manager_destroy to properly unregister*/
	linphone_core_refresh_registers(marie->lc);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneRegistrationOk,marie->stat.number_of_LinphoneRegistrationOk + 1));

	ms_free(to);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_denied(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	char *to = linphone_address_as_string(pauline->identity);
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

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0, int, "%d");
	ms_free(to);
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
	LinphoneInfoMessage *info;
	const LinphoneContent *content;
	const char *hvalue;

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	BC_ASSERT_TRUE(call(pauline,marie));

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

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_inforeceived,1));

	BC_ASSERT_PTR_NOT_NULL(pauline->stat.last_received_info_message);
	hvalue=linphone_info_message_get_header(pauline->stat.last_received_info_message, "Weather");
	content=linphone_info_message_get_content(pauline->stat.last_received_info_message);

	BC_ASSERT_PTR_NOT_NULL(hvalue);
	if (hvalue)
		BC_ASSERT_TRUE(strcmp(hvalue,"still bad")==0);

	if (with_content){
		BC_ASSERT_PTR_NOT_NULL(content);
		if (content) {
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_buffer(content));
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_type(content));
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_subtype(content));
			if (linphone_content_get_type(content)) BC_ASSERT_TRUE(strcmp(linphone_content_get_type(content),"application")==0);
			if (linphone_content_get_subtype(content)) BC_ASSERT_TRUE(strcmp(linphone_content_get_subtype(content),"somexml")==0);
			if (linphone_content_get_buffer(content))BC_ASSERT_TRUE(strcmp((const char*)linphone_content_get_buffer(content),info_content)==0);
			BC_ASSERT_EQUAL(linphone_content_get_size(content),strlen(info_content), int, "%d");
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
	char* to;
	LinphoneChatRoom* chat_room;
	int dummy = 0;

	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	to = linphone_address_as_string(marie->identity);
	chat_room = linphone_core_create_chat_room(pauline->lc, to);

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
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, 1));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingIdleReceived, 2));
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
	in=fopen(from, "rb");
	if ( in == NULL )
	{
		ms_error("Can't open %s for reading: %s\n",from,strerror(errno));
		return 1;
	}

	/* Open "to" file for writing (will truncate existing files) */
	out=fopen(to, "wb");
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
	BC_ASSERT_EQUAL(argc, 1, int, "%d");
	BC_ASSERT_STRING_EQUAL(cNames[0], "COUNT(*)"); // count of non updated messages should be 0
	BC_ASSERT_STRING_EQUAL(argv[0], "0"); // count of non updated messages should be 0
	return 0;
}

static void message_storage_migration() {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	char *src_db = bc_tester_res("messages.db");
	char *tmp_db  = bc_tester_file("tmp.db");
	MSList* chatrooms;

	BC_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 0, int, "%d");

	// enable to test the performances of the migration step
	//linphone_core_message_storage_set_debug(marie->lc, TRUE);

	// the messages.db has 10000 dummy messages with the very first DB scheme.
	// This will test the migration procedure
	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	chatrooms = linphone_core_get_chat_rooms(marie->lc);
	BC_ASSERT(ms_list_size(chatrooms) > 0);

	// check that all messages have been migrated to the UTC time storage
	BC_ASSERT(sqlite3_exec(marie->lc->db, "SELECT COUNT(*) FROM history WHERE time != '-1';", check_no_strange_time, NULL, NULL) == SQLITE_OK );

	linphone_core_manager_destroy(marie);
	remove(tmp_db);
	ms_free(src_db);
	ms_free(tmp_db);
}

static void history_message_count_helper(LinphoneChatRoom* chatroom, int x, int y, int expected ){
	MSList* messages = linphone_chat_room_get_history_range(chatroom, x, y);
	int size = ms_list_size(messages);
	if( expected != size ){
		ms_warning("History retrieved from %d to %d returned %d records, but expected %d", x, y, size, expected);
	}
	BC_ASSERT_EQUAL(size, expected, int, "%d");

	ms_list_free_with_data(messages, (void (*)(void *))linphone_chat_message_unref);

}

static void history_range_full_test(){
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *jehan_addr = linphone_address_new("<sip:Jehan@sip.linphone.org>");
	LinphoneChatRoom *chatroom;
	char *src_db = bc_tester_res("messages.db");
	char *tmp_db  = bc_tester_file("tmp.db");

	BC_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 0, int, "%d");

	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	chatroom = linphone_core_get_chat_room(marie->lc, jehan_addr);
	BC_ASSERT_PTR_NOT_NULL(chatroom);
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
	ms_free(src_db);
	ms_free(tmp_db);
}


static void history_messages_count() {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *jehan_addr = linphone_address_new("<sip:Jehan@sip.linphone.org>");
	LinphoneChatRoom *chatroom;
	MSList *messages;
	char *src_db = bc_tester_res("messages.db");
	char *tmp_db  = bc_tester_file("tmp.db");

	BC_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 0, int, "%d");

	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	chatroom = linphone_core_get_chat_room(marie->lc, jehan_addr);
	BC_ASSERT_PTR_NOT_NULL(chatroom);
	if (chatroom){
		messages=linphone_chat_room_get_history(chatroom,10);
		BC_ASSERT_EQUAL(ms_list_size(messages), 10, int, "%d");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		messages=linphone_chat_room_get_history(chatroom,1);
		BC_ASSERT_EQUAL(ms_list_size(messages), 1, int, "%d");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		messages=linphone_chat_room_get_history(chatroom,0);
		BC_ASSERT_EQUAL(linphone_chat_room_get_history_size(chatroom), 1270, int, "%d");
		BC_ASSERT_EQUAL(ms_list_size(messages), 1270, int, "%d");
		/*check the second most recent message*/
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text((LinphoneChatMessage *)messages->next->data), "Fore and aft follow each other.");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test offset+limit: retrieve the 42th latest message only and check its content*/
		messages=linphone_chat_room_get_history_range(chatroom, 42, 42);
		BC_ASSERT_EQUAL(ms_list_size(messages), 1, int, "%d");
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text((LinphoneChatMessage *)messages->data), "If you open yourself to the Tao is intangible and evasive, yet prefers to keep us at the mercy of the kingdom, then all of the streams of hundreds of valleys because of its limitless possibilities.");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test offset without limit*/
		messages = linphone_chat_room_get_history_range(chatroom, 1265, -1);
		BC_ASSERT_EQUAL(ms_list_size(messages), 1270-1265, int, "%d");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test limit without offset*/
		messages = linphone_chat_room_get_history_range(chatroom, 0, 5);
		BC_ASSERT_EQUAL(ms_list_size(messages), 6, int, "%d");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test invalid start*/
		messages = linphone_chat_room_get_history_range(chatroom, 1265, 1260);
		BC_ASSERT_EQUAL(ms_list_size(messages), 1270-1265, int, "%d");
		ms_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);
	}
	linphone_core_manager_destroy(marie);
	linphone_address_destroy(jehan_addr);
	remove(tmp_db);
	ms_free(src_db);
	ms_free(tmp_db);
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
	{ "File transfer message using external body url", file_transfer_using_external_body_url },
	{ "File transfer 2 messages simultaneously", file_transfer_message_two_messages },
	{ "Text message denied", text_message_denied },
	{ "Info message", info_message },
	{ "Info message with body", info_message_with_body },
	{ "IsComposing notification", is_composing_notification }
#ifdef HAVE_LIME
	,{ "Lime Text Message", lime_text_message }
	,{ "Lime File transfer message", lime_file_transfer_message }
	,{ "Lime File transfer message encryption only", lime_file_transfer_message_without_encryption}
	,{ "Lime Unitary", lime_unit }
#endif /* HAVE_LIME */
#ifdef MSG_STORAGE_ENABLED
	,{ "Database migration", message_storage_migration }
	,{ "History count", history_messages_count }
	,{ "History range", history_range_full_test }
#endif
};

test_suite_t message_test_suite = {
	"Message",
	liblinphone_tester_setup,
	NULL,
	sizeof(message_tests) / sizeof(message_tests[0]),
	message_tests
};

