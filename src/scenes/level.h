/*
 * Open Surge Engine
 * level.h - code for the game levels
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#include <stdbool.h>
#include "../core/color.h"
#include "../util/v2d.h"
#include "../entities/brick.h"

/* scene methods */
void level_init(void *path_to_lev_file); /* pass a string containing the path to the .lev file */
void level_update();
void level_render();
void level_release();




/* forward declarations */
struct image_t;
struct actor_t;
struct player_t;
struct brick_t;
struct item_t;
struct item_list_t;
struct enemy_t;
struct enemy_list_t;
struct sound_t;
struct music_t;
struct bgtheme_t;
struct obstaclemap_t;

/* level data */
const char* level_file();
const char* level_name();
int level_act();
const char* level_version();
const char* level_author();
const char* level_license();
const char* level_bgtheme();

/* players */
void level_change_player(struct player_t *new_player); /* character switching */
struct player_t* level_player(); /* active player */
struct player_t* level_get_player_by_name(const char* name);
struct player_t* level_get_player_by_id(int id);

/* level objects */
struct brick_t* level_create_brick(int id, v2d_t position, bricklayer_t layer, brickflip_t flip);
struct item_t* level_create_legacy_item(int id, v2d_t position);
struct enemy_t* level_create_legacy_object(const char *name, v2d_t position);
surgescript_object_t* level_create_object(const char* object_name, v2d_t position);
surgescript_object_t* level_get_entity_by_id(const char* entity_id);
const char* level_get_entity_id(const surgescript_object_t* entity);
surgescript_object_t* level_child_object(const char* object_name);
bool level_is_setup_object(const char* object_name);
const struct obstaclemap_t* level_obstaclemap();
void level_set_obstaclemap_dirty();

/* camera */
void level_set_camera_focus(struct actor_t *act);
struct actor_t* level_get_camera_focus();
int level_inside_screen(int x, int y, int w, int h);

/* music */
struct music_t* level_music();

/* management */
void level_change(const char* path_to_lev_file); /* change the level */
int level_persist(); /* save the current level */
void level_clear(struct actor_t *end_sign);
void level_undo_clear();
int level_has_been_cleared();
void level_jump_to_next_stage();
void level_ask_to_leave();
void level_pause();
void level_restart();
void level_abort();
void level_push_quest(const char* path_to_qst_file);
void level_quit_with_gameover();

/* level state */
void level_save_state();
void level_set_spawnpoint(v2d_t newpos);
v2d_t level_spawnpoint();
int level_waterlevel();
void level_set_waterlevel(int ycoord);
color_t level_watercolor();
void level_set_watercolor(color_t color);
void level_set_act(int new_act_number);
void level_change_background(const char* filepath);
const struct bgtheme_t* level_background();

/* misc */
v2d_t level_size();
int level_height_at(int xpos);
float level_time();
float level_gravity();
void level_add_to_score(int score);
void level_call_dialogbox(const char *title, const char *message);
void level_hide_dialogbox();

/* editor & development */
int level_editmode(); /* active editor? */
int level_is_displaying_gizmos(); /* are we displaying gizmos for visual debugging? */
void level_enter_debug_mode();
bool level_is_in_debug_mode();

#endif