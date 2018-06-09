/*
 * Open Surge Engine
 * on_event.h - Events: if an event is true, then the state is changed
 * Copyright (C) 2010-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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
 * Edits by Dalton Sterritt (all edits released under same license):
 * on_player_invincible
 */

#ifndef _OD_ONEVENT_H
#define _OD_ONEVENT_H

#include "base/objectdecorator.h"
#include "../../core/nanocalc/nanocalc.h"

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

#endif

