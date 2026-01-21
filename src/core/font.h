/*
 * Open Surge Engine
 * font.h - font module
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FONT_H
#define _FONT_H

#include <stdbool.h>
#include "../util/v2d.h"

/* font type */
typedef struct font_t font_t;
typedef enum fontalign_t fontalign_t;
enum fontalign_t { FONTALIGN_LEFT, FONTALIGN_CENTER, FONTALIGN_RIGHT };

/* forward declarations */
struct image_t;

/* public functions */
font_t* font_create(const char* font_name); /* creates a new font instance. font_name is a font in the *.fnt scripts */
void font_destroy(font_t* f); /* destroys an existing font instance */
void font_render(font_t* f, v2d_t camera_position); /* renders the font */
void font_set_text(font_t* f, const char* fmt, ...); /* sets a new text for the font; printf style. BE CAREFUL with unsanitized fmt's! */
void font_set_textarguments(font_t* f, int amount, ...); /* pass <amount> of const char*'s; they'll be stored in $1, $2, ... up to $9 */
void font_set_textargumentsv(font_t* f, int argc, const char** argv); /* an alternative to font_set_textarguments() without variadic arguments */
const char* font_get_text(const font_t* f); /* gets the text of the font */
void font_set_position(font_t* f, v2d_t position); /* sets the position of the font in the scene */
v2d_t font_get_position(const font_t* f); /* gets the position of the font in the scene */
v2d_t font_get_textsize(const font_t* f); /* returns the size, in pixels, of the rendered text */
void font_set_width(font_t* f, int w); /* wordwrap (w is given in pixels) */
bool font_is_visible(const font_t* f); /* is the font visible? */
void font_set_visible(font_t* f, bool is_visible); /* show/hide font */
void font_use_substring(font_t* f, int index_of_first_char, int length); /* since fonts may have color tags, variables, etc. , use this to display a substring of the font (not the whole text) */
fontalign_t font_get_align(const font_t* f); /* get the current align */
void font_set_align(font_t* f, fontalign_t align); /* set the align */
int font_get_maxlength(const font_t* f); /* get the maximum number of characters that can be displayed, ignoring color tags and blanks */
void font_set_maxlength(font_t* f, int max_length); /* set the maximum number of characters, ignoring color tags and blanks */
const char* font_get_filepath(const font_t* f); /* get the relative path of the file (image, truetype font...) that originates this font */
const struct image_t* font_get_image(const font_t* f); /* get the image atlas if it's a bitmap font; otherwise NULL is returned */

/* misc */
void font_init(); /* initializes the font module */
void font_release(); /* releases the font module */
void font_register_variable(const char* variable_name, const char* (*callback)()); /* variable/text interpolation */
bool font_exists(const char* font_name); /* checks if a font script (.fnt) of the given name exists */


#endif
