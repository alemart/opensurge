/*
 * Open Surge Engine
 * credits.c - credits scene
 * Copyright (C) 2009-2012, 2019  Alexandre Martins <alemartf@gmail.com>
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

#include <math.h>
#include "credits.h"
#include "options.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/font.h"
#include "../core/assetfs.h"
#include "../core/logfile.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/sfx.h"

/* credits text */
static const char CREDITS_TEXT[] = "\n\
$CREDITS_HEADER \n\
\n\
\n\
$CREDITS_ACTIVE \n\
\n\
Alexandre Martins \n\
$CREDITS_ALEXANDRE \n\
\n\
João Victor \n\
$CREDITS_JOAO \n\
\n\
Mateus Reis \n\
$CREDITS_MATEUS \n\
\n\
Cody Licorish \n\
$CREDITS_CODY \n\
\n\
\n\
$CREDITS_THANKS \n\
\n\
Johan Brodd \n\
$CREDITS_JOHAN \n\
\n\
Di Rodrigues \n\
$CREDITS_DI \n\
\n\
Victor Seven \n\
$CREDITS_VICTOR \n\
\n\
Colin Beard \n\
$CREDITS_COLIN \n\
\n\
Brian Zablocky \n\
$CREDITS_BRIAN \n\
\n\
Wuzzy \n\
$CREDITS_TRANSLATOR \n\
\n\
Álvaro Bazán \n\
$CREDITS_TRANSLATOR \n\
\n\
Ruben Caceres \n\
Lainz \n\
allegro.cc \n\
\n\
\n\
$CREDITS_RES_TITLE \n\
\n\
$CREDITS_RES_TEXT \n\
\n\
\n\
$CREDITS_RES_MUSICS \n\
\n\
Di Rodrigues \n\
--- musics/options.ogg \n\
--- musics/speed.ogg \n\
--- musics/invincible.ogg \n\
--- musics/gameover.ogg \n\
--- musics/waterworks.ogg \n\
--- musics/goal.ogg \n\
--- musics/drowning.ogg \n\
\n\
Johan Brodd (jobromedia) \n\
--- musics/theme.ogg (public domain) \n\
--- musics/template.ogg (public domain) \n\
--- musics/silence.ogg (public domain) \n\
--- musics/intro.ogg (public domain) \n\
--- musics/sharp.ogg (public domain) \n\
--- musics/winning.ogg \n\
--- musics/winning_plus.ogg \n\
--- musics/winning_plusplus.ogg \n\
--- musics/menu.ogg \n\
\n\
Victor Seven \n\
--- musics/sunshine.ogg \n\
\n\
Colin Beard \n\
--- musics/boss.ogg \n\
--- musics/gimacian.ogg \n\
\n\
Mateus Reis \n\
--- musics/citychill.ogg \n\
\n\
\n\
\n\
$CREDITS_RES_IMAGES \n\
\n\
João Victor \n\
--- surge.png (CC-0) \n\
--- images/neon.png \n\
--- images/animals.png \n\
--- images/title.png \n\
--- images/goal.png \n\
--- images/heroes.png \n\
--- images/heroes.png \n\
--- images/extra/big_waterfall.png \n\
--- images/extra/surge_talk.png \n\
--- images/extra/neon_talk.png \n\
--- images/extra/neon_talk_big.png \n\
--- images/extra/surge_artwork.png \n\
--- images/extra/surge_entrance.png \n\
--- images/opensurge.png \n\
--- images/wolfey.png \n\
--- images/mosquito.png \n\
\n\
Colin Beard \n\
--- images/charge.png \n\
--- images/even_more_shields.png \n\
--- images/more_shields.png \n\
--- images/pause.png \n\
--- images/eraser.png \n\
\n\
Colin Beard & Cody Licorish \n\
--- images/springfling.png \n\
\n\
Cody Licorish \n\
--- images/collectibles.png \n\
--- images/marmot-green.png \n\
--- images/marmotred.png \n\
--- images/harrier-swoop.png \n\
--- images/skaterbug.png \n\
--- images/ruler-salamander.png \n\
--- images/barrel.png \n\
--- logo.png \n\
\n\
João Victor & Alexandre Martins \n\
--- images/surge.png \n\
--- images/loading.png \n\
--- images/dialogs.png \n\
--- images/jumping_fish.png \n\
--- images/giant_wolf.png \n\
--- images/surgebg.png \n\
--- images/credits.png \n\
\n\
Alexandre Martins \n\
--- images/core.png \n\
--- images/fade_effect.png \n\
--- images/hud.png \n\
--- images/intro.png \n\
--- images/null.png \n\
--- images/special.png \n\
--- images/stars.png \n\
--- images/pixel.png \n\
--- images/impact.png \n\
--- images/dialogbox.png \n\
--- images/waterworks.png \n\
--- images/capsule.png \n\
--- images/elevators.png \n\
--- images/bridges.png \n\
--- images/sunshine.png (includes contributions by João Victor, Alex V., Carl Olsson) \n\
--- images/waterworks.png (includes contributions by João Victor, Carl Olsson) \n\
--- images/watersurface.png \n\
--- images/zipline.png \n\
--- images/bugsy.png \n\
--- images/fish.png \n\
\n\
Brian Zablocky \n\
--- images/bumper.png \n\
--- images/checkpoint_orb.png \n\
--- images/grassland_template.png \n\
--- images/item_boxes.png (battery icons by Cody Licorish) \n\
--- images/lady_bugsy.png \n\
--- images/shields.png \n\
--- images/switches.png \n\
\n\
Brian Zablocky (Celdecea) & Alex V. (squirrel) & Alexandre Martins \n\
--- images/spring_pads.png \n\
\n\
Christopher Martinus \n\
--- images/acts.png \n\
\n\
Mateus Reis \n\
--- images/surge_cool.png \n\
--- images/watersplash.png \n\
\n\
d1337r \n\
--- images/teleporter.png \n\
\n\
MTK358 \n\
--- images/goal_sign.png \n\
\n\
Ste Pickford \n\
--- fonts/default.png \n\
--- fonts/powerfest.png \n\
\n\
Kenney \n\
--- fonts/kenney.ttf (CC-0) \n\
\n\
Dalton Maag / Canonical Ltd \n\
--- fonts/Ubuntu-M.ttf (Ubuntu Font Licence 1.0) \n\
--- fonts/Ubuntu-B.ttf (Ubuntu Font Licence 1.0) \n\
\n\
HanYang I&C Co Ltd \n\
--- fonts/GothicA1-Medium.ttf (SIL Open Font License 1.1) \n\
--- fonts/GothicA1-Bold.ttf (SIL Open Font License 1.1) \n\
\n\
Clint Bellanger \n\
--- fonts/good_neighbors.png (CC-0) \n\
\n\
Robey Pointer \n\
--- fonts/tiny.png \n\
\n\
Constantin (supertuxkart-team) \n\
--- images/tux.png \n\
\n\
Carl Olsson aka surt (opengameart.org) \n\
--- images/spikes.png \n\
\n\
Daniel Cook (lostgarden.com) \n\
--- images/explosion.png \n\
\n\
qubodup (opengameart.org) \n\
--- images/cursor.png \n\
\n\
Christopher Martinus & Colin Beard \n\
--- images/dnadoors.png \n\
\n\
Colin Beard & Mateus Reis \n\
--- images/animalprison.png \n\
\n\
Alexandre Martins & Ste Pickford \n\
--- images/water.png \n\
\n\
Gaivota & Alexandre Martins \n\
--- images/rings.png \n\
\n\
treeform (opengameart.org) \n\
--- images/violet.png \n\
\n\
Shawn Hargreaves \n\
--- images/allegro.png (giftware) \n\
\n\
creative commons (creativecommons.org) \n\
--- images/creativecommons.png \n\
\n\
Alex V. \n\
--- images/crococopter.png \n\
\n\
Alexandre, Andrew Pociask, KZR \n\
--- images/extra/big_sunshine.png \n\
\n\
thekingphoenix \n\
--- images/explode.png \n\
--- images/hand.png \n\
\n\
Luis Zuno aka ansimus (pixelgameart.org) \n\
--- images/sunshine_bg.png \n\
\n\
Alexandre & Luis Zuno \n\
--- images/waterworks_bg.png (includes contributions by Mateus Reis) \n\
\n\
klausdell \n\
--- images/speed_smoke.png \n\
\n\
KnoblePersona \n\
--- images/smoke.png \n\
\n\
Master484 \n\
--- images/mini_smoke.png \n\
\n\
Antifarea (opengameart.org) & Alexandre \n\
--- images/hydra.png (CC-BY-SA 3.0) \n\
\n\
João Victor, Colin Beard, Constantin, Alexandre \n\
--- images/life_icon.png \n\
\n\
\n\
$CREDITS_RES_SAMPLES \n\
\n\
Mateus Reis \n\
--- samples/break.wav \n\
--- samples/checkpoint.wav \n\
--- samples/jump.wav \n\
--- samples/spring.wav \n\
--- samples/bosshit.wav \n\
--- samples/brake.wav \n\
--- samples/break.wav \n\
--- samples/drown.wav \n\
--- samples/collectible.wav \n\
--- samples/collectible_loss.wav \n\
--- samples/collectible_count.wav \n\
--- samples/select.wav \n\
--- samples/select_2.wav \n\
--- samples/return.wav \n\
--- samples/roll.wav \n\
--- samples/charge.wav \n\
--- samples/release.wav \n\
--- samples/choose.wav \n\
--- samples/bigring.wav \n\
--- samples/endsign.wav \n\
--- samples/glasses.wav \n\
--- samples/teleporter.wav \n\
--- samples/teleporter_backwards.wav \n\
--- samples/water_in.wav \n\
--- samples/water_out.wav \n\
--- samples/death.wav \n\
--- samples/spikes.wav \n\
--- samples/switch.wav \n\
--- samples/bigring.wav \n\
--- samples/door1.wav \n\
--- samples/door2.wav \n\
--- samples/powerup.wav \n\
--- samples/cannon.ogg \n\
--- samples/floorhit.wav \n\
--- samples/shot.wav \n\
--- samples/growlmod.wav \n\
--- samples/trotada.wav \n\
--- samples/help.wav \n\
--- samples/crococopter.wav \n\
--- samples/crococopter_swoop_up.wav \n\
--- samples/crococopter_swoop_down.wav \n\
--- samples/secret.wav \n\
--- samples/pipe_in.wav \n\
--- samples/pipe_out.wav \n\
--- samples/jetpack2.wav (modified by Alexandre) \n\
\n\
Di Rodrigues \n\
--- samples/1up.ogg \n\
--- samples/push_stone.ogg \n\
--- samples/creativecommons.ogg \n\
\n\
Colin Beard \n\
--- samples/shield.wav \n\
--- samples/acidshield.wav \n\
--- samples/fireshield.wav \n\
--- samples/thundershield.wav \n\
--- samples/watershield.wav \n\
--- samples/windshield.wav \n\
--- samples/bubbleget.wav \n\
--- samples/bumper.wav \n\
--- samples/cash.wav \n\
\n\
Alexandre Martins \n\
--- samples/spikes_appearing.wav \n\
--- samples/spikes_disappearing.wav \n\
--- samples/destroy.wav \n\
--- samples/damaged.wav \n\
--- samples/talk.wav \n\
--- samples/zipline.wav \n\
--- samples/zipline2.wav \n\
--- samples/impact.wav \n\
--- samples/slide.wav \n\
--- samples/jetpack.wav \n\
--- samples/underwater_tick.wav \n\
--- samples/lighting_boom.wav \n\
\n\
d1337r \n\
--- samples/deny.wav \n\
\n\
CGEffex (freesound.org) \n\
--- samples/roar.ogg \n\
\n\
Johan Brodd \n\
--- samples/allegro.ogg (public domain) \n\
--- samples/creativecommons2.ogg (public domain) \n\
\n\
InspectorJ (www.jshaw.co.uk) of freesound.org \n\
--- samples/waterlevel.wav \n\
\n\
mas123456 (freesound.org) \n\
--- samples/waterfall.wav (CC-0) \n\
\n\
LG (freesound.org) \n\
--- samples/charging_up.wav \n\
\n\
Eddy Bergman (youtube.com/ededitz, freesound.org) \n\
--- samples/electric_bulb.wav \n\
\n\
Selector (freesound.org) \n\
--- samples/discharge.wav (CC-0) \n\
\n\
little_dog4000 (freesound.org) \n\
--- samples/teleport_appear.wav (CC-0 - sound edited by Alexandre) \n\
--- samples/teleport_disappear.wav (CC-0 - sound edited by Alexandre) \n\
\n\
\n\
$CREDITS_RES_SCRIPTS \n\
\n\
$CREDITS_RES_SCRIPTS_TEXT \n\
";


