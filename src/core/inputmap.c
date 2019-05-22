/*
 * Open Surge Engine
 * inputmap.c - custom input mapping
 * Copyright (C) 2011, 2019  Alexandre Martins <alemartf@gmail.com>
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

#if defined(A5BUILD)
#include <allegro5/allegro.h>
#else
#include <allegro.h>
#endif

#include "inputmap.h"
#include "stringutil.h"
#include "util.h"
#include "logfile.h"
#include "resourcemanager.h"
#include "assetfs.h"
#include "hashtable.h"
#include "nanoparser/nanoparser.h"

/* storage */
typedef struct inputmapnode_t inputmapnode_t;
struct inputmapnode_t {
    inputmap_t *data;
};
static inputmapnode_t* inputmapnode_create(const char* name);
static void inputmapnode_destroy(inputmapnode_t *x);
HASHTABLE_GENERATE_CODE(inputmapnode_t, inputmapnode_destroy);
static HASHTABLE(inputmapnode_t, mappings);

/* file reader stuff (nanoparser) */
static void load_inputmap_table();
static int traverse(const parsetree_statement_t *stmt);
static int traverse_inputmap(const parsetree_statement_t *stmt, void *inputmapnode);
static int traverse_inputmap_keyboard(const parsetree_statement_t *stmt, void *inputmapnode);
static int traverse_inputmap_joystick(const parsetree_statement_t *stmt, void *inputmapnode);

/* misc */
static const char* INPUTMAP_FILE = "config/input.def";
static const char* NULL_INPUTMAP = "null";
static int keycode_of(const char* key_name);
static int joybtncode_of(const char* joybtn_name);

#if defined(A5BUILD)

