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



#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"
#include "lime.h"

#ifdef SQLITE_STORAGE_ENABLED
#include <sqlite3.h>
#endif

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


static char* message_external_body_url=NULL;

static const char *marie_zid_cache = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>ef7692d0792a67491ae2d44e</selfZID><peer><ZID>005dbe0399643d953a2202dd</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</sndKey><rcvKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</rcvKey><sndSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000078</sndIndex><rcvIndex>000001cf</rcvIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>72d80ab1cad243cf45634980c1d02cfb2df81ce0dd5dfcf1ebeacfc5345a9176</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000000f</sndIndex><rcvIndex>00000000</rcvIndex></peer></cache>";
static const char *pauline_zid_cache = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>005dbe0399643d953a2202dd</selfZID><peer><ZID>ef7692d0792a67491ae2d44e</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><rcvKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</rcvKey><sndKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</sndKey><rcvSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvIndex>00000078</rcvIndex><sndIndex>000001cf</sndIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>81e6e6362c34dc974263d1f77cbb9a8d6d6a718330994379099a8fa19fb12faa</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000002e</sndIndex><rcvIndex>00000000</rcvIndex><pvs>01</pvs></peer></cache>";

void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *msg) {
	stats* counters = get_stats(lc);
	counters->number_of_LinphoneMessageReceivedLegacy++;
}

void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage* msg) {
	char* from=linphone_address_as_string(linphone_chat_message_get_from(msg));
	stats* counters;
	const char *text=linphone_chat_message_get_text(msg);
	const char *external_body_url=linphone_chat_message_get_external_body_url(msg);
	ms_message("Message from [%s]  is [%s] , external URL [%s]",from?from:""
																,text?text:""
																,external_body_url?external_body_url:"");
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_LinphoneMessageReceived++;
	if (counters->last_received_chat_message) linphone_chat_message_unref(counters->last_received_chat_message);
	counters->last_received_chat_message=linphone_chat_message_ref(msg);
	if (linphone_chat_message_get_file_transfer_information(msg)) {
		counters->number_of_LinphoneMessageReceivedWithFile++;
	} else if (linphone_chat_message_get_external_body_url(msg)) {
		counters->number_of_LinphoneMessageExtBodyReceived++;
		if (message_external_body_url) {
			BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_external_body_url(msg),message_external_body_url);
			message_external_body_url=NULL;
		}
	}
}

/**
 * function invoked when a file transfer is received.
 * */
void file_transfer_received(LinphoneChatMessage *msg, const LinphoneContent* content, const LinphoneBuffer *buffer){
	FILE* file=NULL;
	char *receive_file = NULL;
	
	// If a file path is set, we should NOT call the on_recv callback !
	BC_ASSERT_PTR_NULL(msg->file_transfer_filepath);
	
	receive_file = bc_tester_file("receive_file.dump");
	if (!linphone_chat_message_get_user_data(msg)) {
		/*first chunk, creating file*/
		file = fopen(receive_file,"wb");
		linphone_chat_message_set_user_data(msg,(void*)file); /*store fd for next chunks*/
	}
	bc_free(receive_file);
	file = (FILE*)linphone_chat_message_get_user_data(msg);
	BC_ASSERT_PTR_NOT_NULL(file);
	if (linphone_buffer_is_empty(buffer)) { /* tranfer complete */
		linphone_chat_message_set_user_data(msg, NULL);
		fclose(file);
	} else { /* store content on a file*/
		if (fwrite(linphone_buffer_get_content(buffer),linphone_buffer_get_size(buffer),1,file)==0){
			ms_error("file_transfer_received(): write() failed: %s",strerror(errno));
		}
	}
}

/*
 * function called when the file transfer is initiated. file content should be feed into object LinphoneContent
 * */
LinphoneBuffer * tester_file_transfer_send(LinphoneChatMessage *msg, const LinphoneContent* content, size_t offset, size_t size){
	LinphoneBuffer *lb;
	size_t file_size;
	size_t size_to_send;
	uint8_t *buf;
	FILE *file_to_send = linphone_chat_message_get_user_data(msg);
	
	// If a file path is set, we should NOT call the on_send callback !
	BC_ASSERT_PTR_NULL(msg->file_transfer_filepath);

	BC_ASSERT_PTR_NOT_NULL(file_to_send);
	if (file_to_send == NULL){
		return NULL;
	}
	fseek(file_to_send, 0, SEEK_END);
	file_size = ftell(file_to_send);
	fseek(file_to_send, (long)offset, SEEK_SET);
	size_to_send = MIN(size, file_size - offset);
	buf = ms_malloc(size_to_send);
	if (fread(buf, sizeof(uint8_t), size_to_send, file_to_send) != size_to_send){
		// reaching end of file, close it
		fclose(file_to_send);
		linphone_chat_message_set_user_data(msg, NULL);
	}
	lb = linphone_buffer_new_from_data(buf, size_to_send);
	ms_free(buf);
	return lb;
}

/**
 * function invoked to report file transfer progress.
 * */
void file_transfer_progress_indication(LinphoneChatMessage *msg, const LinphoneContent* content, size_t offset, size_t total) {
	LinphoneChatRoom *cr = linphone_chat_message_get_chat_room(msg);
	LinphoneCore *lc = linphone_chat_room_get_core(cr);
	const LinphoneAddress* from_address = linphone_chat_message_get_from(msg);
	const LinphoneAddress* to_address = linphone_chat_message_get_to(msg);
	char *address = linphone_chat_message_is_outgoing(msg)?linphone_address_as_string(to_address):linphone_address_as_string(from_address);
	stats* counters = get_stats(lc);
	int progress = (int)((offset * 100)/total);
	ms_message(" File transfer  [%d%%] %s of type [%s/%s] %s [%s] \n", progress
																	,(linphone_chat_message_is_outgoing(msg)?"sent":"received")
																	, linphone_content_get_type(content)
																	, linphone_content_get_subtype(content)
																	,(linphone_chat_message_is_outgoing(msg)?"to":"from")
																	, address);
	counters->progress_of_LinphoneFileTransfer = progress;
	if (progress == 100) {
		counters->number_of_LinphoneFileTransferDownloadSuccessful++;
	}
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
		case LinphoneChatMessageStateDeliveredToUser:
			counters->number_of_LinphoneMessageDeliveredToUser++;
			return;
		case LinphoneChatMessageStateDisplayed:
			counters->number_of_LinphoneMessageDisplayed++;
			return;
	}
	ms_error("Unexpected state [%s] for msg [%p]",linphone_chat_message_state_to_string(state), msg);
}

void compare_files(const char *path1, const char *path2) {
	size_t size1;
	size_t size2;
	uint8_t *buf1;
	uint8_t *buf2;

	buf1 = (uint8_t*)ms_load_path_content(path1, &size1);
	buf2 = (uint8_t*)ms_load_path_content(path2, &size2);
	BC_ASSERT_PTR_NOT_NULL(buf1);
	BC_ASSERT_PTR_NOT_NULL(buf2);
	BC_ASSERT_EQUAL((uint8_t)size2, (uint8_t)size1, uint8_t, "%u");
	BC_ASSERT_EQUAL(memcmp(buf1, buf2, size1), 0, int, "%d");
	ms_free(buf1);
	ms_free(buf2);
}

LinphoneChatMessage* create_message_from_nowebcam(LinphoneChatRoom *chat_room) {
	FILE *file_to_send = NULL;
	LinphoneChatMessageCbs *cbs;
	LinphoneContent* content;
	LinphoneChatMessage* msg;
	char *send_filepath = bc_tester_res("images/nowebcamVGA.jpg");
	size_t file_size;
	file_to_send = fopen(send_filepath, "rb");
	fseek(file_to_send, 0, SEEK_END);
	file_size = ftell(file_to_send);
	fseek(file_to_send, 0, SEEK_SET);

	content = linphone_core_create_content(chat_room->lc);
	belle_sip_object_set_name(&content->base, "nowebcam content");
	linphone_content_set_type(content,"image");
	linphone_content_set_subtype(content,"jpeg");
	linphone_content_set_size(content,file_size); /*total size to be transfered*/
	linphone_content_set_name(content,"nowebcamVGA.jpg");

	msg = linphone_chat_room_create_file_transfer_message(chat_room, content);
	cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, tester_file_transfer_send);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
	linphone_chat_message_set_user_data(msg, file_to_send);
	BC_ASSERT_PTR_NOT_NULL(linphone_chat_message_get_user_data(msg));

	linphone_content_unref(content);
	ms_free(send_filepath);
	return msg;
}

LinphoneChatMessage* create_file_transfer_message_from_nowebcam(LinphoneChatRoom *chat_room) {
	LinphoneChatMessageCbs *cbs;
	LinphoneContent* content;
	LinphoneChatMessage* msg;
	char *send_filepath = bc_tester_res("images/nowebcamVGA.jpg");

	content = linphone_core_create_content(chat_room->lc);
	belle_sip_object_set_name(&content->base, "nowebcam content");
	linphone_content_set_type(content,"image");
	linphone_content_set_subtype(content,"jpeg");
	linphone_content_set_name(content,"nowebcamVGA.jpg");

	msg = linphone_chat_room_create_file_transfer_message(chat_room, content);
	linphone_chat_message_set_file_transfer_filepath(msg, send_filepath);
	cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, tester_file_transfer_send);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);

	linphone_content_unref(content);
	ms_free(send_filepath);
	return msg;
}

