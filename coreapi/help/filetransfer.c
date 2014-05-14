
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

#ifdef IN_LINPHONE
#include "linphonecore.h"
#else
#include "linphone/linphonecore.h"
#endif

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}
/**
 * function invoked to report file transfer progress.
 * */
static void file_transfer_progress_indication(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, size_t progress) {
	const LinphoneAddress* from_address = linphone_chat_message_get_from(message);
	const LinphoneAddress* to_address = linphone_chat_message_get_to(message);
	char *address = linphone_chat_message_is_outgoing(message)?linphone_address_as_string(to_address):linphone_address_as_string(from_address);
	printf(" File transfer  [%i&] %s of type [%s/%s] %s [%s] \n", (int)(content->size/progress)*100
																	,(linphone_chat_message_is_outgoing(message)?"sent":"received")
																	, content->type
																	, content->subtype
																	,(linphone_chat_message_is_outgoing(message)?"to":"from")
																	, address);
	free(address);
}
/**
 * function invoked when a file transfer is received.
 * */
static void file_transfer_received(LinphoneCore *lc, LinphoneChatMessage *message, const LinphoneContent* content, const char* buff, size_t size){
	const LinphoneAddress* from_address = linphone_chat_message_get_from(message);
	char *from = linphone_address_as_string(from_address);
	int file=-1;
	printf(" File transfer receive [%i] bytes of type [%s/%s] from [%s] \n"	, (int)size
																			, content->type
																			, content->subtype
																			, from);
	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk, creating file*/
		file = open("receive_file.dump",O_WRONLY);
		linphone_chat_message_set_user_data(message,(void*)(long)(0x00000000FFFFFFFF&file)); /*store fd for next chunks*/
	} else {
		/*next chunk*/
		file = (int)linphone_chat_message_get_user_data(message);
	}

	/*store content on a file*/
	write(file,buff,size);

	if (size==0) {
		printf("File transfert completed");
		close(file);
	} /*else wait for next chunk*/

	free(from);
}

char big_file [128000];
/*
 * function call when is file transfer is initiated. file content should be feed into object LinphoneContent
 * */
static void file_transfer_send(LinphoneCore *lc, LinphoneChatMessage *message,  const LinphoneContent* content, char* buff, size_t* size){
	const LinphoneAddress* to_address = linphone_chat_message_get_to(message);
	char *to = linphone_address_as_string(to_address);
	int offset=-1;
	/*content->size can be feed*/

	if (!linphone_chat_message_get_user_data(message)) {
		/*first chunk*/
		offset=0;
	} else {
		/*subsequent chunk*/
		offset = (int)linphone_chat_message_get_user_data(message);
	}
	*size = MIN(*size,sizeof(big_file)-offset); /*updating content->size with minimun between remaining data and requested size*/

	if (*size==0) {
		/*end of file*/
		return;
	}
	memcpy(buff,big_file+offset,*size);

	printf(" File transfer sending [%i] bytes of type [%s/%s] from [%s] \n"	, (int)*size
																			, content->type
																			, content->subtype
																			, to);
	/*store offset for next chunk*/
	linphone_chat_message_set_user_data(message,(void*)(offset+*size));

	free(to);

}
/*
 * Call back to get delivery status of a message
 * */
static void linphone_file_transfer_state_changed(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud) {
	const LinphoneAddress* to_address = linphone_chat_message_get_to(msg);
	char *to = linphone_address_as_string(to_address);
	printf("File transfer sent to [%s] delivery status is [%s] \n"	, to
																	, linphone_chat_message_state_to_string(state));
	free(to);
}

LinphoneCore *lc;
int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};

	char* dest_friend=NULL;
	int i;
	const char* big_file_content="big file";
	/*seting dummy file content to something*/
	for (i=0;i<sizeof(big_file)/strlen(big_file_content);i++)
		sprintf(big_file+i,"%s",big_file_content);

	/* takes   sip uri  identity from the command line arguments */
	if (argc>1){
		dest_friend=argv[1];
	}
	signal(SIGINT,stop);
//#define DEBUG
#ifdef DEBUG
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif
	/* 
	 Fill the LinphoneCoreVTable with application callbacks.
	 All are optional. Here we only use the file_transfer_received callback
	 in order to get notifications about incoming file receive, file_transfer_send to feed file to be transfered
	 and file_transfer_progress_indication to print progress.
	 */
	vtable.file_transfer_received=file_transfer_received;
	vtable.file_transfer_send=file_transfer_send;
	vtable.file_transfer_progress_indication=file_transfer_progress_indication;


	/*
	 Instantiate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);

	/**
	 * Globally configure an http file transfer server.
	 */
	linphone_core_set_file_transfer_server(lc,"http://sharing.linphone.org/upload.php");


	/*Next step is to create a chat root*/
	LinphoneChatRoom* chat_room = linphone_core_create_chat_room(lc,dest_friend);

	LinphoneContent content;
	memset(&content,0,sizeof(content));
	content.type="text";
	content.subtype="plain";
	content.size=sizeof(big_file); /*total size to be transfered*/

	/*now create a chat message with custom content*/
	LinphoneChatMessage* chat_message = linphone_chat_room_create_file_transfer_message(chat_room,&content);

	/*initiating file transfer*/
	/**/
	linphone_chat_room_send_message2(chat_room, chat_message,linphone_file_transfer_state_changed,NULL);

	/* main loop for receiving incoming messages and doing background linphone core work: */
	while(running){
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}

	printf("Shutting down...\n");
	linphone_chat_room_destroy(chat_room);
	linphone_core_destroy(lc);
	printf("Exited\n");
	return 0;
}

