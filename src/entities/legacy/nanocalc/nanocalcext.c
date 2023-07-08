/*
 * Open Surge Engine
 * nanocalcext.c - nanocalc extensions
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

#include <time.h>
#include <math.h>
#include "nanocalc.h"
#include "nanocalcext.h"
#include "../item.h"
#include "../enemy.h"
#include "../../actor.h"
#include "../../player.h"
#include "../../brick.h"
#include "../../camera.h"
#include "../../../core/global.h"
#include "../../../core/video.h"
#include "../../../core/audio.h"
#include "../../../core/timer.h"
#include "../../../core/input.h"
#include "../../../scenes/level.h"

/* private stuff ;) */
#define PLAYER (enemy_get_observed_player(target))
#define BRICK_AT(ox,oy) actor_brick_at(target->actor,target_bricks_nearby,v2d_new(ox,oy)) /* this ignores passable bricks */
#define OBSTACLE_EXISTS(ox,oy) obstacle_exists(target,target_bricks_nearby,target_items_nearby,target_objects_nearby,v2d_new(ox,oy))
static object_t *target; /* target object */
static brick_list_t *target_bricks_nearby;
static item_list_t *target_items_nearby;
static object_list_t *target_objects_nearby;
static const struct tm* timeinfo() { time_t raw = time(NULL); return localtime(&raw); }
static int obstacle_exists(object_t *o, brick_list_t *bs, item_list_t *is, object_list_t *os, v2d_t offset);

/* BIFs */
static float f_elapsed_time() { return 0.001f * timer_get_ticks(); } /* elapsed time, in seconds */
static float f_dt() { return timer_get_delta(); } /* time difference between 2 consecutive frames */
static float f_fps() { return (float)video_fps(); } /* frames per second */
static float f_collectibles() { return (float)player_get_collectibles(); } /* number of collectibles */
static float f_lives() { return (float)player_get_lives(); } /* number of lives */
static float f_initial_lives() { return (float)PLAYER_INITIAL_LIVES; } /* initial number of lives */
static float f_score() { return (float)player_get_score(); } /* returns the score */
static float f_gravity() { return level_gravity(); } /* returns the gravity */
static float f_act() { return (float)level_act(); } /* returns the current act number */
static float f_xpos() { return target->actor->position.x; } /* x-position of the target object */
static float f_ypos() { return target->actor->position.y; } /* x-position of the target object */
static float f_hotspot_x() { return target->actor->hot_spot.x; } /* x-position of the hotspot */
static float f_hotspot_y() { return target->actor->hot_spot.y; } /* y-position of the hotspot */
static float f_alpha() { return target->actor->alpha; } /* alpha of the target object */
static float f_angle() { return target->actor->angle * 57.2957795131f; } /* angle of the target object */
static float f_scale_x() { return target->actor->scale.x; } /* scale x */
static float f_scale_y() { return target->actor->scale.y; } /* scale y */
static float f_animation_frame() { return actor_animation_frame(target->actor); } /* animation frame */
static float f_animation_speed_factor() { return target->actor->animation_speed_factor; }
static float f_animation_repeats() { return animation_repeats(target->actor->animation) ? 1.0f : 0.0f; }
static float f_animation_fps() { return animation_fps(target->actor->animation); }
static float f_animation_frame_count() { return animation_frame_count(target->actor->animation); }
static float f_animation_id() { return animation_id(target->actor->animation); }
static float f_zindex() { return target->zindex; }
static float f_spawnpoint_x() { return target->actor->spawn_point.x; }
static float f_spawnpoint_y() { return target->actor->spawn_point.y; }
static float f_screen_width() { return (float)VIDEO_SCREEN_W; }
static float f_screen_height() { return (float)VIDEO_SCREEN_H; }
static float f_width() { return image_width(actor_image(target->actor)); }
static float f_height() { return image_height(actor_image(target->actor)); }
static float f_direction() { return target->actor->mirror & IF_HFLIP ? -1.0f : 1.0f; }
static float f_player_xpos() { return PLAYER->actor->position.x; }
static float f_player_ypos() { return PLAYER->actor->position.y; }
static float f_player_spawnpoint_x() { return PLAYER->actor->spawn_point.x; }
static float f_player_spawnpoint_y() { return PLAYER->actor->spawn_point.y; }
static float f_player_xspeed() { return PLAYER->actor->speed.x; }
static float f_player_yspeed() { return PLAYER->actor->speed.y; }
static float f_player_angle() { return PLAYER->actor->angle * 57.2957795131f; }
static float f_player_direction() { return PLAYER->actor->mirror & IF_HFLIP ? -1.0f : 1.0f; }
static float f_player_seconds_remaining_to_drown() { return player_seconds_remaining_to_drown(PLAYER); }
static float f_music_volume() { return music_get_volume(); }
static float f_music_is_playing() { return music_is_playing() ? 1.0f : 0.0f; }
static float f_date_sec() { return (float)(timeinfo()->tm_sec); }  /* seconds after the minute; range: 0-59 */
static float f_date_min() { return (float)(timeinfo()->tm_min); }  /* minutes after the hour; range: 0-59 */
static float f_date_hour() { return (float)(timeinfo()->tm_hour); } /* hours since midnight; range: 0-23 */
static float f_date_mday() { return (float)(timeinfo()->tm_mday); } /* days of the months; range: 1-31 */
static float f_date_mon() { return (float)(timeinfo()->tm_mon); }  /* months since Janurary; range: 0-11 */
static float f_date_year() { return (float)(timeinfo()->tm_year); } /* years since 1900 */
static float f_date_wday() { return (float)(timeinfo()->tm_wday); } /* days since Sunday; range: 0-6 */
static float f_date_yday() { return (float)(timeinfo()->tm_yday); } /* days since January 1st; range: 0-365 */
static float f_music_duration() { return music_duration(); }
static float f_number_of_joysticks() { return input_number_of_joysticks(); }
static float f_camera_x() { return camera_get_position().x; }
static float f_camera_y() { return camera_get_position().y; }
static float f_waterlevel() { return (float)level_waterlevel(); }

