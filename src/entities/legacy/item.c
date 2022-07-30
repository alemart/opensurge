/*
 * Open Surge Engine
 * item.c - legacy items (replaced by SurgeScript)
 * Copyright (C) 2008-2010, 2018  Alexandre Martins <alemartf@gmail.com>
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

#include <math.h>
#include <stdlib.h>
#include <stdarg.h>

#include "item.h"
#include "enemy.h"
#include "../player.h"
#include "../brick.h"
#include "../actor.h"
#include "../sfx.h"

#include "../../core/v2d.h"
#include "../../core/global.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/timer.h"
#include "../../core/video.h"
#include "../../core/image.h"
#include "../../core/color.h"
#include "../../core/font.h"
#include "../../core/audio.h"

#include "../../scenes/quest.h"
#include "../../scenes/level.h"

#include "../../physics/collisionmask.h"
#include "../../physics/obstacle.h"

/* private utilities */
static item_t* find_closest_item(item_t *me, item_list_t *list, int desired_type, float *distance);

/* item functions */
static item_t* animal_create();
static item_t* animalprison_create();
static item_t* bigring_create();
static item_t* bouncingcollectible_create();
static item_t* bumper_create();
static item_t* checkpointorb_create();
static item_t* collectible_create();
static item_t* crushedbox_create();
static item_t* horizontaldanger_create();
static item_t* verticaldanger_create();
static item_t* horizontalfiredanger_create();
static item_t* verticalfiredanger_create();
static item_t* surge_dnadoor_create();
static item_t* neon_dnadoor_create();
static item_t* charge_dnadoor_create();
static item_t* surge_horizontal_dnadoor_create();
static item_t* neon_horizontal_dnadoor_create();
static item_t* charge_horizontal_dnadoor_create();
static item_t* door_create();
static void door_open(item_t *door);
static void door_close(item_t *door);
static item_t* switch_create();
static item_t* endsign_create();
static item_t* explosion_create();
static item_t* flyingtext_create();
static item_t* goalsign_create();
static item_t* icon_create();
static void icon_change_animation(item_t *item, int anim_id);
static item_t* lifebox_create();
static item_t* collectiblebox_create();
static item_t* starbox_create();
static item_t* speedbox_create();
static item_t* glassesbox_create();
static item_t* shieldbox_create();
static item_t* fireshieldbox_create();
static item_t* thundershieldbox_create();
static item_t* watershieldbox_create();
static item_t* acidshieldbox_create();
static item_t* windshieldbox_create();
static item_t* trapbox_create();
static item_t* emptybox_create();
static item_t* loopgreen_create();
static item_t* loopyellow_create();
static item_t* loopright_create();
static item_t* looptop_create();
static item_t* loopleft_create();
static item_t* loopnone_create();
static item_t* loopfloor_create();
static item_t* loopfloornone_create();
static item_t* loopfloortop_create();
static item_t* floorspikes_create();
static item_t* ceilingspikes_create();
static item_t* leftwallspikes_create();
static item_t* rightwallspikes_create();
static item_t* periodic_floorspikes_create();
static item_t* periodic_ceilingspikes_create();
static item_t* periodic_leftwallspikes_create();
static item_t* periodic_rightwallspikes_create();
static item_t* yellowspring_create();
static item_t* tryellowspring_create();
static item_t* ryellowspring_create();
static item_t* bryellowspring_create();
static item_t* byellowspring_create();
static item_t* blyellowspring_create();
static item_t* lyellowspring_create();
static item_t* tlyellowspring_create();
static item_t* redspring_create();
static item_t* trredspring_create();
static item_t* rredspring_create();
static item_t* brredspring_create();
static item_t* bredspring_create();
static item_t* blredspring_create();
static item_t* lredspring_create();
static item_t* tlredspring_create();
static item_t* bluespring_create();
static item_t* trbluespring_create();
static item_t* rbluespring_create(); 
static item_t* brbluespring_create();
static item_t* bbluespring_create();
static item_t* blbluespring_create();
static item_t* lbluespring_create();
static item_t* tlbluespring_create();
static item_t* supercollectible_create();
static item_t* teleporter_create();
static void teleporter_activate(item_t *teleporter, player_t *who);



/* =========== SurgeScript port ================ */

/*
 * item2surgescript()
 * Returns the SurgeScript object name corresponding to
 * the legacy item of the given type, or NULL if the item
 * hasn't been ported. Included for retro-compatibility.
 */
const char* item2surgescript(int type)
{
    /* conversion table */
    static const char* table[] = {
        [IT_COLLECTIBLE] = "Collectible",
        [IT_BOUNCINGCOLLECT] = "Bouncing Collectible",
        [IT_YELLOWSPRING] = "Spring Standard",
        [IT_TRYELLOWSPRING] = "Spring Standard Up Right",
        [IT_RYELLOWSPRING] = "Spring Standard Right",
        [IT_BRYELLOWSPRING] = "Spring Standard Down Right",
        [IT_BYELLOWSPRING] = "Spring Standard Down",
        [IT_BLYELLOWSPRING] = "Spring Standard Down Left",
        [IT_LYELLOWSPRING] = "Spring Standard Left",
        [IT_TLYELLOWSPRING] = "Spring Standard Up Left",
        [IT_REDSPRING] = "Spring Stronger",
        [IT_TRREDSPRING] = "Spring Stronger Up Right",
        [IT_RREDSPRING] = "Spring Stronger Right",
        [IT_BRREDSPRING] = "Spring Stronger Down Right",
        [IT_BREDSPRING] = "Spring Stronger Down",
        [IT_BLREDSPRING] = "Spring Stronger Down Left",
        [IT_LREDSPRING] = "Spring Stronger Left",
        [IT_TLREDSPRING] = "Spring Stronger Up Left",
        [IT_BLUESPRING] = "Spring Strongest",
        [IT_TRBLUESPRING] = "Spring Strongest Up Right",
        [IT_RBLUESPRING] = "Spring Strongest Right",
        [IT_BRBLUESPRING] = "Spring Strongest Down Right",
        [IT_BBLUESPRING] = "Spring Strongest Down",
        [IT_BLBLUESPRING] = "Spring Strongest Down Left",
        [IT_LBLUESPRING] = "Spring Strongest Left",
        [IT_TLBLUESPRING] = "Spring Strongest Up Left",
        [IT_LIFEBOX] = "Powerup 1up",
        [IT_COLLECTIBLEBOX] = "Powerup Collectibles",
        [IT_STARBOX] = "Powerup Invincibility",
        [IT_SPEEDBOX] = "Powerup Speed",
        [IT_SHIELDBOX] = "Powerup Shield",
        [IT_FIRESHIELDBOX] = "Powerup Shield Fire",
        [IT_THUNDERSHIELDBOX] = "Powerup Shield Thunder",
        [IT_WATERSHIELDBOX] = "Powerup Shield Water",
        [IT_ACIDSHIELDBOX] = "Powerup Shield Acid",
        [IT_WINDSHIELDBOX] = "Powerup Shield Wind",
        [IT_TRAPBOX] = "Powerup Trap",
        [IT_CHECKPOINT] = "Checkpoint",
        [IT_ENDSIGN] = "Goal",
        [IT_ENDLEVEL] = "Goal Capsule",
        [IT_BUMPER] = "Bumper",
        [IT_SPIKES] = "Spikes",
        [IT_CEILSPIKES] = "Spikes Down",
        [IT_DOOR] = "Door",
        [IT_TELEPORTER] = "Teleporter",
        [IT_SWITCH] = ".compat_switch",
        [IT_LOOPGREEN] = ".compat_loopgreen",
        [IT_LOOPYELLOW] = ".compat_loopyellow",
        [IT_PERSPIKES] = ".compat_perspikes",
        [IT_PERCEILSPIKES] = ".compat_perceilspikes"
    };

    /* return the object name */
    if(type >= 0 && type < sizeof(table) / sizeof(table[0]))
        return table[type];
    else
        return NULL;
}



/* ========= public functions ============ */

/*
 * item_create()
 * Creates a new item
 */
item_t *item_create(int type)
{
    item_t *item = NULL;

    switch(type) {
        case IT_COLLECTIBLE:
            item = collectible_create();
            break;

        case IT_BOUNCINGCOLLECT:
            item = bouncingcollectible_create();
            break;

        case IT_LIFEBOX:
            item = lifebox_create();
            break;

        case IT_COLLECTIBLEBOX:
            item = collectiblebox_create();
            break;

        case IT_STARBOX:
            item = starbox_create();
            break;

        case IT_SPEEDBOX:
            item = speedbox_create();
            break;

        case IT_GLASSESBOX:
            item = glassesbox_create();
            break;

        case IT_SHIELDBOX:
            item = shieldbox_create();
            break;

        case IT_FIRESHIELDBOX:
            item = fireshieldbox_create();
            break;

        case IT_THUNDERSHIELDBOX:
            item = thundershieldbox_create();
            break;

        case IT_WATERSHIELDBOX:
            item = watershieldbox_create();
            break;

        case IT_ACIDSHIELDBOX:
            item = acidshieldbox_create();
            break;

        case IT_WINDSHIELDBOX:
            item = windshieldbox_create();
            break;

        case IT_TRAPBOX:
            item = trapbox_create();
            break;

        case IT_EMPTYBOX:
            item = emptybox_create();
            break;

        case IT_CRUSHEDBOX:
            item = crushedbox_create();
            break;

        case IT_ICON:
            item = icon_create();
            break;

        case IT_EXPLOSION:
            item = explosion_create();
            break;

        case IT_FLYINGTEXT:
            item = flyingtext_create();
            break;

        case IT_ANIMAL:
            item = animal_create();
            break;

        case IT_LOOPRIGHT:
            item = loopright_create();
            break;

        case IT_LOOPMIDDLE:
            item = looptop_create();
            break;

        case IT_LOOPLEFT:
            item = loopleft_create();
            break;

        case IT_LOOPNONE:
            item = loopnone_create();
            break;

        case IT_LOOPFLOOR:
            item = loopfloor_create();
            break;

        case IT_LOOPFLOORNONE:
            item = loopfloornone_create();
            break;

        case IT_LOOPFLOORTOP:
            item = loopfloortop_create();
            break;

        case IT_YELLOWSPRING:
            item = yellowspring_create();
            break;

        case IT_BYELLOWSPRING:
            item = byellowspring_create();
            break;

        case IT_TRYELLOWSPRING:
            item = tryellowspring_create();
            break;

        case IT_RYELLOWSPRING:
            item = ryellowspring_create();
            break;

        case IT_BRYELLOWSPRING:
            item = bryellowspring_create();
            break;

        case IT_BLYELLOWSPRING:
            item = blyellowspring_create();
            break;

        case IT_LYELLOWSPRING:
            item = lyellowspring_create();
            break;

        case IT_TLYELLOWSPRING:
            item = tlyellowspring_create();
            break;

        case IT_REDSPRING:
            item = redspring_create();
            break;

        case IT_BREDSPRING:
            item = bredspring_create();
            break;

        case IT_TRREDSPRING:
            item = trredspring_create();
            break;

        case IT_RREDSPRING:
            item = rredspring_create();
            break;

        case IT_BRREDSPRING:
            item = brredspring_create();
            break;

        case IT_BLREDSPRING:
            item = blredspring_create();
            break;

        case IT_LREDSPRING:
            item = lredspring_create();
            break;

        case IT_TLREDSPRING:
            item = tlredspring_create();
            break;

        case IT_BLUESPRING:
            item = bluespring_create();
            break;

        case IT_BBLUESPRING:
            item = bbluespring_create();
            break;

        case IT_TRBLUESPRING:
            item = trbluespring_create();
            break;

        case IT_RBLUESPRING:
            item = rbluespring_create();
            break;

        case IT_BRBLUESPRING:
            item = brbluespring_create();
            break;

        case IT_BLBLUESPRING:
            item = blbluespring_create();
            break;

        case IT_LBLUESPRING:
            item = lbluespring_create();
            break;

        case IT_TLBLUESPRING:
            item = tlbluespring_create();
            break;

        case IT_BLUECOLLECTIBLE:
            item = supercollectible_create();
            break;

        case IT_SWITCH:
            item = switch_create();
            break;

        case IT_DOOR:
            item = door_create();
            break;

        case IT_TELEPORTER:
            item = teleporter_create();
            break;

        case IT_BIGRING:
            item = bigring_create();
            break;

        case IT_CHECKPOINT:
            item = checkpointorb_create();
            break;

        case IT_GOAL:
            item = goalsign_create();
            break;

        case IT_ENDSIGN:
            item = endsign_create();
            break;

        case IT_ENDLEVEL:
            item = animalprison_create();
            break;

        case IT_BUMPER:
            item = bumper_create();
            break;

        case IT_DANGER:
            item = horizontaldanger_create();
            break;

        case IT_VDANGER:
            item = verticaldanger_create();
            break;

        case IT_FIREDANGER:
            item = horizontalfiredanger_create();
            break;

        case IT_VFIREDANGER:
            item = verticalfiredanger_create();
            break;

        case IT_SPIKES:
            item = floorspikes_create();
            break;

        case IT_CEILSPIKES:
            item = ceilingspikes_create();
            break;

        case IT_LWSPIKES:
            item = leftwallspikes_create();
            break;

        case IT_RWSPIKES:
            item = rightwallspikes_create();
            break;

        case IT_PERSPIKES:
            item = periodic_floorspikes_create();
            break;

        case IT_PERCEILSPIKES:
            item = periodic_ceilingspikes_create();
            break;

        case IT_PERLWSPIKES:
            item = periodic_leftwallspikes_create();
            break;

        case IT_PERRWSPIKES:
            item = periodic_rightwallspikes_create();
            break;

        case IT_DNADOOR:
            item = surge_dnadoor_create();
            break;

        case IT_DNADOORNEON:
            item = neon_dnadoor_create();
            break;

        case IT_DNADOORCHARGE:
            item = charge_dnadoor_create();
            break;

        case IT_HDNADOOR:
            item = surge_horizontal_dnadoor_create();
            break;

        case IT_HDNADOORNEON:
            item = neon_horizontal_dnadoor_create();
            break;

        case IT_HDNADOORCHARGE:
            item = charge_horizontal_dnadoor_create();
            break;

        case IT_LOOPGREEN:
            item = loopgreen_create();
            break;

        case IT_LOOPYELLOW:
            item = loopyellow_create();
            break;
    }

    if(item != NULL) {
        item->type = type;
        item->state = IS_IDLE;
        item->init(item);
        item->mask = item->obstacle ? collisionmask_create_box(
            image_width(actor_image(item->actor)),
            image_height(actor_image(item->actor))
        ) : NULL;
    }
    else
        fatal_error("Can't create item %d: item not found", type);

    return item;
}