void text_message_base(LinphoneCoreManager* marie, LinphoneCoreManager* pauline) {
	LinphoneChatMessage* msg = linphone_chat_room_create_message(linphone_core_get_chat_room(pauline->lc,marie->identity),"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(msg->chat_room,msg);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
}

/****************************** Tests starting below ******************************/

static void text_message(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	linphone_chat_room_send_message(linphone_core_get_chat_room(pauline->lc,marie->identity), "hello");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedLegacy,1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_within_call_dialog(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	lp_config_set_int(pauline->lc->config,"sip","chat_use_call_dialogs",1);

	BC_ASSERT_TRUE(call(marie,pauline));
	linphone_chat_room_send_message(linphone_core_get_chat_room(pauline->lc, marie->identity),"Bla bla bla bla");

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	// when using call dialogs, we will never receive delivered status
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,0,int,"%d");

	end_call(marie, pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static LinphoneAuthInfo* text_message_with_credential_from_auth_cb_auth_info;
static void text_message_with_credential_from_auth_cb_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
	ms_message("text_message_with_credential_from_auth_callback:Auth info requested  for user id [%s] at realm [%s]\n"
						,username
						,realm);
	linphone_core_add_auth_info(lc,text_message_with_credential_from_auth_cb_auth_info); /*add stored authentication info to LinphoneCore*/
}
static void text_message_with_credential_from_auth_callback(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCoreVTable* vtable = linphone_core_v_table_new();

	/*to force cb to be called*/
	text_message_with_credential_from_auth_cb_auth_info=linphone_auth_info_clone((LinphoneAuthInfo*)(linphone_core_get_auth_info_list(pauline->lc)->data));
	linphone_core_clear_all_auth_info(pauline->lc);
	vtable->auth_info_requested=text_message_with_credential_from_auth_cb_auth_info_requested;
	linphone_core_add_listener(pauline->lc, vtable);

	text_message_base(marie, pauline);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_auth_info_destroy(text_message_with_credential_from_auth_cb_auth_info);
	text_message_with_credential_from_auth_cb_auth_info = NULL;
}

static void text_message_with_privacy(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	linphone_proxy_config_set_privacy(linphone_core_get_default_proxy_config(pauline->lc),LinphonePrivacyId);

	text_message_base(marie, pauline);
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1, int, "%d");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_compatibility_mode(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneProxyConfig* proxy = linphone_core_get_default_proxy_config(marie->lc);
	LinphoneAddress* proxy_address=linphone_address_new(linphone_proxy_config_get_server_addr(proxy));
	char route[256];
	char*tmp;
	/*only keep tcp*/
	LCSipTransports transport = {0,-1,0,0};
	linphone_address_clean(proxy_address);
	tmp=linphone_address_as_string_uri_only(proxy_address);
	linphone_proxy_config_set_server_addr(proxy,tmp);
	sprintf(route,"sip:%s",test_route);
	linphone_proxy_config_set_route(proxy,route);
	ms_free(tmp);
	linphone_address_unref(proxy_address);
	linphone_core_set_sip_transports(marie->lc,&transport);
	marie->stat.number_of_LinphoneRegistrationOk=0;
	BC_ASSERT_TRUE (wait_for(marie->lc,marie->lc,&marie->stat.number_of_LinphoneRegistrationOk,1));

	text_message_base(marie, pauline);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_ack(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	text_message_base(marie, pauline);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_send_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(marie->lc, pauline->identity);
	LinphoneChatMessage* msg = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);

	/*simulate a network error*/
	sal_set_send_error(marie->lc->sal, -1);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,msg);

	/* check transient msg list: the msg should be in it, and should be the only one */
	BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(chat_room->transient_messages), 1, unsigned int, "%u");
	BC_ASSERT_PTR_EQUAL(bctbx_list_nth_data(chat_room->transient_messages,0), msg);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	/*BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageInProgress,1, int, "%d");*/
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0, int, "%d");

	/* the msg should have been discarded from transient list after an error */
	BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(chat_room->transient_messages), 0, unsigned int, "%u");

	sal_set_send_error(marie->lc->sal, 0);

	/*give a chance to register again to allow linphone_core_manager_destroy to properly unregister*/
	linphone_core_refresh_registers(marie->lc);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneRegistrationOk,marie->stat.number_of_LinphoneRegistrationOk + 1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_external_body(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatMessage* msg = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);

	message_external_body_url="http://www.linphone.org";
	linphone_chat_message_set_external_body_url(msg,message_external_body_url);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,msg);

	/* check transient msg list: the msg should be in it, and should be the only one */
	BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(chat_room->transient_messages), 1, unsigned int, "%u");
	BC_ASSERT_PTR_EQUAL(bctbx_list_nth_data(chat_room->transient_messages,0), msg);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1, int, "%d");

	BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(chat_room->transient_messages), 0, unsigned int, "%u");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void transfer_message_base2(LinphoneCoreManager* marie, LinphoneCoreManager* pauline, bool_t upload_error, bool_t download_error, 
							bool_t use_file_body_handler_in_upload, bool_t use_file_body_handler_in_download, bool_t download_from_history) {
	char *send_filepath = bc_tester_res("images/nowebcamVGA.jpg");
	char *receive_filepath = bc_tester_file("receive_file.dump");
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* msg;
	LinphoneChatMessageCbs *cbs;
	bctbx_list_t *msg_list = NULL;

	/* Remove any previously downloaded file */
	remove(receive_filepath);
	
	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

	/* create a file transfer msg */
	if (use_file_body_handler_in_upload) {
		msg = create_file_transfer_message_from_nowebcam(chat_room);
	} else {
		msg = create_message_from_nowebcam(chat_room);
	}
	
	linphone_chat_room_send_chat_message(chat_room,msg);

	if (upload_error) {
		/*wait for file to be 25% uploaded and simulate a network error*/
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.progress_of_LinphoneFileTransfer,25));
		sal_set_send_error(pauline->lc->sal, -1);

		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");

		sal_set_send_error(pauline->lc->sal, 0);

		linphone_core_refresh_registers(pauline->lc); /*to make sure registration is back in registered and so it can be later unregistered*/
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneRegistrationOk,pauline->stat.number_of_LinphoneRegistrationOk+1));
	} else {
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
		if (marie->stat.last_received_chat_message) {
			LinphoneChatMessage *recv_msg;
			if (download_from_history) {
				LinphoneChatRoom *marie_room = linphone_core_get_chat_room(marie->lc, pauline->identity);
				msg_list = linphone_chat_room_get_history(marie_room,1);
				BC_ASSERT_PTR_NOT_NULL(msg_list);
				if (!msg_list)  goto end;
				recv_msg = (LinphoneChatMessage *)msg_list->data;
			} else {
				recv_msg = marie->stat.last_received_chat_message;
			}
			cbs = linphone_chat_message_get_callbacks(recv_msg);
			linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
			linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
			linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
			if (use_file_body_handler_in_download) {
				linphone_chat_message_set_file_transfer_filepath(recv_msg, receive_filepath);
			}
			linphone_chat_message_download_file(recv_msg);

			if (download_error) {
				/* wait for file to be 50% downloaded */
				BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.progress_of_LinphoneFileTransfer, 50));
				/* and simulate network error */
				belle_http_provider_set_recv_error(marie->lc->http_provider, -1);
				BC_ASSERT_TRUE(wait_for_until(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneMessageNotDelivered,1, 10000));
				belle_http_provider_set_recv_error(marie->lc->http_provider, 0);
			} else {
				/* wait for a long time in case the DNS SRV resolution takes times - it should be immediate though */
				if (BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1,55000))) {
					compare_files(send_filepath, receive_filepath);
				}
			}
		}
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,2, int, "%d"); //sent twice because of file transfer
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
	}
end:
	bctbx_list_free_with_data(msg_list, (bctbx_list_free_func)linphone_chat_message_unref);
	remove(receive_filepath);
	ms_free(send_filepath);
	bc_free(receive_filepath);
}

void transfer_message_base(bool_t upload_error, bool_t download_error, bool_t use_file_body_handler_in_upload, 
						   bool_t use_file_body_handler_in_download, bool_t download_from_history) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
		transfer_message_base2(marie,pauline,upload_error,download_error, use_file_body_handler_in_upload, use_file_body_handler_in_download, download_from_history);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}
static void transfer_message(void) {
	transfer_message_base(FALSE, FALSE, FALSE, FALSE, FALSE);
}

static void transfer_message_2(void) {
	transfer_message_base(FALSE, FALSE, TRUE, FALSE, FALSE);
}

static void transfer_message_3(void) {
	transfer_message_base(FALSE, FALSE, FALSE, TRUE, FALSE);
}

static void transfer_message_4(void) {
	transfer_message_base(FALSE, FALSE, TRUE, TRUE, FALSE);
}

static void transfer_message_from_history(void) {
	transfer_message_base(FALSE, FALSE, TRUE, TRUE, TRUE);
}

static void transfer_message_with_upload_io_error(void) {
	transfer_message_base(TRUE, FALSE, FALSE, FALSE, FALSE);
}

static void transfer_message_with_download_io_error(void) {
	transfer_message_base(FALSE, TRUE, FALSE, FALSE, FALSE);
}

static void transfer_message_upload_cancelled(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneChatRoom* chat_room;
		LinphoneChatMessage* msg;
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

		/* Globally configure an http file transfer server. */
		linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

		/* create a chatroom on pauline's side */
		chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

		msg = create_message_from_nowebcam(chat_room);
		linphone_chat_room_send_chat_message(chat_room,msg);

		/*wait for file to be 50% uploaded and cancel the transfer */
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.progress_of_LinphoneFileTransfer, 50));
		linphone_chat_message_cancel_file_transfer(msg);

		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));

		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");

		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

static void transfer_message_download_cancelled(void) {
	LinphoneChatRoom* chat_room;
	LinphoneChatMessage* msg;
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	chat_room = linphone_core_get_chat_room(pauline->lc,marie->identity);
	msg = create_message_from_nowebcam(chat_room);
	linphone_chat_room_send_chat_message(chat_room,msg);

	/* wait for marie to receive pauline's msg */
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));

	if (marie->stat.last_received_chat_message ) { /* get last msg and use it to download file */
		LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(marie->stat.last_received_chat_message);
		linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
		linphone_chat_message_start_file_download(marie->stat.last_received_chat_message, liblinphone_tester_chat_message_state_change, marie->lc);
		/* wait for file to be 50% downloaded */
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.progress_of_LinphoneFileTransfer, 50));
		/* and cancel the transfer */
		linphone_chat_message_cancel_file_transfer(marie->stat.last_received_chat_message);
	}

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,2, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,0, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageNotDelivered,1, int, "%d");

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void file_transfer_using_external_body_url(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
		LinphoneChatRoom *chat_room;
		LinphoneChatMessage *msg;
		LinphoneChatMessageCbs *cbs;
		LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");

		/* make sure lime is disabled */
		linphone_core_enable_lime(marie->lc, LinphoneLimeDisabled);
		linphone_core_enable_lime(pauline->lc, LinphoneLimeDisabled);

		/* create a chatroom on pauline's side */
		chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

		msg = linphone_chat_room_create_message(chat_room, NULL);

		cbs = linphone_chat_message_get_callbacks(msg);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);

		linphone_chat_message_set_external_body_url(msg, "https://www.linphone.org:444//tmp/54ec58280ace9_c30709218df8eaba61d1.jpg");
		linphone_chat_room_send_chat_message(chat_room, msg);

		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
		if (marie->stat.last_received_chat_message) {
			linphone_chat_message_download_file(marie->stat.last_received_chat_message);
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageExtBodyReceived, 1));
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageInProgress, 1));
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

