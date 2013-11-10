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
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "common.h"
#include "template.h"
#include "config.h"
#include "log.h"
#include "socket.h"
#include "draw.h"
#include "fb.h"

/* http://stackoverflow.com/a/13654646 */
void template_reverse_struct(struct template_t **tpl) {
    if(!tpl || !*tpl)
        return;

    struct template_t *tptr = *tpl, *tnext = NULL, *tprev = NULL;
    struct tpl_settings_t *sptr = NULL, *snext = NULL, *sprev = NULL;

    while(tptr) {

		sptr = tptr->settings;
		while(sptr != NULL) {
			snext = sptr->next;
			sptr->next = sprev;
			sprev = sptr;
			sptr = snext;
		}
		tptr->settings = sprev;
		sptr = NULL;
		sprev = NULL;
		snext = NULL;

        tnext = tptr->next;
        tptr->next = tprev;
        tprev = tptr;
        tptr = tnext;
    }
	free(tptr);
	free(tnext);
	free(sptr);
	free(snext);
    *tpl = tprev;
}

void template_print(void) {
	struct template_t *tmp_tpl = template;
	struct tpl_settings_t *snode = NULL;
	while(tmp_tpl) {
		printf("%s %d\n", tmp_tpl->name, tmp_tpl->type);
		snode = tmp_tpl->settings;
		while(snode) {
			if(snode->type == STRING) {
				printf("\t%s %s\n", snode->name, snode->value->string_);
			} else if(snode->type == NUMBER) {
				printf("\t%s %d\n", snode->name, snode->value->number_);
			}  else if(snode->type == COLOR) {
				printf("\t%s (%d %d %d)\n", snode->name, snode->value->color_[0], snode->value->color_[1], snode->value->color_[2]);
			}
			snode = snode->next;
		}
		tmp_tpl = tmp_tpl->next;
	}
}

int template_get_setting_string(struct template_t *tnode, const char *name, char **out) {
	struct tpl_settings_t *snode = NULL;
	snode = tnode->settings;
	while(snode) {
		if(strcmp(snode->name, name) == 0 && snode->type == STRING) {
			*out = snode->value->string_;
			return 0;
		}
		snode = snode->next;
	}
	return -1;
}

int template_get_setting_number(struct template_t *tnode, const char *name, int *out) {
	struct tpl_settings_t *snode = NULL;
	snode = tnode->settings;
	while(snode) {
		if(strcmp(snode->name, name) == 0 && snode->type == NUMBER) {
			*out = snode->value->number_;
			return 0;
		}
		snode = snode->next;
	}
	return -1;
}

int template_get_setting_color(struct template_t *tnode, const char *name, unsigned short **out) {
	struct tpl_settings_t *snode = NULL;
	snode = tnode->settings;
	while(snode) {
		if(strcmp(snode->name, name) == 0 && snode->type == COLOR) {
			*out = snode->value->color_;
			return 0;
		}
		snode = snode->next;
	}
	return -1;
}

void template_add_setting_string(struct template_t *tnode, const char *name, char *val) {
	struct tpl_settings_t *snode = malloc(sizeof(struct tpl_settings_t));
	snode->name = malloc(strlen(name)+1);
	strcpy(snode->name, name);
	snode->value = malloc(sizeof(union tpl_value_t));
	snode->value->string_ = malloc(strlen(val)+1);
	strcpy(snode->value->string_, val);
	snode->type = STRING;
	snode->next = tnode->settings;
	tnode->settings = snode;

}

void template_add_setting_number(struct template_t *tnode, const char *name, int val) {
	struct tpl_settings_t *snode = malloc(sizeof(struct tpl_settings_t));
	snode->name = malloc(strlen(name)+1);
	strcpy(snode->name, name);	
	snode->value = malloc(sizeof(union tpl_value_t));
	snode->value->number_ = val;
	snode->type = NUMBER;
	snode->next = tnode->settings;
	tnode->settings = snode;
}

void template_add_setting_color(struct template_t *tnode, const char *name, unsigned short *val) {
	struct tpl_settings_t *snode = malloc(sizeof(struct tpl_settings_t));
	snode->name = malloc(strlen(name)+1);	
	strcpy(snode->name, name);
	snode->value = malloc(sizeof(union tpl_value_t));
	snode->value->color_ = val;
	snode->type = COLOR;
	snode->next = tnode->settings;
	tnode->settings = snode;
}