/*
 * item_destroy()
 * Destroys an item
 */
item_t* item_destroy(item_t *item)
{
    if(item->mask != NULL)
        collisionmask_destroy(item->mask);
    item->release(item);
    free(item);
    return NULL;
}




/*
 * item_render()
 * Renders an item
 */
void item_render(item_t *item, v2d_t camera_position)
{
    item->render(item, camera_position);
}



/*
 * item_update()
 * Runs every cycle of the game to update an item
 */
void item_update(item_t *item, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, enemy_list_t *enemy_list)
{
    item->update(item, team, team_size, brick_list, item_list, enemy_list);
}


/* ============ private utilities ============== */

/*
 * find_closest_item()
 * Finds the closest item (minimal distance) of
 * a given type relative to 'me'. Returns NULL
 * if nothing nice is found.
 */
item_t *find_closest_item(item_t *me, item_list_t *list, int desired_type, float *distance)
{
    float min_dist = INFINITY;
    item_list_t *it;
    item_t *ret = NULL;
    v2d_t v;

    for(it=list; it; it=it->next) { /* this list must be small enough */
        if(it->data->type == desired_type) {
            v = v2d_subtract(it->data->actor->position, me->actor->position);
            if(v2d_magnitude(v) < min_dist) {
                ret = it->data;
                min_dist = v2d_magnitude(v);
            }
        }
    }

    if(distance)
        *distance = min_dist;

    return ret;
}




/* ============ legacy item code ============== */


/* (little) animal class */
typedef struct animal_t animal_t;
struct animal_t {
    item_t item; /* base class */
    int animal_id;
    int is_running;
};

static void animal_init(item_t *item);
static void animal_release(item_t* item);
static void animal_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void animal_render(item_t* item, v2d_t camera_position);

/* public methods */
item_t* animal_create()
{
    item_t *item = mallocx(sizeof(animal_t));

    item->init = animal_init;
    item->release = animal_release;
    item->update = animal_update;
    item->render = animal_render;

    return item;
}


/* private methods */
void animal_init(item_t *item)
{
    const int MAX_ANIMALS = 12;
    animal_t *me = (animal_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = FALSE;
    item->actor = actor_create();
    item->actor->speed.x = (random(2) ? 1 : -1) * (45 + random(21));

    me->is_running = FALSE;
    me->animal_id = random(MAX_ANIMALS);
    actor_change_animation(item->actor, sprite_get_animation("SD_ANIMAL", 0));
}



void animal_release(item_t* item)
{
    actor_destroy(item->actor);
}



void animal_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    animal_t *me = (animal_t*)item;
    actor_t *act = item->actor;
    int animation_id = 2*me->animal_id + (me->is_running?1:0);
    float dt = timer_get_delta(), g = level_gravity();

    /* in order to avoid too much processor load,
       we adopt this simplified platform system */
    int rx, ry, rw, rh, bx, by, bw, bh, j;
    const image_t *ri;
    brick_list_t *it;
    enum { NONE, FLOOR, RIGHTWALL, CEILING, LEFTWALL } bounce = NONE;
    const obstacle_t* bo;

    ri = actor_image(act);
    rx = (int)(act->position.x - act->hot_spot.x);
    ry = (int)(act->position.y - act->hot_spot.y);
    rw = image_width(ri);
    rh = image_height(ri);

    /* check for collisions */
    for(it = brick_list; it != NULL && bounce == NONE; it = it->next) {
        bo = brick_obstacle(it->data);
        if(bo && brick_type(it->data) != BRK_PASSABLE) {
            bx = brick_position(it->data).x;
            by = brick_position(it->data).y;
            bw = brick_size(it->data).x;
            bh = brick_size(it->data).y;

            if(rx<bx+bw && rx+rw>bx && ry<by+bh && ry+rh>by) {
                if(obstacle_got_collision(bo, rx, ry+rh/2, rx, ry+rh/2)) {
                    /* left wall */
                    bounce = LEFTWALL;
                    for(j=1; j<=bw; j++) {
                        if(!obstacle_got_collision(bo, rx+j, ry, rx+j, ry)) {
                            act->position.x += j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw-1, ry+rh/2, rx+rw-1, ry+rh/2)) {
                    /* right wall */
                    bounce = RIGHTWALL;
                    for(j=1; j<=bw; j++) {
                        if(!obstacle_got_collision(bo, rx-j, ry, rx-j, ry)) {
                            act->position.x -= j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw/2, ry, rx+rw/2, ry)) {
                    /* ceiling */
                    bounce = CEILING;
                    for(j=1; j<=bh; j++) {
                        if(!obstacle_got_collision(bo, rx, ry+j, rx, ry+j)) {
                            act->position.y += j-1;
                            break;
                        }
                    }
                }
                else if(obstacle_got_collision(bo, rx+rw/2, ry+rh-1, rx+rw/2, ry+rh-1)) {
                    /* floor */
                    bounce = FLOOR;
                    for(j=1; j<=bh; j++) {
                        if(!obstacle_got_collision(bo, rx, ry-j, rx, ry-j)) {
                            act->position.y -= j-1;
                            break;
                        }
                    }
                }
            }
        }
    }

    /* bounce & gravity */
    switch(bounce) {
        case FLOOR:
            me->is_running = TRUE;
            if(act->speed.y > 0.0f)
                act->speed.y = -240.0f-random(27);
            break;

        case RIGHTWALL:
            if(act->speed.x > 0.0f)
                act->speed.x *= -1.0f;
            break;

        case LEFTWALL:
            if(act->speed.x < 0.0f)
                act->speed.x *= -1.0f;
            break;

        case CEILING:
            if(act->speed.y < 0.0f)
                act->speed.y *= -0.25f;
            break;

        default:
            act->speed.y += g * dt;
            break;
    }

    /* move */
    if(me->is_running)
        act->position.x += act->speed.x * dt;
    act->position.y += act->speed.y * dt;

    /* animation */
    act->mirror = (act->speed.x >= 0.0f) ? IF_NONE : IF_HFLIP;
    actor_change_animation(item->actor, sprite_get_animation("SD_ANIMAL", animation_id));
}


void animal_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* legacy item: animal prison */


/*
   The animal prison is that object you hit at the end of the level
   in order to free the little animals. This is state machine:

   IDLE ---> EXPLODING ---> RELEASING THE ANIMALS ---> BROKEN

   We implement the State design pattern.
*/



/* Basic state (abstract) */
typedef struct state_t state_t; /* abstract class */
struct state_t {
    void (*handle)(state_t*,item_t*,player_t**,int); /* receives: state, animalprison, team, team_size */
};

/* Idle state: waiting to be hit... */
typedef struct state_idle_t state_idle_t; /* concrete state: idle */
struct state_idle_t {
    state_t state; /* base class */
    int being_hit; /* am I being hit? */
    int hit_count; /* the player has hit me 'hit_count' times */
};

static state_t* state_idle_new(); /* state constructor */
static void state_idle_handle(state_t*,item_t*,player_t**,int); /* private method */

/* After the animal prison got hit a few times, it explodes for a little while */
typedef struct state_exploding_t state_exploding_t; /* concrete state: exploding */
struct state_exploding_t {
    state_t state; /* base class */
    float explode_timer; /* explosion time accumulator */
    float break_timer; /* how long until it breaks... */
};

static state_t* state_exploding_new(); /* state constructor */
static void state_exploding_handle(state_t*,item_t*,player_t**,int); /* private method */

/* After it explodes, it must release the little animals ;) */
typedef struct state_releasing_t state_releasing_t; /* concrete state: releasing */
struct state_releasing_t {
    state_t state;
};

static state_t* state_releasing_new(); /* state constructor */
static void state_releasing_handle(state_t*,item_t*,player_t**,int); /* private method */

/* This is finally broken */
typedef struct state_broken_t state_broken_t; /* concrete state: broken */
struct state_broken_t {
    state_t state;
};

static state_t* state_broken_new(); /* state constructor */
static void state_broken_handle(state_t*,item_t*,player_t**,int); /* private method */



/* animalprison class */
typedef struct animalprison_t animalprison_t;
struct animalprison_t {
    item_t item; /* base class */
    state_t *state; /* state pattern */
};

static void animalprison_init(item_t *item);
static void animalprison_release(item_t* item);
static void animalprison_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void animalprison_render(item_t* item, v2d_t camera_position);

static void animalprison_set_state(item_t *item, state_t *state);
static int animalprison_got_hit_by_player(item_t *item, player_t *player);



/* public methods */
item_t* animalprison_create()
{
    item_t *item = mallocx(sizeof(animalprison_t));
    animalprison_t *me = (animalprison_t*)item;

    item->init = animalprison_init;
    item->release = animalprison_release;
    item->update = animalprison_update;
    item->render = animalprison_render;

    me->state = NULL;

    return item;
}



/* private methods */
void animalprison_set_state(item_t *item, state_t *state)
{
    animalprison_t *me = (animalprison_t*)item;

    if(me->state != NULL)
        free(me->state);

    me->state = state;
}

void animalprison_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    animalprison_set_state(item, state_idle_new());
    actor_change_animation(item->actor, sprite_get_animation("SD_ENDLEVEL", 0));
}

void animalprison_release(item_t* item)
{
    actor_destroy(item->actor);
    animalprison_set_state(item, NULL);
}

void animalprison_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    animalprison_t *me = (animalprison_t*)item;
    me->state->handle(me->state, item, team, team_size);
}

