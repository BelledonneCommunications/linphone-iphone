
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
 * @defgroup buddy_tutorials Basic buddy status notification
 * @ingroup tutorials
 *This program is a _very_ simple usage example of liblinphone.
 *Desmonstrating how to initiate a SIP subscription and receive notification from a sip uri identity passed from the command line.
 *<br>Argument must be like sip:jehan@sip.linphone.org .
 *<br>
 *ex registration sip:jehan@sip.linphone.org secret
 *<br>Subscription is cleared on SIGINT
 *<br>
 *@include buddy_status.c

 *
 */

#include "linphonecore.h"

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}

/**
 * presence state change notification callback
 */
static void notify_presence_recv_updated (struct _LinphoneCore *lc,  LinphoneFriend *friend) {
	const LinphoneAddress* friend_address = linphone_friend_get_address(friend);
	printf("New state state [%s] for user id [%s] \n"
				,linphone_online_status_to_string(linphone_friend_get_status(friend))
				,linphone_address_as_string (friend_address));
}

LinphoneCore *lc;
int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};

	char* dest_friend=NULL;


	/* takes   sip uri  identity from the command line arguments */
	if (argc>1){
		dest_friend=argv[1];
	}

	signal(SIGINT,stop);
#define DEBUG
#ifdef DEBUG
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif
	/* 
	 Fill the LinphoneCoreVTable with application callbacks.
	 All are optional. Here we only use the registration_state_changed callbacks
	 in order to get notifications about the progress of the registration.
	 */
	vtable.notify_presence_recv=notify_presence_recv_updated;

	/*
	 Instanciate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);
	LinphoneFriend* my_friend=NULL;

	if (dest_friend) {
		my_friend = linphone_friend_new_with_addr(dest_friend); /*creates friend object from dest*/
		if (my_friend == NULL) {
			printf("bad destination uri for friend [%s]\n",dest_friend);
			goto end;
		}

		linphone_friend_enable_subscribes(my_friend,TRUE); /*configure this friend to emit SUBSCRIBE message after being added to LinphoneCore*/
		linphone_friend_set_name(my_friend,"My best friend"); /* add a nickname to this buddy */
		//linphone_friend_set_inc_subscribe_policy(my_friend,)
		linphone_core_add_friend(lc,my_friend); /* add my friend to the buddy list, initiate SUBSCRIBE message*/

	}

	/* main loop for receiving notifications and doing background linphonecore work: */
	while(running){
		linphone_core_iterate(lc); /* first iterate initiates registration */
		ms_usleep(50000);
	}

	linphone_friend_edit(my_friend); /* start editing friend */
	linphone_friend_enable_subscribes(my_friend,FALSE); /*disable subscription for this friend*/
	linphone_friend_done(my_friend); /*commit changes triggering an UNSUBSCRIBE message*/


end:
	printf("Shutting down...\n");
	linphone_core_destroy(lc);
	printf("Exited\n");
	return 0;
}

