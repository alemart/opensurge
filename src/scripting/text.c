/*
 * Open Surge Engine
 * prefs.c - scripting system: UI Text
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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

#include <surgescript.h>
#include <string.h>
#include <math.h>
#include "scripting.h"
#include "../core/font.h"
#include "../core/util.h"
#include "../core/video.h"
#include "../core/stringutil.h"
#include "../entities/camera.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfont(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_settext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmaxwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setmaxwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmaxlength(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setmaxlength(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getalign(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setalign(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsize(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t FONT_ADDR = 0;
static const surgescript_heapptr_t TEXT_ADDR = 1;
static const surgescript_heapptr_t ALIGN_ADDR = 2;
static const surgescript_heapptr_t ZINDEX_ADDR = 3;
static const surgescript_heapptr_t VISIBLE_ADDR = 4;
static const surgescript_heapptr_t DETACHED_ADDR = 5;
static const surgescript_heapptr_t OFFSET_ADDR = 6;
static const surgescript_heapptr_t MAXWIDTH_ADDR = 7;
static const surgescript_heapptr_t SIZE_ADDR = 8;
static const char* DEFAULT_TEXT = "";
static const char* DEFAULT_FONT = "default";
static const char* DEFAULT_ALIGN = "left";
static const double DEFAULT_ZINDEX = 0.5;
static const double DEFAULT_MAXWIDTH = INFINITY; /* no wordwrap */
static const bool DEFAULT_VISIBILITY = true;
static inline font_t* get_font(const surgescript_object_t* object);
static inline fontalign_t str2align(const char* align);
static inline const char* align2str(fontalign_t align);

/*
 * scripting_register_text()
 * Register the Text object
 */
void scripting_register_text(surgescript_vm_t* vm)
{
    /* tags */
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    surgescript_tagsystem_add_tag(tag_system, "Text", "renderable");

    /* methods */
    surgescript_vm_bind(vm, "Text", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Text", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Text", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Text", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Text", "set_zindex", fun_setzindex, 1);
    surgescript_vm_bind(vm, "Text", "get_zindex", fun_getzindex, 0);
    surgescript_vm_bind(vm, "Text", "get_font", fun_getfont, 0);
    surgescript_vm_bind(vm, "Text", "set_text", fun_settext, 1);
    surgescript_vm_bind(vm, "Text", "get_text", fun_gettext, 0);
    surgescript_vm_bind(vm, "Text", "set_align", fun_setalign, 1);
    surgescript_vm_bind(vm, "Text", "get_align", fun_getalign, 0);
    surgescript_vm_bind(vm, "Text", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "Text", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "Text", "set_maxWidth", fun_setmaxwidth, 1);
    surgescript_vm_bind(vm, "Text", "get_maxWidth", fun_getmaxwidth, 0);
    surgescript_vm_bind(vm, "Text", "set_maxLength", fun_setmaxlength, 1);
    surgescript_vm_bind(vm, "Text", "get_maxLength", fun_getmaxlength, 0);
    surgescript_vm_bind(vm, "Text", "get_offset", fun_getoffset, 0);
    surgescript_vm_bind(vm, "Text", "set_offset", fun_setoffset, 1);
    surgescript_vm_bind(vm, "Text", "get_size", fun_getsize, 0);
    surgescript_vm_bind(vm, "Text", "onRender", fun_onrender, 0);
}

/*
 * scripting_text_fontptr()
 * Returns the font_t* associated with the given SurgeScript Text object
 */
font_t* scripting_text_fontptr(const surgescript_object_t* object)
{
    font_t* font = get_font(object);
    if(font == NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        const char* text = surgescript_var_fast_get_string(surgescript_heap_at(heap, TEXT_ADDR));
        scripting_error(object, "Font not found for \"%s\"", text);
    }
    return font;
}