/* listing all keys */
#define FOREACH_KEY(K) \
    /* empty entry */ \
    K(0, "KEY_NONE") \
    \
    /* Valid keys */ \
    K(ALLEGRO_KEY_A, "KEY_A") \
    K(ALLEGRO_KEY_B, "KEY_B") \
    K(ALLEGRO_KEY_C, "KEY_C") \
    K(ALLEGRO_KEY_D, "KEY_D") \
    K(ALLEGRO_KEY_E, "KEY_E") \
    K(ALLEGRO_KEY_F, "KEY_F") \
    K(ALLEGRO_KEY_G, "KEY_G") \
    K(ALLEGRO_KEY_H, "KEY_H") \
    K(ALLEGRO_KEY_I, "KEY_I") \
    K(ALLEGRO_KEY_J, "KEY_J") \
    K(ALLEGRO_KEY_K, "KEY_K") \
    K(ALLEGRO_KEY_L, "KEY_L") \
    K(ALLEGRO_KEY_M, "KEY_M") \
    K(ALLEGRO_KEY_N, "KEY_N") \
    K(ALLEGRO_KEY_O, "KEY_O") \
    K(ALLEGRO_KEY_P, "KEY_P") \
    K(ALLEGRO_KEY_Q, "KEY_Q") \
    K(ALLEGRO_KEY_R, "KEY_R") \
    K(ALLEGRO_KEY_S, "KEY_S") \
    K(ALLEGRO_KEY_T, "KEY_T") \
    K(ALLEGRO_KEY_U, "KEY_U") \
    K(ALLEGRO_KEY_V, "KEY_V") \
    K(ALLEGRO_KEY_W, "KEY_W") \
    K(ALLEGRO_KEY_X, "KEY_X") \
    K(ALLEGRO_KEY_Y, "KEY_Y") \
    K(ALLEGRO_KEY_Z, "KEY_Z") \
    K(ALLEGRO_KEY_0, "KEY_0") \
    K(ALLEGRO_KEY_1, "KEY_1") \
    K(ALLEGRO_KEY_2, "KEY_2") \
    K(ALLEGRO_KEY_3, "KEY_3") \
    K(ALLEGRO_KEY_4, "KEY_4") \
    K(ALLEGRO_KEY_5, "KEY_5") \
    K(ALLEGRO_KEY_6, "KEY_6") \
    K(ALLEGRO_KEY_7, "KEY_7") \
    K(ALLEGRO_KEY_8, "KEY_8") \
    K(ALLEGRO_KEY_9, "KEY_9") \
    K(ALLEGRO_KEY_PAD_0, "KEY_PAD_0", "KEY_0_PAD") \
    K(ALLEGRO_KEY_PAD_1, "KEY_PAD_1", "KEY_1_PAD") \
    K(ALLEGRO_KEY_PAD_2, "KEY_PAD_2", "KEY_2_PAD") \
    K(ALLEGRO_KEY_PAD_3, "KEY_PAD_3", "KEY_3_PAD") \
    K(ALLEGRO_KEY_PAD_4, "KEY_PAD_4", "KEY_4_PAD") \
    K(ALLEGRO_KEY_PAD_5, "KEY_PAD_5", "KEY_5_PAD") \
    K(ALLEGRO_KEY_PAD_6, "KEY_PAD_6", "KEY_6_PAD") \
    K(ALLEGRO_KEY_PAD_7, "KEY_PAD_7", "KEY_7_PAD") \
    K(ALLEGRO_KEY_PAD_8, "KEY_PAD_8", "KEY_8_PAD") \
    K(ALLEGRO_KEY_PAD_9, "KEY_PAD_9", "KEY_9_PAD") \
    K(ALLEGRO_KEY_F1, "KEY_F1") \
    K(ALLEGRO_KEY_F2, "KEY_F2") \
    K(ALLEGRO_KEY_F3, "KEY_F3") \
    K(ALLEGRO_KEY_F4, "KEY_F4") \
    K(ALLEGRO_KEY_F5, "KEY_F5") \
    K(ALLEGRO_KEY_F6, "KEY_F6") \
    K(ALLEGRO_KEY_F7, "KEY_F7") \
    K(ALLEGRO_KEY_F8, "KEY_F8") \
    K(ALLEGRO_KEY_F9, "KEY_F9") \
    K(ALLEGRO_KEY_F10, "KEY_F10") \
    K(ALLEGRO_KEY_F11, "KEY_F11") \
    K(ALLEGRO_KEY_F12, "KEY_F12") \
    K(ALLEGRO_KEY_ESCAPE, "KEY_ESCAPE", "KEY_ESC") \
    K(ALLEGRO_KEY_TILDE, "KEY_TILDE") \
    K(ALLEGRO_KEY_MINUS, "KEY_MINUS") \
    K(ALLEGRO_KEY_EQUALS, "KEY_EQUALS") \
    K(ALLEGRO_KEY_BACKSPACE, "KEY_BACKSPACE") \
    K(ALLEGRO_KEY_TAB, "KEY_TAB") \
    K(ALLEGRO_KEY_OPENBRACE, "KEY_OPENBRACE") \
    K(ALLEGRO_KEY_CLOSEBRACE, "KEY_CLOSEBRACE") \
    K(ALLEGRO_KEY_ENTER, "KEY_ENTER") \
    K(ALLEGRO_KEY_SEMICOLON, "KEY_SEMICOLON") \
    K(ALLEGRO_KEY_QUOTE, "KEY_QUOTE") \
    K(ALLEGRO_KEY_BACKSLASH, "KEY_BACKSLASH") \
    K(ALLEGRO_KEY_BACKSLASH2, "KEY_BACKSLASH2") \
    K(ALLEGRO_KEY_COMMA, "KEY_COMMA") \
    K(ALLEGRO_KEY_FULLSTOP, "KEY_FULLSTOP") \
    K(ALLEGRO_KEY_SLASH, "KEY_SLASH") \
    K(ALLEGRO_KEY_SPACE, "KEY_SPACE") \
    K(ALLEGRO_KEY_INSERT, "KEY_INSERT") \
    K(ALLEGRO_KEY_DELETE, "KEY_DELETE", "KEY_DEL") \
    K(ALLEGRO_KEY_HOME, "KEY_HOME") \
    K(ALLEGRO_KEY_END, "KEY_END") \
    K(ALLEGRO_KEY_PGUP, "KEY_PGUP", "KEY_PAGEUP") \
    K(ALLEGRO_KEY_PGDN, "KEY_PGDN", "KEY_PAGEDOWN") \
    K(ALLEGRO_KEY_LEFT, "KEY_LEFT") \
    K(ALLEGRO_KEY_RIGHT, "KEY_RIGHT") \
    K(ALLEGRO_KEY_UP, "KEY_UP") \
    K(ALLEGRO_KEY_DOWN, "KEY_DOWN") \
    K(ALLEGRO_KEY_PAD_SLASH, "KEY_PAD_SLASH", "KEY_SLASH_PAD") \
    K(ALLEGRO_KEY_PAD_ASTERISK, "KEY_PAD_ASTERISK", "KEY_ASTERISK_PAD") \
    K(ALLEGRO_KEY_PAD_MINUS, "KEY_PAD_MINUS", "KEY_MINUS_PAD") \
    K(ALLEGRO_KEY_PAD_PLUS, "KEY_PAD_PLUS", "KEY_PLUS_PAD") \
    K(ALLEGRO_KEY_PAD_DELETE, "KEY_PAD_DELETE", "KEY_PAD_DEL", "KEY_DEL_PAD") \
    K(ALLEGRO_KEY_PAD_ENTER, "KEY_PAD_ENTER", "KEY_ENTER_PAD") \
    K(ALLEGRO_KEY_PRINTSCREEN, "KEY_PRINTSCREEN", "KEY_PRTSCR") \
    K(ALLEGRO_KEY_PAUSE, "KEY_PAUSE") \
    K(ALLEGRO_KEY_ABNT_C1, "KEY_ABNT_C1") \
    K(ALLEGRO_KEY_YEN, "KEY_YEN") \
    K(ALLEGRO_KEY_KANA, "KEY_KANA") \
    K(ALLEGRO_KEY_CONVERT, "KEY_CONVERT") \
    K(ALLEGRO_KEY_NOCONVERT, "KEY_NOCONVERT") \
    K(ALLEGRO_KEY_AT, "KEY_AT") \
    K(ALLEGRO_KEY_CIRCUMFLEX, "KEY_CIRCUMFLEX") \
    K(ALLEGRO_KEY_COLON2, "KEY_COLON2") \
    K(ALLEGRO_KEY_KANJI, "KEY_KANJI") \
    K(ALLEGRO_KEY_LSHIFT, "KEY_LSHIFT") \
    K(ALLEGRO_KEY_RSHIFT, "KEY_RSHIFT") \
    K(ALLEGRO_KEY_LCTRL, "KEY_LCTRL", "KEY_LCONTROL") \
    K(ALLEGRO_KEY_RCTRL, "KEY_RCTRL", "KEY_RCONTROL") \
    K(ALLEGRO_KEY_ALT, "KEY_ALT") \
    K(ALLEGRO_KEY_ALTGR, "KEY_ALTGR") \
    K(ALLEGRO_KEY_LWIN, "KEY_LWIN") \
    K(ALLEGRO_KEY_RWIN, "KEY_RWIN") \
    K(ALLEGRO_KEY_MENU, "KEY_MENU") \
    K(ALLEGRO_KEY_SCROLLLOCK, "KEY_SCROLLLOCK", "KEY_SCRLOCK") \
    K(ALLEGRO_KEY_NUMLOCK, "KEY_NUMLOCK") \
    K(ALLEGRO_KEY_CAPSLOCK, "KEY_CAPSLOCK") \
    K(ALLEGRO_KEY_PAD_EQUALS, "KEY_PAD_EQUALS", "KEY_EQUALS_PAD") \
    K(ALLEGRO_KEY_BACKQUOTE, "KEY_BACKQUOTE") \
    K(ALLEGRO_KEY_SEMICOLON2, "KEY_SEMICOLON2") \
    K(ALLEGRO_KEY_COMMAND, "KEY_COMMAND") \
    \
    /* Mobile */ \
    K(ALLEGRO_KEY_BACK, "KEY_BACK") \
    K(ALLEGRO_KEY_VOLUME_UP, "KEY_VOLUME_UP") \
    K(ALLEGRO_KEY_VOLUME_DOWN, "KEY_VOLUME_DOWN") \
    \
    /* Android game keys */ \
    K(ALLEGRO_KEY_SEARCH, "KEY_SEARCH") \
    K(ALLEGRO_KEY_DPAD_CENTER, "KEY_DPAD_CENTER") \
    K(ALLEGRO_KEY_BUTTON_X, "KEY_BUTTON_X") \
    K(ALLEGRO_KEY_BUTTON_Y, "KEY_BUTTON_Y") \
    K(ALLEGRO_KEY_DPAD_UP, "KEY_DPAD_UP") \
    K(ALLEGRO_KEY_DPAD_DOWN, "KEY_DPAD_DOWN") \
    K(ALLEGRO_KEY_DPAD_LEFT, "KEY_DPAD_LEFT") \
    K(ALLEGRO_KEY_DPAD_RIGHT, "KEY_DPAD_RIGHT") \
    K(ALLEGRO_KEY_SELECT, "KEY_SELECT") \
    K(ALLEGRO_KEY_START, "KEY_START") \
    K(ALLEGRO_KEY_BUTTON_L1, "KEY_BUTTON_L1") \
    K(ALLEGRO_KEY_BUTTON_R1, "KEY_BUTTON_R1") \
    K(ALLEGRO_KEY_BUTTON_L2, "KEY_BUTTON_L2") \
    K(ALLEGRO_KEY_BUTTON_R2, "KEY_BUTTON_R2") \
    K(ALLEGRO_KEY_BUTTON_A, "KEY_BUTTON_A") \
    K(ALLEGRO_KEY_BUTTON_B, "KEY_BUTTON_B") \
    K(ALLEGRO_KEY_THUMBL, "KEY_THUMBL") \
    K(ALLEGRO_KEY_THUMBR, "KEY_THUMBR") \
    \
    /* end of list */ \
    K(0, NULL)

