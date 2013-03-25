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

Various functions used in Splash are derived from fbi (Gerd Hoffmann <gerd@kraxel.org>)

sudo apt-get install gcc uthash-dev

gcc splash.c -lm -o splash /usr/lib/arm-linux-gnueabihf/libjpeg.a /usr/lib/arm-linux-gnueabihf/libfreetype.a /usr/lib/arm-linux-gnueabihf/libz.a -I/usr/include/freetype2/
*/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#endif

#include "logger.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <locale.h>
#include <ctype.h>
#include <setjmp.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <getopt.h>

#include "uthash.h"
#include <jpeglib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int fbfd, tty = 0, orig_vt_no = 0, kd_mode;
struct fb_var_screeninfo vinfo;
struct fb_var_screeninfo ovinfo;
struct fb_fix_screeninfo finfo;
struct vt_mode vt_omode;
struct termios term;
static unsigned short ored[256], ogreen[256], oblue[256];
static struct fb_cmap ocmap = {0, 256, ored, ogreen, oblue};

long int screensize = 0;
char *fbp = 0;
FILE *fperc;

static int keepRunning = 1;
int fb_mem_offset = 0;
int output;
int suspended = 0;
pid_t pid;

//Shared memory
size_t mlength = 4096; 
char * addr = 0; 
off_t moffset = 0; 
int prot = (PROT_READ| PROT_WRITE); 
int flags = (MAP_SHARED); 
int fd = -1; 
int pid_file = -1;
int child = 0;

//Progress bar
int offset = 0, speed = 5000;
int nperc = 0;
int lperc = 0;
int update = 0;

//Cache
typedef struct caches_t {
	char id;
	char name[255];
	unsigned long size;
	UT_hash_handle hh; 
} caches_t;

caches_t *caches;

unsigned char *bytes[255];
int nrCachedFiles = 0;

//Arguments
typedef struct arguments_t {
	char id;
	char value[255];
	UT_hash_handle hh; 
} arguments_t;

//Options
char PATH[255];
int INITIALIZED=0;

int SHOWPERCENTAGEBAR=1;
int SHOWINFINITEBAR=0;
int PERCENTAGE=0;
int DIRECTION=0;
int BLACK=0;
int REDRAW=0;

//Paths relative to /usr/bin/splash
char *BARIMG="images/progressbar.jpg";
char *FONT="fonts/Comfortaa-Light.ttf";
char MEMORY[255]="/run/";
char PID[255]="/run/";
char absPid[255];
char absMem[255];
char g_logfile[255]="/var/log/splash.log";
int  g_logging=0;

char *MSGTXT="loading...";
int FONTSIZE=32;

int LOGOCHANGE=1;
int BARCHANGE=1;
int MSGCHANGE=1;

void fb_memset(void *addr, int c, size_t len) {
#if 1
    unsigned int i, *p;
    
    i = (c & 0xff) << 8;
    i |= i << 16;
    len >>= 2;
    for (p = addr; len--; p++)
		*p = i;
#else
    memset(addr, c, len);
#endif
}	
	
static void fb_setvt(int vtno) {
    struct vt_stat vts;
    char vtname[12];

    if(vtno < 0) {
		if (-1 == ioctl(tty,VT_OPENQRY, &vtno) || vtno == -1) {
			perror("ioctl VT_OPENQRY");
			exit(1);
		}
    }

    vtno &= 0xff;
    sprintf(vtname, "/dev/tty%d", vtno);
    chown(vtname, getuid(), getgid());
    if(access(vtname, R_OK | W_OK) == -1) {
		fprintf(stderr,"access %s: %s\n",vtname,strerror(errno));
		exit(1);
    }
	
	switch(fork()) {
		case 0:	
			break;
		case -1:
			perror("fork");
			exit(1);
		default:
			exit(0);
	}

    close(tty);
    close(0);
    close(1);
    close(2);
    
	setsid();
	open(vtname,O_RDWR);
	dup(0);
    dup(0);

    if(-1 == ioctl(tty,VT_GETSTATE, &vts) == -1) {
		perror("ioctl VT_GETSTATE");
		exit(1);
    }
    orig_vt_no = vts.v_active;
    
	if(ioctl(tty,VT_ACTIVATE, vtno) == -1) {
		perror("ioctl VT_ACTIVATE");
		exit(1);
    }
    if(ioctl(tty,VT_WAITACTIVE, vtno) == -1) {
		perror("ioctl VT_WAITACTIVE");
		exit(1);
    }
}

