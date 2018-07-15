/*
 * Open Surge Engine
 * player.c - player module
 * Copyright (C) 2008-2012, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
 *
 * Edits by Dalton Sterritt (all edits released under same license, copyright given to Alexandre):
 * player_enable_roll, player_disable_roll
 */

#include <math.h>
#include "actor.h"
#include "player.h"
#include "brick.h"
#include "enemy.h"
#include "item.h"
#include "character.h"
#include "physics/collisionmask.h"
#include "items/collectible.h"
#include "items/bouncingcollectible.h"
#include "../core/global.h"
#include "../core/audio.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/darray.h"
#include "../core/timer.h"
#include "../core/logfile.h"
#include "../core/input.h"
#include "../core/sprite.h"
#include "../core/soundfactory.h"
#include "../scenes/level.h"
#include "physics/physicsactor.h"
#include "physics/obstaclemap.h"
#include "physics/obstacle.h"


/* Uncomment to render the sensors */
/*#define SHOW_SENSORS*/

/* Smoothing the angle (the greater the value, the faster it rotates) */
#define ANGLE_SMOOTHING 4

/* macros */
#define ON_STATE(s) \
    if(p->pa_old_state != (s) && physicsactor_get_state(p->pa) == (s))

#define CHANGE_ANIM(id) do { \
    animation_t *an = sprite_get_animation(charactersystem_get(p->name)->animation.sprite_name, charactersystem_get(p->name)->animation.id); \
    actor_change_animation(p->actor, an); \
} while(0)

/* private data */
#define PLAYER_MAX_BLINK            2.0  /* how many seconds does the player must blink if he/she gets hurt? */
#define PLAYER_UNDERWATER_BREATH    30.0 /* how many seconds does the player can stay underwater before drowning? */
static int collectibles, hundred_collectibles;         /* shared collectibles */
static int lives;                        /* shared lives */
static int score;                        /* shared score */


