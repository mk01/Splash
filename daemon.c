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
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <ctype.h>
#define __USE_GNU
#include <pthread.h>

#include "common.h"
#include "config.h"
#include "gc.h"
#include "log.h"
#include "options.h"
#include "socket.h"
#include "json.h"
#include "fb.h"
#include "draw.h"
#include "template.h"
#include "fcache.h"

/* The pid_file and pid of this daemon */
char *pid_file;
pid_t pid;
int nodaemon = 0;
int running = 1;
char *prevMessage = NULL;
char *template_file = NULL;
pthread_t pth;
pthread_mutex_t progress_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_cond_t progress_signal = PTHREAD_COND_INITIALIZER;

int black = 0;
unsigned short draw_loop = 0;
unsigned short main_loop = 1;
int speed = 5000;
int update = 0;
int infinite = 0;

typedef struct progress_t {
	int background_x;
	int background_y;
	int background_z;
	int background_w;
	int background_h;
	char background_e[4];
	char background_img[1024];
	int active_start_w;
	int active_start_h;
	char active_start_e[4];
	char active_start_img[1024];
	int active_fill_w;
	int active_fill_h;
	char active_fill_e[4];
	char active_fill_img[1024];
	int active_end_w;
	int active_end_h;
	char active_end_e[4];
	char active_end_img[1024];
	int inactive_start_w;
	int inactive_start_h;
	char inactive_start_e[4];
	char inactive_start_img[1024];
	int inactive_fill_w;
	int inactive_fill_h;
	char inactive_fill_e[4];
	char inactive_fill_img[1024];
	int inactive_end_w;
	int inactive_end_h;
	char inactive_end_e[4];
	char inactive_end_img[1024];
} progress_t;

struct progress_t progress;

int progress_active = 0;
int percentage = 0;

void main_draw_shape(struct template_t *tmp_tpl) {
	char *sval = NULL;
	if(template_get_setting_string(tmp_tpl, "shape", &sval) == 0) {
		int filled = 0, x = 0, y = 0, width = fb_width(), height = fb_height(), border = 0, fcolor = 0;
		int x1 = 0, x2 = 0, y1 = 0, y2 = 0, radius = 0, thickness = 1, zindex = 0;
		unsigned short *color = NULL;

		if(strcmp(sval, "rectangle") == 0 || strcmp(sval, "circle") == 0) {
			template_get_setting_number(tmp_tpl, "x", &x);
			template_get_setting_number(tmp_tpl, "y", &y);
			template_get_setting_number(tmp_tpl, "border", &border);
			template_get_setting_number(tmp_tpl, "filled", &filled);
		} else if(strcmp(sval, "line") == 0) {
			template_get_setting_number(tmp_tpl, "x1", &x1);
			template_get_setting_number(tmp_tpl, "x2", &x2);
			template_get_setting_number(tmp_tpl, "y1", &y1);
			template_get_setting_number(tmp_tpl, "y2", &y2);
			template_get_setting_number(tmp_tpl, "thickness", &thickness);
		}
		if(strcmp(sval, "rectangle") == 0) {
			template_get_setting_number(tmp_tpl, "width", &width);
			template_get_setting_number(tmp_tpl, "height", &height);
		} else if(strcmp(sval, "circle") == 0) {
			template_get_setting_number(tmp_tpl, "radius", &radius);
		}

		if(template_get_setting_color(tmp_tpl, "color", &color) != 0) {
			color = malloc(sizeof(unsigned short)*3);
			color[0] = 0, color[1] = 0, color[2] = 0;
			fcolor = 1;
		}

		template_get_setting_number(tmp_tpl, "z-index", &zindex);

		if(nodaemon == 0) {
			if(strcmp(sval, "rectangle") == 0) {
				if(filled) {
					draw_rectangle_filled(x, y, zindex, width, height, draw_color(color[0], color[1], color[2]));
				} else {
					draw_rectangle(x, y, zindex, width, height, border, draw_color(color[0], color[1], color[2]));
				}
			} else if(strcmp(sval, "circle") == 0) {
				if(filled) {
					draw_circle_filled(x, y, zindex, radius, draw_color(color[0], color[1], color[2]));
				} else {
					draw_circle(x, y, zindex, radius, border, draw_color(color[0], color[1], color[2]));
				}
			} else if(strcmp(sval, "line") == 0) {
				draw_line(x1, y1, x2, y2, zindex, thickness, draw_color(color[0], color[1], color[2]));
			}
		}
		if(fcolor) {
			free(color);
		}
	}
}