static void file_transfer_2_messages_simultaneously(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneChatRoom* pauline_room;
		LinphoneChatMessage* msg;
		LinphoneChatMessage* msg2;
		LinphoneChatMessageCbs *cbs;
		char *send_filepath = bc_tester_res("images/nowebcamVGA.jpg");
		char *receive_filepath = bc_tester_file("receive_file.dump");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

		/* Remove any previously downloaded file */
		remove(receive_filepath);
	
		/* Globally configure an http file transfer server. */
		linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

		/* create a chatroom on pauline's side */
		pauline_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
		msg = create_message_from_nowebcam(pauline_room);
		msg2 = create_message_from_nowebcam(pauline_room);

		cbs = linphone_chat_message_get_callbacks(msg2);
		linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);

		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(linphone_core_get_chat_rooms(marie->lc)), 0, unsigned int, "%u");
		if (bctbx_list_size(linphone_core_get_chat_rooms(marie->lc)) == 0) {
			linphone_chat_room_send_chat_message(pauline_room,msg);
			linphone_chat_room_send_chat_message(pauline_room,msg2);
			if (BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1))) {
				msg = linphone_chat_message_clone(marie->stat.last_received_chat_message);
				BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,2));
				msg2 = marie->stat.last_received_chat_message;
				BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(linphone_core_get_chat_rooms(marie->lc)), 1, unsigned int, "%u");
				if (bctbx_list_size(linphone_core_get_chat_rooms(marie->lc)) != 1) {
					char * buf = ms_strdup_printf("Found %d rooms instead of 1: ", bctbx_list_size(linphone_core_get_chat_rooms(marie->lc)));
					const bctbx_list_t *it = linphone_core_get_chat_rooms(marie->lc);
					while (it) {
						const LinphoneAddress * peer = linphone_chat_room_get_peer_address(it->data);
						buf = ms_strcat_printf("%s, ", linphone_address_get_username(peer));
						it = it->next;
					}
					ms_error("%s", buf);
				}

				cbs = linphone_chat_message_get_callbacks(msg);
				linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
				linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
				linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
				linphone_chat_message_download_file(msg);

				cbs = linphone_chat_message_get_callbacks(msg2);
				linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
				linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
				linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
				linphone_chat_message_download_file(msg2);

				BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,2));

				BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,4, int, "%d");
				BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,2, int, "%d");
				compare_files(send_filepath, receive_filepath);

				linphone_chat_message_unref(msg);
			}
		}
		linphone_core_manager_destroy(pauline);
		remove(receive_filepath);
		ms_free(send_filepath);
		bc_free(receive_filepath);
		linphone_core_manager_destroy(marie);
	}
}

static void text_message_denied(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(marie->lc, pauline->identity);
	LinphoneChatMessage* msg = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);

	/*pauline doesn't want to be disturbed*/
	linphone_core_disable_chat(pauline->lc,LinphoneReasonDoNotDisturb);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,msg);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0, int, "%d");
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

void info_message_base(bool_t with_content) {
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
	linphone_call_send_info_message(linphone_core_get_current_call(marie->lc),info);
	linphone_info_message_destroy(info);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_inforeceived,1));

	BC_ASSERT_PTR_NOT_NULL(pauline->stat.last_received_info_message);
	hvalue=linphone_info_message_get_header(pauline->stat.last_received_info_message, "Weather");
	content=linphone_info_message_get_content(pauline->stat.last_received_info_message);

	BC_ASSERT_PTR_NOT_NULL(hvalue);
	if (hvalue)
		BC_ASSERT_STRING_EQUAL(hvalue, "still bad");

	if (with_content){
		BC_ASSERT_PTR_NOT_NULL(content);
		if (content) {
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_buffer(content));
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_type(content));
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_subtype(content));
			if (linphone_content_get_type(content)) BC_ASSERT_STRING_EQUAL(linphone_content_get_type(content),"application");
			if (linphone_content_get_subtype(content)) BC_ASSERT_STRING_EQUAL(linphone_content_get_subtype(content),"somexml");
			if (linphone_content_get_buffer(content))BC_ASSERT_STRING_EQUAL((const char*)linphone_content_get_buffer(content),info_content);
			BC_ASSERT_EQUAL((int)linphone_content_get_size(content),(int)strlen(info_content), int, "%d");
		}
	}
	end_call(marie, pauline);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void info_message(void){
	info_message_base(FALSE);
}

static void info_message_with_body(void){
	info_message_base(TRUE);
}

static FILE* fopen_from_write_dir(const char * name, const char * mode) {
	char *filepath = bc_tester_file(name);
	FILE * file = fopen(filepath,mode);
	bc_free(filepath);
	return file;
}

static void _is_composing_notification(bool_t lime_enabled) {
	LinphoneChatRoom* chat_room;
	int dummy = 0;
	char* filepath = NULL;
	FILE *ZIDCacheMarieFD = NULL, *ZIDCachePaulineFD = NULL;

	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	if (lime_enabled) {
		if (!linphone_core_lime_available(marie->lc)) {
			ms_warning("Lime not available, skiping");
			goto end;
		}
		/* make sure lime is enabled */
		linphone_core_enable_lime(marie->lc, LinphoneLimeMandatory);
		linphone_core_enable_lime(pauline->lc, LinphoneLimeMandatory);
		
		/* set the zid caches files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri */
		ZIDCacheMarieFD = fopen_from_write_dir("tmpZIDCacheMarie.xml", "w");
		ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "w");
		fprintf(ZIDCacheMarieFD, marie_zid_cache, linphone_address_as_string_uri_only(pauline->identity), linphone_address_as_string_uri_only(pauline->identity));
		fprintf(ZIDCachePaulineFD, pauline_zid_cache, linphone_address_as_string_uri_only(marie->identity), linphone_address_as_string_uri_only(marie->identity));
		fclose(ZIDCacheMarieFD);
		fclose(ZIDCachePaulineFD);

		filepath = bc_tester_file("tmpZIDCacheMarie.xml");
		linphone_core_set_zrtp_secrets_file(marie->lc, filepath);
		bc_free(filepath);

		filepath = bc_tester_file("tmpZIDCachePauline.xml");
		linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
		bc_free(filepath);
	}
	
	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	linphone_core_get_chat_room(marie->lc, pauline->identity); /*make marie create the chatroom with pauline, which is necessary for receiving the is-composing*/
	linphone_chat_room_compose(chat_room);
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /*just to sleep while iterating*/
	linphone_chat_room_send_message(chat_room, "Composing a msg");
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, 1));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingIdleReceived, 2));
	
end:
	remove("tmpZIDCacheMarie.xml");
	remove("tmpZIDCachePauline.xml");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void is_composing_notification(void) {
	_is_composing_notification(FALSE);
}

static void is_composing_notification_with_lime(void) {
	_is_composing_notification(TRUE);
}

static void imdn_notifications(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneChatRoom *pauline_chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatRoom *marie_chat_room;
	LinphoneChatMessage *sent_cm;
	LinphoneChatMessage *received_cm;
	LinphoneChatMessageCbs *cbs;
	bctbx_list_t *history;

	linphone_im_notif_policy_enable_all(linphone_core_get_im_notif_policy(marie->lc));
	linphone_im_notif_policy_enable_all(linphone_core_get_im_notif_policy(pauline->lc));
	sent_cm = linphone_chat_room_create_message(pauline_chat_room, "Tell me if you get my message");
	linphone_chat_message_ref(sent_cm);
	cbs = linphone_chat_message_get_callbacks(sent_cm);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(pauline_chat_room, sent_cm);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
	marie_chat_room = linphone_core_get_chat_room(marie->lc, pauline->identity);
	history = linphone_chat_room_get_history(marie_chat_room, 1);
	BC_ASSERT_EQUAL((int)bctbx_list_size(history), 1, int, "%d");
	received_cm = (LinphoneChatMessage *)bctbx_list_nth_data(history, 0);
	BC_ASSERT_PTR_NOT_NULL(received_cm);
	if (received_cm != NULL) {
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageDeliveredToUser, 1));
		linphone_chat_room_mark_as_read(marie_chat_room); /* This sends the display notification */
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageDisplayed, 1));
		bctbx_list_free_with_data(history, (bctbx_list_free_func)linphone_chat_message_unref);
	}

	linphone_chat_message_unref(sent_cm);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void im_notification_policy(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneImNotifPolicy *marie_policy = linphone_core_get_im_notif_policy(marie->lc);
	LinphoneImNotifPolicy *pauline_policy = linphone_core_get_im_notif_policy(pauline->lc);
	LinphoneChatRoom *pauline_chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatRoom *marie_chat_room;
	LinphoneChatMessage *msg1;
	LinphoneChatMessage *msg2;
	LinphoneChatMessage *msg3;
	LinphoneChatMessage *msg4;
	LinphoneChatMessageCbs *cbs;
	int dummy = 0;

	linphone_im_notif_policy_enable_all(marie_policy);
	linphone_im_notif_policy_clear(pauline_policy);
	marie_chat_room = linphone_core_get_chat_room(marie->lc, pauline->identity); /* Make marie create the chatroom with pauline, which is necessary for receiving the is-composing */

	/* Test is_composing sending */
	linphone_chat_room_compose(pauline_chat_room);
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /* Just to sleep while iterating */
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneIsComposingActiveReceived, 0, int, "%d");
	linphone_im_notif_policy_set_send_is_composing(pauline_policy, TRUE);
	linphone_chat_room_compose(pauline_chat_room);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, 1));

	/* Test is_composing receiving */
	linphone_chat_room_compose(marie_chat_room);
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /* Just to sleep while iterating */
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneIsComposingActiveReceived, 0, int, "%d");
	linphone_im_notif_policy_set_recv_is_composing(pauline_policy, TRUE);
	linphone_chat_room_compose(marie_chat_room);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneIsComposingActiveReceived, 1));

	/* Test imdn delivered */
	msg1 = linphone_chat_room_create_message(pauline_chat_room, "Happy new year!");
	linphone_chat_message_ref(msg1);
	cbs = linphone_chat_message_get_callbacks(msg1);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(pauline_chat_room, msg1);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /* Just to sleep while iterating */
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDeliveredToUser, 0, int, "%d");
	linphone_im_notif_policy_set_recv_imdn_delivered(pauline_policy, TRUE);
	msg2 = linphone_chat_room_create_message(pauline_chat_room, "I said: Happy new year!");
	linphone_chat_message_ref(msg2);
	cbs = linphone_chat_message_get_callbacks(msg2);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(pauline_chat_room, msg2);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 2));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageDeliveredToUser, 1));
	msg3 = linphone_chat_room_create_message(marie_chat_room, "Thank you! Happy easter to you!");
	linphone_chat_message_ref(msg3);
	cbs = linphone_chat_message_get_callbacks(msg3);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(marie_chat_room, msg3);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageReceived, 1));
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /* Just to sleep while iterating */
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageDeliveredToUser, 0, int, "%d");
	linphone_im_notif_policy_set_send_imdn_delivered(pauline_policy, TRUE);
	msg4 = linphone_chat_room_create_message(marie_chat_room, "Yeah, yeah, I heard that...");
	linphone_chat_message_ref(msg4);
	cbs = linphone_chat_message_get_callbacks(msg4);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(marie_chat_room, msg4);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageReceived, 2));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageDeliveredToUser, 1));

	/* Test imdn displayed */
	linphone_im_notif_policy_set_send_imdn_displayed(pauline_policy, TRUE);
	linphone_chat_room_mark_as_read(pauline_chat_room);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageDisplayed, 2));
	linphone_im_notif_policy_set_recv_imdn_displayed(pauline_policy, TRUE);
	linphone_chat_room_mark_as_read(marie_chat_room);
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageDisplayed, 2));

	linphone_chat_message_unref(msg1);
	linphone_chat_message_unref(msg2);
	linphone_chat_message_unref(msg3);
	linphone_chat_message_unref(msg4);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void _im_error_delivery_notification(bool_t online) {
	FILE *ZIDCacheMarieFD, *ZIDCachePaulineFD;
	LinphoneChatRoom *chat_room;
	char *filepath;
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneChatMessage *msg;
	LinphoneChatMessageCbs *cbs;
	int dummy = 0;

	if (!linphone_core_lime_available(marie->lc)) {
		ms_warning("Lime not available, skiping");
		goto end;
	}

	/* Make sure lime is enabled */
	linphone_core_enable_lime(marie->lc, LinphoneLimeMandatory);
	linphone_core_enable_lime(pauline->lc, LinphoneLimeMandatory);

	/* Set the zid cache files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri. */
	ZIDCacheMarieFD = fopen_from_write_dir("tmpZIDCacheMarie.xml", "w");
	ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "w");
	fprintf(ZIDCacheMarieFD, marie_zid_cache, linphone_address_as_string_uri_only(pauline->identity), linphone_address_as_string_uri_only(pauline->identity));
	fprintf(ZIDCachePaulineFD, pauline_zid_cache, linphone_address_as_string_uri_only(marie->identity), linphone_address_as_string_uri_only(marie->identity));
	fclose(ZIDCacheMarieFD);
	fclose(ZIDCachePaulineFD);

	filepath = bc_tester_file("tmpZIDCacheMarie.xml");
	linphone_core_set_zrtp_secrets_file(marie->lc, filepath);
	bc_free(filepath);

	filepath = bc_tester_file("tmpZIDCachePauline.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
	bc_free(filepath);

	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

	linphone_chat_room_send_message(chat_room, "Bla bla bla bla");
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceivedLegacy, 1));
	BC_ASSERT_PTR_NOT_NULL(marie->stat.last_received_chat_message);
	if (marie->stat.last_received_chat_message) {
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(marie->stat.last_received_chat_message), "Bla bla bla bla");
	}

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc, pauline->identity));

	/* Clear the ZID cache of the receiver of the chat message and enable all IM notifications */
	linphone_core_set_zrtp_secrets_file(marie->lc, NULL);
	linphone_im_notif_policy_enable_all(linphone_core_get_im_notif_policy(marie->lc));
	linphone_im_notif_policy_enable_all(linphone_core_get_im_notif_policy(pauline->lc));

	msg = linphone_chat_room_create_message(chat_room, "Happy new year!");
	linphone_chat_message_ref(msg);
	cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room, msg);
	if (!online) {
		linphone_core_set_network_reachable(marie->lc, FALSE);
		BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageDelivered, 1, 60000));
		linphone_core_set_network_reachable(marie->lc, TRUE);
		BC_ASSERT_TRUE (wait_for(marie->lc, marie->lc, &marie->stat.number_of_LinphoneRegistrationOk, 2));
		wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /* Just to sleep while iterating */
	}
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /* Just to sleep while iterating */
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceived, 1, int, "%d"); /* Check the new message is not considered as received */
	BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageNotDelivered, 1));
	linphone_chat_message_unref(msg);

