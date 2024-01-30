/*
 * Open Surge Engine
 * inputmap.c - custom input mappings
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

#include <allegro5/allegro.h>

#include "inputmap.h"
#include "logfile.h"
#include "resourcemanager.h"
#include "asset.h"
#include "nanoparser.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/hashtable.h"

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
static int read_script(const char* vpath, void* param);
static int traverse(const parsetree_statement_t* stmt, void* vpath);
static int traverse_inputmap(const parsetree_statement_t* stmt, void* inputmapnode);
static int traverse_inputmap_keyboard(const parsetree_statement_t* stmt, void* inputmapnode);
static int traverse_inputmap_joystick(const parsetree_statement_t* stmt, void* inputmapnode);

/* misc */
static const char* NULL_INPUTMAP = "null";
static int keycode_of(const char* key_name);
static bool parse_joystick_button_name(const char* joybtn_name, int* result);
static bool parse_button_name(const char* button_name, inputbutton_t* result);
static bool digits_only(const char* str);

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



/* public functions */

/*
 * inputmap_init()
 * Loads the inputmaps
 */
void inputmap_init()
{
    logfile_message("Initializing inputmaps...");

    /* create table */
    mappings = hashtable_inputmapnode_t_create();

    /* read the inputmap scripts */
    asset_foreach_file("inputs/", ".in", read_script, NULL, true);

    /* read the legacy script AFTER you read all the regular scripts */
    if(asset_exists("config/input.def"))
        read_script("config/input.def", NULL);
}

/*
 * inputmap_release()
 * Unloads the inputmaps
 */
void inputmap_release()
{
    logfile_message("Releasing inputmaps...");
    mappings = hashtable_inputmapnode_t_destroy(mappings);
}

/*
 * inputmap_get()
 * Get an inputmap given its name
 */
const inputmap_t* inputmap_get(const char* name)
{
    inputmapnode_t* f = hashtable_inputmapnode_t_find(mappings, name);

    if(f == NULL) {
        /* fail silently */
        logfile_message("WARNING: Can't find inputmap '%s'", name);

        if((f = hashtable_inputmapnode_t_find(mappings, NULL_INPUTMAP)) == NULL) {
            /* this shouldn't happen */
            fatal_error("Can't find inputmap '%s'", name);
            return NULL;
        }
    }

    return f->data;
}

/*
 * inputmap_exists()
 * Checks if an input mapping with the given name exists
 */
bool inputmap_exists(const char* name)
{
    if(name == NULL)
        return false;

    return NULL != hashtable_inputmapnode_t_find(mappings, name);
}





/* private functions */

/* read an inputmap script */
int read_script(const char* vpath, void* param)
{
    /* load the script */
    const char* fullpath = asset_path(vpath);
    parsetree_program_t* prog = nanoparser_construct_tree(fullpath);

    /* traverse the script */
    nanoparser_traverse_program_ex(prog, (void*)vpath, traverse);

    /* done! */
    nanoparser_deconstruct_tree(prog);
    return 0;
}