static void update_shield(player_t *p);
static void update_animation(player_t *p);
static void physics_adapter(player_t *player, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static obstacle_t* item2obstacle(const item_t *item);
static obstacle_t* object2obstacle(const object_t *object);
static int ignore_obstacle(bricklayer_t brick_layer, bricklayer_t player_layer);
static float ang_diff(float alpha, float beta);


/*
 * player_create()
 * Creates a player
 */
player_t *player_create(const char *character_name)
{
    int i;
    player_t *p = mallocx(sizeof *p);
    const character_t *c = charactersystem_get(character_name);

    logfile_message("player_create(\"%s\")", character_name);

    /* initializing... */
    p->name = str_dup(character_name);
    p->actor = actor_create();
    p->disable_movement = FALSE;
    p->disable_roll = FALSE;
    p->in_locked_area = FALSE;
    p->at_some_border = FALSE;
    p->bring_to_back = FALSE;
    p->disable_collectible_loss = FALSE;
    p->disable_animation_control = FALSE;
    p->attacking = FALSE;
    p->actor->input = input_create_user(NULL);
    CHANGE_ANIM(stopped);

    /* auxiliary variables */
    p->on_moveable_platform = FALSE;
    p->got_glasses = FALSE;

    /* blink */
    p->blinking = FALSE;
    p->blink_timer = 0.0f;
    p->blink_visibility_timer = 0.0f;

    /* shield */
    p->shield = actor_create();
    p->shield_type = SH_NONE;

    /* invincibility */
    p->invincible = FALSE;
    p->invtimer = 0;
    for(i=0; i<PLAYER_MAX_INVSTAR; i++) {
        p->invstar[i] = actor_create();
        actor_change_animation(p->invstar[i], sprite_get_animation("SD_INVSTAR", 0));
    }

    /* speed shoes */
    p->got_speedshoes = FALSE;
    p->speedshoes_timer = 0;

    /* old loop system */
    p->disable_wall = PLAYER_WALL_NONE;
    p->entering_loop = FALSE;
    p->at_loopfloortop = FALSE;

    /* loop system */
    p->layer = BRL_DEFAULT;

    /* physics */
    p->pa = physicsactor_create(p->actor->position);
    p->pa_old_state = physicsactor_get_state(p->pa);

    /* misc */
    p->underwater = FALSE;
    p->underwater_timer = 0.0f;

    /* character system */
    if(str_icmp(c->companion_object_name, "") != 0) {
        /* try to create the companion object using the new API if possible */
        if(!level_create_ssobject(c->companion_object_name, v2d_new(0, 0))) {
            /* not possible; use the old API */
            enemy_t* e = level_create_enemy(c->companion_object_name, v2d_new(0, 0));
            e->created_from_editor = FALSE;
        }
    }

    physicsactor_set_acc(p->pa, physicsactor_get_acc(p->pa) * c->multiplier.acc);
    physicsactor_set_dec(p->pa, physicsactor_get_dec(p->pa) * c->multiplier.dec);
    physicsactor_set_topspeed(p->pa, physicsactor_get_topspeed(p->pa) * c->multiplier.topspeed);
    physicsactor_set_jmp(p->pa, physicsactor_get_jmp(p->pa) * c->multiplier.jmp);
    physicsactor_set_jmprel(p->pa, physicsactor_get_jmprel(p->pa) * c->multiplier.jmprel);
    physicsactor_set_grv(p->pa, physicsactor_get_grv(p->pa) * c->multiplier.grv);
    physicsactor_set_rollthreshold(p->pa, physicsactor_get_rollthreshold(p->pa) * c->multiplier.rollthreshold);
    physicsactor_set_brakingthreshold(p->pa, physicsactor_get_brakingthreshold(p->pa) * c->multiplier.brakingthreshold);
    physicsactor_set_slp(p->pa, physicsactor_get_slp(p->pa) * c->multiplier.slp);
    physicsactor_set_rolluphillslp(p->pa, physicsactor_get_rolluphillslp(p->pa) * c->multiplier.rolluphillslp);
    physicsactor_set_rolldownhillslp(p->pa, physicsactor_get_rolldownhillslp(p->pa) * c->multiplier.rolldownhillslp);

    /* success! */
    hundred_collectibles = collectibles = 0;
    logfile_message("player_create() ok");
    return p;
}


/*
 * player_destroy()
 * Destroys a player
 */
player_t* player_destroy(player_t *player)
{
    int i;

    for(i=0; i<PLAYER_MAX_INVSTAR; i++)
        actor_destroy(player->invstar[i]);

    physicsactor_destroy(player->pa);
    actor_destroy(player->shield);
    actor_destroy(player->actor);
    free(player->name);
    free(player);

    return NULL;
}



/*
 * player_update()
 * Updates the player
 */
void player_update(player_t *player, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, enemy_list_t *enemy_list)
{
    int i;
    actor_t *act = player->actor;
    float dt = timer_get_delta();

    /* "gambiarra" */
    act->hot_spot = v2d_new(image_width(actor_image(act))/2, image_height(actor_image(act))-20);

    /* physics */
    if(!player->disable_movement) {
        player->pa_old_state = physicsactor_get_state(player->pa);
        physics_adapter(player, team, team_size, brick_list, item_list, enemy_list);
    }

    /* the player blinks */
    if(player->blinking) {
        player->blink_timer += timer_get_delta();

        if(player->blink_timer - player->blink_visibility_timer >= 0.067f) {
            player->blink_visibility_timer = player->blink_timer;
            act->visible = !act->visible;
        }

        if(player->blink_timer >= PLAYER_MAX_BLINK) {
            player->blinking = FALSE;
            act->visible = TRUE;
        }
    }

    if(physicsactor_get_state(player->pa) != PAS_GETTINGHIT && player->pa_old_state == PAS_GETTINGHIT) {
        player->blinking = TRUE;
        player->blink_timer = 0.0f;
        player->blink_visibility_timer = 0.0f;
    }

    /* shield */
    if(player->shield_type != SH_NONE)
        update_shield(player);

    /* get underwater */
    if(!(player->underwater) && player->actor->position.y >= level_waterlevel())
        player_enter_water(player);
    else if(player->underwater && player->actor->position.y < level_waterlevel())
        player_leave_water(player);

    /* underwater? */
    if(player->underwater) {
        player->speedshoes_timer = max(player->speedshoes_timer, PLAYER_MAX_SPEEDSHOES); /* disable speed shoes */

        if(player->shield_type != SH_WATERSHIELD)
            player->underwater_timer += dt;
        else
            player->underwater_timer = 0.0f;

        if(player_seconds_remaining_to_drown(player) <= 0.0f)
            player_drown(player);
    }

    /* invincibility stars */
    if(player->invincible) {
        int maxf = sprite_get_animation("SD_INVSTAR", 0)->frame_count;
        int invangle[PLAYER_MAX_INVSTAR];
        v2d_t starpos;

        player->invtimer += dt;

        for(i=0; i<PLAYER_MAX_INVSTAR; i++) {
            invangle[i] = (180*4) * timer_get_ticks()*0.001 + (i+1)*(360/PLAYER_MAX_INVSTAR);
            starpos.x = 25*cos(invangle[i]*PI/180);
            starpos.y = ((timer_get_ticks()+i*400)%2000)/40;
            /*starpos = v2d_rotate(starpos, act->angle);*/
            player->invstar[i]->position.x = act->position.x - act->hot_spot.x + image_width(actor_image(act))/2 + starpos.x;
            player->invstar[i]->position.y = act->position.y - act->hot_spot.y + image_height(actor_image(act)) - starpos.y + 5;
            actor_change_animation_frame(player->invstar[i], random(maxf));
        }

        if(player->invtimer >= PLAYER_MAX_INVINCIBILITY)
            player->invincible = FALSE;
    }

    /* speed shoes */
    if(player->got_speedshoes) {
        physicsactor_t *pa = player->pa;

        if(player->speedshoes_timer == 0) {
            physicsactor_set_acc(pa, physicsactor_get_acc(pa) * 2.0f);
            physicsactor_set_frc(pa, physicsactor_get_frc(pa) * 2.0f);
            physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * 2.0f);
            physicsactor_set_air(pa, physicsactor_get_air(pa) * 2.0f);
            physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * 2.0f);
            player->speedshoes_timer += dt;
        }
        else if(player->speedshoes_timer >= PLAYER_MAX_SPEEDSHOES) {
            physicsactor_set_acc(pa, physicsactor_get_acc(pa) / 2.0f);
            physicsactor_set_frc(pa, physicsactor_get_frc(pa) / 2.0f);
            physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) / 2.0f);
            physicsactor_set_air(pa, physicsactor_get_air(pa) / 2.0f);
            physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) / 2.0f);
            player->got_speedshoes = FALSE;
        }
        else
            player->speedshoes_timer += dt;
    }

    /* animation */
    update_animation(player);

    /* is it a CPU controlled player? */
    if(player != level_player()) {
        for(i=0; i<IB_MAX; i++)
            input_simulate_button_up(act->input, (inputbutton_t)i);
    }

    /* winning pose */
    if(level_has_been_cleared())
        physicsactor_enable_winning_pose(player->pa);
}