void animalprison_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* states: constructors */
state_t* state_idle_new()
{
    state_t *base = mallocx(sizeof(state_idle_t));
    state_idle_t *derived = (state_idle_t*)base;

    base->handle = state_idle_handle;
    derived->being_hit = FALSE;
    derived->hit_count = 0;

    return base;
}

state_t* state_exploding_new()
{
    state_t *base = mallocx(sizeof(state_exploding_t));
    state_exploding_t *derived = (state_exploding_t*)base;

    base->handle = state_exploding_handle;
    derived->explode_timer = 0.0f;
    derived->break_timer = 0.0f;

    return base;
}

state_t* state_releasing_new()
{
    state_t *base = mallocx(sizeof(state_releasing_t));
    base->handle = state_releasing_handle;
    return base;
}

state_t* state_broken_new()
{
    state_t *base = mallocx(sizeof(state_broken_t));
    base->handle = state_broken_handle;
    return base;
}

/* implementation of the states */
void state_idle_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    int i;
    state_idle_t *s = (state_idle_t*)state;
    actor_t *act = item->actor;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(animalprison_got_hit_by_player(item, player) && !s->being_hit) {
            /* oh no! the player is attacking this object! */
            s->being_hit = TRUE;
            actor_change_animation(act, sprite_get_animation("SD_ENDLEVEL", 1));
            sound_play(SFX_BOSSHIT);
            player_bounce_ex(player, act, FALSE);
            player->actor->speed.x *= -0.5;

            if(++(s->hit_count) >= 3) /* 3 hits and you're done */
                animalprison_set_state(item, state_exploding_new());
        }
    }

    /* after getting hit, restore the animation */
    if(actor_animation_finished(act) && s->being_hit) {
        actor_change_animation(act, sprite_get_animation("SD_ENDLEVEL", 0));
        s->being_hit = FALSE;
    }
}

void state_exploding_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    state_exploding_t *s = (state_exploding_t*)state;
    actor_t *act = item->actor;
    float dt = timer_get_delta();

    s->explode_timer += dt;
    s->break_timer += dt;

    /* keep exploding for a while... */
    if(s->explode_timer >= 0.1f) {
        v2d_t pos = v2d_new(
            act->position.x - act->hot_spot.x + random(image_width(actor_image(act))),
            act->position.y - act->hot_spot.y + random(image_height(actor_image(act))/2)
        );
        level_create_legacy_item(IT_EXPLOSION, pos);
        sound_play(SFX_EXPLODE);

        s->explode_timer = 0.0f;
    }

    /* okay, I can't explode anymore */
    if(s->break_timer >= 2.0f)
        animalprison_set_state(item, state_releasing_new());
}

void state_releasing_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    actor_t *act = item->actor;
    int i;

    /* release the animals! */
    for(i=0; i<20; i++) {
        v2d_t pos = v2d_new(
            act->position.x - act->hot_spot.x + random(image_width(actor_image(act))),
            act->position.y - act->hot_spot.y + random(image_height(actor_image(act))/2)
        );
        level_create_legacy_item(IT_ANIMAL, pos);
    }

    /* congratulations! you have just cleared the level! */
    level_clear(act);

    /* sayonara bye bye */
    actor_change_animation(act, sprite_get_animation("SD_ENDLEVEL", 2));
    animalprison_set_state(item, state_broken_new());
}

void state_broken_handle(state_t *state, item_t *item, player_t **team, int team_size)
{
    ; /* do nothing */
}

/* misc */
/* returns true if the animal prison got hit by the given player */
int animalprison_got_hit_by_player(item_t *item, player_t *player)
{
    float a[4], b[4];
    actor_t *act = item->actor;
    actor_t *pl = player->actor;

    a[0] = pl->position.x - pl->hot_spot.x;
    a[1] = pl->position.y - pl->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(pl));
    a[3] = a[1] + image_height(actor_image(pl));

    b[0] = act->position.x - act->hot_spot.x + 5;
    b[1] = act->position.y - act->hot_spot.y;
    b[2] = b[0] + image_width(actor_image(act)) - 10;
    b[3] = b[1] + image_height(actor_image(act))/2;

    return (player_is_attacking(player) && bounding_box(a,b) && player_collision(player, act));
}




/* bigring class */
typedef struct bigring_t bigring_t;
struct bigring_t {
    item_t item; /* base class */
};

static void bigring_init(item_t *item);
static void bigring_release(item_t* item);
static void bigring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void bigring_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* bigring_create()
{
    item_t *item = mallocx(sizeof(bigring_t));

    item->init = bigring_init;
    item->release = bigring_release;
    item->update = bigring_update;
    item->render = bigring_render;

    return item;
}


/* private methods */
void bigring_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_BIGRING", 0));
    actor_synchronize_animation(item->actor, TRUE);
}



void bigring_release(item_t* item)
{
    actor_destroy(item->actor);
}



void bigring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && player_collision(player, item->actor)) {
            item->state = IS_DEAD;
            player_set_collectibles( player_get_collectibles() + 50 );
            sound_play(SFX_BONUS);
        }
    }
}


void bigring_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}





/* bouncingcollectible class */
typedef struct bouncingcollectible_t bouncingcollectible_t;
struct bouncingcollectible_t {
    item_t item; /* base class */
    int is_disappearing; /* is this bouncing collectible disappearing? */
    float life_time; /* life time (used to destroy the moving bouncing collectible after some time) */
};

static void bouncingcollectible_init(item_t *item);
static void bouncingcollectible_release(item_t* item);
static void bouncingcollectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void bouncingcollectible_render(item_t* item, v2d_t camera_position);

/* public methods */
item_t* bouncingcollectible_create()
{
    item_t *item = mallocx(sizeof(bouncingcollectible_t));

    item->init = bouncingcollectible_init;
    item->release = bouncingcollectible_release;
    item->update = bouncingcollectible_update;
    item->render = bouncingcollectible_render;

    return item;
}

void bouncingcollectible_set_velocity(item_t *item, v2d_t velocity)
{
    item->actor->speed = velocity;
}



/* private methods */
void bouncingcollectible_init(item_t *item)
{
    bouncingcollectible_t *me = (bouncingcollectible_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = FALSE;
    item->actor = actor_create();

    me->is_disappearing = FALSE;
    me->life_time = 0.0f;

    actor_change_animation(item->actor, sprite_get_animation("SD_COLLECTIBLE", 0));
}



void bouncingcollectible_release(item_t* item)
{
    actor_destroy(item->actor);
}



void bouncingcollectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    float dt = timer_get_delta();
    bouncingcollectible_t *me = (bouncingcollectible_t*)item;
    actor_t *act = item->actor;
    sound_t *sfx = SFX_COLLECTIBLE;

    /* a player has just got this bouncing collectible */
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(
            me->life_time >= 1.067f && 
            !me->is_disappearing &&
            !player_is_dying(player) &&
            player_collision(player, act)
        ) {
            player_set_collectibles(player_get_collectibles() + 1);
            me->is_disappearing = TRUE;
            sound_stop(sfx);
            sound_play(sfx);
            break;
        }
    }

    /* disappearing animation... */
    if(me->is_disappearing) {
        item->bring_to_back = FALSE;
        actor_change_animation(act, sprite_get_animation("SD_COLLECTIBLE", 1));
        if(actor_animation_finished(act))
            item->state = IS_DEAD;
    }

    /* this ring is bouncing around... */
    else {
        /* in order to avoid too much processor load,
           we adopt this simplified platform system */
        int rx, ry, rw, rh, bx, by, bw, bh, j;
        const image_t *ri;
        brick_list_t *it;
        enum { NONE, FLOOR, RIGHTWALL, CEILING, LEFTWALL } bounce = NONE;
        const obstacle_t* bo;

        ri = actor_image(act);
        rx = (int)(act->position.x - act->hot_spot.x);
        ry = (int)(act->position.y - act->hot_spot.y);
        rw = image_width(ri);
        rh = image_height(ri);

        /* who wants to live forever? */
        if((me->life_time += dt) > 4.267f)
            item->state = IS_DEAD;

        /* check for collisions */
        for(it = brick_list; it != NULL && bounce == NONE; it = it->next) {
            bo = brick_obstacle(it->data);
            if(bo && brick_type(it->data) != BRK_PASSABLE) {
                bx = brick_position(it->data).x;
                by = brick_position(it->data).y;
                bw = brick_size(it->data).x;
                bh = brick_size(it->data).y;

                if(rx<bx+bw && rx+rw>bx && ry<by+bh && ry+rh>by) {
                    if(obstacle_got_collision(bo, rx, ry+rh/2, rx, ry+rh/2)) {
                        /* left wall */
                        bounce = LEFTWALL;
                        for(j=1; j<=bw; j++) {
                            if(!obstacle_got_collision(bo, rx+j, ry, rx+j, ry)) {
                                act->position.x += j-1;
                                break;
                            }
                        }
                    }
                    else if(obstacle_got_collision(bo, rx+rw-1, ry+rh/2, rx+rw-1, ry+rh/2)) {
                        /* right wall */
                        bounce = RIGHTWALL;
                        for(j=1; j<=bw; j++) {
                            if(!obstacle_got_collision(bo, rx-j, ry, rx-j, ry)) {
                                act->position.x -= j-1;
                                break;
                            }
                        }
                    }
                    else if(obstacle_got_collision(bo, rx+rw/2, ry, rx+rw/2, ry)) {
                        /* ceiling */
                        bounce = CEILING;
                        for(j=1; j<=bh; j++) {
                            if(!obstacle_got_collision(bo, rx, ry+j, rx, ry+j)) {
                                act->position.y += j-1;
                                break;
                            }
                        }
                    }
                    else if(obstacle_got_collision(bo, rx+rw/2, ry+rh-1, rx+rw/2, ry+rh-1)) {
                        /* floor */
                        bounce = FLOOR;
                        for(j=1; j<=bh; j++) {
                            if(!obstacle_got_collision(bo, rx, ry-j, rx, ry-j)) {
                                act->position.y -= j-1;
                                break;
                            }
                        }
                    }
                }
            }
        }

        /* bounce & gravity */
        switch(bounce) {
            case FLOOR:
                if(act->speed.y > 0.0f)
                    act->speed.y *= act->speed.y > 1.0f ? -0.75f : 0.0f;
                break;

            case RIGHTWALL:
                if(act->speed.x > 0.0f)
                    act->speed.x *= -0.25f;
                break;

            case LEFTWALL:
                if(act->speed.x < 0.0f)
                    act->speed.x *= -0.25f;
                break;

            case CEILING:
                if(act->speed.y < 0.0f)
                    act->speed.y *= -0.25f;
                break;

            default:
                act->speed.y += 337.5f * dt;
                break;
        }

        /* move */
        act->position.x += act->speed.x * dt;
        act->position.y += act->speed.y * dt;
    }
}


void bouncingcollectible_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}





/* bumper class */
typedef struct bumper_t bumper_t;
struct bumper_t {
    item_t item; /* base class */
    int getting_hit;
};

static void bumper_init(item_t *item);
static void bumper_release(item_t* item);
static void bumper_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void bumper_render(item_t* item, v2d_t camera_position);

static void bump(item_t *bumper, player_t *player);


/* public methods */
item_t* bumper_create()
{
    item_t *item = mallocx(sizeof(bumper_t));

    item->init = bumper_init;
    item->release = bumper_release;
    item->update = bumper_update;
    item->render = bumper_render;

    return item;
}


/* private methods */
void bumper_init(item_t *item)
{
    bumper_t *me = (bumper_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->getting_hit = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_BUMPER", 0));
}



void bumper_release(item_t* item)
{
    actor_destroy(item->actor);
}



void bumper_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    bumper_t *me = (bumper_t*)item;
    actor_t *act = item->actor;
    int i;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && player_collision(player, act)) {
            if(!me->getting_hit) {
                me->getting_hit = TRUE;
                actor_change_animation(act, sprite_get_animation("SD_BUMPER", 1));
                sound_play(SFX_BUMPER);
                bump(item, player);
            }
        }
    }

    if(me->getting_hit) {
        if(actor_animation_finished(act)) {
            me->getting_hit = FALSE;
            actor_change_animation(act, sprite_get_animation("SD_BUMPER", 0));
        }
    }
}


