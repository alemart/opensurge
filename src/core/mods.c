/*
 * Open Surge Engine
 * mods.c - utilities for MODs & compatibility mode
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

#include <allegro5/allegro.h>
#include <allegro5/allegro_memfile.h>
#include <physfs.h>
#include <stdio.h>
#include "mods.h"
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
    "scripts/players/dash_smoke.ss",                    "0.5.0:0.6.0.3",            /* changes to the physics system; player hitbox */
    "scripts/players/lock_angle.ss",                    "0.5.0:0.6.0.3",            /* changes to the update cycle; now using lateUpdate() */
    "scripts/ui/menubuttonlist.ss",                     "0.5.0:0.6.0.3",            /* changes to the entity system; omit warnings */

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
    "scripts/items/zipline.ss",                         ":",                        /* changes to the physics system in 0.6.1; player hitbox; change the collider */
    /*"sprites/items/zipline.spr",                      "0.5.0:0.6.0.3",*/          /* add action spot to the zipline */
    "scripts/items/bridge.ss",                          ":",                        /* optimized collisions in 0.6.1 */
    "scripts/items/audio_source.ss",                    ":",                        /* optimizations in 0.6.1 */
    "scripts/items/walk_on_water.ss",                   "0.6.0:",                   /* changes to the update cycle in 0.6.1; now using lateUpdate() */
    "scripts/items/bg_xchg.ss",                         ":",                        /* changes to the entity system in 0.6.1; omit warnings */
    "scripts/items/event_trigger.ss",                   ":",                        /* changes to the entity system in 0.6.1; omit warnings */

    /* NULL-terminated array */
    NULL, NULL
};

static const size_t COMPATIBILITY_PACK_MAX_FILE_COUNT = (sizeof(COMPATIBILITY_LIST) / sizeof(char* const*)) / 2 - 1;
typedef char __compatibility_list_check[1 - 2 * ((sizeof(COMPATIBILITY_LIST) / sizeof(char* const*)) % 2 != 0)];

static const char** select_files_for_compatibility_pack(const char* engine_version);
static bool falls_within_version_range(const char* version, const char* range);
static int count_files(const char** file_list);
static bool has_pak_support();

static int scan_required_engine_version(const char* vpath, void* context);
static bool scan_level_line(const char* vpath, int line, levparser_command_t command, const char* command_name, int param_count, const char** param, void* context);


/*
 *
 * public
 *
 */

/*
 * guess_required_engine_version()
 * Guess the required engine version of the currently running MOD
 */
char* guess_required_engine_version(char* buffer, size_t buffer_size)
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
 * generate_compatibility_pack()
 * Generates a compatibility .pak file for an engine version. You must call
 * release_pak_file() on the output file data. Returns true on success.
 */
bool generate_compatibility_pack(const char* engine_version, void** out_file_data, size_t* out_file_size)
{
    /* generate .pak file */
    const char** file_list = select_files_for_compatibility_pack(engine_version);
    int file_count = count_files(file_list);
    bool success = generate_pak_file(file_list, file_count, out_file_data, out_file_size);
    free(file_list);

    /* done! */
    return success;
}

/*
 * generate_pak_file()
 * Generates a .pak file given an array of virtual paths. You must call
 * release_pak_file() on the output file data. Returns true on success.
 */
