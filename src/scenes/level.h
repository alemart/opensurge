/*
 * Open Surge Engine
 * level.h - code for the game levels
 * Copyright (C) 2008-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _LEVEL_H
#define _LEVEL_H

#include <surgescript.h>
#include "../core/v2d.h"
#include "../core/color.h"
#include "../entities/brick.h"

/* scene methods */
void level_init(void *path_to_lev_file); /* pass an string containing the path to the .lev file */
void level_update();
void level_render();
void level_release();




/* forward declarations */
struct image_t;
struct actor_t;
struct player_t;
struct brick_t;
struct brick_list_t;
struct item_t;
struct item_list_t;
struct enemy_t;
struct enemy_list_t;
struct sound_t;
struct music_t;

/* level data */
const char* level_file();
const char* level_name();
int level_act();
const char* level_version();
const char* level_author();
const char* level_license();

/* players */
void level_change_player(struct player_t *new_player); /* character switching */
void level_set_spawn_point(v2d_t newpos);
struct player_t* level_player(); /* active player */
struct player_t* level_get_player_by_name(const char* name);
struct player_t* level_get_player_by_id(int id);

/* level objects */
struct brick_t* level_create_brick(int id, v2d_t position, bricklayer_t layer, brickflip_t flip);
struct item_t* level_create_item(int id, v2d_t position);
struct enemy_t* level_create_enemy(const char *name, v2d_t position);
surgescript_object_t* level_create_ssobject(const char* object_name, v2d_t position);
void level_create_particle(struct image_t *image, v2d_t position, v2d_t speed, int destroy_on_brick);
struct item_t* level_create_animal(v2d_t position);

/* camera */
void level_set_camera_focus(struct actor_t *act);
struct actor_t* level_get_camera_focus();
int level_is_camera_locked();
void level_lock_camera(int x1, int y1, int x2, int y2);
void level_unlock_camera();
int level_inside_screen(int x, int y, int w, int h);

/* editor */
int level_editmode();

/* music */
void level_override_music(struct sound_t *sample);
struct music_t* level_music();

/* management */
void level_change(const char* path_to_lev_file); /* change the level */
int level_persist(); /* persists (saves) the current level */
void level_clear(struct actor_t *end_sign);
int level_has_been_cleared();
void level_jump_to_next_stage();
void level_ask_to_leave();
void level_pause();
void level_restart();
void level_abort();
void level_push_quest(const char* path_to_qst_file);

/* water */
int level_waterlevel();
void level_set_waterlevel(int ycoord);
color_t level_watercolor();
void level_set_watercolor(color_t color);

/* misc */
v2d_t level_size();
float level_gravity();
void level_add_to_score(int score);
void level_call_dialogbox(const char *title, const char *message);
void level_hide_dialogbox();

#endif