/*
 * player_render()
 * Rendering function
 */
void player_render(player_t *player, v2d_t camera_position)
{
    float ang;
    actor_t *act = player->actor;
    int i, behind_player[PLAYER_MAX_INVSTAR];

    /* invincibility stars I */
    for(i=0; i<PLAYER_MAX_INVSTAR && player->invincible; i++) {
        behind_player[i] = (int)((180*4) * timer_get_ticks()*0.001 + (i+1)*(360/PLAYER_MAX_INVSTAR)) % 360 >= 180;
        if(behind_player[i])
            actor_render(player->invstar[i], camera_position);
    }

    /* render the player */
    switch(physicsactor_get_movmode(player->pa)) {
        case MM_FLOOR: act->position.y -= 1; break;
        case MM_LEFTWALL: act->position.x += 3; break;
        case MM_RIGHTWALL: act->position.x -= 1; break;
        case MM_CEILING: act->position.y += 3; break;
    }

    act->angle = old_school_angle(ang = act->angle);
    actor_render(act, camera_position);
    act->angle = ang;

    switch(physicsactor_get_movmode(player->pa)) {
        case MM_FLOOR: act->position.y += 1; break;
        case MM_LEFTWALL: act->position.x -= 3; break;
        case MM_RIGHTWALL: act->position.x += 1; break;
        case MM_CEILING: act->position.y -= 3; break;
    }

    /* render the shield */
    if(player->shield_type != SH_NONE)
        actor_render(player->shield, camera_position);

    /* invincibility stars II */
    for(i=0; i<PLAYER_MAX_INVSTAR && player->invincible; i++) {
        if(!behind_player[i])
            actor_render(player->invstar[i], camera_position);
    }

#ifdef SHOW_SENSORS
    /* sensors */
    physicsactor_render_sensors(player->pa, camera_position);
#endif
}



