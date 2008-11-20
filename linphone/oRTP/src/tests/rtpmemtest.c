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

/* this program shows how to receive streams in paralel using the SessionSet api 
	and two threads only. */

#include <ortp/ortp.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifndef _WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#else
//#include <time.h>
#endif

int runcond=1;

void stophandler(int signum)
{
	runcond=0;
}

static char *help="usage: mrtprecv	file_prefix local_port number_of_streams \n"
		"Receives multiples rtp streams on local_port+2*k, k={0..number_of_streams}\n";

#define STREAMS_COUNT 1000

int rtp2disk(RtpSession *session,uint32_t ts, int fd)
{
	unsigned char buffer[160];
	int err,havemore=1;
	while (havemore){
		err=rtp_session_recv_with_ts(session,buffer,160,ts,&havemore);
		if (err>0){
			rtp_session_set_data(session,(void*)1);
			/* to indicate that (for the application) the stream has started, so we can start
			recording on disk */
		}
		if (session->user_data != NULL) {
			size_t ret = write(fd,buffer,err);
			assert( ret == err );
		}
	}
	return 0;
}


int main(int argc, char *argv[])
{
	RtpSession *session[STREAMS_COUNT];
	int i;
	int filefd[STREAMS_COUNT];
	int port;
	uint32_t user_ts=0;
	int channels;
	SessionSet *set;
	char *filename;

	argc=4;
	argv[1]="/tmp/output";
	argv[2]="8000";
	argv[3]="100";

	if (argc<4){
		printf("%s",help);
		return -1;
	}

	channels=atoi(argv[3]);
	if (channels==0){
		printf("%s",help);
		return -1;
	}

	ortp_init();
	ortp_scheduler_init();

	port=atoi(argv[2]);
	for (i=0;i<channels;i++){
		session[i]=rtp_session_new(RTP_SESSION_RECVONLY);
		rtp_session_set_scheduling_mode(session[i],1);
		rtp_session_set_blocking_mode(session[i],0);
#ifdef ORTP_INET6
		rtp_session_set_local_addr(session[i],"::",port);
#else
		rtp_session_set_local_addr(session[i],"0.0.0.0",port);
#endif
		rtp_session_set_payload_type(session[i],0);
		rtp_session_set_recv_buf_size(session[i],256);
		port+=2;
	}

	filename=ortp_malloc(strlen(argv[1])+8);
	for (i=0;i<channels;i++){
		sprintf(filename,"%s%4.4d.dat",argv[1],i);
		filefd[i]=open(filename,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
		if (filefd[i]<0) ortp_error("Could not open %s for writing: %s",filename,strerror(errno));
	}
	ortp_free(filename);
	signal(SIGINT,stophandler);
	/* create a set */
	set=session_set_new();
	while(runcond)
	{
		int k;

		for (k=0;k<channels;k++){
			/* add the session to the set */
			session_set_set(set,session[k]);
			//printf("session[k]->setflags=%i\n",session[k]->setflags);
		}
		/* and then suspend the process by selecting() */
		session_set_select(set,NULL,NULL);
		for (k=0;k<channels;k++){
			if (session_set_is_set(set,session[k])){
				rtp2disk(session[k],user_ts,filefd[k]);
			}
		}
		user_ts+=160;
	}
	for (i=0;i<channels;i++){
		close(filefd[i]);
		rtp_session_destroy(session[i]);
	}
	session_set_destroy(set);
	ortp_global_stats_display();
	ortp_exit();
	return 0;
}
