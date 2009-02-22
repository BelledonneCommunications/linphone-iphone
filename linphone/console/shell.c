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

#define DEFAULT_TCP_PORT "32333"
#define DEFAULT_REPLY_SIZE 4096

#define STATUS_REGISTERED (1<<0)
#define STATUS_REGISTERING (1<<1)
#define STATUS_DIALING (1<<2)
#define STATUS_AUTOANSWER (1<<3)
#define STATUS_IN_CONNECTED (1<<4) /* incoming call accepted */
#define STATUS_OUT_CONNECTED (1<<5) /*outgoing call accepted */

#ifndef WIN32
static int tcp=0;
#else
static int tcp=1;
#endif

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

static int send_command(const char *command, const char * port, char *reply, int reply_len, int print_errors){
	ortp_socket_t sock;
	int i;
	int err;
	if (tcp){
		struct addrinfo *ai=NULL;
		struct addrinfo hints;
		memset(&hints,0,sizeof(hints));
		hints.ai_family=AF_INET;
		hints.ai_socktype=SOCK_STREAM;
		err=getaddrinfo("127.0.0.1",port,&hints,&ai);
		if (err!=0){
			if (print_errors) fprintf(stderr,"ERROR: getaddrinfo failed: error %i\n", err);
			return -1;
		}
		sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (connect(sock,ai->ai_addr,ai->ai_addrlen)!=0){
			if (print_errors) fprintf(stderr,"ERROR: Failed to connect socket.\n");
			freeaddrinfo(ai);
			return -1;
		}
		freeaddrinfo(ai);
	}else{
#ifndef WIN32
		struct sockaddr_un sa;
		char path[128];
		sock=socket(AF_UNIX,SOCK_STREAM,0);
		sa.sun_family=AF_UNIX;
		snprintf(path,sizeof(path)-1,"/tmp/linphonec-%i",getuid());
		strncpy(sa.sun_path,path,sizeof(sa.sun_path)-1);
		if (connect(sock,(struct sockaddr*)&sa,sizeof(sa))!=0){
			if (print_errors) fprintf(stderr,"ERROR: Failed to connect socket: %s\n",getSocketError());
			return -1;
		}
#else
		fprintf(stderr,"ERROR: windows pipes communication not yet implemented.\n");
		return -1;
#endif
	}
	if (send(sock,command,strlen(command),0)<0){
		if (print_errors) fprintf(stderr,"ERROR: Fail to send command to remote linphonec\n");
		close_socket(sock);
		return -1;
	}
	/*wait for replies */
	i=0;
	while ((err=recv(sock,&reply[i],reply_len-i-1,0))>0){
		i+=err;
	}
	reply[i]='\0';
	close_socket(sock);
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
	if (tcp){
		args[j++]="--tcp";
		args[j++]=DEFAULT_TCP_PORT;
	}else args[j++]="--pipe";
	args[j++]="-c";
	args[j++]="/dev/null";
	for(i=0;i<argc;++i){
		args[j++]=argv[i];
	}
	args[j++]=NULL;

	pid = fork();
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


	BOOL ret=CreateProcess(NULL,"linphonec.exe --tcp " DEFAULT_TCP_PORT " -c NUL",
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
	err=send_command(command,DEFAULT_TCP_PORT,reply,sizeof(reply),print_result);
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
		err=send_command(cmd,DEFAULT_TCP_PORT,reply,sizeof(reply),TRUE);
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

int main(int argc, char *argv[]){
	int argi;
	if (argc<2){
		print_usage();
		return -1;
	}
	ortp_init();
	for(argi=1;argi<argc;++argi){
		if (strcmp(argv[argi],"--tcp")==0){
			tcp=1;
		}else if (strcmp(argv[argi],"init")==0){
			/*check if there is running instance*/
			if (send_generic_command("help",0)==0){
				fprintf(stderr,"A running linphonec has been found, not spawning a second one.\n");
				return 0;
			}
			spawn_linphonec(argc-argi-1,&argv[argi+1]);
			if (tcp) fprintf(stderr,"WARNING: using --tcp is unsafe: unprivilegied users can make calls.\n");
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
		}else if (strcmp(argv[argi],"exit")==0){
			return send_generic_command("quit",TRUE);
		}else print_usage();
	}
  	return 0;
}
