/*
 * Open Surge Engine
 * editorhelp.c - level editor help
 * Copyright (C) 2011, 2018, 2020  Alexandre Martins <alemartf@gmail.com>
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

#include <stdbool.h>
#include "editorhelp.h"
#include "../core/font.h"
#include "../core/scene.h"
#include "../core/audio.h"
#include "../core/util.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/input.h"
#include "../entities/sfx.h"

/* private stuff */
const int COLUMN_SPACING = -180;
const int MAX_COLUMN_WIDTH = 360;
const int MIN_PREFERRED_WINDOW_HEIGHT = 720;
const int MIN_WINDOW_HEIGHT = 480;

static font_t *columnfnt[2], *errorfnt;
static image_t *background;
static input_t *in;

static const char text[] = 
"$EDITOR_HELP_GENERAL                 [ $EDITOR_HELP_BACK ]\n"
"\n"
"F12 | ESC                            $EDITOR_HELP_CMD_RETURN\n"
"1 | 2                                $EDITOR_HELP_CMD_PALETTE\n"
"$EDITOR_INPUT_CTRL + S               $EDITOR_HELP_CMD_SAVE\n"
"$EDITOR_INPUT_CTRL + R               $EDITOR_HELP_CMD_RELOAD\n"
"$EDITOR_INPUT_ARROWS | WASD          $EDITOR_HELP_CMD_MOVE\n"
"$EDITOR_INPUT_SHIFT + $EDITOR_INPUT_ARROWS | $EDITOR_INPUT_SHIFT + WASD     $EDITOR_HELP_CMD_MOVEFASTER\n"
"$EDITOR_INPUT_CTRL + Z | $EDITOR_INPUT_CTRL + Y                             $EDITOR_HELP_CMD_UNDO | $EDITOR_HELP_CMD_REDO\n"
"F1                                   $EDITOR_HELP_CMD_HELP\n"
"G                                    $EDITOR_HELP_CMD_GRID\n"
"M                                    $EDITOR_HELP_CMD_MASKS\n"
"\n"
"$EDITOR_HELP_ITEMS\n"
"\n"
"$EDITOR_INPUT_LEFTCLICK                                $EDITOR_HELP_CMD_PUTITEM\n"
"$EDITOR_INPUT_MIDDLECLICK                              $EDITOR_HELP_CMD_PICKITEM\n"
"$EDITOR_INPUT_RIGHTCLICK                               $EDITOR_HELP_CMD_DELETEITEM\n"
"$EDITOR_INPUT_MOUSEWHEEL                               $EDITOR_HELP_CMD_CHANGEITEM\n"
"$EDITOR_INPUT_SHIFT + $EDITOR_INPUT_LEFTCLICK          $EDITOR_HELP_CMD_WATERLEVEL\n"
"$EDITOR_INPUT_CTRL + $EDITOR_INPUT_LEFTCLICK           $EDITOR_HELP_CMD_SPAWNPOINT\n"
"L | $EDITOR_INPUT_SHIFT + L                            $EDITOR_HELP_CMD_BRICKLAYER\n"
"F | $EDITOR_INPUT_SHIFT + F                            $EDITOR_HELP_CMD_FLIPBRICK\n"
"$EDITOR_INPUT_SHIFT + $EDITOR_INPUT_MOUSEWHEEL         $EDITOR_HELP_CMD_CHANGETYPE\n"
"\n"
"$EDITOR_HELP_LAYERS\n"
"\n"
"$EDITOR_HELP_LAYERSTUTORIAL"
;

static char text_column[2][sizeof(text)] = { "", "" };

static void split_columns(const char *text, char *col1, char *col2, bool save_space);
static void compute_box_dimensions(v2d_t col1size, v2d_t col2size, int *xpos, int *ypos, int *width, int *height);



/*
 * editorhelp_init()
 * Initializes the editor help screen
 */
