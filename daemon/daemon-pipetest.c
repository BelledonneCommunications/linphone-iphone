/*
daemon-pipetest.c
Copyright (C) 2016 Belledonne Communications, Grenoble, France 

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>

#ifndef _WIN32
#include <poll.h>
#endif



#include "ortp/ortp.h"

static int running=1;

int main(int argc, char *argv[]){
	char buf[32768];
	ortp_pipe_t fd;

	/* handle args */
	if (argc < 2) {
		ortp_error("Usage: %s pipename", argv[0]);
		return 1;
	}

	ortp_init();
	ortp_set_log_level_mask(NULL, ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR | ORTP_FATAL);

	fd=ortp_client_pipe_connect(argv[1]);
	if (fd==(ortp_pipe_t)-1){
		ortp_error("Could not connect to control pipe: %s",strerror(errno));
		return -1;
	}

#ifdef _WIN32
	DWORD fdwMode, fdwOldMode;
	HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(hin, &fdwOldMode);
	fdwMode = fdwOldMode ^ ENABLE_MOUSE_INPUT ^ ENABLE_WINDOW_INPUT;
	SetConsoleMode(hin, fdwMode);
	FlushConsoleInputBuffer(hin);
	while (running) {
		DWORD read = 0;
		DWORD written = 0;
		ReadFile(hin, buf, sizeof(buf), &read, NULL);
		if (read > 2) {
			buf[read - 2] = '\0'; // Remove ending '\r\n'
			if (ortp_pipe_write(fd, (uint8_t *)buf, strlen(buf)) < 0) {
				running = 0;
			} else {
				read = ortp_pipe_read(fd, (uint8_t *)buf, sizeof(buf));
				if (read > 0) {
					WriteFile(hout, buf, read, &written, NULL);
				} else {
					running = 0;
				}
			}
		}
	}
	SetConsoleMode(hin, fdwOldMode);
#else
	struct pollfd pfds[2] = { { 0 } };
	int bytes;
	pfds[0].fd=fd;
	pfds[0].events=POLLIN;
	pfds[1].fd=1;
	pfds[1].events=POLLIN;
	while (running){
		int err;
		err=poll(pfds,2,-1);
		if (err>0){
			/*splice to stdout*/
			if (pfds[0].revents & POLLIN){
				if ((bytes=read(pfds[0].fd,buf,sizeof(buf)))>0){
					if (write(0,buf,bytes)==-1){
						ortp_error("Fail to write to stdout?");
						break;
					}
					fprintf(stdout,"\n");		
				}else if (bytes==0){
					break;
				}
			}
			/*splice from stdin to pipe */
			if (pfds[1].revents & POLLIN){
				if ((bytes=read(pfds[1].fd,buf,sizeof(buf)))>0){
					if (write(pfds[0].fd,buf,bytes)==-1){
						ortp_error("Fail to write to unix socket");
						break;
					}		
				}else if (bytes==0){
					break;
				}
			}
		}
	}
#endif

	return 0;
}

