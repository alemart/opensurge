/*
 * Open Surge Engine
 * modutils.c - utilities for MODs & compatibility mode
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

#include <string.h>
#include <ctype.h>
#include "modutils.h"
#include "asset.h"
#include "global.h"
#include "logfile.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../scenes/util/levparser.h"

/* utility macros */
#define LOG(...)                        logfile_message("[modutils] " __VA_ARGS__)
#define WARN(...)                       do { fprintf(stderr, "[modutils] " __VA_ARGS__); fprintf(stderr, "\n"); LOG(__VA_ARGS__); } while(0)
#define CRASH(...)                      fatal_error("[modutils] " __VA_ARGS__)

/*

Compatibility mode
------------------

In compatibility mode, we automatically generate a compatibility pack based on
the engine version of the MOD and on the engine version of the executable. This
pack overrides any files of the MOD.

The compatibility pack is a small set of files (mostly scripts) that allows a
MOD to run in the present version of the engine with - ideally - no errors, no
warnings and no missing features.

The compatibility pack is generated on a file-by-file basis. It is generated
with a subset of the compatibility list below. Each file is linked to a version
range of the form [first:last] (inclusive). If the compatibility version falls
within the range, the corresponding file will be included in the compatibility
pack. The compatibility version is usually the engine version of the MOD. When
writing the "last" part of the range, consider the development builds as well.

If a particular file is added to the compatibility pack but does not exist in
the present version of the engine, then that file will be considered empty,
effectively removing it from the MOD.

Note: we assume that the user has been using the Open Surge Import Utility to
port his or her MOD to newer versions of the engine. The files listed below
are assumed to be the latest versions of the official releases of the engine.
If the user intentionally mixes up old scripts with new versions of the engine,
the outcome is undefined behavior.



Usage policy
------------

Even though overwriting files of a MOD may work fine, it is nonetheless an
invasive operation. Doing so may lead to undefined behavior in the case of
merge conflicts. Therefore, we wish to change the MOD as little as possible.
We adopt the following policy to minimize the number of file substitutions:

1) keep the list of files small; don't include a script unless there is a
   good reason for it. You may include it due to some new feature that is
   required in newer engine versions, or to fix an observed bug.

2) restrict the version range of the included files as much as possible.

Ideally, old scripts shipped with the MOD will run fine in new versions of
the engine. Occasionally this is not possible, as in the case of required
new features or bugfixes. Bugfixes that are not critical may be left out of
the compatibility pack, or they may be added if a merge conflict is unlikely.

It's safer to substitute files that the user is unlikely to change in a
breaking way. If a file substitution leads to error in a particular MOD due
to a merge conflict, then we can patch the MOD individually or drop the
substitution in that case - if it's not critical. TODO

*/
static const char* const COMPATIBILITY_LIST[] = {
    /* filepath (up to 55 characters) */                /* version range */         /* notes */

    /* TODO move this list to a CSV file?
       TODO exclusion filter? */

#if 1
    /* active changes */
    "sprites/ui/pause.spr",                             ":0.6.0.3",                 /* introduce a new pause menu in 0.6.1 */
    "scripts/core/hud.ss",                              ":0.6.1",                   /* mobile: add pause button to the Default HUD in 0.6.1 */
    "scripts/core/pause.ss",                            ":0.6.1",                   /* better user experience in 0.6.1 */
    "inputs/default.in",                                ":0.6.1",                   /* updated mappings are better */

    "scripts/core/camera.ss",                           ":0.6.1",                   /* changes to the update cycle in 0.6.1; now using lateUpdate() */
    "scripts/items/walk_on_water.ss",                   "0.6.0:0.6.1",              /* changes to the update cycle in 0.6.1; now using lateUpdate() */
    "scripts/players/lock_angle.ss",                    ":0.6.1",                   /* changes to the update cycle in 0.6.1; now using lateUpdate() */
    "scripts/friends/skaterbug.ss",                     ":0.6.1",                   /* changes to the update cycle in 0.6.1; now using player.moveBy() */

    "scripts/core/water.ss",                            ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/items/bg_xchg.ss",                         ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/items/event_trigger.ss",                   ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/enemies/marmotred.ss",                     ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/functions/ui/show_message.ss",             ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/functions/camera/lock_camera.ss",          ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/functions/player/give_extra_lives.ss",     ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/misc/lucky_bonus.ss",                      ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/ui/menubuttonlist.ss",                     ":0.6.1",                   /* changes to the entity system in 0.6.1; omit warnings */

    "scripts/items/power_pluggy.ss",                    "0.6.0:0.6.1",              /* changes to the physics and to the entity system in 0.6.1; using lateUpdate(), increased the size of a collider, and more */
    "scripts/items/salamander_bridge.ss",               "0.6.0:0.6.1",              /* changes to the physics system in 0.6.1; prevent soft lock */
    "scripts/items/pipes.ss",                           ":0.6.1",                   /* changes to the physics system in 0.6.1; player hitbox; change the collider and the repositioning method of the pipe sensor */

    "scripts/items/tubes.ss",                           ":0.6.1",                   /* bugfixes in 0.6.1; prevent soft lock */
    "scripts/items/bridge.ss",                          ":0.6.1",                   /* optimized collisions in 0.6.1 */
    "scripts/items/collectibles.ss",                    ":0.6.1",                   /* performance updates in 0.6.1 */
    "scripts/items/audio_source.ss",                    ":0.6.1",                   /* optimizations in 0.6.1 */
    "scripts/items/springs.ss",                         ":0.6.1",                   /* bugfix in 0.6.1 */
    "scripts/items/zipline.ss",                         ":0.6.1",                   /* changes to the physics systems in 0.6.1; fix collisions due to a changed player hitbox */
    "sprites/items/zipline.spr",                        ":0.6.1",                   /* add action spot to the zipline */
    "scripts/misc/animal.ss",                           ":0.6.1",                   /* animation fix in 0.6.1 */
    "scripts/behaviors/platformer.ss",                  ":0.6.0",                   /* since 0.6.1, animal.ss uses Platformer.gravityMultiplier introduced in 0.6.0 (underwater effect) */

    "scripts/surge_the_rabbit/surge_the_rabbit.ss",     ":0.6.1",                   /* use own script */

    "sprites/ui/pointer.spr",                           ":0.6.1",                   /* simplified in 0.6.1 */

    "scripts/items/profiler.ss",                        ":",                        /* always use own Profiler */
#endif

#if 0
    /* removed changes */
    "scripts/core/cleared.ss",                          ":0.6.1",                   /* mobile: show and hide the mobile gamepad; need special exception */
    "scripts/players/dash_smoke.ss",                    ":0.6.1",                   /* changes to the physics system in 0.6.1; repositioned due to the updated hitbox of the player */
    "-scripts/core/motd.ss",                            "0.6.1:",                   /* example: prefix a file with '-' to remove it (make it blank) */
#endif

    /* NULL-terminated array */
    NULL, NULL
};

