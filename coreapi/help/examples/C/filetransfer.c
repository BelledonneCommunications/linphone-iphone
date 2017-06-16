
/*
linphone
Copyright (C) 2010  Belledonne Communications SARL

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/**
 * @defgroup chatroom_tuto Chat room and messaging
 * @ingroup tutorials
 *This program is a _very_ simple usage example of liblinphone,
 *desmonstrating how to send/receive  SIP MESSAGE from a sip uri identity passed from the command line.
 *<br>Argument must be like sip:jehan@sip.linphone.org .
 *<br>
 *ex chatroom sip:jehan@sip.linphone.org
 *<br>
 *@include chatroom.c

 *
 */

#include "linphone/core.h"

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}
/**
 * function invoked to report file transfer progress.
 * */
static void file_transfer_progress_indication(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t total) {
	const LinphoneAddress* from_address = linphone_chat_message_get_from(message);
	const LinphoneAddress* to_address = linphone_chat_message_get_to(message);
	char *address = linphone_chat_message_is_outgoing(message)?linphone_address_as_string(to_address):linphone_address_as_string(from_address);
	printf(" File transfer  [%d%%] %s of type [%s/%s] %s [%s] \n", (int)((offset *100)/total)
																	,(linphone_chat_message_is_outgoing(message)?"sent":"received")
																	, linphone_content_get_type(content)
																	, linphone_content_get_subtype(content)
																	,(linphone_chat_message_is_outgoing(message)?"to":"from")
																	, address);
	free(address);
}
/**
 * function invoked when a file transfer is received.
 **/
static void file_transfer_received(LinphoneChatMessage *message, const LinphoneContent* content, const LinphoneBuffer *buffer){
	FILE* file=NULL;
	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk, creating file*/
		file = fopen("receive_file.dump","wb");
		linphone_chat_message_set_user_data(message,(void*)file); /*store fd for next chunks*/
	}

	file = (FILE*)linphone_chat_message_get_user_data(message);
	if (linphone_buffer_is_empty(buffer)) {
		printf("File transfert completed\n");
		linphone_chat_message_destroy(message);
		fclose(file);
		running=FALSE;
	} else { /* store content on a file*/
		if (fwrite(linphone_buffer_get_content(buffer),linphone_buffer_get_size(buffer),1,file)==0){
			ms_warning("file_transfer_received() write failed: %s",strerror(errno));
		}
	}
}

char big_file [128000];

/*
 * function called when the file transfer is initiated. file content should be feed into object LinphoneContent
 * */
static LinphoneBuffer * file_transfer_send(LinphoneChatMessage *message, const LinphoneContent* content, size_t offset, size_t size){
	size_t size_to_send = MIN(size, sizeof(big_file) - offset);
	if (size == 0) return linphone_buffer_new(); /*end of file*/
	return linphone_buffer_new_from_data((uint8_t *)big_file + offset, size_to_send);
}

/*
 * Call back to get delivery status of a message
 * */
static void linphone_file_transfer_state_changed(LinphoneChatMessage* msg,LinphoneChatMessageState state) {
	const LinphoneAddress* to_address = linphone_chat_message_get_to(msg);
	char *to = linphone_address_as_string(to_address);
	printf("File transfer sent to [%s] delivery status is [%s] \n"	, to
																	, linphone_chat_message_state_to_string(state));
	free(to);
}

/*
 * Call back called when a message is received
 */
static void message_received(LinphoneCore *lc, LinphoneChatRoom *cr, LinphoneChatMessage *msg) {
	const LinphoneContent *file_transfer_info = linphone_chat_message_get_file_transfer_information(msg);
	printf ("Do you really want to download %s (size %ld)?[Y/n]\nOk, let's go\n", linphone_content_get_name(file_transfer_info), (long int)linphone_content_get_size(file_transfer_info));

	linphone_chat_message_download_file(msg);

}

LinphoneCore *lc;
int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};

	const char* dest_friend=NULL;
	size_t i;
	const char* big_file_content="big file";
	LinphoneChatRoom* chat_room;
	LinphoneContent* content;
	LinphoneChatMessage* chat_message;
	LinphoneChatMessageCbs *cbs;

	/*seting dummy file content to something*/
	for (i=0;i<sizeof(big_file);i+=strlen(big_file_content))
		memcpy(big_file+i, big_file_content, strlen(big_file_content));

	big_file[0]=*"S";
	big_file[sizeof(big_file)-1]=*"E";

	signal(SIGINT,stop);
//#define DEBUG
#ifdef DEBUG
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif
	vtable.message_received=message_received;


	/*
	 Instantiate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);
	dest_friend = linphone_core_get_primary_contact(lc);
	printf("Send message to me : %s\n", dest_friend);

	/**
	 * Globally configure an http file transfer server.
	 */
	//linphone_core_set_file_transfer_server(lc,"http://npasc.al/lft.php");
	linphone_core_set_file_transfer_server(lc,"https://www.linphone.org:444/lft.php");


	/*Next step is to create a chat room*/
	chat_room = linphone_core_get_chat_room_from_uri(lc,dest_friend);

	content = linphone_core_create_content(lc);
	linphone_content_set_type(content,"text");
	linphone_content_set_subtype(content,"plain");
	linphone_content_set_size(content,sizeof(big_file)); /*total size to be transfered*/
	linphone_content_set_name(content,"bigfile.txt");

	/*now create a chat message with custom content*/
	chat_message = linphone_chat_room_create_file_transfer_message(chat_room,content);
	if (chat_message == NULL) {
		printf("returned message is null\n");
	}

	/**
	 * Fill the application callbacks. The file_transfer_received callback is used in order to get notifications
	 * about incoming file reception, file_transfer_send to feed file to be transfered and
	 * file_transfer_progress_indication to print progress.
	 */
	cbs = linphone_chat_message_get_callbacks(chat_message);
	linphone_chat_message_cbs_set_file_transfer_recv(cbs, file_transfer_received);
	linphone_chat_message_cbs_set_file_transfer_send(cbs, file_transfer_send);
	linphone_chat_message_cbs_set_file_transfer_progress_indication(cbs, file_transfer_progress_indication);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, linphone_file_transfer_state_changed);

	/*initiating file transfer*/
	linphone_chat_room_send_chat_message(chat_room, chat_message);

	/* main loop for receiving incoming messages and doing background linphone core work: */
	while(running){
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}


	printf("Shutting down...\n");
	linphone_content_unref(content);
	linphone_core_destroy(lc);
	printf("Exited\n");
	return 0;
}