bool generate_pak_file(const char** file_list, int file_count, void** out_pak_data, size_t* out_pak_size)
{
    /* initialize */
    *out_pak_data = NULL;
    *out_pak_size = 0;

    /* validation */
    if(!has_pak_support()) {
        CRASH("Compatibility mode is not available because PhysFS has been compiled without PAK support.");
        return false;
    }
    else if(file_count == 0) {
        WARN("No files have been added to the compatibility pack!");
        return false;
    }

    /* determine the size of each file */
    size_t* file_size = alloca(file_count * sizeof(*file_size));
    size_t* accum_file_size = alloca((1 + file_count) * sizeof(*accum_file_size));

    accum_file_size[0] = 0;
    for(int i = 0; i < file_count; i++) {
        if(PHYSFS_exists(file_list[i])) {
            ALLEGRO_FILE* f = al_fopen(file_list[i], "rb");
            if(f != NULL) {
                int64_t length = al_fsize(f);
                if(length < 0)
                    WARN("Can't determine the size of \"%s\". It will be removed.", file_list[i]);

                file_size[i] = (length >= 0) ? length : 0;
                accum_file_size[i+1] = accum_file_size[i] + file_size[i];

                al_fclose(f);
                continue;
            }
            else
                WARN("Can't open file for reading: \"%s\". It will be removed.", file_list[i]);
        }

        WARN("Removing \"%s\"...", file_list[i]);
        file_size[i] = 0;
        accum_file_size[i+1] = accum_file_size[i] + file_size[i];
    }

    /* compute the size of the pack */
    const size_t header_size = 16;
    const size_t toc_entry_size = 64;
    size_t toc_size = file_count * toc_entry_size;
    size_t data_size = accum_file_size[file_count];
    size_t pack_size = header_size + toc_size + data_size;

    /* allocate memory for the pack file */
    uint8_t* pack_data = mallocx(pack_size);

    /* open the pack file for writing */
    ALLEGRO_FILE* packf = al_open_memfile(pack_data, pack_size, "wb");
    if(NULL == packf) {
        LOG("Can't open the compatibility pack file for writing!");
        free(pack_data);
        return false;
    }

    /* write the header (16 bytes) */
    al_fwrite(packf, "PACK", 4); /* signature (4 bytes) */
    al_fwrite32le(packf, header_size); /* position of the table of contents (4 bytes) */
    al_fwrite32le(packf, toc_size); /* size in bytes of the table of contents (4 bytes) */
    al_fwrite(packf, "COOL", 4); /* magic blanks (4 bytes) */

    /* write the entries of the table of contents
       (each is 64 bytes) */
    char filename[56];
    uint32_t data_start = header_size + toc_size;

    for(int i = 0; i < file_count; i++) {
        /* validate the filename */
        int length_of_filename = strlen(file_list[i]);
        assertx(length_of_filename > 0 && length_of_filename < sizeof(filename));

        /* write the filename (56 bytes) */
        memset(filename, 0, sizeof(filename));
        str_cpy(filename, file_list[i], sizeof(filename));
        al_fwrite(packf, filename, sizeof(filename));

        /* write the position of the file (4 bytes) */
        al_fwrite32le(packf, data_start + accum_file_size[i]);

        /* write the size of the file (4 bytes) */
        al_fwrite32le(packf, file_size[i]);
    }

    /* close the pack file
       we'll write directly to memory now */
    al_fclose(packf);

    /* tightly write the data of the compatibility pack */
    for(int i = 0; i < file_count; i++) {
        if(file_size[i] > 0) { /* if the file exists... */
            ALLEGRO_FILE* f = al_fopen(file_list[i], "rb");
            uint8_t* ptr = pack_data + data_start + accum_file_size[i];

            if(f == NULL) {
                WARN("File \"%s\" hasn't been added to the compatibility pack because it can't be read!", file_list[i]);
                memset(ptr, ' ', file_size[i]);
                continue;
            }

            size_t n = al_fread(f, ptr, file_size[i]);
            if(n < file_size[i]) {
                WARN("File \"%s\" hasn't been fully written to the compatibility pack!", file_list[i]);
                WARN("File size: %lu. Written bytes: %lu.", (unsigned long)file_size[i], (unsigned long)n);
            }

            al_fclose(f);
        }
    }

    /* done! */
    *out_pak_data = (void*)pack_data;
    *out_pak_size = pack_size;
    return true;
}

/*
 * generate_pak_file_from_memory()
 * Generate a .pak archive with files stored in memory.
 * Call release_pak_file() on the output data after usage.
 */
