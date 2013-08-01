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

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include <jpeglib.h>

#include "fb.h"
#include "log.h"
#include "draw.h"
#include "fcache.h"

unsigned short draw_color_16bit(unsigned short r, unsigned short g, unsigned short b) {
	return (unsigned short)((r / 8) << 11) | (unsigned short)((g / 4) << 5) | (b / 8);
}

void draw_rectangle_filled(int x, int y, int width, int height, unsigned short color) {
	int a=0, b=0;
	
	for(a = x; a < x+width; a++) {
		for(b = y; b < y+height; b++) {
			fb_put_pixel(a, b, color);
		} 
	}
} 

void draw_rectangle(int x, int y, int width, int height, int border, unsigned short color) {
	int a=0, b=0;
	
	for(a = x; a < x+width; a++) {
		for(b = y; b < y+height; b++) {
			if((a<(x+border)) || (a>((x+width)-border)) || (b<(y+border)) || (b>((y+height)-border))) {
				fb_put_pixel(a, b, color);
			}
		}
	}
}

void draw_line(int x2, int y2, int x1, int y1, unsigned short color) {
	const int dx = abs(x1-x2);
	const int dy = abs(y1-y2);
	int const1, const2, p, x, y, step;

	if(dx >= dy) {
		const1 = 2 * dy;
		const2 = 2 * (dy - dx);
		p = 2 * dy - dx;
		x = min(x1,x2);
		y = (x1 < x2) ? (y1) : (y2);
		step = (y1 > y2) ? (1) : (-1);
		fb_put_pixel(x, y, color);
		while (x < max(x1,x2)) {
			if(p < 0) {
				p += const1;
			} else {
				y += step;
				p += const2;
			}
        fb_put_pixel(++x, y, color);
		}
	} else {
		const1 = 2 * dx;
		const2 = 2 * (dx - dy);
		p = 2 * dx - dy;
		y = min(y1,y2);
		x = (y1 < y2) ? (x1) : (x2);
		step = (x1 > x2) ? (1) : (-1);
		fb_put_pixel(x, y, color);
		while(y < max(y1,y2)) {
			if(p < 0)
				p += const1;
			else {
				x += step;
				p += const2;
			}
		fb_put_pixel(x, ++y, color);
		}
	}
}


void draw_circle_filled(int xc, int yc, int r, unsigned short color) {
	int x = 0, y = 0, p = 0, z = 0;
	xc+=r;
	yc+=r;
	
	x = 0, y = 0;
	y = (r-z);
	p = 3 - 2 * (r-z);
	while (x <= y) {
		draw_line(xc - x, yc - y, xc + x, yc + y, color);
		draw_line(xc + x, yc - y, xc - x, yc + y, color);
		draw_line(xc - y, yc - x, xc + y, yc + x, color);
		draw_line(xc - y, yc + x, xc + y, yc - x, color);
		if (p < 0)
			p += 4 * x++ + 6;
		else
			p += 4 * (x++ - y--) + 10;
	}
}

void draw_circle(int xc, int yc, int r, int border, unsigned short color) {
	int x = 0, y = 0, p = 0, z = 0;
	xc+=r;
	yc+=r;
	for(z = 0; z < border; z++) {
		x = 0, y = 0;
		y = (r-z);
		p = 3 - 2 * (r-z);
		while (x <= y) {
			fb_put_pixel(xc + x, yc + y, color);
			fb_put_pixel(xc - x, yc + y, color);
			fb_put_pixel(xc + x, yc - y, color);
			fb_put_pixel(xc - x, yc - y, color);
			fb_put_pixel(xc + y, yc + x, color);
			fb_put_pixel(xc - y, yc + x, color);
			fb_put_pixel(xc + y, yc - x, color);
			fb_put_pixel(xc - y, yc - x, color);
			if (p < 0)
				p += 4 * x++ + 6;
			else
				p += 4 * (x++ - y--) + 10;
		}
	}
}