void template_parse_color(JsonNode *root, unsigned short *color) {
	JsonNode *child = json_first_child(root);
	int i = 0;
	while(child != NULL) {
		color[i] = (unsigned short)child->number_;
		i++;
		child = child->next;
	}
}

void template_add_progress_elements(char *name, char *image, int zindex, enum tpl_type_t type) {
	struct template_t *tnode = malloc(sizeof(struct template_t));
	int width = 0, height = 0;
	char *newname = malloc(strlen(name)+1);
	strcpy(newname, name);
	switch(type) {
		case PROGRESS_INACTIVE_START:
			newname = realloc(newname, strlen(newname)+16);
			strcat(newname, "-inactive-start");
		break;
		case PROGRESS_INACTIVE_FILL:
			newname = realloc(newname, strlen(newname)+15);
			strcat(newname, "-inactive-fill");
		break;
		case PROGRESS_INACTIVE_END:
			newname = realloc(newname, strlen(newname)+14);
			strcat(newname, "-inactive-end");
		break;
		case PROGRESS_ACTIVE_START:
			newname = realloc(newname, strlen(newname)+14);
			strcat(newname, "-active-start");
		break;
		case PROGRESS_ACTIVE_FILL:
			newname = realloc(newname, strlen(newname)+13);
			strcat(newname, "-active-fill");
		break;
		case PROGRESS_ACTIVE_END:
			newname = realloc(newname, strlen(newname)+12);
			strcat(newname, "-active-end");
		break;
		case SHAPE:
		case IMAGE:
		case TEXT:
		case PROGRESS:
		default:
		break;
	}
	
	tnode->name = malloc(strlen(newname)+1);
	strcpy(tnode->name, newname);
	free(newname);

	char *dot = NULL;
	dot = strrchr(image, '.');
	if(!dot || dot == image) {
		logprintf(LOG_ERR, "image has got no extension");
		exit(EXIT_FAILURE);
	}

	char *ext = malloc(strlen(dot)+1);
	memset(ext, '\0', strlen(dot)+1);
	strcpy(ext, dot+1);

	if(strcmp(ext, "jpg") == 0) {
		width = jpg_get_width(image);
		height = jpg_get_height(image);
	} else if(strcmp(ext, "png") == 0) {
		width = png_get_width(image);
		height = png_get_height(image);
	}

	tnode->settings = NULL;
	
	template_add_setting_string(tnode, "image", image);
	template_add_setting_number(tnode, "width", width);
	template_add_setting_number(tnode, "height", height);
	template_add_setting_number(tnode, "z-index", zindex);
	template_add_setting_string(tnode, "extension", ext);

	tnode->type = type;
	tnode->next = template;
	template = tnode;
	
	sfree((void *)&ext);	
}

