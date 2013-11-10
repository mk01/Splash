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

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <time.h>

#include "common.h"
#include "log.h"

void logmarkup(void) {
	char fmt[64], buf[64];
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	if((tm = localtime(&tv.tv_sec)) != NULL) {
		strftime(fmt, sizeof(fmt), "%b %d %H:%M:%S", tm);
		snprintf(buf, sizeof(buf), "%s:%03u", fmt, (unsigned int)tv.tv_usec);
	}
	
	sprintf(debug_log, "[%22.22s] %s: ", buf, progname);
}

#ifdef DEBUG

const char *debug_filename(const char *file) {
	return strrchr(file, '/') ? strrchr(file, '/') + 1 : file;
}

void *debug_malloc(size_t len, const char *file, int line) {
	void *(*libc_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
	if(shelllog == 1 && loglevel == LOG_DEBUG) {
		logmarkup();
		printf("%s", debug_log);
		printf("DEBUG: malloc from %s #%d\n", debug_filename(file), line);
	}
	return libc_malloc(len);
}

void *debug_realloc(void *addr, size_t len, const char *file, int line) {
	void *(*libc_realloc)(void *, size_t) = dlsym(RTLD_NEXT, "realloc");
	if(addr == NULL) {
	if(shelllog == 1 && loglevel == LOG_DEBUG) {
			logmarkup();
			printf("%s", debug_log);			
			printf("DEBUG: realloc from %s #%d\n", debug_filename(file), line);
		}
		return debug_malloc(len, file, line);
	} else {
		return libc_realloc(addr, len);
	}
}

void debug_free(void **addr, const char *file, int line) {
	void ** __p = addr;
	if(*(__p) != NULL) {
	if(shelllog == 1 && loglevel == LOG_DEBUG) {
			logmarkup();
			printf("%s", debug_log);			
			printf("DEBUG: free from %s #%d\n", debug_filename(file), line);
		}
		free(*(__p));
		*(__p) = NULL;
	}
}

#else

void sfree(void **addr) {
	void ** __p = addr;
	if(*(__p) != NULL) {
		free(*(__p));
		*(__p) = NULL;
	}
}

#endif