void main_draw_image(struct template_t *tmp_tpl) {
	int x = 0, y = 0, zindex = 0;
	char *image = NULL, *ext = NULL;
	template_get_setting_number(tmp_tpl, "x", &x);
	template_get_setting_number(tmp_tpl, "y", &y);
	template_get_setting_number(tmp_tpl, "z-index", &zindex);
	template_get_setting_string(tmp_tpl, "image", &image);
	template_get_setting_string(tmp_tpl, "extension", &ext);

	if(nodaemon == 0) {
		if(strcmp(ext, "jpg") == 0) {
			draw_jpg(x, y, zindex, image);
		} else if(strcmp(ext, "png") == 0) {
			draw_png(x, y, zindex, image);
		}
	}
}

void main_clear_text(struct template_t *tmp_tpl) {
	int x = 0, y = 0, size = 0, spacing = 0, zindex = 0;
	int align = ALIGN_RIGHT, decoration = NONE;
	char *font = NULL;

	template_get_setting_number(tmp_tpl, "x", &x);
	template_get_setting_number(tmp_tpl, "y", &y);
	template_get_setting_number(tmp_tpl, "spacing", &spacing);
	template_get_setting_number(tmp_tpl, "size", &size);
	template_get_setting_number(tmp_tpl, "z-index", &zindex);
	template_get_setting_string(tmp_tpl, "font", &font);
	template_get_setting_number(tmp_tpl, "align", &align);
	template_get_setting_number(tmp_tpl, "decoration", &decoration);

	if(strlen(prevMessage) > 0) {
		if(nodaemon == 0) {
			draw_rmtxt(x, y, zindex, font, (FT_UInt)size, prevMessage, 0, spacing, decoration, align);
		}
		memset(prevMessage, '\0', strlen(prevMessage));
	}
}

void main_draw_text(struct template_t *tmp_tpl, char *ntext) {
	int x = 0, y = 0, size = 0, spacing = 0, ftext = 0, fcolor = 0, zindex = 0;
	int align = ALIGN_RIGHT, decoration = NONE;
	unsigned short *color = NULL;
	char *font = NULL, *text = NULL;

	template_get_setting_number(tmp_tpl, "x", &x);
	template_get_setting_number(tmp_tpl, "y", &y);
	template_get_setting_number(tmp_tpl, "spacing", &spacing);
	template_get_setting_number(tmp_tpl, "size", &size);
	template_get_setting_number(tmp_tpl, "z-index", &zindex);
	template_get_setting_string(tmp_tpl, "font", &font);
	if(!ntext) {
		template_get_setting_string(tmp_tpl, "text", &text);
	} else {
		text = malloc(strlen(ntext)+1);
		strcpy(text, ntext);
		ftext = 1;
	}

	template_get_setting_number(tmp_tpl, "align", &align);
	template_get_setting_number(tmp_tpl, "decoration", &decoration);

	if(template_get_setting_color(tmp_tpl, "color", &color) != 0) {
		color = malloc(sizeof(unsigned short)*3);
		color[0] = 255, color[1] = 255, color[2] = 255;
		fcolor = 1;
	}

	if(text) {
		prevMessage = realloc(prevMessage, strlen(text)+1);
		strcpy(prevMessage, text);
		if(nodaemon == 0) {
			draw_txt(x, y, zindex, font, (FT_UInt)size, text, draw_color(color[0], color[1], color[2]), spacing, decoration, align);
		}
	}

	if(fcolor) {
		free(color);
	}
	if(ftext) {
		free(text);
	}
}

