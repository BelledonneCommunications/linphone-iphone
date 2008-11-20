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
#include <assert.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <stdio.h>

int runcond=1;

static int dtmf_tab[16]={'0','1','2','3','4','5','6','7','8','9','*','#','A','B','C','D'};

void stophandler(int signum)
{
	runcond=0;
}

static char *help="usage: test_tevrecv	filename loc_port\n";

int dtmf_count=0;

void recv_tev_cb(RtpSession *session,int type,long user_data)
{
	printf("Receiving telephony event:%i\n",type);
	if (type<16) printf("This is dtmf %c\n",dtmf_tab[type]);
	dtmf_count++;
}

int main(int argc, char *argv[])
{
	RtpSession *session;
	unsigned char buffer[160];
	int err;
	FILE *outfile;
	uint32_t ts=0;
	int have_more;

	if (argc<3){
		printf("%s",help);
		return -1;
	}

	ortp_init();
	ortp_scheduler_init();
	
	/* set the telephony event payload type to 96 in the av profile.*/
	rtp_profile_set_payload(&av_profile,96,&payload_type_telephone_event);
	
	session=rtp_session_new(RTP_SESSION_RECVONLY);	
	
	rtp_session_set_scheduling_mode(session,1);
	rtp_session_set_blocking_mode(session,1);
	rtp_session_set_local_addr(session,"0.0.0.0",atoi(argv[2]));
	rtp_session_set_payload_type(session,0);
	
	/* register for telephony events */
	rtp_session_signal_connect(session,"telephone-event",(RtpCallback)recv_tev_cb,0);
		
	outfile=fopen(argv[1],"wb");
	if (outfile==NULL) {
		perror("Cannot open file");
		return -1;
	}
	signal(SIGINT,stophandler);
	while(runcond)
	{
		have_more=1;
		while (have_more){
			err=rtp_session_recv_with_ts(session,buffer,160,ts,&have_more);
			if (err>0) {
				size_t ret = fwrite(buffer,1,err,outfile);
				assert( ret == err );
			}
		}
		ts+=160;
		//ortp_message("Receiving packet.");
	}
	fclose(outfile);
	rtp_session_destroy(session);
	ortp_exit();
	ortp_global_stats_display();
	printf("Total dtmf events received: %i\n",dtmf_count);
	return 0;
}