end:
	remove("tmpZIDCacheMarie.xml");
	remove("tmpZIDCachePauline.xml");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void im_error_delivery_notification_online(void) {
	_im_error_delivery_notification(TRUE);
}

static void im_error_delivery_notification_offline(void) {
	_im_error_delivery_notification(FALSE);
}

static void lime_text_message(void) {
	FILE *ZIDCacheMarieFD, *ZIDCachePaulineFD;
	LinphoneChatRoom* chat_room;
	char* filepath;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	if (!linphone_core_lime_available(marie->lc)) {
		ms_warning("Lime not available, skiping");
		goto end;
	}
	/* make sure lime is enabled */
	linphone_core_enable_lime(marie->lc, LinphoneLimeMandatory);
	linphone_core_enable_lime(pauline->lc, LinphoneLimeMandatory);

	/* set the zid caches files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri */
	ZIDCacheMarieFD = fopen_from_write_dir("tmpZIDCacheMarie.xml", "w");
	ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "w");
	fprintf(ZIDCacheMarieFD, marie_zid_cache, linphone_address_as_string_uri_only(pauline->identity), linphone_address_as_string_uri_only(pauline->identity));
	fprintf(ZIDCachePaulineFD, pauline_zid_cache, linphone_address_as_string_uri_only(marie->identity), linphone_address_as_string_uri_only(marie->identity));
	fclose(ZIDCacheMarieFD);
	fclose(ZIDCachePaulineFD);

	filepath = bc_tester_file("tmpZIDCacheMarie.xml");
	linphone_core_set_zrtp_secrets_file(marie->lc, filepath);
	bc_free(filepath);

	filepath = bc_tester_file("tmpZIDCachePauline.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
	bc_free(filepath);

	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedLegacy,1));
	BC_ASSERT_PTR_NOT_NULL(marie->stat.last_received_chat_message);
	if (marie->stat.last_received_chat_message) {
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(marie->stat.last_received_chat_message), "Bla bla bla bla");
	}

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
	/* TODO : check the msg arrived correctly deciphered */
end:
	remove("tmpZIDCacheMarie.xml");
	remove("tmpZIDCachePauline.xml");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void lime_text_message_to_non_lime(bool_t sender_policy_mandatory, bool_t lime_key_available) {
	FILE *ZIDCachePaulineFD;
	LinphoneChatRoom* chat_room;
	char* filepath;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");

	if (!linphone_core_lime_available(marie->lc)) {
		ms_warning("Lime not available, skiping");
		goto end;
	}
	/* make sure lime is enabled */
	linphone_core_enable_lime(marie->lc, LinphoneLimeDisabled);
	linphone_core_enable_lime(pauline->lc, sender_policy_mandatory ? LinphoneLimeMandatory : LinphoneLimePreferred);

	if (lime_key_available) {
		/* set the zid caches files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri */
		ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "w");
		fprintf(ZIDCachePaulineFD, pauline_zid_cache, linphone_address_as_string_uri_only(marie->identity), linphone_address_as_string_uri_only(marie->identity));
		fclose(ZIDCachePaulineFD);

		filepath = bc_tester_file("tmpZIDCachePauline.xml");
		linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
		bc_free(filepath);
	}

	chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	//since we cannot decrypt message, we should not receive any message
	if (sender_policy_mandatory || lime_key_available) {
		BC_ASSERT_FALSE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
		BC_ASSERT_FALSE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageNotDelivered,1));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy, 0, int, "%d");
	} else {
		BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
		BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy, 1, int, "%d");
	}

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
end:
	remove("tmpZIDCachePauline.xml");
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void lime_text_message_to_non_lime_mandatory_policy(void) {
	lime_text_message_to_non_lime(TRUE, TRUE);
}

static void lime_text_message_to_non_lime_preferred_policy(void) {
	lime_text_message_to_non_lime(FALSE, TRUE);
}

static void lime_text_message_to_non_lime_preferred_policy_2(void) {
	lime_text_message_to_non_lime(FALSE, FALSE);
}

