/*
linphone
Copyright (C) 2016  Belledonne Communications SARL
 (simon.morlat@linphone.org)

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


#include "linphone/core.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>

static bool_t running = TRUE;
static const char *passwd = NULL;

static void stop(int signum){
	running=FALSE;
}

static void helper(const char *progname) {
	printf("%s --from <from-uri> --to <to-uri> --route <route> --passwd <pass> --text <text-to-send>\n"
			"\t\t\t--help\n"
			"\t\t\t--from <uri> uri to send from\n"
			"\t\t\t--to <uri> uri to send to\n"
			"\t\t\t--route <uri> uri to send the message through\n"
			"\t\t\t--passwd <authentication password>\n"
			"\t\t\t--text <text to send>\n"
			"\t\t\t--verbose\n", progname);
	exit(0);
}

static void on_msg_state_changed(LinphoneChatMessage *msg, LinphoneChatMessageState state){
	switch(state){
		case LinphoneChatMessageStateInProgress:
			printf("Sending message...\n");
		break;
		case LinphoneChatMessageStateNotDelivered:
			fprintf(stderr, "Error sending message. Use --verbose to troubleshoot.\n");
			running = FALSE;
		break;
		case LinphoneChatMessageStateDelivered:
			printf("Message transmitted succesfully.\n");
			running = FALSE;
		break;
		case LinphoneChatMessageStateDeliveredToUser:
		case LinphoneChatMessageStateDisplayed:
		case LinphoneChatMessageStateFileTransferDone:
		case LinphoneChatMessageStateFileTransferError:
		case LinphoneChatMessageStateIdle:
		break;
	}
}

static void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain){
	running = FALSE;
	if (passwd){
		fprintf(stderr, "Server rejected the supplied username or password\n");
	}else{
		fprintf(stderr, "Authentication requested by server. Please retry by supplying a password with --passwd.\n");
	}
}

int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;
	int i;
	LinphoneAddress *from = NULL;
	LinphoneAddress *to = NULL;
	LinphoneAddress *route = NULL;
	LinphoneProxyConfig *cfg;
	LinphoneChatMessage *msg;
	LinphoneChatRoom *room;
	LinphoneChatMessageCbs *cbs;
	char * text = NULL;
	char *tmp;
	
	signal(SIGINT,stop);

	for(i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--verbose") == 0) {
			linphone_core_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
		} else if (strcmp(argv[i], "--from") == 0){
			i++;
			if (i < argc){
				from = linphone_address_new(argv[i]);
				if (!from) {
					fprintf(stderr, "Incorrect from specified\n");
					return -1;
				}
			}
		} else if (strcmp(argv[i], "--to") == 0){
			i++;
			if (i < argc){
				to = linphone_address_new(argv[i]);
				if (!to) {
					fprintf(stderr, "Incorrect to specified\n");
					return -1;
				}
			}
		}else if (strcmp(argv[i], "--route") == 0){
			i++;
			if (i < argc){
				route = linphone_address_new(argv[i]);
				if (!route) {
					fprintf(stderr, "Incorrect route specified\n");
					return -1;
				}
			}
		} else if (strcmp(argv[i], "--passwd") == 0){
			i++;
			if (i < argc){
				passwd = argv[i];
			}
		} else if (strcmp(argv[i], "--text") == 0){
			i++;
			if (i < argc){
				text = argv[i];
			}
		} else {
			helper(argv[0]);
		}
	}

	if (!from) {
		fprintf(stderr, "Please specify from.\n");
		helper(argv[0]);
		return -1;
	}
	if (!to) {
		fprintf(stderr, "Please specify to.\n");
		helper(argv[0]);
		return -1;
	}
	if (!text){
		fprintf(stderr, "Please specify text to send.\n");
		helper(argv[0]);
		return -1;
	}

	vtable.auth_info_requested = auth_info_requested;
	lc=linphone_core_new(&vtable,NULL, NULL, NULL);
	
	if (passwd){
		LinphoneAuthInfo *ai = linphone_core_create_auth_info(lc, linphone_address_get_username(from), NULL, passwd, NULL, NULL, NULL);
		linphone_core_add_auth_info(lc, ai);
		linphone_auth_info_destroy(ai);
	}
	
	if (!route){
		route = linphone_address_clone(from);
		linphone_address_set_username(route, NULL);
		linphone_address_set_display_name(route, NULL);
	}
	
	cfg = linphone_core_create_proxy_config(lc);
	linphone_proxy_config_set_identity_address(cfg, from);
	tmp = linphone_address_as_string(route);
	linphone_proxy_config_set_server_addr(cfg, tmp);
	ms_free(tmp);
	linphone_proxy_config_enable_register(cfg, FALSE);
	linphone_core_add_proxy_config(lc, cfg);
	linphone_proxy_config_unref(cfg);
	
	room = linphone_core_get_chat_room(lc, to);
	msg = linphone_chat_room_create_message(room, text);
	cbs = linphone_chat_message_get_callbacks(msg);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, on_msg_state_changed);
	linphone_chat_room_send_chat_message(room, msg);
	/* main loop for receiving notifications and doing background linphonecore work: */
	while(running){
		linphone_core_iterate(lc);
		ms_usleep(50000);
	}
	linphone_address_unref(from);
	linphone_address_unref(to);
	linphone_address_unref(route);
	linphone_core_destroy(lc);
	return 0;
}

