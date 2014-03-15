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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/mman.h>

#include "common.h"
#include "fb.h"
#include "gc.h"
#include "config.h"
#include "log.h"

struct fb_var_screeninfo vinfo;
struct fb_var_screeninfo ovinfo;
struct fb_fix_screeninfo finfo;
struct vt_mode vt_omode;
struct termios term;

static unsigned short ored[256], ogreen[256], oblue[256];
static struct fb_cmap ocmap = {0, 256, ored, ogreen, oblue};

unsigned long screensize = 0;
int fbfd = 0;
int tty = 0;
int orig_vt_no = 0;
int kd_mode;
unsigned long fb_mem_offset = 0;

char *fbp = 0;

unsigned int ***zmap = NULL;

void fb_init_zmap(void) {
    size_t zindexes = ZINDEXES, width = fb_width(), height = fb_height();
    int a = 0, b = 0;
 
    if(!(zmap = calloc(zindexes, sizeof(int**)))) {
        logprintf(LOG_ERR, "out of memory");
        exit(EXIT_FAILURE);
    }
 
    for(a=0;a<zindexes;a++) {
        if(!(zmap[a] = calloc(width, sizeof(int*)))) {
            logprintf(LOG_ERR, "out of memory");
            exit(EXIT_FAILURE);
        }
 
        for(b=0;b<width;b++) {
            if(!(zmap[a][b] = calloc(height, sizeof(int)))) {
                fprintf(stderr, "out of memory");
                exit(EXIT_FAILURE);
            }
        }
    }
}

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

void fb_setvt(void) {
	int vtno = -1;
    struct vt_stat vts;
    char vtname[12];

	int cur_tty = open("/dev/tty0", O_RDWR);
	if(cur_tty == -1) {
		logprintf(LOG_ERR, "could not open /dev/tty0");
		exit(EXIT_FAILURE);
	}

	if(ioctl(cur_tty, VT_GETSTATE, &vts) == -1) {
		logprintf(LOG_ERR, "VT_GETSTATE failed on /dev/tty0");
		exit(EXIT_FAILURE);
	}

	orig_vt_no = vts.v_active;

	if(ioctl(cur_tty, VT_OPENQRY, &vtno) == -1) {
		logprintf(LOG_ERR, "no open ttys available");
	}

	if(close(cur_tty) == -1) {
		logprintf(LOG_ERR, "could not close parent tty");
		exit(EXIT_FAILURE);
	}

    if(vtno < 0) {
		if(ioctl(tty, VT_OPENQRY, &vtno) == -1 || vtno == -1) {
			logprintf(LOG_ERR, "ioctl VT_OPENQRY");
			exit(EXIT_FAILURE);
		}
    }

    vtno &= 0xff;
    sprintf(vtname, "/dev/tty%d", vtno);
    chown(vtname, getuid(), getgid());
    if(access(vtname, R_OK | W_OK) == -1) {
		logprintf(LOG_ERR, "access %s: %s\n", vtname,strerror(errno));
		exit(EXIT_FAILURE);
    }

    close(tty);
    close(0);
    close(1);
    close(2);

	setsid();
	open(vtname, O_RDWR);
	dup(0);
    dup(0);

    if(ioctl(tty, VT_GETSTATE, &vts) == -1) {
		logprintf(LOG_ERR, "ioctl VT_GETSTATE");
		exit(EXIT_FAILURE);
    }

    orig_vt_no = vts.v_active;

	if(ioctl(tty, VT_ACTIVATE, vtno) == -1) {
		logprintf(LOG_ERR, "ioctl VT_ACTIVATE");
		exit(EXIT_FAILURE);
    }

    if(ioctl(tty, VT_WAITACTIVE, vtno) == -1) {
		logprintf(LOG_ERR, "ioctl VT_WAITACTIVE");
		exit(EXIT_FAILURE);
    }
}

void fb_cleanup(void) {
    if(ioctl(tty, KDSETMODE, kd_mode) == -1)
		logprintf(LOG_ERR, "ioctl KDSETMODE");

    if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &ovinfo) == -1)
		logprintf(LOG_ERR, "ioctl FBIOPUT_VSCREENINFO");

    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
		logprintf(LOG_ERR, "ioctl FBIOGET_FSCREENINFO");

	if(ovinfo.bits_per_pixel == 8 || finfo.visual == FB_VISUAL_DIRECTCOLOR) {
		if(ioctl(fbfd, FBIOPUTCMAP, &ocmap) == -1)
			logprintf(LOG_ERR, "ioctl FBIOPUTCMAP");
	}
	close(fbfd);

    if(orig_vt_no && ioctl(tty, VT_ACTIVATE, orig_vt_no) == -1)
		logprintf(LOG_ERR, "ioctl VT_ACTIVATE");

    if(orig_vt_no && ioctl(tty, VT_WAITACTIVE, orig_vt_no) == -1)
		logprintf(LOG_ERR, "ioctl VT_WAITACTIVE");

    tcsetattr(tty, TCSANOW, &term);
    close(tty);
}

void fb_clear_mem(void) {
	fb_memset(fbp, 0, finfo.smem_len);
}

void fb_clear_screen(void) {
	fb_memset(fbp, 0, finfo.line_length * vinfo.yres);
}

int fb_gc(void) {
	fb_clear_mem();
	fb_clear_screen();
	fb_cleanup();
	sfree((void *)&zmap);
	close(fbfd);
	return EXIT_SUCCESS;
}