void lime_transfer_message_base(bool_t encrypt_file,bool_t download_file_from_stored_msg, bool_t use_file_body_handler_in_upload, bool_t use_file_body_handler_in_download) {
	FILE *ZIDCacheMarieFD, *ZIDCachePaulineFD;
	LinphoneCoreManager *marie, *pauline;
	LinphoneChatMessage *msg;
	LinphoneChatMessageCbs *cbs;
	char *pauline_id, *marie_id;
	char *filepath;
	char *send_filepath = bc_tester_res("images/nowebcamVGA.jpg");
	char *receive_filepath = bc_tester_file("receive_file.dump");
	MSList * msg_list = NULL;
	
	/* Remove any previously downloaded file */
	remove(receive_filepath);

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_tcp_rc");

	if (!linphone_core_lime_available(marie->lc)) {
		ms_warning("Lime not available, skiping");
		goto end;
	}
	/* make sure lime is enabled */
	linphone_core_enable_lime(marie->lc, LinphoneLimeMandatory);
	linphone_core_enable_lime(pauline->lc, LinphoneLimeMandatory);
	if (!encrypt_file) {
		LpConfig *pauline_lp = linphone_core_get_config(pauline->lc);
		lp_config_set_int(pauline_lp, "sip", "lime_for_file_sharing", 0);
	}

	/* set the zid caches files : create two ZID cache from this valid one inserting the auto-generated sip URI for the peer account as keys in ZID cache are indexed by peer sip uri */
	ZIDCacheMarieFD = fopen_from_write_dir("tmpZIDCacheMarie.xml", "wb");
	ZIDCachePaulineFD = fopen_from_write_dir("tmpZIDCachePauline.xml", "wb");
	pauline_id = linphone_address_as_string_uri_only(pauline->identity);
	marie_id = linphone_address_as_string_uri_only(marie->identity);
	fprintf(ZIDCacheMarieFD, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<cache><selfZID>ef7692d0792a67491ae2d44e</selfZID><peer><ZID>005dbe0399643d953a2202dd</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778cbdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>08df5907d30959b8cb70f6fff2d8febd88fb41b0c8afc39e4b972f86dd5cfe2d</sndKey><rcvKey>60f020a3fe11dc2cc0e1e8ed9341b4cd14944db806ca4fc95456bbe45d95c43a</rcvKey><sndSId>5f9aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>bcffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>00000078</sndIndex><rcvIndex>000001cf</rcvIndex><pvs>01</pvs></peer><peer><ZID>1234567889643d953a2202ee</ZID><rs1>9b5c8f06f3b6c2c695f2dfc3c26f31f5fef8661f8c5fe7c95aeb5c5b0435b045</rs1><aux>f8324dd18ea905171ec2be89f879d01d5994132048d92ea020778csal_set_uuid(lc->sal, account->instance_id);bdf31c605e</aux><rs2>2fdcef69380937c2cf221f7d11526f286c39f49641452ba9012521c705094899</rs2><uri>%s</uri><sndKey>72d80ab1cad243cf45634980c1d02cfb2df81ce0dd5dfcf1ebeacfc5345a9176</sndKey><rcvKey>25d9ac653a83c4559cb0ae7394e7cd3b2d3c57bb28e62068d2df23e8f9b77193</rcvKey><sndSId>f69aa1e5e4c7ec88fa389a9f6b8879b42d3c57bb28e62068d2df23e8f9b77193</sndSId><rcvSId>22ffd51e7316a6c6f53a50fcf01b01bf2d3c57bb28e62068d2df23e8f9b77193</rcvSId><sndIndex>0000000f</sndIndex><rcvIndex>00000000</rcvIndex></peer></cache>", pauline_id, pauline_id);
	fprintf(ZIDCachePaulineFD, pauline_zid_cache, marie_id, marie_id);
	fclose(ZIDCacheMarieFD);
	fclose(ZIDCachePaulineFD);
	ms_free(marie_id);
	ms_free(pauline_id);

	filepath = bc_tester_file("tmpZIDCacheMarie.xml");
	linphone_core_set_zrtp_secrets_file(marie->lc, filepath);
	bc_free(filepath);

	filepath = bc_tester_file("tmpZIDCachePauline.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, filepath);
	bc_free(filepath);

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a file transfer msg */
	if (use_file_body_handler_in_upload) {
		msg = create_file_transfer_message_from_nowebcam(linphone_core_get_chat_room(pauline->lc, marie->identity));
	} else {
		msg = create_message_from_nowebcam(linphone_core_get_chat_room(pauline->lc, marie->identity));
	}

	linphone_chat_room_send_chat_message(msg->chat_room, msg);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceivedWithFile,1));
	if (marie->stat.last_received_chat_message ) {
		LinphoneChatMessage *recv_msg;
		const LinphoneContent* content;
		if (download_file_from_stored_msg) {
			LinphoneChatRoom *marie_room = linphone_core_get_chat_room(marie->lc, pauline->identity);
			msg_list = linphone_chat_room_get_history(marie_room,1);
			BC_ASSERT_PTR_NOT_NULL(msg_list);
			if (!msg_list)  goto end;
			recv_msg = (LinphoneChatMessage *)msg_list->data;
		} else {
			recv_msg = marie->stat.last_received_chat_message;
		}
		cbs = linphone_chat_message_get_callbacks(recv_msg);
		linphone_chat_message_cbs_set_msg_state_changed(cbs, liblinphone_tester_chat_message_msg_state_changed);
		linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
		linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
		content = linphone_chat_message_get_file_transfer_information(recv_msg);
		if (!content) goto end;
		if (encrypt_file)
			BC_ASSERT_PTR_NOT_NULL(linphone_content_get_key(content));
		else
			BC_ASSERT_PTR_NULL(linphone_content_get_key(content));
		
		if (use_file_body_handler_in_download) {
			linphone_chat_message_set_file_transfer_filepath(recv_msg, receive_filepath);
		}
		linphone_chat_message_download_file(recv_msg);
		
		if (BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1,55000))) {
			compare_files(send_filepath, receive_filepath);
		}
		bctbx_list_free_with_data(msg_list, (bctbx_list_free_func)linphone_chat_message_unref);
	}

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,2, int, "%d"); // file transfer
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageDelivered,1, int, "%d");
	BC_ASSERT_EQUAL(marie->stat.number_of_LinphoneFileTransferDownloadSuccessful,1, int, "%d");
end:
	remove("tmpZIDCacheMarie.xml");
	remove("tmpZIDCachePauline.xml");
	remove(receive_filepath);
	ms_free(send_filepath);
	bc_free(receive_filepath);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static  void lime_transfer_message(void) {
	lime_transfer_message_base(TRUE, FALSE, FALSE, FALSE);
}

static  void lime_transfer_message_2(void) {
	lime_transfer_message_base(TRUE, FALSE, TRUE, FALSE);
}

static  void lime_transfer_message_3(void) {
	lime_transfer_message_base(TRUE, FALSE, FALSE, TRUE);
}

static  void lime_transfer_message_4(void) {
	lime_transfer_message_base(TRUE, FALSE, TRUE, TRUE);
}

static  void lime_transfer_message_from_history(void) {
	lime_transfer_message_base(TRUE, TRUE, FALSE, FALSE);
}

static  void lime_transfer_message_without_encryption(void) {
	lime_transfer_message_base(FALSE, FALSE, FALSE, FALSE);
}

static  void lime_transfer_message_without_encryption_2(void) {
	lime_transfer_message_base(FALSE, FALSE, TRUE, FALSE);
}

static void printHex(char *title, uint8_t *data, size_t length) {
	size_t i;
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

static void lime_unit(void) {
	if (lime_is_available()) {
		const char* PLAIN_TEXT_TEST_MESSAGE = "Ceci est un fabuleux msg de test à encrypter";
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
		BC_ASSERT_EQUAL(retval, 0, int, "%d");
		BC_ASSERT_EQUAL(associatedKeys.associatedZIDNumber, 2, int, "%d"); /* there are 2 keys associated to pipo1@pipo.com address in the cache above*/
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
		BC_ASSERT_EQUAL(retval, 0, int, "%d");
		printHex("Got receiver key for ZID", targetZID, 12);
		printHex("Key", associatedKey.key, 32);
		printHex("sessionID", associatedKey.sessionId, 32);
		ms_message("session index %d\n", associatedKey.sessionIndex);

		/* encrypt/decrypt a msg */
		lime_encryptMessage(associatedKeys.peerKeys[0], (uint8_t *)PLAIN_TEXT_TEST_MESSAGE, (uint32_t)strlen(PLAIN_TEXT_TEST_MESSAGE), senderZID, encryptedMessage);
		printHex("Ciphered", encryptedMessage, strlen((char *)encryptedMessage));
		/* invert sender and receiverZID to decrypt/authenticate */
		memcpy(receiverZID, associatedKeys.peerKeys[0]->peerZID, 12);
		memcpy(associatedKeys.peerKeys[0]->peerZID, senderZID, 12);
		retval = lime_decryptMessage(associatedKeys.peerKeys[0], encryptedMessage, (uint32_t)strlen(PLAIN_TEXT_TEST_MESSAGE)+16, receiverZID, plainMessage);
		BC_ASSERT_EQUAL(retval, 0, int, "%d");
		BC_ASSERT_STRING_EQUAL((char *)plainMessage, (char *)PLAIN_TEXT_TEST_MESSAGE);
		ms_message("Decrypt and auth returned %d\nPlain text is %s\n", retval, plainMessage);

		/* update receiver data */
		associatedKey.sessionIndex++;
		associatedKey.key[0]++;
		associatedKey.sessionId[0]++;
		retval = lime_setCachedKey(cacheBuffer, &associatedKey, LIME_RECEIVER);
		BC_ASSERT_EQUAL(retval, 0, int, "%d");

		/* update sender data */
		associatedKeys.peerKeys[0]->sessionIndex++;
		associatedKeys.peerKeys[0]->key[0]++;
		associatedKeys.peerKeys[0]->sessionId[0]++;
		retval = lime_setCachedKey(cacheBuffer, associatedKeys.peerKeys[0], LIME_SENDER);
		BC_ASSERT_EQUAL(retval, 0, int, "%d");

		/* free memory */
		lime_freeKeys(&associatedKeys);

		/* write the file */
		/* dump the xml document into a string */
		xmlDocDumpFormatMemoryEnc(cacheBuffer, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
		/* write it to the file */
		CACHE = fopen_from_write_dir("ZIDCache.xml", "w+");
		fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
		xmlFree(xmlStringOutput);
		fclose(CACHE);
		xmlFreeDoc(cacheBuffer);

		/**** Higher level tests using 2 caches to encrypt/decrypt a msg ****/
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



		/* encrypt a msg */
		retval = lime_createMultipartMessage(cacheBufferAlice, (uint8_t *)PLAIN_TEXT_TEST_MESSAGE, (uint8_t *)"sip:pauline@sip.example.org", &multipartMessage);

		BC_ASSERT_EQUAL(retval, 0, int, "%d");
		if (retval == 0) {
			ms_message("Encrypted msg created is %s", multipartMessage);
		}

		/* decrypt the multipart msg */
		retval = lime_decryptMultipartMessage(cacheBufferBob, multipartMessage, &decryptedMessage);

		BC_ASSERT_EQUAL(retval, 0, int, "%d");
		if (retval == 0) {
			BC_ASSERT_STRING_EQUAL((char *)decryptedMessage, (char *)PLAIN_TEXT_TEST_MESSAGE);
			ms_message("Succesfully decrypted msg is %s", decryptedMessage);
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
		remove("ZIDCache.xml");
		remove("ZIDCacheAlice.xml");
		remove("ZIDCacheBob.xml");
	} else {
		ms_warning("Lime not available, skiping");
	}
}

#ifdef SQLITE_STORAGE_ENABLED

/*
 * Copy file "from" to file "to".
 * Destination file is truncated if existing.
 * Return 0 on success, positive value on error.
 */
int message_tester_copy_file(const char *from, const char *to)
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
	while ( (n=fread(buf, sizeof(char), sizeof(buf), in)) > 0 )
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

int check_no_strange_time(void* data,int argc, char** argv,char** cNames) {
	BC_ASSERT_EQUAL(argc, 1, int, "%d");
	BC_ASSERT_STRING_EQUAL(cNames[0], "COUNT(*)"); // count of non updated messages should be 0
	BC_ASSERT_STRING_EQUAL(argv[0], "0"); // count of non updated messages should be 0
	return 0;
}

void history_message_count_helper(LinphoneChatRoom* chatroom, int x, int y, unsigned int expected ){
	bctbx_list_t* messages = linphone_chat_room_get_history_range(chatroom, x, y);
	BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), expected, unsigned int, "%u");
	bctbx_list_free_with_data(messages, (void (*)(void *))linphone_chat_message_unref);
}

static void database_migration(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	char *src_db = bc_tester_res("messages.db");
	char *tmp_db  = bc_tester_file("tmp.db");
	const bctbx_list_t* chatrooms;
	LinphoneChatRoom *cr;

	BC_ASSERT_EQUAL(message_tester_copy_file(src_db, tmp_db), 0, int, "%d");

	// enable to test the performances of the migration step
	//linphone_core_message_storage_set_debug(marie->lc, TRUE);

	// the messages.db has 10000 dummy messages with the very first DB scheme.
	// This will test the migration procedure
	linphone_core_set_chat_database_path(marie->lc, tmp_db);
	BC_ASSERT_PTR_NOT_NULL(marie->lc->db);
	if (!marie->lc->db) goto end;

	chatrooms = linphone_core_get_chat_rooms(marie->lc);
	BC_ASSERT(bctbx_list_size(chatrooms) > 0);

	// check that all messages have been migrated to the UTC time storage
	BC_ASSERT(sqlite3_exec(marie->lc->db, "SELECT COUNT(*) FROM history WHERE time != '-1';", check_no_strange_time, NULL, NULL) == SQLITE_OK);

	// check that the read messages (field read=1) has been migrated to the LinphoneChatMessageStateDisplayed state
	cr = linphone_core_get_chat_room_from_uri(marie->lc, "sip:Marielle@sip.linphone.org");
	BC_ASSERT_EQUAL(linphone_chat_room_get_unread_messages_count(cr), 8, int, "%i");

end:
	linphone_core_manager_destroy(marie);
	remove(tmp_db);
	ms_free(src_db);
	bc_free(tmp_db);
}

static void history_range(void){
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *jehan_addr = linphone_address_new("<sip:Jehan@sip.linphone.org>");
	LinphoneChatRoom *chatroom;
	char *src_db = bc_tester_res("messages.db");
	char *tmp_db  = bc_tester_file("tmp.db");

	BC_ASSERT_EQUAL(message_tester_copy_file(src_db, tmp_db), 0, int, "%d");

	linphone_core_set_chat_database_path(marie->lc, tmp_db);
	BC_ASSERT_PTR_NOT_NULL(marie->lc->db);
	if (!marie->lc->db) goto end;

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

end:
	linphone_core_manager_destroy(marie);
	linphone_address_unref(jehan_addr);
	remove(tmp_db);
	ms_free(src_db);
	bc_free(tmp_db);
}

static void history_count(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneAddress *jehan_addr = linphone_address_new("<sip:Jehan@sip.linphone.org>");
	LinphoneChatRoom *chatroom;
	bctbx_list_t *messages;
	char *src_db = bc_tester_res("messages.db");
	char *tmp_db  = bc_tester_file("tmp.db");

	BC_ASSERT_EQUAL(message_tester_copy_file(src_db, tmp_db), 0, int, "%d");

	linphone_core_set_chat_database_path(marie->lc, tmp_db);
	BC_ASSERT_PTR_NOT_NULL(marie->lc->db);
	if (!marie->lc->db) goto end;

	chatroom = linphone_core_get_chat_room(marie->lc, jehan_addr);
	BC_ASSERT_PTR_NOT_NULL(chatroom);
	if (chatroom){
		messages=linphone_chat_room_get_history(chatroom,10);
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 10, unsigned int, "%u");
		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		messages=linphone_chat_room_get_history(chatroom,1);
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 1, unsigned int, "%u");
		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		messages=linphone_chat_room_get_history(chatroom,0);
		BC_ASSERT_EQUAL(linphone_chat_room_get_history_size(chatroom), 1270, int, "%d");
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 1270, unsigned int, "%u");

		/*check the second most recent msg*/
		BC_ASSERT_PTR_NOT_NULL(messages);
		if (messages){
			BC_ASSERT_PTR_NOT_NULL(messages->next->data);
			if (messages->next->data){
				BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text((LinphoneChatMessage *)messages->next->data), "Fore and aft follow each other.");
			}
		}

		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test offset+limit: retrieve the 42th latest msg only and check its content*/
		messages=linphone_chat_room_get_history_range(chatroom, 42, 42);
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 1, unsigned int, "%u");
		BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text((LinphoneChatMessage *)messages->data), "If you open yourself to the Tao is intangible and evasive, yet prefers to keep us at the mercy of the kingdom, then all of the streams of hundreds of valleys because of its limitless possibilities.");
		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test offset without limit*/
		messages = linphone_chat_room_get_history_range(chatroom, 1265, -1);
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 1270-1265, unsigned int, "%u");
		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test limit without offset*/
		messages = linphone_chat_room_get_history_range(chatroom, 0, 5);
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 6, unsigned int, "%u");
		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);

		/*test invalid start*/
		messages = linphone_chat_room_get_history_range(chatroom, 1265, 1260);
		BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(messages), 1270-1265, unsigned int, "%u");
		bctbx_list_free_with_data(messages, (void (*)(void*))linphone_chat_message_unref);
	}

