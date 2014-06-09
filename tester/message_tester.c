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
	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk, creating file*/
		file = open("receive_file.dump",O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		linphone_chat_message_set_user_data(message,(void*)(long)(0x00000000FFFFFFFF&file)); /*store fd for next chunks*/
	} else {
		/*next chunk*/
		file = (int)((long)(linphone_chat_message_get_user_data(message))&0x00000000FFFFFFFF);

		if (size==0) { /* tranfer complerte */
			linphone_chat_room_destroy(linphone_chat_message_get_chat_room(message));
			linphone_chat_message_destroy(message);
			stats* counters = get_stats(lc);
			counters->number_of_LinphoneMessageExtBodyReceived++;
			close(file);
		} else { /* store content on a file*/
			write(file,buff,size);
		}
	}
}

static char big_file [128000]; /* a buffer to simulate a big file for the file transfer message test */

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
	char* to = linphone_address_as_string(marie->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(pauline->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	linphone_chat_message_set_external_body_url(message,message_external_body_url="http://www.linphone.org");
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,pauline->lc);
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageReceived,1));
	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneMessageDelivered,1));

	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageInProgress,1);
	CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageExtBodyReceived,1);

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void file_transfer_message(void) {
	int i;
	/* setting dummy file content to something */
	const char* big_file_content="big file";
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new( "pauline_rc");

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
	char* to = linphone_address_as_string(pauline->identity);
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(marie->lc,to);
	LinphoneChatMessage* message = linphone_chat_room_create_message(chat_room,"Bli bli bli \n blu");
	/*simultate a network error*/
	sal_set_send_error(marie->lc->sal, -1);
	linphone_chat_room_send_message2(chat_room,message,liblinphone_tester_chat_message_state_change,marie->lc);

	CU_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneMessageNotDelivered,1));
	/*CU_ASSERT_EQUAL(marie->stat.number_of_LinphoneMessageInProgress,1);*/
	CU_ASSERT_EQUAL(pauline->stat.number_of_LinphoneMessageReceived,0);

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
	{ "Text message denied", text_message_denied },
	{ "Info message", info_message },
	{ "Info message with body", info_message_with_body },
	{ "IsComposing notification", is_composing_notification }
};

test_suite_t message_test_suite = {
	"Message",
	NULL,
	NULL,
	sizeof(message_tests) / sizeof(message_tests[0]),
	message_tests
};

