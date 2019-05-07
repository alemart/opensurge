/*
 * Open Surge Engine
 * soundfactory.h - sound factory
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _SOUNDFACTORY_H
#define _SOUNDFACTORY_H

#include "audio.h"

/* initializes the sound factory */
void soundfactory_init();

/* releases the sound factory */
void soundfactory_release();

/* given a sound name, returns the corresponding sound effect */
sound_t *soundfactory_get(const char *sound_name);

#define SFX_JUMP                sound_load("samples/jump.wav")
#define SFX_BRAKE               sound_load("samples/brake.wav")
#define SFX_DEATH               sound_load("samples/death.wav")
#define SFX_DAMAGE              sound_load("samples/damaged.wav")
#define SFX_GETHIT              sound_load("samples/collectible_loss.wav")
#define SFX_DROWN               sound_load("samples/drown.wav")
#define SFX_BREATHE             sound_load("samples/bubbleget.wav")
#define SFX_CHARGE              sound_load("samples/charge.wav")
#define SFX_RELEASE             sound_load("samples/release.wav")
#define SFX_ROLL                sound_load("samples/roll.wav")
#define SFX_WATERIN             sound_load("samples/water_in.wav")
#define SFX_WATEROUT            sound_load("samples/water_out.wav")
#define SFX_COLLECTIBLE         sound_load("samples/collectible.wav")
#define SFX_1UP                 sound_load("samples/1up.ogg")
#define SFX_DESTROY             sound_load("samples/destroy.wav")
#define SFX_BREAK               sound_load("samples/break.wav")
#define SFX_CHOOSE              sound_load("samples/choose.wav")
#define SFX_DENY                sound_load("samples/deny.wav")
#define SFX_BACK                sound_load("samples/return.wav")
#define SFX_CONFIRM             sound_load("samples/select.wav")
#define SFX_SAVE                sound_load("samples/glasses.wav")
#define SFX_PAUSE               sound_load("samples/select_2.wav")
#define SFX_SECRET              sound_load("samples/secret.wav")

#endif