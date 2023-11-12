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

/* require alloca */
#if !(defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh))
#include <malloc.h>
#if defined(__linux__) || defined(__linux) || defined(__EMSCRIPTEN__)
#include <alloca.h>
#endif
#endif

/* utility macros */
#define LOG(...)                        logfile_message("[mod-compat] " __VA_ARGS__)
#define WARN(...)                       do { fprintf(stderr, "[mod-compat] " __VA_ARGS__); fprintf(stderr, "\n"); LOG(__VA_ARGS__); } while(0)
#define CRASH(...)                      fatal_error("[mod-compat] " __VA_ARGS__)

/*

COMPATIBILITY MODE
------------------

In compatibility mode, we automatically generate a compatibility pack based on
the engine version of the MOD and on the engine version of the executable. This
pack overrides any files of the MOD.

The compatibility pack is a small set of files (mostly scripts) that allows a
MOD to run in the present versin of the engine with no errors, no warnings and
no missing features.

The compatibility pack is generated on a file-by-file basis. It is a subset of
the compatibility list below. Each file is link to a version range of the form
[first:last] (inclusive). If the engine version of the MOD falls within the
range, the corresponding file will be included in the compatibility pack.

If a particular file is added to the compatibility pack but does not exist in
the present version of the engine, then the file will be considered empty,
effectively removing it from the MOD.

Note: this assumes that the user has been using the Open Surge Import Utility
to port his or her MOD to newer versions of the engine. If the user intentionally
mixes up old scripts with new versions of the engine, the outcome is undefined
behavior.

*/
static const char* const COMPATIBILITY_LIST[] = {
    /* filepath (up to 55 characters) */                /* version range */         /* notes */

    /* compatibility fixes */
    "sprites/ui/pause.spr",                             ":0.6.0.3",                 /* introduce a new pause menu in 0.6.1 */
    "scripts/core/hud.ss",                              ":0.6.0.3",                 /* mobile: add pause button to the HUD */
    "scripts/core/water.ss",                            "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */
    "scripts/functions/ui/show_message.ss",             "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */
    "scripts/functions/camera/lock_camera.ss",          "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */
    "scripts/functions/player/give_extra_lives.ss",     "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */
    "scripts/misc/lucky_bonus.ss",                      "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */
    "scripts/items/power_pluggy.ss",                    "0.6.0:0.6.0.3",            /* changes to the physics system */
    "scripts/items/salamander_bridge.ss",               "0.6.0:0.6.0.3",            /* changes to the physics system; prevent soft lock */
    "scripts/enemies/marmotred.ss",                     "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */
    "scripts/misc/animal.ss",                           "0.5.0:0.6.0.3",            /* animation fix in 0.6.1 */
    "scripts/players/lock_angle.ss",                    "0.5.0:0.6.0.3",            /* changes to the update cycle; now using lateUpdate() */
    "scripts/ui/menubuttonlist.ss",                     "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */

#if 0
    /* cancelled changes */

    /* the physics hitbox matches the old one in compatibility mode */
    "scripts/players/dash_smoke.ss",                    "0.5.0:0.6.0.3",            /* changes to the physics system; player hitbox */



    /* future changes TODO */

    /* better with a diff */
    "scripts/core/cleared.ss",                          "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */

    /* better with a diff */
    /* hud.ss */
#endif

    /* basic scripts (keep updated with bugfixes, etc.);
       these scripts must all be backwards-compatible;
       it's like they're built into the engine */

    "scripts/behaviors/circular_movement.ss",           ":",
    "scripts/behaviors/directional_movement.ss",        ":",
    "scripts/behaviors/enemy.ss",                       ":",
    "scripts/behaviors/platformer.ss",                  ":",

    "scripts/items/profiler.ss",                        ":",                        /* always use own Profiler */
    "scripts/core/surge_gameplay.ss",                   "0.6.0:",                   /* update Surge Gameplay */
    "scripts/core/camera.ss",                           ":",                        /* changes to the update cycle in 0.6.1; now using lateUpdate() */
    "scripts/items/springs.ss",                         ":",                        /* changes to the physics in 0.6.1 */
    "scripts/items/spring_booster.ss",                  ":",
    "scripts/items/collectibles.ss",                    ":",                        /* performance updates in 0.6.1 */
    "scripts/items/tubes.ss",                           ":",
    "scripts/items/pipes.ss",                           ":",                        /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/items/zipline.ss",                         ":",                        /* changes to the physics system in 0.6.1; player hitbox; change the collider and the repositioning method of the pipe sensor */
    /*"sprites/items/zipline.spr",                      "0.5.0:0.6.0.3",*/          /* add action spot to the zipline */
    "scripts/items/bridge.ss",                          ":",                        /* optimized collisions in 0.6.1 */
    "scripts/items/audio_source.ss",                    ":",                        /* optimizations in 0.6.1 */
    "scripts/items/walk_on_water.ss",                   "0.6.0:",                   /* changes to the update cycle in 0.6.1; now using lateUpdate() */
    "scripts/items/bg_xchg.ss",                         ":",                        /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/items/event_trigger.ss",                   ":",                        /* changes to the entity system in 0.6.1; omit warnings */

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
 * Compute the ID of an opensurge game
 * Pass NULL to game_dirname, and optionally to required_engine_version,
 * if playing the base game
 */
uint32_t find_game_id(const char* game_dirname, const char* required_engine_version)
{
    /*

    The game ID is a 32-bit number intended to uniquely identify a specific
    release ("version") of an opensurge game.

    Using the name of the game folder to determine the game ID is simple and
    fast to compute, as well as a reasonable choice, but is it robust?

    We want to be able to distinguish between different releases of the same
    game. The name of the game folder may or may not reflect that difference.
    If it doesn't, this distinction can be done (to an extent) by looking at
    the required engine vesion of the game. If multiple releases of a game
    share a required engine version, then this information is insufficient.

    Using user-supplied names & versions is not necessarily robust. A user-
    supplied name (e.g., in surge.cfg) is less likely to change in different
    versions than the name of the folder of the game. Using a user-supplied
    version string is reasonable but not foolproof: it may change in unknown
    ways, or not change at all. Maybe use a checksum in the future?

    Important: any changes to this method should be backwards-compatible.

    */
    const char salt[] = ":/";
    char buffer[256], version_string[16];
    int length = 0, c = 0;

    /* check if base game */
    if(game_dirname == NULL) {
        game_dirname = "./" GAME_UNIXNAME;
        if(required_engine_version == NULL)
            required_engine_version = GAME_VERSION_STRING;
    }
    assertx(*game_dirname != '\0');
    assertx(required_engine_version != NULL && *required_engine_version != '\0');

    /* enforce a valid format for the engine version; remove any -suffix */
    int wip = 0, dots = 0;
    int version_code = parse_version_number_ex(required_engine_version, NULL, NULL, NULL, &wip);
    stringify_version_number(version_code, version_string, sizeof(version_string));
    for(char* p = version_string; *p != '\0'; p++) dots += (*p == '.');
    assertx(wip != 0 || dots <= 2); /* accept x.y.z; reject x.y.z.0 */

    /* generate a salted string */
    str_cpy(buffer, version_string, sizeof(buffer));
    length += strlen(buffer);
    str_cpy(buffer + length, salt, sizeof(buffer) - length);
    length += sizeof(salt) - 1; /* remove '\0' */
    assertx(length < sizeof(buffer));
    str_cpy(buffer + length, game_dirname, sizeof(buffer) - length);

    /* compute a modified, case insensitive djb2 hash of the salted string
       collisions are unlikely in actual games */
    uint32_t hash = 5381;
    for(char* str = buffer; (c = *((unsigned char*)(str++)));)
        hash = ((hash << 5) + hash) + toupper(c); /* hash * 33 + c */

    /* done! */
    return hash;
}

/*
 * guess_engine_version_of_mod()
 * Guess the required engine version of the currently running MOD
 */
char* guess_engine_version_of_mod(char* buffer, size_t buffer_size)
{
    int max_version_code = 0;

    /* begin with an initial guess */
    const char* initial_guess = "0.5.0";
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

/*
 * select_files_for_compatibility_pack()
 * Returns a NULL-terminated array of statically allocated strings of suitable
 * files for a compatibility pack, given an engine version.
 */
const char** select_files_for_compatibility_pack(const char* engine_version)
{
    static const char* array[1 + COMPATIBILITY_PACK_MAX_FILE_COUNT];

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