void main_draw_black(void) {
	if(nodaemon == 0) {
		draw_rectangle_filled(0, 0, -1, fb_width(), fb_height(), draw_color(0, 0, 0));
	}
}

void *update_progress(void *param) {
	pthread_mutex_lock(&progress_lock);
	int last_percentage = 0;
	int new_percentage = -1;
	int n = 0;

	while(main_loop) {
		int target_width = 0;
		int x = 0, y = 0;

		if(percentage == 100 || percentage == -1 || percentage == 0) {
			target_width = (int)progress.background_w;
		} else {
			target_width = (int)(progress.background_w * (((double)percentage/100)));
		}

		if(draw_loop) {
			while(draw_loop) {
				pthread_mutex_lock(&progress_lock);
				if(target_width > 0 && nodaemon == 0 && black == 0) {
					if(progress_active == 0) {
						if(strcmp(progress.active_start_e, "jpg") == 0) {
							draw_jpg(progress.background_x, progress.background_y, progress.background_z, progress.active_start_img);
						} else if(strcmp(progress.active_start_e, "png") == 0) {
							draw_png(progress.background_x, progress.background_y, progress.background_z, progress.active_start_img);
						}
					} else {
						if(strcmp(progress.inactive_start_e, "jpg") == 0) {
							draw_jpg(progress.background_x, progress.background_y, progress.background_z, progress.inactive_start_img);
						} else if(strcmp(progress.inactive_start_e, "png") == 0) {
							draw_png(progress.background_x, progress.background_y, progress.background_z, progress.inactive_start_img);
						}
					}
				}
				if(progress_active == 0) {
					x += progress.active_start_w;
				} else {
					x += progress.inactive_start_w;
				}

				if(progress_active == 0) {
					if(target_width >= (progress.background_w-progress.active_end_w)) {
						y = (int)(progress.background_w-progress.inactive_end_w);
					} else {
						y = (int)target_width;
					}
				} else {
					if(target_width >= (progress.background_w-progress.inactive_end_w)) {
						y = (int)(progress.background_w-progress.inactive_end_w);
					} else {
						y = (int)target_width;
					}
				}

				if(x < y) {
					while(x < y) {
						if(nodaemon == 0 && black == 0) {
							if(progress_active == 0) {
								if(strcmp(progress.active_fill_e, "jpg") == 0) {
									draw_jpg(progress.background_x+x, progress.background_y, progress.background_z, progress.active_fill_img);
								} else if(strcmp(progress.active_fill_e, "png") == 0) {
									draw_png(progress.background_x+x, progress.background_y, progress.background_z, progress.active_fill_img);
								}
								if(percentage == -1) {
									usleep((__useconds_t)(speed*progress.active_fill_w));
								}
							} else {
								if(strcmp(progress.inactive_fill_e, "jpg") == 0) {
									draw_jpg(progress.background_x+x, progress.background_y, progress.background_z, progress.inactive_fill_img);
								} else if(strcmp(progress.inactive_fill_e, "png") == 0) {
									draw_png(progress.background_x+x, progress.background_y, progress.background_z, progress.inactive_fill_img);
								}
								if(percentage == -1) {
									usleep((__useconds_t)(speed*progress.inactive_fill_w));
								}
							}
						}
						x++;
					}
				}

				if(x < target_width) {
					if(nodaemon == 0 && black == 0) {
						if(progress_active == 0) {
							if(strcmp(progress.active_end_e, "jpg") == 0) {
								draw_jpg(progress.background_x+x, progress.background_y, progress.background_z, progress.active_end_img);
							} else if(strcmp(progress.active_end_e, "png") == 0) {
								draw_png(progress.background_x+x, progress.background_y, progress.background_z, progress.active_end_img);
							}
						} else {
							if(strcmp(progress.inactive_end_e, "jpg") == 0) {
								draw_jpg(progress.background_x+x, progress.background_y, progress.background_z, progress.inactive_end_img);
							} else if(strcmp(progress.inactive_end_e, "png") == 0) {
								draw_png(progress.background_x+x, progress.background_y, progress.background_z, progress.inactive_end_img);
							}
						}
						x = (int)target_width;
					}
				}
				if(x == progress.background_w) {
					progress_active ^= 1;
					x = 0;
				}
				if(infinite == 0) {
					if(new_percentage > -1) {
						percentage = new_percentage;
						new_percentage = -1;
						target_width = (int)(progress.background_w * (((double)percentage/100)));
						x = 0;
					} else if(percentage < last_percentage && last_percentage != 100) {
						new_percentage = percentage;
						target_width = (int)progress.background_w;
						x = 0;
					} else {
						draw_loop = 0;
					}
					last_percentage = percentage;
				}
				pthread_mutex_unlock(&progress_lock);
			}
		} else {
			if(percentage > 0 && percentage < 100) {
				struct timeval tp;
				struct timespec ts;

				gettimeofday(&tp, NULL);
				ts.tv_sec = tp.tv_sec;
				ts.tv_nsec = tp.tv_usec * 1000;
				ts.tv_sec += 1;

				n = pthread_cond_timedwait(&progress_signal, &progress_lock, &ts);
				if(n == ETIMEDOUT) {
					percentage++;
					draw_loop = 1;
				}
			} else {
				n = pthread_cond_wait(&progress_signal, &progress_lock);
			}
		}
	}
	return (void *)NULL;
}