/* private data */
static const char* CREDITS_BGFILE = "themes/scenes/credits.bg";
static const int scroll_speed = 24;
static int quit;
static font_t *title, *text, *back;
static input_t *input;
static bgtheme_t *bgtheme;
static music_t *music;
static scene_t *next_scene;




/* public functions */

/*
 * credits_init()
 * Initializes the scene
 */
void credits_init(void *foo)
{
    quit = FALSE;
    next_scene = NULL;
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);

    title = font_create("menu.title");
    font_set_text(title, "%s", lang_get("CREDITS_TITLE"));
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 5));
    font_set_align(title, FONTALIGN_CENTER);

    back = font_create("menu.text");
    font_set_text(back, "%s", lang_get("CREDITS_BACK"));
    font_set_position(back, v2d_new(10, VIDEO_SCREEN_H - font_get_textsize(back).y - 5));

    text = font_create("menu.text");
    font_set_text(text, "%s", CREDITS_TEXT);
    font_set_width(text, VIDEO_SCREEN_W - 20);
    font_set_position(text, v2d_new(10, VIDEO_SCREEN_H));

    bgtheme = background_load(CREDITS_BGFILE);

    fadefx_in(color_rgb(0,0,0), 1.0);
}


/*
 * credits_release()
 * Releases the scene
 */
void credits_release()
{
    bgtheme = background_unload(bgtheme);

    font_destroy(title);
    font_destroy(text);
    font_destroy(back);

    input_destroy(input);
    music_unref(music);
}


/*
 * credits_update()
 * Updates the scene
 */
void credits_update()
{
    float dt = timer_get_delta();
    v2d_t textpos;

    /* background movement */
    background_update(bgtheme);

    /* text movement */
    textpos = font_get_position(text);
    textpos.y -= scroll_speed * dt;
    if(textpos.y < -font_get_textsize(text).y)
        textpos.y = VIDEO_SCREEN_H;
    font_set_position(text, textpos);

    /* quit */
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play(SFX_BACK);
            next_scene = NULL;
            quit = TRUE;
        }
    }

    /* music */
    if(!music_is_playing())
        music_play(music, true);

    /* fade-out */
    if(quit) {
        if(fadefx_is_over()) {
            scenestack_pop();
            if(next_scene != NULL)
                scenestack_push(next_scene, NULL);
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0);
    }
}



/*
 * credits_render()
 * Renders the scene
 */
void credits_render()
{
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    font_render(text, cam);
    background_render_fg(bgtheme, cam);
    font_render(title, cam);
    font_render(back, cam);
}
