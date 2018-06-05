/*
 * Open Surge Engine
 * set_player_animation.c - This decorator changes the animation of the player
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "set_player_animation.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../entities/player.h"

/* objectdecorator_setplayeranimation_t class */
typedef struct objectdecorator_setplayeranimation_t objectdecorator_setplayeranimation_t;
struct objectdecorator_setplayeranimation_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *sprite_name;
    expression_t *animation_id;
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setplayeranimation_new(objectmachine_t *decorated_machine, const char *sprite_name, expression_t *animation_id)
{
    objectdecorator_setplayeranimation_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->sprite_name = str_dup(sprite_name);
    me->animation_id = animation_id;

    return obj;
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
    objectdecorator_setplayeranimation_t *me = (objectdecorator_setplayeranimation_t*)obj;

    free(me->sprite_name);
    expression_destroy(me->animation_id);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayeranimation_t *me = (objectdecorator_setplayeranimation_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));
    int animation_id = (int)expression_evaluate(me->animation_id);
    animation_t *anim = sprite_get_animation(me->sprite_name, animation_id);

    player->disable_animation_control = TRUE;
    actor_change_animation(player->actor, anim);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}
