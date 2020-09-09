/*
 * Open Surge Engine
 * editorcmd.c - level editor: commands & hotkeys
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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

#include <string.h>
#include "editorcmd.h"
#include "../../core/input.h"
#include "../../core/util.h"

/* commands */
typedef struct command_t command_t;
struct command_t {
    const char* name;
    const char* hotkey;
};
static const command_t command[] = {
    { "up", "Up" },
    { "up", "W" },
    { "right", "Right" },
    { "right", "D" },
    { "down", "Down" },
    { "down", "S" },
    { "left", "Left" },
    { "left", "A" },
    { "UP", "Shift+Up" },
    { "UP", "Shift+W" },
    { "RIGHT", "Shift+Right" },
    { "RIGHT", "Shift+D" },
    { "DOWN", "Shift+Down" },
    { "DOWN", "Shift+S" },
    { "LEFT", "Shift+Left" },
    { "LEFT", "Shift+A" },
    { "enter", "F12" },
    { "quit", "F12" },
    { "quit-alt", "ESC" },
    { "save", "Ctrl+S" },
    { "reload", "Ctrl+R" },
    { "put-item", "LeftClick" },
    { "pick-item", "MiddleClick" },
    { "delete-item", "RightClick" },
    { "next-item", "WheelUp" },
    { "previous-item", "WheelDown" },
    { "next-class", "Shift+WheelUp" },
    { "previous-class", "Shift+WheelDown" },
    { "next-category", "Ctrl+WheelUp" },
    { "previous-category", "Ctrl+WheelDown" },
    { "change-spawnpoint", "Ctrl+LeftClick" },
    { "change-waterlevel", "Shift+LeftClick" },
    { "erase-area", "HoldRightClick" },
    { "undo", "Ctrl+Z" },
    { "redo", "Ctrl+Y" },
    { "help", "F1" },
    { "snap-to-grid", "G" },
    { "open-brick-palette", "1" },
    { "open-entity-palette", "2" },
    { "flip-next", "F" },
    { "flip-previous", "Shift+F" },
    { "layer-next", "L" },
    { "layer-previous", "Shift+L" },
    { "toggle-masks", "M" }
};
static const int command_count = sizeof(command) / sizeof(command_t);
static bool hotkey_is_triggered(const editorcmd_t* cmd, const char* hotkey);

/* editorcmd_t struct */
struct editorcmd_t
{
    input_t* keyboard[2];
    input_t* mouse;
};



/* public API */

/*
 * editorcmd_create()
 * Create an editorcmd_t instance
 */
editorcmd_t* editorcmd_create()
{
    editorcmd_t* cmd = mallocx(sizeof *cmd);
    cmd->keyboard[0] = input_create_user("editorcmd1");
    cmd->keyboard[1] = input_create_user("editorcmd2");
    cmd->mouse = input_create_mouse();
    return cmd;
}

/*
 * editorcmd_destroy()
 * Destroys an existing editorcmd_t instance
 */
editorcmd_t* editorcmd_destroy(editorcmd_t* cmd)
{
    input_destroy(cmd->mouse);
    input_destroy(cmd->keyboard[1]);
    input_destroy(cmd->keyboard[0]);
    free(cmd);
    return NULL;
}

/*
 * editorcmd_is_triggered()
 * Checks if a certain command (hotkey) is triggered
 */
bool editorcmd_is_triggered(const editorcmd_t* cmd, const char* command_name)
{
    for(int i = 0; i < command_count; i++) {
        if(*command_name == *(command[i].name) && 0 == strcmp(command_name, command[i].name)) {
            if(hotkey_is_triggered(cmd, command[i].hotkey))
                return true;
        }
    }
    return false;
}


/*
 * editorcmd_mousepos()
 * Mouse position
 */
v2d_t editorcmd_mousepos(const editorcmd_t* cmd)
{
    return input_get_xy((inputmouse_t*)cmd->mouse);
}



/* private stuff */