/* -- object methods -- */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL)
        font_set_position(font, scripting_util_world_position(object));
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    bool is_detached = surgescript_object_has_tag(parent, "detached");
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* internal data */
    ssassert(FONT_ADDR == surgescript_heap_malloc(heap));
    ssassert(TEXT_ADDR == surgescript_heap_malloc(heap));
    ssassert(ALIGN_ADDR == surgescript_heap_malloc(heap));
    ssassert(ZINDEX_ADDR == surgescript_heap_malloc(heap));
    ssassert(VISIBLE_ADDR == surgescript_heap_malloc(heap));
    ssassert(DETACHED_ADDR == surgescript_heap_malloc(heap));
    ssassert(OFFSET_ADDR == surgescript_heap_malloc(heap));
    ssassert(MAXWIDTH_ADDR == surgescript_heap_malloc(heap));
    ssassert(SIZE_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, FONT_ADDR));
    surgescript_var_set_string(surgescript_heap_at(heap, TEXT_ADDR), DEFAULT_TEXT);
    surgescript_var_set_string(surgescript_heap_at(heap, ALIGN_ADDR), DEFAULT_ALIGN);
    surgescript_var_set_number(surgescript_heap_at(heap, ZINDEX_ADDR), DEFAULT_ZINDEX);
    surgescript_var_set_bool(surgescript_heap_at(heap, VISIBLE_ADDR), DEFAULT_VISIBILITY);
    surgescript_var_set_null(surgescript_heap_at(heap, OFFSET_ADDR)); /* lazy allocation */
    surgescript_var_set_bool(surgescript_heap_at(heap, DETACHED_ADDR), is_detached);
    surgescript_var_set_number(surgescript_heap_at(heap, MAXWIDTH_ADDR), DEFAULT_MAXWIDTH);
    surgescript_var_set_null(surgescript_heap_at(heap, SIZE_ADDR)); /* lazy allocation */

    /* sanity check */
    if(!surgescript_object_has_tag(parent, "entity")) {
        const char* parent_name = surgescript_object_name(parent);
        scripting_error(object, "Object \"%s\" spawns a Text. Hence, it should be tagged as an \"entity\".", parent_name);
    }

    /* done! */
    surgescript_object_set_userdata(object, NULL);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL)
        font_destroy(font);
    return NULL;
}

/* __init: pass a font name */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    double max_width = surgescript_var_get_number(surgescript_heap_at(heap, MAXWIDTH_ADDR));
    char* font_name = !surgescript_var_is_null(param[0]) ? surgescript_var_get_string(param[0], manager) : str_dup(DEFAULT_FONT);
    font_t* font = font_create(font_name);

    /* configure font */
    font_set_text(font, "%s", surgescript_var_fast_get_string(surgescript_heap_at(heap, TEXT_ADDR)));
    font_set_align(font, str2align(surgescript_var_fast_get_string(surgescript_heap_at(heap, ALIGN_ADDR))));
    font_set_visible(font, surgescript_var_get_bool(surgescript_heap_at(heap, VISIBLE_ADDR)));
    font_set_width(font, isfinite(max_width) ? max_width : 0);

    /* userdata */
    surgescript_var_set_string(surgescript_heap_at(heap, FONT_ADDR), font_name);
    surgescript_object_set_userdata(object, font);

    /* done! */
    ssfree(font_name);
    return NULL;
}

/* render */
surgescript_var_t* fun_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        bool is_detached = surgescript_var_get_bool(surgescript_heap_at(heap, DETACHED_ADDR));
        v2d_t camera = !is_detached ? camera_get_position() : v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2);
        font_render(font, camera);
    }
    return NULL;
}

/* set zindex */
surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    double zindex = surgescript_var_get_number(param[0]);
    surgescript_var_set_number(surgescript_heap_at(heap, ZINDEX_ADDR), zindex);
    return NULL;
}

/* get zindex (defaults to DEFAULT_ZINDEX) */
surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ZINDEX_ADDR));
}

/* get font name */
surgescript_var_t* fun_getfont(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, FONT_ADDR));
}