void template_add_progress(JsonNode *root) {
	struct template_t *tnode = malloc(sizeof(struct template_t));
	int x = 0, y = 0, width = 0, height = 0, percentage = 0, zindex = 0;
	char *image = NULL, *name = NULL;

	if(json_find_string(root, "name", &name) != 0) {
		logprintf(LOG_ERR, "template element misses a name");
		exit(EXIT_FAILURE);
	}
	
	tnode->name = malloc(strlen(name)+1);	
	strcpy(tnode->name, name);
	
	tnode->settings = NULL;
	
	if(json_find_string(root, "image", &image) != 0) {
		logprintf(LOG_ERR, "template element \"%s\", \"image\" missing", name);
		exit(EXIT_FAILURE);
	}

	json_find_number(root, "x", &x);
	json_find_number(root, "y", &y);
	json_find_number(root, "z-index", &zindex);
	json_find_number(root, "percentage", &percentage);
	char *dot = NULL;
	dot = strrchr(image, '.');
	if(!dot || dot == image) {
		logprintf(LOG_ERR, "image has got no extension");
		exit(EXIT_FAILURE);
	}

	char *ext = malloc(strlen(dot)+1);
	memset(ext, '\0', strlen(dot)+1);
	strcpy(ext, dot+1);

	if(strcmp(ext, "jpg") == 0) {
		width = jpg_get_width(image);
		height = jpg_get_height(image);
	} else if(strcmp(ext, "png") == 0) {
		width = png_get_width(image);
		height = png_get_height(image);
	}
	
	if(x != 0) {
		x = (fb_width()/2)+x;
	} else {
		x = (fb_width()/2)-(width/2);
	}
	
	if(y != 0) {
		y = (fb_height()/2)+y;
	} else {
		y = (fb_height()/2)-(height/2);
	}

	template_add_setting_string(tnode, "extension", ext);	
	template_add_setting_string(tnode, "image", image);
	template_add_setting_number(tnode, "x", x);
	template_add_setting_number(tnode, "y", y);
	template_add_setting_number(tnode, "width", width);
	template_add_setting_number(tnode, "height", height);
	template_add_setting_number(tnode, "percentage", percentage);
	
	tnode->type = PROGRESS;
	tnode->next = template;
	template = tnode;	

	sfree((void *)&ext);

	tnode = NULL;
	
	int a = 0;
	JsonNode *active = NULL;
	JsonNode *inactive = NULL;
	JsonNode *child = NULL;	
	
	if((active = json_find_member(root, "active")) == NULL) {
		logprintf(LOG_ERR, "template element \"%s\", \"active\" missing", name);
		exit(EXIT_FAILURE);
	}
	
	if((inactive = json_find_member(root, "inactive")) == NULL) {
		logprintf(LOG_ERR, "template element \"%s\", \"active\" missing", name);
		exit(EXIT_FAILURE);
	}
	
	child = json_first_child(active);
	while(child != NULL) {
		a++;
		child = child->next;
	}
	
	if(a != 3 && a != 1) {
		logprintf(LOG_ERR, "template element \"%s\", \"active\" must contain 1 or 3 images", name);
		exit(EXIT_FAILURE);		
	}
	
	if(a == 1) {
		template_add_progress_elements(name, json_find_element(active, 0)->string_, zindex, PROGRESS_ACTIVE_START);
		template_add_progress_elements(name, json_find_element(active, 0)->string_, zindex, PROGRESS_ACTIVE_FILL);
		template_add_progress_elements(name, json_find_element(active, 0)->string_, zindex, PROGRESS_ACTIVE_END);
	} else {
		template_add_progress_elements(name, json_find_element(active, 0)->string_, zindex, PROGRESS_ACTIVE_START);
		template_add_progress_elements(name, json_find_element(active, 1)->string_, zindex, PROGRESS_ACTIVE_FILL);
		template_add_progress_elements(name, json_find_element(active, 2)->string_, zindex, PROGRESS_ACTIVE_END);
	}	
	
	a = 0;
	child = json_first_child(inactive);
	while(child != NULL) {
		a++;
		child = child->next;
	}
	
	if(a != 3 && a != 1) {
		logprintf(LOG_ERR, "template element \"%s\", \"active\" must contain 1 or 3 images", name);
		exit(EXIT_FAILURE);		
	}
	
	if(a == 1) {
		template_add_progress_elements(name, json_find_element(inactive, 0)->string_, zindex, PROGRESS_INACTIVE_START);
		template_add_progress_elements(name, json_find_element(inactive, 0)->string_, zindex, PROGRESS_INACTIVE_FILL);
		template_add_progress_elements(name, json_find_element(inactive, 0)->string_, zindex, PROGRESS_INACTIVE_END);
	} else {
		template_add_progress_elements(name, json_find_element(inactive, 0)->string_, zindex, PROGRESS_INACTIVE_START);
		template_add_progress_elements(name, json_find_element(inactive, 1)->string_, zindex, PROGRESS_INACTIVE_FILL);
		template_add_progress_elements(name, json_find_element(inactive, 2)->string_, zindex, PROGRESS_INACTIVE_END);
	}
}