/* helpers */
#define __SELECT(_0, _1, _2, _3, X, ...) X
#define EXPAND_KEY_NAME_3(k, a, b, c) a, b, c,
#define EXPAND_KEY_NAME_2(k, a, b) a, b,
#define EXPAND_KEY_NAME_1(k, a) a,
#define EXPAND_KEY_CODE_3(k, a, b, c) k, k, k,
#define EXPAND_KEY_CODE_2(k, a, b) k, k,
#define EXPAND_KEY_CODE_1(k, a) k,
#define EXPAND_KEY_NAME(...) __SELECT(__VA_ARGS__, EXPAND_KEY_NAME_3, EXPAND_KEY_NAME_2, EXPAND_KEY_NAME_1)(__VA_ARGS__)
#define EXPAND_KEY_CODE(...) __SELECT(__VA_ARGS__, EXPAND_KEY_CODE_3, EXPAND_KEY_CODE_2, EXPAND_KEY_CODE_1)(__VA_ARGS__)

/* key data */
static const int key_codes[] = { FOREACH_KEY(EXPAND_KEY_CODE) };
static const char* key_names[] = { FOREACH_KEY(EXPAND_KEY_NAME) };

#else
/* key names */
static const char* key_names[] = {
    "KEY_NONE",

    "KEY_A",
    "KEY_B",
    "KEY_C",
    "KEY_D",
    "KEY_E",
    "KEY_F",
    "KEY_G",
    "KEY_H",
    "KEY_I",
    "KEY_J",
    "KEY_K",
    "KEY_L",
    "KEY_M",
    "KEY_N",
    "KEY_O",
    "KEY_P",
    "KEY_Q",
    "KEY_R",
    "KEY_S",
    "KEY_T",
    "KEY_U",
    "KEY_V",
    "KEY_W",
    "KEY_X",
    "KEY_Y",
    "KEY_Z",
    "KEY_0",
    "KEY_1",
    "KEY_2",
    "KEY_3",
    "KEY_4",
    "KEY_5",
    "KEY_6",
    "KEY_7",
    "KEY_8",
    "KEY_9",
    "KEY_0_PAD",
    "KEY_1_PAD",
    "KEY_2_PAD",
    "KEY_3_PAD",
    "KEY_4_PAD",
    "KEY_5_PAD",
    "KEY_6_PAD",
    "KEY_7_PAD",
    "KEY_8_PAD",
    "KEY_9_PAD",
    "KEY_F1",
    "KEY_F2",
    "KEY_F3",
    "KEY_F4",
    "KEY_F5",
    "KEY_F6",
    "KEY_F7",
    "KEY_F8",
    "KEY_F9",
    "KEY_F10",
    "KEY_F11",
    "KEY_F12",
    "KEY_ESC",
    "KEY_TILDE",
    "KEY_MINUS",
    "KEY_EQUALS",
    "KEY_BACKSPACE",
    "KEY_TAB",
    "KEY_OPENBRACE",
    "KEY_CLOSEBRACE",
    "KEY_ENTER",
    "KEY_COLON",
    "KEY_QUOTE",
    "KEY_BACKSLASH",
    "KEY_BACKSLASH2",
    "KEY_COMMA",
    "KEY_STOP",
    "KEY_SLASH",
    "KEY_SPACE",
    "KEY_INSERT",
    "KEY_DEL",
    "KEY_HOME",
    "KEY_END",
    "KEY_PGUP",
    "KEY_PGDN",
    "KEY_LEFT",
    "KEY_RIGHT",
    "KEY_UP",
    "KEY_DOWN",
    "KEY_SLASH_PAD",
    "KEY_ASTERISK",
    "KEY_MINUS_PAD",
    "KEY_PLUS_PAD",
    "KEY_DEL_PAD",
    "KEY_ENTER_PAD",
    "KEY_PRTSCR",
    "KEY_PAUSE",
    "KEY_ABNT_C1",
    "KEY_YEN",
    "KEY_KANA",
    "KEY_CONVERT",
    "KEY_NOCONVERT",
    "KEY_AT",
    "KEY_CIRCUMFLEX",
    "KEY_COLON2",
    "KEY_KANJI",
    "KEY_EQUALS_PAD",
    "KEY_BACKQUOTE",
    "KEY_SEMICOLON",
    "KEY_COMMAND",
/*
    "KEY_UNKNOWN1",
    "KEY_UNKNOWN2",
    "KEY_UNKNOWN3",
    "KEY_UNKNOWN4",
    "KEY_UNKNOWN5",
    "KEY_UNKNOWN6",
    "KEY_UNKNOWN7",
    "KEY_UNKNOWN8",

    "KEY_MODIFIERS",
*/
    "KEY_LSHIFT",
    "KEY_RSHIFT",
    "KEY_LCONTROL",
    "KEY_RCONTROL",
    "KEY_ALT",
    "KEY_ALTGR",
    "KEY_LWIN",
    "KEY_RWIN",
    "KEY_MENU",
    "KEY_SCRLOCK",
    "KEY_NUMLOCK",
    "KEY_CAPSLOCK",

    NULL /* end of list */
};

