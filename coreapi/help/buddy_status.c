
/*
buddy_status
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
 * @defgroup buddy_tutorials Basic buddy status notification
 * @ingroup tutorials
 *This program is a _very_ simple usage example of liblinphone,
 *demonstrating how to initiate  SIP subscriptions and receive notifications from a sip uri identity passed from the command line.
 *<br>Argument must be like sip:jehan@sip.linphone.org .
 *<br>
 *ex budy_list sip:jehan@sip.linphone.org
 *<br>Subscription is cleared on SIGINT
 *<br>
 *@include buddy_status.c

 *
 */

#include "linphone/core.h"

#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}

/**
 * presence state change notification callback
 */
static void notify_presence_recv_updated (LinphoneCore *lc,  LinphoneFriend *friend) {
	const LinphoneAddress* friend_address = linphone_friend_get_address(friend);
	if (friend_address != NULL) {
		const LinphonePresenceModel* model = linphone_friend_get_presence_model(friend);
		LinphonePresenceActivity *activity = linphone_presence_model_get_activity(model);
		char *activity_str = linphone_presence_activity_to_string(activity);
		char *str = linphone_address_as_string (friend_address);
		printf("New state state [%s] for user id [%s] \n"
					,activity_str
					,str);
		ms_free(str);
	}
}
static void new_subscription_requested (LinphoneCore *lc,  LinphoneFriend *friend, const char* url) {
	const LinphoneAddress* friend_address = linphone_friend_get_address(friend);

	if (friend_address != NULL) {
		char *str = linphone_address_as_string (friend_address);
		printf(" [%s] wants to see your status, accepting\n", str);
		ms_free(str);
	}
	linphone_friend_edit(friend); /* start editing friend */
	linphone_friend_set_inc_subscribe_policy(friend,LinphoneSPAccept); /* Accept incoming subscription request for this friend*/
	linphone_friend_done(friend); /*commit change*/
	linphone_core_add_friend(lc,friend); /* add this new friend to the buddy list*/

}
/**
 * Registration state notification callback
 */
static void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message){
		printf("New registration state %s for user id [%s] at proxy [%s]"
				,linphone_registration_state_to_string(cstate)
				,linphone_proxy_config_get_identity(cfg)
				,linphone_proxy_config_get_addr(cfg));
}

LinphoneCore *lc;
int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};

	char* dest_friend=NULL;
	char* identity=NULL;
	char* password=NULL;

	LinphoneFriend* my_friend=NULL;

	/* takes   sip uri  identity from the command line arguments */
	if (argc>1){
		dest_friend=argv[1];
	}
	/* takes   sip uri  identity from the command line arguments */
	if (argc>2){
		identity=argv[2];
	}
	/* takes   password from the command line arguments */
	if (argc>3){
		password=argv[3];
	}
	signal(SIGINT,stop);
//#define DEBUG
#ifdef DEBUG
	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
#endif
	/*
	 Fill the LinphoneCoreVTable with application callbacks.
	 All are optional. Here we only use the both notify_presence_received and new_subscription_requested callbacks
	 in order to get notifications about friend status.
	 */
	vtable.notify_presence_received=notify_presence_recv_updated;
	vtable.new_subscription_requested=new_subscription_requested;
	vtable.registration_state_changed=registration_state_changed; /*just in case sip proxy is used*/

	/*
	 Instantiate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);
	/*sip proxy might be requested*/
	if (identity != NULL)  {
		/*create proxy config*/
		LinphoneProxyConfig* proxy_cfg = linphone_proxy_config_new();
		/*parse identity*/
		LinphoneAddress *from = linphone_address_new(identity);
		LinphoneAuthInfo *info;
		if (from==NULL){
			printf("%s not a valid sip uri, must be like sip:toto@sip.linphone.org \n",identity);
			goto end;
		}
		if (password!=NULL){
			info=linphone_auth_info_new(linphone_address_get_username(from),NULL,password,NULL,NULL,NULL); /*create authentication structure from identity*/
			linphone_core_add_auth_info(lc,info); /*add authentication info to LinphoneCore*/
		}

		// configure proxy entries
		linphone_proxy_config_set_identity(proxy_cfg,identity); /*set identity with user name and domain*/
		linphone_proxy_config_set_server_addr(proxy_cfg,linphone_address_get_domain(from)); /* we assume domain = proxy server address*/
		linphone_proxy_config_enable_register(proxy_cfg,TRUE); /*activate registration for this proxy config*/
		linphone_proxy_config_enable_publish(proxy_cfg,TRUE); /* enable presence satus publication for this proxy*/
		linphone_address_unref(from); /*release resource*/

		linphone_core_add_proxy_config(lc,proxy_cfg); /*add proxy config to linphone core*/
		linphone_core_set_default_proxy(lc,proxy_cfg); /*set to default proxy*/


		/* Loop until registration status is available */
		do {
			linphone_core_iterate(lc); /* first iterate initiates registration */
			ms_usleep(100000);
		}
		while(	running && linphone_proxy_config_get_state(proxy_cfg) == LinphoneRegistrationProgress);

	}

	if (dest_friend) {
		my_friend = linphone_core_create_friend_with_address(lc, dest_friend); /*creates friend object from dest*/
		if (my_friend == NULL) {
			printf("bad destination uri for friend [%s]\n",dest_friend);
			goto end;
		}

		linphone_friend_enable_subscribes(my_friend,TRUE); /*configure this friend to emit SUBSCRIBE message after being added to LinphoneCore*/
		linphone_friend_set_inc_subscribe_policy(my_friend,LinphoneSPAccept); /* Accept incoming subscription request for this friend*/
		linphone_core_add_friend(lc,my_friend); /* add my friend to the buddy list, initiate SUBSCRIBE message*/

	}

	/*set my status to online*/
	linphone_core_set_presence_model(lc, linphone_presence_model_new_with_activity(LinphonePresenceActivityOnline, NULL));

	/* main loop for receiving notifications and doing background linphone core work: */
	while(running){
		linphone_core_iterate(lc); /* first iterate initiates subscription */
		ms_usleep(50000);
	}

	/* change my presence status to offline*/
	linphone_core_set_presence_model(lc, linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline, NULL));
	linphone_core_iterate(lc); /* just to make sure new status is initiate message is issued */

	linphone_friend_edit(my_friend); /* start editing friend */
	linphone_friend_enable_subscribes(my_friend,FALSE); /*disable subscription for this friend*/
	linphone_friend_done(my_friend); /*commit changes triggering an UNSUBSCRIBE message*/

	linphone_core_iterate(lc); /* just to make sure unsubscribe message is issued */

end:
	printf("Shutting down...\n");
	linphone_core_destroy(lc);
	printf("Exited\n");
	return 0;
}

