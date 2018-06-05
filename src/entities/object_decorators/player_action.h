/*
 * Open Surge Engine
 * player_action.h - Makes the player perform some actions
 * Copyright (C) 2010-2012  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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
 * enable_roll, disable_roll
 */

#ifndef _OD_PLAYERACTION_H
#define _OD_PLAYERACTION_H

#include "base/objectdecorator.h"

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

#endif

