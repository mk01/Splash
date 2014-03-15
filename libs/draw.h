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

#ifndef _DRAW_H_
#define _DRAW_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#define min(a, b) ((a<b) ? a : b)
#define max(a, b) ((a>b) ? a : b)
#define sqr(a) (a*a)

#define ALIGN_LEFT 		0
#define ALIGN_RIGHT 	1
#define ALIGN_CENTER 	2
#define NONE 			0
#define ITALIC 			1

unsigned int draw_color(unsigned short r, unsigned short g, unsigned short b);
void draw_rectangle_filled(int x, int y, int z, int width, int height, unsigned int color);
void draw_rectangle(int x, int y, int z, int width, int height, int border, unsigned int color);
void draw_line(int x2, int y2, int x1, int y1, int z, int thickness, unsigned int color);
void draw_circle_filled(int xc, int yc, int z, int r, unsigned int color);
void draw_circle(int xc, int yc, int z, int r, int border, unsigned int color);
short jpg_get_height(char *filename);
short jpg_get_width(char *filename);
short png_get_height(char *filename);
short png_get_width(char *filename);
short txt_get_width(char *font, char *txt, FT_UInt size, double spacing, int decoration, int align);
short txt_get_height(char *font, char *txt, FT_UInt size, double spacing, int decoration, int align);
void draw_jpg(int xc, int yc, int z, char *filename);
void draw_png(int xc, int yc, int z, char *filename);
void draw_txt(double xc, double yc, int z, char *font, FT_UInt size, char *text, unsigned int color, double spacing, int decoration, int align);
void draw_rmtxt(double xc, double yc, int z, char *font, FT_UInt size, char *text, unsigned int color, double spacing, int decoration, int align);

#endif
