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

#ifndef _LOG_H_
#define _LOG_H_

#include <syslog.h>
#include "config.h"

void logprintf(int prio, const char *format_str, ...);
void logperror(int prio, const char *s);
void enable_file_log(void);
void disable_file_log(void);
void enable_shell_log(void);
void disable_shell_log(void);
void set_logfile(char *file);
void set_loglevel(int level);
int get_loglevel(void);

#endif
