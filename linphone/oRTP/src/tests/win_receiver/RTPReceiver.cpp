#include <ortp/ortp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define STREAMS_COUNT 1000

BOOL			m_bExit				= FALSE;

static char *help="usage: mrtprecv	file_prefix local_port number_of_streams \n"
		"Receives multiples rtp streams on local_port+2*k, k={0..number_of_streams}\n";


void ProductVersion()
{
	char	strBuffer[255];

	printf("====================================\n");
	printf("Author  : Simon Morlat             =\n");
	printf("Porting : Yann STEPHAN             =\n");
	printf("====================================\n");	
  	
	memset(&strBuffer, 0x0, sizeof(strBuffer));

	sprintf((char *) &strBuffer, "= RTPReceiver V1.0   - Date : %s - %s\n", __DATE__, __TIME__);
	printf(strBuffer);	

	printf("====================================\n");	
}	

BOOL ctrlHandlerFunction(DWORD fdwCtrlType) 
{ 
	switch (fdwCtrlType) 
	{ 
		// Handle the CTRL+C signal. 
		// CTRL+CLOSE: confirm that the user wants to exit. 
		case CTRL_C_EVENT: 
		case CTRL_CLOSE_EVENT: 
		case CTRL_BREAK_EVENT: 
		case CTRL_LOGOFF_EVENT: 
		case CTRL_SHUTDOWN_EVENT: 
			m_bExit = TRUE;
			return TRUE; 

		default: 
			return FALSE; 
	} 
} 

int rtp2disk(RtpSession *session,uint32_t ts, FILE * fd)
{
	char buffer[160];
	int err,havemore=1;
	
	while (havemore)
	{
		err=rtp_session_recv_with_ts(session,buffer,160,ts,&havemore);
		
		if (havemore) 
			printf("==> Warning: havemore=1!\n");
		
		if (err>0)
		{
			rtp_session_set_data(session,(void*)1);
			/* to indicate that (for the application) the stream has started, so we can start
			recording on disk */
		}

		if  (session->user_data != NULL)
		{
			fwrite(&buffer,1,160, fd);
		}
	}
	return 0;
}

int GetSystemInformation()
{
	SYSTEM_INFO	SystemInfo;

	GetSystemInfo(&SystemInfo);

	return SystemInfo.dwNumberOfProcessors;
}

int __cdecl main(int argc, char *argv[])
{
	RtpSession	*	session[STREAMS_COUNT];
	FILE		*	filefd[STREAMS_COUNT];
	SessionSet	*	set;

	uint32_t			user_ts				= 0;

	int			port				= 0;
	int			channels			= 0;
	int			i					= 0;
	int			nCPUCount			= 0;
	int			nSchedulerCPU		= 2;

	char			strFilename[MAX_PATH];
	
	ProductVersion();

	if (argc<4)
	{
		printf(help);
		return -1;
	}
	
	channels=atoi(argv[3]);
	if (channels==0){
		printf(help);
		return -1;
	}
	
	// Now it's time to use the power of multiple CPUs
	nCPUCount = GetSystemInformation();

	printf("==> # of CPU detected : %d\n", nCPUCount);

	ortp_init();
	ortp_scheduler_init();
	
	if (nCPUCount > 1)
	{
		if (nCPUCount > 2)
		{
			nSchedulerCPU	= 3;
		}

/*		if (ortp_bind_scheduler_to_cpu(nSchedulerCPU) != -1)
		{
			printf("==> Scheduler has been binded to CPU %d\n", nSchedulerCPU);
		}
		else
		{
			printf("==> Scheduler still binded to CPU 1\n");
		}
*/
	}

	port=atoi(argv[2]);

	for (i=0;i<channels;i++)
	{
		session[i]=rtp_session_new(RTP_SESSION_RECVONLY);	
		rtp_session_set_scheduling_mode(session[i],1);
		rtp_session_set_blocking_mode(session[i],0);
		rtp_session_set_local_addr(session[i],"0.0.0.0",port);
		rtp_session_set_send_payload_type(session[i],0);
		rtp_session_enable_adaptive_jitter_compensation(session[i], TRUE);
		rtp_session_set_recv_buf_size(session[i],256);
		port+=2;
	}
		
	memset(strFilename, 0x0, sizeof(strFilename));

	for (i=0;i<channels;i++)
	{
		sprintf(strFilename,"%s%4.4d.dat",argv[1],i);

		filefd[i]=fopen(strFilename, "wb");

		if (filefd[i]<0) 
		{
			printf("Could not open %s for writing: %s",strFilename,strerror(errno));
		}
	}

	// =============== INSTALL THE CONTROL HANDLER ===============
	if (SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrlHandlerFunction, TRUE) == 0)
	{
		printf("==> Cannot handle the CTRL-C...\n");
	}

	/* create a set */
	set=session_set_new();
	printf("==> RTP Receiver started\n");

	while(m_bExit == FALSE)
	{
		int k;
		
		for (k=0;k<channels;k++){
			/* add the session to the set */
			session_set_set(set,session[k]);
			//printf("session_set_set %d\n", k);
		}
		/* and then suspend the process by selecting() */
		k=session_set_select(set,NULL,NULL);
		//printf("session_set_select\n");
		if (k==0)
		{
			printf("==> Warning: session_set_select() is returning 0...\n");
		}

		for (k=0;k<channels;k++){
			if (session_set_is_set(set,session[k]))
			{
				rtp2disk(session[k],user_ts,filefd[k]);
				//printf("==> Session_set_is_set %d\n", k);
			} 
			else
			{
				//printf("warning: session %i is not set !\n",k);
			}
		}
		user_ts+=160;
	}

	printf("==> Exiting\n");

	for (i=0;i<channels;i++)
	{
		fclose(filefd[i]);
		rtp_session_destroy(session[i]);
	}
	session_set_destroy(set);

	ortp_exit();

	ortp_global_stats_display();

	printf("Waiting for exit : ");

	for (i = 0; i < 4*5; i++)
	{
		printf(".");
		Sleep(250);
	}

	return 0;
}

