/*
 * Open Surge Engine
 * asset.c - asset manager (virtual filesystem)
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
#include <allegro5/allegro_physfs.h>
#include <physfs.h>
#include <stdio.h>
#include <string.h>
#include "asset.h"
#include "mods.h"
#include "global.h"
#include "logfile.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../third_party/ignorecase.h"

/* The default directory of the game assets provided by upstream (*nix only) */
#ifndef GAME_DATADIR
#define GAME_DATADIR                    "/usr/share/games/" GAME_UNIXNAME
#endif

/* The default name of the user-modifiable asset directory */
#ifndef GAME_USERDIRNAME
#define GAME_USERDIRNAME                GAME_UNIXNAME
#endif

/* If RUNINPLACE is non-zero, then the assets will be read only from the directory of the executable */
#ifndef GAME_RUNINPLACE
#define GAME_RUNINPLACE                 0
#endif

/* Utility macros */
#define ENVIRONMENT_VARIABLE_NAME       "OPENSURGE_USER_PATH"
#define ASSET_PATH_MAX                  4096
#define PHYSFSx_getLastErrorMessage()   PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#define LOG(...)                        logfile_message("[asset] " __VA_ARGS__)
#define WARN(...)                       do { fprintf(stderr, "[asset] " __VA_ARGS__); fprintf(stderr, "\n"); LOG(__VA_ARGS__); } while(0)
#define CRASH(...)                      fatal_error("[asset] " __VA_ARGS__)

/* Name of the user-modifiable asset directory */
#define DEFAULT_USER_DATADIRNAME                    GAME_USERDIRNAME
#define DEFAULT_USER_DATADIRNAME_LENGTH             (sizeof(_DEFAULT_USER_DATADIRNAME) - 1)
#define GENERATED_USER_DATADIRNAME_PREFIX           DEFAULT_USER_DATADIRNAME "_" /* a constant, known string */
#define GENERATED_USER_DATADIRNAME_SUFFIX           "12345678" /* template for a 32-bit hash */
#define GENERATED_USER_DATADIRNAME_SUFFIX_LENGTH    (sizeof(_GENERATED_USER_DATADIRNAME_SUFFIX) - 1)
static const char _GENERATED_USER_DATADIRNAME_SUFFIX[] = GENERATED_USER_DATADIRNAME_SUFFIX;
static const char _GENERATED_USER_DATADIRNAME[] = GENERATED_USER_DATADIRNAME_PREFIX GENERATED_USER_DATADIRNAME_SUFFIX;
static const char _DEFAULT_USER_DATADIRNAME[] = DEFAULT_USER_DATADIRNAME;
static char user_datadirname[sizeof(_GENERATED_USER_DATADIRNAME)] = DEFAULT_USER_DATADIRNAME;
typedef char _32bit_hash_assert[ !!(GENERATED_USER_DATADIRNAME_SUFFIX_LENGTH == 8) * 2 - 1 ];
typedef char _default_user_datadirname_assert[ !!(sizeof(user_datadirname) > DEFAULT_USER_DATADIRNAME_LENGTH) * 2 - 1 ];
typedef char _user_datadirname_capacity_assert[ !!(sizeof(user_datadirname) == sizeof(_GENERATED_USER_DATADIRNAME)) * 2 - 1 ];

/* Utilities */
static char* gamedir = NULL; /* custom asset folder specified by the user */
static char required_engine_version[16] = "0.5.0";
static bool compatibility_mode = false;

static ALLEGRO_STATE state;
static ALLEGRO_PATH* find_exedir();
static ALLEGRO_PATH* find_homedir();
static ALLEGRO_PATH* find_shared_datadir();
static ALLEGRO_PATH* find_user_datadir(const char* dirname);

static void create_dir(const ALLEGRO_PATH* path);
static uint32_t get_fs_mode(const char* path);
static const char* find_extension(const char* path);
static const char* case_insensitive_fix(const char* virtual_path);
static bool foreach_file(ALLEGRO_PATH* dirpath, const char* extension_filter, int (*callback)(const char* virtual_path, void* user_data), void* user_data, bool recursive);
static bool clear_dir(ALLEGRO_FS_ENTRY* entry);
static uint32_t hash32(const char* str);
static bool is_valid_root_folder();

