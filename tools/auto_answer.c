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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "linphone/core.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>

static bool_t running=TRUE;
static bool_t print_stats=FALSE;
static bool_t dump_stats=FALSE;

static void stop(int signum){
	running=FALSE;
}
#ifndef _WIN32
static void stats(int signum){
	print_stats=TRUE;
}
static void dump_call_logs(int signum){
	dump_stats=TRUE;
}

#endif
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
			ms_message("Incoming call arriving !\n");
			/* accept the incoming call*/
			call_params = linphone_core_create_call_params(lc, call);
			linphone_call_params_enable_video(call_params,TRUE);
			linphone_call_params_set_audio_direction(call_params,LinphoneMediaDirectionSendOnly);
			linphone_call_params_set_video_direction(call_params,LinphoneMediaDirectionSendOnly);
			linphone_core_accept_call_with_params(lc,call,call_params);
			linphone_call_params_unref(call_params);
		break;
		default:
			break;
	}
}
extern MSWebCamDesc mire_desc;
static void helper(const char *progname) {
	printf("%s --help\n"
			"\t\t\t--listening-uri <uri> uri to listen on, default [sip:localhost:5060]\n"
			"\t\t\t--max-call-duration max duration of a call in seconds, default [3600]\n"
			"\t\t\t--media-file <wav or mkv file to play to caller>\n"
			"\t\t\t--verbose\n", progname);
	exit(0);
}

int main(int argc, char *argv[]){
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc;
	LinphoneVideoPolicy policy;
	int i;
	LinphoneAddress *addr=NULL;
	LCSipTransports tp;
	char * tmp = NULL;
	LpConfig * lp_config = lp_config_new(NULL);
	int max_call_duration=3600;
	static const char *media_file = NULL;

	policy.automatically_accept=TRUE;
	signal(SIGINT,stop);
#ifndef _WIN32
	signal(SIGUSR1,stats);
	signal(SIGUSR2,dump_call_logs);
#endif
	for(i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--verbose") == 0) {
			linphone_core_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
		} else if (strcmp(argv[i], "--max-call-duration") == 0){
			max_call_duration = atoi(argv[++i]);
		} else if (strcmp(argv[i], "--listening-uri") == 0){
			addr = linphone_address_new(argv[++i]);
			if (!addr) {
				printf("Error, bad sip uri");
				helper(argv[0]);
			}
/*			switch(linphone_address_get_transport(addr)) {
			case LinphoneTransportUdp:
			case LinphoneTransportTcp:
				break;
			default:
				ms_error("Error, bad sip uri [%s] transport, should be udp | tcp",argv[i]);
				helper();
				break;
			}*/
		} else if (strcmp(argv[i], "--media-file") == 0){
			i++;
			if (i<argc){
				media_file = argv[i];
			}else helper(argv[0]);
		} else {
			helper(argv[0]);
		}
	}

	if (!addr) {
		addr = linphone_address_new("sip:bot@0.0.0.0:5060");
	}

	lp_config_set_string(lp_config,"sip","bind_address",linphone_address_get_domain(addr));
	lp_config_set_string(lp_config,"rtp","bind_address",linphone_address_get_domain(addr));
	lp_config_set_int(lp_config,"misc","history_max_size",100000);

	vtable.call_state_changed=call_state_changed;

	lc=linphone_core_new_with_config(&vtable,lp_config,NULL);
	linphone_core_enable_video_capture(lc,TRUE);
	linphone_core_enable_video_display(lc,FALSE);
	linphone_core_set_video_policy(lc,&policy);
	linphone_core_enable_keep_alive(lc,FALSE);


	/*instead of using sound capture card, a file is played to the calling party*/
	linphone_core_set_use_files(lc,TRUE);
	linphone_core_enable_echo_cancellation(lc, FALSE); /*no need for local echo cancellation when playing files*/
	if (!media_file){
		linphone_core_set_play_file(lc,PACKAGE_DATA_DIR "/sounds/linphone/hello16000.wav");
		linphone_core_set_preferred_framerate(lc,5);
	}else{
		PayloadType *pt = linphone_core_find_payload_type(lc, "opus", 48000, -1);
		/*if opus is present, give it a bitrate for good quality with music, and stereo enabled*/
		if (pt){
			linphone_core_set_payload_type_bitrate(lc, pt, 150);
			payload_type_set_send_fmtp(pt, "stereo=1");
			payload_type_set_recv_fmtp(pt, "stereo=1");
		}
		linphone_core_set_play_file(lc, media_file);
		linphone_core_set_preferred_video_size_by_name(lc, "720p");
	}

	{
		MSWebCamDesc *desc = ms_mire_webcam_desc_get();
		if (desc){
			ms_web_cam_manager_add_cam(ms_factory_get_web_cam_manager(linphone_core_get_ms_factory(lc)),ms_web_cam_new(desc));
			linphone_core_set_video_device(lc,"Mire: Mire (synthetic moving picture)");
		}
	}



	memset(&tp,0,sizeof(LCSipTransports));

	tp.udp_port = linphone_address_get_port(addr);
	tp.tcp_port = linphone_address_get_port(addr);

	linphone_core_set_sip_transports(lc,&tp);
	linphone_core_set_audio_port_range(lc,1024,65000);
	linphone_core_set_video_port_range(lc,1024,65000);
	linphone_core_set_primary_contact(lc,tmp=linphone_address_as_string(addr));
	ms_free(tmp);

	/* main loop for receiving notifications and doing background linphonecore work: */
	while(running){
		const bctbx_list_t * iterator;
		linphone_core_iterate(lc);
		ms_usleep(50000);
		if (print_stats) {
			ms_message("*********************************");
			ms_message("*Current number of calls   [%10u]  *", (unsigned int)bctbx_list_size(linphone_core_get_calls(lc)));
			ms_message("*Number of calls until now [%10u]  *", (unsigned int)bctbx_list_size(linphone_core_get_call_logs(lc)));
			ms_message("*********************************");
			print_stats=FALSE;
		}
		if (dump_stats) {
			ms_message("*********************************");
			for (iterator=linphone_core_get_call_logs(lc);iterator!=NULL;iterator=iterator->next) {
				LinphoneCallLog *call_log=(LinphoneCallLog *)iterator->data;
				char * tmp_str = linphone_call_log_to_str(call_log);
				ms_message("\n%s",tmp_str);
				ms_free(tmp_str);
			}
			dump_stats=FALSE;
			ms_message("*********************************");
		}
		for (iterator=linphone_core_get_calls(lc);iterator!=NULL;iterator=iterator->next) {
			LinphoneCall *call=(LinphoneCall *)iterator->data;
			if (linphone_call_get_duration(call) > max_call_duration) {
				ms_message("Terminating call [%p] after [%i] s",call,linphone_call_get_duration(call));
				linphone_core_terminate_call(lc,call);
				break;
			}
		}

	}

	ms_message("Shutting down...\n");
	linphone_core_destroy(lc);
	ms_message("Exited\n");
	return 0;
}