/* scancodes */
static const int key_codes[] = {
    0,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0_PAD,
    KEY_1_PAD,
    KEY_2_PAD,
    KEY_3_PAD,
    KEY_4_PAD,
    KEY_5_PAD,
    KEY_6_PAD,
    KEY_7_PAD,
    KEY_8_PAD,
    KEY_9_PAD,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_ESC,
    KEY_TILDE,
    KEY_MINUS,
    KEY_EQUALS,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_OPENBRACE,
    KEY_CLOSEBRACE,
    KEY_ENTER,
    KEY_COLON,
    KEY_QUOTE,
    KEY_BACKSLASH,
    KEY_BACKSLASH2,
    KEY_COMMA,
    KEY_STOP,
    KEY_SLASH,
    KEY_SPACE,
    KEY_INSERT,
    KEY_DEL,
    KEY_HOME,
    KEY_END,
    KEY_PGUP,
    KEY_PGDN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_SLASH_PAD,
    KEY_ASTERISK,
    KEY_MINUS_PAD,
    KEY_PLUS_PAD,
    KEY_DEL_PAD,
    KEY_ENTER_PAD,
    KEY_PRTSCR,
    KEY_PAUSE,
    KEY_ABNT_C1,
    KEY_YEN,
    KEY_KANA,
    KEY_CONVERT,
    KEY_NOCONVERT,
    KEY_AT,
    KEY_CIRCUMFLEX,
    KEY_COLON2,
    KEY_KANJI,
    KEY_EQUALS_PAD,
    KEY_BACKQUOTE,
    KEY_SEMICOLON,
    KEY_COMMAND,
/*
    KEY_UNKNOWN1,
    KEY_UNKNOWN2,
    KEY_UNKNOWN3,
    KEY_UNKNOWN4,
    KEY_UNKNOWN5,
    KEY_UNKNOWN6,
    KEY_UNKNOWN7,
    KEY_UNKNOWN8,

    KEY_MODIFIERS,
*/
    KEY_LSHIFT,
    KEY_RSHIFT,
    KEY_LCONTROL,
    KEY_RCONTROL,
    KEY_ALT,
    KEY_ALTGR,
    KEY_LWIN,
    KEY_RWIN,
    KEY_MENU,
    KEY_SCRLOCK,
    KEY_NUMLOCK,
    KEY_CAPSLOCK,

    0 /* end of list */
};
#endif