/*
 * player_bounce()
 * Rebound
 */
void player_bounce(player_t *player, actor_t *hazard)
{
    int w = image_width(actor_image(hazard))/2, h = image_height(actor_image(hazard))/2;
    v2d_t hazard_centre = v2d_add(v2d_subtract(hazard->position, hazard->hot_spot), v2d_new(w/2, h/2));
    actor_t *act = player->actor;

    if(physicsactor_is_in_the_air(player->pa)) {
        if(act->position.y <= hazard_centre.y && act->speed.y > 0.0f) {
            if(input_button_down(act->input, IB_FIRE1) || physicsactor_get_state(player->pa) == PAS_ROLLING)
                act->speed.y *= -1.0f;
            else
                act->speed.y = 4 * max(-60.0f, -60.0f * act->speed.y);

            player->pa_old_state = physicsactor_get_state(player->pa);
            physicsactor_bounce(player->pa);
        }
        else
            act->speed.y -= 4 * 60.0f * sign(act->speed.y);
    }
}



/*
 * player_hit()
 * Hits a player. If it has no collectibles, then
 * it must die
 */
void player_hit(player_t *player, actor_t *hazard)
{
    int w = image_width(actor_image(hazard))/2, h = image_height(actor_image(hazard))/2;
    v2d_t hazard_centre = v2d_add(v2d_subtract(hazard->position, hazard->hot_spot), v2d_new(w/2, h/2));

    if(player->invincible || physicsactor_get_state(player->pa) == PAS_GETTINGHIT || player->blinking || player_is_dying(player))
        return;

    if(player_get_collectibles() > 0 || player->shield_type != SH_NONE) {
        player->actor->speed.x = 120.0f * sign(player->actor->position.x - hazard_centre.x);
        player->actor->speed.y = -240.0f;
        player->actor->position.y -= 2; /* bugfix */

        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_hit(player->pa);

        if(player->shield_type != SH_NONE) {
            player->shield_type = SH_NONE;
            sound_play( soundfactory_get("damaged") );
        }
        else if(!player->disable_collectible_loss) {
            float a = 101.25f, spd = 240.0f*2;
            int i, r = min(32, player_get_collectibles());
            item_t *b;
            player_set_collectibles(0);

            /* create collectibles */
            for(i=0; i<r; i++) {
                b = level_create_item(IT_BOUNCINGRING, player->actor->position);
                bouncingcollectible_set_speed(b, v2d_new(-sin(a*PI/180.0f)*spd*(1-2*(i%2)), cos(a*PI/180.0f)*spd));
                a += 22.5f * (i%2);

                if(i%16 == 0) {
                    spd /= 2.0f;
                    a = 101.25f;
                }
            }

            sound_play( soundfactory_get("collectible loss") );
        }
        else
            sound_play( soundfactory_get("damaged") );
    }
    else
        player_kill(player);
}



/*
 * player_kill()
 * Kills a player
 */
void player_kill(player_t *player)
{
    if(!player_is_dying(player)) {
        player->invincible = FALSE;
        player->got_speedshoes = FALSE;
        player->shield_type = SH_NONE;
        player->blinking = FALSE;
        player->attacking = FALSE;

        player->actor->position.y -= 2;
        player->actor->speed = v2d_new(0, -420);

        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_kill(player->pa);
        sound_play( charactersystem_get(player->name)->sample.death );
    }
}


/*
 * player_roll()
 * Rolls
 */
void player_roll(player_t *player)
{
    player->pa_old_state = physicsactor_get_state(player->pa);
    physicsactor_roll(player->pa);
}

/*
 * player_enable_roll()
 * disables player rolling
 */
void player_enable_roll(player_t *player)
{
    physicsactor_t *pa = player->pa;
    if(player->disable_roll) {
        physicsactor_set_rollthreshold(pa, physicsactor_get_rollthreshold(pa) - 1000.0f);
        player->disable_roll = FALSE;
    }
}

/*
 * player_disable_roll()
 * disables player rolling
 */
void player_disable_roll(player_t *player)
{
    physicsactor_t *pa = player->pa;
    if(!(player->disable_roll)) {
        physicsactor_set_rollthreshold(pa, physicsactor_get_rollthreshold(pa) + 1000.0f);
        player->disable_roll = TRUE;
    }
}

