#include <ortp/ortp.h>
#include <string.h>

#define STREAMS_COUNT 1000

enum 
{
    EVENT_STOP,
    EVENT_RTP,
    EVENT_COUNT						//  Always last
};


RtpSession *	m_Session[STREAMS_COUNT];

int				m_nPacket_Size		= 160;
int				m_nTimestamp_Inc	= 160;

char		*	m_pBuffer			= NULL;
char		*	m_SSRC				= NULL;

int				m_nChannels			= 0;
int				m_nPort				= 0;

HANDLE			m_hEvents[EVENT_COUNT];

BOOL			m_bExit				= FALSE;

static char *help="usage: mrtpsend	filename ip port nstreams [--packet-size size] [--ts-inc value]\n";

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
			SetEvent(m_hEvents[EVENT_STOP]);
			return TRUE; 

		default: 
			return FALSE; 
	} 
} 

int GetCommandArguments(int argc, char *argv[])
{
	int				nCounter;

	// Check the number of arguments
	if (argc<5)
	{
		printf(help);
		return -1;
	}

	m_nChannels = atoi(argv[4]);

	// Get the number of channels
	if (m_nChannels == 0)
	{
		printf(help);
		return -1;
	}

	/* look at command line options */
	for (nCounter=5; nCounter<argc; nCounter++)
	{
		if (strcmp(argv[nCounter],"--packet-size")==0)
		{
			if ( nCounter+1 < argc ){
				m_nPacket_Size=atoi(argv[nCounter+1]);
			}
			else {
				printf(help);
				return -1;
			}
			if (m_nPacket_Size==0)
			{
				printf("Packet size can't be %s.\n",argv[nCounter+1]);
				return -1;
			}
			nCounter+=1;
			
		}
		else if (strcmp(argv[nCounter],"--ts-inc")==0)
		{
			if ( nCounter+1 < argc )
			{
				m_nTimestamp_Inc=atoi(argv[nCounter+1]);
			}
			else {
				printf(help);
				return -1;
			}
			if (m_nTimestamp_Inc==0) 
			{
				printf("Timestanp increment can't be %s.\n",argv[nCounter+1]);
				return -1;
			}

			nCounter+=1;			
		}
	}

	return 0;
}

void ProductVersion()
{
	char	strBuffer[255];

	printf("====================================\n");
	printf("Author  : Simon Morlat             =\n");
	printf("Porting : Yann STEPHAN             =\n");
	printf("====================================\n");	
  	
	memset(&strBuffer, 0x0, sizeof(strBuffer));

	sprintf((char *) &strBuffer, "= RTPSender V1.0   - Date : %s - %s\n", __DATE__, __TIME__);
	printf(strBuffer);	

	printf("====================================\n");	
}	

int __cdecl main(int argc, char *argv[])
{
	FILE		*	infile				= NULL;
	SessionSet	*	pSessionSet			= NULL;
	int				nCounter			= 0;
	UINT32			m_nUser_Timestamp	= 0;

	ProductVersion();

	if (GetCommandArguments(argc, argv) != 0)
	{
		printf("==> Sorry dude...\n");
		Sleep(1000);
		return -1;
	}

	printf("==> Starting the RTP Sender test\n");


	// =============== INSTALL THE CONTROL HANDLER ===============
	if (SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrlHandlerFunction, TRUE) == 0)
	{
		printf("==> Cannot handle the CTRL-C...\n");
	}


	printf("==> Timestamp increment will be %i\n"	, m_nTimestamp_Inc);
	printf("==> Packet size will be %i\n"			, m_nPacket_Size);

	m_pBuffer = (char *) ortp_malloc(m_nPacket_Size);

	ortp_init();
	ortp_scheduler_init();
	printf("==> Scheduler initialized\n");

	m_SSRC	= getenv("SSRC");
	m_nPort	= atoi(argv[3]);

	for (nCounter=0; nCounter < m_nChannels; nCounter++)
	{
		//printf("==> Channel [#%d]\n", nCounter);

		m_Session[nCounter] = rtp_session_new(RTP_SESSION_SENDONLY);	

		rtp_session_set_scheduling_mode(m_Session[nCounter],1);
		rtp_session_set_blocking_mode(m_Session[nCounter],0);
		rtp_session_set_remote_addr(m_Session[nCounter],argv[2], m_nPort);
		rtp_session_set_send_payload_type(m_Session[nCounter],0);
		
		if (m_SSRC != NULL) 
		{
			rtp_session_set_ssrc(m_Session[nCounter],atoi(m_SSRC));
		}

		m_nPort+=2;
	}

	infile=fopen(argv[1],"rb");

	if (infile==NULL) 
	{
		printf("==> Cannot open file !!!!");
		Sleep(1000);
		return -1;
	}

//	printf("==> Open file\n");
	
	/* Create a set */
	pSessionSet = session_set_new();
//	printf("==> Session set\n");

	while( ((nCounter= (int) fread(m_pBuffer,1,m_nPacket_Size,infile))>0) && (m_bExit == FALSE) )
	{
		int k;
		//g_message("Sending packet.");
		for (k=0;k<m_nChannels;k++){	
			/* add the session to the set */
			session_set_set(pSessionSet,m_Session[k]);
			//printf("==> Session set set %d\n", k);
		}
		/* and then suspend the process by selecting() */
		session_set_select(NULL,pSessionSet,NULL);
		//printf("==> Session set select\n");

		for (k=0;k<m_nChannels;k++)
		{
			//printf("---\n");
			/* this is stupid to do this test, because all session work the same way,
			as the same user_ts is used for all sessions, here. */
			if (session_set_is_set(pSessionSet,m_Session[k]))
			{
				//printf("==> Session set is set %d\n", k);
				rtp_session_send_with_ts(m_Session[k],m_pBuffer,nCounter,m_nUser_Timestamp);
				//g_message("packet sended !");
			}
		}
		m_nUser_Timestamp+=m_nTimestamp_Inc;
	}

	fclose(infile);
	printf("==> Close file\n");



	for(nCounter=0;nCounter<m_nChannels;nCounter++)
	{
		rtp_session_destroy(m_Session[nCounter]);
	}

	session_set_destroy(pSessionSet);

	// Give us some time
	Sleep(250);

	ortp_exit();
	ortp_global_stats_display();

	ortp_free(m_pBuffer);

	printf("==> Remove the CTRL-C handler...\n");
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ctrlHandlerFunction, FALSE);

	// Wait for an input key
	printf("Waiting for exit : ");

	for (nCounter = 0; nCounter < 4*5; nCounter++)
	{
		printf(".");
		Sleep(250);
	}

	return 0;
}

