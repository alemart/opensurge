/*
 * Open Surge Engine
 * editorhelp.c - level editor help
 * Copyright (C) 2011, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "editorhelp.h"
#include "../core/font.h"
#include "../core/scene.h"
#include "../core/audio.h"
#include "../core/soundfactory.h"
#include "../core/util.h"
#include "../core/video.h"
#include "../core/input.h"


/* private data */
#define BOX_WIDTH               615
#define BOX_HEIGHT              350
#define BOX_XPOS                ((VIDEO_SCREEN_W - BOX_WIDTH) / 2)
#define BOX_YPOS                ((VIDEO_SCREEN_H - BOX_HEIGHT) / 2)
#define BOX_PADDING             15

static font_t *title, *label, *quitlabel;
static image_t *buf;
static input_t *in;

static const char text[] = 
"<color=ff8060>General</color>\n\n"
"F12                                Return to the game\n"
"Ctrl + S                           Save the level\n"
"Ctrl + R                           Reload the level\n"
"Arrow keys | WASD                  Move the camera\n"
"Shift + Arrows | Shift + WASD      Move the camera (faster)\n"
"Ctrl + Z | Ctrl + Y                Undo | Redo\n"
"F1                                 Show help\n"
"G                                  Snap to grid\n"
"1 | 2                              Open palette: brick | entity\n"
"\n\n"
"<color=ff8060>Item placement</color>\n\n"
"Left mouse button                  Put item\n"
"Middle mouse button                Pick item\n"
"Right mouse button                 Erase item (hold: eraser)\n"
"Mouse wheel                        Change the current item\n"
"Ctrl + Left mouse button           Change the spawn point\n"
"L | Shift + L                      Change brick layer\n"
"F | Shift + F                      Flip brick\n"
"Shift + Mouse wheel                Change item type (legacy)\n"
"Ctrl + Mouse wheel                 Change obj. category (legacy)\n"
"\n\n"
"<color=ff8060>Layer system summary</color>: yellow object activates yellow layer and disables green. Green object activates green layer and disables yellow. Change brick layers with L.\n"
;


/*
 * editorhelp_init()
 * Initializes the editor help screen
 */
void editorhelp_init(void *foo)
{
    title = font_create("default");
    font_set_text(title, "<color=80ff60>LEVEL EDITOR</color>");
    font_set_position(title, v2d_new(BOX_XPOS + (BOX_WIDTH - font_get_textsize(title).x) / 2, BOX_YPOS + BOX_PADDING));
    font_set_visible(title, FALSE);

    label = font_create("default");
    font_set_position(label, v2d_new(BOX_XPOS + BOX_PADDING, BOX_YPOS + BOX_PADDING));
    font_set_width(label, BOX_WIDTH - BOX_PADDING * 2);
    font_set_text(label, "%s", text);
    if(video_get_window_size().x < BOX_WIDTH) {
        font_set_position(label, v2d_new(BOX_PADDING, BOX_PADDING));
        font_set_width(label, VIDEO_SCREEN_W - BOX_PADDING * 2);
        font_set_text(label,
            "<color=ff8060>LEVEL EDITOR</color>\n\n"
            "Please increase the window size on the options screen and try again.\n\n"
            "Press <color=ff8060>ESC</color> to go back."
        );
    }

    quitlabel = font_create("default");
    font_set_text(quitlabel, "[press <color=ff8060>ESC</color>]");
    font_set_position(quitlabel, v2d_new(BOX_XPOS + BOX_WIDTH - BOX_PADDING - font_get_textsize(quitlabel).x, BOX_YPOS + BOX_PADDING));

    buf = image_create(image_width(video_get_backbuffer()), image_height(video_get_backbuffer()));
    image_clear(buf, image_rgb(18, 18, 18));
    image_draw_trans(video_get_backbuffer(), buf, 0, 0, 0.15f, IF_NONE);

    in = input_create_user("editorhelp");
    sound_play( soundfactory_get("select") );
}



/*
 * editorhelp_update()
 * Updates the editor help screen
 */
void editorhelp_update()
{
    if(input_button_pressed(in, IB_FIRE1)) {
        scenestack_pop();
        return;
    }
}



/*
 * editorhelp_render()
 * Renders the editor help screen
 */
void editorhelp_render()
{
    v2d_t v = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);
    image_blit(buf, video_get_backbuffer(), 0, 0, 0, 0, image_width(buf), image_height(buf));
    image_rectfill(video_get_backbuffer(), BOX_XPOS, BOX_YPOS, BOX_XPOS + BOX_WIDTH - 1, BOX_YPOS + BOX_HEIGHT - 1, image_rgb(40, 44, 52));
    font_render(title, v);
    font_render(label, v);
    font_render(quitlabel, v);
}



/*
 * editorhelp_release()
 * Releases the editor help screen
 */
void editorhelp_release()
{
    sound_play( soundfactory_get("return") );

    input_destroy(in);
    image_destroy(buf);
    font_destroy(quitlabel);
    font_destroy(label);
    font_destroy(title);
}
