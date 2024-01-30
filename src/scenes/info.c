/*
 * Open Surge Engine
 * info.c - engine information
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

#if defined(__ANDROID__)
#include <allegro5/allegro.h>
#include <allegro5/allegro_android.h>
#endif

#include <stdbool.h>
#include "info.h"
#include "../core/global.h"
#include "../core/audio.h"
#include "../core/asset.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/font.h"
#include "../core/input.h"
#include "../core/fadefx.h"
#include "../core/scene.h"
#include "../util/util.h"
#include "../util/v2d.h"
#include "../entities/sfx.h"
#include "../entities/mobilegamepad.h"

#define FONT_NAME           "BoxyBold"
#define BACKGROUND_COLOR    "303030" /* rgb hex */
#define FADE_COLOR          "000000"
#define FADE_TIME           0.5f

static int prev_opacity = 0;
static const int GAMEPAD_OPACITY = 20;

static bool go_back = false;
static font_t* font = NULL;
static input_t* input = NULL;
static void set_info_text(font_t* font);



/*
 * info_init()
 * Initialize scene
 */
void info_init(void* data)
{
    /* data */
    go_back = false;

    /* input */
    input = input_create_user(NULL);

    /* font */
    font = font_create(FONT_NAME);
    font_set_align(font, FONTALIGN_CENTER);
    font_set_width(font, VIDEO_SCREEN_W - 8);
    font_set_position(font, v2d_new(VIDEO_SCREEN_W / 2, 4));
    set_info_text(font);

    /* mobile gamepad */
    prev_opacity = mobilegamepad_opacity();
    mobilegamepad_set_opacity(min(prev_opacity, GAMEPAD_OPACITY));

    /* fade in */
    fadefx_in(color_hex(FADE_COLOR), FADE_TIME);
}

/*
 * info_release()
 * Release scene
 */
void info_release()
{
    mobilegamepad_set_opacity(prev_opacity);
    font_destroy(font);
    input_destroy(input);
}

/*
 * info_update()
 * Update scene
 */
void info_update()
{
    /* fade effect */
    if(fadefx_is_fading()) {
        return;
    }
    else if(go_back) {
        scenestack_pop();
        return;
    }

    /* go back */
    if(
        input_button_pressed(input, IB_FIRE1) ||
        input_button_pressed(input, IB_FIRE2) ||
        input_button_pressed(input, IB_FIRE3) ||
        input_button_pressed(input, IB_FIRE4)
    ) {
        go_back = true;
        fadefx_out(color_hex(FADE_COLOR), FADE_TIME);
        sound_play(SFX_BACK);
    }
}

/*
 * info_render()
 * Render scene
 */
void info_render()
{
    v2d_t camera_position = v2d_multiply(video_get_screen_size(), 0.5f);

    image_clear(color_hex(BACKGROUND_COLOR));
    font_render(font, camera_position);
}




/*
 * private
 */

void set_info_text(font_t* font)
{
    char path[2][1024];
    bool multiple_datadirs;

    asset_shared_datadir(path[0], sizeof(path[0]));
    asset_user_datadir(path[1], sizeof(path[1]));
    multiple_datadirs = (0 != strcmp(path[0], path[1]));

    #define SEPARATOR       "    "
    #define NOWRAP_SPACE    "<color=" BACKGROUND_COLOR ">_</color>"
    #define HIGHLIGHT_COLOR "ffee11"

    font_set_text(font,

        "%.48s\n"
        "is created with an open source game engine:\n"
        "\n"
        "%s\n"
        "%s\n"
        "\n"
        "%s\n"
        "\n"
        "<color=" HIGHLIGHT_COLOR ">Engine"             NOWRAP_SPACE "version:</color>" NOWRAP_SPACE "%s" SEPARATOR
        "<color=" HIGHLIGHT_COLOR ">SurgeScript"        NOWRAP_SPACE "version:</color>" NOWRAP_SPACE "%s" SEPARATOR
        "<color=" HIGHLIGHT_COLOR ">Allegro"            NOWRAP_SPACE "version:</color>" NOWRAP_SPACE "%s" SEPARATOR
        "<color=" HIGHLIGHT_COLOR ">Build"              NOWRAP_SPACE "date:</color>"    NOWRAP_SPACE "%s" SEPARATOR
#if defined(__ANDROID__)
        "<color=" HIGHLIGHT_COLOR ">Platform:</color>"  NOWRAP_SPACE "Android"  NOWRAP_SPACE "%s" SEPARATOR
#else
        "<color=" HIGHLIGHT_COLOR ">Platform:</color>"  NOWRAP_SPACE "%s" SEPARATOR
#endif
        "<color=" HIGHLIGHT_COLOR ">Data"               NOWRAP_SPACE "%s:</color> %s\n%s",

        opensurge_game_name(),

        GAME_TITLE,
        GAME_COPYRIGHT,
        GAME_LICENSE,

        GAME_VERSION_STRING,
        surgescript_version_string(),
        allegro_version_string(),

        GAME_BUILD_DATE,

#if defined(__ANDROID__)
        al_android_get_os_version(),
#elif defined(ALLEGRO_PLATFORM_STR)
        ALLEGRO_PLATFORM_STR,
#else
        "Undefined",
#endif

        multiple_datadirs ? "directories" : "directory",
        path[0],
        multiple_datadirs ? path[1] : ""
    );

    #undef HIGHLIGHT_COLOR
    #undef NOWRAP_SPACE
    #undef SEPARATOR
}
