/*
Copyright 2013 CurlyMo <curlymoo1@gmail.com>

This file is part of Splash - Linux GUI Framebuffer Splash.

Splash is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Splash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along
with Splash. If not, see <http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ctype.h>

#include "config.h"
#include "gc.h"
#include "log.h"
#include "options.h"
#include "socket.h"
#include "json.h"
#include "fb.h"
#include "draw.h"

/* The pid_file and pid of this daemon */
char *pid_file;
pid_t pid;
/* Daemonize or not */
int nodaemon = 0;
/* Are we already running */
int running = 1;
/* Previous message to clean up */
char *prevMessage = NULL;

/* Parse the incoming buffer from the client */
void socket_parse_data(int i, char buffer[BUFFER_SIZE]) {
	int sd = socket_clients[i];
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char *message;
	JsonNode *json = json_mkobject();

	getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	json = json_decode(buffer);
	if(json_find_string(json, "message", &message) == 0) {
		if(prevMessage != NULL) {
			draw_txt(100, 100, strdup("arial.ttf"), 25, prevMessage, draw_color_16bit(0, 0, 0), 5, NONE, ALIGN_LEFT);
		}
		draw_txt(100, 100, strdup("arial.ttf"), 25, message, draw_color_16bit(255, 0, 0), 5, NONE, ALIGN_LEFT);
		prevMessage = strdup(message);
	}
}

void deamonize(void) {
	int f;
	char buffer[BUFFER_SIZE];

	enable_file_log();
	disable_shell_log();
	//Get the pid of the fork
	pid_t npid = fork();
	switch(npid) {
		case 0:
		break;
		case -1:
			logprintf(LOG_ERR, "could not daemonize program");
			exit(1);
		break;
		default:
			if((f = open(pid_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) != -1) {
				lseek(f, 0, SEEK_SET);
				sprintf(buffer, "%d", npid);
				if(write(f, buffer, strlen(buffer)) != -1) {
					logprintf(LOG_ERR, "could not store pid in %s", pid_file);
				}
			}
			close(f);
			logprintf(LOG_INFO, "started daemon with pid %d", npid);
			exit(0);
		break;
	}
}

/* Garbage collector of main program */
int main_gc(void) {

	if(running == 0) {
		/* Remove the stale pid file */
		if(access(pid_file, F_OK) != -1) {
			if(remove(pid_file) != -1) {
				logprintf(LOG_DEBUG, "removed stale pid_file %s", pid_file);
			} else {
				logprintf(LOG_ERR, "could not remove stale pid file %s", pid_file);
			}
		}
	}
	return 0;
}

int main(int argc , char **argv) {
	/* Run main garbage collector when quiting the daemon */
	gc_attach(main_gc);

	/* Catch all exit signals for gc */
	gc_catch();

	disable_file_log();
	enable_shell_log();

	progname = malloc((10*sizeof(char))+1);
	progname = strdup("splash-daemon");

	struct socket_callback_t socket_callback;
	char buffer[BUFFER_SIZE];
	int f;

	addOption(&options, 'H', "help", no_value, 0, NULL);
	addOption(&options, 'V', "version", no_value, 0, NULL);
	addOption(&options, 'D', "nodaemon", no_value, 0, NULL);

	while (1) {
		int c;
		c = getOptions(&options, argc, argv, 1);
		if (c == -1)
			break;
		switch(c) {
			case 'H':
				printf("Usage: %s [options]\n", progname);
				printf("\t -H --help\t\tdisplay usage summary\n");
				printf("\t -V --version\t\tdisplay version\n");
				printf("\t -S --settings\t\tsettings file\n");
				printf("\t -D --nodaemon\t\tdo not daemonize and\n");
				printf("\t\t\t\tshow debug information\n");
				return (EXIT_SUCCESS);
			break;
			case 'V':
				printf("%s %s\n", progname, "1.0");
				return (EXIT_SUCCESS);
			break;
			case 'D':
				nodaemon=1;
			break;
			default:
				printf("Usage: %s [options]\n", progname);
				return (EXIT_FAILURE);
			break;
		}
	}
	
	pid_file = strdup(PID_FILE);

	if((f = open(pid_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) != -1) {
		if(read(f, buffer, BUFFER_SIZE) != -1) {
			//If the file is empty, create a new process
			if(!atoi(buffer)) {
				running = 0;
			} else {
				//Check if the process is running
				kill(atoi(buffer), 0);
				//If not, create a new process
				if(errno == ESRCH) {
					running = 0;
				}
			}
		}
	} else {
		logprintf(LOG_ERR, "could not open / create pid_file %s", pid_file);
		return EXIT_FAILURE;
	}
	close(f);	
	
	if(nodaemon == 1 || running == 1) {
		set_loglevel(LOG_DEBUG);
	}
	
	if(running == 1) {
		nodaemon=1;
		logprintf(LOG_NOTICE, "already active (pid %d)", atoi(buffer));
		return EXIT_FAILURE;
	}
		
	socket_start(PORT);
	if(nodaemon == 0) {
		deamonize();
	}
	
	fb_init();
	
    //initialise all socket_clients and handshakes to 0 so not checked
	memset(socket_clients, 0, sizeof(socket_clients));

    socket_callback.client_disconnected_callback = NULL;
    socket_callback.client_connected_callback = NULL;
    socket_callback.client_data_callback = &socket_parse_data;

	/* Make sure the server part is non-blocking by creating a new thread */
	wait_for_data((void *)&socket_callback);

	return EXIT_FAILURE;
}