void fb_init(void) {

	unsigned long page_mask;
    struct vt_stat vts;
	
	fb_setvt();
	
	if(ioctl(tty, VT_GETSTATE, &vts) == -1) {
		logprintf(LOG_ERR, "ioctl VT_GETSTATE: %s (not a linux console?)\n", strerror(errno));
		exit(EXIT_FAILURE);
    }
	
	fbfd = open(DEFAULT_FB, O_RDWR);
	if (fbfd == -1) {
        logprintf(LOG_ERR, "cannot open framebuffer device");
        exit(EXIT_FAILURE);
    }	

    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &ovinfo) == -1) {
        logprintf(LOG_ERR, "cannnot read variable screen information");
        exit(EXIT_FAILURE);
    }

    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        logprintf(LOG_ERR, "cannot read fixed screen information");
        exit(EXIT_FAILURE);
    }		
	
	if(ovinfo.bits_per_pixel == 8 || finfo.visual == FB_VISUAL_DIRECTCOLOR) {
		if(ioctl(fbfd, FBIOGETCMAP, &ocmap) == -1) {
			logprintf(LOG_ERR, "ioctl FBIOGETCMAP");
			exit(EXIT_FAILURE);
		}
    }
	
	if(ioctl(tty, KDGETMODE, &kd_mode) == -1) {
		logprintf(LOG_ERR, "ioctl KDGETMODE");
		exit(EXIT_FAILURE);
	}
    if(ioctl(tty, VT_GETMODE, &vt_omode) == -1) {
		logprintf(LOG_ERR, "ioctl VT_GETMODE");
		exit(EXIT_FAILURE);
    }

    tcgetattr(tty, &term);

    if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		logprintf(LOG_ERR,"ioctl FBIOGET_VSCREENINFO");
		exit(EXIT_FAILURE);
    }
 
    if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		logprintf(LOG_ERR, "ioctl FBIOGET_FSCREENINFO");
		exit(EXIT_FAILURE);
    }

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	page_mask = (unsigned long)getpagesize()-1;
	fb_mem_offset = (unsigned long)(finfo.smem_start) & page_mask;
    fbp = (char *)mmap(0, finfo.smem_len+fb_mem_offset, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

	if((int)fbp == -1) {
        logprintf(LOG_ERR, "failed to map framebuffer device to memory");
        exit(EXIT_FAILURE);
    }
 
	if(ioctl(tty, KDSETMODE, KD_GRAPHICS) == -1) {
		logprintf(LOG_ERR, "ioctl KDSETMODE");
		fb_cleanup();
		exit(EXIT_FAILURE);
    }

	fb_memset(fbp+fb_mem_offset, 0, finfo.line_length * vinfo.yres);
	
	fb_init_zmap();
}

unsigned long fb_get_location(int x, int y) {
	return ((unsigned long)x+(unsigned long)vinfo.xoffset) * (vinfo.bits_per_pixel/8) + ((unsigned long)y+(unsigned long)vinfo.yoffset) * finfo.line_length;
}

void fb_rm_pixel(int x, int y, int z, unsigned int color) {
	int a = 0;
	unsigned long location = fb_get_location(x, y);

	if(z > -1) {
		if(z > ZINDEXES) {
			logprintf(LOG_ERR, "z-index out of range");
			exit(EXIT_FAILURE);
		}

		zmap[z][x][y] = 0;

		for(a=z-1;a>=0;--a) {
			if(zmap[a][x][y] > 0) {
				color = zmap[a][x][y];
				break;
			}
		}
	}

	if((fbp + location)) 
        {
                if(vinfo.bits_per_pixel == 16)
                        *((unsigned short*)((unsigned long)fbp + location)) = (unsigned short) color & 0xFFFF;
                else
                        *((unsigned int*)((unsigned long)fbp + location)) = color;
        }
}


void fb_put_pixel(int x, int y, int z, unsigned int color) {
	int a = 0;
	unsigned long location = fb_get_location(x, y);

	if(color == 0) {
		color = 1;
	}


	if(z > -1) {
		if(z > ZINDEXES) {
			logprintf(LOG_ERR, "z-index out of range");
			exit(EXIT_FAILURE);
		}
		zmap[z][x][y] = color;

		if(ZINDEXES > z) {
			for(a=ZINDEXES-1;a>=0;--a) {
				if(zmap[a][x][y] > 0) {
					color = zmap[a][x][y];
					break;
				}
			}
		}
	}

	if((fbp + location)) 
        {
                if(vinfo.bits_per_pixel == 16)
                        *((unsigned short*)((unsigned long)fbp + location)) = (unsigned short) color & 0xFFFF;
                else
                        *((unsigned int*)((unsigned long)fbp + location)) = color;
        }
}

unsigned long fb_width2px(int i) {
	return ((unsigned long)i+(unsigned long)vinfo.xoffset)*(vinfo.bits_per_pixel/8);
}

unsigned long fb_height2px(int i) {
	return ((unsigned long)i+(unsigned long)vinfo.yoffset)*finfo.line_length;
}

unsigned short fb_width(void) {
	return (unsigned short)vinfo.xres;
}

unsigned short fb_height(void) {
	return (unsigned short)vinfo.yres;
}
unsigned short fb_bpp(void) {
	return (unsigned short)vinfo.bits_per_pixel;
}
unsigned short fb_roffset(void) {
	return (unsigned short)vinfo.red.offset;
}
unsigned short fb_goffset(void) {
	return (unsigned short)vinfo.green.offset;
}
unsigned short fb_boffset(void) {
	return (unsigned short)vinfo.blue.offset;
}
