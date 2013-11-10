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

int main(int argc, char **argv) {

	log_file_disable();
	log_shell_enable();
	log_level_set(LOG_NOTICE);

	progname = malloc(12);
	strcpy(progname, "splash-send");

	int sockfd = 0;
	int black = 0;
	int percentage = -1;
	int infinite = 0;
	struct options_t *options = NULL;
	char *args = NULL;
    char *broadcast = NULL;

	char server[16] = "127.0.0.1";
	unsigned short port = PORT;

	/* Define all CLI arguments of this program */
	options_add(&options, 'H', "help", no_value, 0, NULL);
	options_add(&options, 'V', "version", no_value, 0, NULL);
	options_add(&options, 'S', "server", has_value, 0, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
	options_add(&options, 'P', "port", has_value, 0, "[0-9]{1,4}");
	options_add(&options, 'm', "message", has_value, 0, NULL);
	options_add(&options, 'p', "percentage", has_value, 0, "^([0-9]|[1-9][0-9]|100)$");
	options_add(&options, 'i', "infinite", no_value, 0, "^([0-9]|[1-9][0-9]|100)$");
	options_add(&options, 'b', "black", no_value, 0, NULL);

	/* Store all CLI arguments for later usage
	   and also check if the CLI arguments where
	   used correctly by the user. This will also
	   fill all necessary values in the options struct */
	while(1) {
		int c;
		c = options_parse(&options, argc, argv, 1, &args);
		if(c == -1)
			break;
		switch(c) {
			case 'H':
				printf("\t -H --help\t\t\tdisplay this message\n");
				printf("\t -V --version\t\t\tdisplay version\n");
				printf("\t -S --server=%s\t\tconnect to server address\n", server);
				printf("\t -P --port=%d\t\t\tconnect to server port\n", port);
				printf("\t -m --message=message\t\tprogress message\n");
				printf("\t -p --percentage=percentage\tprogress percentage\n");
				printf("\t -i --infinite\t\t\tactivate the infinite bar\n");
				printf("\t -b --black\t\t\tblacken out the splash\n");
				exit(EXIT_SUCCESS);
			break;
			case 'V':
				printf("%s %s\n", progname, "1.0");
				exit(EXIT_SUCCESS);
			break;
			case 'S':
				strcpy(server, args);
			break;
			case 'm':
				broadcast = malloc(strlen(args)+1);
				strcpy(broadcast, args);
			break;
			case 'b':
					black = 1;
			break;
			case 'i':
					infinite = 1;
			break;
			case 'p':
					percentage = atoi(args);
			break;
			case 'P':
				port = (unsigned short)atoi(args);
			break;
			default:
				printf("Usage: %s -m\n", progname);
				exit(EXIT_SUCCESS);
			break;
		}
	}


	if((sockfd = socket_connect(server, port)) == -1) {
		logprintf(LOG_ERR, "could not connect to splash-daemon");
		goto close;
	}

	JsonNode *json = json_mkobject();
	if(black) {
		json_append_member(json, "black", json_mkstring("1"));
	} else {
		if(percentage != -1) {
			char perc[3];
			sprintf(perc, "%d", percentage);
			json_append_member(json, "percentage", json_mkstring(perc));
		} else if(infinite) {
			json_append_member(json, "infinite", json_mkstring("1"));
		}
		if(broadcast) {
			json_append_member(json, "message", json_mkstring(broadcast));
		}
	}
	char *output = json_stringify(json, NULL);
	socket_write(sockfd, output);
	free(output);
	json_delete(json);

close:
	free(progname);
	socket_close(sockfd);

return EXIT_SUCCESS;
}
