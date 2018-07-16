/*
 * Open Surge Engine
 * player.h - player module
 * Copyright (C) 2008-2011, 2014  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "../core/v2d.h"

/* constants */
#define PLAYER_INITIAL_LIVES        5    /* initial lives */
#define PLAYER_MAX_INVINCIBILITY    23.0 /* invincibility timer */
#define PLAYER_MAX_SPEEDSHOES       23.0 /* speed shoes timer */
#define PLAYER_MAX_INVSTAR          5    /* how many invincibility stars */

/* old loop system */
#define PLAYER_WALL_NONE            0
#define PLAYER_WALL_TOP             1
#define PLAYER_WALL_RIGHT           2
#define PLAYER_WALL_BOTTOM          4
#define PLAYER_WALL_LEFT            8

/* shield list */
#define SH_NONE                     0 /* no shield */
#define SH_SHIELD                   1 /* regular shield */
#define SH_FIRESHIELD               2 /* fire shield */
#define SH_THUNDERSHIELD            3 /* thunder shield */
#define SH_WATERSHIELD              4 /* water shield */
#define SH_ACIDSHIELD               5 /* acid shield */
#define SH_WINDSHIELD               6 /* wind shield */

/* forward declarations */
struct actor_t;
struct brick_list_t;
struct item_list_t;
struct enemy_list_t;
struct physicsactor_t;
typedef struct player_t player_t;

/* player structure */
struct player_t {
    /* general */
    char *name;
    struct actor_t *actor;
    int disable_movement;
    int disable_roll;
    int in_locked_area;
    int at_some_border;
    int bring_to_back;
    int on_movable_platform;
    int disable_collectible_loss;
    int disable_animation_control;
    int attacking; /* this is set by the user, and if it's true, player_is_attacking() will return true */

    /* magic glasses */
    int got_glasses; /* got glasses? */

    /* shields */
    int shield_type;
    struct actor_t *shield;

    /* invincibility */
    int invincible;
    float invtimer;
    struct actor_t* invstar[PLAYER_MAX_INVSTAR];

    /* speed shoes */
    int got_speedshoes;
    float speedshoes_timer;

    /* old loop system (PLAYER_WALL_*) */
    int disable_wall;
    int entering_loop;
    int at_loopfloortop;

    /* loop system */
    int layer;

    /* private */
    struct physicsactor_t *pa;
    int pa_old_state;
    int underwater;
    float underwater_timer;
    int blinking;
    float blink_timer;
    float blink_visibility_timer;
};

/* public functions */
player_t* player_create(const char *character_name);
player_t* player_destroy(player_t *player);
void player_update(player_t *player, struct player_t **team, int team_size, struct brick_list_t *brick_list, struct item_list_t *item_list, struct enemy_list_t *enemy_list);
void player_render(player_t *player, v2d_t camera_position);

void player_hit(player_t *player, struct actor_t *hazard);
void player_bounce(player_t *player, struct actor_t *hazard);
void player_kill(player_t *player);
void player_spring(player_t *player);
void player_roll(player_t *player);
void player_enable_roll(player_t *player);
void player_disable_roll(player_t *player);
void player_lock_horizontally_for(player_t *player, float seconds);

void player_enter_water(player_t *player);
void player_leave_water(player_t *player);
void player_breathe(player_t *player);
void player_reset_underwater_timer(player_t *player);
void player_drown(player_t *player);
int player_is_underwater(const player_t *player);
float player_seconds_remaining_to_drown(const player_t *player);

int player_is_in_the_air(const player_t *player);
int player_is_attacking(const player_t *player);
int player_is_ultrafast(const player_t *player); /* wearing speed shoes? */
int player_is_invincible(const player_t *player); /* invunerible? */

int player_is_stopped(const player_t *player);
int player_is_walking(const player_t *player);
int player_is_running(const player_t *player);
int player_is_jumping(const player_t *player);
int player_is_springing(const player_t *player);
int player_is_rolling(const player_t *player);
int player_is_pushing(const player_t *player);
int player_is_getting_hit(const player_t *player);
int player_is_dying(const player_t *player);
int player_is_braking(const player_t *player);
int player_is_at_ledge(const player_t *player);
int player_is_drowning(const player_t *player);
int player_is_breathing(const player_t *player);
int player_is_ducking(const player_t *player);
int player_is_lookingup(const player_t *player);
int player_is_waiting(const player_t *player);
int player_is_winning(const player_t *player);

int player_get_collectibles();
void player_set_collectibles(int c);
int player_get_lives();
void player_set_lives(int l);
int player_get_score();
void player_set_score(int s);
const char *player_get_sprite_name(const player_t *player);

#endif
