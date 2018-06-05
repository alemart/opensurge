/*
 * Open Surge Engine
 * fontext.h - extensions for the font module
 * Copyright (C) 2010, 2012-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "fontext.h"
#include "font.h"
#include "lang.h"
#include "input.h"
#include "../scenes/level.h"

static const char* f_dollar() { return "$"; }
static const char* f_lowethan() { return "<"; }
static const char* f_greaterthan() { return ">"; }
static const char* f_level_name() { return level_name(); }
static const char* f_level_version() { return level_version(); }
static const char* f_level_author() { return level_author(); }
static const char* f_input_directional() { return lang_get(input_joystick_available() ? "INPUT_JOY_DIRECTIONAL" : "INPUT_KEYB_DIRECTIONAL"); }
static const char* f_input_left() { return lang_get(input_joystick_available() ? "INPUT_JOY_LEFT" : "INPUT_KEYB_LEFT"); }
static const char* f_input_right() { return lang_get(input_joystick_available() ? "INPUT_JOY_RIGHT" : "INPUT_KEYB_RIGHT"); }
static const char* f_input_up() { return lang_get(input_joystick_available() ? "INPUT_JOY_UP" : "INPUT_KEYB_UP"); }
static const char* f_input_down() { return lang_get(input_joystick_available() ? "INPUT_JOY_DOWN" : "INPUT_KEYB_DOWN"); }
static const char* f_input_fire1() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE1" : "INPUT_KEYB_FIRE1"); }
static const char* f_input_fire2() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE2" : "INPUT_KEYB_FIRE2"); }
static const char* f_input_fire3() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE3" : "INPUT_KEYB_FIRE3"); }
static const char* f_input_fire4() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE4" : "INPUT_KEYB_FIRE4"); }
static const char* f_input_fire5() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE5" : "INPUT_KEYB_FIRE5"); }
static const char* f_input_fire6() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE6" : "INPUT_KEYB_FIRE6"); }
static const char* f_input_fire7() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE7" : "INPUT_KEYB_FIRE7"); }
static const char* f_input_fire8() { return lang_get(input_joystick_available() ? "INPUT_JOY_FIRE8" : "INPUT_KEYB_FIRE8"); }
static const char* f_engine_name() { return GAME_TITLE; }
static const char* f_engine_version() { return GAME_VERSION_STRING; }
static const char* f_engine_website() { return GAME_WEBSITE; }
static const char* f_engine_year() { return GAME_YEAR; }


/*
 * fontext_register_variables()
 * Registers a lot of useful variables on the font module.
 * Call this AFTER font_init()
 */
void fontext_register_variables()
{
    font_register_variable("$", f_dollar);
    font_register_variable("$$", f_dollar);
    font_register_variable("$LT", f_lowethan);
    font_register_variable("$GT", f_greaterthan);
    font_register_variable("$LEVEL_NAME", f_level_name);
    font_register_variable("$LEVEL_VERSION", f_level_version);
    font_register_variable("$LEVEL_AUTHOR", f_level_author);
    font_register_variable("$INPUT_DIRECTIONAL", f_input_directional);
    font_register_variable("$INPUT_LEFT", f_input_left);
    font_register_variable("$INPUT_RIGHT", f_input_right);
    font_register_variable("$INPUT_UP", f_input_up);
    font_register_variable("$INPUT_DOWN", f_input_down);
    font_register_variable("$INPUT_FIRE1", f_input_fire1);
    font_register_variable("$INPUT_FIRE2", f_input_fire2);
    font_register_variable("$INPUT_FIRE3", f_input_fire3);
    font_register_variable("$INPUT_FIRE4", f_input_fire4);
    font_register_variable("$INPUT_FIRE5", f_input_fire5);
    font_register_variable("$INPUT_FIRE6", f_input_fire6);
    font_register_variable("$INPUT_FIRE7", f_input_fire7);
    font_register_variable("$INPUT_FIRE8", f_input_fire8);
    font_register_variable("$ENGINE_NAME", f_engine_name);
    font_register_variable("$ENGINE_VERSION", f_engine_version);
    font_register_variable("$ENGINE_WEBSITE", f_engine_website);
    font_register_variable("$ENGINE_YEAR", f_engine_year);
}