void bumper_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* misc */
void bump(item_t *bumper, player_t *player)
{
    /* law of conservation of linear momentum */
    float ec = 1.0f; /* (coefficient of restitution == 1.0) => elastic collision */
    float mass_player = 1.0f;
    float mass_bumper = 10000.0f;
    float mass_ratio = mass_bumper / mass_player;
    v2d_t v0, approximation_speed, separation_speed;
    actor_t *act = bumper->actor;



    v0 = player->actor->speed; /* initial speed of the player */
    v0.x = (v0.x < 0) ? min(-300, v0.x) : max(300, v0.x);



    approximation_speed = v2d_multiply(
        v2d_normalize(
            v2d_subtract(act->position, player->actor->position)
        ),
        v2d_magnitude(v0)
    );

    separation_speed = v2d_multiply(approximation_speed, ec);



    player->actor->speed = v2d_multiply(
        v2d_add(
            v0,
            v2d_multiply(separation_speed, -mass_ratio)
        ),
        1.0f / (mass_ratio + 1.0f)
    );

    act->speed = v2d_multiply(
        v2d_add(v0, separation_speed),
        1.0f / (mass_ratio + 1.0f)
    );
}




/* checkpointorb class */
typedef struct checkpointorb_t checkpointorb_t;
struct checkpointorb_t {
    item_t item; /* base class */
    int is_active; /* has this checkpoint orb been touched? */
};

static void checkpointorb_init(item_t *item);
static void checkpointorb_release(item_t* item);
static void checkpointorb_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void checkpointorb_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* checkpointorb_create()
{
    item_t *item = mallocx(sizeof(checkpointorb_t));

    item->init = checkpointorb_init;
    item->release = checkpointorb_release;
    item->update = checkpointorb_update;
    item->render = checkpointorb_render;

    return item;
}


/* private methods */
void checkpointorb_init(item_t *item)
{
    checkpointorb_t *me = (checkpointorb_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_active = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_CHECKPOINT", 0));
}



void checkpointorb_release(item_t* item)
{
    actor_destroy(item->actor);
}



void checkpointorb_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    checkpointorb_t *me = (checkpointorb_t*)item;
    actor_t *act = item->actor;
    int i;

    if(!me->is_active) {
        /* activating the checkpoint orb... */
        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(!player_is_dying(player) && player_collision(player, act)) {
                me->is_active = TRUE; /* I'm active! */
                sound_play(SFX_CHECKPOINT);
                level_set_spawnpoint(act->position);
                level_save_state();
                actor_change_animation(act, sprite_get_animation("SD_CHECKPOINT", 1));
                break;
            }
        }
    }
    else {
        if(actor_animation_finished(act))
            actor_change_animation(act, sprite_get_animation("SD_CHECKPOINT", 2));
    }
}


void checkpointorb_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* collectible class */
typedef struct collectible_t collectible_t;
struct collectible_t {
    item_t item; /* base class */
    int is_disappearing; /* is this collectible disappearing? */
};

static void collectible_init(item_t *item);
static void collectible_release(item_t* item);
static void collectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void collectible_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* collectible_create()
{
    item_t *item = mallocx(sizeof(collectible_t));

    item->init = collectible_init;
    item->release = collectible_release;
    item->update = collectible_update;
    item->render = collectible_render;

    return item;
}



/* private methods */
void collectible_init(item_t *item)
{
    collectible_t *me = (collectible_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_disappearing = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_COLLECTIBLE", 0));
    actor_synchronize_animation(item->actor, TRUE);
}



void collectible_release(item_t* item)
{
    actor_destroy(item->actor);
}



void collectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    float dt = timer_get_delta();
    collectible_t *me = (collectible_t*)item;
    actor_t *act = item->actor;
    sound_t *sfx = SFX_COLLECTIBLE;

    /* a player has just got this collectible */
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(
            !me->is_disappearing &&
            !player_is_dying(player) &&
            player_collision(player, act)
        ) {
            player_set_collectibles(player_get_collectibles() + 1);
            me->is_disappearing = TRUE;
            item->bring_to_back = FALSE;
            sound_stop(sfx);
            sound_play(sfx);
            break;
        }
    }

    /* disappearing animation... */
    if(me->is_disappearing) {
        actor_change_animation(act, sprite_get_animation("SD_COLLECTIBLE", 1));
        if(actor_animation_finished(act))
            item->state = IS_DEAD;
    }

    /* this collectible is being attracted by the thunder shield */
    else {
        float mindist = 160.0f;
        player_t *attracted_by = NULL;

        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(player_shield_type(player) == SH_THUNDERSHIELD) {
                float d = v2d_magnitude(v2d_subtract(act->position, player->actor->position));
                if(d < mindist) {
                    attracted_by = player;
                    mindist = d;
                }
            }
        }

        if(attracted_by != NULL) {
            float speed = 320.0f;
            v2d_t diff = v2d_subtract(attracted_by->actor->position, act->position);
            v2d_t d = v2d_multiply(v2d_normalize(diff), speed);
            act->position.x += d.x * dt;
            act->position.y += d.y * dt;
        }
    }
}


void collectible_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* crushedbox class */
typedef struct crushedbox_t crushedbox_t;
struct crushedbox_t {
    item_t item; /* base class */
};

static void crushedbox_init(item_t *item);
static void crushedbox_release(item_t* item);
static void crushedbox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void crushedbox_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* crushedbox_create()
{
    item_t *item = mallocx(sizeof(crushedbox_t));

    item->init = crushedbox_init;
    item->release = crushedbox_release;
    item->update = crushedbox_update;
    item->render = crushedbox_render;

    return item;
}


/* private methods */
void crushedbox_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_ITEMBOX", 10));
}



void crushedbox_release(item_t* item)
{
    actor_destroy(item->actor);
}



void crushedbox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    ; /* empty */
}


void crushedbox_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* danger class */
typedef struct danger_t danger_t;
struct danger_t {
    item_t item; /* base class */
    char *sprite_name;
    int (*player_is_vulnerable)(player_t*);
};

static item_t* danger_create(const char *sprite_name, int (*player_is_vulnerable)(player_t*));
static void danger_init(item_t *item);
static void danger_release(item_t* item);
static void danger_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void danger_render(item_t* item, v2d_t camera_position);

static int always_vulnerable(player_t *player);
static int can_defend_against_fire(player_t *player);


/* public methods */
item_t* horizontaldanger_create()
{
    return danger_create("SD_DANGER", always_vulnerable);
}

item_t* verticaldanger_create()
{
    return danger_create("SD_VERTICALDANGER", always_vulnerable);
}

item_t* horizontalfiredanger_create()
{
    return danger_create("SD_FIREDANGER", can_defend_against_fire);
}

item_t* verticalfiredanger_create()
{
    return danger_create("SD_VERTICALFIREDANGER", can_defend_against_fire);
}


/* private methods */
item_t* danger_create(const char *sprite_name, int (*player_is_vulnerable)(player_t*))
{
    item_t *item = mallocx(sizeof(danger_t));
    danger_t *me = (danger_t*)item;

    item->init = danger_init;
    item->release = danger_release;
    item->update = danger_update;
    item->render = danger_render;

    me->sprite_name = str_dup(sprite_name);
    me->player_is_vulnerable = player_is_vulnerable;

    return item;
}

void danger_init(item_t *item)
{
    danger_t *me = (danger_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));
}



void danger_release(item_t* item)
{
    danger_t *me = (danger_t*)item;

    actor_destroy(item->actor);
    free(me->sprite_name);
}



void danger_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    danger_t *me = (danger_t*)item;
    actor_t *act = item->actor;

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && !player_is_blinking(player) && !player_is_invincible(player) && player_collision(player, act)) {
            if(me->player_is_vulnerable(player))
                player_hit_ex(player, act);
        }
    }

    act->visible = level_editmode();
}


void danger_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* misc */
int always_vulnerable(player_t *player)
{
    return TRUE;
}

int can_defend_against_fire(player_t *player)
{
    return (player_shield_type(player) != SH_FIRESHIELD);
}


/* dnadoor class */
typedef struct dnadoor_t dnadoor_t;
struct dnadoor_t {
    item_t item; /* base class */
    char *authorized_player_name;
    int is_vertical_door;
};

static item_t *dnadoor_create(const char *authorized_player_name, int is_vertical_door);
static void dnadoor_init(item_t *item);
static void dnadoor_release(item_t* item);
static void dnadoor_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void dnadoor_render(item_t* item, v2d_t camera_position);

static int dnadoor_hittest(player_t *player, item_t *dnadoor);


/* public methods */
item_t* surge_dnadoor_create()
{
    return dnadoor_create("Surge", TRUE);
}

item_t* neon_dnadoor_create()
{
    return dnadoor_create("Neon", TRUE);
}

item_t* charge_dnadoor_create()
{
    return dnadoor_create("Charge", TRUE);
}

item_t* surge_horizontal_dnadoor_create()
{
    return dnadoor_create("Surge", FALSE);
}

item_t* neon_horizontal_dnadoor_create()
{
    return dnadoor_create("Neon", FALSE);
}

item_t* charge_horizontal_dnadoor_create()
{
    return dnadoor_create("Charge", FALSE);
}


/* private methods */
item_t* dnadoor_create(const char *authorized_player_name, int is_vertical_door)
{
    item_t *item = mallocx(sizeof(dnadoor_t));
    dnadoor_t *me = (dnadoor_t*)item;

    item->init = dnadoor_init;
    item->release = dnadoor_release;
    item->update = dnadoor_update;
    item->render = dnadoor_render;

    me->authorized_player_name = str_dup(authorized_player_name);
    me->is_vertical_door = is_vertical_door;

    return item;
}


void dnadoor_init(item_t *item)
{
    dnadoor_t *me = (dnadoor_t*)item;
    char *sprite_name;
    int anim_id = 0;

    item->always_active = FALSE;
    item->obstacle = TRUE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    if(str_icmp(me->authorized_player_name, "Surge") == 0)
        anim_id = 0;
    else if(str_icmp(me->authorized_player_name, "Neon") == 0)
        anim_id = 1;
    else if(str_icmp(me->authorized_player_name, "Charge") == 0)
        anim_id = 2;

    sprite_name = me->is_vertical_door ? "SD_DNADOOR" : "SD_HORIZONTALDNADOOR";
    actor_change_animation(item->actor, sprite_get_animation(sprite_name, anim_id));
    actor_synchronize_animation(item->actor, TRUE);
}



void dnadoor_release(item_t* item)
{
    dnadoor_t *me = (dnadoor_t*)item;
    free(me->authorized_player_name);
    actor_destroy(item->actor);
}



void dnadoor_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    dnadoor_t *me = (dnadoor_t*)item;
    actor_t *act = item->actor;
    item_list_t *it;
    int block_anyway = FALSE;
    int collision = FALSE;
    float dt = timer_get_delta();
    float a[4], b[4], diff=5;
    int i;

    /* should we block the DNA Door? */
    item->obstacle = TRUE;
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && dnadoor_hittest(player, item)) {
            if(str_icmp(player->name, me->authorized_player_name) == 0) {
                item->obstacle = FALSE;
                collision = player_collision(player, act);
            }
            else
                block_anyway = TRUE;
        }
    }
    if(block_anyway)
        item->obstacle = TRUE;

    /* cute effect */
    if(item->obstacle)
        act->alpha = min(1.0f, act->alpha + 2.0f * dt);
    else if(collision)
        act->alpha = max(0.4f, act->alpha - 2.0f * dt);

    /* cute effect propagation */
    if(collision) {
        a[0] = act->position.x - act->hot_spot.x - diff;
        a[1] = act->position.y - act->hot_spot.y - diff;
        a[2] = a[0] + image_width(actor_image(act)) + 2*diff;
        a[3] = a[1] + image_height(actor_image(act)) + 2*diff;
        for(it = item_list; it != NULL; it = it->next) {
            if(it->data->type == item->type) {
                b[0] = it->data->actor->position.x - it->data->actor->hot_spot.x - diff;
                b[1] = it->data->actor->position.y - it->data->actor->hot_spot.y - diff;
                b[2] = b[0] + image_width(actor_image(it->data->actor)) + 2*diff;
                b[3] = b[1] + image_height(actor_image(it->data->actor)) + 2*diff;
                if(bounding_box(a,b)) {
                    if(it->data->actor->alpha < act->alpha)
                        act->alpha = it->data->actor->alpha;
                    else
                        it->data->actor->alpha = act->alpha;
                }
            }
        }
    }
}