/*
 * player_spring()
 * Springfy player
 */
void player_spring(player_t *player)
{
    player->pa_old_state = physicsactor_get_state(player->pa);
    physicsactor_spring(player->pa);
}

/*
 * player_drown()
 * Drown (underwater). This will be
 * called automatically, internally.
 */
void player_drown(player_t *player)
{
    if(player->underwater && !player_is_dying(player)) {
        player->actor->position.y -= 2;
        player->actor->speed = v2d_new(0, 0);
        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_drown(player->pa);
        sound_play( soundfactory_get("drown") );
    }
}


/*
 * player_breathe()
 * Breathe (air bubble, underwater)
 */
void player_breathe(player_t *player)
{
    if(player->underwater && physicsactor_get_state(player->pa) != PAS_BREATHING && physicsactor_get_state(player->pa) != PAS_DROWNED && physicsactor_get_state(player->pa) != PAS_DEAD) {
        player_reset_underwater_timer(player);
        player->actor->speed = v2d_new(0, 0);
        player->pa_old_state = physicsactor_get_state(player->pa);
        physicsactor_breathe(player->pa);
        sound_play( soundfactory_get("breathing") );
    }
}


/*
 * player_enter_water()
 * Enters the water
 */
void player_enter_water(player_t *player)
{
    physicsactor_t *pa = player->pa;

    if(!player->underwater) {
        player->actor->speed.x *= 0.5f;
        player->actor->speed.y *= 0.25f;

        physicsactor_set_acc(pa, physicsactor_get_acc(pa) / 2.0f);
        physicsactor_set_dec(pa, physicsactor_get_dec(pa) / 2.0f);
        physicsactor_set_frc(pa, physicsactor_get_frc(pa) / 2.0f);
        physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) / 2.0f);
        physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) / 2.0f);
        physicsactor_set_air(pa, physicsactor_get_air(pa) / 2.0f);
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) / 3.5f);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) / 1.85f);
        physicsactor_set_jmprel(pa, physicsactor_get_jmprel(pa) / 2.0f);

        sound_play( soundfactory_get("enter water") );
        player->underwater_timer = 0.0f;
        player->underwater = TRUE;
    }
}


/*
 * player_leave_water()
 * Leaves the water
 */
void player_leave_water(player_t *player)
{
    physicsactor_t *pa = player->pa;

    if(player->underwater) {
        if(!player_is_springing(player) && !player_is_dying(player))
            player->actor->speed.y *= 2.0f;

        physicsactor_set_acc(pa, physicsactor_get_acc(pa) * 2.0f);
        physicsactor_set_dec(pa, physicsactor_get_dec(pa) * 2.0f);
        physicsactor_set_frc(pa, physicsactor_get_frc(pa) * 2.0f);
        physicsactor_set_rollfrc(pa, physicsactor_get_rollfrc(pa) * 2.0f);
        physicsactor_set_topspeed(pa, physicsactor_get_topspeed(pa) * 2.0f);
        physicsactor_set_air(pa, physicsactor_get_air(pa) * 2.0f);
        physicsactor_set_grv(pa, physicsactor_get_grv(pa) * 3.5f);
        physicsactor_set_jmp(pa, physicsactor_get_jmp(pa) * 1.85f);
        physicsactor_set_jmprel(pa, physicsactor_get_jmprel(pa) * 2.0f);

        sound_play( soundfactory_get("leave water") );
        player->underwater = FALSE;
    }
}


/*
 * player_is_underwater()
 * Is the player underwater?
 */
int player_is_underwater(const player_t *player)
{
    return player->underwater;
}



/*
 * player_reset_underwater_timer()
 * Reset underwater timer
 */
void player_reset_underwater_timer(player_t *player)
{
    player->underwater_timer = 0.0f;
}


/*
 * player_seconds_remaining_to_drown()
 * How many seconds to drown?
 */
float player_seconds_remaining_to_drown(const player_t *player)
{
    return player->underwater && player->shield_type != SH_WATERSHIELD ? max(0.0f, PLAYER_UNDERWATER_BREATH - player->underwater_timer) : INFINITY_FLT;
}


/*
 * player_lock_horizontally_for()
 * Horizontal control lock timer
 */
void player_lock_horizontally_for(player_t *player, float seconds)
{
    physicsactor_lock_horizontally_for(player->pa, seconds);
}


