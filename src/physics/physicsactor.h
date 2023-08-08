/*
 * Open Surge Engine
 * physicsactor.h - physics system: actor
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _PHYSICSACTOR_H
#define _PHYSICSACTOR_H

#include <stdbool.h>
#include "../util/v2d.h"

/*
 * a physics actor is a sprite that respects the laws of
 * physics defined in this engine
 */
typedef struct physicsactor_t physicsactor_t;

/*
 * a physics actor may be in one of the following states:
 */
typedef enum physicsactorstate_t physicsactorstate_t;
enum physicsactorstate_t
{
    PAS_STOPPED,
    PAS_WALKING,
    PAS_RUNNING,
    PAS_JUMPING,
    PAS_SPRINGING,
    PAS_ROLLING,
    PAS_CHARGING,
    PAS_PUSHING,
    PAS_GETTINGHIT,
    PAS_DEAD,
    PAS_BRAKING,
    PAS_LEDGE,
    PAS_DROWNED,
    PAS_BREATHING,
    PAS_DUCKING,
    PAS_LOOKINGUP,
    PAS_WAITING,
    PAS_WINNING
};

/*
 * a physics actor may be in one of the following
 * modes of movement, according to its angle:
 */
typedef enum movmode_t movmode_t;
enum movmode_t
{
    MM_FLOOR,
    MM_RIGHTWALL,
    MM_CEILING,
    MM_LEFTWALL
};

/* forward declarations */
struct obstaclemap_t;
struct obstacle_t;
enum obstaclelayer_t;

/* API */
physicsactor_t* physicsactor_create(v2d_t position);
physicsactor_t* physicsactor_destroy(physicsactor_t *pa);
void physicsactor_reset_model_parameters(physicsactor_t* pa);

void physicsactor_update(physicsactor_t *pa, const struct obstaclemap_t *obstaclemap);
void physicsactor_render_sensors(const physicsactor_t *pa, v2d_t camera_position);

bool physicsactor_is_facing_right(const physicsactor_t *pa);
bool physicsactor_is_touching_ceiling(const physicsactor_t *pa);
bool physicsactor_is_midair(const physicsactor_t *pa);
bool physicsactor_is_smashed(const physicsactor_t *pa);
physicsactorstate_t physicsactor_get_state(const physicsactor_t *pa);
int physicsactor_get_angle(const physicsactor_t *pa); /* get the angle in degrees */
v2d_t physicsactor_get_position(const physicsactor_t *pa); /* the position of the physics actor is the center of its sprite */
void physicsactor_set_position(physicsactor_t *pa, v2d_t position);
void physicsactor_lock_horizontally_for(physicsactor_t *pa, float seconds); /* set the horizontal control lock timer */
float physicsactor_hlock_timer(const physicsactor_t *pa); /* get the horizontal control lock timer (in seconds) */
bool physicsactor_ressurrect(physicsactor_t *pa, v2d_t position);
void physicsactor_enable_winning_pose(physicsactor_t *pa);
void physicsactor_detach_from_ground(physicsactor_t *pa);
movmode_t physicsactor_get_movmode(const physicsactor_t *pa);
enum obstaclelayer_t physicsactor_get_layer(const physicsactor_t *pa);
void physicsactor_set_layer(physicsactor_t *pa, enum obstaclelayer_t layer);
int physicsactor_roll_delta(const physicsactor_t* pa); /* roll delta (sensors) */
float physicsactor_charge_intensity(const physicsactor_t* pa); /* in [0,1] */
void physicsactor_bounding_box(const physicsactor_t *pa, int *width, int *height, v2d_t *center); /* center may be NULL */
bool physicsactor_is_standing_on_platform(const physicsactor_t *pa, const struct obstacle_t *obstacle);

void physicsactor_walk_right(physicsactor_t *pa); /* call before physicsactor_update() */
void physicsactor_walk_left(physicsactor_t *pa);
void physicsactor_duck(physicsactor_t *pa);
void physicsactor_look_up(physicsactor_t *pa);
void physicsactor_jump(physicsactor_t *pa);

void physicsactor_kill(physicsactor_t *pa);
void physicsactor_hit(physicsactor_t *pa, float direction);
void physicsactor_bounce(physicsactor_t *pa);
void physicsactor_spring(physicsactor_t *pa);
void physicsactor_roll(physicsactor_t *pa);
void physicsactor_drown(physicsactor_t *pa);
void physicsactor_breathe(physicsactor_t *pa);

