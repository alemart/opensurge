/*
 * Open Surge Engine
 * set_animation.c - This decorator sets the animation of the object
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "set_animation.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"

/* strategy pattern */
typedef struct objectdecorator_setanimationstrategy_t objectdecorator_setanimationstrategy_t;
struct objectdecorator_setanimationstrategy_t { /* <<interface>> */
    void (*init)(objectmachine_t *o);
    void (*release)(objectmachine_t *o);
    void (*update)(objectmachine_t *o);
};

/* command: set_animation */
typedef struct objectdecorator_setanimationstrategy_anim_t objectdecorator_setanimationstrategy_anim_t;
struct objectdecorator_setanimationstrategy_anim_t { /* implements objectdecorator_setanimationstrategy_t */
    objectdecorator_setanimationstrategy_t base;
    char *sprite_name;
    expression_t *animation_id;
};
static objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_anim_new(const char *sprite_name, expression_t *animation_id);
static void objectdecorator_setanimationstrategy_anim_init(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_anim_release(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_anim_update(objectmachine_t *o);

/* command: set_animation_frame */
typedef struct objectdecorator_setanimationstrategy_frame_t objectdecorator_setanimationstrategy_frame_t;
struct objectdecorator_setanimationstrategy_frame_t { /* implements objectdecorator_setanimationstrategy_t */
    objectdecorator_setanimationstrategy_t base;
    expression_t *animation_frame;
};
static objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_frame_new(expression_t *animation_frame);
static void objectdecorator_setanimationstrategy_frame_init(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_frame_release(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_frame_update(objectmachine_t *o);

/* command: set_animation_speed_factor */
typedef struct objectdecorator_setanimationstrategy_speed_t objectdecorator_setanimationstrategy_speed_t;
struct objectdecorator_setanimationstrategy_speed_t { /* implements objectdecorator_setanimationstrategy_t */
    objectdecorator_setanimationstrategy_t base;
    expression_t *animation_speed;
};
static objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_speed_new(expression_t *animation_speed);
static void objectdecorator_setanimationstrategy_speed_init(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_speed_release(objectmachine_t *o);
static void objectdecorator_setanimationstrategy_speed_update(objectmachine_t *o);



/* ------------------------------------------ */


/* objectdecorator_setanimation_t class */
typedef struct objectdecorator_setanimation_t objectdecorator_setanimation_t;
struct objectdecorator_setanimation_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    objectdecorator_setanimationstrategy_t *strategy;
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* make_decorator(objectdecorator_setanimationstrategy_t *strategy, objectmachine_t *decorated_machine);
static void change_the_animation(objectmachine_t *obj);


/* ------------------------------------------ */


/* public methods */

objectmachine_t* objectdecorator_setanimation_new(objectmachine_t *decorated_machine, const char *sprite_name, expression_t *animation_id)
{
    return make_decorator( objectdecorator_setanimationstrategy_anim_new(sprite_name, animation_id), decorated_machine );
}

objectmachine_t* objectdecorator_setanimationframe_new(objectmachine_t *decorated_machine, expression_t *animation_frame)
{
    return make_decorator( objectdecorator_setanimationstrategy_frame_new(animation_frame), decorated_machine );
}

objectmachine_t* objectdecorator_setanimationspeedfactor_new(objectmachine_t *decorated_machine, expression_t *animation_speed_factor)
{
    return make_decorator( objectdecorator_setanimationstrategy_speed_new(animation_speed_factor), decorated_machine );
}





/* private methods */
void init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;

    me->strategy->init(obj);

    decorated_machine->init(decorated_machine);
}

void release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;

    me->strategy->release(obj);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;

    me->strategy->update(obj);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}



/* ------------------------------------------ */


/* command: set_animation */
objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_anim_new(const char *sprite_name, expression_t *animation_id)
{
    objectdecorator_setanimationstrategy_anim_t *s = mallocx(sizeof *s); /* strategy */
    objectdecorator_setanimationstrategy_t *p = (objectdecorator_setanimationstrategy_t*)s; /* parent class */

    p->init = objectdecorator_setanimationstrategy_anim_init;
    p->release = objectdecorator_setanimationstrategy_anim_release;
    p->update = objectdecorator_setanimationstrategy_anim_update;
    s->sprite_name = str_dup(sprite_name);
    s->animation_id = animation_id;

    return p;
}

void objectdecorator_setanimationstrategy_anim_init(objectmachine_t *o)
{
    change_the_animation(o);
}

void objectdecorator_setanimationstrategy_anim_release(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_anim_t *s = (objectdecorator_setanimationstrategy_anim_t*)(me->strategy);

    expression_destroy(s->animation_id);
    free(s->sprite_name);
    free(s);
}

void objectdecorator_setanimationstrategy_anim_update(objectmachine_t *o)
{
    change_the_animation(o);
}



/* command: set_animation_frame */
objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_frame_new(expression_t *animation_frame)
{
    objectdecorator_setanimationstrategy_frame_t *s = mallocx(sizeof *s); /* strategy */
    objectdecorator_setanimationstrategy_t *p = (objectdecorator_setanimationstrategy_t*)s; /* parent class */

    p->init = objectdecorator_setanimationstrategy_frame_init;
    p->release = objectdecorator_setanimationstrategy_frame_release;
    p->update = objectdecorator_setanimationstrategy_frame_update;
    s->animation_frame = animation_frame;

    return p;
}

void objectdecorator_setanimationstrategy_frame_init(objectmachine_t *o)
{
    ; /* empty */
}

void objectdecorator_setanimationstrategy_frame_release(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_frame_t *s = (objectdecorator_setanimationstrategy_frame_t*)(me->strategy);

    expression_destroy(s->animation_frame);
    free(s);
}

void objectdecorator_setanimationstrategy_frame_update(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_frame_t *s = (objectdecorator_setanimationstrategy_frame_t*)(me->strategy);
    object_t *object = o->get_object_instance(o);

    actor_change_animation_frame(object->actor, (int)expression_evaluate(s->animation_frame));
}


/* command: set_animation_speed_factor */
objectdecorator_setanimationstrategy_t* objectdecorator_setanimationstrategy_speed_new(expression_t *animation_speed)
{
    objectdecorator_setanimationstrategy_speed_t *s = mallocx(sizeof *s); /* strategy */
    objectdecorator_setanimationstrategy_t *p = (objectdecorator_setanimationstrategy_t*)s; /* parent class */

    p->init = objectdecorator_setanimationstrategy_speed_init;
    p->release = objectdecorator_setanimationstrategy_speed_release;
    p->update = objectdecorator_setanimationstrategy_speed_update;
    s->animation_speed = animation_speed;

    return p;
}

void objectdecorator_setanimationstrategy_speed_init(objectmachine_t *o)
{
    ; /* empty */
}

void objectdecorator_setanimationstrategy_speed_release(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_speed_t *s = (objectdecorator_setanimationstrategy_speed_t*)(me->strategy);

    expression_destroy(s->animation_speed);
    free(s);
}

void objectdecorator_setanimationstrategy_speed_update(objectmachine_t *o)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)o;
    objectdecorator_setanimationstrategy_speed_t *s = (objectdecorator_setanimationstrategy_speed_t*)(me->strategy);
    object_t *object = o->get_object_instance(o);

    actor_change_animation_speed_factor(object->actor, expression_evaluate(s->animation_speed));
}



/* ------------------------------------------ */



/* this method effectively changes the animation of the object */
void change_the_animation(objectmachine_t *obj)
{
    objectdecorator_setanimation_t *me = (objectdecorator_setanimation_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    objectdecorator_setanimationstrategy_anim_t *s = (objectdecorator_setanimationstrategy_anim_t*)(me->strategy);
    int animation_id = (int)expression_evaluate(s->animation_id);
    animation_t *anim = sprite_get_animation(s->sprite_name, animation_id);

    actor_change_animation(object->actor, anim);
}

/* instantiates a decorator */
objectmachine_t* make_decorator(objectdecorator_setanimationstrategy_t *strategy, objectmachine_t *decorated_machine)
{
    objectdecorator_setanimation_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->strategy = strategy;

    return obj;
}
