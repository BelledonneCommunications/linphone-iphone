
/*
linphone
Copyright (C) 2015  Belledonne Communications SARL

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
 * @defgroup real_time_text_sender Real Time Text Sender
 * @ingroup tutorials
 This program just send chat message in real time to dest uri. Use realtimetext_receiver to receive  message.
 usage: ./realtimetext_sender sip:localhost:5060


 @include realtimetext_sender.c
 */

#include "linphone/core.h"

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}



int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;
	LinphoneCall *call=NULL;
	LinphoneChatRoom *chat_room;
	LinphoneChatMessage *chat_message=NULL;
	const char *dest=NULL;
	LCSipTransports tp;
	tp.udp_port=LC_SIP_TRANSPORT_RANDOM;
	tp.tcp_port=LC_SIP_TRANSPORT_RANDOM;
	tp.tls_port=LC_SIP_TRANSPORT_RANDOM;

	/* take the destination sip uri from the command line arguments */
	if (argc>1){
		dest=argv[1];
	}

	signal(SIGINT,stop);

#ifdef DEBUG
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif

	/*
	 Instanciate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);


	linphone_core_set_sip_transports(lc,&tp); /*to avoid port colliding with receiver*/
	if (dest){
		/*
		 Place an outgoing call with rtt enabled
		*/
		LinphoneCallParams *cp = linphone_core_create_call_params(lc, NULL);
		linphone_call_params_enable_realtime_text(cp,TRUE); /*enable real time text*/
		call=linphone_core_invite_with_params(lc,dest,cp);
		linphone_call_params_unref(cp);
		if (call==NULL){
			printf("Could not place call to %s\n",dest);
			goto end;
		}else printf("Call to %s is in progress...",dest);
		linphone_call_ref(call);

	}
	/*wait for call to be established*/
	while 	(running && (linphone_call_get_state(call) == LinphoneCallOutgoingProgress
						|| linphone_call_get_state(call) == LinphoneCallOutgoingInit)) {
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}
	/*check if call is established*/
	switch (linphone_call_get_state(call)) {
	case LinphoneCallError:
	case LinphoneCallReleased:
	case LinphoneCallEnd:
		printf("Could not place call to %s\n",dest);
		goto end;
		break;
	default:
		break;
		/*continue*/
	}

	chat_room=linphone_call_get_chat_room(call); /*create a chat room associated to this call*/

	/* main loop for sending message and doing background linphonecore work: */
	while(running){
		int character;
		/*to disable terminal buffering*/
		if (system ("/bin/stty raw") == -1){
			ms_error("/bin/stty error");
		}
		character = getchar();
		if (system("/bin/stty cooked") == -1){
			ms_error("/bin/stty error");
		}
		if (character==0x03) {/*CTRL C*/
			running=0;
			break;
		}
		if (character != EOF) {
			/* user has typed something*/
			if (chat_message == NULL) {
				/*create a new message*/
				chat_message = linphone_chat_room_create_message(chat_room,""); /*create an empty message*/
			}
			if (character == '\r') {
				/*new line, committing message*/
				linphone_chat_room_send_chat_message(chat_room,chat_message);
				chat_message = NULL; /*reset message*/
			} else {
				linphone_chat_message_put_char(chat_message,character); /*send char in realtime*/
			}
		}
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}
	if (call && linphone_call_get_state(call)!=LinphoneCallEnd){
		/* terminate the call */
		printf("Terminating the call...\n");
		linphone_core_terminate_call(lc,call);
		/*at this stage we don't need the call object */
		linphone_call_unref(call);
	}

end:
	printf("Shutting down...\n");
	linphone_core_destroy(lc);
	printf("Exited\n");
	return 0;
}

