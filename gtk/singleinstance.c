/*
linphone, gtk interface.
Copyright (C) 2011 Belledonne Communications SARL
Author: Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphone.h"

static ms_thread_t pipe_thread;
static ortp_pipe_t server_pipe=(ortp_pipe_t)-1;
static gboolean server_pipe_running=TRUE;
static char *pipe_name=NULL;

gchar *make_name(const char *appname){
	const char *username=getenv("USER");
	if (username){
		return g_strdup_printf("%s-%s",appname,username);
	}
	return g_strdup(appname);
}

static gboolean execute_wakeup(char *buf){
	char uri[255]={0};
	int option;

	if (strlen(buf)>1) sscanf(buf,"%i%s",&option,uri);
	else sscanf(buf,"%i",&option);

	switch(option){
		case START_LINPHONE:
			linphone_gtk_show_main_window();
			break;
		case START_AUDIO_ASSISTANT:
			linphone_gtk_show_audio_assistant();
			break;
		case START_LINPHONE_WITH_CALL:
			linphone_gtk_refer_received(linphone_gtk_get_core(),uri);
			break;
	};

	g_free(buf);
	return FALSE;
}

static void * server_pipe_thread(void *pointer){
	ortp_pipe_t child;

	do{
		child=ortp_server_pipe_accept_client(server_pipe);
		if (server_pipe_running && child!=(ortp_pipe_t)-1){
			char buf[256]={0};
			if (ortp_pipe_read(child,(uint8_t*)buf,sizeof(buf))>0){
				g_message("Received wakeup command with arg %s",buf);
				gdk_threads_enter();
				g_timeout_add(20,(GSourceFunc)execute_wakeup,g_strdup(buf));
				gdk_threads_leave();
			}
			ortp_server_pipe_close_client(child);
		}
	}while(server_pipe_running);
	ortp_server_pipe_close(server_pipe);
	return NULL;
}

static void linphone_gtk_init_pipe(const char *name){
	server_pipe=ortp_server_pipe_create(name);
	if (server_pipe==(ortp_pipe_t)-1){
		g_warning("Fail to create server pipe for name %s: %s",name,strerror(errno));
		return;
	}
	server_pipe_running=TRUE;
	ms_thread_create(&pipe_thread,NULL,server_pipe_thread,NULL);
}

bool_t linphone_gtk_init_instance(const char *app_name, int option, const char *addr_to_call){
	ortp_pipe_t p;
	pipe_name=make_name(app_name);
	p=ortp_client_pipe_connect(pipe_name);
	if (p!=(ortp_pipe_t)-1){
		uint8_t buf[256]={0};
		g_message("There is already a running instance.");
		if (addr_to_call!=NULL){
			sprintf((char *)buf,"%i%s",option,addr_to_call);
		} else {
			sprintf((char *)buf,"%i",option);
		}
		if (ortp_pipe_write(p,buf,sizeof(buf))==-1){
			g_error("Fail to send wakeup command to running instance: %s",strerror(errno));
		}else{
			g_message("Message to running instance sent.");
		}
		ortp_client_pipe_close(p);
		return FALSE;
	}else{
		linphone_gtk_init_pipe(pipe_name);
	}
	return TRUE;
}

void linphone_gtk_uninit_instance(void){
	if (server_pipe!=(ortp_pipe_t)-1){
		ortp_pipe_t client;
		server_pipe_running=FALSE;
		/*this is to unblock the accept() of the server pipe*/
		client=ortp_client_pipe_connect(pipe_name);
		ortp_pipe_write(client,(uint8_t*)" ",1);
		ortp_client_pipe_close(client);
		ms_thread_join(pipe_thread,NULL);
		server_pipe=(ortp_pipe_t)-1;
		g_free(pipe_name);
		pipe_name=NULL;
	}
}