end:
	linphone_core_manager_destroy(marie);
	linphone_address_unref(jehan_addr);
	remove(tmp_db);
	ms_free(src_db);
	bc_free(tmp_db);
}
#endif

static void text_status_after_destroying_chat_room(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneChatRoom *chatroom = linphone_core_get_chat_room_from_uri(marie->lc, "<sip:Jehan@sip.linphone.org>");
	LinphoneChatMessage *msg = linphone_chat_room_create_message(chatroom, "hello");
	linphone_chat_room_send_chat_message(chatroom, msg);
	linphone_core_delete_chat_room(marie->lc, chatroom);
	//since message is orphan, we do not expect to be notified of state change
	BC_ASSERT_FALSE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneMessageNotDelivered, 1, 1000));
	linphone_core_manager_destroy(marie);
}


static void file_transfer_not_sent_if_invalid_url(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneChatRoom *chatroom = linphone_core_get_chat_room_from_uri(marie->lc, "<sip:Jehan@sip.linphone.org>");
	LinphoneChatMessage *msg = create_message_from_nowebcam(chatroom);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_core_set_file_transfer_server(marie->lc, "INVALID URL");
	linphone_chat_room_send_chat_message(chatroom, msg);
	BC_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneMessageNotDelivered, 1, 1000));
	linphone_core_manager_destroy(marie);
}

void file_transfer_io_error_base(char *server_url, bool_t destroy_room) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneChatRoom *chatroom = linphone_core_get_chat_room_from_uri(marie->lc, "<sip:Jehan@sip.linphone.org>");
	LinphoneChatMessage *msg = create_message_from_nowebcam(chatroom);
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_core_set_file_transfer_server(marie->lc, server_url);
	linphone_chat_room_send_chat_message(chatroom, msg);
	BC_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneMessageInProgress, 1, 1000));
	if (destroy_room) {
		//since message is orphan, we do not expect to be notified of state change
		linphone_core_delete_chat_room(marie->lc, chatroom);
		BC_ASSERT_FALSE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneMessageNotDelivered, 1, 1000));
	} else {
		BC_ASSERT_TRUE(wait_for_until(marie->lc, NULL, &marie->stat.number_of_LinphoneMessageNotDelivered, 1, 3000));
	}
	linphone_core_manager_destroy(marie);
}

static void file_transfer_not_sent_if_host_not_found(void) {
	file_transfer_io_error_base("https://not-existing-url.com", FALSE);
}

static void file_transfer_not_sent_if_url_moved_permanently(void) {
	file_transfer_io_error_base("http://linphone.org/toto.php", FALSE);
}

static void file_transfer_io_error_after_destroying_chatroom(void) {
	file_transfer_io_error_base("https://www.linphone.org:444/lft.php", TRUE);
}

static void real_time_text(bool_t audio_stream_enabled, bool_t srtp_enabled, bool_t mess_with_marie_payload_number, bool_t mess_with_pauline_payload_number,
						   bool_t ice_enabled, bool_t sql_storage, bool_t do_not_store_rtt_messages_in_sql_storage) {
	LinphoneChatRoom *pauline_chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallParams *marie_params = NULL;
	LinphoneCall *pauline_call, *marie_call;
	char *marie_db  = bc_tester_file("marie.db");
	char *pauline_db  = bc_tester_file("pauline.db");

	if (sql_storage) {
		linphone_core_set_chat_database_path(marie->lc, marie_db);
		linphone_core_set_chat_database_path(pauline->lc, pauline_db);
#ifdef SQLITE_STORAGE_ENABLED
		BC_ASSERT_PTR_NOT_NULL(marie->lc->db);
		BC_ASSERT_PTR_NOT_NULL(pauline->lc->db);
#endif
			
		if (do_not_store_rtt_messages_in_sql_storage) {
			lp_config_set_int(marie->lc->config, "misc", "store_rtt_messages", 0);
			lp_config_set_int(pauline->lc->config, "misc", "store_rtt_messages", 0);
		}
	}

	if (mess_with_marie_payload_number) {
		bctbx_list_t *elem;
		for (elem = marie->lc->codecs_conf.text_codecs; elem != NULL; elem = elem->next) {
			PayloadType *pt = (PayloadType*)elem->data;
			if (strcasecmp(pt->mime_type, payload_type_t140.mime_type) == 0) {
				payload_type_set_number(pt, 99);
				break;
			}
		}
	} else if (mess_with_pauline_payload_number) {
		bctbx_list_t *elem;
		for (elem = pauline->lc->codecs_conf.text_codecs; elem != NULL; elem = elem->next) {
			PayloadType *pt = (PayloadType*)elem->data;
			if (strcasecmp(pt->mime_type, payload_type_t140.mime_type) == 0) {
				payload_type_set_number(pt, 99);
				break;
			}
		}
	}

	if (ice_enabled) {
		linphone_core_set_firewall_policy(marie->lc, LinphonePolicyUseIce);
		linphone_core_set_firewall_policy(pauline->lc, LinphonePolicyUseIce);
	}

	if (srtp_enabled) {
		if (!ms_srtp_supported()) {
			ms_warning("test skipped, missing srtp support");
			goto srtp_end;
		}
		BC_ASSERT_TRUE(linphone_core_media_encryption_supported(marie->lc, LinphoneMediaEncryptionSRTP));
		linphone_core_set_media_encryption(marie->lc, LinphoneMediaEncryptionSRTP);
		linphone_core_set_media_encryption(pauline->lc, LinphoneMediaEncryptionSRTP);
		linphone_core_set_media_encryption_mandatory(marie->lc, TRUE);
		linphone_core_set_media_encryption_mandatory(pauline->lc, TRUE);
	}

	marie_params = linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_enable_realtime_text(marie_params,TRUE);
	if (!audio_stream_enabled) {
		linphone_call_params_enable_audio(marie_params,FALSE);
		linphone_core_set_nortp_timeout(marie->lc, 5);
		linphone_core_set_nortp_timeout(pauline->lc, 5);
	}

	BC_ASSERT_TRUE(call_with_caller_params(marie, pauline, marie_params));
	pauline_call = linphone_core_get_current_call(pauline->lc);
	marie_call = linphone_core_get_current_call(marie->lc);
	if (pauline_call) {
		BC_ASSERT_TRUE(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(pauline_call)));
		if (audio_stream_enabled) {
			BC_ASSERT_TRUE(linphone_call_params_audio_enabled(linphone_call_get_current_params(pauline_call)));
		}

		pauline_chat_room = linphone_call_get_chat_room(pauline_call);
		BC_ASSERT_PTR_NOT_NULL(pauline_chat_room);
		if (pauline_chat_room) {
			const char* message = "Be l3l";
			size_t i;
			LinphoneChatMessage* rtt_message = linphone_chat_room_create_message(pauline_chat_room,NULL);
			LinphoneChatRoom *marie_chat_room = linphone_call_get_chat_room(marie_call);

			for (i = 0; i < strlen(message); i++) {
				BC_ASSERT_FALSE(linphone_chat_message_put_char(rtt_message, message[i]));
				BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, (int)i+1, 1000));
				BC_ASSERT_EQUAL(linphone_chat_room_get_char(marie_chat_room), message[i], char, "%c");
			}
			linphone_chat_room_send_chat_message(pauline_chat_room, rtt_message);
			BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));

			if (sql_storage) {
				bctbx_list_t *marie_messages = linphone_chat_room_get_history(marie_chat_room, 0);
				bctbx_list_t *pauline_messages = linphone_chat_room_get_history(pauline_chat_room, 0);
				LinphoneChatMessage *marie_msg = NULL;
				LinphoneChatMessage *pauline_msg = NULL;
				if (do_not_store_rtt_messages_in_sql_storage) {
					BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(marie_messages), 0, unsigned int , "%u");
					BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(pauline_messages), 0, unsigned int , "%u");
				} else {
					BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(marie_messages), 1, unsigned int , "%u");
					BC_ASSERT_EQUAL((unsigned int)bctbx_list_size(pauline_messages), 1, unsigned int , "%u");
					if (!marie_messages || !pauline_messages) {
						goto end;
					}
					marie_msg = (LinphoneChatMessage *)marie_messages->data;
					pauline_msg = (LinphoneChatMessage *)pauline_messages->data;
					BC_ASSERT_STRING_EQUAL(marie_msg->message, message);
					BC_ASSERT_STRING_EQUAL(pauline_msg->message, message);
					bctbx_list_free_with_data(marie_messages, (void (*)(void *))linphone_chat_message_unref);
					bctbx_list_free_with_data(pauline_messages, (void (*)(void *))linphone_chat_message_unref);
				}
			}
		}

		if (!audio_stream_enabled) {
			int dummy = 0;
			wait_for_until(pauline->lc, marie->lc, &dummy, 1, 7000); /* Wait to see if call is dropped after the nortp_timeout */
			BC_ASSERT_FALSE(marie->stat.number_of_LinphoneCallEnd > 0);
			BC_ASSERT_FALSE(pauline->stat.number_of_LinphoneCallEnd > 0);
		}

		if (ice_enabled) {
			BC_ASSERT_TRUE(check_ice(pauline,marie,LinphoneIceStateHostConnection));
		}

