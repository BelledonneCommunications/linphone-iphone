/****************************************************************************
 *  Copyright (C) 2009  Simon MORLAT <simon.morlat@linphone.org>
 *
 ****************************************************************************
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 ****************************************************************************/


#include <stdio.h>
#include <stdlib.h>


#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include <ws2tcpip.h>
#include <ctype.h>
#include <conio.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>

#endif

#include "ortp/ortp.h"

#define DEFAULT_REPLY_SIZE 4096

#define STATUS_REGISTERED (1<<0)
#define STATUS_REGISTERING (1<<1)
#define STATUS_DIALING (1<<2)
#define STATUS_AUTOANSWER (1<<3)
#define STATUS_IN_CONNECTED (1<<4) /* incoming call accepted */
#define STATUS_OUT_CONNECTED (1<<5) /*outgoing call accepted */


static int make_status_value(const char *status_string){
	int ret=0;
	if (strstr(status_string,"registered, identity=")){
		ret|=STATUS_REGISTERED;
	}
	if (strstr(status_string,"registered=-1")){
		ret|=STATUS_REGISTERING;
	}
	if (strstr(status_string,"autoanswer=1")){
		ret|=STATUS_AUTOANSWER;
	}
	if (strstr(status_string,"dialing")){
		ret|=STATUS_DIALING;
	}
	if (strstr(status_string,"Call out")){
		ret|=STATUS_OUT_CONNECTED;
	}
	if (strstr(status_string,"hook=answered")){
		ret|=STATUS_IN_CONNECTED;
	}
	return ret;
}

static int send_command(const char *command, char *reply, int reply_len, int print_errors){
	ortp_pipe_t pp;
	int i;
	int err;
	char path[128];
#ifndef WIN32
	snprintf(path,sizeof(path)-1,"linphonec-%i",getuid());
#else
	{
		char username[128];
		DWORD size=sizeof(username)-1;
		GetUserName(username,&size);
		snprintf(path,sizeof(path)-1,"linphonec-%s",username);
	}
#endif
	if ((pp=ortp_client_pipe_connect(path))==ORTP_PIPE_INVALID){
		if (print_errors) fprintf(stderr,"ERROR: Failed to connect pipe: %s\n",strerror(errno));
		return -1;
	}
	if (ortp_pipe_write(pp,(uint8_t*)command,strlen(command))==-1){
		if (print_errors) fprintf(stderr,"ERROR: Fail to send command to remote linphonec\n");
		ortp_client_pipe_close(pp);
		return -1;
	}
	/*wait for replies */
	i=0;
	while ((err=ortp_pipe_read(pp,(uint8_t*)&reply[i],reply_len-i-1))>0){
		i+=err;
	}
	reply[i]='\0';
	ortp_client_pipe_close(pp);
	return 0;
}

static void print_usage(void){
	fprintf(stderr,"Usage:\nlinphonecsh <action> [arguments]\n"
			"where action is one of\n"
			"\tinit\t\t: spawn a linphonec daemon (first step to make other actions)\n"
			"\t\t\tfollowed by the arguments sent to linphonec\n"
			"\tgeneric\t\t: sends a generic command to the running linphonec daemon\n"
			"\t\t\tfollowed by the generic command surrounded by quotes,\n\t\t\t for example \"call sip:joe@example.net\"\n"
			"\tregister\t: register; arguments are \n\t\t\t--host <host>\n\t\t\t--username <username>\n\t\t\t--password <password>\n"
			"\tunregister\t: unregister\n"
			"\tdial\t\t: dial <sip uri or number>\n"
			"\tstatus\t\t: can be 'status register', 'status autoanswer' or 'status hook'\n"
			"\tsoundcard\t: can be 'soundcard capture', 'soundcard playback', 'soundcard ring',\n"
			"\t\t\t followed by an optional number representing the index of the soundcard,\n"
			"\t\t\t in which case the soundcard is set instead of just read.\n"
			"\texit\t\t: make the linphonec daemon to exit.\n"
	);
	exit(-1);
}

#define MAX_ARGS 10

#ifndef WIN32
static void spawn_linphonec(int argc, char *argv[]){
	char * args[10];
	int i,j;
	pid_t pid;
	j=0;
	args[j++]="linphonec";
	args[j++]="--pipe";
	args[j++]="-c";
	args[j++]="/dev/null";
	for(i=0;i<argc;++i){
		args[j++]=argv[i];
	}
	args[j++]=NULL;

#ifdef __uClinux__
	pid = vfork();
#else
	pid = fork();
#endif
	if (pid < 0){
		fprintf(stderr,"Could not fork\n");
		exit(-1);
	}
	if (pid == 0) {
		int fd;
		/*we are the new process*/
		setsid();
		
		fd = open("/dev/null", O_RDWR);
		if (fd==-1){
			fprintf(stderr,"Could not open /dev/null\n");
			exit(-1);
		}
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		
		if (execvp("linphonec",args)==-1){
			fprintf(stderr,"Fail to spawn linphonec: %s\n",strerror(errno));
			exit(-1);
		}
	}
}
#else