/* public functions */

/* initializes the module */
void inputmap_init()
{
    logfile_message("inputmap_init()");
    mappings = hashtable_inputmapnode_t_create();
    load_inputmap_table();
}

/* releases the module */
void inputmap_release()
{
    logfile_message("inputmap_release()");
    mappings = hashtable_inputmapnode_t_destroy(mappings);
}

/* returns an input mapping */
const inputmap_t *inputmap_get(const char *name)
{
    inputmapnode_t *f = hashtable_inputmapnode_t_find(mappings, name);

    if(f == NULL) {
        logfile_message("Can't find inputmap '%s' in '%s'", name, INPUTMAP_FILE);
        if((f = hashtable_inputmapnode_t_find(mappings, NULL_INPUTMAP)) == NULL) { /* fail silently */
            fatal_error("Can't find inputmap '%s' in '%s'", name, INPUTMAP_FILE);
            return NULL;
        }
    }

    return f->data;
}




/* private functions */

/* traverses an inputmap configuration file */
int traverse(const parsetree_statement_t *stmt)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    const char *name = "null";
    inputmapnode_t *f = NULL;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "inputmap") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "inputmap: must provide inputmap name");
        nanoparser_expect_program(p2, "inputmap: must provide inputmap attributes");

        name = nanoparser_get_string(p1);
        if(NULL == hashtable_inputmapnode_t_find(mappings, name)) {
            f = inputmapnode_create(name);
            nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)f, traverse_inputmap);
            hashtable_inputmapnode_t_add(mappings, name, f);
        }
        else
            fatal_error("inputmap: redefinition of inputmap '%s'", name);

        logfile_message("inputmap: loaded input map '%s'", name);
    }
    else
        fatal_error("inputmap: unknown identifier '%s' at the input map definition file. Valid keywords: 'inputmap'", identifier);

    return 0;
}

