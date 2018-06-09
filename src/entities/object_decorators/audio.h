/*
 * Open Surge Engine
 * Audio.h - Audio commands
 * Copyright (C) 2010, 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _OD_AUDIO_H
#define _OD_AUDIO_H

#include "base/objectdecorator.h"
#include "../../core/nanocalc/nanocalc.h"

objectmachine_t* objectdecorator_playsample_new(objectmachine_t *decorated_machine, const char *sample_name, expression_t *vol, expression_t *pan, expression_t *freq, expression_t *loop);
objectmachine_t* objectdecorator_playmusic_new(objectmachine_t *decorated_machine, const char *music_name, expression_t *loop);
objectmachine_t* objectdecorator_playlevelmusic_new(objectmachine_t *decorated_machine);
objectmachine_t* objectdecorator_setmusicvolume_new(objectmachine_t *decorated_machine, expression_t *vol);
objectmachine_t* objectdecorator_stopsample_new(objectmachine_t *decorated_machine, const char *sample_name);

#endif