void fb_cleanup(void) {
    if(ioctl(tty,KDSETMODE, kd_mode) == -1)
		perror("ioctl KDSETMODE");
    if(ioctl(fbfd,FBIOPUT_VSCREENINFO,&ovinfo) == -1)
		perror("ioctl FBIOPUT_VSCREENINFO");
    if(ioctl(fbfd,FBIOGET_FSCREENINFO,&finfo) == -1)
		perror("ioctl FBIOGET_FSCREENINFO");
	if(ovinfo.bits_per_pixel == 8 || finfo.visual == FB_VISUAL_DIRECTCOLOR) {
		if(ioctl(fbfd,FBIOPUTCMAP,&ocmap) == -1)
			perror("ioctl FBIOPUTCMAP");
	}
	close(fbfd);

    if(ioctl(tty,VT_SETMODE, &vt_omode) == -1)
		perror("ioctl VT_SETMODE");
    if(orig_vt_no && ioctl(tty, VT_ACTIVATE, orig_vt_no) == -1)
		perror("ioctl VT_ACTIVATE");
    if(orig_vt_no && ioctl(tty, VT_WAITACTIVE, orig_vt_no) == -1)
		perror("ioctl VT_WAITACTIVE");
    tcsetattr(tty, TCSANOW, &term);
    close(tty);
}
	
