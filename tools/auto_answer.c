/*
linphone
Copyright (C) 2010  Belledonne Communications SARL
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#ifdef IN_LINPHONE
#include "linphonecore.h"
#else
#include "linphone/linphonecore.h"
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>

static bool_t running=TRUE;

static void stop(int signum){
	running=FALSE;
}

#ifndef PACKAGE_DATA_DIR
	#define PACKAGE_DATA_DIR  '.'
#endif
/*
 * Call state notification callback
 */
static void call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg){
	LinphoneCallParams * call_params;
	switch(cstate){
		case LinphoneCallIncomingReceived:
			printf("Incoming call arrive  !\n");
			/* accept the incoming call*/
			call_params = linphone_core_create_default_call_parameters(lc);
			linphone_call_params_set_audio_direction(call_params,LinphoneMediaDirectionSendOnly);
			linphone_call_params_set_video_direction(call_params,LinphoneMediaDirectionSendOnly);
			linphone_core_accept_call_with_params(lc,call,call_params);
			linphone_call_params_destroy(call_params);

		break;
		case LinphoneCallOutgoingEarlyMedia:
			printf("Receiving some early media\n");
		break;
		case LinphoneCallConnected:
			printf("We are connected !\n");
		break;
		case LinphoneCallStreamsRunning:
			printf("Media streams established !\n");
		break;
		case LinphoneCallEnd:
			printf("Call is terminated.\n");
		break;
		case LinphoneCallError:
			printf("Call failure !");
		break;
		default:
			printf("Unhandled notification %i\n",cstate);
	}
}
extern MSWebCamDesc mire_desc;
static void helper() {
	printf("auto_answer --help\n"
			"\t\t\t--listening-uri <uri> uri to listen on, default [sip:localhost:5060]\n"
			"\t\t\t--verbose\n");
	return ;
}

int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;
	LinphoneVideoPolicy policy;
	int i;
	LinphoneAddress *addr=NULL;
	LCSipTransports tp;
	char * tmp = NULL;

	policy.automatically_accept=TRUE;
	signal(SIGINT,stop);
	for(i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--verbose") == 0) {
			linphone_core_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
		} else if (strcmp(argv[i], "--listening-uri") == 0){
			addr = linphone_address_new(argv[++i]);
			if (!addr) {
				printf("Error, bad sip uri");
				helper();
			}
			switch(linphone_address_get_transport(addr)) {
			case LinphoneTransportUdp:
			case LinphoneTransportTcp:
				break;
			default:
				printf("Error, bad sip uri [%s] transport, should be udp | tcp",argv[i]);
				helper();
				break;
			}
		} else {
			helper();
		}
	}


	linphone_core_enable_logs(NULL); /*enable liblinphone logs.*/
	/*
	 Fill the LinphoneCoreVTable with application callbacks.
	 All are optional. Here we only use the call_state_changed callbacks
	 in order to get notifications about the progress of the call.
	 */
	vtable.call_state_changed=call_state_changed;

	/*
	 Instanciate a LinphoneCore object given the LinphoneCoreVTable
	*/
	lc=linphone_core_new(&vtable,NULL,NULL,NULL);

	linphone_core_enable_video_capture(lc,TRUE);
	linphone_core_enable_video_display(lc,FALSE);
	linphone_core_set_video_policy(lc,&policy);


	/*instead of using sound capture card, a file is played to the calling party*/
	linphone_core_set_play_file(lc,PACKAGE_DATA_DIR "/sounds/linphone/hello16000.wav");
	linphone_core_set_use_files(lc,TRUE);

	{
		MSWebCamDesc *desc = ms_mire_webcam_desc_get();
		if (desc){
			ms_web_cam_manager_add_cam(ms_web_cam_manager_get(),ms_web_cam_new(desc));
			linphone_core_set_video_device(lc,"Mire: Mire (synthetic moving picture)");
		}
	}

	if (!addr) {
		addr = linphone_address_new("sip:bot@localhost:5060");
	}

	memset(&tp,0,sizeof(LCSipTransports));
	switch(linphone_address_get_transport(addr)) {
	case LinphoneTransportUdp:
		tp.udp_port = linphone_address_get_port(addr);
		break;
	case LinphoneTransportTcp:
		tp.tcp_port = linphone_address_get_port(addr);
		break;
	default:
		break;
	}
	linphone_core_set_sip_transports(lc,&tp);

	linphone_core_set_primary_contact(lc,tmp=linphone_address_as_string(addr));
	ms_free(tmp);

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