void dnadoor_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* misc */

/*
as the bounding boxes must a bit larger than usual,
we use some fancy logic to test the collision between
the player and the DNA door.
we don't use a regular actor_collision() test here.
*/
int dnadoor_hittest(player_t *player, item_t *dnadoor)
{
    float a[4], b[4];
    int offset = 3;
    actor_t *pl = player->actor;
    actor_t *act = dnadoor->actor;

    a[0] = pl->position.x - pl->hot_spot.x;
    a[1] = pl->position.y - pl->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(pl));
    a[3] = a[1] + image_height(actor_image(pl));

    b[0] = act->position.x - act->hot_spot.x;
    b[1] = act->position.y - act->hot_spot.y - offset;
    b[2] = b[0] + image_width(actor_image(act));
    b[3] = b[1] + image_height(actor_image(act)) + offset;

    return bounding_box(a, b);
}



/* door class */
typedef struct door_t door_t;
struct door_t {
    item_t item; /* base class */
    int is_closed; /* is the door closed? */
};

static void door_init(item_t *item);
static void door_release(item_t* item);
static void door_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void door_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* door_create()
{
    item_t *item = mallocx(sizeof(door_t));

    item->init = door_init;
    item->release = door_release;
    item->update = door_update;
    item->render = door_render;

    return item;
}

void door_open(item_t *door)
{
    door_t *me = (door_t*)door;
    me->is_closed = FALSE;
    sound_play(SFX_DOOROPEN);
}

void door_close(item_t *door)
{
    door_t *me = (door_t*)door;
    me->is_closed = TRUE;
    sound_play(SFX_DOORCLOSE);
}


/* private methods */
void door_init(item_t *item)
{
    door_t *me = (door_t*)item;

    item->always_active = TRUE;
    item->obstacle = TRUE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_closed = TRUE;
    actor_change_animation(item->actor, sprite_get_animation("SD_DOOR", 0));
}



void door_release(item_t* item)
{
    actor_destroy(item->actor);
}



void door_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    door_t *me = (door_t*)item;
    actor_t *act = item->actor;
    float speed = 2000.0f;
    float dt = timer_get_delta();

    if(me->is_closed)
        act->position.y = min(act->position.y + speed*dt, act->spawn_point.y);
    else    
        act->position.y = max(act->position.y - speed*dt, act->spawn_point.y - image_height(actor_image(act)) * 0.8);
}


void door_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* endsign class */
typedef struct endsign_t endsign_t;
struct endsign_t {
    item_t item; /* base class */
    player_t *who; /* who has touched the end sign? */
};

static void endsign_init(item_t *item);
static void endsign_release(item_t* item);
static void endsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void endsign_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* endsign_create()
{
    item_t *item = mallocx(sizeof(endsign_t));

    item->init = endsign_init;
    item->release = endsign_release;
    item->update = endsign_update;
    item->render = endsign_render;

    return item;
}


/* private methods */
void endsign_init(item_t *item)
{
    endsign_t *me = (endsign_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->who = NULL;
    actor_change_animation(item->actor, sprite_get_animation("SD_ENDSIGN", 0));
}



void endsign_release(item_t* item)
{
    actor_destroy(item->actor);
}



void endsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    endsign_t *me = (endsign_t*)item;
    actor_t *act = item->actor;

    if(me->who == NULL) {
        /* I haven't been touched yet */
        int i;

        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(!player_is_dying(player) && player_collision(player, act)) {
                me->who = player; /* I have just been touched by 'player' */
                sound_play(SFX_GOALSIGN);
                actor_change_animation(act, sprite_get_animation("SD_ENDSIGN", 1));
                level_clear(item->actor);
            }
        }
    }
    else {
        /* me->who has touched me! */
        if(actor_animation_finished(act)) {
            int anim_id;

            if(str_icmp(me->who->name, "Surge") == 0)
                anim_id = 2;
            else if(str_icmp(me->who->name, "Neon") == 0)
                anim_id = 3;
            else if(str_icmp(me->who->name, "Charge") == 0)
                anim_id = 4;
            else
                anim_id = 5;

            actor_change_animation(act, sprite_get_animation("SD_ENDSIGN", anim_id));
        }
    }
}


void endsign_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* explosion class */
typedef struct explosion_t explosion_t;
struct explosion_t {
    item_t item; /* base class */
};

static void explosion_init(item_t *item);
static void explosion_release(item_t* item);
static void explosion_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void explosion_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* explosion_create()
{
    item_t *item = mallocx(sizeof(explosion_t));

    item->init = explosion_init;
    item->release = explosion_release;
    item->update = explosion_update;
    item->render = explosion_render;

    return item;
}


/* private methods */
void explosion_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = FALSE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_EXPLOSION", 0));
}



void explosion_release(item_t* item)
{
    actor_destroy(item->actor);
}



void explosion_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    if(actor_animation_finished(item->actor))
        item->state = IS_DEAD;
}


void explosion_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* flyingtext class */
typedef struct flyingtext_t flyingtext_t;
struct flyingtext_t {
    item_t item; /* base class */
    font_t *font;
    float elapsed_time;
    v2d_t textsize;
};

static void flyingtext_init(item_t *item);
static void flyingtext_release(item_t* item);
static void flyingtext_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void flyingtext_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* flyingtext_create()
{
    item_t *item = mallocx(sizeof(flyingtext_t));

    item->init = flyingtext_init;
    item->release = flyingtext_release;
    item->update = flyingtext_update;
    item->render = flyingtext_render;

    return item;
}

void flyingtext_set_text(item_t *item, const char *fmt, ...)
{
    flyingtext_t *me = (flyingtext_t*)item;
    va_list args;
    char buf[256];

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    font_set_text(me->font, "%s", buf);
    me->textsize = font_get_textsize(me->font);
}


/* private methods */
void flyingtext_init(item_t *item)
{
    flyingtext_t *me = (flyingtext_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = FALSE;
    item->actor = actor_create();

    me->elapsed_time = 0.0f;
    me->font = font_create("default");
    flyingtext_set_text(item, "0");

    actor_change_animation(item->actor, sprite_get_animation(NULL, 0));
    item->actor->visible = false;
}



void flyingtext_release(item_t* item)
{
    flyingtext_t *me = (flyingtext_t*)item;

    actor_destroy(item->actor);
    font_destroy(me->font);
}



void flyingtext_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    flyingtext_t *me = (flyingtext_t*)item;
    float dt = timer_get_delta();

    me->elapsed_time += dt;
    if(me->elapsed_time < 0.5f)
        item->actor->position.y -= 100.0f * dt;
    else if(me->elapsed_time > 2.0f)
        item->state = IS_DEAD;

    font_set_position(me->font, v2d_subtract(item->actor->position, v2d_new(me->textsize.x/2, me->textsize.y/2)));
}


void flyingtext_render(item_t* item, v2d_t camera_position)
{
    flyingtext_t *me = (flyingtext_t*)item;
    font_render(me->font, camera_position);
}



/* goalsign class */
typedef struct goalsign_t goalsign_t;
struct goalsign_t {
    item_t item; /* base class */
};

static void goalsign_init(item_t *item);
static void goalsign_release(item_t* item);
static void goalsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void goalsign_render(item_t* item, v2d_t camera_position);

/* public methods */
item_t* goalsign_create()
{
    item_t *item = mallocx(sizeof(goalsign_t));

    item->init = goalsign_init;
    item->release = goalsign_release;
    item->update = goalsign_update;
    item->render = goalsign_render;

    return item;
}


/* private methods */
void goalsign_init(item_t *item)
{
    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_GOAL", 0));
}



void goalsign_release(item_t* item)
{
    actor_destroy(item->actor);
}



void goalsign_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    item_t *endsign;
    int anim;

    endsign = find_closest_item(item, item_list, IT_ENDSIGN, NULL);
    if(endsign != NULL) {
        if(endsign->actor->position.x > item->actor->position.x)
            anim = 0;
        else
            anim = 1;
    }
    else
        anim = 0;

    actor_change_animation(item->actor, sprite_get_animation("SD_GOAL", anim));
}


void goalsign_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* icon class */
typedef struct icon_t icon_t;
struct icon_t {
    item_t item; /* base class */
    float elapsed_time; /* elapsed time since this object has been created */
};

static void icon_init(item_t *item);
static void icon_release(item_t* item);
static void icon_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void icon_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* icon_create()
{
    item_t *item = mallocx(sizeof(icon_t));

    item->init = icon_init;
    item->release = icon_release;
    item->update = icon_update;
    item->render = icon_render;

    return item;
}

void icon_change_animation(item_t *item, int anim_id)
{
    actor_change_animation(item->actor, sprite_get_animation("SD_ICON", anim_id));
}


/* private methods */
void icon_init(item_t *item)
{
    icon_t *me = (icon_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = FALSE;
    item->actor = actor_create();

    me->elapsed_time = 0.0f;
    icon_change_animation(item, 0);
}



void icon_release(item_t* item)
{
    actor_destroy(item->actor);
}



void icon_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    icon_t *me = (icon_t*)item;
    actor_t *act = item->actor;
    float dt = timer_get_delta();

    me->elapsed_time += dt;
    if(me->elapsed_time < 1.0f) {
        /* rise */
        act->position.y -= 40.0f * dt;
    }
    else if(me->elapsed_time >= 2.5f) {
        /* death */
        int i, j;
        int x = (int)(act->position.x-act->hot_spot.x);
        int y = (int)(act->position.y-act->hot_spot.y);
        image_t *img = actor_image(act), *particle;
        int w = image_width(img), h = image_height(img);

        /* particle party! :) */
        for(i=0; i<h; i++) {
            for(j=0; j<w; j++) {
                particle = image_clone_region(img, j, i, 1, 1);
                level_create_particle(particle, v2d_new(x+j, y+i), v2d_new(
                    (j - w/2) * 2.0f + (random(w) - w/2),
                    (i - h/2) * 2.0f + (random(h) - h/2)
                ), FALSE);
            }
        }

        /* done */
        item->state = IS_DEAD;
    }
}


void icon_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* itembox class */
typedef struct itembox_t itembox_t;
struct itembox_t {
    item_t item; /* base class */
    int anim_id; /* animation id */
    void (*on_destroy)(item_t*,player_t*); /* strategy pattern */
};

static item_t* itembox_create(void (*on_destroy)(item_t*,player_t*), int anim_id);
static void itembox_init(item_t *item);
static void itembox_release(item_t* item);
static void itembox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void itembox_render(item_t* item, v2d_t camera_position);

static void lifebox_strategy(item_t *item, player_t *player);
static void collectiblebox_strategy(item_t *item, player_t *player);
static void starbox_strategy(item_t *item, player_t *player);
static void speedbox_strategy(item_t *item, player_t *player);
static void glassesbox_strategy(item_t *item, player_t *player);
static void shieldbox_strategy(item_t *item, player_t *player);
static void fireshieldbox_strategy(item_t *item, player_t *player);
static void thundershieldbox_strategy(item_t *item, player_t *player);
static void watershieldbox_strategy(item_t *item, player_t *player);
static void acidshieldbox_strategy(item_t *item, player_t *player);
static void windshieldbox_strategy(item_t *item, player_t *player);
static void trapbox_strategy(item_t *item, player_t *player);
static void emptybox_strategy(item_t *item, player_t *player);

static int get_anim_id(const char *player_name);


/* public methods */
item_t* lifebox_create()
{
    return itembox_create(lifebox_strategy, 0); /* (strategy, animation id) */
}

item_t* collectiblebox_create()
{
    return itembox_create(collectiblebox_strategy, 3);
}

item_t* starbox_create()
{
    return itembox_create(starbox_strategy, 4);
}

item_t* speedbox_create()
{ 
   return itembox_create(speedbox_strategy, 5);
}

item_t* glassesbox_create()
{
    return itembox_create(glassesbox_strategy, 6);
}