bool generate_pak_file_from_memory(const char** vpath, int file_count, const void** file_data, const size_t* file_size, void** out_pak_data, size_t* out_pak_size)
{
    /* initialize */
    *out_pak_data = NULL;
    *out_pak_size = 0;

    /* validation */
    if(!has_pak_support()) {
        CRASH("Compatibility mode is not available because PhysFS has been compiled without PAK support.");
        return false;
    }
    else if(vpath == NULL || file_count == 0) {
        WARN("No files have been added to the compatibility pack!");
        return false;
    }

    /* accumulate file_size[] */
    size_t* accum_file_size = alloca((1 + file_count) * sizeof(*accum_file_size));

    accum_file_size[0] = 0;
    for(int i = 0; i < file_count; i++)
        accum_file_size[i+1] = accum_file_size[i] + file_size[i];

    /* compute the size of the pack */
    const size_t header_size = 16;
    const size_t toc_entry_size = 64;
    size_t toc_size = file_count * toc_entry_size;
    size_t data_size = accum_file_size[file_count];
    size_t pack_size = header_size + toc_size + data_size;

    /* allocate memory for the pack file */
    uint8_t* pack_data = mallocx(pack_size);

    /* open the pack file for writing */
    ALLEGRO_FILE* packf = al_open_memfile(pack_data, pack_size, "wb");
    if(NULL == packf) {
        LOG("Can't open the compatibility pack file for writing!");
        free(pack_data);
        return false;
    }

    /* ----- */

    /* write the header (16 bytes) */
    al_fwrite(packf, "PACK", 4); /* signature (4 bytes) */
    al_fwrite32le(packf, header_size); /* position of the table of contents (4 bytes) */
    al_fwrite32le(packf, toc_size); /* size in bytes of the table of contents (4 bytes) */
    al_fwrite(packf, "COOL", 4); /* magic blanks (4 bytes) */

    /* ----- */

    /* write the table of contents (each is an entry of 64 bytes) */
    char filename[56];
    uint32_t data_start = header_size + toc_size;
    
    for(int i = 0; i < file_count; i++) {

        /* validate the data */
        assertx(file_data[i] != NULL || file_size[i] == 0);

        /* validate the filename */
        int length_of_filename = strlen(vpath[i]);
        assertx(length_of_filename > 0 && length_of_filename < sizeof(filename));

        /* write the filename (56 bytes) */
        memset(filename, 0, sizeof(filename));
        str_cpy(filename, vpath[i], sizeof(filename));
        al_fwrite(packf, filename, sizeof(filename));

        /* write the position of the file (4 bytes) */
        al_fwrite32le(packf, data_start + accum_file_size[i]);

        /* write the size of the file (4 bytes) */
        al_fwrite32le(packf, file_size[i]);

    }

    /* ----- */

    /* tightly write the file data */
    for(int i = 0; i < file_count; i++)
        al_fwrite(packf, file_data[i], file_size[i]);

    /* ----- */

    /* close the pack file */
    al_fclose(packf);

    /* done! */
    *out_pak_data = (void*)pack_data;
    *out_pak_size = pack_size;
    return true;
}

/*
 * release_pak_file()
 * Releases a .pak file previously generated with generate_pak_file()
 */
void release_pak_file(void* pak)
{
    uint8_t* file_data = (uint8_t*)pak;

    if(file_data != NULL)
        free(file_data);
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
 * files for a compatibility pack, given an engine version. You must free() the
 * return of this function.
 */
const char** select_files_for_compatibility_pack(const char* engine_version)
{
    const char** array = mallocx((1 + COMPATIBILITY_PACK_MAX_FILE_COUNT) * sizeof(const char*));

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
 * count_files()
 * Counts the number of elements of a NULL-terminated array of strings
 */
int count_files(const char** file_list)
{
    int count = 0;

    if(file_list != NULL) {
        while(*file_list != NULL) {
            ++count;
            ++file_list;
        }
    }

    return count;
}

/*
 * has_pak_support()
 * Checks if physfs has been compiled with .PAK file support
 */
bool has_pak_support()
{
    assertx(PHYSFS_isInit());

    for(const PHYSFS_ArchiveInfo **i = PHYSFS_supportedArchiveTypes(); *i != NULL; i++) {
        if(str_icmp((*i)->extension, "PAK") == 0)
            return true;
    }

    return false;
}