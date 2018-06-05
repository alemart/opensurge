/*
 * Open Surge Engine
 * switch_character.c - Switches the active character
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <math.h>
#include "switch_character.h"
#include "../player.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/audio.h"
#include "../../core/soundfactory.h"
#include "../../scenes/level.h"

/* objectdecorator_switchcharacter_t class */
typedef struct objectdecorator_switchcharacter_t objectdecorator_switchcharacter_t;
struct objectdecorator_switchcharacter_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *name; /* character name */
    int force_switch; /* forces the character switch, even if the engine does not want it */
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_switchcharacter_new(objectmachine_t *decorated_machine, const char *name, int force_switch)
{
    objectdecorator_switchcharacter_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->name = (name != NULL && *name != 0) ? str_dup(name) : NULL;
    me->force_switch = force_switch;

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
    objectdecorator_switchcharacter_t *me = (objectdecorator_switchcharacter_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    if(me->name != NULL)
        free(me->name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_switchcharacter_t *me = (objectdecorator_switchcharacter_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = level_player(); /* active player */
    player_t *new_player = NULL;
    int i;

    if(me->name != NULL) {
        for(i=0; i<team_size && new_player == NULL; i++) {
            if(str_icmp(team[i]->name, me->name) == 0)
                new_player = team[i];
        }
    }
    else
        new_player = enemy_get_observed_player(object);

    if(new_player != NULL) {
        int allow_switching, got_dying_player = FALSE;

        for(i=0; i<team_size && !got_dying_player; i++)
            got_dying_player = player_is_dying(team[i]);

        allow_switching = !got_dying_player && !level_has_been_cleared() && !player_is_in_the_air(player) && !player->on_moveable_platform && !player->disable_movement && !player->in_locked_area;

        if(allow_switching || me->force_switch)
            level_change_player(new_player);
        else
            sound_play( soundfactory_get("deny") );
    }
    else
        fatal_error("Can't switch character: player '%s' does not exist!", me->name);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