void editorhelp_init(void *foo)
{
    v2d_t window_size = video_get_window_size();
    int box_xpos, box_ypos, box_width, box_height;

    /* setup background & input device */
    background = image_clone(video_get_backbuffer());
    in = input_create_user("editorhelp");

    /* configure the text columns */
    columnfnt[0] = font_create("EditorUI");
    columnfnt[1] = font_create("EditorUI");
    split_columns(text, text_column[0], text_column[1], (int)window_size.y < MIN_PREFERRED_WINDOW_HEIGHT);
    font_set_text(columnfnt[0], "%s", text_column[0]);
    font_set_text(columnfnt[1], "%s", text_column[1]);
    font_set_width(columnfnt[1], MAX_COLUMN_WIDTH);
    font_set_align(columnfnt[1], FONTALIGN_RIGHT);

    /* position the columns */
    compute_box_dimensions(
        font_get_textsize(columnfnt[0]), font_get_textsize(columnfnt[1]),
        &box_xpos, &box_ypos, &box_width, &box_height
    );
    font_set_position(columnfnt[0], v2d_new(
        box_xpos,
        box_ypos
    ));
    font_set_position(columnfnt[1], v2d_new(
        box_xpos + box_width,
        box_ypos
    ));

    /* setup the error font (the window is tiny) */
    errorfnt = font_create("EditorUI");
    if((int)window_size.y < MIN_WINDOW_HEIGHT) {
        const int PADDING = 8;
        font_set_position(errorfnt, v2d_new(PADDING, PADDING));
        font_set_width(errorfnt, video_get_window_size().x - PADDING * 2);
        font_set_text(errorfnt, "$EDITOR_HELP_TINYWINDOW");
        font_set_visible(columnfnt[0], false);
        font_set_visible(columnfnt[1], false);
    }
    else
        font_set_visible(errorfnt, false);

    /* a nice touch */
    sound_play(SFX_CONFIRM);
}



/*
 * editorhelp_update()
 * Updates the editor help screen
 */
void editorhelp_update()
{
    /* go back */
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
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    image_clear(color_rgb(18, 18, 18));
    image_draw_trans(background, 0, 0, 0.1f, IF_NONE);
    font_render(columnfnt[0], cam);
    font_render(columnfnt[1], cam);
    font_render(errorfnt, cam);
}



/*
 * editorhelp_release()
 * Releases the editor help screen
 */
void editorhelp_release()
{
    /* release resources */
    input_destroy(in);
    image_destroy(background);
    font_destroy(errorfnt);
    font_destroy(columnfnt[1]);
    font_destroy(columnfnt[0]);

    /* a nice touch */
    sound_play(SFX_BACK);
}


/* private stuff */

/* split the text into two columns */
void split_columns(const char *text, char *col1, char *col2, bool save_space)
{
    const int priority_sections = 2;
    const char *p = text;
    char **q = &col1;
    int k = priority_sections * 2;

    while(*p) {
        if(*p == '\n') {
            /* new line */
            q = &col1;
            *col1++ = *col2++ = *p++;
            if(save_space && *p == '\n' && !(--k))
                break; /* discard text - need to save space */
            continue;
        }
        else if(*p == ' ' && *(p+1) == ' ') {
            /* change column */
            q = &col2;
            while(*p == ' ')
                p++;
            continue;
        }

        /* copy character */
        *((*q)++) = *p++;
    }

    *col1 = *col2 = '\0';
}

/* compute a bounding box surrounding the text */
void compute_box_dimensions(v2d_t col1size, v2d_t col2size, int *xpos, int *ypos, int *width, int *height)
{
    const int COLUMN_HEIGHT[2] = { (int)(col1size.y), (int)(col2size.y) };

    *width = MAX_COLUMN_WIDTH * 2 + COLUMN_SPACING;
    *height = max(COLUMN_HEIGHT[0], COLUMN_HEIGHT[1]);
    *xpos = (VIDEO_SCREEN_W - *width) / 2;
    *ypos = (VIDEO_SCREEN_H - *height) / 2;
}