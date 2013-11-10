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

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

#include "json.h"

#define STRING 	1
#define NUMBER 	2
#define COLOR 	3

typedef enum tpl_type_t {
	SHAPE,
	IMAGE,
	TEXT,
	PROGRESS,
	PROGRESS_INACTIVE_START,
	PROGRESS_INACTIVE_FILL,
	PROGRESS_INACTIVE_END,
	PROGRESS_ACTIVE_START,
	PROGRESS_ACTIVE_FILL,
	PROGRESS_ACTIVE_END
} tpl_type_t;

typedef union tpl_value_t {
	int number_;
	char *string_;
	unsigned short *color_;
} tpl_value_t;

typedef struct tpl_settings_t {
	char *name;
	union tpl_value_t *value;
	int type;
	struct tpl_settings_t *next;
} tpl_settings_t;

typedef struct template_t {
	enum tpl_type_t type;
	char *name;
	struct tpl_settings_t *settings;
	struct template_t *next;
} template_t;

struct template_t *template;

void template_reverse_struct(struct template_t **tpl);
void template_print(void);
void template_add_progress(JsonNode *root);
void template_add_image(JsonNode *root);
void template_parse_color(JsonNode *root, unsigned short *color);
void template_add_text(JsonNode *root);
void template_add_shape(JsonNode *root);
int template_read(char *templatefile);
int template_gc(void);
int template_get_setting_string(struct template_t *tnode, const char *name, char **val);
int template_get_setting_number(struct template_t *tnode, const char *name, int *out);
int template_get_setting_color(struct template_t *tnode, const char *name, unsigned short **out);

#endif