item_t* shieldbox_create()
{
    return itembox_create(shieldbox_strategy, 7);
}

item_t* trapbox_create()
{
    return itembox_create(trapbox_strategy, 8);
}

item_t* emptybox_create()
{
    return itembox_create(emptybox_strategy, 9);
}

item_t* fireshieldbox_create()
{
    return itembox_create(fireshieldbox_strategy, 11);
}

item_t* thundershieldbox_create()
{
    return itembox_create(thundershieldbox_strategy, 12);
}

item_t* watershieldbox_create()
{
    return itembox_create(watershieldbox_strategy, 13);
}

item_t* acidshieldbox_create()
{
    return itembox_create(acidshieldbox_strategy, 14);
}

item_t* windshieldbox_create()
{
    return itembox_create(windshieldbox_strategy, 15);
}


/* private strategies */
void lifebox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_set_lives( player_get_lives()+1 );
    sound_play(SFX_1UP);
}

void collectiblebox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_set_collectibles( player_get_collectibles()+10 );
    sound_play(SFX_COLLECTIBLE);
}

void starbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_set_invincible(player, TRUE);
    music_play(music_load("musics/invincible.ogg"), false);
}

void speedbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_set_turbo(player, TRUE);
    music_play(music_load("musics/speed.ogg"), false);
}

void glassesbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player->got_glasses = TRUE;
}

void shieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_grant_shield(player, SH_SHIELD);
    sound_play(SFX_SHIELD);
}

void fireshieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_grant_shield(player, SH_FIRESHIELD);
    sound_play(SFX_FIRESHIELD);
}

void thundershieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_grant_shield(player, SH_THUNDERSHIELD);
    sound_play(SFX_THUNDERSHIELD);
}

void watershieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_grant_shield(player, SH_WATERSHIELD);
    sound_play(SFX_WATERSHIELD);
}

void acidshieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_grant_shield(player, SH_ACIDSHIELD);
    sound_play(SFX_ACIDSHIELD);
}

void windshieldbox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
    player_grant_shield(player, SH_WINDSHIELD);
    sound_play(SFX_WINDSHIELD);
}

void trapbox_strategy(item_t *item, player_t *player)
{
    player_hit_ex(player, item->actor);
}

void emptybox_strategy(item_t *item, player_t *player)
{
    level_add_to_score(100);
}


/* private methods */
item_t* itembox_create(void (*on_destroy)(item_t*,player_t*), int anim_id)
{
    item_t *item = mallocx(sizeof(itembox_t));
    itembox_t *me = (itembox_t*)item;

    item->init = itembox_init;
    item->release = itembox_release;
    item->update = itembox_update;
    item->render = itembox_render;

    me->on_destroy = on_destroy;
    me->anim_id = anim_id;

    return item;
}

void itembox_init(item_t *item)
{
    itembox_t *me = (itembox_t*)item;

    item->always_active = FALSE;
    item->obstacle = TRUE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation("SD_ITEMBOX", me->anim_id));
}

void itembox_release(item_t* item)
{
    actor_destroy(item->actor);
}

void itembox_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    actor_t *act = item->actor;
    itembox_t *me = (itembox_t*)item;

    item->obstacle = !player_is_attacking( level_player() );

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];

        /* the player is about to crash this box... */
        if(item->state == IS_IDLE && player_collision(player, item->actor) && player_is_attacking(player)) {
            item_t *icon = level_create_legacy_item(IT_ICON, v2d_add(act->position, v2d_new(0,-5)));
            icon_change_animation(icon, me->anim_id);
            level_create_legacy_item(IT_EXPLOSION, v2d_add(act->position, v2d_new(0,-20)));
            level_create_legacy_item(IT_CRUSHEDBOX, act->position);

            sound_play(SFX_DESTROY);
            player_bounce_ex(player, act, TRUE);

            me->on_destroy(item, player);
            item->state = IS_DEAD;
        }
    }

    /* animation */
    me->anim_id = me->anim_id < 3 ? get_anim_id( level_player()->name ) : me->anim_id;
    actor_change_animation(item->actor, sprite_get_animation("SD_ITEMBOX", me->anim_id));
}

void itembox_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


int get_anim_id(const char *player_name)
{
    if(str_icmp(player_name, "Surge") == 0)
        return 0;
    else if(str_icmp(player_name, "Neon") == 0)
        return 1;
    else if(str_icmp(player_name, "Charge") == 0)
        return 2;
    else
        return 0;
}


/* loop class */
typedef struct loop_t loop_t;
struct loop_t {
    item_t item; /* base class */
    animation_t *animation;
    bricklayer_t layer_to_be_activated;

    int *player_was_touching_me;
    int team_size;
};

static item_t* loop_create(const char *sprite_name, bricklayer_t layer_to_be_activated);
static void loop_init(item_t *item);
static void loop_release(item_t* item);
static void loop_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void loop_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* loopgreen_create()
{
    return loop_create("SD_LOOPGREEN", BRL_GREEN);
}

item_t* loopyellow_create()
{
    return loop_create("SD_LOOPYELLOW", BRL_YELLOW);
}


/* private methods */
item_t* loop_create(const char *sprite_name, bricklayer_t layer_to_be_activated)
{
    loop_t *me = mallocx(sizeof *me);
    item_t *item = (item_t*)me;

    item->init = loop_init;
    item->release = loop_release;
    item->update = loop_update;
    item->render = loop_render;

    me->animation = sprite_get_animation(sprite_name, 0);
    me->layer_to_be_activated = layer_to_be_activated;
    me->player_was_touching_me = NULL;
    me->team_size = 0;

    return item;
}


void loop_init(item_t *item)
{
    loop_t *me = (loop_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, me->animation);
}



void loop_release(item_t* item)
{
    loop_t *me = (loop_t*)item;

    actor_destroy(item->actor);

    if(me->player_was_touching_me != NULL)
        free(me->player_was_touching_me);
}



void loop_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    int i;
    loop_t *me = (loop_t*)item;
    actor_t *act = item->actor;

    act->visible = level_editmode();

    if(team_size != me->team_size) {
        me->team_size = team_size;
        me->player_was_touching_me = reallocx(me->player_was_touching_me, team_size * sizeof(int));
        for(i=0; i<team_size; i++)
            me->player_was_touching_me[i] = player_collision(team[i], act);
    }

    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(player_collision(player, act)) {
            if(!me->player_was_touching_me[i]) {
                player_set_layer(player, me->layer_to_be_activated);
                me->player_was_touching_me[i] = TRUE;
            }
        }
        else
            me->player_was_touching_me[i] = FALSE;
    }
}


void loop_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}


/* Note: the following old loop system has been removed from the engine */

/* loop class */
typedef struct oldloop_t oldloop_t;
struct oldloop_t {
    item_t item; /* base class */
    char *sprite_name; /* sprite name */
    void (*on_collision)(player_t*); /* strategy pattern */
};

static item_t* oldloop_create(void (*strategy)(player_t*), const char *sprite_name);
static void oldloop_init(item_t *item);
static void oldloop_release(item_t* item);
static void oldloop_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void oldloop_render(item_t* item, v2d_t camera_position);

static void loopright_strategy(player_t *player);
static void looptop_strategy(player_t *player);
static void loopleft_strategy(player_t *player);
static void loopnone_strategy(player_t *player);
static void loopfloor_strategy(player_t *player);
static void loopfloornone_strategy(player_t *player);
static void loopfloortop_strategy(player_t *player);

/* public methods */
item_t* loopright_create()
{
    return oldloop_create(loopright_strategy, "SD_LOOPRIGHT");
}

item_t* looptop_create()
{
    return oldloop_create(looptop_strategy, "SD_LOOPMIDDLE");
}

item_t* loopleft_create()
{
    return oldloop_create(loopleft_strategy, "SD_LOOPLEFT");
}

item_t* loopnone_create()
{
    return oldloop_create(loopnone_strategy, "SD_LOOPNONE");
}

item_t* loopfloor_create()
{
    return oldloop_create(loopfloor_strategy, "SD_LOOPFLOOR");
}

item_t* loopfloornone_create()
{
    return oldloop_create(loopfloornone_strategy, "SD_LOOPFLOORNONE");
}

item_t* loopfloortop_create()
{
    return oldloop_create(loopfloortop_strategy, "SD_LOOPFLOORTOP");
}

/* private strategies */

/* Loop Right: right loop entrance */
void loopright_strategy(player_t *player)
{
    ;
}

/* Loop Top (x-axis): toggles left/right walls */
void looptop_strategy(player_t *player)
{
    ;
}

/* Loop Left: left loop entrance */
void loopleft_strategy(player_t *player)
{
    ;
}

/* Loop None: deactivate loops (x-axis), restoring the walls */
void loopnone_strategy(player_t *player)
{
    ;
}

/* Loop Floor (y-axis): bottom loop entrance */
void loopfloor_strategy(player_t *player)
{
    ;
}

/* Loop Floor None: deactivate loops (y-axis), restoring the floor */
void loopfloornone_strategy(player_t *player)
{
    ;
}

/* Loop Floor Top: activate left and right walls (x-axis) */
void loopfloortop_strategy(player_t *player)
{
    ;
}

/* private methods */
item_t* oldloop_create(void (*strategy)(player_t*), const char *sprite_name)
{
    item_t *item = mallocx(sizeof(oldloop_t));
    oldloop_t *me = (oldloop_t*)item;

    item->init = oldloop_init;
    item->release = oldloop_release;
    item->update = oldloop_update;
    item->render = oldloop_render;

    me->on_collision = strategy;
    me->sprite_name = str_dup(sprite_name);

    return item;
}

void oldloop_init(item_t *item)
{
    oldloop_t *me = (oldloop_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = FALSE;
    item->preserve = TRUE;
    item->actor = actor_create();

    actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));
}

void oldloop_release(item_t* item)
{
    oldloop_t *me = (oldloop_t*)item;
    free(me->sprite_name);
    actor_destroy(item->actor);
}

void oldloop_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    actor_t *act = item->actor;
    act->visible = level_editmode();
}

void oldloop_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* spikes class */
typedef struct spikes_t spikes_t;
struct spikes_t {
    item_t item; /* base class */
    int (*collision)(item_t*,player_t*); /* strategy pattern */
    int anim_id; /* animation number */
    float timer; /* periodic spikes */
    float cycle_length; /* periodic spikes */
    int hidden; /* is this object hidden? (it shows and hides every cycle) */
};

static item_t* spikes_create(int (*collision)(item_t*,player_t*), int anim_id, float cycle_length);
static void spikes_init(item_t *item);
static void spikes_release(item_t* item);
static void spikes_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void spikes_render(item_t* item, v2d_t camera_position);

static int spikes_hittest(player_t *player, float rect[4]);

static int floor_strategy(item_t *spikes, player_t *player);
static int ceiling_strategy(item_t *spikes, player_t *player);
static int leftwall_strategy(item_t *spikes, player_t *player);
static int rightwall_strategy(item_t *spikes, player_t *player);



/* public methods */
item_t* floorspikes_create()
{
    return spikes_create(floor_strategy, 0, INFINITY);
}

item_t* ceilingspikes_create()
{
    return spikes_create(ceiling_strategy, 2, INFINITY);
}

item_t* leftwallspikes_create()
{
    return spikes_create(leftwall_strategy, 1, INFINITY);
}

item_t* rightwallspikes_create()
{
    return spikes_create(rightwall_strategy, 3, INFINITY);
}

item_t* periodic_floorspikes_create()
{
    return spikes_create(floor_strategy, 0, 5.0f);
}

item_t* periodic_ceilingspikes_create()
{
    return spikes_create(ceiling_strategy, 2, 5.0f);
}

item_t* periodic_leftwallspikes_create()
{
    return spikes_create(leftwall_strategy, 1, 5.0f);
}

item_t* periodic_rightwallspikes_create()
{
    return spikes_create(rightwall_strategy, 3, 5.0f);
}

/* private methods */
item_t* spikes_create(int (*collision)(item_t*,player_t*), int anim_id, float cycle_length)
{
    item_t *item = mallocx(sizeof(spikes_t));
    spikes_t *me = (spikes_t*)item;

    item->init = spikes_init;
    item->release = spikes_release;
    item->update = spikes_update;
    item->render = spikes_render;

    me->collision = collision;
    me->anim_id = anim_id;
    me->cycle_length = cycle_length;

    return item;
}

