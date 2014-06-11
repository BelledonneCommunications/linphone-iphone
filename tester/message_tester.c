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
#include "lime.h"

#ifdef MSG_STORAGE_ENABLED
#include <sqlite3.h>
#endif


static char* message_external_body_url;

void text_message_received(LinphoneCore *lc, LinphoneChatRoom *room, const LinphoneAddress *from_address, const char *message) {
	stats* counters = get_stats(lc);
	counters->number_of_LinphoneMessageReceivedLegacy++;
}

void message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage* message) {
	char* from=linphone_address_as_string(linphone_chat_message_get_from(message));
	stats* counters;
	const char *text=linphone_chat_message_get_text(message);
	const char *external_body_url=linphone_chat_message_get_external_body_url(message);
	const LinphoneContent *file_transfer_info=linphone_chat_message_get_file_transfer_information(message);

	ms_message("Message from [%s]  is [%s] , external URL [%s]",from?from:""
																,text?text:""
																,external_body_url?external_body_url:"");
	ms_free(from);
	counters = get_stats(lc);
	counters->number_of_LinphoneMessageReceived++;
	if (file_transfer_info) { /* if we have a file transfer in RCS mode, start the download */
		linphone_chat_message_start_file_download(message);
	} else if (linphone_chat_message_get_external_body_url(message)) {
		counters->number_of_LinphoneMessageExtBodyReceived++;
		CU_ASSERT_STRING_EQUAL(linphone_chat_message_get_external_body_url(message),message_external_body_url);
	}
}

/**
 * function invoked when a file transfer is received.
 * */
void file_transfer_received(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, const char* buff, size_t size){
	int file=-1;
	/* first chunk, creating file */
	if (!linphone_chat_message_get_user_data(message)) {
		file = open("receive_file.dump",O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		linphone_chat_message_set_user_data(message,(void*)(long)(0x00000000FFFFFFFF&file)); /*store fd for next chunks*/
	}

	file = (int)((long)(linphone_chat_message_get_user_data(message))&0x00000000FFFFFFFF);

	if (size==0) { /* tranfer complete */
		linphone_chat_room_destroy(linphone_chat_message_get_chat_room(message));
		linphone_chat_message_destroy(message);
		stats* counters = get_stats(lc);
		counters->number_of_LinphoneMessageExtBodyReceived++;
		close(file);
	} else { /* store content on a file*/
		write(file,buff,size);
	}
}

char big_file[128000]; /* a buffer to simulate a big file for the file transfer message test */

/*
 * function called when the file transfer is initiated. file content should be feed into object LinphoneContent
 * */
void file_transfer_send(LinphoneCore *lc, LinphoneChatMessage *message,  const LinphoneContent* content, char* buff, size_t* size){
	int offset=-1;

	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk*/
		offset=0;
	} else {
		/*subsequent chunk*/
		offset = (int)((long)(linphone_chat_message_get_user_data(message))&0x00000000FFFFFFFF);
	}
	*size = MIN(*size,sizeof(big_file)-offset); /*updating content->size with minimun between remaining data and requested size*/

	if (*size==0) {
		/*end of file*/
		return;
	}
	memcpy(buff,big_file+offset,*size);

	/*store offset for next chunk*/
	linphone_chat_message_set_user_data(message,(void*)(offset+*size));
}

/**
 * function invoked to report file transfer progress.
 * */
void file_transfer_progress_indication(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, size_t progress) {
	const LinphoneAddress* from_address = linphone_chat_message_get_from(message);
	const LinphoneAddress* to_address = linphone_chat_message_get_to(message);
	char *address = linphone_chat_message_is_outgoing(message)?linphone_address_as_string(to_address):linphone_address_as_string(from_address);
	printf(" File transfer  [%d%%] %s of type [%s/%s] %s [%s] \n", (int)progress
																	,(linphone_chat_message_is_outgoing(message)?"sent":"received")
																	, content->type
																	, content->subtype
																	,(linphone_chat_message_is_outgoing(message)?"to":"from")
																	, address);
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
	LinphoneCore* lc=(LinphoneCore*)ud;
	stats* counters = get_stats(lc);
	ms_message("Message [%s] [%s]",linphone_chat_message_get_text(msg),linphone_chat_message_state_to_string(state));
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
	default:
		ms_error("Unexpected state [%s] for message [%p]",linphone_chat_message_state_to_string(state),msg);
	}

}