static void setup_compatibility_scripts(const char* shared_dirpath, const char* engine_version);
static void setup_compatibility_translations(const char* shared_dirpath);
static int scan_translations(const char* vpath, void* context);
static int append_translations(const char* vpath, void* context);
static size_t crlf_to_lf(uint8_t* data, size_t size);


/*
 * asset_init()
 * Initializes the asset manager.
 * Pass NULL to optional_gamedir to use the default search paths
 * compatibility_mode is only applicable when optional_gamedir != NULL
 */
void asset_init(const char* argv0, const char* optional_gamedir, bool want_compatibility_mode)
{
    /* already initialized? */
    if(asset_is_init())
        return;

    /* log */
    LOG("Initializing the asset manager...");

    /* save the I/O backend */
    al_store_state(&state, ALLEGRO_STATE_NEW_FILE_INTERFACE);

    /* initialize physfs */
#if !defined(__ANDROID__)
    if(!PHYSFS_init(argv0))
        CRASH("Can't initialize physfs. %s", PHYSFSx_getLastErrorMessage());
#else
    if(!PHYSFS_init(NULL))
        CRASH("Can't initialize physfs. %s", PHYSFSx_getLastErrorMessage());
#endif

    /* set the default name of the user-modifiable asset directory */
    str_cpy(user_datadirname, DEFAULT_USER_DATADIRNAME, sizeof(user_datadirname));

    /* compatibility mode */
    if(optional_gamedir == NULL)
        want_compatibility_mode = false;

    /* copy the gamedir, if specified */
    gamedir = (optional_gamedir != NULL) ? str_dup(optional_gamedir) : NULL;

    /* set the search path to the specified gamedir */
    if(gamedir != NULL) {

        uint32_t mode = get_fs_mode(gamedir);
        char* generated_user_datadir = NULL;

        /* log */
        LOG("Using a custom game directory: %s", gamedir);

        /* validate the gamedir */
        if(!(mode & ALLEGRO_FILEMODE_READ)) {

            /* gamedir isn't readable */
            CRASH("Can't use game directory %s. Make sure that it exists and that it is readable.", gamedir);

        }
        else if(!(mode & ALLEGRO_FILEMODE_WRITE)) {

            /* gamedir isn't writable... could this be flatpak?
               let's generate a write folder based on gamedir */
            snprintf(
                user_datadirname, sizeof(user_datadirname),
                "%s%0*x",
                GENERATED_USER_DATADIRNAME_PREFIX,
                (int)(GENERATED_USER_DATADIRNAME_SUFFIX_LENGTH),
                hash32(gamedir)
            );

            /* find the path to the writable folder and create it if necessary */
            ALLEGRO_PATH* user_datadir = find_user_datadir(user_datadirname);
            generated_user_datadir = str_dup(al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP));
            create_dir(user_datadir);
            al_destroy_path(user_datadir);

            /* log */
            LOG("The specified game directory isn't writable. Trying %s", generated_user_datadir);

#if 0
            /* check the permissions of the generated write folder */
            uint32_t gmode = get_fs_mode(generated_user_datadir);
            if(!(gmode & ALLEGRO_FILEMODE_WRITE)) /* for whatever reason this fails with a chmod 777 folder. Why? */
                CRASH("Can't use game directory %s because it's not writable. Write directory %s is also not writable. Check the permissions of your filesystem.", gamedir, generated_user_datadir);
            else if(!(gmode & ALLEGRO_FILEMODE_READ)) /* not needed. good to have */
                WARN("Can't use game directory %s because it's not writable. Write directory %s is writable but not readable. Check the permissions of your filesystem.", gamedir, generated_user_datadir);
#endif

        }

        /* set the write dir to gamedir if possible;
           otherwise set it to a generated directory */
        const char* writedir = generated_user_datadir != NULL ? generated_user_datadir : gamedir;
        if(!PHYSFS_setWriteDir(writedir))
            CRASH("Can't set the write directory to %s. Error: %s", writedir, PHYSFSx_getLastErrorMessage());
        LOG("Setting the write directory to %s", writedir);

        /* mount gamedir to the root */
        if(!PHYSFS_mount(gamedir, "/", 1))
            CRASH("Can't mount the game directory at %s. Error: %s", gamedir, PHYSFSx_getLastErrorMessage());
        LOG("Mounting gamedir: %s", gamedir);

        /* which engine version does this MOD require? */
        al_set_physfs_file_interface();
        guess_required_engine_version(required_engine_version, sizeof(required_engine_version));
        al_restore_state(&state);
        LOG("Required engine version of this MOD: %s", required_engine_version);
        {
            int sup, sub, wip;
            int mod_version = parse_version_number_ex(required_engine_version, &sup, &sub, &wip, NULL);
            int engine_version = parse_version_number(GAME_VERSION_STRING);

            if(game_version_compare(sup, sub, wip) < 0)
                CRASH("This MOD requires a newer version of the engine, %s. Please upgrade the engine or downgrade the MOD to version %s of the engine.", required_engine_version, GAME_VERSION_STRING);
            else if(engine_version < mod_version)
                LOG("This MOD requires a newer version of the engine, %s. We'll try to run it anyway. Engine version is %s.", required_engine_version, GAME_VERSION_STRING);
        }

        /* compatibility mode */
        compatibility_mode = want_compatibility_mode;
        if(compatibility_mode) {
            ALLEGRO_PATH* shared_datadir = find_shared_datadir();
            const char* dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP);

            /* log */
            LOG("=== Using compatibility mode for MODs ===");

            /* override scripts */
            al_set_physfs_file_interface();
            setup_compatibility_scripts(dirpath, required_engine_version);
            setup_compatibility_translations(dirpath);
            al_restore_state(&state);

            /* mount the default shared data directory with lower precedence */
            if(!PHYSFS_mount(dirpath, "/", 1))
                CRASH("Can't mount the shared data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
            LOG("Mounting shared data directory [compatibility mode]: %s", dirpath);

#if defined(__ANDROID__)
            /* on Android, read from the assets/ folder inside the .apk */
            PHYSFS_setRoot(dirpath, "/assets");
#endif

            al_destroy_path(shared_datadir); /* invalidates dirpath */
        }

        /* done */
        if(generated_user_datadir != NULL)
            free(generated_user_datadir);

    }

    /* set the default search paths */
    else {
        ALLEGRO_PATH* shared_datadir = find_shared_datadir();
        ALLEGRO_PATH* user_datadir = find_user_datadir(user_datadirname);
        const char* dirpath;

        /* create the user dir if it doesn't exist */
        create_dir(user_datadir);

        /* set the write dir */
        dirpath = al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP);
        if(!PHYSFS_setWriteDir(dirpath))
            CRASH("Can't set the write directory to %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
        LOG("Setting the write directory to %s", dirpath);

        /* mount the user path to the root (higher precedence) */
        /*dirpath = al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP);*/
        if(!PHYSFS_mount(dirpath, "/", 1))
            CRASH("Can't mount the user data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
        LOG("Mounting user data directory: %s", dirpath);

        /* mount the shared path to the root */
        dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP);
        if(!PHYSFS_mount(dirpath, "/", 1))
            CRASH("Can't mount the shared data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
        LOG("Mounting shared data directory: %s", dirpath);

        /* which engine version does this MOD require? */
        str_cpy(required_engine_version, GAME_VERSION_STRING, sizeof(required_engine_version));
        LOG("Required engine version of this MOD: %s", required_engine_version);

#if defined(__ANDROID__)
        /* on Android, read from the assets/ folder inside the .apk */
        dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP); /* redundant */
        PHYSFS_setRoot(dirpath, "/assets");
#endif

        /* done! */
        al_destroy_path(user_datadir);
        al_destroy_path(shared_datadir);
    }

    /* validate */
    if(!is_valid_root_folder()) {
        if(gamedir == NULL)
            CRASH("Not a valid Open Surge installation. Please reinstall the game.");
        else
            CRASH("Not a valid Open Surge game directory: %s", gamedir);
    }

    /* enable the physfs file interface. This should be the last task
       performed in this function (e.g., see create_dir()) */
    al_set_physfs_file_interface();

    /* done! */
    LOG("The asset manager has been initialized!");
}

