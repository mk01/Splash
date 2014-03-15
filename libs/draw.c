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
#include <math.h>

#include <jpeglib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "common.h"
#include "fb.h"
#include "log.h"
#include "draw.h"
#include "fcache.h"
#include "lodepng.h"


/*
struct fb_bitfield {
        __u32 offset;                   // beginning of bitfield        
        __u32 length;                   // length of bitfield           
        __u32 msb_right;                // != 0 : Most significant bit is 
                                        // right 
};

struct fb_var_screeninfo {
        __u32 xres;                     // visible resolution           
        __u32 yres;
        __u32 xres_virtual;             // virtual resolution           
        __u32 yres_virtual;
        __u32 xoffset;                  // offset from virtual to visible 
        __u32 yoffset;                  // resolution                   

        __u32 bits_per_pixel;           // guess what                   
        __u32 grayscale;                // 0 = color, 1 = grayscale,    
                                        // >1 = FOURCC                  
        struct fb_bitfield red;         // bitfield in fb mem if true color, 
        struct fb_bitfield green;       // else only length is significant 
        struct fb_bitfield blue;
        struct fb_bitfield transp;      // transparency                 

*/

unsigned int draw_color(unsigned short r, unsigned short g, unsigned short b) {
	switch(fb_bpp())
        {
            default:
            case 32:
                    return (unsigned int)(r << fb_roffset() | g << fb_goffset() | b << fb_boffset());
                    break;
            case 16:
                    return (unsigned short)((r / 8) << 11) | (unsigned short)((g / 4) << 5) | (b / 8);
        }
}

void draw_rectangle_filled(int x, int y, int z, int width, int height, unsigned int color) {
	int a=0, b=0;
	
	for(a = x; a < x+width; a++) {
		for(b = y; b < y+height; b++) {
			fb_put_pixel(a, b, z, color);
		} 
	}
} 

void draw_rectangle(int x, int y, int z, int width, int height, int border, unsigned int color) {
	int a=0, b=0;
	
	for(a = x; a < x+width; a++) {
		for(b = y; b < y+height; b++) {
			if((a<(x+border)) || (a>((x+width)-border)) || (b<(y+border)) || (b>((y+height)-border))) {
				fb_put_pixel(a, b, z, color);
			}
		}
	}
}

void draw_line(int x2, int y2, int x1, int y1, int z, int thickness, unsigned int color) {
	const int dx = abs(x1-x2);
	const int dy = abs(y1-y2);
	int const1, const2, p, x, y, step, a;

	if(dx >= dy) {
		for(a=0;a<thickness;a++) {
			const1 = 2 * dy;
			const2 = 2 * (dy - dx);
			p = 2 * dy - dx;
			x = min(x1, x2);
			y = (x1 < x2) ? (y1) : (y2);
			step = (y1 > y2) ? (1) : (-1);
			fb_put_pixel(x, y, z, color);
			while (x < max(x1, x2)) {
				if(p < 0) {
					p += const1;
				} else {
					y += step;
					p += const2;
				}
			fb_put_pixel(++x, y+a, z, color);
			}
		}
	} else {
		for(a=0;a<thickness;a++) {
			const1 = 2 * dx;
			const2 = 2 * (dx - dy);
			p = 2 * dx - dy;
			y = min(y1,y2);
			x = (y1 < y2) ? (x1) : (x2);
			step = (x1 > x2) ? (1) : (-1);
			fb_put_pixel(x, y, 0, color);
			while(y < max(y1,y2)) {
				if(p < 0)
					p += const1;
				else {
					x += step;
					p += const2;
				}
			fb_put_pixel(x+a, ++y, z, color);
			}
		}
	}
}


void draw_circle_filled(int xc, int yc, int z, int r, unsigned int color) {
	int y, x;
	xc+=r;
	yc+=r;
	for(y=r*-1; y<=r; y++) {
		for(x=r*-1; x<=r; x++) {
			if((x*x)+(y*y) <= r*r) {
				fb_put_pixel(xc+x, yc+y, z, color);
			}
		}
	}
}