end:
		end_call(marie, pauline);
	}
	linphone_call_params_unref(marie_params);
srtp_end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	remove(marie_db);
	bc_free(marie_db);
	remove(pauline_db);
	bc_free(pauline_db);
}

static void real_time_text_message(void) {
	real_time_text(TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
}

static void real_time_text_sql_storage(void) {
	real_time_text(TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE);
}

static void real_time_text_sql_storage_rtt_disabled(void) {
	real_time_text(TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE);
}

static void real_time_text_conversation(void) {
	LinphoneChatRoom *pauline_chat_room, *marie_chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallParams *marie_params = linphone_core_create_call_params(marie->lc, NULL);
	LinphoneCall *pauline_call, *marie_call;
	linphone_call_params_enable_realtime_text(marie_params,TRUE);

	BC_ASSERT_TRUE(call_with_caller_params(marie, pauline, marie_params));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	marie_call=linphone_core_get_current_call(marie->lc);
	BC_ASSERT_TRUE(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(pauline_call)));

	pauline_chat_room = linphone_call_get_chat_room(pauline_call);
	BC_ASSERT_PTR_NOT_NULL(pauline_chat_room);
	marie_chat_room = linphone_call_get_chat_room(marie_call);
	BC_ASSERT_PTR_NOT_NULL(pauline_chat_room);
	if (pauline_chat_room && marie_chat_room) {
		const char* message1_1 = "Lorem";
		const char* message1_2 = "Ipsum";
		const char* message2_1 = "Be lle Com";
		const char* message2_2 = "eB ell moC";
		size_t i;
		LinphoneChatMessage* pauline_rtt_message = linphone_chat_room_create_message(pauline_chat_room,NULL);
		LinphoneChatMessage* marie_rtt_message = linphone_chat_room_create_message(marie_chat_room,NULL);

		for (i = 0; i < strlen(message1_1); i++) {
			linphone_chat_message_put_char(pauline_rtt_message, message1_1[i]);
			BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, (int)i+1, 1000));
			BC_ASSERT_EQUAL(linphone_chat_room_get_char(marie_chat_room), message1_1[i], char, "%c");

			linphone_chat_message_put_char(marie_rtt_message, message1_2[i]);
			BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneIsComposingActiveReceived, (int)i+1, 1000));
			BC_ASSERT_EQUAL(linphone_chat_room_get_char(pauline_chat_room), message1_2[i], char, "%c");
		}

		/*Commit the message, triggers a NEW LINE in T.140 */
		linphone_chat_room_send_chat_message(pauline_chat_room, pauline_rtt_message);
		linphone_chat_room_send_chat_message(marie_chat_room, marie_rtt_message);

		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
		{
			LinphoneChatMessage * msg = marie->stat.last_received_chat_message;
			BC_ASSERT_PTR_NOT_NULL(msg);
			if (msg) {
				BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(msg), message1_1);
			}
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageReceived, 1));
		{
			LinphoneChatMessage * msg = pauline->stat.last_received_chat_message;
			BC_ASSERT_PTR_NOT_NULL(msg);
			if (msg) {
				BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(msg), message1_2);
			}
		}

		reset_counters(&pauline->stat);
		reset_counters(&marie->stat);
		pauline_rtt_message = linphone_chat_room_create_message(pauline_chat_room,NULL);
		marie_rtt_message = linphone_chat_room_create_message(marie_chat_room,NULL);

		for (i = 0; i < strlen(message2_1); i++) {
			linphone_chat_message_put_char(pauline_rtt_message, message2_1[i]);
			BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, (int)i+1, 1000));
			BC_ASSERT_EQUAL(linphone_chat_room_get_char(marie_chat_room), message2_1[i], char, "%c");

			linphone_chat_message_put_char(marie_rtt_message, message2_2[i]);
			BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneIsComposingActiveReceived, (int)i+1, 1000));
			BC_ASSERT_EQUAL(linphone_chat_room_get_char(pauline_chat_room), message2_2[i], char, "%c");
		}

		/*Commit the message, triggers a NEW LINE in T.140 */
		linphone_chat_room_send_chat_message(pauline_chat_room, pauline_rtt_message);
		linphone_chat_room_send_chat_message(marie_chat_room, marie_rtt_message);

		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
		{
			LinphoneChatMessage * msg = marie->stat.last_received_chat_message;
			BC_ASSERT_PTR_NOT_NULL(msg);
			if (msg) {
				BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(msg), message2_1);
			}
		}
		BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &pauline->stat.number_of_LinphoneMessageReceived, 1));
		{
			LinphoneChatMessage * msg = pauline->stat.last_received_chat_message;
			BC_ASSERT_PTR_NOT_NULL(msg);
			if (msg) {
				BC_ASSERT_STRING_EQUAL(linphone_chat_message_get_text(msg), message2_2);
			}
		}
	}
	end_call(marie, pauline);
	linphone_call_params_unref(marie_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void real_time_text_without_audio(void) {
	real_time_text(FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE);
}

static void real_time_text_srtp(void) {
	real_time_text(TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE);
}

static void real_time_text_ice(void) {
	real_time_text(TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE);
}

static void real_time_text_message_compat(bool_t end_with_crlf, bool_t end_with_lf) {
	LinphoneChatRoom *pauline_chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallParams *marie_params = NULL;
	LinphoneCall *pauline_call, *marie_call;

	marie_params = linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_enable_realtime_text(marie_params,TRUE);

	BC_ASSERT_TRUE(call_with_caller_params(marie, pauline, marie_params));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	marie_call=linphone_core_get_current_call(marie->lc);
	if (pauline_call) {
		BC_ASSERT_TRUE(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(pauline_call)));

		pauline_chat_room = linphone_call_get_chat_room(pauline_call);
		BC_ASSERT_PTR_NOT_NULL(pauline_chat_room);
		if (pauline_chat_room) {
			const char* message = "Be l3l";
			size_t i;
			LinphoneChatMessage* rtt_message = linphone_chat_room_create_message(pauline_chat_room,NULL);
			LinphoneChatRoom *marie_chat_room = linphone_call_get_chat_room(marie_call);
			uint32_t crlf = 0x0D0A;
			uint32_t lf = 0x0A;

			for (i = 0; i < strlen(message); i++) {
				linphone_chat_message_put_char(rtt_message, message[i]);
				BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, (int)i+1, 1000));
				BC_ASSERT_EQUAL(linphone_chat_room_get_char(marie_chat_room), message[i], char, "%c");
			}

			if (end_with_crlf) {
				linphone_chat_message_put_char(rtt_message, crlf);
			} else if (end_with_lf) {
				linphone_chat_message_put_char(rtt_message, lf);
			}
			BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, (int)strlen(message), 1000));
			BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
			linphone_chat_message_unref(rtt_message);
		}
		end_call(marie, pauline);
	}
	linphone_call_params_unref(marie_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void real_time_text_message_compat_crlf(void) {
	real_time_text_message_compat(TRUE, FALSE);
}

static void real_time_text_message_compat_lf(void) {
	real_time_text_message_compat(FALSE, TRUE);
}

static void real_time_text_message_accented_chars(void) {
	LinphoneChatRoom *pauline_chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallParams *marie_params = NULL;
	LinphoneCall *pauline_call, *marie_call;

	marie_params = linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_enable_realtime_text(marie_params,TRUE);

	BC_ASSERT_TRUE(call_with_caller_params(marie, pauline, marie_params));
	pauline_call=linphone_core_get_current_call(pauline->lc);
	marie_call=linphone_core_get_current_call(marie->lc);
	if (pauline_call) {
		BC_ASSERT_TRUE(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(pauline_call)));

		pauline_chat_room = linphone_call_get_chat_room(pauline_call);
		BC_ASSERT_PTR_NOT_NULL(pauline_chat_room);
		if (pauline_chat_room) {
			LinphoneChatMessage* rtt_message = linphone_chat_room_create_message(pauline_chat_room,NULL);
			LinphoneChatRoom *marie_chat_room = linphone_call_get_chat_room(marie_call);
			int i;
			uint32_t message[8];
			int message_len = 8;

			message[0] = 0xE3; // ã
			message[1] = 0xE6; // æ
			message[2] = 0xE7; // ç
			message[3] = 0xE9; // é
			message[4] = 0xEE; // î
			message[5] = 0xF8; // ø
			message[6] = 0xF9; // ù
			message[7] = 0xFF; // ÿ
			for (i = 0; i < message_len; i++) {
				linphone_chat_message_put_char(rtt_message, message[i]);
				BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, i+1, 1000));
				BC_ASSERT_EQUAL(linphone_chat_room_get_char(marie_chat_room), message[i], unsigned long, "%lu");
			}

			linphone_chat_room_send_chat_message(pauline_chat_room, rtt_message);
			BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
			BC_ASSERT_EQUAL(strcmp(marie->stat.last_received_chat_message->message, "ãæçéîøùÿ"), 0, int, "%i");
		}
		end_call(marie, pauline);
	}
	linphone_call_params_unref(marie_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void real_time_text_message_different_text_codecs_payload_numbers_sender_side(void) {
	real_time_text(FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE);
}

static void real_time_text_message_different_text_codecs_payload_numbers_receiver_side(void) {
	real_time_text(FALSE, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE);
}

static void real_time_text_copy_paste(void) {
	LinphoneChatRoom *pauline_chat_room;
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneCallParams *marie_params = NULL;
	LinphoneCall *pauline_call, *marie_call;

	marie_params = linphone_core_create_call_params(marie->lc, NULL);
	linphone_call_params_enable_realtime_text(marie_params,TRUE);

	BC_ASSERT_TRUE(call_with_caller_params(marie, pauline, marie_params));
	pauline_call = linphone_core_get_current_call(pauline->lc);
	marie_call = linphone_core_get_current_call(marie->lc);
	if (pauline_call) {
		BC_ASSERT_TRUE(linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(pauline_call)));

		pauline_chat_room = linphone_call_get_chat_room(pauline_call);
		BC_ASSERT_PTR_NOT_NULL(pauline_chat_room);
		if (pauline_chat_room) {
			const char* message = "Be l3l";
			size_t i;
			LinphoneChatMessage* rtt_message = linphone_chat_room_create_message(pauline_chat_room,NULL);
			LinphoneChatRoom *marie_chat_room = linphone_call_get_chat_room(marie_call);

			for (i = 1; i <= strlen(message); i++) {
				linphone_chat_message_put_char(rtt_message, message[i-1]);
				if (i % 4 == 0) {
					int j;
					BC_ASSERT_TRUE(wait_for_until(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, (int)i, 1000));
					for (j = 4; j > 0; j--) {
						BC_ASSERT_EQUAL(linphone_chat_room_get_char(marie_chat_room), message[i-j], char, "%c");
					}
				}
			}
			linphone_chat_room_send_chat_message(pauline_chat_room, rtt_message);
			BC_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneMessageReceived, 1));
		}

		end_call(marie, pauline);
	}
	linphone_call_params_unref(marie_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void file_transfer_with_http_proxy(void) {
	if (transport_supported(LinphoneTransportTls)) {
		LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
		LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
		linphone_core_set_http_proxy_host(marie->lc, "sip.linphone.org");
		transfer_message_base2(marie,pauline,FALSE,FALSE,FALSE,FALSE,FALSE);
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
	}
}

void chat_message_custom_headers(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_tcp_rc");
	LinphoneChatRoom* chat_room = linphone_core_get_chat_room(pauline->lc, marie->identity);
	LinphoneChatMessage* msg = linphone_chat_room_create_message(chat_room, "Lorem Ipsum");
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(msg);

	linphone_chat_message_add_custom_header(msg, "Test1", "Value1");
	linphone_chat_message_add_custom_header(msg, "Test2", "Value2");
	linphone_chat_message_remove_custom_header(msg, "Test1");

	linphone_chat_message_cbs_set_msg_state_changed(cbs,liblinphone_tester_chat_message_msg_state_changed);
	linphone_chat_room_send_chat_message(chat_room,msg);

	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
	
	if (marie->stat.last_received_chat_message) {
		const char *header = linphone_chat_message_get_custom_header(marie->stat.last_received_chat_message, "Test2");
		BC_ASSERT_STRING_EQUAL(header, "Value2");
		header = linphone_chat_message_get_custom_header(marie->stat.last_received_chat_message, "Test1");
		BC_ASSERT_PTR_NULL(header);
		BC_ASSERT_STRING_EQUAL(marie->stat.last_received_chat_message->message, "Lorem Ipsum");
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t message_tests[] = {
	TEST_NO_TAG("Text message", text_message),
	TEST_NO_TAG("Text message within call dialog", text_message_within_call_dialog),
	TEST_NO_TAG("Text message with credentials from auth callback", text_message_with_credential_from_auth_callback),
	TEST_NO_TAG("Text message with privacy", text_message_with_privacy),
	TEST_NO_TAG("Text message compatibility mode", text_message_compatibility_mode),
	TEST_NO_TAG("Text message with ack", text_message_with_ack),
	TEST_NO_TAG("Text message with send error", text_message_with_send_error),
	TEST_NO_TAG("Text message with external body", text_message_with_external_body),
	TEST_NO_TAG("Transfer message", transfer_message),
	TEST_NO_TAG("Transfer message 2", transfer_message_2),
	TEST_NO_TAG("Transfer message 3", transfer_message_3),
	TEST_NO_TAG("Transfer message 4", transfer_message_4),
	TEST_NO_TAG("Transfer message from history", transfer_message_from_history),
	TEST_NO_TAG("Transfer message with http proxy", file_transfer_with_http_proxy),
	TEST_NO_TAG("Transfer message with upload io error", transfer_message_with_upload_io_error),
	TEST_NO_TAG("Transfer message with download io error", transfer_message_with_download_io_error),
	TEST_NO_TAG("Transfer message upload cancelled", transfer_message_upload_cancelled),
	TEST_NO_TAG("Transfer message download cancelled", transfer_message_download_cancelled),
	TEST_ONE_TAG("Transfer message using external body url", file_transfer_using_external_body_url, "LeaksMemory"),
	TEST_NO_TAG("Transfer 2 messages simultaneously", file_transfer_2_messages_simultaneously),
	TEST_NO_TAG("Text message denied", text_message_denied),
	TEST_NO_TAG("Info message", info_message),
	TEST_NO_TAG("Info message with body", info_message_with_body),
	TEST_NO_TAG("IsComposing notification", is_composing_notification),
	TEST_ONE_TAG("IsComposing notification lime", is_composing_notification_with_lime, "LIME"),
	TEST_NO_TAG("IMDN notifications", imdn_notifications),
	TEST_NO_TAG("IM notification policy", im_notification_policy),
	TEST_ONE_TAG("IM error delivery notification online", im_error_delivery_notification_online, "LIME"),
	TEST_ONE_TAG("IM error delivery notification offline", im_error_delivery_notification_offline, "LIME"),
	TEST_ONE_TAG("Lime text message", lime_text_message, "LIME"),
	TEST_ONE_TAG("Lime text message to non lime", lime_text_message_to_non_lime_mandatory_policy, "LIME"),
	TEST_ONE_TAG("Lime text message to non lime with preferred policy", lime_text_message_to_non_lime_preferred_policy, "LIME"),
	TEST_ONE_TAG("Lime text message to non lime with preferred policy 2", lime_text_message_to_non_lime_preferred_policy_2, "LIME"),
	TEST_ONE_TAG("Lime transfer message", lime_transfer_message, "LIME"),
	TEST_ONE_TAG("Lime transfer message 2", lime_transfer_message_2, "LIME"),
	TEST_ONE_TAG("Lime transfer message 3", lime_transfer_message_3, "LIME"),
	TEST_ONE_TAG("Lime transfer message 4", lime_transfer_message_4, "LIME"),
	TEST_ONE_TAG("Lime transfer message from history", lime_transfer_message_from_history, "LIME"),
	TEST_ONE_TAG("Lime transfer message without encryption", lime_transfer_message_without_encryption, "LIME"),
	TEST_ONE_TAG("Lime transfer message without encryption 2", lime_transfer_message_without_encryption_2, "LIME"),
	TEST_ONE_TAG("Lime unitary", lime_unit, "LIME"),
#ifdef SQLITE_STORAGE_ENABLED
	TEST_NO_TAG("Database migration", database_migration),
	TEST_NO_TAG("History range", history_range),
	TEST_NO_TAG("History count", history_count),
#endif
	TEST_NO_TAG("Text status after destroying chat room", text_status_after_destroying_chat_room),
	TEST_NO_TAG("Transfer not sent if invalid url", file_transfer_not_sent_if_invalid_url),
	TEST_NO_TAG("Transfer not sent if host not found", file_transfer_not_sent_if_host_not_found),
	TEST_NO_TAG("Transfer not sent if url moved permanently", file_transfer_not_sent_if_url_moved_permanently),
	TEST_ONE_TAG("Transfer io error after destroying chatroom", file_transfer_io_error_after_destroying_chatroom, "LeaksMemory"),
	TEST_ONE_TAG("Real Time Text message", real_time_text_message, "RTT"),
	TEST_ONE_TAG("Real Time Text SQL storage", real_time_text_sql_storage, "RTT"),
	TEST_ONE_TAG("Real Time Text SQL storage with RTT messages not stored", real_time_text_sql_storage_rtt_disabled, "RTT"),
	TEST_ONE_TAG("Real Time Text conversation", real_time_text_conversation, "RTT"),
	TEST_ONE_TAG("Real Time Text without audio", real_time_text_without_audio, "RTT"),
	TEST_ONE_TAG("Real Time Text with srtp", real_time_text_srtp, "RTT"),
	TEST_ONE_TAG("Real Time Text with ice", real_time_text_ice, "RTT"),
	TEST_ONE_TAG("Real Time Text message compatibility crlf", real_time_text_message_compat_crlf, "RTT"),
	TEST_ONE_TAG("Real Time Text message compatibility lf", real_time_text_message_compat_lf, "RTT"),
	TEST_ONE_TAG("Real Time Text message with accented characters", real_time_text_message_accented_chars, "RTT"),
	TEST_ONE_TAG("Real Time Text offer answer with different payload numbers (sender side)", real_time_text_message_different_text_codecs_payload_numbers_sender_side, "RTT"),
	TEST_ONE_TAG("Real Time Text offer answer with different payload numbers (receiver side)", real_time_text_message_different_text_codecs_payload_numbers_receiver_side, "RTT"),
	TEST_ONE_TAG("Real Time Text copy paste", real_time_text_copy_paste, "RTT"),
	TEST_NO_TAG("IM Encryption Engine custom headers", chat_message_custom_headers)
};

test_suite_t message_test_suite = {
	"Message",
	NULL,
	NULL,
	liblinphone_tester_before_each,
	liblinphone_tester_after_each,
	sizeof(message_tests) / sizeof(message_tests[0]), message_tests
};

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif
