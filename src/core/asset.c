/*
 * Open Surge Engine
 * asset.c - asset manager (virtual filesystem)
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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
#include <allegro5/allegro_physfs.h>
#include <physfs.h>
#include <stdio.h>
#include <string.h>
#include "physfs/ignorecase.h"
#include "asset.h"
#include "global.h"
#include "util.h"
#include "stringutil.h"
#include "logfile.h"

/* The default directory of the game assets provided by upstream (*nix only) */
#ifndef GAME_DATADIR
#define GAME_DATADIR                   "/usr/share/games/" GAME_UNIXNAME
#endif

/* The default name of the user-modifiable asset directory (*nix only) */
#ifndef GAME_USERDIRNAME
#define GAME_USERDIRNAME               GAME_UNIXNAME
#endif

/* If RUNINPLACE is non-zero, then the assets will be read only from the directory of the executable */
#ifndef GAME_RUNINPLACE
#define GAME_RUNINPLACE                0
#endif

/* Utility macros */
#define ASSET_PATH_MAX                 4096
#define PHYSFSx_getLastErrorMessage()  PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#define LOG(...)                       logfile_message("[asset] " __VA_ARGS__)
#define WARN(...)                      do { fprintf(stderr, "[asset] " __VA_ARGS__); fprintf(stderr, "\n"); LOG(__VA_ARGS__); } while(0)
#define CRASH(...)                     fatal_error("[asset] " __VA_ARGS__)
#define ASSERT(expr) do { \
    if(!(expr)) { \
        CRASH("Assertion failed: " #expr " in %s at %s:%d", __func__, __FILE__, __LINE__); \
    } \
} while(0)

/* Utilities */
static char* gamedir = NULL; /* custom asset folder specified by the user */
static ALLEGRO_STATE state;
static ALLEGRO_PATH* find_exedir();
static ALLEGRO_PATH* find_homedir();
static ALLEGRO_PATH* find_shared_datadir();
static ALLEGRO_PATH* find_user_datadir();
static void create_dir(const ALLEGRO_PATH* path);
static bool is_valid_gamedir(const char* gamedir);
static const char* find_extension(const char* path);
static const char* case_insensitive_fix(const char* virtual_path);
static bool foreach_file(ALLEGRO_PATH* dirpath, const char* extension_filter, int (*callback)(const char* virtual_path, void* user_data), void* user_data, bool recursive);
static bool clear_dir(ALLEGRO_FS_ENTRY* entry);



/*
 * asset_init()
 * Initializes the asset manager.
 * Pass NULL to optional_gamedir to use the default search paths
 */