/* set text */
surgescript_var_t* fun_settext(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        const char* str = surgescript_var_fast_get_string(param[0]);
        if(*str == 0) {
            surgescript_objectmanager_t* manager = surgescript_object_manager(object);
            char* text = surgescript_var_get_string(param[0], manager);
            font_set_text(font, "%s", text);
            surgescript_var_set_string(surgescript_heap_at(heap, TEXT_ADDR), text);
            ssfree(text);
        }
        else {
            const char* text = str;
            font_set_text(font, "%s", text);
            surgescript_var_set_string(surgescript_heap_at(heap, TEXT_ADDR), text);
        }
    }
    return NULL;
}

/* get text */
surgescript_var_t* fun_gettext(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, TEXT_ADDR));
}

/* set align */
surgescript_var_t* fun_setalign(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        fontalign_t align = str2align(surgescript_var_fast_get_string(param[0]));
        font_set_align(font, align);
        surgescript_var_set_string(surgescript_heap_at(heap, ALIGN_ADDR), align2str(align));
    }
    return NULL;
}

/* get align */
surgescript_var_t* fun_getalign(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ALIGN_ADDR));
}

/* set max width */
surgescript_var_t* fun_setmaxwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        double max_width = max(1, surgescript_var_get_number(param[0]));
        font_set_width(font, isfinite(max_width) ? max_width : 0);
        surgescript_var_set_number(surgescript_heap_at(heap, MAXWIDTH_ADDR), max_width);
    }
    return NULL;
}

/* get max width */
surgescript_var_t* fun_getmaxwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, MAXWIDTH_ADDR));
}

/* set visible */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        bool is_visible = surgescript_var_get_bool(param[0]);
        font_set_visible(font, is_visible);
        surgescript_var_set_bool(surgescript_heap_at(heap, VISIBLE_ADDR), is_visible);
    }
    return NULL;
}

/* get visible */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, VISIBLE_ADDR));
}

/* get offset */
surgescript_var_t* fun_getoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* v2ptr = surgescript_heap_at(heap, OFFSET_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;

    if(surgescript_var_is_null(v2ptr)) { /* lazy allocation */
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(v2ptr, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(v2ptr);

    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, transform->position.x, transform->position.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* set offset */
surgescript_var_t* fun_setoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    transform->position.x = x;
    transform->position.y = y;

    return NULL;
}

/* get maxLength */
surgescript_var_t* fun_getmaxlength(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    return surgescript_var_set_number(surgescript_var_create(), font != NULL ? font_get_maxlength(font) : 0);
}

/* set maxLength */
surgescript_var_t* fun_setmaxlength(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    font_t* font = get_font(object);
    if(font != NULL) {
        int maxlength = max(0, surgescript_var_get_number(param[0]));
        font_set_maxlength(font, maxlength);
    }
    return NULL;
}

/* get size, in pixels */
surgescript_var_t* fun_getsize(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* v2ptr = surgescript_heap_at(heap, SIZE_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;
    font_t* font = get_font(object);
    v2d_t size = font_get_textsize(font);

    if(surgescript_var_is_null(v2ptr)) { /* lazy allocation */
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(v2ptr, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(v2ptr);

    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, size.x, size.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* -- private -- */

/*
  * get_font(): get the font_t* associated with the object
 * ** may be NULL! **
 */
font_t* get_font(const surgescript_object_t* object)
{
    return (font_t*)surgescript_object_userdata(object);
}

/* string to fontalign_t */
fontalign_t str2align(const char* align)
{
    if(strcmp(align, "left") == 0)
        return FONTALIGN_LEFT;
    else if(strcmp(align, "center") == 0)
        return FONTALIGN_CENTER;
    else if(strcmp(align, "right") == 0)
        return FONTALIGN_RIGHT;
    else
        return FONTALIGN_LEFT;
}

/* fontalign_t to string */
const char* align2str(fontalign_t align)
{
    switch(align) {
        case FONTALIGN_LEFT:   return "left";
        case FONTALIGN_CENTER: return "center";
        case FONTALIGN_RIGHT:  return "right";
        default:               return "left";
    }
}