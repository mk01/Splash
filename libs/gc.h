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

#ifndef _GC_H_
#define _GC_H_

typedef struct {
	int nr;
	int (*listeners[10])(void);
} collectors_t;

collectors_t gc;

void gc_handler(int signal);
void gc_attach(int (*fp)(void));
void gc_detach(int (*fp)(void));
void gc_catch(void);
int gc_run(void);

#endif