void asset_init(const char* argv0, const char* optional_gamedir)
{
    /* already initialized? */
    if(PHYSFS_isInit())
        return;

    /* initialize physfs */
    if(!PHYSFS_init(argv0))
        CRASH("Can't initialize physfs. %s", PHYSFSx_getLastErrorMessage());

    /* copy the gamedir, if specified */
    gamedir = (optional_gamedir != NULL) ? str_dup(optional_gamedir) : NULL;

    /* set the search path to the specified gamedir */
    if(gamedir != NULL) {

        /* validate the gamedir */
        if(!is_valid_gamedir(gamedir))
            CRASH("Can't use the game directory at %s. Make sure that it exists and that you have read-write privileges to it.", gamedir);

        /* set the write dir */
        if(!PHYSFS_setWriteDir(gamedir))
            CRASH("Can't set the write directory to %s. Error: %s", gamedir, PHYSFSx_getLastErrorMessage());

        /* mount gamedir to the root */
        if(!PHYSFS_mount(gamedir, "/", 1))
            CRASH("Can't mount the game directory at %s. Error: %s", gamedir, PHYSFSx_getLastErrorMessage());

    }

    /* set the default search paths */
    else {
        ALLEGRO_PATH* shared_datadir = find_shared_datadir();
        ALLEGRO_PATH* user_datadir = find_user_datadir();
        const char* dirpath;

        /* create the user dir if it doesn't exist */
        create_dir(user_datadir);

        /* set the write dir */
        dirpath = al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP);
        if(!PHYSFS_setWriteDir(dirpath))
            CRASH("Can't set the write directory to %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());

        /* mount the user path to the root (higher precedence) */
        /*dirpath = al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP);*/
        if(!PHYSFS_mount(dirpath, "/", 1))
            CRASH("Can't mount the user data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());

        /* mount the shared path to the root */
        dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP);
        if(!PHYSFS_mount(dirpath, "/", 1))
            CRASH("Can't mount the shared data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());

        /* done! */
        al_destroy_path(user_datadir);
        al_destroy_path(shared_datadir);
    }

    /* enable the physfs file interface. This should be the last task
       performed in this function (e.g., see create_dir()) */
    al_store_state(&state, ALLEGRO_STATE_NEW_FILE_INTERFACE);
    al_set_physfs_file_interface();
}

/*
 * asset_release()
 * Releases the asset manager
 */
void asset_release()
{
    /* not initialized? */
    if(!PHYSFS_isInit())
        return;

    /* release the gamedir string, if any */
    if(gamedir != NULL)
        free(gamedir);

    /* restore the previous I/O backend */
    al_restore_state(&state);

    /* deinit physfs */
    if(!PHYSFS_deinit())
        LOG("Can't deinitialize physfs. %s", PHYSFSx_getLastErrorMessage());
}

/*
 * asset_exists()
 * Checks if a file exists
 */
bool asset_exists(const char* virtual_path)
{
    if(PHYSFS_exists(virtual_path))
        return true;

    /* Try a case-insensitive match. Modders on Windows
       sometimes specify paths that do not match the files
       in a case-sensitive manner. This creates difficulties
       when running their mods on Linux for example. */
    return (NULL != case_insensitive_fix(virtual_path));
}

/*
 * asset_path()
 * Get the actual path of a file, given its virtual path on the virtual filesystem
 * Do not modify the returned string!
 */
const char* asset_path(const char* virtual_path)
{
    /* the virtual path exists on the virtual filesystem */
    if(PHYSFS_exists(virtual_path))
        return virtual_path;

    /* try a case-insensitive match */
    const char* fixed_path = case_insensitive_fix(virtual_path);
    if(fixed_path != NULL)
        return fixed_path; /* allocated statically */

    /* the file doesn't exist. Maybe we'll write to it? */
    return virtual_path;
}

/*
 * asset_foreach_file()
 * Enumerate files.
 * virtual_path_of_directory is the path to a directory in the virtual filesystem ("/" is the root)
 * extension_filter may be NULL or a single extension with a dot, e.g., ".lev", ".ss"
 * callback must return 0 to let the enumeration proceed, or non-zero to stop it
 * user_data is any pointer
 * recursive lets you decide whether or not we'll enter sub-folders
 */
void asset_foreach_file(const char* virtual_path_of_directory, const char* extension_filter, int (*callback)(const char* virtual_path, void* user_data), void* user_data, bool recursive)
{
    ALLEGRO_PATH* dirpath = al_create_path_for_directory(virtual_path_of_directory);
    foreach_file(dirpath, extension_filter, callback, user_data, recursive);
    al_destroy_path(dirpath);
}

/*
 * asset_purge_user_data()
 * Purge the user-space data. Return false on error.
 * Cannot be called if the virtual filesystem is initialized
 */
bool asset_purge_user_data()
{
#if (GAME_RUNINPLACE)

    (void)clear_dir;

    WARN("Unsupported operation when GAME_RUNINPLACE is set");
    return false;

#elif defined(_WIN32)

    (void)clear_dir;

    WARN("Unsupported operation on this operating system");
    return false;

#else

    /* fail if the virtual filesystem is initialized */
    if(PHYSFS_isInit())
        return false;

    /* uninitialized Allegro? */
    if(!al_is_system_installed())
        al_init();

    /* get the user data dir */
    ALLEGRO_PATH* path = find_user_datadir();
    const char* user_datadir = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

    /* validity check */
    ALLEGRO_FS_ENTRY* entry = al_create_fs_entry(user_datadir);
    bool valid = (entry != NULL) && al_fs_entry_exists(entry);
    if(!valid)
        WARN("Invalid directory: %s. errno = %d", user_datadir, al_get_errno());

    /* clear & remove folder */
    bool success = valid && clear_dir(entry) && al_remove_fs_entry(entry);

    /* cleanup */
    al_destroy_fs_entry(entry); /* it's okay if entry is NULL. Will it ever be? */
    al_destroy_path(path);

    /* done! */
    return success;

#endif
}

/*
 * asset_user_datadir()
 * Get the absolute path to the user-modifiable data folder
 * Return dest
 */
char* asset_user_datadir(char* dest, size_t dest_size)
{
    /* custom gamedir? */
    if(gamedir != NULL)
        return str_cpy(dest, gamedir, dest_size);

    /* uninitialized Allegro? */
    if(!al_is_system_installed())
        al_init();

    /* find the default path */
    ALLEGRO_PATH* path = find_user_datadir();
    const char* dirpath = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

    str_cpy(dest, dirpath, dest_size);

    al_destroy_path(path);
    return dest;
}

/*
 * asset_user_datadir()
 * Get the absolute path to the data folder provided by upstream
 * Return dest
 */
char* asset_shared_datadir(char* dest, size_t dest_size)
{
    /* custom gamedir? */
    if(gamedir != NULL)
        return str_cpy(dest, gamedir, dest_size);

    /* uninitialized Allegro? */
    if(!al_is_system_installed())
        al_init();

    /* find the default path */
    ALLEGRO_PATH* path = find_shared_datadir();
    const char* dirpath = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

    str_cpy(dest, dirpath, dest_size);

    al_destroy_path(path);
    return dest;
}




/*
 * private stuff
 */


/*
 * find_exedir()
 * Path of the directory of the executable file
 */
ALLEGRO_PATH* find_exedir()
{
    ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
    ASSERT(path != NULL);

    al_set_path_filename(path, NULL);

    return path;
}


/*
 * find_homedir()
 * User's home directory
 */
ALLEGRO_PATH* find_homedir()
{
    ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
    ASSERT(path != NULL);

    return path;
}



/*
 * find_shared_datadir()
 * Finds the path of the directory storing the game assets provided by upstream
 */
ALLEGRO_PATH* find_shared_datadir()
{
#if (GAME_RUNINPLACE)

    return find_exedir();

#elif defined(_WIN32)

    return find_exedir();

#elif defined(__APPLE__) && defined(__MACH__)

    /*
    macOS App Bundle

    exedir: Contents/MacOS/
    datadir: Contents/Resources/
    */

    ALLEGRO_PATH* path = find_exedir();

    al_append_path_component(path, "..");
    al_append_path_component(path, "Resources");

    return path;

#elif defined(__unix__) || defined(__unix)

    (void)find_exedir;

    return al_create_path_for_directory(GAME_DATADIR);

#else
#error "Unsupported operating system."
#endif
}

/*
 * find_user_datadir()
 * Finds the path of the user-modifiable directory storing game assets
 */
ALLEGRO_PATH* find_user_datadir()
{
#if (GAME_RUNINPLACE)

    (void)find_homedir;

    return find_exedir();

#else

    /* Use custom user path? */
    const char* env = getenv("OPENSURGE_USER_PATH");
    if(env != NULL)
        return al_create_path_for_directory(env);

#if defined(_WIN32)

    (void)find_homedir;

    return find_exedir();

#elif defined(__APPLE__) && defined(__MACH__)

    /*
    Return ~/Library/Application Support/opensurge/
    */

    ALLEGRO_PATH* path = find_homedir();

    al_append_path_component(path, "Library");
    al_append_path_component(path, "Application Support");
    al_append_path_component(path, GAME_USERDIRNAME);

    return path;

#elif defined(__unix__) || defined(__unix)

    /*
    Return $XDG_DATA_HOME/opensurge/
    or ~/.local/share/opensurge/
    */

    const char* xdg_data_home = getenv("XDG_DATA_HOME");
    ALLEGRO_PATH* path;

    if(xdg_data_home == NULL) {
        path = find_homedir();
        al_append_path_component(path, ".local");
        al_append_path_component(path, "share");
    }
    else
        path = al_create_path_for_directory(xdg_data_home);

    al_append_path_component(path, GAME_USERDIRNAME);
    return path;

#else
#error "Unsupported operating system."
#endif
#endif
}

/*
 * create_dir()
 * Create a new directory (and any parent directories as needed)
 */
void create_dir(const ALLEGRO_PATH* path)
{
    const char* path_str = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

    if(!al_make_directory(path_str))
        LOG("Can't create directory %s. errno = %d", path_str, al_get_errno());
}

/*
 * clear_dir()
 * Remove the contents of a directory. Return true on success
 */
bool clear_dir(ALLEGRO_FS_ENTRY* entry)
{
    ALLEGRO_FS_ENTRY* next;
    uint32_t mode = al_get_fs_entry_mode(entry);
    bool success = true;

    if(!(mode & ALLEGRO_FILEMODE_ISDIR)) {
        WARN("Not a directory: %s. errno = %d", al_get_fs_entry_name(entry), al_get_errno());
        return false;
    }

    if(!al_open_directory(entry)) {
        WARN("Can't open directory %s. errno = %d", al_get_fs_entry_name(entry), al_get_errno());
        return false;
    }

    while(NULL != (next = al_read_directory(entry))) {
        /* recurse on directories */
        mode = al_get_fs_entry_mode(next);
        if(mode & ALLEGRO_FILEMODE_ISDIR)
            success = success && clear_dir(next);

        /* remove entry */
        if(!al_remove_fs_entry(next)) {
            WARN("Can't remove %s. errno = %d", al_get_fs_entry_name(next), al_get_errno());
            success = false;
        }
        
        /* we're done with this entry */
        al_destroy_fs_entry(next);
    }

    if(!al_close_directory(entry)) {
        WARN("Can't close directory %s. errno = %d", al_get_fs_entry_name(entry), al_get_errno());
        return false;
    }

    return success;
}

/*
 * is_valid_gamedir()
 * Checks if the given gamedir exists and has read-write permissions
 * gamedir is assumed not to be NULL
 */
bool is_valid_gamedir(const char* gamedir)
{
    ALLEGRO_FS_ENTRY* entry = al_create_fs_entry(gamedir);
    bool exists = al_fs_entry_exists(entry);
    uint32_t mode = exists ? al_get_fs_entry_mode(entry) : 0;
    al_destroy_fs_entry(entry);

    if(!exists)
        return false;

    #if 0
    if(!(mode & ALLEGRO_FILEMODE_ISDIR)) {
        const char* extension = find_extension(gamedir);
        if(!((mode & ALLEGRO_FILEMODE_ISFILE) && (0 == str_icmp(extension, ".zip")))) /* only zips? */
            return false;
    }
    #endif

    return (mode & ALLEGRO_FILEMODE_READ) && (mode & ALLEGRO_FILEMODE_WRITE);
}

/*
 * case_insensitive_fix()
 * Returns the "fixed"-case virtual path, or NULL if there is no such file
 */
const char* case_insensitive_fix(const char* virtual_path)
{
    static char buffer[ASSET_PATH_MAX];

    ASSERT(strlen(virtual_path) < sizeof(buffer)); /* really?! */
    str_cpy(buffer, virtual_path, sizeof(buffer));

    if(0 == PHYSFSEXT_locateCorrectCase(buffer))
        return buffer;
    else
        return NULL;
}

/*
 * foreach_file()
 * Enumerate files with an optional extension filter and a callback
 */
bool foreach_file(ALLEGRO_PATH* path, const char* extension_filter, int (*callback)(const char* virtual_path, void* user_data), void* user_data, bool recursive)
{
    bool stop = false;
    const char* virtual_path = al_path_cstr(path, '/');
    char** list = PHYSFS_enumerateFiles(virtual_path); /* get sorted items without duplicates */

    /* for each entry */
    for(char** it = list; *it != NULL && !stop; it++) {
        PHYSFS_Stat stat;

        /* update path */
        al_set_path_filename(path, *it);
        virtual_path = al_path_cstr(path, '/');

        /* get information about the entry */
        if(!PHYSFS_stat(virtual_path, &stat))
            continue;

        /* found a directory */
        if(stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
            if(recursive) {
                al_set_path_filename(path, NULL);
                al_append_path_component(path, *it);
                stop = foreach_file(path, extension_filter, callback, user_data, recursive);
                al_remove_path_component(path, -1);
            }

            continue;
        }

        /* found an entry that is not a regular file */
        else if(stat.filetype != PHYSFS_FILETYPE_REGULAR)
            continue;

        /* does the extension filter match the name of the file? */
        if(extension_filter != NULL) {
            if(str_icmp(extension_filter, find_extension(*it)) != 0)
                continue;
        }

        /* invoke the callback */
        stop = (0 != callback(virtual_path, user_data));
    }

    /* cleanup */
    al_set_path_filename(path, NULL);
    PHYSFS_freeList(list);
    return stop;
}

/*
 * find_extension()
 * Find the extension of a path. If no extension exists, return an empty string.
 * Otherwise, return the extension - including the dot '.'
 */
const char* find_extension(const char* path)
{
    const char* ext = strrchr(path, '.');
    return ext != NULL ? ext : "";
}