static void text_message(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_within_dialog(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	lp_config_set_int(pauline->lc->config,"sip","chat_use_call_dialogs",1);

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	CU_ASSERT_TRUE(call(marie,pauline));

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static LinphoneAuthInfo* text_message_with_credential_from_auth_cb_auth_info;
static void text_message_with_credential_from_auth_cb_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
	stats* counters;
	ms_message("text_message_with_credential_from_auth_cb:Auth info requested  for user id [%s] at realm [%s]\n"
						,username
						,realm);
	counters = get_stats(lc);
	counters->number_of_auth_info_requested++;
	linphone_core_add_auth_info(lc,text_message_with_credential_from_auth_cb_auth_info); /*add stored authentication info to LinphoneCore*/
}


static void text_message_with_credential_from_auth_cb(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");
	text_message_with_credential_from_auth_cb_auth_info=linphone_auth_info_clone((LinphoneAuthInfo*)(linphone_core_get_auth_info_list(marie->lc)->data));

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	/*to force cb to be called*/
	linphone_core_clear_all_auth_info(marie->lc);
	marie->lc->vtable.auth_info_requested=text_message_with_credential_from_auth_cb_auth_info_requested;

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);


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

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	LinphoneProxyConfig* pauline_proxy;
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	/*test proxy config privacy*/
	linphone_core_get_default_proxy(pauline->lc,&pauline_proxy);
	linphone_proxy_config_set_privacy(pauline_proxy,LinphonePrivacyId);

	CU_ASSERT_PTR_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));

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

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

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
	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceivedLegacy,1);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_ack(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void text_message_with_external_body(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	linphone_chat_message_set_external_body_url(message,message_external_body_url="http://www.linphone.org");
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);

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

static void file_transfer_message(void) {
	int i;
	/* setting dummy file content to something */
	const char* big_file_content="big file";
	for (i=0;i<=sizeof(big_file)-strlen(big_file_content);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	if (i<sizeof(big_file)) {
		memcpy(big_file+i, big_file_content, sizeof(big_file)-i);
	}

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);

	/* create a file transfer message */
	LinphoneContent content;
	memset(&content,0,sizeof(content));
	content.type="text";
	content.subtype="plain";
	content.size=sizeof(big_file); /*total size to be transfered*/
	content.name = "bigfile.txt";
	LinphoneChatMessage* message = linphone_chat_room_create_file_transfer_message(chat_room, &content);

	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageExtBodyReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void lime_file_transfer_message(void) {
	int i;
	/* setting dummy file content to something */
	const char* big_file_content="big file";
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is enabled */
	linphone_core_set_lime(marie->lc, 1);
	linphone_core_set_lime(pauline->lc, 1);

	/* set the zid caches files */
	linphone_core_set_zrtp_secrets_file(marie->lc, "ZIDCacheAlice.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, "ZIDCacheBob.xml");

	/* Globally configure an http file transfer server. */
	linphone_core_set_file_transfer_server(pauline->lc,"https://www.linphone.org:444/lft.php");

	/* create a chatroom on pauline's side */
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);

	/* create a file transfer message */
	LinphoneContent content;
	memset(&content,0,sizeof(content));
	content.type="text";
	content.subtype="plain";
	content.size=sizeof(big_file); /*total size to be transfered*/
	content.name = "bigfile.txt";
	LinphoneChatMessage* message = linphone_chat_room_create_file_transfer_message(chat_room, &content);

	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageExtBodyReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


static void text_message_with_send_error(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(marie->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	/*simultate a network error*/
	sal_set_send_error(marie->lc->sal, -1);
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,marie->lc);

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

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(marie->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");

	/*pauline doesn't want to be disturbed*/
	linphone_core_disable_chat(pauline->lc,LinphoneReasonDoNotDisturb);

	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,marie->lc);

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

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	LinphoneInfoMessage *info;
	const LinphoneContent *content;
	const char *hvalue;

	CU_ASSERT_TRUE(call(pauline,marie));

	info=linphone_core_create_info_message(marie->lc);
	linphone_info_message_add_header(info,"Weather","still bad");
	if (with_content) {
		LinphoneContent ct={0};
		ct.type="application";
		ct.subtype="somexml";
		ct.data=(void*)info_content;
		ct.size=strlen(info_content);
		linphone_info_message_set_content(info,&ct);
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
			CU_ASSERT_PTR_NOT_NULL(content->data);
			CU_ASSERT_PTR_NOT_NULL(content->type);
			CU_ASSERT_PTR_NOT_NULL(content->subtype);
			if (content->type) CU_ASSERT_TRUE(strcmp(content->type,"application")==0);
			if (content->subtype) CU_ASSERT_TRUE(strcmp(content->subtype,"somexml")==0);
			if (content->data)CU_ASSERT_TRUE(strcmp((const char*)content->data,info_content)==0);
			CU_ASSERT_EQUAL(content->size,strlen(info_content));
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

	/* make sure lime is not enabled */
	linphone_core_set_lime(marie->lc, 0);
	linphone_core_set_lime(pauline->lc, 0);

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc, to);
	int dummy = 0;

	ms_free(to);
	linphone_chat_room_compose(chat_room);
	wait_for_until(pauline->lc, marie->lc, &dummy, 1, 1500); /*just to sleep while iterating*/
	linphone_chat_room_send_message(chat_room, "Composing a message");
	CU_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingActiveReceived, 1));
	CU_ASSERT_TRUE(wait_for(pauline->lc, marie->lc, &marie->stat.number_of_LinphoneIsComposingIdleReceived, 2));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

void printHex(char *title, uint8_t *data, uint32_t length) {
	printf ("%s : ", title);
	int i;
	for (i=0; i<length; i++) {
		printf ("0x%02x, ", data[i]);
	}
	printf ("\n");
}

static void lime_unit(void) {
	int retval;
	/* Load Alice cache file */
	FILE *CACHE = fopen("ZIDCacheAlice.xml", "r+");
	fseek(CACHE, 0L, SEEK_END);  /* Position to end of file */
  	int size = ftell(CACHE);     /* Get file length */
  	rewind(CACHE);               /* Back to start of file */
	uint8_t *cacheBufferString = (uint8_t *)malloc(size*sizeof(uint8_t)+1);
	fread(cacheBufferString, 1, size, CACHE);
	*(cacheBufferString+size) = '\0';
	fclose(CACHE);
	/* parse it to an xmlDoc */
	xmlDocPtr cacheBufferAlice = xmlParseDoc(cacheBufferString);
	free(cacheBufferString);
	
	/* Load Bob cache file */
	CACHE = fopen("ZIDCacheBob.xml", "r+");
	fseek(CACHE, 0L, SEEK_END);  /* Position to end of file */
  	size = ftell(CACHE);     /* Get file length */
  	rewind(CACHE);               /* Back to start of file */
	cacheBufferString = (uint8_t *)malloc(size*sizeof(uint8_t)+1);
	fread(cacheBufferString, 1, size, CACHE);
	*(cacheBufferString+size) = '\0';
	fclose(CACHE);
	/* parse it to an xmlDoc */
	xmlDocPtr cacheBufferBob = xmlParseDoc(cacheBufferString);
	free(cacheBufferString);



	/* encrypt a message */
	uint8_t *multipartMessage = NULL;
	retval = lime_createMultipartMessage(cacheBufferAlice, (uint8_t *)"Bonjour les petits lapins,ca va? éh oui oui", (uint8_t *)"sip:pauline@sip.example.org", &multipartMessage);

	printf("create message return %d\n", retval);
	if (retval == 0) {
		printf("message is %s\n", multipartMessage);
	}

	/* decrypt the multipart message */
	uint8_t *decryptedMessage = NULL;
	retval = lime_decryptMultipartMessage(cacheBufferBob, multipartMessage, &decryptedMessage);

	printf("decrypt message return %d\n", retval);
	if (retval == 0) {
		printf("message is %s##END\n", decryptedMessage);
	}
	free(multipartMessage);
	free(decryptedMessage);

	/* update ZID files */
	/* dump the xml document into a string */
	xmlChar *xmlStringOutput;
	int xmlStringLength;
	xmlDocDumpFormatMemoryEnc(cacheBufferAlice, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
	/* write it to the file */
	CACHE = fopen("ZIDCacheAlice.xml", "w+");
	fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
	xmlFree(xmlStringOutput);
	fclose(CACHE);

	xmlDocDumpFormatMemoryEnc(cacheBufferBob, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
	/* write it to the file */
	CACHE = fopen("ZIDCacheBob.xml", "w+");
	fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
	xmlFree(xmlStringOutput);
	fclose(CACHE);


	xmlFreeDoc(cacheBufferAlice);
	xmlFreeDoc(cacheBufferBob);

	/* Load cache file */
	CACHE = fopen("ZIDCache.xml", "r+");
	fseek(CACHE, 0L, SEEK_END);  /* Position to end of file */
  	size = ftell(CACHE);     /* Get file length */
  	rewind(CACHE);               /* Back to start of file */
	cacheBufferString = (uint8_t *)malloc(size*sizeof(uint8_t)+1);
	fread(cacheBufferString, 1, size, CACHE);
	*(cacheBufferString+size) = '\0';
	fclose(CACHE);
	/* parse it to an xmlDoc */
	xmlDocPtr cacheBuffer = xmlParseDoc(cacheBufferString);
	free(cacheBufferString);

	/* get data from cache : sender */
	limeURIKeys_t associatedKeys;
	associatedKeys.peerURI = (uint8_t *)malloc(15);
	memcpy(associatedKeys.peerURI, "pipo1@pipo.com", 15);
	associatedKeys.associatedZIDNumber  = 0;
	retval = lime_getCachedSndKeysByURI(cacheBuffer, &associatedKeys);
	printf("getCachedKeys returns %d, number of key found %d\n", retval, associatedKeys.associatedZIDNumber);

	int i;
	for (i=0; i<associatedKeys.associatedZIDNumber; i++) {
		printHex("ZID", associatedKeys.peerKeys[i]->peerZID, 12);
		printHex("key", associatedKeys.peerKeys[i]->key, 32);
		printHex("sessionID", associatedKeys.peerKeys[i]->sessionId, 32);
		printf("session index %d\n", associatedKeys.peerKeys[i]->sessionIndex);
	}

	/* get data from cache : receiver */
	limeKey_t associatedKey;
	uint8_t targetZID[12] = {0x00, 0x5d, 0xbe, 0x03, 0x99, 0x64, 0x3d, 0x95, 0x3a, 0x22, 0x02, 0xdd};
	memcpy(associatedKey.peerZID, targetZID, 12);
	retval = lime_getCachedRcvKeyByZid(cacheBuffer, &associatedKey);
	printf("getCachedKey by ZID return %d\n", retval);

	printHex("Key", associatedKey.key, 32);
	printHex("sessionID", associatedKey.sessionId, 32);
	printf("session index %d\n", associatedKey.sessionIndex);

	/* encrypt/decrypt a message */
	uint8_t senderZID[12] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x70, 0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0};
	uint8_t encryptedMessage[48];
	uint8_t plainMessage[48];
	lime_encryptMessage(associatedKeys.peerKeys[0], (uint8_t *)"bla Bla bla b! Pipo", 20, senderZID, encryptedMessage);
	printHex("Ciphered", encryptedMessage, 32);
	/* invert sender and receiverZID to decrypt/authenticate */
	uint8_t receiverZID[12];
	memcpy(receiverZID, associatedKeys.peerKeys[0]->peerZID, 12);
	memcpy(associatedKeys.peerKeys[0]->peerZID, senderZID, 12);
	retval = lime_decryptMessage(associatedKeys.peerKeys[0], encryptedMessage, 36, receiverZID, plainMessage);
	printf("Decrypt and auth returned %d\nPlain: %s\n", retval, plainMessage);

	/* update receiver data */
	associatedKey.sessionIndex++;
	associatedKey.key[0]++;
	associatedKey.sessionId[0]++;
	retval = lime_setCachedKey(cacheBuffer, &associatedKey, LIME_RECEIVER);
	printf("setCachedKey return %d\n", retval);

	/* update sender data */
	associatedKeys.peerKeys[0]->sessionIndex++;
	associatedKeys.peerKeys[0]->key[0]++;
	associatedKeys.peerKeys[0]->sessionId[0]++;
	retval = lime_setCachedKey(cacheBuffer, associatedKeys.peerKeys[0], LIME_SENDER);
	printf("setCachedKey return %d\n", retval);

	/* free memory */
	lime_freeKeys(associatedKeys);

	/* write the file */
	/* dump the xml document into a string */
	xmlDocDumpFormatMemoryEnc(cacheBuffer, &xmlStringOutput, &xmlStringLength, "UTF-8", 0);
	/* write it to the file */
	CACHE = fopen("ZIDCache.xml", "w+");
	fwrite(xmlStringOutput, 1, xmlStringLength, CACHE);
	xmlFree(xmlStringOutput);
	fclose(CACHE);
	xmlFreeDoc(cacheBuffer);
}

static void lime_text_message(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

	/* make sure lime is enabled */
	linphone_core_set_lime(marie->lc, 1);
	linphone_core_set_lime(pauline->lc, 1);

	/* set the zid caches files */
	linphone_core_set_zrtp_secrets_file(marie->lc, "ZIDCacheAlice.xml");
	linphone_core_set_zrtp_secrets_file(pauline->lc, "ZIDCacheBob.xml");

	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	ms_free(to);

	linphone_chat_room_send_message(chat_room,"Bla bla bla bla");
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageReceivedLegacy,1);

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_chat_room(marie->lc,pauline->identity));
	/* TODO : check the message arrived correctly deciphered */

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}


#ifdef MSG_STORAGE_ENABLED

/*
 * Copy file "from" to file "to".
 * Destination file is truncated if existing.
 * Return 1 on success, 0 on error (printing an error).
 */
static int
message_tester_copy_file(const char *from, const char *to)
{
	char message[256];
	FILE *in, *out;
	char buf[256];
	size_t n;

	/* Open "from" file for reading */
	in=fopen(from, "r");
	if ( in == NULL )
	{
		snprintf(message, 255, "Can't open %s for reading: %s\n",
			from, strerror(errno));
		fprintf(stderr, "%s", message);
		return 0;
	}

	/* Open "to" file for writing (will truncate existing files) */
	out=fopen(to, "w");
	if ( out == NULL )
	{
		snprintf(message, 255, "Can't open %s for writing: %s\n",
			to, strerror(errno));
		fprintf(stderr, "%s", message);
		fclose(in);
		return 0;
	}

	/* Copy data from "in" to "out" */
	while ( (n=fread(buf, 1, sizeof buf, in)) > 0 )
	{
		if ( ! fwrite(buf, 1, n, out) )
		{
			fclose(in);
			fclose(out);
			return 0;
		}
	}

	fclose(in);
	fclose(out);

	return 1;
}

static int check_no_strange_time(void* data,int argc, char** argv,char** cNames) {
	CU_ASSERT_EQUAL(argc, 0);
	return 0;
}

static void message_storage_migration() {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	char src_db[256];
	char tmp_db[256];
	snprintf(src_db,sizeof(src_db), "%s/messages.db", liblinphone_tester_file_prefix);
	snprintf(tmp_db,sizeof(tmp_db), "%s/tmp.db", liblinphone_tester_writable_dir_prefix);

	CU_ASSERT_EQUAL_FATAL(message_tester_copy_file(src_db, tmp_db), 1);

	// enable to test the performances of the migration step
	//linphone_core_message_storage_set_debug(marie->lc, TRUE);

	// the messages.db has 10000 dummy messages with the very first DB scheme.
	// This will test the migration procedure
	linphone_core_set_chat_database_path(marie->lc, tmp_db);

	MSList* chatrooms = linphone_core_get_chat_rooms(marie->lc);
	CU_ASSERT(ms_list_size(chatrooms) > 0);

	// check that all messages have been migrated to the UTC time storage
	CU_ASSERT(sqlite3_exec(marie->lc->db, "SELECT * FROM history WHERE time != '-1';", check_no_strange_time, NULL, NULL) == SQLITE_OK );
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
	{ "Lime File transfer message", lime_file_transfer_message },
	{ "Text message denied", text_message_denied },
	{ "Info message", info_message },
	{ "Info message with body", info_message_with_body },
	{ "IsComposing notification", is_composing_notification },
	{ "Lime Unitary", lime_unit },
	{ "Lime Text Message", lime_text_message }
#ifdef MSG_STORAGE_ENABLED
	,{ "Database migration", message_storage_migration }
#endif
};

test_suite_t message_test_suite = {
	"Message",
	NULL,
	NULL,
	sizeof(message_tests) / sizeof(message_tests[0]),
	message_tests
};