void template_add_image(JsonNode *root) {
	struct template_t *tnode = malloc(sizeof(struct template_t));
	int y = 0, x = 0, width = 0, height = 0, zindex = 0;
	char *image = NULL, *name = NULL;
	
	if(json_find_string(root, "name", &name) != 0) {
		logprintf(LOG_ERR, "template element misses a name");
		exit(EXIT_FAILURE);
	}
	
	if(json_find_string(root, "image", &image) != 0) {
		logprintf(LOG_ERR, "template element \"%s\", \"image\" missing", tnode->name);
		exit(EXIT_FAILURE);
	}
	
	json_find_number(root, "x", &x);
	json_find_number(root, "y", &y);
	json_find_number(root, "z-index", &zindex);

	char *dot = NULL;
	dot = strrchr(image, '.');
	if(!dot || dot == image) {
		logprintf(LOG_ERR, "image has got no extension");
		exit(EXIT_FAILURE);
	}

	char *ext = malloc(strlen(dot)+1);
	memset(ext, '\0', strlen(dot)+1);
	strcpy(ext, dot+1);

	if(strcmp(ext, "jpg") == 0) {
		width = jpg_get_width(image);
		height = jpg_get_height(image);
	} else if(strcmp(ext, "png") == 0) {
		width = png_get_width(image);
		height = png_get_height(image);
	}

	if(x != 0) {
		x = (fb_width()/2)+x;
	} else {
		x = (fb_width()/2)-(width/2);
	}
	
	if(y != 0) {
		y = (fb_height()/2)+y;
	} else {
		y = (fb_height()/2)-(height/2);
	}
	
	tnode->name = malloc(strlen(name)+1);
	strcpy(tnode->name, name);	
	
	tnode->settings = NULL;
	
	template_add_setting_string(tnode, "extension", ext);	
	template_add_setting_string(tnode, "image", image);
	template_add_setting_number(tnode, "x", x);
	template_add_setting_number(tnode, "y", y);
	template_add_setting_number(tnode, "width", width);
	template_add_setting_number(tnode, "height", height);
	template_add_setting_number(tnode, "z-index", zindex);
	
	tnode->type = IMAGE;
	tnode->next = template;
	template = tnode;

	sfree((void *)&ext);	
}

void template_add_text(JsonNode *root) {
	struct template_t *tnode = malloc(sizeof(struct template_t));
	int x = 0, y = 0, size = 30, spacing = 3, z = ALIGN_LEFT;
	int a = NONE, width = 0, height = 0, zindex = 0;
	char *text = NULL;
	char *name = NULL;
	char *align = NULL;
	char *decoration = NULL;
	char *font = NULL;
	unsigned short *color = malloc(sizeof(unsigned short)*3);
	
	if(json_find_string(root, "name", &name) != 0) {
		logprintf(LOG_ERR, "template element misses a name");
		exit(EXIT_FAILURE);
	}
	
	if(json_find_string(root, "font", &font) != 0) {
		logprintf(LOG_ERR, "template element \"%s\", \"font\" missing", tnode->name);
		exit(EXIT_FAILURE);
	}
	
	if(json_find_string(root, "text", &text) != 0) {
		logprintf(LOG_ERR, "template element \"%s\", \"text\" missing", tnode->name);
		exit(EXIT_FAILURE);
	}		
	
	if(json_find_member(root, "color") != NULL) {
		template_parse_color(json_find_member(root, "color"), color);
	} else {
		color[0] = 0, color[1] = 0, color[2] = 0;
	}	
	
	json_find_number(root, "x", &x);
	json_find_number(root, "y", &y);
	json_find_number(root, "spacing", &spacing);
	json_find_number(root, "size", &size);
	json_find_number(root, "z-index", &zindex);
	json_find_string(root, "text", &text);
	json_find_string(root, "decoration", &decoration);
	json_find_string(root, "align", &align);
	json_find_string(root, "font", &font);

	if(strcmp(align, "left") == 0) {
		z = ALIGN_LEFT;
	} else if(strcmp(align, "right") == 0) {
		z = ALIGN_RIGHT;
	} else if(strcmp(align, "center") == 0) {
		z = ALIGN_CENTER;
	} else {
		z = ALIGN_LEFT;
	}
	
	if(strcmp(decoration, "italic") == 0) {
		a = ITALIC;
	} else {
		a = NONE;
	}
	
	width = txt_get_width(font, text, (FT_UInt)size, spacing, a, z);
	height = txt_get_height(font, text, (FT_UInt)size, spacing, a, z);	
	
	if(x != 0) {
		x = (fb_width()/2)+x;
	} else {
		x = (fb_width()/2)-(width/2);
	}
	
	if(y != 0) {
		y = (fb_height()/2)+y;
	} else {
		y = (fb_height()/2)-(height/2);
	}

	tnode->name = malloc(strlen(name)+1);
	strcpy(tnode->name, name);
	
	tnode->settings = NULL;
	
	template_add_setting_string(tnode, "text", text);
	template_add_setting_string(tnode, "font", font);	
	template_add_setting_number(tnode, "size", size);
	template_add_setting_number(tnode, "x", x);
	template_add_setting_number(tnode, "y", y);
	template_add_setting_number(tnode, "width", width);
	template_add_setting_number(tnode, "height", height);
	template_add_setting_number(tnode, "align", z);
	template_add_setting_number(tnode, "decoration", a);
	template_add_setting_number(tnode, "spacing", spacing);
	template_add_setting_number(tnode, "z-index", zindex);
	template_add_setting_color(tnode, "color", color);
	
	tnode->type = TEXT;
	tnode->next = template;
	template = tnode;
}