/*
 * player_is_attacking()
 * Returns TRUE if a given player is attacking;
 * FALSE otherwise
 */
int player_is_attacking(const player_t *player)
{
    return player->attacking || player->invincible || physicsactor_get_state(player->pa) == PAS_JUMPING || physicsactor_get_state(player->pa) == PAS_ROLLING;
}


/*
 * player_is_rolling()
 * TRUE iff the player is rolling
 */
int player_is_rolling(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_ROLLING;
}


/*
 * player_is_getting_hit()
 * TRUE iff the player is getting hit
 */
int player_is_getting_hit(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_GETTINGHIT;
}

/*
 * player_is_dying()
 * TRUE iff the player is dying
 */
int player_is_dying(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_DEAD || physicsactor_get_state(player->pa) == PAS_DROWNED;
}


/*
 * player_is_stopped()
 * TRUE iff the player is stopped
 */
int player_is_stopped(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_STOPPED;
}

/*
 * player_is_walking()
 * TRUE iff the player is walking
 */
int player_is_walking(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_WALKING;
}

/*
 * player_is_running()
 * TRUE iff the player is running
 */
int player_is_running(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_RUNNING;
}

/*
 * player_is_jumping()
 * TRUE iff the player is jumping
 */
int player_is_jumping(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_JUMPING;
}

/*
 * player_is_springing()
 * TRUE iff the player is springing
 */
int player_is_springing(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_SPRINGING;
}

/*
 * player_is_pushing()
 * TRUE iff the player is pushing
 */
int player_is_pushing(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_PUSHING;
}

/*
 * player_is_braking()
 * TRUE iff the player is braking
 */
int player_is_braking(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_BRAKING;
}

/*
 * player_is_at_ledge()
 * TRUE iff the player is at ledge
 */
int player_is_at_ledge(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_LEDGE;
}

/*
 * player_is_drowning()
 * TRUE iff the player is drowning
 */
int player_is_drowning(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_DROWNED;
}

/*
 * player_is_breathing()
 * TRUE iff the player is breathing an air bubble
 */
int player_is_breathing(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_BREATHING;
}

/*
 * player_is_ducking()
 * TRUE iff the player is ducking
 */
int player_is_ducking(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_DUCKING;
}

/*
 * player_is_lookingup()
 * TRUE iff the player is looking up
 */
int player_is_lookingup(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_LOOKINGUP;
}

/*
 * player_is_waiting()
 * TRUE iff the player is waiting
 */
int player_is_waiting(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_WAITING;
}

/*
 * player_is_winning()
 * TRUE iff the player is winning
 */
int player_is_winning(const player_t *player)
{
    return physicsactor_get_state(player->pa) == PAS_WINNING;
}




/*
 * player_is_in_the_air()
 * TRUE iff the player is in the air
 */
int player_is_in_the_air(const player_t *player)
{
    return physicsactor_is_in_the_air(player->pa);
}


/*
 * player_is_ultrafast()
 * TRUE if, and only if, the player is wearing speed shoes
 */
int player_is_ultrafast(const player_t* player)
{
    return player->got_speedshoes;
}

int player_is_invincible(const player_t* player)
{
    return player->invincible;
}














/*
 * player_get_collectibles()
 * Returns the amount of collectibles
 * the player has got so far
 */
int player_get_collectibles()
{
    return collectibles;
}



/*
 * player_set_collectibles()
 * Sets a new amount of collectibles
 */
void player_set_collectibles(int c)
{
    collectibles = clip(c, 0, 999);

    /* (100+) * k collectibles (k integer) = new life! */
    if(c/100 > hundred_collectibles) {
        hundred_collectibles = c/100;
        player_set_lives( player_get_lives()+1 );
        level_override_music( soundfactory_get("1up") );
    }
}



/*
 * player_get_lives()
 * How many lives does the player have?
 */
int player_get_lives()
{
    return lives;
}



/*
 * player_set_lives()
 * Sets the number of lives
 */
void player_set_lives(int l)
{
    lives = max(0, l);
}



/*
 * player_get_score()
 * Returns the score
 */
int player_get_score()
{
    return score;
}



/*
 * player_set_score()
 * Sets the score
 */
void player_set_score(int s)
{
    score = max(0, s);
}




/* private functions */

