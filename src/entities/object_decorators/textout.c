/*
 * Open Surge Engine
 * textout.c - text output
 * Copyright (C) 2010-2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include "textout.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/font.h"
#include "../object_vm.h"

typedef enum { TEXTOUT_LEFT, TEXTOUT_CENTRE, TEXTOUT_RIGHT } textoutstyle_t;


/* objectdecorator_textout_t class */
typedef struct objectdecorator_textout_t objectdecorator_textout_t;
struct objectdecorator_textout_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    textoutstyle_t style;
    font_t *fnt;
    char *text;
    expression_t *xpos, *ypos;
    expression_t *max_width;
    expression_t *index_of_first_char, *length;
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

objectmachine_t* make_decorator(objectmachine_t *decorated_machine, textoutstyle_t style, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length);

static int tagged_strlen(const char *s);

/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_textout_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    return make_decorator(decorated_machine, TEXTOUT_LEFT, font_name, xpos, ypos, text, max_width, index_of_first_char, length);
}

objectmachine_t* objectdecorator_textoutcentre_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    return make_decorator(decorated_machine, TEXTOUT_CENTRE, font_name, xpos, ypos, text, max_width, index_of_first_char, length);
}

objectmachine_t* objectdecorator_textoutright_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    return make_decorator(decorated_machine, TEXTOUT_RIGHT, font_name, xpos, ypos, text, max_width, index_of_first_char, length);
}




/* private methods */
void init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_textout_t *me = (objectdecorator_textout_t*)obj;

    expression_destroy(me->xpos);
    expression_destroy(me->ypos);
    expression_destroy(me->max_width);
    expression_destroy(me->index_of_first_char);
    expression_destroy(me->length);
    font_destroy(me->fnt);
    free(me->text);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_textout_t *me = (objectdecorator_textout_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    symboltable_t *st = objectvm_get_symbol_table(object->vm);
    char *processed_text;
    int start, length;
    float wpx;
    v2d_t pos;

    /* calculate the range of the string (no need to clip() it) */
    start = (int)expression_evaluate(me->index_of_first_char);
    length = (int)expression_evaluate(me->length);

    /* configuring the font */
    font_use_substring(me->fnt, start, length);
    font_set_width(me->fnt, (int)expression_evaluate(me->max_width));

    /* font text */
    processed_text = nanocalc_interpolate_string(me->text, st);
    font_set_text(me->fnt, "%s", processed_text);
    free(processed_text);

    /* symbol table tricks */
    symboltable_set(st, "$_STRLEN", tagged_strlen(font_get_text(me->fnt))); /* store the length of the text in $_STRLEN */

    /* font position */
    pos = v2d_new(expression_evaluate(me->xpos), expression_evaluate(me->ypos));
    wpx = font_get_textsize(me->fnt).x;
    switch(me->style) {
    case TEXTOUT_LEFT: break;
    case TEXTOUT_CENTRE: pos.x -= wpx/2; break;
    case TEXTOUT_RIGHT: pos.x -= wpx; break;
    }
    font_set_position(me->fnt, v2d_add(object->actor->position, pos));

    /* done! */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_textout_t *me = (objectdecorator_textout_t*)obj;

    /* render */
    font_render(me->fnt, camera_position);

    /* done! */
    decorated_machine->render(decorated_machine, camera_position);
}


objectmachine_t* make_decorator(objectmachine_t *decorated_machine, textoutstyle_t style, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length)
{
    objectdecorator_textout_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->style = style;
    me->fnt = font_create(font_name);
    me->xpos = xpos;
    me->ypos = ypos;
    me->text = str_dup(text);
    me->max_width = max_width;
    me->index_of_first_char = index_of_first_char;
    me->length = length;

    return obj;
}


/* stuff */
static int tagged_strlen(const char *s)
{
    const char *p;
    int k = 0, in_tag = FALSE;

    for(p=s; *p; p++) {
        if(*p == '<') { in_tag = TRUE; continue; }
        else if(*p == '>') { in_tag = FALSE; continue; }
        k += in_tag ? 0 : 1;
    }

    return k;
}
