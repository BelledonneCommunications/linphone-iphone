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
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "ortp/ortp.h"

#define DEFAULT_TCP_PORT "32333"
#define DEFAULT_REPLY_SIZE 4096

static int send_command(const char *command, const char * port, char *reply, int reply_len, int print_errors){
	ortp_socket_t sock;
	struct addrinfo *ai=NULL;
	struct addrinfo hints;
	int err;
	int i;
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
			"\tinit     : spawn a linphonec daemon (first step to make other actions)\n"
			"\t\tfollowed by the arguments sent to linphonec\n"
			"\tgeneric  : sends a generic command to the running linphonec daemon\n"
			"\t\tfollowed by the generic command surrounded by quotes, for example \"call sip:joe@example.net\"\n"
			"\tregister : register with specified proxy\n");
	exit(-1);
}

#define MAX_ARGS 10

static void spawn_linphonec(int argc, char *argv[]){
	char * args[10];
	int i,j;
	pid_t pid;
	j=0;
	args[j++]="linphonec";
	args[j++]="--tcp";
	args[j++]=DEFAULT_TCP_PORT;
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

static int send_generic_command(const char *command, int print_result){
	char reply[DEFAULT_REPLY_SIZE];
	int err;
	err=send_command(command,DEFAULT_TCP_PORT,reply,sizeof(reply),print_result);
	if (err==0 && print_result) printf("%s",reply);
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
			}
			spawn_linphonec(argc-argi-1,&argv[argi+1]);
			return 0;
		}else if (strcmp(argv[argi],"generic")==0){
			if (argi+1<argc){
				return send_generic_command(argv[argi+1],1);
			}else print_usage();
		}else if (strcmp(argv[argi],"register")==0){
			return register_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"dial")==0){
			return dial_execute(argc-argi-1,&argv[argi+1]);
		}else if (strcmp(argv[argi],"hangup")==0){
			send_generic_command("terminate",FALSE);
			send_generic_command("duration",TRUE);
		}
	}
  	return 0;
}
