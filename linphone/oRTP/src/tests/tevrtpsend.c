/*
  The oRTP LinPhone RTP library intends to provide basics for a RTP stack.
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
#include <ortp/telephonyevents.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <stdio.h>

int runcond=1;

void stophandler(int signum)
{
	runcond=0;
}

static char *help="usage: test_tevsend	filename dest_ip4addr dest_port\n";

int main(int argc, char *argv[])
{
	RtpSession *session;
	unsigned char buffer[160];
	int i;
	FILE *infile;
	char *ssrc;
	uint32_t user_ts=0;
	int tel=0;
	
	if (argc<4){
            	printf("%s",help);
		return -1;
	}
	
	ortp_init();
	ortp_scheduler_init();
	
	/* set the telephony event payload type to 96 in the av profile.*/
	rtp_profile_set_payload(&av_profile,96,&payload_type_telephone_event);
	
	session=rtp_session_new(RTP_SESSION_SENDONLY);
	
	rtp_session_set_scheduling_mode(session,1);
	rtp_session_set_blocking_mode(session,1);
	rtp_session_set_remote_addr(session,argv[2],atoi(argv[3]));
	rtp_session_set_send_payload_type(session,0);
	
	ssrc=getenv("SSRC");
	if (ssrc!=NULL) {
		printf("using SSRC=%i.\n",atoi(ssrc));
		rtp_session_set_ssrc(session,atoi(ssrc));
	}
		
	infile=fopen(argv[1],"rb");
	if (infile==NULL) {
		perror("Cannot open file");
		return -1;
	}
	signal(SIGINT,stophandler);
	while( ((i=fread(buffer,1,160,infile))>0) && (runcond) )
	{
		//ortp_message("Sending packet.");
		rtp_session_send_with_ts(session,buffer,i,user_ts);
		user_ts+=160;
		tel++;
		if (tel==50){
			tel=0;
			ortp_message("Sending telephony event packet.");
			rtp_session_send_dtmf(session,'*',user_ts);
			user_ts+=160+160+160; /* the duration of the dtmf */
		}
	}
	fclose(infile);
	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();
	return 0;
}
