  /*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <ortp/ortp.h>
#include <signal.h>
#include <stdlib.h>

#ifndef _WIN32 
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#endif

int runcond=1;

void stophandler(int signum)
{
	runcond=0;
}

static const char *help="usage: rtpsend	filename dest_ip4addr dest_port [ --with-clockslide <value> ] [ --with-jitter <milliseconds>]\n";

int main(int argc, char *argv[])
{
	RtpSession *session;
	unsigned char buffer[160];
	int i;
	FILE *infile;
	char *ssrc;
	uint32_t user_ts=0;
	int clockslide=0;
	int jitter=0;
	if (argc<4){
		printf("%s", help);
		return -1;
	}
	for(i=4;i<argc;i++){
		if (strcmp(argv[i],"--with-clockslide")==0){
			i++;
			if (i>=argc) {
				printf("%s", help);
				return -1;
			}
			clockslide=atoi(argv[i]);
			ortp_message("Using clockslide of %i milisecond every 50 packets.",clockslide);
		}else if (strcmp(argv[i],"--with-jitter")==0){
			ortp_message("Jitter will be added to outgoing stream.");
			i++;
			if (i>=argc) {
				printf("%s", help);
				return -1;
			}
			jitter=atoi(argv[i]);
		}
	}
	
	ortp_init();
	ortp_scheduler_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
	session=rtp_session_new(RTP_SESSION_SENDONLY);	
	
	rtp_session_set_scheduling_mode(session,1);
	rtp_session_set_blocking_mode(session,1);
	rtp_session_set_connected_mode(session,TRUE);
	rtp_session_set_remote_addr(session,argv[2],atoi(argv[3]));
	rtp_session_set_payload_type(session,0);
	
	ssrc=getenv("SSRC");
	if (ssrc!=NULL) {
		printf("using SSRC=%i.\n",atoi(ssrc));
		rtp_session_set_ssrc(session,atoi(ssrc));
	}
		
	#ifndef _WIN32
	infile=fopen(argv[1],"r");
	#else
	infile=fopen(argv[1],"rb");
	#endif

	if (infile==NULL) {
		perror("Cannot open file");
		return -1;
	}

	signal(SIGINT,stophandler);
	while( ((i=fread(buffer,1,160,infile))>0) && (runcond) )
	{
		rtp_session_send_with_ts(session,buffer,i,user_ts);
		user_ts+=160;
		if (clockslide!=0 && user_ts%(160*50)==0){
			ortp_message("Clock sliding of %i miliseconds now",clockslide);
			rtp_session_make_time_distorsion(session,clockslide);
		}
		/*this will simulate a burst of late packets */
		if (jitter && (user_ts%(8000)==0)) {
			struct timespec pausetime, remtime;
			ortp_message("Simulating late packets now (%i milliseconds)",jitter);
			pausetime.tv_sec=jitter/1000;
			pausetime.tv_nsec=(jitter%1000)*1000000;
			while(nanosleep(&pausetime,&remtime)==-1 && errno==EINTR){
				pausetime=remtime;
			}
		}
	}

	fclose(infile);
	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();

	return 0;
}