enum { COMPATIBILITY_PACK_MAX_FILE_COUNT = (sizeof(COMPATIBILITY_LIST) / sizeof(char* const*)) / 2 - 1 };
typedef char __compatibility_list_check[1 - 2 * ((sizeof(COMPATIBILITY_LIST) / sizeof(char* const*)) % 2 != 0)];

static bool falls_within_version_range(const char* version, const char* range);
static int scan_required_engine_version(const char* vpath, void* context);
static bool scan_level_line(const char* vpath, int line, levparser_command_t command, const char* command_name, int param_count, const char** param, void* context);


/*
 *
 * public
 *
 */


/*
 * find_game_id()
 * Compute the ID of an opensurge game. You may pass NULL to game_dirname,
 * in which case GAME_ID_UNAVAILABLE will be returned
 */
uint32_t find_game_id(const char* game_title, const char* game_version, const char* game_dirname, const char* required_engine_version)
{
    /*

    The game ID is a 32-bit number intended to uniquely identify a specific
    release ("version") of an opensurge game.

    ---

    How to compute an ID:

    1. Use a user-supplied game title, the required engine version and the
       recursive directory listing of levels/, converting all .lev filenames
       to uppercase and replacing \ by / for cross-platform consistency.

    2. If a user-supplied game title is not available, use in its place the
       name of the directory informed via --game-folder (command line option).

    3. If the name of the directory is not available, then that is probably
       a legacy game running in-place. [... to be continued ...]

    Compute a 32-bit hash of the data and return it as the game ID. Lowercase
    or uppercase letters should not change the ID.

    The ID should be computed quickly.

    Important: any changes to this method should be backwards-compatible.

    */

   /* TODO */
   (void)game_title;
   (void)game_version;
   (void)game_dirname;
   (void)required_engine_version;

   return GAME_ID_UNAVAILABLE;
}

/*
 * guess_engine_version_of_mod()
 * Guess the required engine version of the currently running MOD
 */