static void spawn_linphonec(int argc, char *argv[]){
	PROCESS_INFORMATION pinfo;
	STARTUPINFO si;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pinfo, sizeof(pinfo) );


	BOOL ret=CreateProcess(NULL,"linphoned.exe --pipe -c NUL",
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pinfo);
	if (!ret){
		fprintf(stderr,"Spawning of linphonec.exe failed.\n");
	}else{
		WaitForInputIdle(pinfo.hProcess,1000);
	}
}

#endif

static int send_generic_command(const char *command, int print_result){
	char reply[DEFAULT_REPLY_SIZE];
	int err;
	err=send_command(command,reply,sizeof(reply),print_result);
	if (err==0 && print_result) {
		printf("%s",reply);
		fflush(stdout);
	}
	return err;
}

static int register_execute(int argc, char *argv[]){
	char cmd[512];
	char *username=NULL;
	char *host=NULL;
	char *passwd=NULL;
	int i;
	for(i=0;i<argc;++i){
		if (strcmp(argv[i],"--host")==0){
			i++;
			if (i<argc){
				host=argv[i];
			}else print_usage();
		}else if (strcmp(argv[i],"--username")==0){
			i++;
			if (i<argc){
				username=argv[i];
			}else print_usage();
		}else if (strcmp(argv[i],"--password")==0){
			i++;
			if (i<argc){
				passwd=argv[i];
			}else print_usage();
		}else print_usage();
	}
	if (username==NULL) {
		fprintf(stderr,"Missing --username\n");
		print_usage();
	}
	if (host==NULL) {
		fprintf(stderr,"Missing --host\n");
		print_usage();
	}
	if (passwd) snprintf(cmd,sizeof(cmd),"register sip:%s@%s sip:%s %s",username,host,host,passwd);
	else snprintf(cmd,sizeof(cmd),"register sip:%s@%s sip:%s",username,host,host);
	return send_generic_command(cmd,TRUE);
}

static int unregister_execute(int argc, char *argv[]){
	return send_generic_command("unregister",FALSE);
}


static int dial_execute(int argc, char *argv[]){
	char cmd[512];
	if (argc==1){
		snprintf(cmd,sizeof(cmd),"call %s",argv[0]);
		return send_generic_command(cmd,TRUE);
	}else{
		print_usage();
	}
	return -1;
}

static int status_execute(int argc, char *argv[]){
	char cmd[512];
	char reply[DEFAULT_REPLY_SIZE];
	int err;
	
	if (argc==1){
		snprintf(cmd,sizeof(cmd),"status %s",argv[0]);
		err=send_command(cmd,reply,sizeof(reply),TRUE);
		if (err==0) {
			printf("%s",reply);
			err=make_status_value(reply);
		}
		return err;
	}else{
		print_usage();
	}
	return -1;
}

static int parse_card_index(const char *reply){
	int index=-1;
	reply=strstr(reply,"device #");
	if (!reply || sscanf(reply,"device #%i",&index)!=1){
		fprintf(stderr,"Error while parsing linphonec daemon output !\n");
	}
	return index;
}

static int soundcard_execute(int argc, char *argv[]){
	char cmd[512];
	char reply[DEFAULT_REPLY_SIZE];
	int err;
	if (argc==1){
		snprintf(cmd,sizeof(cmd),"soundcard %s",argv[0]);
		err=send_command(cmd,reply,sizeof(reply),TRUE);
		if (err==0) {
			printf("%s",reply);
			return parse_card_index(reply);
		}
	}else if (argc==2){/*setting a soundcard */
		snprintf(cmd,sizeof(cmd),"soundcard %s %s",argv[0],argv[1]);
		err=send_command(cmd,reply,sizeof(reply),TRUE);
		if (err==0) {
			printf("%s",reply);
			return 0;
		}
	}else{
		print_usage();
	}
	return -1;
}

int main(int argc, char *argv[]){
	int argi;
	if (argc<2){
		print_usage();
		return -1;
	}
	ortp_init();
	for(argi=1;argi<argc;++argi){
		if (strcmp(argv[argi],"init")==0){
			/*check if there is running instance*/
			if (send_generic_command("help",0)==0){
				fprintf(stderr,"A running linphonec has been found, not spawning a second one.\n");
				return 0;
			}
			spawn_linphonec(argc-argi-1,&argv[argi+1]);
			return 0;
		}else if (strcmp(argv[argi],"generic")==0){
			if (argi+1<argc){
				return send_generic_command(argv[argi+1],1);
			}else print_usage();
		}else if (strcmp(argv[argi],"register")==0){
			return register_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"unregister")==0){
			return unregister_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"dial")==0){
			return dial_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"hangup")==0){
			send_generic_command("terminate",FALSE);
			send_generic_command("duration",TRUE);
		}else if (strcmp(argv[argi],"status")==0){
			return status_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"soundcard")==0){
			return soundcard_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"exit")==0){
			return send_generic_command("quit",TRUE);
		}else print_usage();
	}
  	return 0;
}