/* checks if a hotkey is triggered */
bool hotkey_is_triggered(const editorcmd_t* cmd, const char* hotkey)
{
    const char* p;

    /* check modifier */
    if(NULL != (p = strchr(hotkey, '+'))) {
        if(0 == strncmp(hotkey, "Ctrl", 4)) {
            if(!(input_button_down(cmd->keyboard[0], IB_FIRE1) || input_button_down(cmd->keyboard[0], IB_FIRE2)))
                return false;
        }
        else if(0 == strncmp(hotkey, "Shift", 5)) {
             if(!(input_button_down(cmd->keyboard[0], IB_FIRE3) || input_button_down(cmd->keyboard[0], IB_FIRE4)))
                return false;           
        }
        hotkey = p + 1; /* okay, the modifier is checked */
    }
    else {
        if(input_button_down(cmd->keyboard[0], IB_FIRE1) || input_button_down(cmd->keyboard[0], IB_FIRE2) ||
        input_button_down(cmd->keyboard[0], IB_FIRE3) || input_button_down(cmd->keyboard[0], IB_FIRE4))
            return false;
    }

    /* check key */
    if(0 == strcmp(hotkey, "Up"))
        return input_button_down(cmd->keyboard[0], IB_UP) || input_button_down(cmd->keyboard[1], IB_UP);
    else if(0 == strcmp(hotkey, "Right"))
        return input_button_down(cmd->keyboard[0], IB_RIGHT) || input_button_down(cmd->keyboard[1], IB_RIGHT);
    else if(0 == strcmp(hotkey, "Down"))
        return input_button_down(cmd->keyboard[0], IB_DOWN) || input_button_down(cmd->keyboard[1], IB_DOWN);
    else if(0 == strcmp(hotkey, "Left"))
        return input_button_down(cmd->keyboard[0], IB_LEFT) || input_button_down(cmd->keyboard[1], IB_LEFT);
    else if(0 == strcmp(hotkey, "F1"))
        return input_button_pressed(cmd->keyboard[0], IB_FIRE7);
    else if(0 == strcmp(hotkey, "F12"))
        return input_button_pressed(cmd->keyboard[0], IB_FIRE8);
    else if(0 == strcmp(hotkey, "ESC"))
        return input_button_pressed(cmd->keyboard[0], IB_FIRE5);
    else if(0 == strcmp(hotkey, "LeftClick"))
        return input_button_pressed(cmd->mouse, IB_FIRE1);
    else if(0 == strcmp(hotkey, "RightClick"))
        return input_button_pressed(cmd->mouse, IB_FIRE2);
    else if(0 == strcmp(hotkey, "MiddleClick"))
        return input_button_pressed(cmd->mouse, IB_FIRE3);
    else if(0 == strcmp(hotkey, "WheelUp"))
        return input_button_pressed(cmd->mouse, IB_UP);
    else if(0 == strcmp(hotkey, "WheelDown"))
        return input_button_pressed(cmd->mouse, IB_DOWN);
    else if(0 == strcmp(hotkey, "HoldRightClick"))
        return input_button_down(cmd->mouse, IB_FIRE2);
    else switch(*hotkey) {
        case 'S': return input_button_pressed(cmd->keyboard[1], IB_DOWN);
        case 'R': return input_button_pressed(cmd->keyboard[0], IB_FIRE6);
        case '1': return input_button_pressed(cmd->keyboard[1], IB_FIRE1);
        case '2': return input_button_pressed(cmd->keyboard[1], IB_FIRE2);
        case 'M': return input_button_pressed(cmd->keyboard[1], IB_FIRE3);
        case 'G': return input_button_pressed(cmd->keyboard[1], IB_FIRE4);
        case 'L': return input_button_pressed(cmd->keyboard[1], IB_FIRE5);
        case 'F': return input_button_pressed(cmd->keyboard[1], IB_FIRE6);
        case 'Z': return input_button_pressed(cmd->keyboard[1], IB_FIRE7);
        case 'Y': return input_button_pressed(cmd->keyboard[1], IB_FIRE8);
    }

    /* nope */
    return false;
}