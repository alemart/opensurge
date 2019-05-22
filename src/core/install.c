/*
 * Open Surge Engine
 * install.c - installs/builds/lists Open Surge games
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
#include "stringutil.h"
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
static void console_print(const char* fmt, ...);
static bool console_ask(const char* fmt, ...);
static bool remove_folder(const char* fullpath);



/*
 * install_game()
 * Installs a game, given the absolute path to its zip package
 * Will write the gameid to buffer out_gameid, if it's not NULL
 * Note: interactive_mode may be set to true only on a console
 */
bool install_game(const char* zip_fullpath, char* out_gameid, size_t out_gameid_size, bool interactive_mode)
{
    if(is_game_package(zip_fullpath) && !assetfs_initialized()) {
        char* root_folder = guess_root_folder(zip_fullpath); /* ends with a '/', or is empty ("") */
        char* gameid = guess_gameid(zip_fullpath);
        bool success = true;

        /* Are we overwriting something? */
        if(interactive_mode) {
#if !defined(_WIN32)
            if(is_game_installed(gameid))
                success = console_ask("It seems that %s is already installed. Overwrite?", gameid);
#else
            success = console_ask("Files will be overwritten to install %s. Proceed?", gameid);
#endif
        }

        /* Install the game */
        if(success) {
            struct zip_t* zip;
            if((zip = zip_open(zip_fullpath, 0, 'r')) != NULL) {
                int i, n = zip_total_entries(zip), r = strlen(root_folder);
                const char* destdir;
                bool use_strict;

                /* Init assetfs */
                use_strict = assetfs_use_strict(false);
                assetfs_init(gameid, NULL);
                destdir = assetfs_create_data_file("", true);
                console_print("Installing %s to \"%s\"...", gameid, destdir);

                /* Inform the gameid */
                if(out_gameid != NULL) {
                    strncpy(out_gameid, gameid, out_gameid_size);
                    out_gameid[out_gameid_size] = '\0';
                }

                /* Unpack the game */
                for(i = 0; i < n; i++) {
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
                console_print("Success.");

                /* Release assetfs */
                assetfs_release();
                assetfs_use_strict(use_strict);
            }
            else {
                console_print("Can't open \"%s\".", str_basename(zip_fullpath)); /* shouldn't happen */
                success = false;
            }
        }
        else
            console_print("Won't install.");

        /* done */
        free(gameid);
        free(root_folder);
        return success;
    }
    else if(assetfs_initialized()) {
        console_print("Can't install \"%s\": assetfs is initialized.", str_basename(zip_fullpath));
        return false;
    }
    else {
        console_print("Not a game package: \"%s\".", str_basename(zip_fullpath));
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
    const char* gameid = GAME_UNIXNAME;
    if(!is_valid_id(gameid) || 0 != callback(gameid, data))
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
        console_print("Can't build %s: assetfs is initialized", gameid);
        return false;
    }

    /* Does the game we want to build a package for exist? */
    if(is_game_installed(gameid)) {
        struct zip_t* zip;
        char* zip_path = strcat(strcpy(mallocx((5 + strlen(gameid)) * sizeof(*gameid)), gameid), ".zip");

        console_print("Building %s...", gameid);
        assetfs_init(gameid, NULL);

        if((zip = zip_open(zip_path, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w')) != NULL) {
            assetfs_foreach_file("/", NULL, write_to_zip, zip, true);
            zip_close(zip);
            console_print("Saved to \"%s\".", zip_path);
            if(0 == strcmp(gameid, GAME_UNIXNAME))
                console_print("You may rename the file.");
        }
        else {
            console_print("Can't open \"%s\" for writing.", zip_path);
            success = false;
        }

        assetfs_release();
        free(zip_path);
    }
    else {
        console_print("Can't build %s: game doesn't exist.", gameid);
        console_print("Existing games:");
        foreach_installed_game(list_installed_games, NULL);
        success = false;
    }

    return success;
}


/*
 * uninstall_game()
 * Removes files created by install_game()
 * Note: interactive_mode may be set to true only on a console
 */
bool uninstall_game(const char* gameid, bool interactive_mode)
{
#if !defined(_WIN32)
    bool use_strict;
    bool remove_config_files = false; /* defaults to false */
    const char* data_folder;
    const char* cache_folder;
    const char* config_folder;

    /* validate gameid */
    gameid = gameid && *gameid ? gameid : GAME_UNIXNAME;

    /* is assetfs already initialized? */
    if(assetfs_initialized()) {
        console_print("Can't uninstall %s: assetfs is initialized.", gameid);
        return false;
    }

    /* is the given game not installed? */
    if(!is_game_installed(gameid)) {
        console_print("Game %s is not installed. Check if the gameid is spelled correctly.", gameid);
        return false;
    }

    /* confirm the operation */
    if(interactive_mode) {
        bool default_game = (strcmp(gameid, GAME_UNIXNAME) == 0);
        if(!console_ask("Are you sure you want to %s %s? This will delete data!", (default_game ? "reset" : "uninstall"), gameid)) {
            console_print("Won't proceed.");
            return false;
        }
        remove_config_files = console_ask(
            "Delete save states and configuration data as well? [default: %c]",
            remove_config_files ? 'y' : 'n'
        );
    }

    /* init assetfs */
    use_strict = assetfs_use_strict(false);
    assetfs_init(gameid, NULL);

    /* get the absolute paths */
    data_folder = assetfs_create_data_file("", true);
    cache_folder = assetfs_create_cache_file("");
    config_folder = assetfs_create_config_file("");

    /* deleting the data */
    console_print("Deleting data files in \"%s\"...", data_folder);
    remove_folder(data_folder);
    console_print("Deleting cache files in \"%s\"...", cache_folder);
    remove_folder(cache_folder);
    if(remove_config_files) {
        console_print("Deleting configuration files in \"%s\"...", config_folder);
        remove_folder(config_folder);
    }

    /* release assetfs */
    assetfs_release();
    assetfs_use_strict(use_strict);

    /* done */
    console_print("Done!");
    return true;
#else
    console_print("Not implemented on this operating system.");
    return false;
#endif
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
    console_print("- %s", gameid);
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
                    console_print("Can't pack \"%s\"", vpath);
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
    char gameid[32] = { 0 };
    const char* p = str_basename(zip_fullpath);
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
void console_print(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
    fprintf(stdout, "\n");
}

/* asks a y/n question on the console */
bool console_ask(const char* fmt, ...)
{
    va_list args;
    char c, buf[80] = { 0 };

    for(;;) {
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);
        fprintf(stdout, " (y/n) ");
        fflush(stdout);

        if(fgets(buf, sizeof(buf), stdin) != NULL) {
            buf[max(0, strlen(buf)-1)] = '\0';
            c = tolower(buf[0]);
            if((c == 'y' || c == 'n') && buf[1] == '\0')
                return (c == 'y');
        }
        else
            return false;
    }
}

/* Removes a folder: rm -rf fullpath
   Returns true on success */
bool remove_folder(const char* fullpath)
{
#if !defined(_WIN32)
    bool success = true;
    DIR* dir = opendir(fullpath);

    if(dir != NULL) {
        /* scan the directory */
        struct dirent* d;
        while((d = readdir(dir))) {
            char* path = mallocx((strlen(fullpath) + strlen(d->d_name) + 2) * sizeof(*path));
            bool is_dir = false;
            bool is_file = false;
            struct stat st;

            /* compute the path of the current entry */
            strcpy(path, fullpath);
            if(path[0] && path[strlen(path) - 1] != '/')
                strcat(path, "/");
            strcat(path, d->d_name);

            /* check the type of the entry */
            lstat(path, &st);
            is_dir = S_ISDIR(st.st_mode) && !(S_ISLNK(st.st_mode));
            is_file = S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);

            /* recurse on directories */
            if(is_dir) {
                if(0 != strcmp(d->d_name, ".") && 0 != strcmp(d->d_name, "..")) {
                    if(!remove_folder(path))
                        success = false;
                }
            }

            /* remove the file */
            else if(is_file) {
                if(0 != unlink(path)) {
                    console_print("Can't remove file \"%s\": %s", path, strerror(errno));
                    success = false;
                }
            }

            /* done */
            free(path);
        }

        /* close the directory */
        if(closedir(dir))
            console_print("Can't close directory \"%s\": %s", fullpath, strerror(errno));

        /* remove the empty folder */
        if(success && 0 != rmdir(fullpath)) {
            console_print("Can't remove directory \"%s\": %s", fullpath, strerror(errno));
            success = false;
        }
    }
    else
        console_print("Can't scan directory \"%s\": %s", fullpath, strerror(errno));

    /* done! */
    return success;
#else
    ; /* not implemented */
#endif
}