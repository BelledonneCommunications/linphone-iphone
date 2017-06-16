
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
 * @defgroup real_time_text Real Time Text Receiver
 * @ingroup tutorials
 This program is able to receive chat message in real time on port 5060. Use realtimetext_sender to generate chat message
 usage: ./realtimetext_receiver

 @include realtimetext_sender.c
 */

#include "linphone/core.h"

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}

/*
 * Call state notification callback
 */
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	switch(cstate){
		case LinphoneCallIncomingReceived:
			printf("It is now ringing remotely !\n");
			linphone_core_accept_call(lc,call);
			break;
		case LinphoneCallReleased:
			printf("call terminated, exit...\n");
			running=FALSE;
		break;
		default:
			printf("Unhandled notification %i\n",cstate);
	}
}

/*
 * Completed message received
 */
static void  message_received(LinphoneCore *lc, LinphoneChatRoom *room, LinphoneChatMessage *message) {
	char *from=linphone_address_as_string(linphone_chat_room_get_peer_address(room));
	printf(" Message [%s] received from [%s] \n",linphone_chat_message_get_text(message),from);
	ms_free(from);
}

/*
 *
 * Remote is typing
 */
static void is_composing_received(LinphoneCore *lc, LinphoneChatRoom *room) {
	LinphoneCall *call = linphone_chat_room_get_call(room); /*get corresponding call*/
	if (call && linphone_call_params_realtime_text_enabled(linphone_call_get_current_params(call))) /*check if realtime text enabled for this call*/
		printf("%c",linphone_chat_room_get_char(room));
	/*else ignored*/
}

int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;


	signal(SIGINT,stop);

#ifdef DEBUG
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif
	/* 
	 Fill the LinphoneCoreVTable with application callbacks.
	 All are optional. Here we only use the call_state_changed callbacks
	 in order to get notifications about the progress of the call.
	 */
	vtable.call_state_changed=call_state_changed; /*to receive incoming call*/
	vtable.message_received=message_received; /*to receive committed messages*/
	vtable.is_composing_received=is_composing_received; /*to receive char in real time*/
	/*
	 Instanciate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);

	/* main loop for receiving notifications and doing background linphonecore work: */
	while(running){
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}

	printf("Shutting down...\n");
	linphone_core_destroy(lc);
	printf("Exited\n");
	return 0;
}