void draw_jpg(int xc, int yc, char *filename) {
	unsigned char *raw_image = NULL;
	int id;
	unsigned long size;
	unsigned long location = 0;
	int i = 0, x = 0;
	unsigned char r, g, b;

	struct jpeg_decompress_struct jpegInfo;
	struct jpeg_error_mgr jpegErr;
	int line;
	
	if(fcache_get_id(filename, &id) == -1) {
		if((id=fcache_add(filename)) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(id > -1) {
		fcache_get_size(filename, &size);
			
		JSAMPROW row_pointer[1];
		jpegInfo.err = jpeg_std_error(&jpegErr);
		jpeg_create_decompress(&jpegInfo);
		jpeg_mem_src(&jpegInfo, fcache_get_bytes(id), size);
		jpeg_read_header(&jpegInfo, TRUE);
		
		jpeg_start_decompress(&jpegInfo);

		raw_image = (unsigned char*)malloc((JDIMENSION)jpegInfo.output_width*(JDIMENSION)jpegInfo.output_height*(size_t)jpegInfo.num_components);
		row_pointer[0] = (unsigned char *)malloc((JDIMENSION)jpegInfo.output_width*(size_t)jpegInfo.num_components);
		

		while(jpegInfo.output_scanline < jpegInfo.image_height) {
			jpeg_read_scanlines(&jpegInfo, row_pointer, 1);
			
			for(i=0; i<(JDIMENSION)jpegInfo.image_width*(size_t)jpegInfo.num_components; i++) {
				raw_image[location++] = row_pointer[0][i];
			}
		}

		for(line = (int)jpegInfo.image_height-1; line >= 0; line--) {
			for(x=0; x < jpegInfo.image_width; x++) {
				b = *(raw_image+(x+line*(int)jpegInfo.image_width)*jpegInfo.num_components+2);
				g = *(raw_image+(x+line*(int)jpegInfo.image_width)*jpegInfo.num_components+1);
				r = *(raw_image+(x+line*(int)jpegInfo.image_width)*jpegInfo.num_components+0);

				fb_put_pixel(x+xc, line+yc, draw_color_16bit((((int)r & 0xFF) << 0), (((int)g & 0xFF) << 0), (((int)b & 0xFF) << 0)));
			}
		}

		jpeg_finish_decompress(&jpegInfo);
		jpeg_destroy_decompress(&jpegInfo);
		free(row_pointer[0]);
		free(raw_image);
	}
}

void draw_txt(double xc, double yc, char *font, FT_UInt size, char *text, unsigned short int color, double spacing, int decoration, int align) {
	int id = -1, y = 0, x = 0, n = 0;
	double z = 0;
	int fontHeight=0, fontWidth=0;
	unsigned long fsize; 
	
	if(fcache_get_id(font, &id) == -1) {
		if((id=fcache_add(font)) == -1) {
			logprintf(LOG_ERR, "could not cache %s", font);
		}
	}

	if(id > -1) {
		fcache_get_size(font, &fsize);
	
		FT_Library library;
		FT_Face face;
		FT_Glyph_Metrics *metrics;

		FT_Init_FreeType(&library);
		FT_New_Memory_Face(library, fcache_get_bytes(id), (FT_Long)fsize, 0, &face);
		FT_Set_Char_Size(face, (FT_F26Dot6)size*64, 0, 72, 0);
		FT_Set_Pixel_Sizes(face, 0, size);
		FT_UInt glyph_index;
		
		for(n=0;n<strlen(text);n++) {
			glyph_index = FT_Get_Char_Index(face, text[n]);
			FT_Load_Glyph(face, glyph_index, 0);
			metrics = &face->glyph->metrics;
			FT_Render_Glyph(face->glyph, ft_render_mode_normal);
			if((metrics->horiBearingY/64) > fontHeight) {
				fontHeight=(metrics->horiBearingY/64);
			}
			if(face->glyph->bitmap.width > fontWidth) {
				fontWidth=face->glyph->bitmap.width;
			}
		}
		yc+=fontHeight;
		if(align == ALIGN_LEFT) {
			for(n=((int)strlen(text)-1);n>=0;n--) {
				if(text[n]==' ') {
					xc -= fontWidth;
				} else {		
					glyph_index = FT_Get_Char_Index(face, text[n]);
					FT_Load_Glyph(face, glyph_index, 0);
					metrics = &face->glyph->metrics;	
					FT_Render_Glyph(face->glyph, ft_render_mode_normal);
					if(decoration==1) {
						z=face->glyph->bitmap.rows*0.25;
					}
					xc -= face->glyph->bitmap.width+spacing;
					for(y=0;y<face->glyph->bitmap.rows;y++) {
						for (x=0;x<face->glyph->bitmap.width;x++) {
							if(face->glyph->bitmap.buffer[y*face->glyph->bitmap.width + x] > 0) {
								fb_put_pixel((int)(x+xc+z), (int)(y+yc-(metrics->horiBearingY/64)),color);	
							}
						}
						if(decoration==1) {			
							z-=0.25;
						}
					}
				}
			}
		} else {
			for(n=0;n<=strlen(text);n++) {
				if(text[n]==' ') {
					xc += fontWidth;
				} else {
					glyph_index = FT_Get_Char_Index(face, text[n]);
					FT_Load_Glyph(face, glyph_index, 0);
					metrics = &face->glyph->metrics;	
					FT_Render_Glyph(face->glyph, ft_render_mode_normal);
					if(decoration==1) {
						z=face->glyph->bitmap.rows*0.25;
					}
					for(y=0;y<face->glyph->bitmap.rows;y++) {
						for (x=0;x<face->glyph->bitmap.width;x++) {
							if(face->glyph->bitmap.buffer[y*face->glyph->bitmap.width + x] > 0) {
								fb_put_pixel((int)(x+xc+z), (int)(y+yc-(metrics->horiBearingY/64)), color);	
							}
						}
						if(decoration==ITALIC) {			
							z-=0.25;
						}
					}
					xc -= face->glyph->bitmap.width-z+spacing;
				}
			}
		}
	FT_Done_Face(face);
	FT_Done_FreeType(library);	
	}
}