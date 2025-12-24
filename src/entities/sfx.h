/*
 * Open Surge Engine
 * sfx.h - default sounds
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _SFX_H
#define _SFX_H

#include "../core/audio.h"

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
#define SFX_QUESTION            sound_load("samples/pause_appear.wav")
#define SFX_CONFIRM             sound_load("samples/select.wav")
#define SFX_SAVE                sound_load("samples/glasses.wav")
#define SFX_PAUSE               sound_load("samples/select_2.wav")
#define SFX_SECRET              sound_load("samples/secret.wav")
#define SFX_BOSSHIT             sound_load("samples/bosshit.wav")
#define SFX_EXPLODE             sound_load("samples/bosshit.wav")
#define SFX_BONUS               sound_load("samples/bigring.wav")
#define SFX_BUMPER              sound_load("samples/bumper.wav")
#define SFX_CHECKPOINT          sound_load("samples/checkpoint.wav")
#define SFX_SWITCH              sound_load("samples/switch.wav")
#define SFX_DOOROPEN            sound_load("samples/door1.wav")
#define SFX_DOORCLOSE           sound_load("samples/door2.wav")
#define SFX_TELEPORTER          sound_load("samples/teleporter.wav")
#define SFX_GOALSIGN            sound_load("samples/endsign.wav")
#define SFX_SPIKES              sound_load("samples/spikes.wav")
#define SFX_SPIKESIN            sound_load("samples/spikes_appearing.wav")
#define SFX_SPIKESOUT           sound_load("samples/spikes_disappearing.wav")
#define SFX_SPRING              sound_load("samples/spring.wav")
#define SFX_SHIELD              sound_load("samples/shield.wav")
#define SFX_FIRESHIELD          sound_load("samples/fireshield.wav")
#define SFX_THUNDERSHIELD       sound_load("samples/thundershield.wav")
#define SFX_WATERSHIELD         sound_load("samples/watershield.wav")
#define SFX_ACIDSHIELD          sound_load("samples/acidshield.wav")
#define SFX_WINDSHIELD          sound_load("samples/windshield.wav")

#endif