/* updates the position and the animation of the current shield */
void update_shield(player_t *p)
{
    actor_t *sh = p->shield, *act = p->actor;
    v2d_t off = v2d_new(0,0);
    sh->position = v2d_add(act->position, v2d_rotate(off, -old_school_angle(act->angle)));

    switch(p->shield_type) {

        case SH_SHIELD:
            actor_change_animation(sh, sprite_get_animation("SD_SHIELD", 0));
            break;

        case SH_FIRESHIELD:
            actor_change_animation(sh, sprite_get_animation("SD_FIRESHIELD", 0));
            break;

        case SH_THUNDERSHIELD:
            actor_change_animation(sh, sprite_get_animation("SD_THUNDERSHIELD", 0));
            break;

        case SH_WATERSHIELD:
            actor_change_animation(sh, sprite_get_animation("SD_WATERSHIELD", 0));
            break;

        case SH_ACIDSHIELD:
            actor_change_animation(sh, sprite_get_animation("SD_ACIDSHIELD", 0));
            break;

        case SH_WINDSHIELD:
            actor_change_animation(sh, sprite_get_animation("SD_WINDSHIELD", 0));
            break;
    }
}

/* updates the animation of the player */
void update_animation(player_t *p)
{
    /* animations */
    if(!p->disable_animation_control) {
        switch(physicsactor_get_state(p->pa)) {
            case PAS_STOPPED:    CHANGE_ANIM(stopped);    break;
            case PAS_WALKING:    CHANGE_ANIM(walking);    break;
            case PAS_RUNNING:    CHANGE_ANIM(running);    break;
            case PAS_JUMPING:    CHANGE_ANIM(jumping);    break;
            case PAS_SPRINGING:  CHANGE_ANIM(springing);  break;
            case PAS_ROLLING:    CHANGE_ANIM(rolling);    break;
            case PAS_PUSHING:    CHANGE_ANIM(pushing);    break;
            case PAS_GETTINGHIT: CHANGE_ANIM(gettinghit); break;
            case PAS_DEAD:       CHANGE_ANIM(dead);       break;
            case PAS_BRAKING:    CHANGE_ANIM(braking);    break;
            case PAS_LEDGE:      CHANGE_ANIM(ledge);      break;
            case PAS_DROWNED:    CHANGE_ANIM(drowned);    break;
            case PAS_BREATHING:  CHANGE_ANIM(breathing);  break;
            case PAS_WAITING:    CHANGE_ANIM(waiting);    break;
            case PAS_DUCKING:    CHANGE_ANIM(ducking);    break;
            case PAS_LOOKINGUP:  CHANGE_ANIM(lookingup);  break;
            case PAS_WINNING:    CHANGE_ANIM(winning);    break;
        }
    }
    else
        p->disable_animation_control = FALSE; /* for set_player_animation (scripting) */

    /* sounds */
    ON_STATE(PAS_JUMPING) {
        sound_play( charactersystem_get(p->name)->sample.jump );
    }

    ON_STATE(PAS_ROLLING) {
        sound_play( charactersystem_get(p->name)->sample.roll );
    }

    ON_STATE(PAS_BRAKING) {
        sound_play( charactersystem_get(p->name)->sample.brake );
    }

    /* "gambiarra" */
    p->actor->hot_spot = v2d_new(image_width(actor_image(p->actor))/2, image_height(actor_image(p->actor))-20);
}

