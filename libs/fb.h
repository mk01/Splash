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

Various functions used in this file are derived from fbi (Gerd Hoffmann <gerd@kraxel.org>)
*/

#ifndef _FB_H_
#define _FB_H_

void fb_memset(void *addr, int c, size_t len);
void fb_setvt(void);
void fb_cleanup(void);
void fb_clear_mem(void);
void fb_clear_screen(void);
void fb_init(void);
int fb_gc(void);
unsigned long getLocation(int x, int y);
void fb_put_pixel(int x, int y, int z, unsigned int color);
void fb_rm_pixel(int x, int y, int z, unsigned int color);
unsigned long fb_width2px(int i);
unsigned long fb_height2px(int i);
unsigned short fb_width(void);
unsigned short fb_height(void);
unsigned short fb_bpp(void);
unsigned short fb_roffset(void);
unsigned short fb_goffset(void);
unsigned short fb_boffset(void);


#endif