/* traverses an inputmap block */
int traverse_inputmap(const parsetree_statement_t *stmt, void *inputmapnode)
{
    inputmapnode_t* f = (inputmapnode_t*)inputmapnode;
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    int n;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "keyboard") == 0) {
        n = nanoparser_get_number_of_parameters(param_list);
        if(n == 1) {
            p1 = nanoparser_get_nth_parameter(param_list, 1);
            nanoparser_expect_program(p1, "inputmap: must provide the keyboard mappings");

            if(f->data->keyboard.enabled)
                fatal_error("inputmap: can't define multiple keyboard mappings for inputmap '%s'", f->data->name);

            f->data->keyboard.enabled = true;
            nanoparser_traverse_program_ex(nanoparser_get_program(p1), (void*)f, traverse_inputmap_keyboard);
        }
        else
            fatal_error("inputmap: 'keyboard' accepts only one parameter: a block.");
    }
    else if(str_icmp(identifier, "joystick") == 0) {
        n = nanoparser_get_number_of_parameters(param_list);
        if(n == 2) {
            p1 = nanoparser_get_nth_parameter(param_list, 1);
            p2 = nanoparser_get_nth_parameter(param_list, 2);
            nanoparser_expect_string(p1, "inputmap: must provide the joystick id");
            nanoparser_expect_program(p2, "inputmap: must provide the joystick mappings");

            if(f->data->joystick.enabled)
                fatal_error("inputmap: can't define multiple joysticks for inputmap '%s'", f->data->name);

            f->data->joystick.enabled = true;
            f->data->joystick.id = max(0, atoi(nanoparser_get_string(p1)));
            nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)f, traverse_inputmap_joystick);
        }
        else
            fatal_error("inputmap: 'joystick' requires two parameters: joystick_id and a block containing the mappings.");
    }
    else
        fatal_error("inputmap: unknown identifier '%s' defined at a inputmap block. Valid keywords: 'keyboard', 'joystick'", identifier);

    return 0;
}

