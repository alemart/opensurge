/*
 * Open Surge Engine
 * info.c - engine information subscene for mobile devices
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#include <stdio.h>
#include "info.h"
#include "../../../core/video.h"
#include "../../../core/image.h"
#include "../../../core/color.h"
#include "../../../core/font.h"
#include "../../../core/util.h"
#include "../../../core/stringutil.h"
#include "../../../core/asset.h"
#include "../../../core/global.h"

#if defined(__ANDROID__)
#include <allegro5/allegro_android.h>
#endif

typedef struct mobile_subscene_info_t mobile_subscene_info_t;
struct mobile_subscene_info_t {
    mobile_subscene_t super;
    font_t* font;
};

static void init(mobile_subscene_t*);
static void release(mobile_subscene_t*);
static void update(mobile_subscene_t*,v2d_t);
static void render(mobile_subscene_t*,v2d_t);
static const mobile_subscene_t super = { .init = init, .release = release, .update = update, .render = render };

#define BACKGROUND_COLOR "303030" /* RGB hex code */
static const v2d_t FONT_POSITION = { .x = 4, .y = 4 };
static const char FONT_NAME[] = "BoxyBold";

static const char* opensurge_game_name();
static const char* opensurge_game_author();



/*
 * mobile_subscene_info()
 * Returns a new instance of the info subscene
 */
mobile_subscene_t* mobile_subscene_info()
{
    mobile_subscene_info_t* subscene = mallocx(sizeof *subscene);

    subscene->super = super;
    subscene->font = NULL;
    
    return (mobile_subscene_t*)subscene;
}






/*
 * init()
 * Initializes the subscene
 */
void init(mobile_subscene_t* subscene_ptr)
{
    mobile_subscene_info_t* subscene = (mobile_subscene_info_t*)subscene_ptr;
    char path[2][1024];

    /* create font */
    font_t* font = font_create(FONT_NAME);
    font_set_position(font, FONT_POSITION);
    font_set_align(font, FONTALIGN_LEFT);
    font_set_width(font, VIDEO_SCREEN_W - 2 * FONT_POSITION.x);
    font_set_text(font,

        #define SEPARATOR    "     "
        #define NOWRAP_SPACE "<color=" BACKGROUND_COLOR ">_</color>"

        "%s\n"
        "Created by %s\n"
        "\n"
        "Powered by %s\n"
        "%s\n"
        "\n"
        "This program is free software; you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation; either version 3 of the License, or "
        "(at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License "
        "along with this program.  If not, see < http://www.gnu.org/licenses/ >.\n"
        "\n"
        "Engine"        NOWRAP_SPACE "version:" NOWRAP_SPACE "%s" SEPARATOR
        "Build"         NOWRAP_SPACE "date:"    NOWRAP_SPACE "%s" SEPARATOR
        "SurgeScript"   NOWRAP_SPACE "version:" NOWRAP_SPACE "%s" SEPARATOR
        "Allegro"       NOWRAP_SPACE "version:" NOWRAP_SPACE "%s" SEPARATOR
#if defined(__ANDROID__)
        "Platform:"     NOWRAP_SPACE "Android"  NOWRAP_SPACE "%s" SEPARATOR
#else
        "Platform:"     NOWRAP_SPACE "%s" SEPARATOR
#endif
        "Data" NOWRAP_SPACE "folders: %s %s",

        opensurge_game_name(),
        opensurge_game_author(),

        GAME_TITLE,
        GAME_COPYRIGHT,

        GAME_VERSION_STRING,
        GAME_BUILD_DATE,

        surgescript_version_string(),
        allegro_version_string(),
#if defined(__ANDROID__)
        al_android_get_os_version(),
#elif defined(ALLEGRO_PLATFORM_STR)
        ALLEGRO_PLATFORM_STR,
#else
        "Undefined",
#endif

        asset_shared_datadir(path[0], sizeof(path[0])),
        asset_user_datadir(path[1], sizeof(path[1]))
    );
    subscene->font = font;
}

/*
 * release()
 * Releases the subscene
 */
void release(mobile_subscene_t* subscene_ptr)
{
    mobile_subscene_info_t* subscene = (mobile_subscene_info_t*)subscene_ptr;

    font_destroy(subscene->font);

    free(subscene);
}

/*
 * update()
 * Updates the subscene
 */
void update(mobile_subscene_t* subscene_ptr, v2d_t subscene_offset)
{
    (void)subscene_ptr;
    (void)subscene_offset;
}

/*
 * render()
 * Renders the subscene
 */
void render(mobile_subscene_t* subscene_ptr, v2d_t subscene_offset)
{
    mobile_subscene_info_t* subscene = (mobile_subscene_info_t*)subscene_ptr;

    /* render background */
    int x = subscene_offset.x;
    int y = subscene_offset.y;
    image_rectfill(x, y, VIDEO_SCREEN_W, VIDEO_SCREEN_H, color_hex(BACKGROUND_COLOR));

    /* render font */
    v2d_t center = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t camera = v2d_subtract(center, subscene_offset);
    font_render(subscene->font, camera);
}



/* --- private --- */

/* the sanitized name of the game / MOD that is being run in the engine */
const char* opensurge_game_name()
{
    /* don't use a large buffer because wordwrap is enabled */
    static char buffer[64];

    /* FIXME: is the title of the window always equal to the name of the game? */
    str_cpy(buffer, video_get_window_title(), sizeof(buffer));

    /* get rid of newlines */
    for(char* p = buffer; *p; p++) {
        if(*p == '\n' || *p == '\r')
            *p = ' ';
    }

    /* done! */
    return buffer;
}

/* the author(s) of the game / MOD that is being run in the engine */
const char* opensurge_game_author()
{
    /* don't use a large buffer because wordwrap is enabled */
    static char buffer[64];

    /* FIXME: this is a stub */
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(buffer, sizeof(buffer), "%s authors", opensurge_game_name());
    #pragma GCC diagnostic pop

    /* get rid of newlines */
    for(char* p = buffer; *p; p++) {
        if(*p == '\n' || *p == '\r')
            *p = ' ';
    }

    /* done! */
    return buffer;
}