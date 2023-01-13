/*
 * Open Surge Engine
 * info.c - engine information subscene for mobile devices
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#include "info.h"
#include "../../../core/video.h"
#include "../../../core/image.h"
#include "../../../core/color.h"
#include "../../../core/font.h"
#include "../../../core/util.h"
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


#define BACKGROUND_COLOR color_rgb(32, 32, 32)
static const v2d_t FONT_POSITION = { .x = 4, .y = 4 };
static const char FONT_NAME[] = "GoodNeighbors";





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
        "%s"
        "\n"
        "Version: %s\n"
        "Build date: %s\n"
        "SurgeScript version: %s\n"
        "Allegro version: %s\n"
#if defined(__ANDROID__)
        "Android version: %s\n"
#endif
        "\n"
        "Asset directory: %s\n"
        "User directory: %s\n"
        "\n",

        GAME_COPYRIGHT,

        GAME_VERSION_STRING,
        GAME_BUILD_DATE,
        surgescript_version_string(),
        allegro_version_string(),
#if defined(__ANDROID__)
        al_android_get_os_version(),
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
    image_rectfill(x, y, VIDEO_SCREEN_W, VIDEO_SCREEN_H, BACKGROUND_COLOR);

    /* render font */
    v2d_t center = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t camera = v2d_subtract(center, subscene_offset);
    font_render(subscene->font, camera);
}