void init() {
	unsigned long page_mask;
	int vt = 1;
    struct vt_stat vts;
	
    if(vt != 0)
		fb_setvt(vt);
	
	if(ioctl(tty,VT_GETSTATE, &vts) == -1) {
		fprintf(stderr,"ioctl VT_GETSTATE: %s (not a linux console?)\n", strerror(errno));
		exit(1);
    }
	
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }	

    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &ovinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }

    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }		
	
	if(ovinfo.bits_per_pixel == 8 || finfo.visual == FB_VISUAL_DIRECTCOLOR) {
		if(ioctl(fbfd,FBIOGETCMAP,&ocmap) == -1) {
			perror("ioctl FBIOGETCMAP");
			exit(1);
		}
    }
	
	if(ioctl(tty,KDGETMODE, &kd_mode) == -1) {
		perror("ioctl KDGETMODE");
		exit(1);
	}
    if(ioctl(tty,VT_GETMODE, &vt_omode) == -1) {
		perror("ioctl VT_GETMODE");
		exit(1);
    }
    tcgetattr(tty, &term);
    
    if(ioctl(fbfd,FBIOGET_VSCREENINFO,&vinfo) == -1) {
		perror("ioctl FBIOGET_VSCREENINFO");
		exit(1);
    }
 
    if(ioctl(fbfd,FBIOGET_FSCREENINFO,&finfo) == -1) {
		perror("ioctl FBIOGET_FSCREENINFO");
		exit(1);
    }	

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	page_mask = getpagesize()-1;
	fb_mem_offset = (unsigned long)(finfo.smem_start) & page_mask;
    fbp = (char *)mmap(0, finfo.smem_len+fb_mem_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
   
	if((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
	
	if(ioctl(tty,KDSETMODE, KD_GRAPHICS) == -1) {
		perror("ioctl KDSETMODE");
		fb_cleanup();
		exit(0);
    }
	
	fb_memset(fbp+fb_mem_offset, 0, finfo.line_length * vinfo.yres);
}

int min(int a, int b) {
	if(a<b)
		return a;
	else
		return b;
}

int max(int a, int b) {
	if(a>b)
		return a;
	else
		return b;
}

double sqr(double a) {
	return (a*a);
}

char * rel2abs(const char *base, const char *path ) {
	int size=255;
	char result[size];

	if(strlen(path)+strlen(base) > size) {
		errno = ERANGE;
		return (NULL);
	}	

	sprintf(result, "%s.%s", path, base);
	return result;
}

unsigned short int createColor16bit(int r, int g, int b) {
	return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

long int getLocation(int x, int y) {
	return (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
}

void drawPixel(int x, int y, unsigned short int color) {
	long int location;
	location = getLocation(x, y);
	if((fbp + location))
		*((unsigned short int*)(fbp + location)) = color;
}

void setBackgroundColor(int r, int g, int b) {
	int x = 0, y = 0;
	unsigned short int color = createColor16bit(r, b, g);
	long int location;
	for (y = 0; y < vinfo.yres; y++) {
		for (x = 0; x < vinfo.xres; x++) {
            drawPixel(x, y, color);
		}
	}
}

long int widthToPixel(int i) {
	return (i+vinfo.xoffset)*(vinfo.bits_per_pixel/8);
}

long int heightToPixel(int i) {
	return (i+vinfo.yoffset)*finfo.line_length;
}

void drawFilledRectangle(int x, int y, int width, int height, unsigned short int color) {
	int a=0, b=0;
	long int location;
	
	for(a = x; a < x+width; a++) {
		for(b = y; b < y+height; b++) {
			drawPixel(a, b, color);
		} 
	}
} 

void drawRectangle(int x, int y, int width, int height, int border, unsigned short int color) {
	int a=0, b=0;
	long int location;
	
	for(a = x; a < x+width; a++) {
		for(b = y; b < y+height; b++) {
			if((a<(x+border)) || (a>((x+width)-border)) || (b<(y+border)) || (b>((y+height)-border))) {
				drawPixel(a, b, color);
			}
		}
	}
}

void drawLine(int x2, int y2, int x1, int y1, unsigned short int color) {
	const int dx = abs(x1 - x2);
	const int dy = abs(y1 - y2);
	int const1, const2, p, x, y, step;
	if(dx >= dy) {
		const1 = 2 * dy;
		const2 = 2 * (dy - dx);
		p = 2 * dy - dx;
		x = min(x1,x2);
		y = (x1 < x2) ? (y1) : (y2);
		step = (y1 > y2) ? (1) : (-1);
		drawPixel(x, y, color);
		while (x < max(x1,x2)) {
			if(p < 0) {
				p += const1;
			} else {
				y += step;
				p += const2;
			}
        drawPixel(++x, y, color);
		}
	} else {
		const1 = 2 * dx;
		const2 = 2 * (dx - dy);
		p = 2 * dx - dy;
		y = min(y1,y2);
		x = (y1 < y2) ? (x1) : (x2);
		step = (x1 > x2) ? (1) : (-1);
		drawPixel(x, y, color);
		while(y < max(y1,y2)) {
			if(p < 0)
				p += const1;
			else {
				x += step;
				p += const2;
			}
		drawPixel(x, ++y, color);
		}
	}
}

void drawFilledCircle(int xc, int yc, int r, unsigned short int color) {
	int x = 0, y = 0, p = 0, z = 0;
	xc+=r;
	yc+=r;
	
	x = 0, y = 0;
	y = (r-z);
	p = 3 - 2 * (r-z);
	while (x <= y) {
		drawLine(xc - x, yc - y, xc + x, yc + y, color);
		drawLine(xc + x, yc - y, xc - x, yc + y, color);
		drawLine(xc - y, yc - x, xc + y, yc + x, color);
		drawLine(xc - y, yc + x, xc + y, yc - x, color);
		if (p < 0)
			p += 4 * x++ + 6;
		else
			p += 4 * (x++ - y--) + 10;
	}
}

void drawCircle(int xc, int yc, int r, int border, unsigned short int color) {
	int x = 0, y = 0, p = 0, z = 0;
	xc+=r;
	yc+=r;
	for(z = 0; z < border; z++) {
		x = 0, y = 0;
		y = (r-z);
		p = 3 - 2 * (r-z);
		while (x <= y) {
			drawPixel(xc + x, yc + y, color);
			drawPixel(xc - x, yc + y, color);
			drawPixel(xc + x, yc - y, color);
			drawPixel(xc - x, yc - y, color);
			drawPixel(xc + y, yc + x, color);
			drawPixel(xc - y, yc + x, color);
			drawPixel(xc + y, yc - x, color);
			drawPixel(xc - y, yc - x, color);
			if (p < 0)
				p += 4 * x++ + 6;
			else
				p += 4 * (x++ - y--) + 10;
		}
	}
}

int cacheFile(char *filename) {
	unsigned long filesize;
	struct stat fileinfo;
	char absPath[255];
	int i, rc;

	LOG_PRINT("Going to cache %s.%s", PATH,filename);
	strcpy(absPath,rel2abs(filename, PATH));
	rc = stat(absPath, &fileinfo);

	if(rc) {
		LOG_PRINT("Can't cache (open?) %s : %s -> %s", filename, PATH, rel2abs(filename, PATH));
		return -1;
	} else {
		filesize = fileinfo.st_size;
		bytes[nrCachedFiles] = (unsigned char*) malloc(filesize + 100);
		int fd = open(absPath, O_RDONLY);

		i = 0;
		while (i < filesize) {
			rc = read(fd, bytes[nrCachedFiles] + i, filesize - i);
			i += rc;
		}
		close(fd);
		
		caches_t *cache=NULL;
		cache = (caches_t*)malloc(sizeof(caches_t));
		cache->id=nrCachedFiles;
		cache->size=filesize;
		strcpy(cache->name,filename);
		HASH_ADD_STR(caches,name,cache);
		nrCachedFiles++;		
		return nrCachedFiles-1;	
	}
}

int findCachedFileId(char *filename) {
	caches_t *tmp, *cache=NULL;
	HASH_FIND_STR(caches,filename,tmp);
	if(tmp) {
		return tmp->id;
	} else {
		return -1;
	}
}

unsigned long findCachedFileSize(char *filename) {
	caches_t *tmp, *cache=NULL;
	HASH_FIND_STR(caches,filename,tmp);
	if(tmp) {
		return tmp->size;
	} else {
		return -1;
	}
}
	
int drawJPEG(int xc, int yc, char *filename) {
	unsigned char *raw_image = NULL;
	int id;
	unsigned long size;
	unsigned long location = 0;
	int i = 0;
	unsigned char a,r,g,b;
	int ir, ig, ib;	
	struct jpeg_decompress_struct jpegInfo;
	struct jpeg_error_mgr jpegErr;
				
	if((id = findCachedFileId(filename)) == -1) {
		id=cacheFile(filename);
	}
	if(id > -1) {
		size=findCachedFileSize(filename);
			
		JSAMPROW row_pointer[1];
		jpegInfo.err = jpeg_std_error(&jpegErr);
		jpeg_create_decompress(&jpegInfo);
		jpeg_mem_src(&jpegInfo, bytes[id], size);
		jpeg_read_header(&jpegInfo, TRUE);
		
		jpeg_start_decompress(&jpegInfo);

		raw_image = (unsigned char*)malloc(jpegInfo.output_width*jpegInfo.output_height*jpegInfo.num_components);
		row_pointer[0] = (unsigned char *)malloc(jpegInfo.output_width*jpegInfo.num_components);
		
		int x=1, y=1, line;
		while(jpegInfo.output_scanline < jpegInfo.image_height) {
			jpeg_read_scanlines(&jpegInfo, row_pointer, 1);
			
			for(i=0; i<jpegInfo.image_width*jpegInfo.num_components; i++) {
				raw_image[location++] = row_pointer[0][i];
			}
		}
		for(line = jpegInfo.image_height-1; line >= 0; line--) {
			for(x=0; x < jpegInfo.image_width; x++) {
				b = *(raw_image+(x+line*jpegInfo.image_width)*jpegInfo.num_components+2);
				g = *(raw_image+(x+line*jpegInfo.image_width)*jpegInfo.num_components+1);
				r = *(raw_image+(x+line*jpegInfo.image_width)*jpegInfo.num_components+0);
				
				drawPixel(x+xc,line+yc,createColor16bit((((int)r & 0xFF) << 0),(((int)g & 0xFF) << 0),(((int)b & 0xFF) << 0)));
			}
		}

		jpeg_finish_decompress(&jpegInfo);
		jpeg_destroy_decompress(&jpegInfo);
		free(row_pointer[0]);
		free(raw_image);
	}
	
	return 1;
}

void drawText(int xc, int yc, char *font, int size, char *text, unsigned short int color, double spacing, int decoration, int align) {
	int n, y, x, id;
	double z;
	int fontHeight=0, fontWidth=0;
	unsigned long fsize; 
	
	if((id = findCachedFileId(font)) == -1) {
		id=cacheFile(font);
	}
	if(id > -1) {
		fsize=findCachedFileSize(font);	
	
		FT_Library library;
		FT_Face face;
		FT_GlyphSlot slot;
		FT_Matrix matrix;
		FT_Vector pen;
		FT_Error error;
		FT_Glyph_Metrics *metrics;

		FT_Init_FreeType(&library);
		FT_New_Memory_Face(library, bytes[id], fsize, 0, &face);
		FT_Set_Char_Size(face, size*64, 0, 72, 0);
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
		if(align == 1) {
			for(n=(strlen(text)-1);n>=0;n--) {
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
								drawPixel(x+xc+z,y+yc-(metrics->horiBearingY/64),color);	
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
								drawPixel(x+xc+z,y+yc-(metrics->horiBearingY/64),color);	
							}
						}
						if(decoration==1) {			
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

void fb_clear_mem(void) {
	fb_memset(fbp,0,finfo.smem_len);
}

void fb_clear_screen(void) {
	fb_memset(fbp,0,finfo.line_length * vinfo.yres);
}

void gc() {
	int x=0, length, err;

    keepRunning = 0;
	if(child) {
		fb_clear_mem();
		fb_clear_screen();
		fb_cleanup();
		
	}
	munmap(addr, mlength);
	close(fd);
	
	if(child) {
		flock(pid_file, LOCK_UN);
	}
	
	close(pid_file);
	if(child) {
		unlink(absMem);		
		unlink(absPid);
		close(fbfd);

		for(x=0;x<nrCachedFiles;x++) {
			free(bytes[x]);
		}		
	}
}

void trap(int dummy) {
	gc();
}

static jmp_buf fb_fatal_cleanup;

static void fb_catch_exit_signal(int signal) {
    siglongjmp(fb_fatal_cleanup,signal);
}

void fb_catch_exit_signals(void) {
    struct sigaction act,old;
    int termsig;

    memset(&act,0,sizeof(act));
    act.sa_handler = fb_catch_exit_signal;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act,&old);
    sigaction(SIGQUIT,&act,&old);
    sigaction(SIGTERM,&act,&old);

    sigaction(SIGABRT,&act,&old);
    sigaction(SIGTSTP,&act,&old);

    sigaction(SIGBUS, &act,&old);
    sigaction(SIGILL, &act,&old);
    sigaction(SIGSEGV,&act,&old);

    if ((termsig = sigsetjmp(fb_fatal_cleanup,0)) == 0)
		return;

    gc();
    exit(42);
}

void showProgressBar() {
	int x = 0, y = 0, i = 0;
	
	if(PERCENTAGE) {
		if(nperc < PERCENTAGE && PERCENTAGE <= 100 && PERCENTAGE >= 1) {
			nperc=PERCENTAGE;
			update=1;
		}
	}
	offset=253;
	if((nperc >= 3 && update==1) || SHOWINFINITEBAR) {
		if(DIRECTION) {
			drawJPEG(((vinfo.xres/2)-offset),((vinfo.yres/2)+54),"images/emptyStart.jpg");
		} else {
			drawJPEG(((vinfo.xres/2)-offset),((vinfo.yres/2)+54),"images/barStart.jpg");
		}			
		if(nperc <= 2) {
			update=0;
		}
		if(SHOWINFINITEBAR) {
			usleep(speed);
		}
	}
	offset-=12;
	
	if((nperc >= 3 && update==1) || SHOWINFINITEBAR) {
		if(nperc > 96 || SHOWINFINITEBAR) {
			x=96;
		} else {
			x=nperc;
		}
		offset-=lperc;
		for(i=lperc;i<=((x*5));i++) {
			if(DIRECTION) {
				drawJPEG(((vinfo.xres/2)-offset),((vinfo.yres/2)+54),"images/emptyFill.jpg");
			} else {
				drawJPEG(((vinfo.xres/2)-offset),((vinfo.yres/2)+54),"images/barFill.jpg");
			}
			offset--;
			if(SHOWINFINITEBAR) {
				usleep(speed);
			}
		}
		if(nperc <= 96) {
			lperc=(x*5);
			update=0;
		}
	}
	if((nperc >= 97 && update==1) || SHOWINFINITEBAR) {
		if(DIRECTION) {
			drawJPEG(((vinfo.xres/2)-offset),((vinfo.yres/2)+54),"images/emptyEnd.jpg");
		} else {
			drawJPEG(((vinfo.xres/2)-offset),((vinfo.yres/2)+54),"images/barEnd.jpg");
		}
		update=0;
		if(SHOWINFINITEBAR) {
			usleep(speed);
			nperc=0;
			lperc=0;
			if(DIRECTION) {
				DIRECTION=0;
			} else {
				DIRECTION=1;
			}
		}
	}
	if(SHOWPERCENTAGEBAR) {
		sleep(1);
		if(nperc >= 3 && nperc <= 96) {
			nperc++;
			update=1;
		}
	}
}

void writeArguments(struct arguments_t *arguments) {
	arguments_t *argument;
	int x,y=0;
	if(!addr) {
		addr = mmap(NULL, mlength, prot, flags, fd, moffset); 
		if(addr == 0) { 
			exit(1);
		}
	}
	
	for(argument=arguments; argument != NULL; argument=(arguments_t*)(argument->hh.next)) {
		addr[y++] = '@';
		addr[y++] = argument->id;
		addr[y++] = '@';
		for(x=0;x<strlen(argument->value);x++) {
			addr[y++] = argument->value[x];
		}
		addr[y++] = '@';
	}
	addr[y++] = '#';
	msync(addr,sizeof(int),MS_SYNC|MS_INVALIDATE); 
	if(munmap(addr, mlength) == -1) { 
		exit(0);
	}
}

struct arguments_t *readArguments() {
	int i=0,y=0,x=0;
	arguments_t *argument, *arguments=NULL;
	
	if(!addr) {
		addr = mmap(NULL, mlength, prot, flags, fd, moffset); 
		if(addr == 0) { 
			exit(1);
		}
	addr[0]='#'; 		
	}
	
	msync(addr,sizeof(int),MS_SYNC|MS_INVALIDATE);
	
	for (i=0; i<=mlength; i++) {
		if(addr[i] == '#') {
			break;
		} else if(addr[i] == '@') {
			y++;
		}
		if(y==1) {
			argument = (arguments_t*)malloc(sizeof(arguments_t));
			argument->id=addr[++i];
		} else if(y==2) {
			y++;
		} else if(y==3) {
			argument->value[x]=addr[i];
			x++;
		} else if(y==4) {
			HASH_ADD_INT(arguments,id,argument);
			y=0;
			x=0;
		}
	}
	memset(addr,0,mlength);
	addr[0]='#';
	return arguments;
}

void parseArguments(struct arguments_t *arguments) {
	arguments_t *argument;
	for(argument=arguments; argument != NULL; argument=(arguments_t*)(argument->hh.next)) {
		switch(argument->id) {
			case 'a':
				SHOWPERCENTAGEBAR=1;
				SHOWINFINITEBAR=0;
				PERCENTAGE=0;
				lperc=0;
				nperc=0;
			break;
			case 'b':
				SHOWPERCENTAGEBAR=0;
				SHOWINFINITEBAR=1;
				if(PERCENTAGE==100) {
					if(DIRECTION) {
						DIRECTION=0;
					} else {
						DIRECTION=1;
					}
				lperc=0;
				nperc=0;
				}
				PERCENTAGE=0;
			break;
			case 'c':
				if(argument->value) {
					PERCENTAGE=atoi(argument->value);
				} else {
					PERCENTAGE++;
				}
			break;
			case 'd':
				BARIMG=argument->value;
				BARCHANGE=1;
			break;
			case 'e':
				drawText(((vinfo.xres/2)+250), ((vinfo.yres/2)+85), FONT, FONTSIZE, MSGTXT, createColor16bit(0,0,0), 5, 1, 1);			
				MSGTXT=argument->value;
				MSGCHANGE=1;
			break;
			case 'f':
				BLACK=1;
				REDRAW=1;
			break;
			case 'g':
				if(BLACK) {
					lperc=0;
					MSGCHANGE=1;
					BARCHANGE=1;
					LOGOCHANGE=1;
				}
				BLACK=0;
			break;
			case 'h':
				drawText(((vinfo.xres/2)+250), ((vinfo.yres/2)+85), FONT, FONTSIZE, MSGTXT, createColor16bit(0,0,0), 5, 1, 1);						
				FONT=argument->value;
				MSGCHANGE=1;
			break;
			case 'i':
				drawText(((vinfo.xres/2)+250), ((vinfo.yres/2)+85), FONT, FONTSIZE, MSGTXT, createColor16bit(0,0,0), 5, 1, 1);			
				FONTSIZE=atoi(argument->value);
				MSGCHANGE=1;
			break;
			case 'j':
				if(INITIALIZED) {
					child=1;
				} else {
					child=0;
				}
				keepRunning=0;
				kill(getpid(), SIGQUIT);
				sleep(1);
				exit(0);
			break;
			case 'k':
				g_logging=1;
				if(strlen(argument->value)>255) 
					continue;
				else
					sprintf(g_logfile,"%s",argument->value);
			break;
		}
	}					
}

int main(int argc, char **argv) {
	int i, x, y, opt, length;
	struct stat s;
	char buf[20];
	arguments_t *argument, *arguments=NULL;
	
	//Extract absolute path of executable
	length=readlink("/proc/self/exe", PATH, sizeof(PATH));
	PATH[length] = '\0';
	LOG_PRINT("Actual path is %s", PATH);
	
	//Command line arguments
	static struct option options[] = {
		{"percentagebar", no_argument, NULL, 'a'},
		{"infinitebar", no_argument, NULL, 'b'},
		{"percentage", optional_argument, NULL, 'c'},
		{"barimg", required_argument, NULL, 'd'},
		{"msgtxt", required_argument, NULL, 'e'},
		{"black", no_argument, NULL, 'f'},
		{"flip", no_argument, NULL, 'g'},
		{"font", required_argument, NULL, 'h'},
		{"fontsize", required_argument, NULL, 'i'},
		{"exit", no_argument, NULL, 'j'},
		{"log", optional_argument, NULL, 'k'},
		{0, 0, 0, 0}
	};
	
	setlocale(LC_ALL,"");
	
	//Catch all exit signal for garbage collection
	fb_catch_exit_signals();
	
	//Create data/pid file witht the same name as the executable

	sprintf(absPid,"%s%s%s",PID,basename(argv[0]),".pid");
	sprintf(absMem,"%s%s%s",MEMORY,basename(argv[0]),".dat");
	LOG_PRINT("absPid %s", absPid);

	//Open PID file and lock
	int pid_file = open(absPid, O_RDWR | O_CREAT, 0644);
	lockf(pid_file, F_LOCK, 0);
	
	//Read PID from pid file
	length = read(pid_file, buf, sizeof(buf)-1);
	buf[length] = '\0';
		
	//Open memory file
	fd = open(absMem, O_RDWR | O_CREAT | O_TRUNC);
	write(fd,"0",1);
	if(fd == 0) { 
		exit(1);
	} 
	if(lseek(fd, mlength - 1, SEEK_SET) == -1) { 
		exit(1);
	}
	
	//Store all command line arguments into hash 
	while(1) {
		int opt = getopt_long(argc, argv, "abcd:e:fgh:i:k:", options, NULL);

		if(opt == -1) {
			break;
		}
		if(opt != '?') {
			argument = (arguments_t*)malloc(sizeof(arguments_t));
			argument->id=opt;
			if(optarg) {
				strcpy(argument->value,optarg);
			}
			HASH_ADD_INT(arguments,id,argument);
		}
	}

	//If program is already running or if no pid found
	if(kill((pid_t)(atoi(buf)), 0) != 0 || (atoi(buf)) == 0) {
		child=1;
		
		//Process the arguments given to the server
		parseArguments(arguments);
		while(keepRunning) {
		
			//Read arguments from memory given to client
			parseArguments(readArguments());
			if(keepRunning) {
				if(!INITIALIZED) {
					//Initialize framebuffer
					init();
					
					//Assign new PID to file
					lseek(pid_file, 0L, SEEK_SET);
					length = snprintf(buf, sizeof(buf)-1, "%d\n", getpid());
					buf[length] = '\0';
					write(pid_file, buf, length);
					ftruncate(pid_file, length);
					
					cacheFile("images/emptyStart.jpg");
					cacheFile("images/emptyEnd.jpg");
					cacheFile("images/emptyFill.jpg");
					cacheFile("images/barStart.jpg");
					cacheFile("images/barEnd.jpg");
					cacheFile("images/barFill.jpg");
					cacheFile("images/logo.jpg");
					cacheFile("images/progressbar.jpg");
					cacheFile(BARIMG);
					cacheFile(FONT);
					
					INITIALIZED=1;
				}
			
				if(!BLACK) {
					if(LOGOCHANGE) {
						drawJPEG(((vinfo.xres/2)-277),((vinfo.yres/2)-145),"images/logo.jpg");
						LOGOCHANGE=0;
					} if(BARCHANGE) {
						drawJPEG(((vinfo.xres/2)-277),((vinfo.yres/2)+16),BARIMG);
						BARCHANGE=0;
					} if(MSGCHANGE) {
						drawText(((vinfo.xres/2)+250), ((vinfo.yres/2)+85), FONT, FONTSIZE, MSGTXT, createColor16bit(141,141,141), 5, 1, 1);		
						MSGCHANGE=0;
					}
					
					showProgressBar();
				} else {
					if(REDRAW) {
						setBackgroundColor(0, 0, 0);
						REDRAW=0;
					}
					sleep(1);
				}
			}
		else
			gc();
		}
		
		//Garbage collect
		gc();
	} else {
		child=0;
		//Write arguments to memory for server to read
		writeArguments(arguments);
	}
    return 0;
}
