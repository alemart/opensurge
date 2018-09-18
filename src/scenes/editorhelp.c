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
static font_t *title, *label, *quitlabel;
static image_t *buf;
static input_t *in;

static const char text[] = 
"<color=00bbff>General</color>\n\n"
"F12                                Leave the editor\n"
"LCtrl + F12                        Save the level\n"
"LShift + F12                       Reload the level\n"
"Arrow keys or WASD                 Move the camera\n"
"LShift + Arrows or LShift + WASD   Move the camera (faster)\n"
"L / LShift + L                     Switch brick layer\n"
"F / LShift + F                     Flip brick\n"
"G                                  Snap to grid\n"
"P                                  Open item palette\n"
"\n\n"
"<color=00bbff>Item placement</color>\n\n"
"Left mouse button                  Create an item\n"
"Middle mouse button                Pick an item\n"
"Right mouse button                 Delete an item\n"
"LCtrl + Left mouse button          Change the spawn point\n"
"LCtrl + Z                          Undo\n"
"LCtrl + Y                          Redo\n"
"Mouse wheel or B/N                 Previous/next item\n"
"LShift + wheel or LShift + B/N     Previous/next edit mode\n"
"LCtrl + wheel or LCtrl + B/N       Previous/next legacy object category\n"
"Hold Delete                        Eraser\n"
"\n\n"
"<color=00bbff>Tips</color>\n\n"
"<color=ffff00>1)</color> In order to pick up / delete a brick, you must select the brick edit mode. Same about entities;\n"
"<color=ffff00>2)</color> Hold Delete to get an eraser. The eraser will delete items that are under the mouse cursor;\n"
"<color=ffff00>3)</color> Loop system summary: yellow activates yellow layer and disables green. Green activates green layer and disables yellow. Change brick layers with L.\n"
;


/*
 * editorhelp_init()
 * Initializes the editor help screen
 */
void editorhelp_init(void *foo)
{
    title = font_create("powerfest");
    font_set_text(title, "<color=77ff00>LEVEL EDITOR</color>");
    font_set_position(title, v2d_new((VIDEO_SCREEN_W - font_get_textsize(title).x)/2, 20));

    label = font_create("default");
    font_set_position(label, v2d_new(20, 50));
    font_set_width(label, VIDEO_SCREEN_W - 40);
    font_set_text(label, "%s", text);

    quitlabel = font_create("default");
    font_set_position(quitlabel, v2d_new(20, VIDEO_SCREEN_H - 28));
    font_set_text(quitlabel, "Press <color=ffff00>ESC</color> to go back");

    buf = image_create(image_width(video_get_backbuffer()), image_height(video_get_backbuffer()));
    image_clear(buf, image_rgb(0,0,0));
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
