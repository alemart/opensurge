/*
 * Open Surge Engine
 * object_compiler.c - compiles object scripts
 * Copyright (C) 2010-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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
 * enable_player_roll, disable_player_roll
 */

#include <stdarg.h>
#include <ctype.h>
#include "object_compiler.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/nanocalc/nanocalc.h"

#include "object_decorators/look.h"
#include "object_decorators/lock_camera.h"
#include "object_decorators/gravity.h"
#include "object_decorators/jump.h"
#include "object_decorators/walk.h"
#include "object_decorators/elliptical_trajectory.h"
#include "object_decorators/bounce_player.h"
#include "object_decorators/bullet_trajectory.h"
#include "object_decorators/mosquito_movement.h"
#include "object_decorators/on_event.h"
#include "object_decorators/set_animation.h"
#include "object_decorators/set_obstacle.h"
#include "object_decorators/set_alpha.h"
#include "object_decorators/set_angle.h"
#include "object_decorators/set_scale.h"
#include "object_decorators/showhide.h"
#include "object_decorators/enemy.h"
#include "object_decorators/move_player.h"
#include "object_decorators/player_movement.h"
#include "object_decorators/player_action.h"
#include "object_decorators/set_absolute_position.h"
#include "object_decorators/set_player_animation.h"
#include "object_decorators/set_player_speed.h"
#include "object_decorators/set_player_position.h"
#include "object_decorators/set_player_inputmap.h"
#include "object_decorators/hit_player.h"
#include "object_decorators/kill_player.h"
#include "object_decorators/children.h"
#include "object_decorators/create_item.h"
#include "object_decorators/change_closest_object_state.h"
#include "object_decorators/destroy.h"
#include "object_decorators/dialog_box.h"
#include "object_decorators/audio.h"
#include "object_decorators/clear_level.h"
#include "object_decorators/next_level.h"
#include "object_decorators/add_to_score.h"
#include "object_decorators/add_lives.h"
#include "object_decorators/add_collectibles.h"
#include "object_decorators/ask_to_leave.h"
#include "object_decorators/pause.h"
#include "object_decorators/observe_player.h"
#include "object_decorators/attach_to_player.h"
#include "object_decorators/variables.h"
#include "object_decorators/set_zindex.h"
#include "object_decorators/textout.h"
#include "object_decorators/simulate_button.h"
#include "object_decorators/switch_character.h"
#include "object_decorators/return_to_previous_state.h"
#include "object_decorators/load_level.h"
#include "object_decorators/restart_level.h"
#include "object_decorators/save_level.h"
#include "object_decorators/camera_focus.h"
#include "object_decorators/execute.h"
#include "object_decorators/launch_url.h"
#include "object_decorators/quest.h"
#include "object_decorators/reset_globals.h"

/* expression evaluator (nanocalc) helper */
/* given a string, makes an expression_t object */
/* ps: an objectmachine_t** named m must be available in the context */
#define EXPRESSION(str)  expression_new((str), objectvm_get_symbol_table(((*m)->get_object_instance(*m))->vm))

/* compile error macro helper */
/* given an error message, kills the program and tells where the error has ocurred in the script */
/* ps: a parsetree_statement_t* named stmt must be available in the context */
#define COMPILE_ERROR(...) compile_error(stmt, __VA_ARGS__)




/* private stuff ;) */
#define DEFAULT_STATE                   "main"
#define STACKMAX                        1024
static void compile_command(objectmachine_t** machine_ref, const char *command, int n, const char *param[], const parsetree_statement_t *stmt);
static int traverse_object(const parsetree_statement_t *stmt, void *object);
static int traverse_object_state(const parsetree_statement_t *stmt, void *machine);
static int push_object_state(const parsetree_statement_t *stmt, void *machine);
static void compile_error(const parsetree_statement_t *stmt, const char *format, ...);
static struct { const parsetree_statement_t *stmt; void *machine; } stack[STACKMAX];
static int stacksize;

/* -------------------------------------- */

/*
   available actions:
   -----------------------------------------------
   they all receive:
   1. m         : reference to an object machine (used to add a decorator to the machine)
   2. n         : the length of the array containing the parameters
   3. p[0..n-1] : the array containing the parameters
*/