void main_draw(void) {
	struct template_t *tmp_tpl = template;
	char *image = NULL, *ext = NULL;

	while(tmp_tpl != NULL) {
		switch(tmp_tpl->type) {
			case SHAPE:
				main_draw_shape(tmp_tpl);
			break;
			case PROGRESS: {
				template_get_setting_number(tmp_tpl, "x", &progress.background_x);
				template_get_setting_number(tmp_tpl, "y", &progress.background_y);
				template_get_setting_number(tmp_tpl, "z-index", &progress.background_z);
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.background_w = jpg_get_width(image);
						progress.background_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.background_w = png_get_width(image);
						progress.background_h = png_get_height(image);
					}
					strcpy(progress.background_img, image);
				}
				strcpy(progress.background_e, ext);
			}
			case IMAGE:
				main_draw_image(tmp_tpl);
			break;
			case TEXT:
				if(strlen(prevMessage) > 0) {
					main_clear_text(tmp_tpl);
				}
				main_draw_text(tmp_tpl, NULL);
			break;
			case PROGRESS_INACTIVE_START:
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.inactive_start_w = jpg_get_width(image);
						progress.inactive_start_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.inactive_start_w = png_get_width(image);
						progress.inactive_start_h = png_get_height(image);
					}
					strcpy(progress.inactive_start_img, image);
				}
				strcpy(progress.inactive_start_e, ext);
			break;
			case PROGRESS_INACTIVE_FILL:
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.inactive_fill_w = jpg_get_width(image);
						progress.inactive_fill_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.inactive_fill_w = png_get_width(image);
						progress.inactive_fill_h = png_get_height(image);
					}
					strcpy(progress.inactive_fill_img, image);
				}
				strcpy(progress.inactive_fill_e, ext);
			break;
			case PROGRESS_INACTIVE_END:
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.inactive_end_w = jpg_get_width(image);
						progress.inactive_end_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.inactive_end_w = png_get_width(image);
						progress.inactive_end_h = png_get_height(image);
					}
					strcpy(progress.inactive_end_img, image);
				}
				strcpy(progress.inactive_end_e, ext);
			break;
			case PROGRESS_ACTIVE_START:
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.active_start_w = jpg_get_width(image);
						progress.active_start_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.active_start_w = png_get_width(image);
						progress.active_start_h = png_get_height(image);
					}
					strcpy(progress.active_start_img, image);
				}
				strcpy(progress.active_start_e, ext);
			break;
			case PROGRESS_ACTIVE_FILL:
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.active_fill_w = jpg_get_width(image);
						progress.active_fill_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.active_fill_w = png_get_width(image);
						progress.active_fill_h = png_get_height(image);
					}
					strcpy(progress.active_fill_img, image);
				}
				strcpy(progress.active_fill_e, ext);
			break;
			case PROGRESS_ACTIVE_END:
				template_get_setting_string(tmp_tpl, "extension", &ext);
				if(template_get_setting_string(tmp_tpl, "image", &image) == 0) {
					if(strcmp(ext, "jpg") == 0) {
						progress.active_end_w = jpg_get_width(image);
						progress.active_end_h = jpg_get_height(image);
					} else if(strcmp(ext, "png") == 0) {
						progress.active_end_w = png_get_width(image);
						progress.active_end_h = png_get_height(image);
					}
					strcpy(progress.active_end_img, image);
				}
				strcpy(progress.active_end_e, ext);
			break;
			default:
			break;
		}
		tmp_tpl = tmp_tpl->next;
	}
}