static float f_brick_exists(float offset_x, float offset_y) { return (NULL == BRICK_AT(offset_x, offset_y) ? 0.0f : 1.0f); } /* 1 = exists; 0 = otherwise */
static float f_brick_type(float offset_x, float offset_y) { const brick_t *b = BRICK_AT(offset_x, offset_y); return b ? (float)brick_type(b) : 0.0f; }
static float f_brick_angle(float offset_x, float offset_y) { return 0.0f; } /* obsolete */
static float f_brick_layer(float offset_x, float offset_y) { const brick_t *b = BRICK_AT(offset_x, offset_y); return b ? (float)brick_layer(b) : 0.0f; }

static float f_obstacle_exists(float offset_x, float offset_y) { return OBSTACLE_EXISTS(offset_x, offset_y); }



/*
 * nanocalcext_register_bifs()
 * Registers a lot of useful built-in functions in nanocalc
 */
void nanocalcext_register_bifs()
{
    nanocalc_register_bif_arity0("elapsed_time", f_elapsed_time);
    nanocalc_register_bif_arity0("dt", f_dt);
    nanocalc_register_bif_arity0("fps", f_fps);
    nanocalc_register_bif_arity0("collectibles", f_collectibles);
    nanocalc_register_bif_arity0("lives", f_lives);
    nanocalc_register_bif_arity0("initial_lives", f_initial_lives);
    nanocalc_register_bif_arity0("score", f_score);
    nanocalc_register_bif_arity0("gravity", f_gravity);
    nanocalc_register_bif_arity0("act", f_act);
    nanocalc_register_bif_arity0("xpos", f_xpos);
    nanocalc_register_bif_arity0("ypos", f_ypos);
    nanocalc_register_bif_arity0("hotspot_x", f_hotspot_x);
    nanocalc_register_bif_arity0("hotspot_y", f_hotspot_y);
    nanocalc_register_bif_arity0("alpha", f_alpha);
    nanocalc_register_bif_arity0("angle", f_angle);
    nanocalc_register_bif_arity0("scale_x", f_scale_x);
    nanocalc_register_bif_arity0("scale_y", f_scale_y);
    nanocalc_register_bif_arity0("direction", f_direction);
    nanocalc_register_bif_arity0("animation_frame", f_animation_frame);
    nanocalc_register_bif_arity0("animation_speed_factor", f_animation_speed_factor);
    nanocalc_register_bif_arity0("animation_repeats", f_animation_repeats);
    nanocalc_register_bif_arity0("animation_fps", f_animation_fps);
    nanocalc_register_bif_arity0("animation_frame_count", f_animation_frame_count);
    nanocalc_register_bif_arity0("animation_id", f_animation_id);
    nanocalc_register_bif_arity0("zindex", f_zindex);
    nanocalc_register_bif_arity0("spawnpoint_x", f_spawnpoint_x);
    nanocalc_register_bif_arity0("spawnpoint_y", f_spawnpoint_y);
    nanocalc_register_bif_arity0("player_xpos", f_player_xpos);
    nanocalc_register_bif_arity0("player_ypos", f_player_ypos);
    nanocalc_register_bif_arity0("player_spawnpoint_x", f_player_spawnpoint_x);
    nanocalc_register_bif_arity0("player_spawnpoint_y", f_player_spawnpoint_y);
    nanocalc_register_bif_arity0("player_xspeed", f_player_xspeed);
    nanocalc_register_bif_arity0("player_yspeed", f_player_yspeed);
    nanocalc_register_bif_arity0("player_angle", f_player_angle);
    nanocalc_register_bif_arity0("player_direction", f_player_direction);
    nanocalc_register_bif_arity0("player_seconds_remaining_to_drown", f_player_seconds_remaining_to_drown);
    nanocalc_register_bif_arity0("screen_width", f_screen_width);
    nanocalc_register_bif_arity0("screen_height", f_screen_height);
    nanocalc_register_bif_arity0("width", f_width);
    nanocalc_register_bif_arity0("height", f_height);
    nanocalc_register_bif_arity0("music_volume", f_music_volume);
    nanocalc_register_bif_arity0("music_is_playing", f_music_is_playing);
    nanocalc_register_bif_arity0("date_sec", f_date_sec);
    nanocalc_register_bif_arity0("date_min", f_date_min);
    nanocalc_register_bif_arity0("date_hour", f_date_hour);
    nanocalc_register_bif_arity0("date_mday", f_date_mday);
    nanocalc_register_bif_arity0("date_mon", f_date_mon);
    nanocalc_register_bif_arity0("date_year", f_date_year);
    nanocalc_register_bif_arity0("date_wday", f_date_wday);
    nanocalc_register_bif_arity0("date_yday", f_date_yday);
    nanocalc_register_bif_arity0("music_duration", f_music_duration);
    nanocalc_register_bif_arity0("number_of_joysticks", f_number_of_joysticks);
    nanocalc_register_bif_arity0("camera_x", f_camera_x);
    nanocalc_register_bif_arity0("camera_y", f_camera_y);
    nanocalc_register_bif_arity0("waterlevel", f_waterlevel);

    nanocalc_register_bif_arity2("brick_exists", f_brick_exists);
    nanocalc_register_bif_arity2("brick_type", f_brick_type);
    nanocalc_register_bif_arity2("brick_angle", f_brick_angle);
    nanocalc_register_bif_arity2("brick_layer", f_brick_layer);
    nanocalc_register_bif_arity2("obstacle_exists", f_obstacle_exists);

    target = NULL;
}