/*
 * asset_release()
 * Releases the asset manager
 */
void asset_release()
{
    /* not initialized? */
    if(!asset_is_init())
        return;

    /* log */
    LOG("Releasing the asset manager...");

    /* release the gamedir string, if any */
    if(gamedir != NULL) {
        free(gamedir);
        gamedir = NULL;
    }

    /* reset the user_datadirname string, just in case */
    str_cpy(user_datadirname, DEFAULT_USER_DATADIRNAME, sizeof(user_datadirname));

    /* restore the previous I/O backend */
    al_restore_state(&state);

    /* deinit physfs */
    if(!PHYSFS_deinit())
        LOG("Can't deinitialize physfs. %s", PHYSFSx_getLastErrorMessage());

    /* done! */
    LOG("The asset manager has been released!");
}

/*
 * asset_is_init()
 * Checks if the asset subsystem is initialized
 */
bool asset_is_init()
{
    return PHYSFS_isInit();
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
    ALLEGRO_PATH* path = find_user_datadir(user_datadirname);
    const char* user_datadir = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);

    /* validity check */
    ALLEGRO_FS_ENTRY* entry = al_create_fs_entry(user_datadir);
    bool valid = al_fs_entry_exists(entry);
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
    if(gamedir != NULL && 0 != strcmp(user_datadirname, DEFAULT_USER_DATADIRNAME))
        return str_cpy(dest, gamedir, dest_size);

    /* uninitialized Allegro? */
    if(!al_is_system_installed())
        al_init();

    /* find the default path */
    ALLEGRO_PATH* path = find_user_datadir(user_datadirname);
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
 * asset_gamedir()
 * Custom asset folder specified by the user.
 * Returns NULL if no such folder has been specified
 */