/* Parse the incoming buffer from the client */
void socket_parse_data(int i, char buffer[BUFFER_SIZE]) {
	int sd = socket_clients[i];
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	char *message = NULL;
	JsonNode *json = json_decode(buffer);

	getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

	if(json_find_string(json, "black", &message) == 0) {
		if(black == 0) {
			black = 1;
			draw_loop = 0;
			usleep((__useconds_t)speed);
			main_draw_black();
		} else {
			main_draw();
			black = 0;
		}
	} else {
		if(json_find_string(json, "percentage", &message) == 0) {
			if(black == 1) {
				progress_active = 0;
				main_draw();
				black = 0;
			}
			draw_loop = 0;
			infinite = 0;
			pthread_mutex_lock(&progress_lock);
			percentage = atoi(message);
			draw_loop = 1;
			pthread_mutex_unlock(&progress_lock);
			pthread_cond_signal(&progress_signal);
		} else if(json_find_string(json, "infinite", &message) == 0) {
			if(black == 1) {
				progress_active = 0;
				main_draw();
				black = 0;
			}
			draw_loop = 0;
			infinite = 1;
			pthread_mutex_lock(&progress_lock);
			draw_loop = 1;
			percentage = -1;
			pthread_mutex_unlock(&progress_lock);
			pthread_cond_signal(&progress_signal);
		}
		if(json_find_string(json, "message", &message) == 0) {
			struct template_t *tmp_tpl = template;
			while(tmp_tpl) {
				if(tmp_tpl->type == TEXT) {
					break;
				}
				tmp_tpl = tmp_tpl->next;
			}
			if(black == 1) {
				main_draw();
				black = 0;
			}
			if(strlen(prevMessage) > 0) {
				main_clear_text(tmp_tpl);
			}
			main_draw_text(tmp_tpl, message);
		}
	}
	json_delete(json);
}

