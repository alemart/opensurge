/*
 * Open Surge Engine
 * install.c - installs/builds/lists Open Surge games
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include "zip/zip.h"
#include "install.h"
#include "assetfs.h"
#include "global.h"
#include "util.h"

#if !defined(_WIN32)
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

/* private stuff */
static bool is_suffix(const char* str, const char* pattern);
static bool is_prefix(const char* str, const char* pattern);
static char* guess_root_folder(const char* zip_fullpath);
static char* guess_gameid(const char* zip_fullpath);
static bool is_game_package(const char* zip_fullpath);
static int compare_gameid(const char* gameid, void* data);
static int list_installed_games(const char* gameid, void* data);
static int write_to_zip(const char* vpath, void* param);
static bool is_valid_id(const char* str);
static const char* basename(const char* path);
static void print(const char* fmt, ...);



/*
 * install_game()
 * Installs a game, given the absolute path to its zip package
 * Will write the gameid to buffer out_gameid, if it's not NULL
 * Note: interactive_mode may be set to true only on a console
 */
bool install_game(const char* zip_fullpath, char* out_gameid, size_t out_gameid_size, bool interactive_mode)
{
    if(is_game_package(zip_fullpath)) {
        char* root_folder = guess_root_folder(zip_fullpath); /* ends with a '/', or is empty ("") */
        char* gameid = guess_gameid(zip_fullpath);
        bool success = true;

        /* Are we overwriting something? */
#if !defined(_WIN32)
        if(interactive_mode && is_game_installed(gameid)) {
#else
        if(interactive_mode) {
#endif
            char buf[8] = { 0 };
            for(;;) {
#if !defined(_WIN32)
                fprintf(stdout, "It seems that %s is already installed. Overwrite? (y/n) ", gameid);
#else
                fprintf(stdout, "Files will be overwritten to install %s. Proceed? (y/n) ", gameid);
#endif
                fflush(stdout);
                if(fgets(buf, sizeof(buf), stdin) != NULL) {
                    char c = tolower(buf[0]);
                    if(c == 'y' || c == 'n') {
                        success = (c == 'y');
                        break;
                    }
                }
                else {
                    success = false;
                    break;
                }
            }
        }

        /* Sanity check */
        if(assetfs_initialized()) {
            print("Can't install %s: assetfs is initialized", gameid);
            success = false;
        }

        /* Install the game */
        if(success) {
            bool strict = assetfs_use_strict(false);
            struct zip_t* zip;
            print("Installing %s...", gameid);
            assetfs_init(gameid, NULL);
            if((zip = zip_open(zip_fullpath, 0, 'r')) != NULL) {
                /* Unpack the game */
                int n = zip_total_entries(zip), r = strlen(root_folder);
                for(int i = 0; i < n; i++) {
                    zip_entry_openbyindex(zip, i);
                    {
                        const char* path = zip_entry_name(zip);
                        if(is_prefix(path, root_folder)) {
                            const char* fullpath = assetfs_create_data_file(path + r, true); /* will create all the subfolders */
                            if(!zip_entry_isdir(zip))
                                zip_entry_fread(zip, fullpath);
                        }
                    }
                    zip_entry_close(zip);
                }
                zip_close(zip);
                print("Success.");

                /* Inform the gameid */
                if(out_gameid != NULL) {
                    strncpy(out_gameid, gameid, out_gameid_size);
                    out_gameid[out_gameid_size] = '\0';
                }
            }
            else {
                print("Can't open \"%s\".", basename(zip_fullpath)); /* shouldn't happen */
                success = false;
            }
            assetfs_release();
            assetfs_use_strict(strict);
        }
        else
            print("Won't install.");
        
        free(gameid);
        free(root_folder);
        return success;
    }
    else {
        print("Not a game package: \"%s\".", basename(zip_fullpath));
        return false;
    }
}

/*
 * foreach_installed_game()
 * Gets the gameid of the installed games.
 * Returns the number of successful callbacks.
 * If callback returns non-zero, the enumeration stops.
 */
int foreach_installed_game(int (*callback)(const char*,void*), void* data)
{
#if !defined(_WIN32)
    if(assetfs_initialized()) {
        const char* tmp = assetfs_create_data_file("", true); /* path to userdata space */
        char* games_folder = strcat(strcpy(mallocx((5 + strlen(tmp)) * sizeof(*games_folder)), tmp), "/../");
        int num_games = 0;
        DIR* dir = opendir(games_folder);

        if(dir != NULL) {
            struct dirent* d;
            while((d = readdir(dir))) {
                if(0 != strcmp(d->d_name, ".") && 0 != strcmp(d->d_name, "..")) {
                    const char* gameid = d->d_name;
                    if(is_valid_id(gameid)) { /* sanity check */
                        struct stat st;
                        char* path = strcat(strcpy(mallocx((1 + strlen(games_folder) + strlen(gameid)) * sizeof(*path)), games_folder), gameid);
                        stat(path, &st); /* lstat */
                        free(path);
                        if(S_ISDIR(st.st_mode)) {
                            if(0 != callback(gameid, data))
                                break;
                            num_games++;
                        }
                    }
                }
            }
            closedir(dir);
        }

        free(games_folder);
        return num_games;
    }
    else {
        int value = 0;
        assetfs_init(NULL, NULL);
        value = foreach_installed_game(callback, data);
        assetfs_release();
        return value;
    }
#else
    if(0 != callback(GAME_UNIXNAME, data))
        return 0;
    return 1;
#endif
}

/*
 * is_game_installed()
 * Checks if a game is installed
 */
bool is_game_installed(const char* gameid)
{
    if(assetfs_initialized()) {
        int n = foreach_installed_game(compare_gameid, (void*)gameid);
        int m = foreach_installed_game(compare_gameid, (void*)".");
        return n != m;
    }
    else {
        bool result = false;
        assetfs_init(NULL, NULL);
        result = is_game_installed(gameid);
        assetfs_release();
        return result;
    }
}

/*
 * build_game()
 * Builds a package with a game
 */
bool build_game(const char* gameid)
{
    bool success = true;

    /* Starting... */
    gameid = gameid && *gameid ? gameid : GAME_UNIXNAME;

    /* Sanity check */
    if(assetfs_initialized()) {
        print("Can't build %s: assetfs is initialized", gameid);
        return false;
    }

    /* Does the game we want to build a package for exist? */
    if(is_game_installed(gameid)) {
        struct zip_t* zip;
        char* zip_path = strcat(strcpy(mallocx((5 + strlen(gameid)) * sizeof(*gameid)), gameid), ".zip");

        print("Building %s...", gameid);
        assetfs_init(gameid, NULL);

        if((zip = zip_open(zip_path, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w')) != NULL) {
            assetfs_foreach_file("/", NULL, write_to_zip, zip, true);
            zip_close(zip);
            print("Saved to \"%s\".", zip_path);
            if(0 == strcmp(gameid, GAME_UNIXNAME))
                print("You may rename the file.");
        }
        else {
            print("Can't open \"%s\" for writing.", zip_path);
            success = false;
        }

        assetfs_release();
        free(zip_path);
    }
    else {
        print("Can't build %s: game doesn't exist.", gameid);
        print("Existing games:");
        foreach_installed_game(list_installed_games, NULL);
        success = false;
    }

    return success;
}



/* ----- private ----- */

/* checks if pattern is a suffix of str */
bool is_suffix(const char* str, const char* pattern)
{
    int start = strlen(str) - strlen(pattern);
    return start >= 0 && strcmp(str + start, pattern) == 0;
}

/* checks if pattern is a prefix of str */
bool is_prefix(const char* str, const char* pattern)
{
    return strncmp(str, pattern, strlen(pattern)) == 0;
}

/* guess the root (base) folder of a zip; you'll need to free() this
   Returns NULL on error (or if it's not a valid zip) */
char* guess_root_folder(const char* zip_fullpath)
{
    struct zip_t* zip = zip_open(zip_fullpath, 0, 'r');
    char* root_folder = NULL;

    if(zip != NULL) {
        const char* dir = "levels/";
        const char* ext = ".lev";
        int i, n = zip_total_entries(zip);
        for(i = 0; i < n && !root_folder; i++) {
            zip_entry_openbyindex(zip, i);
            if(!zip_entry_isdir(zip)) {
                const char* entry_name = zip_entry_name(zip), *p;
                if((p = strstr(entry_name, dir)) != NULL && is_suffix(entry_name, ext)) {
                    int len = p - entry_name;
                    root_folder = mallocx((1 + len) * sizeof(*root_folder));
                    strncpy(root_folder, entry_name, len);
                    root_folder[len] = '\0';
                }
            }
            zip_entry_close(zip);
        }
        zip_close(zip);
    }

    return root_folder;
}

/* is this a game package? */
bool is_game_package(const char* zip_fullpath)
{
    char* root_folder = guess_root_folder(zip_fullpath);
    bool valid_package = (root_folder != NULL);
    if(root_folder != NULL)
        free(root_folder);
    return valid_package;
}

/* the filename of a path */
const char* basename(const char* path)
{
    const char* p = strpbrk(path, "\\/");
    while(p != NULL) {
        path = p+1;
        p = strpbrk(path, "\\/");
    }
    return path;
}

/* validates an ID: only lowercase alphanumeric characters are accepted */
bool is_valid_id(const char* str)
{
    const int maxlen = 80; /* won't get even close to the NAME_MAX of the system */

    if(str) {
        int len = 0;
        for(; *str; str++) {
            if(!((*str >= 'a' && *str <= 'z') || (*str >= '0' && *str <= '9')) || ++len > maxlen)
                return 0;
        }
        return len > 0;
    }

    return false;
}

/* useful for is_game_installed */
int compare_gameid(const char* gameid, void* data)
{
    return (strcmp(gameid, (const char*)data) == 0);
}

/* helper for build game */
int list_installed_games(const char* gameid, void* data)
{
    print("- %s", gameid);
    return 0;
}

/* helper for build game: write a file to the zip */
int write_to_zip(const char* vpath, void* param)
{
    struct zip_t* zip = (struct zip_t*)param;
    if(vpath[0] != '.' && strstr(vpath, "/.") == NULL) { /* skip files */
        if(assetfs_is_data_file(vpath) && !is_suffix(vpath, ".zip")) { /* very important to skip the .zip */
            if(!(is_prefix(vpath, "screenshots/") && is_suffix(vpath, ".png"))) {
                zip_entry_open(zip, vpath);
                if(0 != zip_entry_fwrite(zip, assetfs_fullpath(vpath)))
                    print("Can't pack \"%s\"", vpath);
                zip_entry_close(zip);
            }
        }
    }
    return 0;
}

/* guesses a gameid (only lowercase letters / numbers);
   you'll have to free() this */
char* guess_gameid(const char* zip_fullpath)
{
    char gameid[16] = { 0 };
    const char* p = basename(zip_fullpath);
    int i, maxlen = sizeof(gameid) - 1;

    for(i = 0; i < maxlen && *p; p++) {
        if(isalnum(*p))
            gameid[i++] = tolower(*p);
        else if(*p == '.')
            break;
    }
    gameid[i] = '\0';

    if(*gameid == '\0')
        strcpy(gameid, "game");

    return strcpy(mallocx(sizeof(gameid)), gameid);
}

/* prints a formatted message (with a newline at the end) */
void print(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}