/* traverses an inputmap configuration file */
int traverse(const parsetree_statement_t* stmt, void* vpath)
{
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    const char *identifier;
    const char *name;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "inputmap") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "inputmap: must provide inputmap name");
        nanoparser_expect_program(p2, "inputmap: must provide inputmap attributes");

        name = nanoparser_get_string(p1);
        if(*name == '\0')
            fatal_error("inputmap: empty names are not accepted in %s:%d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        if(NULL == hashtable_inputmapnode_t_find(mappings, name)) {
            inputmapnode_t* f = inputmapnode_create(name);
            nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)f, traverse_inputmap);
            hashtable_inputmapnode_t_add(mappings, name, f);
            logfile_message("inputmap: loaded inputmap '%s' from %s", name, nanoparser_get_file(stmt));
        }
        else
            logfile_message("WARNING: can't redefine inputmap '%s' in %s:%d", name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    }
    else
        fatal_error("inputmap: unknown identifier '%s' in %s:%d. Valid keywords: 'inputmap'", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* traverses an inputmap block */
int traverse_inputmap(const parsetree_statement_t* stmt, void* inputmapnode)
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
                fatal_error("inputmap: can't define multiple keyboard mappings for inputmap '%s' in %s:%d", f->data->name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

            f->data->keyboard.enabled = true;
            nanoparser_traverse_program_ex(nanoparser_get_program(p1), inputmapnode, traverse_inputmap_keyboard);
        }
        else
            fatal_error("inputmap: 'keyboard' accepts only one parameter: a block (in %s:%d)", nanoparser_get_file(stmt), nanoparser_get_file(stmt));
    }
    else if(str_icmp(identifier, "joystick") == 0) {
        n = nanoparser_get_number_of_parameters(param_list);
        if(n == 2) {
            p1 = nanoparser_get_nth_parameter(param_list, 1);
            p2 = nanoparser_get_nth_parameter(param_list, 2);
            nanoparser_expect_string(p1, "inputmap: must provide the joystick number");
            nanoparser_expect_program(p2, "inputmap: must provide the joystick mappings");

            if(f->data->joystick.enabled)
                fatal_error("inputmap: can't define multiple joysticks for inputmap '%s' in %s:%d", f->data->name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

            f->data->joystick.enabled = true;
            f->data->joystick.number = max(1, atoi(nanoparser_get_string(p1)));
            nanoparser_traverse_program_ex(nanoparser_get_program(p2), inputmapnode, traverse_inputmap_joystick);
        }
        else
            fatal_error("inputmap: 'joystick' requires two parameters: joystick_number and a block containing the mappings (in %s:%d)", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
    }
    else
        fatal_error("inputmap: unknown identifier '%s' defined at inputmap block in %s:%d. Valid keywords: 'keyboard', 'joystick'", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* traverses an inputmap.keyboard block */
int traverse_inputmap_keyboard(const parsetree_statement_t* stmt, void* inputmapnode)
{
    inputmap_t* im = ((inputmapnode_t*)inputmapnode)->data;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    enum inputbutton_t btn = IB_FIRE1;
    const char *identifier;
    int n;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    n = nanoparser_get_number_of_parameters(param_list);
    if(n != 1)
        fatal_error("inputmap: commands inside a 'keyboard' block must have two items: button_name, key_name (in %s:%d)", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    if(!parse_button_name(identifier, &btn))
        fatal_error("inputmap: invalid button name '%s' inside the 'keyboard' block in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    p1 = nanoparser_get_nth_parameter(param_list, 1);
    nanoparser_expect_string(p1, "inputmap: must provide a key name");
    im->keyboard.scancode[(int)btn] = keycode_of(nanoparser_get_string(p1));

    return 0;
}


/* traverses an inputmap.joystick block */
int traverse_inputmap_joystick(const parsetree_statement_t* stmt, void* inputmapnode)
{
    inputmap_t* im = ((inputmapnode_t*)inputmapnode)->data;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    enum inputbutton_t btn = IB_FIRE1;
    const char *identifier, *joybtn_name;
    int joybtn_code = -1;
    int i, n;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    n = nanoparser_get_number_of_parameters(param_list);
    if(n < 1)
        fatal_error("inputmap: declarations inside a 'joystick' block must have at least two items: button_name, joystick_button_name (in %s:%d)", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    if(!parse_button_name(identifier, &btn))
        fatal_error("inputmap: invalid button name '%s' inside the 'joystick' block in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    for(i = 1; i <= n; i += 2) {
        p1 = nanoparser_get_nth_parameter(param_list, i);
        nanoparser_expect_string(p1, "inputmap: must provide a joystick button name");
        joybtn_name = nanoparser_get_string(p1);

        if(!parse_joystick_button_name(joybtn_name, &joybtn_code))
            fatal_error("Failed to setup inputmap: unrecognized joystick button \"%s\" in %s:%d", joybtn_name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        if(joybtn_code >= 0 && joybtn_code < MAX_JOYSTICK_BUTTONS)
            im->joystick.button_mask[(int)btn] |= (1 << joybtn_code);
        
        /* expect the "OR" symbol or the end of the list */
        if(i < n) {
            p2 = nanoparser_get_nth_parameter(param_list, i+1);
            nanoparser_expect_string(p2, "inputmap: expected additional buttons or the end of the list");
            if(strcmp(nanoparser_get_string(p2), "|") != 0 || i+1 == n)
                fatal_error("Failed to setup inputmap: you must write '|' (OR symbol) __between__ joystick buttons in %s:%d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        }
    }

    return 0;
}










/* creates a new inputmapnode object */
inputmapnode_t* inputmapnode_create(const char* name)
{
    const int NO_KEY = keycode_of("KEY_NONE");
    const int NO_BUTTONS = 0; /* empty mask */
    inputbutton_t button;

    /* allocate structure */
    inputmapnode_t* f = mallocx(sizeof *f);
    f->data = mallocx(sizeof *(f->data));
    f->data->name = str_dup(name);

    /* keyboard defaults */
    f->data->keyboard.enabled = false;
    for(button = 0; button < IB_MAX; button++)
        f->data->keyboard.scancode[(int)button] = NO_KEY;

    /* joystick defaults */
    f->data->joystick.enabled = false;
    f->data->joystick.number = 1;
    for(button = 0; button < IB_MAX; button++)
        f->data->joystick.button_mask[(int)button] = NO_BUTTONS;

    /* done! */
    return f;
}

/* destroys a inputmapnode object */
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

/* given a joystick button name, retrieve its button code.
   e.g., BUTTON_1 becomes 0; BUTTON_2 becomes 1, etc.
   BUTTON_NONE becomes -1. Returns true on success */
bool parse_joystick_button_name(const char* joybtn_name, int* result)
{
    if(str_icmp(joybtn_name, "BUTTON_NONE") == 0) {
        *result = -1;
        return true;
    }

    if(str_incmp(joybtn_name, "BUTTON_", 7) == 0 && digits_only(joybtn_name + 7)) {
        int joybtn_number = atoi(joybtn_name + 7);
        if(joybtn_number >= 1 && joybtn_number <= MAX_JOYSTICK_BUTTONS) {
            *result = joybtn_number - 1;
            return true;
        }
    }

    *result = -1;
    return false;
}

/* convert fire1, fire2, ... to IB_FIRE1, IB_FIRE2 ...
   respectively. Returns true on success */
bool parse_button_name(const char* button_name, inputbutton_t* result)
{
    /* fire buttons */
    if(str_incmp(button_name, "fire", 4) == 0 && digits_only(button_name + 4)) {
        int fire_number = atoi(button_name + 4);
        if(fire_number >= 1 && fire_number <= 8) {
            *result = IB_FIRE1 + (fire_number - 1);
            return true;
        }
        else
            return false;
    }

    /* directionals */
    if(str_icmp(button_name, "left") == 0) {
        *result = IB_LEFT;
        return true;
    }

    if(str_icmp(button_name, "right") == 0) {
        *result = IB_RIGHT;
        return true;
    }

    if(str_icmp(button_name, "up") == 0) {
        *result = IB_UP;
        return true;
    }

    if(str_icmp(button_name, "down") == 0) {
        *result = IB_DOWN;
        return true;
    }

    /* unrecognized button name */
    return false;
}

/* does the given string contain only digits? */
bool digits_only(const char* str)
{
    while(*str) {
        if(!isdigit(*(str++)))
            return false;
    }

    return true;
}