char* guess_engine_version_of_mod(char* buffer, size_t buffer_size)
{
    int max_version_code = 0;

    /* begin with an initial guess */
    const char* initial_guess = "0.0.0";
    max_version_code = parse_version_number(initial_guess);

    /* guess the required engine version by reading the .lev files */
    assertx(asset_is_init());
    asset_foreach_file("levels/", ".lev", scan_required_engine_version, &max_version_code, true);

    /* return the guessed version */
    return stringify_version_number(max_version_code, buffer, sizeof(buffer_size));

    /* TODO: also scan import_log.txt if available? */
    /* logfile.txt isn't a reliable source! */
    /* we could look for "Open Surge Engine version ... [space]" in the .exe, if available,
       but then we would not be able to downgrade the game as easily */
}

/*
 * select_files_for_compatibility_pack()
 * Returns a NULL-terminated array of statically allocated strings of suitable
 * files for a compatibility pack, given an engine version.
 */
const char** select_files_for_compatibility_pack(const char* engine_version, uint32_t game_id)
{
    static const char* array[1 + COMPATIBILITY_PACK_MAX_FILE_COUNT];
    (void)game_id;

    /* initialize the array */
    array[0] = NULL;

    /* test each file */
    for(int i = 0, j = 0; COMPATIBILITY_LIST[i] != NULL; i += 2) {
        const char* filepath = COMPATIBILITY_LIST[i];
        const char* version_range = COMPATIBILITY_LIST[1+i];

        if(falls_within_version_range(engine_version, version_range)) {
            LOG("Picking \"%s\"...", filepath);
            array[j++] = filepath;
            array[j] = NULL;
        }
    }

    /* done! */
    return array;
}

/*
 * generate_surge_cfg()
 * Generates a surge.cfg file in-memory. You must free() the output void* pointer afterwards.
 * Returns true on success
 */
bool generate_surge_cfg(const char* game_title, void** out_file_data, size_t* out_file_size)
{
    char buffer[256], title[64], fmt[] = ""
        "game {\n"
        "    title \"%.63s\"\n"
        "}\n"
    ;

    str_addslashes(game_title, title, sizeof(title));
    snprintf(buffer, sizeof(buffer), fmt, title);

    int text_length = strlen(buffer);
    size_t file_size = text_length * sizeof(uint8_t);
    *out_file_data = memcpy(mallocx(file_size), buffer, file_size);
    *out_file_size = file_size;

    return true;
}



/*
 *
 * private
 *
 */


/*
 * scan_required_engine_version()
 * Scan a .lev file, looking for the "required" field
 */
int scan_required_engine_version(const char* vpath, void* context)
{
    levparser_parse(vpath, context, scan_level_line);
    return 0;
}

/*
 * scan_level_line()
 * Scan a line of a .lev file, looking for the "required" field
 */
bool scan_level_line(const char* vpath, int line, levparser_command_t command, const char* command_name, int param_count, const char** param, void* context)
{
    /* skip */
    if(command != LEVCOMMAND_REQUIRES)
        return true;

    /* invalid line? */
    if(param_count == 0)
        return true; /* skip */

    /* read version */
    const char* version = param[0];
    int version_code = parse_version_number(version);

    /* compare version */
    int* max_version_code = (int*)context;
    if(version_code > *max_version_code)
        *max_version_code = version_code;

    /* we're done reading this file */
    return false;
}

/*
 * falls_within_version_range()
 * Checks if an engine version of the form x.y.z[.w] falls within a range of
 * the form [first:last] (inclusive)
 */
bool falls_within_version_range(const char* version, const char* range)
{
    char buf[32], *p, *q;
    const char* min_version = "0.5.0";
    const char* max_version = "99.99.99.99";
    int first = 0, last = 0, test_version = 0;

    /* parse version range */
    p = str_cpy(buf, range, sizeof(buf));
    q = strchr(buf, ':');

    if(q == NULL) {
        CRASH("Invalid version range: %s", range);
        return false;
    }

    *(q++) = '\0';
    first = parse_version_number((*p != '\0') ? p : min_version);

    if(*q == '-')
        last = parse_version_number(GAME_VERSION_STRING) - atoi(q+1);
    else
        last = parse_version_number((*q != '\0') ? q : max_version);

    if(last < first || NULL != strchr(q, ':')) {
        CRASH("Invalid version range: %s", range);
        return false;
    }

    /* parse test version */
    test_version = parse_version_number(version);

    /* test if the engine version falls within the interval */
    return first <= test_version && test_version <= last;
}