void spikes_init(item_t *item)
{
    spikes_t *me = (spikes_t*)item;

    item->always_active = FALSE;
    item->obstacle = TRUE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->timer = 0.0f;
    me->hidden = FALSE;

    actor_change_animation(item->actor, sprite_get_animation("SD_SPIKES", me->anim_id));
}



void spikes_release(item_t* item)
{
    actor_destroy(item->actor);
}



void spikes_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    spikes_t *me = (spikes_t*)item;
    float dt = timer_get_delta();

    /* change state */
    me->timer += dt;
    if(me->timer >= me->cycle_length * 0.5f) {
        me->timer = 0.0f;
        me->hidden = !me->hidden;
        if(me->hidden)
            sound_play(SFX_SPIKESOUT);
        else
            sound_play(SFX_SPIKESIN);
    }
    item->obstacle = !me->hidden;
    item->actor->visible = !me->hidden;

    /* spike collision */
    if(!me->hidden) {
        int i;

        for(i=0; i<team_size; i++) {
            player_t *player = team[i];
            if(!player_is_dying(player) && !player_is_getting_hit(player) && !player_is_blinking(player) && !player_is_invincible(player)) {
                if(me->collision(item, player)) {
                    sound_t *s = SFX_SPIKES;
                    if(!sound_is_playing(s))
                        sound_play(s);
                    player_hit_ex(player, item->actor);
                }
            }
        }
    }
}


void spikes_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* private strategies */
int floor_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    float feet;
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x + 5;
    b[1] = act->position.y - act->hot_spot.y - 5;
    b[2] = b[0] + image_width(actor_image(act)) - 10;
    b[3] = b[1] + 10;

    feet = player->actor->position.y - player->actor->hot_spot.y + image_height(actor_image(player->actor));
    return spikes_hittest(player, b) && feet < (act->position.y - act->hot_spot.y + image_height(actor_image(act))/2);
}

int ceiling_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x + 5;
    b[1] = act->position.y - act->hot_spot.y + image_height(actor_image(act)) - 5;
    b[2] = b[0] + image_width(actor_image(act)) - 10;
    b[3] = b[1] + 10;

    return spikes_hittest(player, b);
}

int leftwall_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x + image_width(actor_image(act)) - 5;
    b[1] = act->position.y - act->hot_spot.y + 5;
    b[2] = b[0] + 10;
    b[3] = b[1] + image_height(actor_image(act)) - 10;

    return spikes_hittest(player, b);
}

int rightwall_strategy(item_t *spikes, player_t *player)
{
    float b[4];
    actor_t *act = spikes->actor;

    b[0] = act->position.x - act->hot_spot.x - 5;
    b[1] = act->position.y - act->hot_spot.y + 5;
    b[2] = b[0] + 10;
    b[3] = b[1] + image_height(actor_image(act)) - 10;

    return spikes_hittest(player, b);
}


/* misc */

/* returns true if the player collides with the given rectangle */
int spikes_hittest(player_t *player, float rect[4])
{
    float a[4];
    actor_t *pl = player->actor;

    a[0] = pl->position.x - pl->hot_spot.x;
    a[1] = pl->position.y - pl->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(pl));
    a[3] = a[1] + image_height(actor_image(pl));

    return bounding_box(a, rect);
}



/* spring class */
typedef struct spring_t spring_t;
struct spring_t {
    item_t item; /* base class */
    v2d_t strength;
    v2d_t box_size;
    v2d_t box_offset;
    char *sprite_name;
    float bang_timer;
    int is_bumping;
    void (*on_bump)(item_t*,player_t*); /* strategy pattern */
};

static item_t* spring_create(void (*strategy)(item_t*,player_t*), const char *sprite_name, v2d_t strength);
static void spring_init(item_t *item);
static void spring_release(item_t* item);
static void spring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void spring_render(item_t* item, v2d_t camera_position);

static void classicspring_strategy(item_t *item, player_t *player); /* activates if you jump on it */
static void volatilespring_strategy(item_t *item, player_t *player); /* activates when touched */

static void springfy_player(player_t *player, v2d_t strength);
static void activate_spring(spring_t *spring, player_t *player);
static int spring_collision(item_t *item, player_t *player);

/* debug */
/*#define SPRING SHOW_COLLIDER*/

/* public methods */
item_t* yellowspring_create() /* regular spring */
{
    return spring_create(classicspring_strategy, "SD_YELLOWSPRING", v2d_new(0,-600));
}

item_t* tryellowspring_create() /* top-right */
{
    return spring_create(volatilespring_strategy, "SD_TRYELLOWSPRING", v2d_new(424,-424));
}

item_t* ryellowspring_create()  /* right-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_RYELLOWSPRING", v2d_new(600,0));
}

item_t* bryellowspring_create() /* bottom-right */
{
    return spring_create(volatilespring_strategy, "SD_BRYELLOWSPRING", v2d_new(424,424));
}

item_t* byellowspring_create() /* bottom-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_BYELLOWSPRING", v2d_new(0,600));
}

item_t* blyellowspring_create() /* bottom-left */
{
    return spring_create(volatilespring_strategy, "SD_BLYELLOWSPRING", v2d_new(-424,424));
}

item_t* lyellowspring_create() /* left-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_LYELLOWSPRING", v2d_new(-600,0));
}

item_t* tlyellowspring_create() /* top-left */
{
    return spring_create(volatilespring_strategy, "SD_TLYELLOWSPRING", v2d_new(-424,-424));
}

item_t* redspring_create() /* regular spring */
{
    return spring_create(classicspring_strategy, "SD_REDSPRING", v2d_new(0,-960));
}

item_t* trredspring_create() /* top-right */
{
    return spring_create(volatilespring_strategy, "SD_TRREDSPRING", v2d_new(679,-679));
}

item_t* rredspring_create()  /* right-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_RREDSPRING", v2d_new(960,0));
}

item_t* brredspring_create() /* bottom-right */
{
    return spring_create(volatilespring_strategy, "SD_BRREDSPRING", v2d_new(679,679));
}

item_t* bredspring_create() /* bottom-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_BREDSPRING", v2d_new(0,960));
}

item_t* blredspring_create() /* bottom-left */
{
    return spring_create(volatilespring_strategy, "SD_BLREDSPRING", v2d_new(-679,679));
}

item_t* lredspring_create() /* left-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_LREDSPRING", v2d_new(-960,0));
}

item_t* tlredspring_create() /* top-left */
{
    return spring_create(volatilespring_strategy, "SD_TLREDSPRING", v2d_new(-679,-679));
}

item_t* bluespring_create() /* regular spring */
{
    return spring_create(classicspring_strategy, "SD_BLUESPRING", v2d_new(0,-1500));
}

item_t* trbluespring_create() /* top-right */
{
    return spring_create(volatilespring_strategy, "SD_TRBLUESPRING", v2d_new(1061,-1061));
}

item_t* rbluespring_create()  /* right-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_RBLUESPRING", v2d_new(1500,0));
}

item_t* brbluespring_create() /* bottom-right */
{
    return spring_create(volatilespring_strategy, "SD_BRBLUESPRING", v2d_new(1061,1061));
}

item_t* bbluespring_create() /* bottom-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_BBLUESPRING", v2d_new(0,1500));
}

item_t* blbluespring_create() /* bottom-left */
{
    return spring_create(volatilespring_strategy, "SD_BLBLUESPRING", v2d_new(-1061,1061));
}

item_t* lbluespring_create() /* left-oriented spring */
{
    return spring_create(volatilespring_strategy, "SD_LBLUESPRING", v2d_new(-1500,0));
}

item_t* tlbluespring_create() /* top-left */
{
    return spring_create(volatilespring_strategy, "SD_TLBLUESPRING", v2d_new(-1061,-1061));
}


/* private strategies */

/* springs using the volatile strategy are activated when you touch them */
void volatilespring_strategy(item_t *item, player_t *player)
{
    activate_spring((spring_t*)item, player);
}

/* springs using the classic strategy are activated when you jump on them */
void classicspring_strategy(item_t *item, player_t *player)
{
    if(player->actor->speed.y >= 1.0f || !nearly_zero(player->actor->angle))
        activate_spring((spring_t*)item, player);
}

/* private methods */
item_t* spring_create(void (*strategy)(item_t*,player_t*), const char *sprite_name, v2d_t strength)
{
    item_t *item = mallocx(sizeof(spring_t));
    spring_t *me = (spring_t*)item;

    item->init = spring_init;
    item->release = spring_release;
    item->update = spring_update;
    item->render = spring_render;

    me->on_bump = strategy;
    me->sprite_name = str_dup(sprite_name);
    me->strength = strength;

    return item;
}


/* private methods */
void spring_init(item_t *item)
{
    spring_t *me = (spring_t*)item;
    image_t* img;
    v2d_t v;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_bumping = FALSE;
    me->bang_timer = 0.0f;
    actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));

    /* initialize the coordinates of the collider */
    img = actor_image(item->actor);
    v = v2d_new(sign(me->strength.x) * (me->strength.x != 0), sign(me->strength.y) * (me->strength.y != 0));
    if(fabs(v.x) + fabs(v.y) <= 1.0f) {
        me->box_offset = v2d_new(image_width(img) * 0.25f * v.x, image_height(img) * 0.25f * v.y);
        if(fabs(v.x) < fabs(v.y))
            me->box_size = v2d_new(image_width(img), image_height(img)/2);
        else
            me->box_size = v2d_new(image_width(img)/2, image_height(img));
    }
    else {
        me->box_size = v2d_new(image_width(img) * 0.67f, image_height(img) * 0.67f);
        me->box_offset = v2d_new(image_width(img) * 0.34f * v.x, image_height(img) * 0.34f * v.y);
    }
}



void spring_release(item_t* item)
{
    spring_t *me = (spring_t*)item;

    actor_destroy(item->actor);
    free(me->sprite_name);
}



void spring_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    spring_t *me = (spring_t*)item;
    float dt = timer_get_delta();
    int i;

    /* bump! */
    me->bang_timer += dt;
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];
        if(!player_is_dying(player) && spring_collision(item, player))
            me->on_bump(item, player);
    }

    /* restore default animation */
    if(me->is_bumping) {
        if(actor_animation_finished(item->actor)) {
            actor_change_animation(item->actor, sprite_get_animation(me->sprite_name, 0));
            me->is_bumping = FALSE;
        }
    }
}


void spring_render(item_t* item, v2d_t camera_position)
{
#ifdef SPRING_SHOW_COLLIDER
    spring_t *me = (spring_t*)item;
    int x1 = (item->actor->position.x) + (me->box_offset.x - me->box_size.x/2.0f) - (camera_position.x - (VIDEO_SCREEN_W / 2));
    int y1 = (item->actor->position.y) + (me->box_offset.y - me->box_size.y/2.0f) - (camera_position.y - (VIDEO_SCREEN_H / 2));
    int x2 = (item->actor->position.x) + (me->box_offset.x + me->box_size.x/2.0f) - (camera_position.x - (VIDEO_SCREEN_W / 2));
    int y2 = (item->actor->position.y) + (me->box_offset.y + me->box_size.y/2.0f) - (camera_position.y - (VIDEO_SCREEN_H / 2));
    image_rect(x1, y1, x2, y2, color_rgb(255,255,0));
#endif
    actor_render(item->actor, camera_position);
}


/* 'springfy' player */
void springfy_player(player_t *player, v2d_t strength)
{
    actor_t *act = player->actor;

    if(!nearly_zero(strength.y) && !nearly_zero(strength.x))
        act->speed = strength;
    else if(!nearly_zero(strength.y))
        act->speed.y = strength.y;
    else if(!nearly_zero(strength.x)) {
        act->speed.x = strength.x;
        player_lock_horizontally_for(player, 0.27f);
    }
}