/* basic actions */
static void set_animation(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_animation_frame(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_animation_speed_factor(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_obstacle(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_alpha(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_angle(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_scale(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_absolute_position(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void hide(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void show(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void enemy(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* player interaction */
static void lock_camera(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void move_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void kill_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void hit_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void burn_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void shock_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void acid_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void add_lives(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void add_collectibles(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void add_to_score(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_player_animation(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void enable_player_movement(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void disable_player_movement(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_player_xspeed(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_player_yspeed(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_player_position(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_player_inputmap(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void bounce_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void observe_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void observe_current_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void observe_active_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void observe_all_players(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void attach_to_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void springfy_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void roll_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void enable_player_roll(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void disable_player_roll(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void strong_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void weak_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void w_player_enter_water(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void w_player_leave_water(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void w_player_breathe(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void w_player_drown(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void w_player_reset_underwater_timer(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void switch_character(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void simulate_button_down(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void simulate_button_up(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* movement */
static void walk(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void gravity(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void jump(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void bullet_trajectory(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void elliptical_trajectory(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void mosquito_movement(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void look_left(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void look_right(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void look_at_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void look_at_walking_direction(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* object management */
static void create_item(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void change_closest_object_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void create_child(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void change_child_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void change_parent_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void destroy(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void set_zindex(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* text output */
static void t_textout(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void t_textout_centre(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void t_textout_right(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* fast loops */
static void execute(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* events */
static void change_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void return_to_previous_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_timeout(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_animation_finished(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_random_event(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_level_cleared(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_attack(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_rect_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_observed_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_stop(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_walk(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_run(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_jump(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_spring(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_roll(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_push(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_gethit(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_death(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_brake(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_ledge(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_drown(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_breathe(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_duck(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_lookup(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_wait(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_win(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_in_the_air(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_underwater(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_speedshoes(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_player_invincible(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_no_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_fire_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_thunder_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_water_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_acid_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_wind_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_brick_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_floor_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_ceiling_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_left_wall_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_right_wall_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_button_down(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_button_pressed(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_button_up(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_camera_focus(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_camera_focus_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_camera_lock(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void on_music_play(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* variables */
static void var_let(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void var_if(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void var_unless(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void var_resetglobals(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* level */
static void show_dialog_box(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void hide_dialog_box(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void clear_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void next_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void ask_to_leave(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void l_pause(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void restart_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void save_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void load_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void request_camera_focus(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void drop_camera_focus(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* quest */
static void push_quest(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void pop_quest(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* audio commands */
static void audio_play_sample(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void audio_play_music(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void audio_play_level_music(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void audio_set_music_volume(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);
static void audio_stop_sample(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* misc */
static void m_launch_url(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt);

/* -------------------------------------- */

/* command table */
typedef struct {
    const char *command;
    void (*action)(objectmachine_t**,int,const char**,const parsetree_statement_t*);
} entry_t;

static entry_t command_table[] = {
    /* basic actions */
    { "set_animation", set_animation },
    { "set_animation_frame", set_animation_frame },
    { "set_animation_speed_factor", set_animation_speed_factor },
    { "set_obstacle", set_obstacle },
    { "set_alpha", set_alpha },
    { "set_angle", set_angle },
    { "set_scale", set_scale },
    { "set_absolute_position", set_absolute_position },
    { "hide", hide },
    { "show", show },
    { "enemy", enemy },

    /* player interaction */
    { "lock_camera", lock_camera },
    { "move_player", move_player },
    { "kill_player", kill_player },
    { "hit_player", hit_player },
    { "burn_player", burn_player },
    { "shock_player", shock_player },
    { "acid_player", acid_player },
    { "add_lives", add_lives },
    { "add_rings", add_collectibles },
    { "add_collectibles", add_collectibles },
    { "add_to_score", add_to_score },
    { "set_player_animation", set_player_animation },
    { "enable_player_movement", enable_player_movement },
    { "disable_player_movement", disable_player_movement },
    { "set_player_xspeed", set_player_xspeed },
    { "set_player_yspeed", set_player_yspeed },
    { "set_player_position", set_player_position },
    { "set_player_inputmap", set_player_inputmap },
    { "bounce_player", bounce_player },
    { "observe_player", observe_player },
    { "observe_current_player", observe_current_player },
    { "observe_active_player", observe_active_player },
    { "observe_all_players", observe_all_players },
    { "observe_next_player", observe_all_players },
    { "attach_to_player", attach_to_player },
    { "springfy_player", springfy_player },
    { "roll_player", roll_player },
    { "enable_player_roll", enable_player_roll },
    { "disable_player_roll", disable_player_roll },
    { "strong_player", strong_player },
    { "weak_player", weak_player },
    { "player_enter_water", w_player_enter_water },
    { "player_leave_water", w_player_leave_water },
    { "player_breathe", w_player_breathe },
    { "player_drown", w_player_drown },
    { "player_reset_underwater_timer", w_player_reset_underwater_timer },
    { "switch_character", switch_character },
    { "simulate_button_down", simulate_button_down },
    { "simulate_button_up", simulate_button_up },

    /* movement */
    { "walk", walk },
    { "gravity", gravity },
    { "jump", jump },
    { "move", bullet_trajectory },
    { "bullet_trajectory", bullet_trajectory },
    { "elliptical_trajectory", elliptical_trajectory },
    { "mosquito_movement", mosquito_movement },
    { "look_left", look_left },
    { "look_right", look_right },
    { "look_at_player", look_at_player },
    { "look_at_walking_direction", look_at_walking_direction },

    /* object management */
    { "create_item", create_item },
    { "change_closest_object_state", change_closest_object_state },
    { "create_child", create_child },
    { "change_child_state", change_child_state },
    { "change_parent_state", change_parent_state },
    { "destroy", destroy },
    { "set_zindex", set_zindex },

    /* text output */
    { "textout", t_textout },
    { "textout_centre", t_textout_centre },
    { "textout_right", t_textout_right },

    /* fast loops */
    { "execute", execute },

    /* events */
    { "change_state", change_state },
    { "return_to_previous_state", return_to_previous_state },
    { "on_timeout", on_timeout },
    { "on_collision", on_collision },
    { "on_animation_finished", on_animation_finished },
    { "on_random_event", on_random_event },
    { "on_level_cleared", on_level_cleared },
    { "on_player_collision", on_player_collision },
    { "on_player_attack", on_player_attack },
    { "on_player_rect_collision", on_player_rect_collision },
    { "on_observed_player", on_observed_player },
    { "on_player_stop", on_player_stop },
    { "on_player_walk", on_player_walk },
    { "on_player_run", on_player_run },
    { "on_player_jump", on_player_jump },
    { "on_player_roll", on_player_roll },
    { "on_player_spring", on_player_spring },
    { "on_player_push", on_player_push },
    { "on_player_gethit", on_player_gethit },
    { "on_player_death", on_player_death },
    { "on_player_brake", on_player_brake },
    { "on_player_ledge", on_player_ledge },
    { "on_player_drown", on_player_drown },
    { "on_player_lookup", on_player_lookup },
    { "on_player_duck", on_player_duck },
    { "on_player_breathe", on_player_breathe },
    { "on_player_wait", on_player_wait },
    { "on_player_win", on_player_win },
    { "on_player_in_the_air", on_player_in_the_air },
    { "on_player_underwater", on_player_underwater },
    { "on_player_speedshoes", on_player_speedshoes },
    { "on_player_invincible", on_player_invincible },
    { "on_no_shield", on_no_shield },
    { "on_shield", on_shield },
    { "on_fire_shield", on_fire_shield },
    { "on_thunder_shield", on_thunder_shield },
    { "on_water_shield", on_water_shield },
    { "on_acid_shield", on_acid_shield },
    { "on_wind_shield", on_wind_shield },
    { "on_brick_collision", on_brick_collision },
    { "on_floor_collision", on_floor_collision },
    { "on_ceiling_collision", on_ceiling_collision },
    { "on_left_wall_collision", on_left_wall_collision },
    { "on_right_wall_collision", on_right_wall_collision },
    { "on_button_down", on_button_down },
    { "on_button_pressed", on_button_pressed },
    { "on_button_up", on_button_up },
    { "on_camera_focus", on_camera_focus },
    { "on_camera_focus_player", on_camera_focus_player },
    { "on_camera_lock", on_camera_lock },
    { "on_music_play", on_music_play },

    /* variables */
    { "let", var_let },
    { "if", var_if },
    { "unless", var_unless },
    { "reset_globals", var_resetglobals },

    /* level */
    { "show_dialog_box", show_dialog_box },
    { "hide_dialog_box", hide_dialog_box },
    { "clear_level", clear_level },
    { "next_level", next_level },
    { "ask_to_leave", ask_to_leave },
    { "pause", l_pause },
    { "restart_level", restart_level },
    { "save_level", save_level },
    { "load_level", load_level },
    { "request_camera_focus", request_camera_focus },
    { "drop_camera_focus", drop_camera_focus },

    /* quest */
    { "push_quest", push_quest },
    { "pop_quest", pop_quest },

    /* audio commands */
    { "play_sample", audio_play_sample },
    { "stop_sample", audio_stop_sample },
    { "play_music", audio_play_music },
    { "play_level_music", audio_play_level_music },
    { "set_music_volume", audio_set_music_volume },

    /* misc */
    { "launch_url", m_launch_url },

    /* end of table */
    { NULL, NULL }
};




/* -------------------------------------- */

/* public methods */

/*
 * objectcompiler_compile()
 * Compiles the given script
 */
void objectcompiler_compile(object_t *obj, const parsetree_program_t *script)
{
    nanoparser_traverse_program_ex(script, (void*)obj, traverse_object);
    objectvm_reset_history(obj->vm);
    objectvm_set_current_state(obj->vm, DEFAULT_STATE);
}




/* -------------------------------------- */

/* private methods */
int traverse_object(const parsetree_statement_t* stmt, void *object)
{
    object_t *e = (object_t*)object;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    objectmachine_t **machine_ref;

    if(str_icmp(id, "state") == 0) {
        const parsetree_parameter_t *p1, *p2;
        const char *state_name;
        const parsetree_program_t *state_code;

        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "Object script error: state name is expected");
        nanoparser_expect_program(p2, "Object script error: state code is expected");

        state_name = nanoparser_get_string(p1);
        state_code = nanoparser_get_program(p2);

        objectvm_create_state(e->vm, state_name);
        objectvm_set_current_state(e->vm, state_name);
        machine_ref = objectvm_get_reference_to_current_state(e->vm);

        stacksize = 0;
        nanoparser_traverse_program_ex(state_code, (void*)machine_ref, push_object_state);
        while(stacksize-- > 0) /* traverse in reverse order - note the order of the decorators */
            traverse_object_state(stack[stacksize].stmt, stack[stacksize].machine);

        (*machine_ref)->init(*machine_ref);
    }
    else if(str_icmp(id, "requires") == 0) {
        if(nanoparser_get_number_of_parameters(param_list) == 1) {
            const parsetree_parameter_t *p1;
            int i, requires[3];

            p1 = nanoparser_get_nth_parameter(param_list, 1);
            nanoparser_expect_string(p1, "Object script error: requires is expected");

            sscanf(nanoparser_get_string(p1), "%d.%d.%d", &requires[0], &requires[1], &requires[2]);
            for(i=0; i<3; i++) requires[i] = clip(requires[i], 0, 99);
            if(game_version_compare(requires[0], requires[1], requires[2]) < 0) {
                COMPILE_ERROR(
                    "Object \"%s\" requires version %d.%d.%d or greater of the game engine.\nYours is %s\nPlease check our for new versions at %s",
                    e->name, requires[0], requires[1], requires[2], GAME_VERSION_STRING, GAME_WEBSITE
                );
            }
        }
        else
            COMPILE_ERROR("Object script error: command 'requires' expects one parameter - minimum required engine version");

    }
    else if(str_icmp(id, "destroy_if_far_from_play_area") == 0) {
        if(nanoparser_get_number_of_parameters(param_list) == 0)
            e->preserve = FALSE;
        else
            COMPILE_ERROR("Object script error: command 'destroy_if_far_from_play_area' expects no parameters");
    }
    else if(str_icmp(id, "always_active") == 0) {
        if(nanoparser_get_number_of_parameters(param_list) == 0)
            e->always_active = TRUE;
        else
            COMPILE_ERROR("Object script error: command 'always_active' expects no parameters");
    }
    else if(str_icmp(id, "hide_unless_in_editor_mode") == 0) {
        if(nanoparser_get_number_of_parameters(param_list) == 0)
            e->hide_unless_in_editor_mode = TRUE;
        else
            COMPILE_ERROR("Object script error: command 'hide_unless_in_editor_mode' expects no parameters");
    }
    else if(str_icmp(id, "detach_from_camera") == 0) {
        if(nanoparser_get_number_of_parameters(param_list) == 0)
            e->detach_from_camera = TRUE;
        else
            COMPILE_ERROR("Object script error: command 'detach_from_camera' expects no parameters");
    }
    else if(str_icmp(id, "annotation") == 0) {
        if(nanoparser_get_number_of_parameters(param_list) == 1) {
            const parsetree_parameter_t *param = nanoparser_get_nth_parameter(param_list, 1);
            nanoparser_expect_string(param, "Object script error: annotation string is expected");
            e->annotation = nanoparser_get_string(param);
        }
        else
            COMPILE_ERROR("Object script error: command 'annotation' expects only one parameter");
    }
    else if(str_icmp(id, "category") == 0) {
        int n = nanoparser_get_number_of_parameters(param_list);
        if(n > 0) {
            if(e->category == NULL) {
                int i;

                e->category = mallocx(n * sizeof(*(e->category)));
                e->category_count = n;

                for(i=1; i<=n; i++) {
                    const parsetree_parameter_t *param = nanoparser_get_nth_parameter(param_list, i);
                    nanoparser_expect_string(param, "Object script error: category string is expected");
                    e->category[i-1] = nanoparser_get_string(param);
                }
            }
        }
        else
            COMPILE_ERROR("Object script error: field 'category' can't be blank");
    }
    else
        COMPILE_ERROR("Object script error: unknown keyword '%s'", id);

    return 0;
}

int traverse_object_state(const parsetree_statement_t* stmt, void *machine)
{
    objectmachine_t **ref = (objectmachine_t**)machine; /* reference to the current object machine */
    const char *id = nanoparser_get_identifier(stmt); /* command string */
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const char **p_k;
    int i, n;

    /* creates the parameter list: p_k[0..n-1] */
    n = nanoparser_get_number_of_parameters(param_list);
    p_k = mallocx(n * (sizeof *p_k));
    for(i=0; i<n; i++) {
        const parsetree_parameter_t *p = nanoparser_get_nth_parameter(param_list, 1+i);
        nanoparser_expect_string(p, "Object script error: command parameters must be strings");
        p_k[i] = nanoparser_get_string(p);
    }

    /* adds the corresponding decorator to the machine */
    compile_command(ref, id, n, p_k, stmt);

    /* releases the parameter list */
    free(p_k);

    /* done! :-) */
    return 0;
}

int push_object_state(const parsetree_statement_t* stmt, void *machine)
{
    if(stacksize < STACKMAX) {
        stack[stacksize].stmt = stmt;
        stack[stacksize].machine = machine;
        stacksize++;
    }
    else
        COMPILE_ERROR("Object script error: you may write %d commands or less per state", STACKMAX);

    return 0;
}

void compile_command(objectmachine_t** machine_ref, const char *command, int n, const char *param[], const parsetree_statement_t *stmt)
{
    int i = 0;
    entry_t e = command_table[i++];

    /* finds the corresponding command in the table */
    while(e.command != NULL && e.action != NULL) {
        if(str_icmp(e.command, command) == 0) {
            (e.action)(machine_ref, n, (const char**)param, stmt);
            return;
        }

        e = command_table[i++];
    }

    COMPILE_ERROR("Object script error - unknown command: '%s'", command);
}


void compile_error(const parsetree_statement_t *stmt, const char *format, ...)
{
    static char buf[1024];
    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    fatal_error("%s\nin \"%s\" near line %d", buf, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
}


/* -------------------------------------- */

/* action programming */
void set_animation(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_setanimation_new(*m, p[0], EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - set_animation expects two parameters: sprite_name, animation_id");
}

void set_animation_frame(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setanimationframe_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_animation_frame expects one parameter: frame_number");
}

void set_animation_speed_factor(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setanimationspeedfactor_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_animation_speed_factor expects one parameter: speed_multiplier");
}

void set_obstacle(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setobstacle_new(*m, atob(p[0]), EXPRESSION("0"));
    else if(n == 2)
        *m = objectdecorator_setobstacle_new(*m, atob(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - set_obstacle expects at least one and at most two parameters: is_obstacle (TRUE or FALSE) [, angle]");
}

void set_absolute_position(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_setabsoluteposition_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - set_absolute_position expects two parameters: x_position, y_position");
}

void set_alpha(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setalpha_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_alpha expects one parameter: alpha (0.0 (transparent) <= alpha <= 1.0 (opaque))");
}

void set_angle(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setangle_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_angle expects one parameter: angle (0 <= angle < 360)");
}

void set_scale(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_setscale_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - set_scale expects two parameters: scale_x, scale_y");
}

void hide(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_hide_new(*m);
    else
        COMPILE_ERROR("Object script error - hide expects no parameters");
}

void show(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_show_new(*m);
    else
        COMPILE_ERROR("Object script error - show expects no parameters");
}

void ask_to_leave(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_asktoleave_new(*m);
    else
        COMPILE_ERROR("Object script error - ask_to_leave expects no parameters");
}

void l_pause(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_pause_new(*m);
    else
        COMPILE_ERROR("Object script error - pause expects no parameters");
}

void restart_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_restartlevel_new(*m);
    else
        COMPILE_ERROR("Object script error - restart_level expects no parameters");
}

void save_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_savelevel_new(*m);
    else
        COMPILE_ERROR("Object script error - save_level expects no parameters");
}

void load_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_loadlevel_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - load_level expects one parameter: level_path");
}

void request_camera_focus(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_requestcamerafocus_new(*m);
    else
        COMPILE_ERROR("Object script error - request_camera_focus expects no parameters");
}

void drop_camera_focus(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_dropcamerafocus_new(*m);
    else
        COMPILE_ERROR("Object script error - drop_camera_focus expects no parameters");
}

void bullet_trajectory(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_bullettrajectory_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - bullet_trajectory expects two parameters: speed_x, speed_y");
}

void create_item(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 3)
        *m = objectdecorator_createitem_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]), EXPRESSION(p[2]));
    else
        COMPILE_ERROR("Object script error - create_item expects three parameters: item_id, offset_x, offset_y");
}

void create_child(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_createchild_new(*m, p[0], EXPRESSION("0"), EXPRESSION("0"), "\201"); /* dummy child name */
    else if(n == 2)
        *m = objectdecorator_createchild_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION("0"), "\201");
    else if(n == 3)
        *m = objectdecorator_createchild_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), "\201");
    else if(n == 4)
        *m = objectdecorator_createchild_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3]);
    else
        COMPILE_ERROR("Object script error - create_child expects at least one and at most four parameters: object_name [, offset_x [, offset_y [, child_name]]]");
}

void change_child_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_changechildstate_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - change_child_state expects two parameters: child_name, new_state_name");
}

void change_parent_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_changeparentstate_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - change_parent_state expects one parameter: new_state_name");
}

void destroy(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_destroy_new(*m);
    else
        COMPILE_ERROR("Object script error - destroy expects no parameters");
}

void set_zindex(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setzindex_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_zindex expects one parameter: zindex, where 0.0 <= zindex <= 1.0");
}

void elliptical_trajectory(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 4)
        *m = objectdecorator_ellipticaltrajectory_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]), EXPRESSION("0"), EXPRESSION("0"));
    else if(n == 5)
        *m = objectdecorator_ellipticaltrajectory_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]), EXPRESSION(p[4]), EXPRESSION("0"));
    else if(n == 6)
        *m = objectdecorator_ellipticaltrajectory_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]), EXPRESSION(p[4]), EXPRESSION(p[5]));
    else
        COMPILE_ERROR("Object script error - elliptical_trajectory expects at least four and at most six parameters: amplitude_x, amplitude_y, angularspeed_x, angularspeed_y [, initialphase_x [, initialphase_y]]");
}

void gravity(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_gravity_new(*m);
    else
        COMPILE_ERROR("Object script error - gravity expects no parameters");
}

void look_left(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_lookleft_new(*m);
    else
        COMPILE_ERROR("Object script error - look_left expects no parameters");
}

void look_right(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_lookright_new(*m);
    else
        COMPILE_ERROR("Object script error - look_right expects no parameters");
}

void look_at_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_lookatplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - look_at_player expects no parameters");
}

void look_at_walking_direction(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_lookatwalkingdirection_new(*m);
    else
        COMPILE_ERROR("Object script error - look_at_walking_direction expects no parameters");
}

void mosquito_movement(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_mosquitomovement_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - mosquito_movement expects one parameter: speed");
}

void move_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_moveplayer_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - move_player expects two parameters: speed_x, speed_y");
}

void kill_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_killplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - kill_player expects no parameters");
}

void hit_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_hitplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - hit_player expects no parameters");
}

void enemy(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_enemy_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - enemy expects one parameter: score");
}

void walk(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_walk_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - walk expects one parameter: speed");
}

void change_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onalways_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - change_state expects one parameter: new_state_name");
}

void return_to_previous_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_returntopreviousstate_new(*m);
    else
        COMPILE_ERROR("Object script error - return_to_previous_state expects no parameters");
}

void on_timeout(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_ontimeout_new(*m, EXPRESSION(p[0]), p[1]);
    else
        COMPILE_ERROR("Object script error - on_timeout expects two parameters: timeout (in seconds), new_state_name");
}

void on_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_oncollision_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - on_collision expects two parameters: object_name, new_state_name");
}

void on_animation_finished(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onanimationfinished_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_animation_finished expects one parameter: new_state_name");
}

void on_random_event(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_onrandomevent_new(*m, EXPRESSION(p[0]), p[1]);
    else
        COMPILE_ERROR("Object script error - on_random_event expects two parameters: probability (0.0 <= probability <= 1.0), new_state_name");
}

void on_level_cleared(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onlevelcleared_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_level_cleared expects one parameter: new_state_name");
}

void on_player_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayercollision_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_collision expects one parameter: new_state_name");
}

void on_player_attack(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerattack_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_attack expects one parameter: new_state_name");
}

void on_player_rect_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 5)
        *m = objectdecorator_onplayerrectcollision_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]), p[4]);
    else
        COMPILE_ERROR("Object script error - on_player_rect_collision expects five parameters: offset_x1, offset_y1, offset_x2, offset_y2, new_state_name");
}

void on_observed_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_onobservedplayer_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - on_observed_player expects two parameters: player_name, new_state_name");
}

void on_player_stop(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerstop_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_stop expects one parameter: new_state_name");
}

void on_player_walk(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerwalk_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_walk expects one parameter: new_state_name");
}

void on_player_run(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerrun_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_run expects one parameter: new_state_name");
}

void on_player_jump(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerjump_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_jump expects one parameter: new_state_name");
}

void on_player_spring(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerspring_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_spring expects one parameter: new_state_name");
}

void on_player_roll(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerroll_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_roll expects one parameter: new_state_name");
}

void on_player_push(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerpush_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_push expects one parameter: new_state_name");
}

void on_player_gethit(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayergethit_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_gethit expects one parameter: new_state_name");
}

void on_player_death(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerdeath_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_death expects one parameter: new_state_name");
}

void on_player_brake(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerbrake_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_brake expects one parameter: new_state_name");
}

void on_player_ledge(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerledge_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_ledge expects one parameter: new_state_name");
}

void on_player_drown(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerdrown_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_drown expects one parameter: new_state_name");
}

void on_player_breathe(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerbreathe_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_breathe expects one parameter: new_state_name");
}

void on_player_duck(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerduck_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_duck expects one parameter: new_state_name");
}

void on_player_lookup(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerlookup_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_lookup expects one parameter: new_state_name");
}

void on_player_wait(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerwait_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_wait expects one parameter: new_state_name");
}

void on_player_win(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerwin_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_win expects one parameter: new_state_name");
}

void on_player_in_the_air(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerintheair_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_in_the_air expects one parameter: new_state_name");
}

void on_player_underwater(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerunderwater_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_underwater expects one parameter: new_state_name");
}

void on_player_speedshoes(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerspeedshoes_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_ultrafast expects one parameter: new_state_name");
}

void on_player_invincible(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onplayerinvincible_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_player_invincible expects one parameter: new_state_name");
}

void on_no_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onnoshield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_no_shield expects one parameter: new_state_name");
}

void on_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onshield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_shield expects one parameter: new_state_name");
}

void on_fire_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onfireshield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_fire_shield expects one parameter: new_state_name");
}

void on_thunder_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onthundershield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_thunder_shield expects one parameter: new_state_name");
}

void on_water_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onwatershield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_water_shield expects one parameter: new_state_name");
}

void on_acid_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onacidshield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_acid_shield expects one parameter: new_state_name");
}

void on_wind_shield(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onwindshield_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_wind_shield expects one parameter: new_state_name");
}

void on_brick_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onbrickcollision_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_brick_collision expects one parameter: new_state_name");
}

void on_floor_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onfloorcollision_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_floor_collision expects one parameter: new_state_name");
}

void on_ceiling_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onceilingcollision_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_ceiling_collision expects one parameter: new_state_name");
}

void on_left_wall_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onleftwallcollision_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_left_wall_collision expects one parameter: new_state_name");
}

void on_right_wall_collision(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onrightwallcollision_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_right_wall_collision expects one parameter: new_state_name");
}

void on_button_down(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_onbuttondown_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - on_button_down expects two parameters: button_name, new_state_name");
}

void on_button_pressed(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_onbuttonpressed_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - on_button_pressed expects two parameters: button_name, new_state_name");
}

void on_button_up(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_onbuttonup_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - on_button_up expects two parameters: button_name, new_state_name");
}

void on_music_play(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_onmusicplay_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_music_play expects one parameter: new_state_name");
}

void on_camera_focus(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_oncamerafocus_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_camera_focus expects one parameter: new_state_name");
}

void on_camera_focus_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_oncamerafocusplayer_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_camera_focus_player expects one parameter: new_state_name");
}

void on_camera_lock(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_oncameralock_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - on_camera_lock expects one parameter: new_state_name");
}

void change_closest_object_state(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_changeclosestobjectstate_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - change_closest_object_state expects two parameters: object_name, new_state_name");
}

void burn_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_burnplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - burn_player expects no parameters");
}

void shock_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_shockplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - shock_player expects no parameters");
}

void acid_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_acidplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - acid_player expects no parameters");
}

void add_lives(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_addlives_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - add_lives expects one parameter: number_of_lives");
}

void add_collectibles(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_addcollectibles_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - add_collectibles expects one parameter: number_of_collectibles");
}

void add_to_score(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_addtoscore_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - add_to_score expects one parameter: score");
}

void audio_play_sample(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_playsample_new(*m, p[0], EXPRESSION("1.0"), EXPRESSION("0.0"), EXPRESSION("1.0"), EXPRESSION("0"));
    else if(n == 2)
        *m = objectdecorator_playsample_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION("0.0"), EXPRESSION("1.0"), EXPRESSION("0"));
    else if(n == 3)
        *m = objectdecorator_playsample_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION("1.0"), EXPRESSION("0"));
    else if(n == 4)
        *m = objectdecorator_playsample_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]), EXPRESSION("0"));
    else if(n == 5)
        *m = objectdecorator_playsample_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]), EXPRESSION(p[4]));
    else
        COMPILE_ERROR("Object script error - play_sample expects at least one and at most five parameters: sound_name [, volume [, pan [, frequency [, loops]]]]");
}

void audio_play_music(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_playmusic_new(*m, p[0], EXPRESSION("0"));
    else if(n == 2)
        *m = objectdecorator_playmusic_new(*m, p[0], EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - play_music expects at least one and at most two parameters: music_name [, loops]");
}

void audio_play_level_music(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_playlevelmusic_new(*m);
    else
        COMPILE_ERROR("Object script error - play_level_music expects no parameters");
}

void audio_set_music_volume(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setmusicvolume_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_music_volume expects one parameter: volume");
}

void audio_stop_sample(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_stopsample_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - stop_sample expects one parameter: sample name");
}

void show_dialog_box(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_showdialogbox_new(*m, p[0], p[1]);
    else
        COMPILE_ERROR("Object script error - show_dialog_box expects two parameters: title, message");
}

void hide_dialog_box(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_hidedialogbox_new(*m);
    else
        COMPILE_ERROR("Object script error - hide_dialog_box expects no parameters");
}

void clear_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_clearlevel_new(*m);
    else
        COMPILE_ERROR("Object script error - clear_level expects no parameters");
}

void next_level(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_nextlevel_new(*m);
    else
        COMPILE_ERROR("Object script error - next_level expects no parameters");
}

void jump(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_jump_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - jump expects one parameter: jump_strength");
}

void set_player_animation(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_setplayeranimation_new(*m, p[0], EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - set_player_animation expects two parameters: sprite_name, animation_id");
}

void enable_player_movement(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_enableplayermovement_new(*m);
    else
        COMPILE_ERROR("Object script error - enable_player_movement expects no parameters");
}

void disable_player_movement(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_disableplayermovement_new(*m);
    else
        COMPILE_ERROR("Object script error - disable_player_movement expects no parameters");
}

void set_player_xspeed(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setplayerxspeed_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_player_xspeed expects one parameter: speed");
}

void set_player_yspeed(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setplayeryspeed_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - set_player_yspeed expects one parameter: speed");
}

void set_player_position(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_setplayerposition_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - set_player_position expects two parameters: xpos, ypos");
}

void set_player_inputmap(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_setplayerinputmap_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - set_player_inputmap expects one parameter: inputmap_name");
}

void bounce_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_bounceplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - bounce_player expects no parameters");
}

void switch_character(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_switchcharacter_new(*m, NULL, FALSE);
    else if(n == 1)
        *m = objectdecorator_switchcharacter_new(*m, p[0], FALSE);
    else if(n == 2)
        *m = objectdecorator_switchcharacter_new(*m, p[0], atob(p[1]));
    else
        COMPILE_ERROR("Object script error - switch_character expects at most two parameters: [player_name [, force_switch]]");
}

void simulate_button_down(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_simulatebuttondown_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - simulate_button_down expects one parameter: button_name");
}

void simulate_button_up(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_simulatebuttonup_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - simulate_button_up expects one parameter: button_name");
}

void lock_camera(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 4)
        *m = objectdecorator_lockcamera_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]), EXPRESSION(p[2]), EXPRESSION(p[3]));
    else
        COMPILE_ERROR("Object script error - lock_camera expects four parameters: x1, y1, x2, y2");
}

void observe_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_observeplayer_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - observe_player expects one parameter: player_name");
}

void observe_current_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_observecurrentplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - observe_current_player expects no parameters");
}

void observe_active_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_observeactiveplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - observe_active_player expects no parameters");
}

void observe_all_players(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_observeallplayers_new(*m);
    else
        COMPILE_ERROR("Object script error - observe_all_players expects no parameters");
}

void attach_to_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_attachtoplayer_new(*m, EXPRESSION("0"), EXPRESSION("0"));
    else if(n == 1)
        *m = objectdecorator_attachtoplayer_new(*m, EXPRESSION(p[0]), EXPRESSION("0"));
    else if(n == 2)
        *m = objectdecorator_attachtoplayer_new(*m, EXPRESSION(p[0]), EXPRESSION(p[1]));
    else
        COMPILE_ERROR("Object script error - attach_to_player expects at most two parameters: [offset_x [, offset_y]]");
}

void springfy_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_springfyplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - springfy_player expects no parameters");
}

void roll_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_rollplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - roll_player expects no parameters");
}

void enable_player_roll(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_enableplayerroll_new(*m);
    else
        COMPILE_ERROR("Object script error - enable_player_roll expects no parameters");
}

void disable_player_roll(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_disableplayerroll_new(*m);
    else
        COMPILE_ERROR("Object script error - disable_player_roll expects no parameters");
}

void strong_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_strongplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - strong_player expects no parameters");
}

void weak_player(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_weakplayer_new(*m);
    else
        COMPILE_ERROR("Object script error - weak_player expects no parameters");
}

void w_player_enter_water(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_playerenterwater_new(*m);
    else
        COMPILE_ERROR("Object script error - player_enter_water expects no parameters");
}

void w_player_leave_water(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_playerleavewater_new(*m);
    else
        COMPILE_ERROR("Object script error - player_leave_water expects no parameters");
}

void w_player_breathe(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_playerbreathe_new(*m);
    else
        COMPILE_ERROR("Object script error - player_breathe expects no parameters");
}

void w_player_drown(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_playerdrown_new(*m);
    else
        COMPILE_ERROR("Object script error - player_drown expects no parameters");
}

void w_player_reset_underwater_timer(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_playerresetunderwatertimer_new(*m);
    else
        COMPILE_ERROR("Object script error - player_reset_underwater_timer expects no parameters");
}

void var_let(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_let_new(*m, EXPRESSION(p[0]));
    else
        COMPILE_ERROR("Object script error - let expects one parameter: expression");
}

void var_if(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_if_new(*m, EXPRESSION(p[0]), p[1]);
    else
        COMPILE_ERROR("Object script error - if expects two parameters: expression, new_state_name");
}

void var_unless(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 2)
        *m = objectdecorator_unless_new(*m, EXPRESSION(p[0]), p[1]);
    else
        COMPILE_ERROR("Object script error - unless expects two parameters: expression, new_state_name");
}

void var_resetglobals(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_resetglobals_new(*m);
    else
        COMPILE_ERROR("Object script error - reset_globals expects no parameters");
}

void t_textout(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 4)
        *m = objectdecorator_textout_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION("9999999"), EXPRESSION("0"), EXPRESSION("9999999"));
    else if(n == 5)
        *m = objectdecorator_textout_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION("0"), EXPRESSION("9999999"));
    else if(n == 6)
        *m = objectdecorator_textout_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION(p[5]), EXPRESSION("9999999"));
    else if(n == 7)
        *m = objectdecorator_textout_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION(p[5]), EXPRESSION(p[6]));
    else
        COMPILE_ERROR("Object script error - textout expects at least four and at most seven parameters: font_name, xpos, ypos, text [, max_width [, index_of_first_char [, length]]]");
}

void t_textout_centre(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 4)
        *m = objectdecorator_textoutcentre_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION("9999999"), EXPRESSION("0"), EXPRESSION("9999999"));
    else if(n == 5)
        *m = objectdecorator_textoutcentre_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION("0"), EXPRESSION("9999999"));
    else if(n == 6)
        *m = objectdecorator_textoutcentre_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION(p[5]), EXPRESSION("9999999"));
    else if(n == 7)
        *m = objectdecorator_textoutcentre_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION(p[5]), EXPRESSION(p[6]));
    else
        COMPILE_ERROR("Object script error - textout_centre expects at least four and at most seven parameters: font_name, xpos, ypos, text [, max_width [, index_of_first_char [, length]]]");
}

void t_textout_right(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 4)
        *m = objectdecorator_textoutright_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION("9999999"), EXPRESSION("0"), EXPRESSION("9999999"));
    else if(n == 5)
        *m = objectdecorator_textoutright_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION("0"), EXPRESSION("9999999"));
    else if(n == 6)
        *m = objectdecorator_textoutright_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION(p[5]), EXPRESSION("9999999"));
    else if(n == 7)
        *m = objectdecorator_textoutright_new(*m, p[0], EXPRESSION(p[1]), EXPRESSION(p[2]), p[3], EXPRESSION(p[4]), EXPRESSION(p[5]), EXPRESSION(p[6]));
    else
        COMPILE_ERROR("Object script error - textout_right expects at least four and at most seven parameters: font_name, xpos, ypos, text [, max_width [, index_of_first_char [, length]]]");
}

void execute(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_execute_new(*m, p[0]); /* execute <state> */
    else if(n == 3) {
        if(str_icmp(p[1], "if") == 0)
            *m = objectdecorator_executeif_new(*m, p[0], EXPRESSION(p[2])); /* execute <state> if <expr> */
        else if(str_icmp(p[1], "unless") == 0)
            *m = objectdecorator_executeunless_new(*m, p[0], EXPRESSION(p[2])); /* execute <state> unless <expr> */
        else if(str_icmp(p[1], "while") == 0)
            *m = objectdecorator_executewhile_new(*m, p[0], EXPRESSION(p[2])); /* execute <state> while <expr> */
        else
            COMPILE_ERROR("Object script error - invalid syntax for command execute (3 args)");
    }
    else if(n == 5) {
        if(str_icmp(p[1], "for") == 0)
            *m = objectdecorator_executefor_new(*m, p[0], EXPRESSION(p[2]), EXPRESSION(p[3]), EXPRESSION(p[4])); /* execute <state> for <e1> <e2> <e3> */
        else
            COMPILE_ERROR("Object script error - invalid syntax for command execute (5 args)");
    }
    else
        COMPILE_ERROR("Object script error - invalid syntax for command execute");
}

void m_launch_url(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_launchurl_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - launch_url expects one parameter: URL");
}

void push_quest(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 1)
        *m = objectdecorator_pushquest_new(*m, p[0]);
    else
        COMPILE_ERROR("Object script error - push_quest expects one parameter: path_to_qst_file");
}

void pop_quest(objectmachine_t** m, int n, const char **p, const parsetree_statement_t *stmt)
{
    if(n == 0)
        *m = objectdecorator_popquest_new(*m);
    else
        COMPILE_ERROR("Object script error - pop_quest expects no parameters");
}