/* the interface between player_t and physicsactor_t */
void physics_adapter(player_t *player, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    DARRAY(obstacle_t*, tmp_obstacle);
    actor_t *act = player->actor;
    physicsactor_t *pa = player->pa;
    obstaclemap_t *obstaclemap;
    float new_angle;
    int i;

    /* converting variables */
    physicsactor_set_position(pa, act->position);
    if(physicsactor_is_in_the_air(pa) || player_is_getting_hit(player) || player_is_dying(player)) {
        physicsactor_set_xsp(pa, act->speed.x);
        physicsactor_set_ysp(pa, act->speed.y);
    }
    else {
        physicsactor_set_gsp(pa, act->speed.x);
        physicsactor_set_ysp(pa, 0.0f);
    }

    /* capturing input */
    if(input_button_down(act->input, IB_RIGHT))
        physicsactor_walk_right(pa);
    if(input_button_down(act->input, IB_LEFT))
        physicsactor_walk_left(pa);
    if(input_button_down(act->input, IB_DOWN))
        physicsactor_duck(pa);
    if(input_button_down(act->input, IB_UP))
        physicsactor_look_up(pa);
    if(input_button_down(act->input, IB_FIRE1))
        physicsactor_jump(pa);

    /* create a container for temp obstacles */
    darray_init(tmp_obstacle);

    /* creating the obstacle map */
    obstaclemap = obstaclemap_create();
    for(; brick_list; brick_list = brick_list->next) {
        if(brick_obstacle(brick_list->data) != NULL && !ignore_obstacle(brick_layer(brick_list->data), player->layer))
            obstaclemap_add_obstacle(obstaclemap, brick_obstacle(brick_list->data));
    }
    for(; item_list; item_list = item_list->next) {
        if(item_list->data->obstacle && item_list->data->mask && !ignore_obstacle(BRL_DEFAULT, player->layer)) {
            obstacle_t* obstacle = item2obstacle(item_list->data);
            obstaclemap_add_obstacle(obstaclemap, obstacle);
            darray_push(tmp_obstacle, obstacle);
        }
    }
    for(; object_list; object_list = object_list->next) {
        if(object_list->data->obstacle && object_list->data->mask && !ignore_obstacle(BRL_DEFAULT, player->layer)) {
            obstacle_t* obstacle = object2obstacle(object_list->data);
            obstaclemap_add_obstacle(obstaclemap, obstacle);
            darray_push(tmp_obstacle, obstacle);
        }
    }

    /* updating the physics actor */
    physicsactor_update(pa, obstaclemap);

    /* destroying the obstacle map */
    obstaclemap = obstaclemap_destroy(obstaclemap);

    /* removing temp obstacles */
    for(i = 0; i < darray_length(tmp_obstacle); i++)
        obstacle_destroy(tmp_obstacle[i]);
    darray_release(tmp_obstacle);

    /* can't leave the screen */
    if(physicsactor_get_position(pa).x < 20) {
        physicsactor_set_position(pa, v2d_new(20, physicsactor_get_position(pa).y));
        physicsactor_set_xsp(pa, 0.0f);
        physicsactor_set_gsp(pa, 0.0f);
    }
    else if(physicsactor_get_position(pa).x > level_size().x - 20) {
        physicsactor_set_position(pa, v2d_new(level_size().x - 20, physicsactor_get_position(pa).y));
        physicsactor_set_xsp(pa, 0.0f);
        physicsactor_set_gsp(pa, 0.0f);
    }

    /* unconverting variables */
    act->position = physicsactor_get_position(pa);
    act->speed = physicsactor_is_in_the_air(pa) || player_is_getting_hit(player) || player_is_dying(player) ? v2d_new(physicsactor_get_xsp(pa), physicsactor_get_ysp(pa)) : v2d_new(physicsactor_get_gsp(pa), 0.0f);

    /* smoothing the angle */
    new_angle = ((256 - physicsactor_get_angle(pa)) * 1.40625f) / 57.2957795131f;
    if(ang_diff(new_angle, act->angle) < 1.6f)
        act->angle = lerp_angle(act->angle, new_angle, (ANGLE_SMOOTHING * PI * timer_get_delta()));
    else
        act->angle = new_angle;

    /* misc */
    act->mirror = !physicsactor_is_facing_right(pa) ? IF_HFLIP : IF_NONE;
}

/* converts a built-in item to an obstacle */
obstacle_t* item2obstacle(const item_t *item)
{
    const collisionmask_t* mask = item->mask;
    v2d_t position = v2d_subtract(item->actor->position, item->actor->hot_spot);
    return obstacle_create_solid(mask, position);
}

/* converts a custom object to an obstacle */
obstacle_t* object2obstacle(const object_t *object)
{
    const collisionmask_t* mask = object->mask;
    v2d_t position = v2d_subtract(object->actor->position, object->actor->hot_spot);
    return obstacle_create_solid(mask, position);
}

/* ignore the obstacle? */
int ignore_obstacle(bricklayer_t brick_layer, bricklayer_t player_layer)
{
    return (brick_layer != BRL_DEFAULT && player_layer != brick_layer);
}

/* given two angles in [0, 2pi], return their difference */
float ang_diff(float alpha, float beta)
{
    float twopi = PI * 2;
    float diff = fmod(fabs(alpha - beta), twopi);
    return min(twopi - diff, diff);
}