/* activate the spring */
void activate_spring(spring_t *spring, player_t *player)
{
    const float SPRING_BANG_TIMER = 0.2f; /* sfx control */
    item_t *item = (item_t*)spring;

    spring->is_bumping = TRUE;
    springfy_player(player, spring->strength);
    actor_change_animation(item->actor, sprite_get_animation(spring->sprite_name, 1));

    if(!nearly_zero(spring->strength.y)) {
        player_detach_from_ground(player);
        player_spring(player);
    }

    if(!nearly_zero(spring->strength.x)) {
        if(spring->strength.x > 0.0f)
            player->actor->mirror &= ~IF_HFLIP;
        else
            player->actor->mirror |= IF_HFLIP;
    }
    else
        player_spring(player);

    if(spring->bang_timer > SPRING_BANG_TIMER) {
        sound_play(SFX_SPRING);
        spring->bang_timer = 0.0f;
    }
}

/* returns true if the player is colliding with the spring */
int spring_collision(item_t *item, player_t *player)
{
    spring_t *me = (spring_t*)item;
    float a[4], b[4];

    a[0] = item->actor->position.x + me->box_offset.x - me->box_size.x / 2.0f;
    a[1] = item->actor->position.y + me->box_offset.y - me->box_size.y / 2.0f;
    a[2] = item->actor->position.x + me->box_offset.x + me->box_size.x / 2.0f;
    a[3] = item->actor->position.y + me->box_offset.y + me->box_size.y / 2.0f;

    b[0] = player->actor->position.x - player->actor->hot_spot.x + image_width(actor_image(player->actor)) * 0.3;
    b[1] = player->actor->position.y - player->actor->hot_spot.y + image_height(actor_image(player->actor)) * 0.5;
    b[2] = b[0] + image_width(actor_image(player->actor)) * 0.4;
    b[3] = b[1] + image_height(actor_image(player->actor)) * 0.5;

    return (!player_is_dying(player) && bounding_box(a,b));
}


/* supercollectible class */
typedef struct supercollectible_t supercollectible_t;
struct supercollectible_t {
    item_t item; /* base class */
    int is_disappearing; /* is the disappearing animation being played? */
};

static void supercollectible_init(item_t *item);
static void supercollectible_release(item_t* item);
static void supercollectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void supercollectible_render(item_t* item, v2d_t camera_position);



/* public methods */
item_t* supercollectible_create()
{
    item_t *item = mallocx(sizeof(supercollectible_t));

    item->init = supercollectible_init;
    item->release = supercollectible_release;
    item->update = supercollectible_update;
    item->render = supercollectible_render;

    return item;
}


/* private methods */
void supercollectible_init(item_t *item)
{
    supercollectible_t *me = (supercollectible_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_disappearing = FALSE;
    actor_change_animation(item->actor, sprite_get_animation("SD_SUPERCOLLECTIBLE", 0));
    actor_synchronize_animation(item->actor, TRUE);
}



void supercollectible_release(item_t* item)
{
    actor_destroy(item->actor);
}



void supercollectible_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    player_t *player = level_player();
    supercollectible_t *me = (supercollectible_t*)item;
    actor_t *act = item->actor;

    act->visible = (player->got_glasses || level_editmode());

    if(!me->is_disappearing) {
        if(!player_is_dying(player) && player->got_glasses && player_collision(player, act)) {
            /* the player is capturing this ring */
            actor_change_animation(act, sprite_get_animation("SD_SUPERCOLLECTIBLE", 1));
            player_set_collectibles( player_get_collectibles() + 5 );
            sound_play(SFX_COLLECTIBLE);
            me->is_disappearing = TRUE;
        }
    }
    else {
        if(actor_animation_finished(act)) {
            /* ouch, I've been caught! It's time to disappear... */
            item->state = IS_DEAD;
        }
    }
}


void supercollectible_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}



/* switch class */
typedef struct switch_t switch_t;
struct switch_t {
    item_t item; /* base class */
    int is_pressed; /* is this switch being pressed? */
    item_t *partner; /* the object I am coupled with (may be NULL, a door or a teleporter) */
};

static void switch_init(item_t *item);
static void switch_release(item_t* item);
static void switch_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void switch_render(item_t* item, v2d_t camera_position);

static int pressed_the_switch(item_t *item, player_t *player);
static void handle_logic(item_t *item, item_t *other, player_t **team, int team_size, void (*stepin)(item_t*,player_t*), void (*stepout)(item_t*));

static void stepin_nothing(item_t *door, player_t *who);
static void stepout_nothing(item_t *door);
static void stepin_door(item_t *door, player_t *who);
static void stepout_door(item_t *door);
static void stepin_teleporter(item_t *teleporter, player_t *who);
static void stepout_teleporter(item_t *teleporter);



/* public methods */
item_t* switch_create()
{
    item_t *item = mallocx(sizeof(switch_t));

    item->init = switch_init;
    item->release = switch_release;
    item->update = switch_update;
    item->render = switch_render;

    return item;
}


/* private methods */
void switch_init(item_t *item)
{
    switch_t *me = (switch_t*)item;

    item->always_active = FALSE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_pressed = FALSE;
    me->partner = NULL;
    actor_change_animation(item->actor, sprite_get_animation("SD_SWITCH", 0));
}



void switch_release(item_t* item)
{
    actor_destroy(item->actor);
}



void switch_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    switch_t *me = (switch_t*)item;
    item_t *door, *teleporter;
    float d1, d2;

    /* I have no partner */
    me->partner = NULL;

    /* figuring out who is my partner */
    door = find_closest_item(item, item_list, IT_DOOR, &d1);
    teleporter = find_closest_item(item, item_list, IT_TELEPORTER, &d2);
    if(door != NULL && d1 < d2)
        me->partner = door;
    if(teleporter != NULL && d2 < d1)
        me->partner = teleporter;

    /* handle the logic. Which logic? That depends. Who is my partner, if any? */
    if(me->partner == NULL)
        handle_logic(item, door, team, team_size, stepin_nothing, stepout_nothing);
    else if(me->partner == door)
        handle_logic(item, door, team, team_size, stepin_door, stepout_door);
    else if(me->partner == teleporter)
        handle_logic(item, teleporter, team, team_size, stepin_teleporter, stepout_teleporter);
}


void switch_render(item_t* item, v2d_t camera_position)
{
    switch_t *me = (switch_t*)item;

    if(level_editmode() && me->partner != NULL) {
        v2d_t p1, p2, offset;
        offset = v2d_subtract(camera_position, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
        p1 = v2d_subtract(item->actor->position, offset);
        p2 = v2d_subtract(me->partner->actor->position, offset);
        image_line((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, color_rgb(255, 0, 0));
    }

    actor_render(item->actor, camera_position);
}



/* misc */
void handle_logic(item_t *item, item_t *other, player_t **team, int team_size, void (*stepin)(item_t*,player_t*), void (*stepout)(item_t*))
{
    int i;
    int nobody_is_pressing_me = TRUE;
    switch_t *me = (switch_t*)item;
    actor_t *act = item->actor;

    /* step in */
    for(i=0; i<team_size; i++) {
        player_t *player = team[i];

        if(pressed_the_switch(item, player)) {
            nobody_is_pressing_me = FALSE;
            if(!me->is_pressed) {
                stepin(other, player);
                sound_play(SFX_SWITCH);
                actor_change_animation(act, sprite_get_animation("SD_SWITCH", 1));
                me->is_pressed = TRUE;
            }
        }
    }

    /* step out */
    if(nobody_is_pressing_me) {
        if(me->is_pressed) {
            stepout(other);
            actor_change_animation(act, sprite_get_animation("SD_SWITCH", 0));
            me->is_pressed = FALSE;
        }
    }
}

void stepin_nothing(item_t *door, player_t *who)
{
    ; /* empty */
}

void stepout_nothing(item_t *door)
{
    ; /* empty */
}

void stepin_door(item_t *door, player_t *who)
{
    door_open(door);
}

void stepout_door(item_t *door)
{
    door_close(door);
}

void stepin_teleporter(item_t *teleporter, player_t *who)
{
    teleporter_activate(teleporter, who);
}

void stepout_teleporter(item_t *teleporter)
{
    ; /* empty */
}

/* returns true if the player has pressed the switch (item) */
int pressed_the_switch(item_t *item, player_t *player)
{
    float a[4], b[4];

    a[0] = item->actor->position.x - item->actor->hot_spot.x;
    a[1] = item->actor->position.y - item->actor->hot_spot.y;
    a[2] = a[0] + image_width(actor_image(item->actor));
    a[3] = a[1] + image_height(actor_image(item->actor));

    b[0] = player->actor->position.x - player->actor->hot_spot.x + image_width(actor_image(player->actor)) * 0.3;
    b[1] = player->actor->position.y - player->actor->hot_spot.y + image_height(actor_image(player->actor)) * 0.5;
    b[2] = b[0] + image_width(actor_image(player->actor)) * 0.4;
    b[3] = b[1] + image_height(actor_image(player->actor)) * 0.5;

    return (!player_is_dying(player) && bounding_box(a,b));
}



/* teleporter class */
typedef struct teleporter_t teleporter_t;
struct teleporter_t {
    item_t item; /* base class */
    int is_disabled; /* is this teleporter disabled? */
    int is_active; /* is this object teleporting the team? */
    float timer; /* time counter */
    player_t *who; /* who has activated me? */
};

static void teleporter_init(item_t *item);
static void teleporter_release(item_t* item);
static void teleporter_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list);
static void teleporter_render(item_t* item, v2d_t camera_position);

static void teleport_player_to(player_t *player, v2d_t position);



/* public methods */
item_t* teleporter_create()
{
    item_t *item = mallocx(sizeof(teleporter_t));

    item->init = teleporter_init;
    item->release = teleporter_release;
    item->update = teleporter_update;
    item->render = teleporter_render;

    return item;
}

void teleporter_activate(item_t *teleporter, player_t *who)
{
    teleporter_t *me = (teleporter_t*)teleporter;
    actor_t *act = teleporter->actor;

    if(!me->is_active && !me->is_disabled) {
        me->is_active = TRUE;
        me->who = who;

        input_ignore(who->actor->input);
        level_set_camera_focus(act);
        sound_play(SFX_TELEPORTER);
    }
}

/* private methods */
void teleporter_init(item_t *item)
{
    teleporter_t *me = (teleporter_t*)item;

    item->always_active = TRUE;
    item->obstacle = FALSE;
    item->bring_to_back = TRUE;
    item->preserve = TRUE;
    item->actor = actor_create();

    me->is_disabled = FALSE;
    me->is_active = FALSE;
    me->timer = 0.0f;

    actor_change_animation(item->actor, sprite_get_animation("SD_TELEPORTER", 0));
}



void teleporter_release(item_t* item)
{
    actor_destroy(item->actor);
}



void teleporter_update(item_t* item, player_t** team, int team_size, brick_list_t* brick_list, item_list_t* item_list, enemy_list_t* enemy_list)
{
    teleporter_t *me = (teleporter_t*)item;
    actor_t *act = item->actor;
    float dt = timer_get_delta();
    int i, k=0;
    
    if(me->is_active) {
        me->timer += dt;
        if(me->timer >= 3.0f) {
            /* okay, teleport them all! */
            player_t *who = me->who; /* who has activated the teleporter? */

            input_restore(who->actor->input);
            level_set_camera_focus(who->actor);

            for(i=0; i<team_size; i++) {
                player_t *player = team[i];
                if(player != who) {
                    v2d_t position = v2d_add(act->position, v2d_new(-20 + 40*(k++), -30));
                    teleport_player_to(player, position);
                }
            }

            me->is_active = FALSE;
            me->is_disabled = TRUE; /* the teleporter works only once */
        }
        else {
            ; /* the players are being teletransported... wait a little bit. */
        }

        actor_change_animation(act, sprite_get_animation("SD_TELEPORTER", 1));
    }
    else
        actor_change_animation(act, sprite_get_animation("SD_TELEPORTER", 0));
}


void teleporter_render(item_t* item, v2d_t camera_position)
{
    actor_render(item->actor, camera_position);
}

/* teleports the player to the given position */
void teleport_player_to(player_t *player, v2d_t position)
{
    player->actor->position = position;
    player->actor->speed = v2d_new(0,0);
    player->actor->angle = 0;
}