/* traverses an inputmap.keyboard block */
int traverse_inputmap_keyboard(const parsetree_statement_t *stmt, void *inputmapnode)
{
    inputmapnode_t* f = (inputmapnode_t*)inputmapnode;
    inputmap_t* im = f->data;
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    enum inputbutton_t btn = IB_FIRE1;
    int n;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    n = nanoparser_get_number_of_parameters(param_list);
    if(n != 1)
        fatal_error("inputmap: commands inside a 'keyboard' block should have two items: button_name, key_name");

    if(str_icmp(identifier, "up") == 0)
        btn = IB_UP;
    else if(str_icmp(identifier, "right") == 0)
        btn = IB_RIGHT;
    else if(str_icmp(identifier, "down") == 0)
        btn = IB_DOWN;
    else if(str_icmp(identifier, "left") == 0)
        btn = IB_LEFT;
    else if(str_icmp(identifier, "fire1") == 0)
        btn = IB_FIRE1;
    else if(str_icmp(identifier, "fire2") == 0)
        btn = IB_FIRE2;
    else if(str_icmp(identifier, "fire3") == 0)
        btn = IB_FIRE3;
    else if(str_icmp(identifier, "fire4") == 0)
        btn = IB_FIRE4;
    else if(str_icmp(identifier, "fire5") == 0)
        btn = IB_FIRE5;
    else if(str_icmp(identifier, "fire6") == 0)
        btn = IB_FIRE6;
    else if(str_icmp(identifier, "fire7") == 0)
        btn = IB_FIRE7;
    else if(str_icmp(identifier, "fire8") == 0)
        btn = IB_FIRE8;
    else
        fatal_error("inputmap: invalid button name '%s' inside the 'keyboard' block", identifier);

    p1 = nanoparser_get_nth_parameter(param_list, 1);
    nanoparser_expect_string(p1, "inputmap: must provide a key name");
    im->keyboard.scancode[(int)btn] = keycode_of(nanoparser_get_string(p1));

    return 0;
}