float physicsactor_get_xsp(const physicsactor_t *pa); /* x speed */
void physicsactor_set_xsp(physicsactor_t *pa, float value);
float physicsactor_get_ysp(const physicsactor_t *pa); /* y speed */
void physicsactor_set_ysp(physicsactor_t *pa, float value);
float physicsactor_get_gsp(const physicsactor_t *pa); /* ground speed */
void physicsactor_set_gsp(physicsactor_t *pa, float value);
float physicsactor_get_acc(const physicsactor_t *pa); /* acceleration */
void physicsactor_set_acc(physicsactor_t *pa, float value);
float physicsactor_get_dec(const physicsactor_t *pa); /* deceleration */
void physicsactor_set_dec(physicsactor_t *pa, float value);
float physicsactor_get_frc(const physicsactor_t *pa); /* friction */
void physicsactor_set_frc(physicsactor_t *pa, float value);
float physicsactor_get_topspeed(const physicsactor_t *pa); /* top speed */
void physicsactor_set_topspeed(physicsactor_t *pa, float value);
float physicsactor_get_capspeed(const physicsactor_t *pa); /* cap speed */
void physicsactor_set_capspeed(physicsactor_t *pa, float value);
float physicsactor_get_air(const physicsactor_t *pa); /* air acceleration */
void physicsactor_set_air(physicsactor_t *pa, float value);
float physicsactor_get_airdrag(const physicsactor_t *pa); /* air drag */
void physicsactor_set_airdrag(physicsactor_t *pa, float value);
float physicsactor_get_jmp(const physicsactor_t *pa); /* initial jump speed */
void physicsactor_set_jmp(physicsactor_t *pa, float value);
float physicsactor_get_jmprel(const physicsactor_t *pa); /* release jump speed */
void physicsactor_set_jmprel(physicsactor_t *pa, float value);
float physicsactor_get_diejmp(const physicsactor_t *pa); /* death jump speed */
void physicsactor_set_diejmp(physicsactor_t *pa, float value);
float physicsactor_get_hitjmp(const physicsactor_t *pa); /* get hit jump speed */
void physicsactor_set_hitjmp(physicsactor_t *pa, float value);
float physicsactor_get_grv(const physicsactor_t *pa); /* gravity */
void physicsactor_set_grv(physicsactor_t *pa, float value);
float physicsactor_get_slp(const physicsactor_t *pa); /* slope */
void physicsactor_set_slp(physicsactor_t *pa, float value);
float physicsactor_get_chrg(const physicsactor_t *pa); /* charge-and-release max speed */
void physicsactor_set_chrg(physicsactor_t *pa, float value);
float physicsactor_get_rollfrc(const physicsactor_t *pa); /* roll friction */
void physicsactor_set_rollfrc(physicsactor_t *pa, float value);
float physicsactor_get_rolldec(const physicsactor_t *pa); /* roll deceleration */
void physicsactor_set_rolldec(physicsactor_t *pa, float value);
float physicsactor_get_rolluphillslp(const physicsactor_t *pa); /* roll uphill slope */
void physicsactor_set_rolluphillslp(physicsactor_t *pa, float value);
float physicsactor_get_rolldownhillslp(const physicsactor_t *pa); /* roll downhill slope */
void physicsactor_set_rolldownhillslp(physicsactor_t *pa, float value);
float physicsactor_get_walkthreshold(const physicsactor_t *pa); /* walk threshold */
void physicsactor_set_walkthreshold(physicsactor_t *pa, float value);
float physicsactor_get_rollthreshold(const physicsactor_t *pa); /* roll threshold */
void physicsactor_set_rollthreshold(physicsactor_t *pa, float value);
float physicsactor_get_unrollthreshold(const physicsactor_t *pa); /* unroll threshold */
void physicsactor_set_unrollthreshold(physicsactor_t *pa, float value);
float physicsactor_get_falloffthreshold(const physicsactor_t *pa); /* fall off threshold */
void physicsactor_set_falloffthreshold(physicsactor_t *pa, float value);
float physicsactor_get_brakingthreshold(const physicsactor_t *pa); /* braking animation threshold */
void physicsactor_set_brakingthreshold(physicsactor_t *pa, float value);
float physicsactor_get_airdragthreshold(const physicsactor_t *pa); /* air drag threshold */
void physicsactor_set_airdragthreshold(physicsactor_t *pa, float value);
float physicsactor_get_airdragxthreshold(const physicsactor_t *pa); /* air drag x-threshold */
void physicsactor_set_airdragxthreshold(physicsactor_t *pa, float value);
float physicsactor_get_chrgthreshold(const physicsactor_t *pa); /* charge intensity threshold */
void physicsactor_set_chrgthreshold(physicsactor_t *pa, float value);
float physicsactor_get_waittime(const physicsactor_t *pa); /* wait time in seconds */
void physicsactor_set_waittime(physicsactor_t *pa, float value);

#endif