const char* asset_gamedir()
{
    return gamedir;
}

/*
 * asset_guessed_engine_version()
 * Guessed engine version, based on the assets.
 * Returns a string of the x.y.z[.w] form.
 */
const char* asset_guessed_engine_version()
{
    return required_engine_version;
}

/*
 * asset_in_compatibility_mode()
 * Was the asset manager initialized in compatibility mode?
 */
bool asset_in_compatibility_mode()
{
    return compatibility_mode;
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
    assertx(path != NULL);

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
    assertx(path != NULL);

    return path;
}



/*
 * find_shared_datadir()
 * Finds the path of the directory storing the game assets provided by upstream
 * This function behaves as if no custom gamedir is specified
 */
ALLEGRO_PATH* find_shared_datadir()
{
#if (GAME_RUNINPLACE) || defined(_WIN32)

    return find_exedir();

#elif defined(__ANDROID__)

    /* on Android, treat the .apk itself as the shared datadir (it's a .zip file) */
    return al_get_standard_path(ALLEGRO_EXENAME_PATH);

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
 * This function behaves as if no custom gamedir is specified
 */
ALLEGRO_PATH* find_user_datadir(const char* dirname)
{
    /* Use custom user path? */
    const char* env = getenv(ENVIRONMENT_VARIABLE_NAME);
    if(env != NULL)
        return al_create_path_for_directory(env);

#if (GAME_RUNINPLACE) || defined(_WIN32)

    ALLEGRO_PATH* path = find_exedir();

    /* If a custom gamedir is specified and that directory is
       not writable, then a new write directory will be created.
       If the game runs in place, then that write directory will
       be a subdirectory of the folder of the executable.
       This situation should happen rarely. */
    if(0 != strcmp(dirname, DEFAULT_USER_DATADIRNAME)) {
        al_append_path_component(path, "__mods__");
        al_append_path_component(path, dirname);
    }

    (void)find_homedir;
    return path;

#elif defined(__ANDROID__)

    ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);

    /* ALLEGRO_USER_DATA_PATH relies on getFilesDir() of the Activity */
    /*al_append_path_component(path, dirname);*/
    (void)dirname;

    (void)find_homedir;
    return path;


#elif defined(__APPLE__) && defined(__MACH__)

    /*
    Return ~/Library/Application Support/opensurge/
    */

    ALLEGRO_PATH* path = find_homedir();

    al_append_path_component(path, "Library");
    al_append_path_component(path, "Application Support");
    al_append_path_component(path, dirname);

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

    al_append_path_component(path, dirname);
    return path;

#else
#error "Unsupported operating system."
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
 * get_fs_mode()
 * Returns the mode flags (ALLEGRO_FILE_MODE) of a filesystem entry
 */
uint32_t get_fs_mode(const char* path)
{
    ALLEGRO_FS_ENTRY* entry = al_create_fs_entry(gamedir);
    uint32_t mode = al_fs_entry_exists(entry) ? al_get_fs_entry_mode(entry) : 0;
    al_destroy_fs_entry(entry);

    return mode;
}

/*
 * case_insensitive_fix()
 * Returns the "fixed"-case virtual path, or NULL if there is no such file
 */
const char* case_insensitive_fix(const char* virtual_path)
{
    static char buffer[ASSET_PATH_MAX];

    assertx(strlen(virtual_path) < sizeof(buffer)); /* really?! */
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

    /* PHYSFS_enumerate() maintains a global mutex:
       https://github.com/icculus/physfs/issues/13

       PHYSFS_enumerateFiles() is based on the former, but just accumulates a
       list of strings. We do not load any files during enumeration, only after. */

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

/*
 * is_valid_root_folder()
 * Check if the root directory of the physfs filesystem is a valid Open Surge folder
 */
bool is_valid_root_folder()
{
    return PHYSFS_exists("surge.rocks") || PHYSFS_exists("surge.prefs") || PHYSFS_exists("surge.cfg") || PHYSFS_exists("languages/english.lng");
}

/*
 * hash32()
 * Jenkins' hash function
 */
uint32_t hash32(const char* str)
{
    const unsigned char* p = (const unsigned char*)str;
    uint32_t hash = 0;

    if(str == NULL)
        return 0;

    while(*p) {
        hash += *(p++);
        hash += hash << 10;
        hash ^= hash >> 6;
    }

    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    return hash;
}

/*
 * setup_compatibility_scripts()
 * Override scripts according to pre-defined compatibility rules
 */
void setup_compatibility_scripts(const char* shared_dirpath, const char* engine_version)
{
    assertx(PHYSFS_isInit());

    /* log */
    LOG("Will build a compatibility pack from %s", shared_dirpath);

    /* mount the default shared data directory with higher precedence */
    if(!PHYSFS_mount(shared_dirpath, "/", 0))
        CRASH("Can't mount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

#if defined(__ANDROID__)
    /* on Android, read from the assets/ folder inside the .apk */
    PHYSFS_setRoot(shared_dirpath, "/assets");
#endif

    /* create compatibility pack */
    void* pack_data = NULL;
    size_t pack_size = 0;
    if(!generate_compatibility_pack(engine_version, &pack_data, &pack_size))
        CRASH("Can't build a compatibility pack from %s", shared_dirpath);

    /* unmount the default shared data directory */
    if(!PHYSFS_unmount(shared_dirpath))
        CRASH("Can't unmount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

    /* mount the compatibility pack with higher precedence */
    if(!PHYSFS_mountMemory(pack_data, pack_size, release_pak_file, "compatibility.pak", "/", 0)) {
        release_pak_file(pack_data);
        CRASH("Can't mount the compatibility pack. Error: %s", PHYSFSx_getLastErrorMessage());
    }
}

/*
 * setup_compatibility_translations()
 * Generate compatibility translation files (.lng)
 */
void setup_compatibility_translations(const char* shared_dirpath)
{
    assertx(PHYSFS_isInit());

    /* log */
    LOG("Will build a language compatibility pack from %s", shared_dirpath);

    /* we'll store the language files in memory */
    int file_count = 0;
    char** file_vpath = NULL;
    uint8_t** file_data = NULL;
    size_t* file_size = NULL;
    void* file_context[] = { &file_count, &file_vpath, &file_data, &file_size };

    /* scan the language files of the gamedir */
    asset_foreach_file("languages/", ".lng", scan_translations, file_context, true);
    if(file_count == 0) {
        WARN("No language files were found!");
        return;
    }

    /* mount the default shared data directory with higher precedence */
    if(!PHYSFS_mount(shared_dirpath, "/", 0))
        CRASH("Can't mount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

    /* read the language files from the shared data directory */
    for(int file_index = 0; file_index < file_count; file_index++) {
        void* file_context[] = { &file_index, &file_vpath, &file_data, &file_size };
        append_translations(file_vpath[file_index], file_context);
    }

    /* unmount the default shared data directory */
    if(!PHYSFS_unmount(shared_dirpath))
        CRASH("Can't unmount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

    /* read the language files from the gamedir */
    for(int file_index = 0; file_index < file_count; file_index++) {
        void* file_context[] = { &file_index, &file_vpath, &file_data, &file_size };
        append_translations(file_vpath[file_index], file_context);
    }

    /* create a compatibility pack */
    void* pack_data = NULL;
    size_t pack_size = 0;
    if(!generate_pak_file_from_memory((const char**)file_vpath, file_count, (const void**)file_data, file_size, &pack_data, &pack_size))
        CRASH("Can't build a language compatibility pack from %s", shared_dirpath);

    /* release the language files (we already have a .pak) */
    for(int i = file_count - 1; i >= 0; i--) {
        if(file_data[i] != NULL)
            free(file_data[i]);
        if(file_vpath[i] != NULL)
            free(file_vpath[i]);
    }
    free(file_size);
    free(file_data);
    free(file_vpath);

    /* mount the compatibility pack with higher precedence */
    if(!PHYSFS_mountMemory(pack_data, pack_size, release_pak_file, "language_compatibility.pak", "/", 0)) {
        release_pak_file(pack_data);
        CRASH("Can't mount the language compatibility pack. Error: %s", PHYSFSx_getLastErrorMessage());
    }
}

/*
 * scan_translation()
 * This callback for asset_foreach_file() scans the .lng files
 */
int scan_translations(const char* vpath, void* context)
{
    void** file_context = (void**)context;
    int* file_count = file_context[0];
    char*** file_vpath = file_context[1];
    uint8_t*** file_data = file_context[2];
    size_t** file_size = file_context[3];

    int last = (*file_count)++;

    *file_vpath = reallocx(*file_vpath, (*file_count) * sizeof(char*));
    (*file_vpath)[last] = str_dup(vpath);

    *file_data = reallocx(*file_data, (*file_count) * sizeof(uint8_t*));
    (*file_data)[last] = NULL;

    *file_size = reallocx(*file_size, (*file_count) * sizeof(size_t));
    (*file_size)[last] = 0;

    return 0;
}

/*
 * append_translations()
 * Append a .lng file to a memory buffer
 */
int append_translations(const char* vpath, void* context)
{
    /* the glue */
    const uint8_t glue[] = "\n\n// [[ compatibility mode ]]\n\n";
    const int glue_length = sizeof(glue) - 1;

    /* the merged file stored in memory */
    void** file_context = (void**)context;
    int file_index = *((int*)(file_context[0]));
    char** file_vpath = *((char***)(file_context[1]));
    uint8_t** file_data = *((uint8_t***)(file_context[2]));
    size_t* file_size = *((size_t**)(file_context[3]));

    /* open the .lng file */
    ALLEGRO_FILE* fp = al_fopen(file_vpath[file_index], "rb");
    if(fp == NULL) {
        WARN("Can't open \"%s\" for reading!", file_vpath[file_index]);
        return 0;
    }

    /* find the size of the .lng file and reallocate the buffer */
    size_t lng_size = al_fsize(fp);
    file_data[file_index] = reallocx(file_data[file_index], file_size[file_index] + lng_size + glue_length);

    /* read to buffer */
    size_t read_bytes = al_fread(fp, file_data[file_index] + file_size[file_index], lng_size);
    if(lng_size != read_bytes) {
        WARN("Expected to read %lu bytes (instead of %lu) from \"%s\"",
            (unsigned long)lng_size, (unsigned long)read_bytes, file_vpath[file_index]);
    }
    else {
        /* the physfs file I/O only works in binary mode
           it's not strictly necessary to convert the translation files from CRLF to LF, but it's good to do... */
        lng_size = crlf_to_lf(file_data[file_index] + file_size[file_index], read_bytes);
        if(lng_size < read_bytes)
            LOG("Converted \"%s\" from CRLF to LF", file_vpath[file_index]);
    }

    /* append glue */
    memcpy(file_data[file_index] + file_size[file_index] + lng_size, glue, glue_length);

    /* increase file size */
    file_size[file_index] += lng_size + glue_length;

    /* done! */
    al_fclose(fp);
    return 0;
}

/*
 * crlf_to_lf()
 * Convert CRLF to LF in a memory buffer
 */
size_t crlf_to_lf(uint8_t* data, size_t size)
{
    size_t i, j;

    for(i = j = 0; i < size; i++) {
        if(data[i] == '\r' && i+1 < size && data[i+1] == '\n') {
            data[j++] = '\n';
            i++;
        }
        else
            data[j++] = data[i];
    }

    return j;
}