/*
 * nanocalcext_set_target_object()
 * Defines a target object, used in some built-in functions called by nanocalc
 */
void nanocalcext_set_target_object(object_t *o, brick_list_t *bricks_nearby, item_list_t *items_nearby, object_list_t *objects_nearby)
{
    target = o;
    target_bricks_nearby = bricks_nearby;
    target_items_nearby = items_nearby;
    target_objects_nearby = objects_nearby;
}


/* ------------------------- */


/* is there an obstacle at (ox, oy) from object o? */
int obstacle_exists(object_t *o, brick_list_t *bs, item_list_t *is, object_list_t *os, v2d_t offset)
{
    actor_t *me = o->actor, *other;
    v2d_t p = v2d_add(me->position, offset);
    const image_t *img;

    if(!actor_brick_at(me, bs, offset)) {
        for(; is; is = is->next) {
            if(is->data->obstacle) {
                other = is->data->actor;
                img = actor_image(other);
                if(p.x >= other->position.x - other->hot_spot.x && p.x < other->position.x - other->hot_spot.x + image_width(img)) {
                    if(p.y >= other->position.y - other->hot_spot.y && p.y < other->position.y - other->hot_spot.y + image_height(img)) {
                        return TRUE;
                    }
                }
            }
        }

        for(; os; os = os->next) {
            if(os->data->obstacle) {
                other = os->data->actor;
                img = actor_image(other);
                if(p.x >= other->position.x - other->hot_spot.x && p.x < other->position.x - other->hot_spot.x + image_width(img)) {
                    if(p.y >= other->position.y - other->hot_spot.y && p.y < other->position.y - other->hot_spot.y + image_height(img)) {
                        return TRUE;
                    }
                }
            }
        }

        return FALSE;
    }
    else
        return TRUE;
}