void template_add_shape(JsonNode *root) {
	struct template_t *tnode = malloc(sizeof(struct template_t));
	char *name = NULL;
	char *shape = NULL;
	int filled = 0, border = 0, fullscreen = 0, y = 0, x = 0;
	int x1 = 0, x2 = 0, y1 = 0, y2 = 0, thickness = 1, zindex = 0;
	unsigned short *color = malloc(sizeof(unsigned short)*3);
	int height = 0, width = 0, radius = 0;
	
	if(json_find_string(root, "name", &name) != 0) {
		logprintf(LOG_ERR, "template element misses a name");
		exit(EXIT_FAILURE);
	}
	
	if(json_find_string(root, "shape", &shape) != 0) {
		logprintf(LOG_ERR, "template element \"%s\", \"shape\" missing", tnode->name);
		exit(EXIT_FAILURE);
	}
	
	if(!(strcmp(shape, "rectangle") == 0 || strcmp(shape, "line") == 0 || strcmp(shape, "circle") == 0)) {
		logprintf(LOG_ERR, "template element \"%s\", shape \"%s\" is not supported,\n"
						   "please choose either \"rectangle\", \"line\", or \"circle\"", tnode->name, shape);
		exit(EXIT_FAILURE);
	}
	
	json_find_number(root, "z-index", &zindex);
	json_find_number(root, "fullscreen", &fullscreen);
	if(strcmp(shape, "rectangle") == 0 || strcmp(shape, "circle") == 0) {
		json_find_number(root, "x", &x);
		json_find_number(root, "y", &y);
		json_find_number(root, "border", &border);
		json_find_number(root, "filled", &filled);
	} else if(strcmp(shape, "line") == 0) {
		json_find_number(root, "x1", &x1);
		json_find_number(root, "x2", &x2);
		json_find_number(root, "y1", &y1);
		json_find_number(root, "y2", &y2);
		json_find_number(root, "thickness", &thickness);
	}
	if(strcmp(shape, "rectangle") == 0) {
		json_find_number(root, "width", &width);
		json_find_number(root, "height", &height);
	} else if(strcmp(shape, "circle") == 0) {
		json_find_number(root, "radius", &radius);
	}

	if(json_find_member(root, "color") != NULL) {
		template_parse_color(json_find_member(root, "color"), color);
	} else {
		color[0] = 0, color[1] = 0, color[2] = 0;
	}

	if(fullscreen == 1) {
		y = 0;
		x = 0;
		if(strcmp(shape, "rectangle") == 0) {
			width = fb_width();
			height = fb_height();
		} else if(strcmp(shape, "circle") == 0) {
			radius = (int)((double)fb_height() / 2);
			radius -= 1;
		}
	}
	if(filled == 1 && border > 0) {
		logprintf(LOG_NOTICE, "template element \"%s\", \"filled\" overrides \"border\"", tnode->name);
	}
	if(fullscreen == 1 && (width > 0 || height > 0)) {
		logprintf(LOG_NOTICE, "template element \"%s\", \"fullscreen\" overrides \"width\" and \"height\"", tnode->name);
	}
	
	tnode->name = malloc(strlen(name)+1);
	strcpy(tnode->name, name);

	tnode->settings = NULL;
	
	if(strcmp(shape, "circle") == 0) {
		if(fullscreen == 1) {
			x += (int)((double)fb_width()/2) - radius;
		} else {
			x += radius;
			y += radius;
		}
	}	
	
	template_add_setting_string(tnode, "shape", shape);
	if(strcmp(shape, "rectangle") == 0 || strcmp(shape, "circle") == 0) {
		template_add_setting_number(tnode, "x", x);
		template_add_setting_number(tnode, "y", y);
		template_add_setting_number(tnode, "filled", filled);
		template_add_setting_number(tnode, "border", border);
	} else if(strcmp(shape, "line") == 0) {
		template_add_setting_number(tnode, "x1", x1);
		template_add_setting_number(tnode, "x2", x2);
		template_add_setting_number(tnode, "y1", y1);
		template_add_setting_number(tnode, "y2", y2);
		template_add_setting_number(tnode, "thickness", thickness);
	}
	if(strcmp(shape, "rectangle") == 0) {	
		template_add_setting_number(tnode, "width", width);
		template_add_setting_number(tnode, "height", height);
	}
	if(strcmp(shape, "circle") == 0) {	
		template_add_setting_number(tnode, "radius", radius);
	}
	template_add_setting_number(tnode, "z-index", zindex);
	template_add_setting_color(tnode, "color", color);
	
	tnode->type = SHAPE;
	tnode->next = template;
	template = tnode;
}

