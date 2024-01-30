/*
 * Open Surge Engine
 * object_decorators.h - Legacy scripting API: commands
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

#ifndef _OBJECT_DECORATORS_H
#define _OBJECT_DECORATORS_H

#include "object_machine.h"
#include "nanocalc/nanocalc.h"

/* commands */
objectmachine_t* objectdecorator_addcollectibles_new(objectmachine_t *decorated_machine, expression_t *collectibles);
objectmachine_t* objectdecorator_addlives_new(objectmachine_t *decorated_machine, expression_t *lives);
objectmachine_t* objectdecorator_addtoscore_new(objectmachine_t *decorated_machine, expression_t *score);
objectmachine_t* objectdecorator_asktoleave_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_attachtoplayer_new(objectmachine_t *decorated_machine, expression_t *offset_x, expression_t *offset_y);
objectmachine_t* objectdecorator_playsample_new(objectmachine_t *decorated_machine, const char *sample_name, expression_t *vol, expression_t *pan, expression_t *freq, expression_t *loop);
objectmachine_t* objectdecorator_playmusic_new(objectmachine_t *decorated_machine, const char *music_name, expression_t *loop);
objectmachine_t* objectdecorator_playlevelmusic_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_setmusicvolume_new(objectmachine_t *decorated_machine, expression_t *vol);
objectmachine_t* objectdecorator_stopsample_new(objectmachine_t *decorated_machine, const char *sample_name);
objectmachine_t* objectdecorator_bounceplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_bullettrajectory_new(objectmachine_t *decorated_machine, expression_t *speed_x, expression_t *speed_y); /* speed_x and speed_y are given in pixels per second */
objectmachine_t* objectdecorator_requestcamerafocus_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_dropcamerafocus_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_changeclosestobjectstate_new(objectmachine_t *decorated_machine, const char* object_name, const char *new_state_name);
objectmachine_t* objectdecorator_createchild_new(objectmachine_t *decorated_machine, const char* object_name, expression_t *offset_x, expression_t *offset_y, const char *child_name);
objectmachine_t* objectdecorator_changechildstate_new(objectmachine_t *decorated_machine, const char *child_name, const char *new_state_name);
objectmachine_t* objectdecorator_changeparentstate_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_clearlevel_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_createitem_new(objectmachine_t *decorated_machine, expression_t *item_id, expression_t *offset_x, expression_t *offset_y);
objectmachine_t* objectdecorator_destroy_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_showdialogbox_new(objectmachine_t *decorated_machine, const char *title, const char *message);
objectmachine_t* objectdecorator_hidedialogbox_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_ellipticaltrajectory_new(objectmachine_t *decorated_machine, expression_t *amplitude_x, expression_t *amplitude_y, expression_t *angularspeed_x, expression_t *angularspeed_y, expression_t *initialphase_x, expression_t *initialphase_y); /* amplitude_x, amplitude_y (in pixels); angularspeed_x, angularspeed_y  (in revolutions per second); initialphase_x, initialphase_y  (in degrees) */
objectmachine_t* objectdecorator_enemy_new(objectmachine_t *decorated_machine, expression_t *score);
objectmachine_t* objectdecorator_execute_new(objectmachine_t *decorated_machine, const char *state_name);
objectmachine_t* objectdecorator_executeif_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* condition);
objectmachine_t* objectdecorator_executeunless_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* condition);
objectmachine_t* objectdecorator_executewhile_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* condition);
objectmachine_t* objectdecorator_executefor_new(objectmachine_t *decorated_machine, const char *state_name, expression_t* initial, expression_t* condition, expression_t* iteration);
objectmachine_t* objectdecorator_gravity_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_hitplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_burnplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_shockplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_acidplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_jump_new(objectmachine_t *decorated_machine, expression_t *jump_strength);
objectmachine_t* objectdecorator_killplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_launchurl_new(objectmachine_t *decorated_machine, const char *url);
objectmachine_t* objectdecorator_loadlevel_new(objectmachine_t *decorated_machine, const char *level_path);
objectmachine_t* objectdecorator_lockcamera_new(objectmachine_t *decorated_machine, expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2);
objectmachine_t* objectdecorator_lookleft_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_lookright_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_lookatplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_lookatwalkingdirection_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_mosquitomovement_new(objectmachine_t *decorated_machine, expression_t *speed);
objectmachine_t* objectdecorator_moveplayer_new(objectmachine_t *decorated_machine, expression_t *speed_x, expression_t *speed_y); /* speed_x and speed_y are given in pixels per second */
objectmachine_t* objectdecorator_nextlevel_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_observeplayer_new(objectmachine_t *decorated_machine, const char *player_name);
objectmachine_t* objectdecorator_observecurrentplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_observeactiveplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_observeallplayers_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_pause_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_springfyplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_rollplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_enableplayerroll_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_disableplayerroll_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_strongplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_weakplayer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_playerenterwater_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_playerleavewater_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_playerbreathe_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_playerdrown_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_playerresetunderwatertimer_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_enableplayermovement_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_disableplayermovement_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_pushquest_new(objectmachine_t *decorated_machine, const char* path_to_qst_file);
objectmachine_t* objectdecorator_popquest_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_resetglobals_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_restartlevel_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_returntopreviousstate_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_savelevel_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_setabsoluteposition_new(objectmachine_t *decorated_machine, expression_t *xpos, expression_t *ypos);
objectmachine_t* objectdecorator_setalpha_new(objectmachine_t *decorated_machine, expression_t *alpha); /* 0.0f (transparent) <= alpha <= 1.0 (opaque) */
objectmachine_t* objectdecorator_setangle_new(objectmachine_t *decorated_machine, expression_t *angle);
objectmachine_t* objectdecorator_setanimation_new(objectmachine_t *decorated_machine, const char *sprite_name, expression_t *animation_id);
objectmachine_t* objectdecorator_setanimationframe_new(objectmachine_t *decorated_machine, expression_t *animation_frame);
objectmachine_t* objectdecorator_setanimationspeedfactor_new(objectmachine_t *decorated_machine, expression_t *animation_speed_factor);
objectmachine_t* objectdecorator_setobstacle_new(objectmachine_t *decorated_machine, int is_obstacle, expression_t *angle);
objectmachine_t* objectdecorator_setplayeranimation_new(objectmachine_t *decorated_machine, const char *sprite_name, expression_t *animation_id);
objectmachine_t* objectdecorator_setplayerinputmap_new(objectmachine_t *decorated_machine, const char *button_name);
objectmachine_t* objectdecorator_setplayerposition_new(objectmachine_t *decorated_machine, expression_t *xpos, expression_t *ypos);
objectmachine_t* objectdecorator_setplayerxspeed_new(objectmachine_t *decorated_machine, expression_t *speed);
objectmachine_t* objectdecorator_setplayeryspeed_new(objectmachine_t *decorated_machine, expression_t *speed);
objectmachine_t* objectdecorator_setscale_new(objectmachine_t *decorated_machine, expression_t *scale_x, expression_t *scale_y);
objectmachine_t* objectdecorator_setzindex_new(objectmachine_t *decorated_machine, expression_t *zindex);
objectmachine_t* objectdecorator_show_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_hide_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_simulatebuttondown_new(objectmachine_t *decorated_machine, const char *button_name);
objectmachine_t* objectdecorator_simulatebuttonup_new(objectmachine_t *decorated_machine, const char *button_name);
objectmachine_t* objectdecorator_switchcharacter_new(objectmachine_t *decorated_machine, const char *name, int force_switch);
objectmachine_t* objectdecorator_walk_new(objectmachine_t *decorated_machine, expression_t *speed);