/* traverses an inputmap.joystick block */
int traverse_inputmap_joystick(const parsetree_statement_t *stmt, void *inputmapnode)
{
    inputmapnode_t* f = (inputmapnode_t*)inputmapnode;
    inputmap_t* im = f->data;
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    enum inputbutton_t btn = IB_FIRE1;
    int n;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    n = nanoparser_get_number_of_parameters(param_list);
    if(n != 1)
        fatal_error("inputmap: commands inside a 'joystick' block should have two items: button_name, joystick_button_name");

    if(str_icmp(identifier, "fire1") == 0)
        btn = IB_FIRE1;
    else if(str_icmp(identifier, "fire2") == 0)
        btn = IB_FIRE2;
    else if(str_icmp(identifier, "fire3") == 0)
        btn = IB_FIRE3;
    else if(str_icmp(identifier, "fire4") == 0)
        btn = IB_FIRE4;
    else if(str_icmp(identifier, "fire5") == 0)
        btn = IB_FIRE5;
    else if(str_icmp(identifier, "fire6") == 0)
        btn = IB_FIRE6;
    else if(str_icmp(identifier, "fire7") == 0)
        btn = IB_FIRE7;
    else if(str_icmp(identifier, "fire8") == 0)
        btn = IB_FIRE8;
    else
        fatal_error("inputmap: invalid button name '%s' inside the 'joystick' block", identifier);

    p1 = nanoparser_get_nth_parameter(param_list, 1);
    nanoparser_expect_string(p1, "inputmap: must provide a joystick button name");
    im->joystick.button[(int)btn] = joybtncode_of(nanoparser_get_string(p1));

    return 0;
}










/* loads the inputmap table */
void load_inputmap_table()
{
    parsetree_program_t *s = NULL;
    const char* fullpath = assetfs_fullpath(INPUTMAP_FILE);

    logfile_message("inputmap: loading the input mappings...");
    hashtable_inputmapnode_t_add(mappings, NULL_INPUTMAP, inputmapnode_create(NULL_INPUTMAP));

    s = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(s, traverse);
    s = nanoparser_deconstruct_tree(s);
}

/* creates a new inputmapnode object */
inputmapnode_t* inputmapnode_create(const char* name)
{
    static const int NO_BUTTON = LARGE_INT;
    inputmapnode_t* f = mallocx(sizeof *f);
    f->data = mallocx(sizeof *(f->data));
    f->data->name = str_dup(name);

    /* defaults */
    f->data->keyboard.enabled = false;
    f->data->keyboard.scancode[IB_UP] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_RIGHT] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_DOWN] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_LEFT] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE1] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE2] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE3] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE4] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE5] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE6] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE7] = keycode_of("KEY_NONE");
    f->data->keyboard.scancode[IB_FIRE8] = keycode_of("KEY_NONE");

    f->data->joystick.enabled = false;
    f->data->joystick.id = 0;
    f->data->joystick.button[IB_FIRE1] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE2] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE3] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE4] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE5] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE6] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE7] = NO_BUTTON;
    f->data->joystick.button[IB_FIRE8] = NO_BUTTON;

    return f;
}

/* destroys a factory sound */
void inputmapnode_destroy(inputmapnode_t *f)
{
    free(f->data->name);
    free(f->data);
    free(f);
}

/* given a key name, return its scancode */
int keycode_of(const char* key_name)
{
    for(int i = 0; key_names[i] != NULL; i++) {
        if(str_icmp(key_names[i], key_name) == 0)
            return key_codes[i];
    }

    fatal_error("Failed to setup inputmap: unrecognized key name \"%s\"", key_name);
    return 0;
}

/* given a joystick button name, return its button code */
int joybtncode_of(const char* joybtn_name)
{
    if(str_icmp(joybtn_name, "BUTTON_1") == 0)
        return 0;
    else if(str_icmp(joybtn_name, "BUTTON_2") == 0)
        return 1;
    else if(str_icmp(joybtn_name, "BUTTON_3") == 0)
        return 2;
    else if(str_icmp(joybtn_name, "BUTTON_4") == 0)
        return 3;
    else if(str_icmp(joybtn_name, "BUTTON_5") == 0)
        return 4;
    else if(str_icmp(joybtn_name, "BUTTON_6") == 0)
        return 5;
    else if(str_icmp(joybtn_name, "BUTTON_7") == 0)
        return 6;
    else if(str_icmp(joybtn_name, "BUTTON_8") == 0)
        return 7;

    fatal_error("Failed to setup inputmap: unrecognized joystick button \"%s\"", joybtn_name);
    return 0;
}