int template_gc(void) {	
	struct template_t *ttmp;
	struct tpl_settings_t *stmp;
	union tpl_value_t *vtmp;
	/* Show the parsed log file */
	while(template) {
		ttmp = template;
		while(ttmp->settings) {
			stmp = ttmp->settings;
			vtmp = stmp->value;
			if(stmp->type == STRING) {
				free(vtmp->string_);
			} else if(stmp->type == COLOR) {
				free(vtmp->color_);
			}
			free(stmp->name);
			ttmp->settings = ttmp->settings->next;
			free(vtmp);
			free(stmp);
		}
		free(ttmp->name);
		template = template->next;
		free(ttmp);
	}
	free(template);
	
	logprintf(LOG_DEBUG, "garbage collected template library");
	
	return EXIT_SUCCESS;
}

int template_read(char *templatefile) {
	FILE *fp;
	char *content;
	size_t bytes;
	struct stat sb;

	/* Read JSON config file */
	if((fp = fopen(templatefile, "rb")) == NULL) {
		logprintf(LOG_ERR, "cannot read config file: %s", templatefile);
		return EXIT_FAILURE;
	}

	fstat(fileno(fp), &sb);
	bytes = (size_t)sb.st_size;

	if((content = calloc(bytes+1, sizeof(char))) == NULL) {
		logprintf(LOG_ERR, "out of memory", templatefile);
		return EXIT_FAILURE;
	}

	if(fread(content, sizeof(char), bytes, fp) == -1) {
		logprintf(LOG_ERR, "cannot read config file: %s", templatefile);
	}
	fclose(fp);

	/* Validate JSON and turn into JSON object */
	if(json_validate(content) == false) {
		logprintf(LOG_ERR, "config is not in a valid json format", templatefile);
		free(content);
		return EXIT_FAILURE;
	}
	
	JsonNode *root = json_decode(content);
	JsonNode *child = json_first_child(root);
	char *type = NULL;
	int  i = 0;
	int nrtext = 0;
	int nrprogress = 0;

	while(child) {
		i++;
		if(child->tag == JSON_OBJECT) {
			if(json_find_string(child, "type", &type) == 0) {
				if(strcmp(type, "image") == 0) {
					template_add_image(child);
				}
				if(strcmp(type, "progress") == 0) {
					template_add_progress(child);
					nrprogress++;
					if(nrprogress > 1) {
						logprintf(LOG_ERR, "currently only one progress object is allowed", i);
						return EXIT_FAILURE;
					}
				}
				if(strcmp(type, "shape") == 0) {
					template_add_shape(child);
				}
				if(strcmp(type, "text") == 0) {
					template_add_text(child);
					nrtext++;
					if(nrtext > 1) {
						logprintf(LOG_ERR, "currently only one text object is allowed", i);
						return EXIT_FAILURE;
					}
				}
			} else {
				logprintf(LOG_ERR, "settings #%d, \"type\" missing", i);
				return EXIT_FAILURE;
			}
		} else {
			logprintf(LOG_ERR, "settings #%d, invalid", i);
			return EXIT_FAILURE;
		}
		child = child->next;
	}
	json_delete(root);
	free(content);
	template_reverse_struct(&template);
	return EXIT_SUCCESS;
}