/* general events */
objectmachine_t* objectdecorator_onalways_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_ontimeout_new(objectmachine_t *decorated_machine, expression_t *timeout, const char *new_state_name);
objectmachine_t* objectdecorator_oncollision_new(objectmachine_t *decorated_machine, const char *target_name, const char *new_state_name);
objectmachine_t* objectdecorator_onanimationfinished_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onrandomevent_new(objectmachine_t *decorated_machine, expression_t *probability, const char *new_state_name);
objectmachine_t* objectdecorator_onlevelcleared_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* input events */
objectmachine_t* objectdecorator_onbuttondown_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name);
objectmachine_t* objectdecorator_onbuttonpressed_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name);
objectmachine_t* objectdecorator_onbuttonup_new(objectmachine_t *decorated_machine, const char *button_name, const char *new_state_name);

/* player events */
objectmachine_t* objectdecorator_onplayercollision_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerattack_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerrectcollision_new(objectmachine_t *decorated_machine, expression_t *x1, expression_t *y1, expression_t *x2, expression_t *y2, const char *new_state_name);
objectmachine_t* objectdecorator_onobservedplayer_new(objectmachine_t *decorated_machine, const char *player_name, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerstop_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerwalk_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerrun_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerjump_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerspring_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerroll_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerpush_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayergethit_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerdeath_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerbrake_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerledge_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerdrown_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerbreathe_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerduck_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerlookup_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerwait_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerwin_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerintheair_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerunderwater_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* player events: shields */
objectmachine_t* objectdecorator_onnoshield_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onshield_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onfireshield_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onthundershield_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onwatershield_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onacidshield_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onwindshield_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* player events: others */
objectmachine_t* objectdecorator_onplayerspeedshoes_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onplayerinvincible_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* brick events */
objectmachine_t* objectdecorator_onbrickcollision_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onfloorcollision_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onceilingcollision_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onleftwallcollision_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_onrightwallcollision_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* camera events */
objectmachine_t* objectdecorator_oncamerafocus_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_oncamerafocusplayer_new(objectmachine_t *decorated_machine, const char *new_state_name);
objectmachine_t* objectdecorator_oncameralock_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* audio events */
objectmachine_t* objectdecorator_onmusicplay_new(objectmachine_t *decorated_machine, const char *new_state_name);

/* text routines */
objectmachine_t* objectdecorator_textout_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length);
objectmachine_t* objectdecorator_textoutcentre_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length);
objectmachine_t* objectdecorator_textoutright_new(objectmachine_t *decorated_machine, const char *font_name, expression_t *xpos, expression_t *ypos, const char *text, expression_t *max_width, expression_t *index_of_first_char, expression_t *length);

/* variables */
objectmachine_t* objectdecorator_let_new(objectmachine_t *decorated_machine, expression_t *expr);
objectmachine_t* objectdecorator_if_new(objectmachine_t *decorated_machine, expression_t *expr, const char *new_state_name);
objectmachine_t* objectdecorator_unless_new(objectmachine_t *decorated_machine, expression_t *expr, const char *new_state_name);

#endif