void draw_circle(int xc, int yc, int z, int r, int border, unsigned int color) {
	int y, x;
	xc+=r;
	yc+=r;
	for(y=r*-1; y<=r; y++) {
		for(x=r*-1; x<=r; x++) {
			if(((x*x)+(y*y) >= ((r-border)*(r-border))) && ((x*x)+(y*y) <= (r*r))) {
				fb_put_pixel(xc+x, yc+y, z, color);
			}
		}
	}
}

short jpg_get_height(char *filename) {
	int size;

	struct jpeg_decompress_struct jpegInfo;
	struct jpeg_error_mgr jpegErr;
	
	if(fcache_get_size(filename, &size) == -1) {
		if(fcache_add(filename) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(fcache_get_size(filename, &size) > -1) {
		jpegInfo.err = jpeg_std_error(&jpegErr);
		jpeg_create_decompress(&jpegInfo);
		jpeg_mem_src(&jpegInfo, fcache_get_bytes(filename), (long unsigned int)size);
		jpeg_read_header(&jpegInfo, TRUE);
		
		jpeg_start_decompress(&jpegInfo);
		return (short)jpegInfo.output_height;
	}
	return -1;
}

short jpg_get_width(char *filename) {
	int size;

	struct jpeg_decompress_struct jpegInfo;
	struct jpeg_error_mgr jpegErr;
	
	if(fcache_get_size(filename, &size) == -1) {
		if(fcache_add(filename) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(fcache_get_size(filename, &size) > -1) {
		jpegInfo.err = jpeg_std_error(&jpegErr);
		jpeg_create_decompress(&jpegInfo);
		jpeg_mem_src(&jpegInfo, fcache_get_bytes(filename), (long unsigned int)size);
		jpeg_read_header(&jpegInfo, TRUE);
		
		jpeg_start_decompress(&jpegInfo);
		return (short)jpegInfo.output_width;
	}
	return -1;
}

void draw_jpg(int xc, int yc, int z, char *filename) {
	unsigned char *raw_image = NULL;
	int size;
	unsigned long location = 0;
	int i = 0, x = 0;
	unsigned char r, g, b;

	struct jpeg_decompress_struct jpegInfo;
	struct jpeg_error_mgr jpegErr;
	int line;
	
	if(fcache_get_size(filename, &size) == -1) {
		if(fcache_add(filename) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(fcache_get_size(filename, &size) > -1) {
		
		JSAMPROW row_pointer[1];
		jpegInfo.err = jpeg_std_error(&jpegErr);
		jpeg_create_decompress(&jpegInfo);
		jpeg_mem_src(&jpegInfo, fcache_get_bytes(filename), (long unsigned int)size);
		jpeg_read_header(&jpegInfo, TRUE);
		
		jpeg_start_decompress(&jpegInfo);

		raw_image = (unsigned char*)malloc((JDIMENSION)jpegInfo.output_width*(JDIMENSION)jpegInfo.output_height*(size_t)jpegInfo.num_components);
		row_pointer[0] = (unsigned char *)malloc((JDIMENSION)jpegInfo.output_width*(size_t)jpegInfo.num_components);
		
		if((yc+(int)jpegInfo.image_height) <= fb_height() && (xc+(int)jpegInfo.image_width) <= fb_width()) {
		
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

					fb_put_pixel(x+xc, line+yc, z, draw_color(r, g, b));
				}
			}

			jpeg_finish_decompress(&jpegInfo);
			jpeg_destroy_decompress(&jpegInfo);
			free(row_pointer[0]);
			free(raw_image);
		} else {
			logprintf(LOG_ERR, "image %s to large for its supposed placing", filename);
		}
	}
}

short png_get_width(char *filename) {
	int width, height;
	unsigned char *image;
	int size;

	if(fcache_get_size(filename, &size) == -1) {
		if(fcache_add(filename) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(fcache_get_size(filename, &size) > -1) {	
		lodepng_decode32(&image, (unsigned int *)&width, (unsigned int *)&height, fcache_get_bytes(filename), (size_t)size);
		sfree((void *)&image);
		return (short)width;
	}
	return -1;
}

short png_get_height(char *filename) {
	int width, height;
	unsigned char *image;
	int size;

	if(fcache_get_size(filename, &size) == -1) {
		if(fcache_add(filename) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(fcache_get_size(filename, &size) > -1) {	
		lodepng_decode32(&image, (unsigned int *)&width, (unsigned int *)&height, fcache_get_bytes(filename), (size_t)size);
		sfree((void *)&image);
		return (short)height;
	}
	return -1;
}

void draw_png(int xc, int yc, int z, char *filename) {
	int width, height;
	unsigned char *image;
	int size, x, y;

	if(fcache_get_size(filename, &size) == -1) {
		if(fcache_add(filename) == -1) {
			logprintf(LOG_ERR, "could not cache %s", filename);
		}
	}
	
	if(fcache_get_size(filename, &size) > -1) {	
		lodepng_decode32(&image, (unsigned int *)&width, (unsigned int *)&height, fcache_get_bytes(filename), (size_t)size);
	  
		for(y = 0;y<height; y++) {
			for(x = 0;x<width; x++) {
				int checkerColor;
				int r, g, b, a ;

				r = image[4 * y * width + 4 * x + 0];
				g = image[4 * y * width + 4 * x + 1];
				b = image[4 * y * width + 4 * x + 2];
				a = image[4 * y * width + 4 * x + 3];
				
				checkerColor = 191 + 64 * (((x / 16) % 2) == ((y / 16) % 2));
				r = (a * r + (255 - a) * checkerColor) / 255;
				g = (a * g + (255 - a) * checkerColor) / 255;
				b = (a * b + (255 - a) * checkerColor) / 255;
				if(a != 0) {
					fb_put_pixel(x+xc, y+yc, z, draw_color((unsigned short)r, (unsigned short)g, (unsigned short)b));
				}
			}
		}
		sfree((void *)&image);
	}
}

short txt_get_width(char *font, char *text, FT_UInt size, double spacing, int decoration, int align) {
	int n = 0;
	int fontWidth = 0;
	int totalWidth = 0;
	int fsize; 

	if(fcache_get_size(font, &fsize) != 0) {
		if(fcache_add(font) != 0) {
			logprintf(LOG_ERR, "could not cache %s", font);
		}
	}

	if(fcache_get_size(font, &fsize) > -1) {
	
		FT_Library library;
		FT_Face face;

		FT_Init_FreeType(&library);
		FT_New_Memory_Face(library, fcache_get_bytes(font), (FT_Long)fsize, 0, &face);
		FT_Set_Char_Size(face, (FT_F26Dot6)size*64, 0, 72, 0);
		FT_Set_Pixel_Sizes(face, 0, size);
		FT_UInt glyph_index;
		
		for(n=0;n<strlen(text);n++) {
			glyph_index = FT_Get_Char_Index(face, text[n]);
			FT_Load_Glyph(face, glyph_index, 0);
			FT_Render_Glyph(face->glyph, ft_render_mode_normal);
			if(face->glyph->bitmap.width > fontWidth) {
				fontWidth = face->glyph->bitmap.width;
			}
		}
		for(n=0;n<=(strlen(text)-1);n++) {
			if(text[n]==' ') {
				totalWidth += (int)(fontWidth*0.75);
			} else {		
				glyph_index = FT_Get_Char_Index(face, text[n]);
				FT_Load_Glyph(face, glyph_index, 0);	
				FT_Render_Glyph(face->glyph, ft_render_mode_normal);
				totalWidth += (int)spacing+face->glyph->bitmap.width;
			}
		}
		return (short)totalWidth;
	}
	return -1;
}

short txt_get_height(char *font, char *text, FT_UInt size, double spacing, int decoration, int align) {
	int n = 0;
	int fontHeight = 0;
	int fsize; 
	
	if(fcache_get_size(font, &fsize) == -1) {
		if(fcache_add(font) == -1) {
			logprintf(LOG_ERR, "could not cache %s", font);
		}
	}
	
	if(fcache_get_size(font, &fsize) > -1) {
	
		FT_Library library;
		FT_Face face;
		FT_Glyph_Metrics *metrics;

		FT_Init_FreeType(&library);
		FT_New_Memory_Face(library, fcache_get_bytes(font), (FT_Long)fsize, 0, &face);
		FT_Set_Char_Size(face, (FT_F26Dot6)size*64, 0, 72, 0);
		FT_Set_Pixel_Sizes(face, 0, size);
		FT_UInt glyph_index;
		
		for(n=0;n<strlen(text);n++) {
			glyph_index = FT_Get_Char_Index(face, text[n]);
			FT_Load_Glyph(face, glyph_index, 0);
			metrics = &face->glyph->metrics;
			FT_Render_Glyph(face->glyph, ft_render_mode_normal);
			if((metrics->horiBearingY/64) > fontHeight) {
				fontHeight = (metrics->horiBearingY/64);
			}
		}
		return (short)fontHeight;
	}
	return -1;
}

void draw_rmtxt(double xc, double yc, int z, char *font, FT_UInt size, char *text, unsigned int color, double spacing, int decoration, int align) {
	int y = 0, x = 0, n = 0;
	double a = 0;
	int fontHeight = 0, fontWidth = 0;
	int totalWidth = 0;
	int fsize; 
	
	if(fcache_get_size(font, &fsize) == -1) {
		if(fcache_add(font) == -1) {
			logprintf(LOG_ERR, "could not cache %s", font);
		}
	}
	
	if(fcache_get_size(font, &fsize) > -1) {
	
		FT_Library library;
		FT_Face face;
		FT_Glyph_Metrics *metrics;

		FT_Init_FreeType(&library);
		FT_New_Memory_Face(library, fcache_get_bytes(font), (FT_Long)fsize, 0, &face);
		FT_Set_Char_Size(face, (FT_F26Dot6)size*64, 0, 72, 0);
		FT_Set_Pixel_Sizes(face, 0, size);
		FT_UInt glyph_index;
		
		for(n=0;n<strlen(text);n++) {
			glyph_index = FT_Get_Char_Index(face, text[n]);
			FT_Load_Glyph(face, glyph_index, 0);
			metrics = &face->glyph->metrics;
			FT_Render_Glyph(face->glyph, ft_render_mode_normal);
			if((metrics->horiBearingY/64) > fontHeight) {
				fontHeight = (metrics->horiBearingY/64);
			}
			if(face->glyph->bitmap.width > fontWidth) {
				fontWidth = face->glyph->bitmap.width;
			}
		}
		for(n=0;n<=(strlen(text)-1);n++) {
			if(text[n]==' ') {
				totalWidth += (int)(fontWidth*0.75);
			} else {		
				glyph_index = FT_Get_Char_Index(face, text[n]);
				FT_Load_Glyph(face, glyph_index, 0);
				metrics = &face->glyph->metrics;	
				FT_Render_Glyph(face->glyph, ft_render_mode_normal);
				if(decoration==ITALIC) {
					a=face->glyph->bitmap.rows*0.25;
				}
				totalWidth += (int)spacing+face->glyph->bitmap.width;
			}
		}
		
		yc+=(fontHeight/2);
		if(align == ALIGN_RIGHT) {
			xc -= totalWidth;
		} else if(align == ALIGN_CENTER) {
			xc -= (totalWidth/2);
		}		
		for(n=0;n<=(strlen(text)-1);n++) {
			if(text[n]==' ') {
				xc += (int)(fontWidth*0.75);
			} else {		
				glyph_index = FT_Get_Char_Index(face, text[n]);
				FT_Load_Glyph(face, glyph_index, 0);
				metrics = &face->glyph->metrics;	
				FT_Render_Glyph(face->glyph, ft_render_mode_normal);
				if(decoration==ITALIC) {
					a=face->glyph->bitmap.rows*0.25;
				}
				for(y=0;y<face->glyph->bitmap.rows;y++) {
					for (x=0;x<face->glyph->bitmap.width;x++) {
						if(face->glyph->bitmap.buffer[y*face->glyph->bitmap.width + x] > 0) {
							fb_rm_pixel((int)(x+xc+a), (int)(y+yc-(metrics->horiBearingY/64)), z, color);	
						}
					}
					if(decoration==ITALIC) {			
						a-=0.25;
					}
				}
				xc += spacing+face->glyph->bitmap.width;
			}
		}
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	}
}

void draw_txt(double xc, double yc, int z, char *font, FT_UInt size, char *text, unsigned int color, double spacing, int decoration, int align) {
	int y = 0, x = 0, n = 0;
	double a = 0;
	int fontHeight = 0, fontWidth = 0;
	int totalWidth = 0;
	int fsize; 
	
	if(fcache_get_size(font, &fsize) == -1) {
		if(fcache_add(font) == -1) {
			logprintf(LOG_ERR, "could not cache %s", font);
		}
	}
	
	if(fcache_get_size(font, &fsize) > -1) {
	
		FT_Library library;
		FT_Face face;
		FT_Glyph_Metrics *metrics;

		FT_Init_FreeType(&library);
		FT_New_Memory_Face(library, fcache_get_bytes(font), (FT_Long)fsize, 0, &face);
		FT_Set_Char_Size(face, (FT_F26Dot6)size*64, 0, 72, 0);
		FT_Set_Pixel_Sizes(face, 0, size);
		FT_UInt glyph_index;
		
		for(n=0;n<strlen(text);n++) {
			glyph_index = FT_Get_Char_Index(face, text[n]);
			FT_Load_Glyph(face, glyph_index, 0);
			metrics = &face->glyph->metrics;
			FT_Render_Glyph(face->glyph, ft_render_mode_normal);
			if((metrics->horiBearingY/64) > fontHeight) {
				fontHeight = (metrics->horiBearingY/64);
			}
			if(face->glyph->bitmap.width > fontWidth) {
				fontWidth = face->glyph->bitmap.width;
			}
		}
		for(n=0;n<=(strlen(text)-1);n++) {
			if(text[n]==' ') {
				totalWidth += (int)(fontWidth*0.75);
			} else {		
				glyph_index = FT_Get_Char_Index(face, text[n]);
				FT_Load_Glyph(face, glyph_index, 0);
				metrics = &face->glyph->metrics;	
				FT_Render_Glyph(face->glyph, ft_render_mode_normal);
				if(decoration==ITALIC) {
					a=face->glyph->bitmap.rows*0.25;
				}
				totalWidth += (int)spacing+face->glyph->bitmap.width;
			}
		}
		
		yc+=(fontHeight/2);
		if(align == ALIGN_RIGHT) {
			xc -= totalWidth;
		} else if(align == ALIGN_CENTER) {
			xc -= (totalWidth/2);
		}		
		for(n=0;n<=(strlen(text)-1);n++) {
			if(text[n]==' ') {
				xc += (int)(fontWidth*0.75);
			} else {		
				glyph_index = FT_Get_Char_Index(face, text[n]);
				FT_Load_Glyph(face, glyph_index, 0);
				metrics = &face->glyph->metrics;	
				FT_Render_Glyph(face->glyph, ft_render_mode_normal);
				if(decoration==ITALIC) {
					a=face->glyph->bitmap.rows*0.25;
				}
				for(y=0;y<face->glyph->bitmap.rows;y++) {
					for (x=0;x<face->glyph->bitmap.width;x++) {
						if(face->glyph->bitmap.buffer[y*face->glyph->bitmap.width + x] > 0) {
							fb_put_pixel((int)(x+xc+a), (int)(y+yc-(metrics->horiBearingY/64)), z, color);	
						}
					}
					if(decoration==ITALIC) {			
						a-=0.25;
					}
				}
				xc += spacing+face->glyph->bitmap.width;
			}
		}
	FT_Done_Face(face);
	FT_Done_FreeType(library);
	}
}