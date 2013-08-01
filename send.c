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
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#include "config.h"
#include "log.h"
#include "options.h"
#include "socket.h"
#include "json.h"

typedef enum {
	WELCOME,
	SEND
} steps_t;

int main(int argc, char **argv) {

	disable_file_log();
	enable_shell_log();
	set_loglevel(LOG_NOTICE);

	progname = malloc((11*sizeof(char))+1);
	progname = strdup("splash-send");

	options = malloc(255*sizeof(struct options_t));

	int sockfd = 0;
    char *recvBuff = NULL;
    char *message = NULL;
    char *broadcast = strdup("\0");
	steps_t steps = WELCOME;

	char server[16] = "127.0.0.1";
	unsigned short port = PORT;

	JsonNode *json = json_mkobject();

	/* Define all CLI arguments of this program */
	addOption(&options, 'H', "help", no_value, 0, NULL);
	addOption(&options, 'V', "version", no_value, 0, NULL);
	addOption(&options, 'S', "server", has_value, 0, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
	addOption(&options, 'P', "port", has_value, 0, "[0-9]{1,4}");
	addOption(&options, 'm', "message", has_value, 0, NULL);

	/* Store all CLI arguments for later usage
	   and also check if the CLI arguments where
	   used correctly by the user. This will also
	   fill all necessary values in the options struct */
	while(1) {
		int c;
		c = getOptions(&options, argc, argv, 1);
		if(c == -1)
			break;
		switch(c) {
			case 'H':
				printf("\t -H --help\t\t\tdisplay this message\n");
				printf("\t -V --version\t\t\tdisplay version\n");
				printf("\t -S --server=%s\t\tconnect to server address\n", server);
				printf("\t -P --port=%d\t\t\tconnect to server port\n", port);
				printf("\t -m --message=%d\t\t\tmessage to send\n", port);
				exit(EXIT_SUCCESS);
			break;
			case 'V':
				printf("%s %s\n", progname, "1.0");
				exit(EXIT_SUCCESS);
			break;
			case 'S':
				strcpy(server, optarg);
			break;
			case 'm':
				broadcast = strdup(optarg);
			break;		
			case 'P':
				port = (unsigned short)atoi(optarg);
			break;			
			default:
				printf("Usage: %s -l location -d device\n", progname);
				exit(EXIT_SUCCESS);
			break;
		}
	}


	if((sockfd = socket_connect(strdup(server), port)) == -1) {
		logprintf(LOG_ERR, "could not connect to splash-daemon");
		goto close;
	}

	while(1) {
		/* Clear the receive buffer again and read the welcome message */
		if((recvBuff = socket_read(sockfd)) != NULL) {
			json = json_decode(recvBuff);
			json_find_string(json, "message", &message);
		} else {
			goto close;
		}

		switch(steps) {
			case WELCOME:
				if(strcmp(message, "accept connection") == 0) {
					steps=SEND;
				}
			case SEND:
				json_delete(json);
				json = json_mkobject();
				json_append_member(json, "message", json_mkstring(broadcast));
				socket_write(sockfd, json_stringify(json, NULL));
				goto close;
			break;
			default:
				goto close;
			break;
		}
	}
close:
	json_delete(json);
	socket_close(sockfd);
return EXIT_SUCCESS;
}
