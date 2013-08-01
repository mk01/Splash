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
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fcache.h"
#include "log.h"

int fcache_nr_files = 0;
unsigned char *fcache_bytes[255];

int fcache_add(char *filename) {
	unsigned long filesize, i = 0;
	struct stat sb;
	int rc;
	struct fcache_t *node;

	logprintf(LOG_NOTICE, "caching %s", filename);

	if((rc = stat(filename, &sb)) != 0) {
		logprintf(LOG_ERR, "failed to stat %s", filename);
		return -1;
	} else {
		filesize = (unsigned long)sb.st_size;
		fcache_bytes[fcache_nr_files] = (unsigned char*)malloc(filesize + 100);
		int fd = open(filename, O_RDONLY);

		i = 0;
		while (i < filesize) {
			rc = read(fd, fcache_bytes[fcache_nr_files]+i, filesize-i);
			i += (unsigned long)rc;
		}
		close(fd);
		
		node = malloc(sizeof(struct fcache_t));
		node->id=fcache_nr_files;
		node->size=filesize;
		strcpy(node->name, filename);
		node->next = fcache;
		fcache = node;

		fcache_nr_files++;		
		return fcache_nr_files-1;	
	}
	return -1;
}

short fcache_get_id(char *filename, int *out) {
	struct fcache_t *ftmp = fcache;
	while(ftmp != NULL) {
		if(strcmp(ftmp->name, filename) == 0) {
			*out = ftmp->id;
			return 0;
		}
		ftmp = ftmp->next;
	}
	return -1;
}

short fcache_get_size(char *filename, unsigned long *out) {
	struct fcache_t *ftmp = fcache;
	while(ftmp != NULL) {
		if(strcmp(ftmp->name, filename) == 0) {
			*out = ftmp->size;
			return 0;
		}
		ftmp = ftmp->next;
	}
	return -1;
}

unsigned char *fcache_get_bytes(int id) {
	return fcache_bytes[id];
}