void save_pid(pid_t npid) {
	int f = 0;
	char buffer[BUFFER_SIZE];
	if((f = open(pid_file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) != -1) {
		lseek(f, 0, SEEK_SET);
		sprintf(buffer, "%d", npid);
		int i = write(f, buffer, strlen(buffer));
		if(i != strlen(buffer)) {
			logprintf(LOG_ERR, "could not store pid in %s", pid_file);
		}
	}
	close(f);
}

void deamonize(void) {
	log_file_enable();
	log_shell_disable();
	/* Get the pid of the fork */
	pid_t npid = fork();
	switch(npid) {
		case 0:
		break;
		case -1:
			logprintf(LOG_ERR, "could not daemonize program");
			exit(1);
		break;
		default:
			save_pid(npid);
			logprintf(LOG_INFO, "daemon started with pid: %d", npid);
			exit(0);
		break;
	}
}

/* Garbage collector of main program */
int main_gc(void) {

	draw_loop = 0;
	main_loop = 0;

	if(pth) {
		pthread_cancel(pth);
		pthread_join(pth, NULL);
	}

	if(nodaemon == 0) {
		fb_gc();
	}

	if(running == 0) {
		/* Remove the stale pid file */
		if(access(pid_file, F_OK) != -1) {
			if(remove(pid_file) != -1) {
				logprintf(LOG_DEBUG, "removed stale pid_file %s", pid_file);
			} else {
				logprintf(LOG_ERR, "could not remove stale pid file %s", pid_file);
			}
		}
	}

	template_gc();
	options_gc();
	fcache_gc();
	socket_gc();

	free(progname);
	free(prevMessage);
	free(pid_file);
	free(template_file);

	return 0;
}

int main(int argc, char **argv) {
	/* Run main garbage collector when quiting the daemon */
	gc_attach(main_gc);

	/* Catch all exit signals for gc */
	gc_catch();

	loglevel = LOG_INFO;

	log_file_disable();
	log_shell_enable();

	char *logfile = malloc(strlen(LOG_FILE)+1);
	strcpy(logfile, LOG_FILE);
	log_file_set(logfile);
	sfree((void *)&logfile);

	prevMessage = malloc(4);
	memset(prevMessage, '\0', 4);

	progname = malloc(14);
	strcpy(progname, "splash-daemon");

	struct socket_callback_t socket_callback;
	struct options_t *options = NULL;
	char *args = NULL;
	char buffer[BUFFER_SIZE];
	int f;

	options_add(&options, 'H', "help", no_value, 0, NULL);
	options_add(&options, 'V', "version", no_value, 0, NULL);
	options_add(&options, 'D', "nodaemon", no_value, 0, NULL);

	while (1) {
		int c;
		c = options_parse(&options, argc, argv, 1, &args);
		if (c == -1)
			break;
		switch(c) {
			case 'H':
				printf("Usage: %s [options]\n", progname);
				printf("\t -H --help\t\tdisplay usage summary\n");
				printf("\t -V --version\t\tdisplay version\n");
				printf("\t -S --settings\t\tsettings file\n");
				printf("\t -D --nodaemon\t\tdo not daemonize and\n");
				printf("\t\t\t\tshow debug information\n");
				return (EXIT_SUCCESS);
			break;
			case 'V':
				printf("%s %s\n", progname, "1.0");
				return (EXIT_SUCCESS);
			break;
			case 'D':
				nodaemon=1;
			break;
			default:
				printf("Usage: %s [options]\n", progname);
				return (EXIT_FAILURE);
			break;
		}
	}
	options_delete(options);

	pid_file = malloc(sizeof(PID_FILE)+1);
	strcpy(pid_file, PID_FILE);

	template_file = malloc(14);
	strcpy(template_file, "template.json");

	if((f = open(pid_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) != -1) {
		if(read(f, buffer, BUFFER_SIZE) != -1) {
			//If the file is empty, create a new process
			if(!atoi(buffer)) {
				running = 0;
			} else {
				//Check if the process is running
				kill(atoi(buffer), 0);
				//If not, create a new process
				if(errno == ESRCH) {
					running = 0;
				}
			}
		}
	} else {
		logprintf(LOG_ERR, "could not open / create pid_file %s", pid_file);
		return EXIT_FAILURE;
	}
	close(f);

	if(nodaemon == 1 || running == 1) {
		log_level_set(LOG_DEBUG);
	}

	if(running == 1) {
		nodaemon=1;
		logprintf(LOG_NOTICE, "already active (pid %d)", atoi(buffer));
		return EXIT_FAILURE;
	}

	if(nodaemon == 0) {
		deamonize();
		socket_start(PORT);
		fb_init();
	} else {
		socket_start(PORT);
	}

	if(template_read(template_file) == EXIT_FAILURE) {
		logprintf(LOG_NOTICE, "failed to read template file %s", template_file);
		main_gc();
		return EXIT_FAILURE;
	}

	if(nodaemon == 1) {
		//template_print();
	}

    //initialise all socket_clients and handshakes to 0 so not checked
	memset(socket_clients, 0, sizeof(socket_clients));

    socket_callback.client_disconnected_callback = NULL;
    socket_callback.client_connected_callback = NULL;
    socket_callback.client_data_callback = &socket_parse_data;

	main_draw();

	/* Make sure the server part is non-blocking by creating a new thread */
	pthread_create(&pth, NULL, &update_progress, (void *)NULL);

	socket_wait((void *)&socket_callback);

	while(main_loop) {
		sleep(1);
	}

	return EXIT_FAILURE;
}
