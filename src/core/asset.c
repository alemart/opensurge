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
#include <allegro5/allegro_memfile.h>
#include <physfs.h>
#include <stdio.h>
#include <string.h>
#include "asset.h"
#include "modutils.h"
#include "global.h"
#include "logfile.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../third_party/ignorecase.h"

/* require alloca */
#if !(defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh))
#include <malloc.h>
#if defined(__linux__) || defined(__linux) || defined(__EMSCRIPTEN__)
#include <alloca.h>
#endif
#endif

/* The default directory of the game assets provided by upstream (*nix only) */
#ifndef GAME_DATADIR
#define GAME_DATADIR                    "/usr/share/games/" GAME_UNIXNAME
#endif

/* The default name of the user-modifiable asset directory */
#ifndef GAME_USERDIRNAME
#define GAME_USERDIRNAME                GAME_UNIXNAME
#endif

/* If GAME_RUNINPLACE is non-zero, then the assets will be read from the directory of the executable */
#ifndef GAME_RUNINPLACE
#define GAME_RUNINPLACE                 0
#endif

/* GAME_RUNINPLACE is not supported on Android */
#if (GAME_RUNINPLACE) && defined(__ANDROID__)
#error GAME_RUNINPLACE is not supported on this platform
#endif

/* Utility macros */
#define ENVIRONMENT_VARIABLE_NAME       "OPENSURGE_USER_PATH"
#define ASSET_PATH_MAX                  4096
#define PHYSFSx_getLastErrorMessage()   PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode())
#define LOG(...)                        logfile_message("[asset] " __VA_ARGS__)
#define WARN(...)                       do { fprintf(stderr, "[asset] " __VA_ARGS__); fprintf(stderr, "\n"); LOG(__VA_ARGS__); } while(0)
#define CRASH(...)                      fatal_error("[asset] " __VA_ARGS__)

/* Name of the user-modifiable asset sub-directory */
#define DEFAULT_GAME_NAME               "Surge the Rabbit"
#define DEFAULT_USER_DATADIRNAME        DEFAULT_GAME_NAME
static char user_datadirname[256] = DEFAULT_USER_DATADIRNAME;

/* Utilities */
#define DEFAULT_COMPATIBILITY_VERSION_CODE VERSION_CODE_EX(GAME_VERSION_SUP, GAME_VERSION_SUB, GAME_VERSION_WIP, GAME_VERSION_FIX)
static char* gamedir = NULL; /* custom asset folder specified by the user */

static ALLEGRO_STATE state;
static ALLEGRO_PATH* find_exedir();
static ALLEGRO_PATH* find_homedir();
static ALLEGRO_PATH* find_shared_datadir();
static ALLEGRO_PATH* find_user_datadir(const char* dirname);

static void create_dir(const ALLEGRO_PATH* path);
static uint32_t get_fs_mode(const char* path);
static const char* find_extension(const char* path);
static char* find_gamedirname(const char* gamedir, char* buffer, size_t buffer_size);
static const char* case_insensitive_fix(const char* virtual_path);
static bool foreach_file(ALLEGRO_PATH* dirpath, const char* extension_filter, int (*callback)(const char* virtual_path, void* user_data), void* user_data, bool recursive);
static bool clear_dir(ALLEGRO_FS_ENTRY* entry);
static char* generate_user_datadirname(const char* game_name, uint32_t game_id, char* buffer, size_t buffer_size);
static bool is_valid_root_folder();
static char* find_root_directory(const char* mount_point, char* buffer, size_t buffer_size);

static bool is_uncompressed_gamedir(const char* fullpath);
static bool is_compressed_gamedir(const char* fullpath);
static bool is_gamedir(const char* root, bool (*file_exists)(const char*,void*), void* context);
static bool actual_file_exists(const char* filepath, void* context);
static bool virtual_file_exists(const char* filepath, void* context);

static void setup_compatibility_pack(const char* shared_dirpath, const char* engine_version, uint32_t game_id, const char* guessed_game_title);
static int scan_translations(const char* vpath, void* context);
static int append_translations(const char* vpath, void* context);
static size_t crlf_to_lf(uint8_t* data, size_t size);

static bool has_pak_support();
bool generate_pak_file(const char** vpath, int file_count, const void** file_data, const size_t* file_size, void** out_pak_data, size_t* out_pak_size);
void release_pak_file(void* pak);

bool read_file(const char* vpath, void** out_file_data, size_t* out_file_size);
bool write_file(const char* vpath, void* file_data, size_t file_size);


/*
 * asset_init()
 * Initializes the asset manager.
 * Pass NULL to optional_gamedir to use the default search paths
 * compatibility_version is only applicable when optional_gamedir != NULL and may be set to:
 * - NULL to disable compatibility mode;
 * - an empty string "" to enable compatibility with an automatically picked version of the engine;
 * - a version string "x.y.z" to indicate a *preference* for compatibility with that version of the engine.
 */
void asset_init(const char* argv0, const char* optional_gamedir, const char* compatibility_version, uint32_t* out_game_id, int* out_compatibility_version_code)
{
    uint32_t game_id = 0;
    int compatibility_version_code = DEFAULT_COMPATIBILITY_VERSION_CODE;

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

    /* set the default name of the user-modifiable asset sub-directory */
    str_cpy(user_datadirname, DEFAULT_USER_DATADIRNAME, sizeof(user_datadirname));

    /* copy the gamedir, if specified */
    gamedir = NULL;
    if(optional_gamedir != NULL && *optional_gamedir != '\0')
        gamedir = str_dup(optional_gamedir);

    /* set the search path to the specified gamedir */
    if(gamedir != NULL) {

        char game_dirname[256] = "";
        char required_engine_version[16] = "0.5.0";
        char* generated_user_datadir = NULL;
#if !defined(__ANDROID__)
        uint32_t mode = get_fs_mode(gamedir);
#else
        uint32_t mode = ALLEGRO_FILEMODE_READ | ALLEGRO_FILEMODE_ISFILE;
        (void)get_fs_mode;
#endif

        /* log */
        LOG("Using a custom game directory: %s", gamedir);

        /* validate the gamedir */
        if(!(mode & ALLEGRO_FILEMODE_READ)) {

            /* gamedir isn't readable */
            CRASH("Can't use game directory %s. Make sure that it exists and that it is readable.", gamedir);

        }

        /* Get the name of the folder of the game */
        find_gamedirname(gamedir, game_dirname, sizeof(game_dirname));

        /* mount gamedir to the root */
        if(!PHYSFS_mount(gamedir, "/", 1))
            CRASH("Can't mount the game directory at %s. Error: %s", gamedir, PHYSFSx_getLastErrorMessage());
        LOG("Mounting gamedir: %s", gamedir);

        /* if gamedir is a compressed archive, do we need to change the root? */
        if(!(mode & ALLEGRO_FILEMODE_ISDIR)) {
            char real_root[256];
            find_root_directory("/", real_root, sizeof(real_root));

            LOG("Detected root: %s", real_root);
            if(0 != strcmp("/", real_root)) {
                /* change the root */
#if VERSION_CODE(PHYSFS_VER_MAJOR, PHYSFS_VER_MINOR, PHYSFS_VER_PATCH) >= VERSION_CODE(3,2,0)
                PHYSFS_setRoot(gamedir, real_root);
#else
                CRASH("Please extract the game archive or, to use it as is, upgrade physfs to version 3.2.0 and recompile the engine (this build uses outdated version %d.%d.%d).", PHYSFS_VER_MAJOR, PHYSFS_VER_MINOR, PHYSFS_VER_PATCH);
#endif
                /* Get the name of the folder of the game again */
                find_gamedirname(real_root, game_dirname, sizeof(game_dirname));
            }
        }

        /* validate asset folder */
        if(!is_valid_root_folder())
            CRASH("Not a valid Open Surge game directory: %s", gamedir);

        /* which engine version does this MOD require? */
        al_set_physfs_file_interface();
        guess_engine_version_of_mod(required_engine_version, sizeof(required_engine_version));
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
            else if(mod_version < VERSION_CODE(0,5,0))
                LOG("Legacy games are unsupported. Detected version: %s", required_engine_version);
        }

        /* find the game ID */
        game_id = find_game_id(NULL, game_dirname, required_engine_version);
        LOG("Game ID: %08x", game_id);

        /* set the write dir to gamedir if possible;
           otherwise set it to a generated directory */
        const char* writedir = gamedir;
        if(!(mode & ALLEGRO_FILEMODE_ISDIR) || !PHYSFS_setWriteDir(writedir)) {
            /* log */
            if(mode & ALLEGRO_FILEMODE_ISDIR)
                LOG("Can't set the write directory to %s. Error: %s", writedir, PHYSFSx_getLastErrorMessage());

            /* gamedir either isn't writable or isn't a folder...
               could this be flatpak? or a compressed archive?
               let's generate a write folder based on gamedir */
            const char* game_name = game_dirname;
            generate_user_datadirname(game_name, game_id, user_datadirname, sizeof(user_datadirname));

            /* find the path to the writable folder and create it if necessary */
            ALLEGRO_PATH* user_datadir = find_user_datadir(user_datadirname);
            generated_user_datadir = str_dup(al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP));
            create_dir(user_datadir);
            al_destroy_path(user_datadir);

            /* try again */
            writedir = generated_user_datadir;
            if(!PHYSFS_setWriteDir(writedir))
                CRASH("Can't set the write directory to %s. Error: %s", writedir, PHYSFSx_getLastErrorMessage());
        }
        else {
            /* the writable user directory is now gamedir */
            user_datadirname[0] = '\0';
        }
        LOG("Setting the write directory to %s", writedir);

        /* compatibility mode */
        if(compatibility_version != NULL) {
            char buffer[16];
            LOG("Using compatibility mode for MODs");

            /* validate compatibility version */
            if(*compatibility_version != '\0') {
                /* manually set compatibility version */
                LOG("Manually set compatibility version: %s", compatibility_version);
                int version_code = parse_version_number(compatibility_version);
                int min_version = parse_version_number(required_engine_version);
                int max_version = parse_version_number(GAME_VERSION_STRING);

                if(version_code < min_version) {
                    compatibility_version = required_engine_version;
                    LOG("Adjusting the compatibility version to %s", compatibility_version);
                }
                else if(version_code > max_version) {
                    LOG("Can't set the compatibility version to %s", compatibility_version);
                    compatibility_version = stringify_version_number(max_version, buffer, sizeof(buffer));
                    LOG("Adjusting the compatibility version to %s", compatibility_version);
                }
            }
            else {
                /* automatically set compatibility version */
                compatibility_version = required_engine_version;
                LOG("Automatically set compatibility version: %s", compatibility_version);
            }

            /* store the compatibility version code */
            compatibility_version_code = parse_version_number(compatibility_version);

            /* find the directory of the base game */
            ALLEGRO_PATH* shared_datadir = find_shared_datadir();
            const char* dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP);

            /* override scripts */
            al_set_physfs_file_interface();
            setup_compatibility_pack(dirpath, compatibility_version, game_id, game_dirname);
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
        else {
            /* not in compatibility mode */
            compatibility_version_code = DEFAULT_COMPATIBILITY_VERSION_CODE;
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

        /* not in compatibility mode */
        compatibility_version_code = DEFAULT_COMPATIBILITY_VERSION_CODE;

        /* find the game ID */
        game_id = find_game_id(NULL, NULL, GAME_VERSION_STRING);
        LOG("Game ID: %08x", game_id);

        /* create the user dir if it doesn't exist */
        create_dir(user_datadir);

        /* set the write dir */
        dirpath = al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP);
        if(!PHYSFS_setWriteDir(dirpath))
            CRASH("Can't set the write directory to %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
        LOG("Setting the write directory to %s", dirpath);

        /* mount the user path to the root (higher precedence) */
        dirpath = al_path_cstr(user_datadir, ALLEGRO_NATIVE_PATH_SEP); /* redundant */
        if(!PHYSFS_mount(dirpath, "/", 0))
            CRASH("Can't mount the user data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
        LOG("Mounting user data directory: %s", dirpath);

        /* mount the shared path to the root */
        dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP);
        if(!PHYSFS_mount(dirpath, "/", 1))
            CRASH("Can't mount the shared data directory at %s. Error: %s", dirpath, PHYSFSx_getLastErrorMessage());
        LOG("Mounting shared data directory: %s", dirpath);

#if defined(__ANDROID__)
        /* on Android, read from the assets/ folder inside the .apk */
        dirpath = al_path_cstr(shared_datadir, ALLEGRO_NATIVE_PATH_SEP); /* redundant */
        PHYSFS_setRoot(dirpath, "/assets");
#endif

        /* validate asset folder */
        if(!is_valid_root_folder())
            CRASH("Not a valid Open Surge installation. Please reinstall the game.");

        /* done! */
        al_destroy_path(user_datadir);
        al_destroy_path(shared_datadir);
    }

    /* set the game ID */
    if(out_game_id != NULL)
        *out_game_id = game_id;

    /* set the compatibility version code */
    if(out_compatibility_version_code != NULL)
        *out_compatibility_version_code = compatibility_version_code;

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

    /* fail if a custom gamedir was specified and it is the write folder */
    if(gamedir != NULL && user_datadirname[0] == '\0') {
        WARN("Unsupported operation when a custom gamedir is specified and it is the user-writable folder");
        return false;
    }

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
    if(gamedir != NULL && user_datadirname[0] == '\0')
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
 * asset_shared_datadir()
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
 * asset_is_gamedir()
 * Checks if a folder or compressed archive stores an opensurge game
 * Both allegro and physfs must be initialized before calling this
 */
bool asset_is_gamedir(const char* fullpath)
{
    bool ret = false;

    assertx(al_is_system_installed());
    assertx(PHYSFS_isInit());
    assertx(fullpath);

    ALLEGRO_FS_ENTRY* e = al_create_fs_entry(fullpath);
    uint32_t mode = al_get_fs_entry_mode(e);

    if(mode & ALLEGRO_FILEMODE_READ) {
        if(mode & ALLEGRO_FILEMODE_ISDIR)
            ret = is_uncompressed_gamedir(fullpath);
#if !defined(__ANDROID__)
        else if(mode & ALLEGRO_FILEMODE_ISFILE)
#else
        /* the ISFILE flag doesn't work on Android, why? */
        else
#endif
            ret = is_compressed_gamedir(fullpath);
    }

    al_destroy_fs_entry(e);
    return ret;
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
    /* note: the assets/ subfolder will be defined as the root on physfs */

    (void)find_exedir;
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

    /* validate */
    assertx(*dirname != '\0');

#if (GAME_RUNINPLACE) || defined(_WIN32)

    ALLEGRO_PATH* path = find_exedir();

    /* If a custom gamedir is specified and that directory is
       not writable, then a new write directory will be created.
       If the game runs in place, then that write directory will
       be a subdirectory of the folder of the executable. */
    if(0 != strcmp(dirname, DEFAULT_USER_DATADIRNAME)) {
        al_append_path_component(path, "__user__");
        al_append_path_component(path, dirname);
    }

    (void)find_homedir;
    return path;

#elif defined(__ANDROID__)

    /* ALLEGRO_USER_DATA_PATH relies on getFilesDir() of the Activity */
    ALLEGRO_PATH* path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
    al_append_path_component(path, dirname);

    (void)find_homedir;
    return path;


#elif defined(__APPLE__) && defined(__MACH__)

    /*
    Return ~/Library/Application Support/opensurge/<dirname>
    */

    ALLEGRO_PATH* path = find_homedir();

    al_append_path_component(path, "Library");
    al_append_path_component(path, "Application Support");
    al_append_path_component(path, GAME_USERDIRNAME);
    al_append_path_component(path, dirname);

    return path;

#elif defined(__unix__) || defined(__unix)

    /*
    Return $XDG_DATA_HOME/opensurge/<dirname>
    or ~/.local/share/opensurge/<dirname>
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
    assertx(list != NULL);

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
 * find_gamedirname()
 * Find the name of the game folder, given its absolute path
 */
char* find_gamedirname(const char* gamedir, char* buffer, size_t buffer_size)
{
    char* tmp = str_dup(gamedir);

    /* remove trailing slashes, if any */
    int length = strlen(tmp);
    while(length > 0 && (tmp[length-1] == '/' || tmp[length-1] == '\\'))
        tmp[--length] = '\0';

    /* remove the extension, if any
       (gamedir may be a .zip archive) */
    str_basename_without_extension(tmp, buffer, buffer_size);

    /* done! */
    free(tmp);
    return buffer;
}

/*
 * is_valid_root_folder()
 * Check if the root directory of the physfs filesystem is a valid Open Surge folder
 */
bool is_valid_root_folder()
{
    return is_gamedir("/", virtual_file_exists, NULL);
}

/*
 * generate_user_datadirname()
 * Generate the name of a write sub-directory
 */
char* generate_user_datadirname(const char* game_name, uint32_t game_id, char* buffer, size_t buffer_size)
{
    /* try using the name of the game first */
    str_cpy(buffer, game_name, buffer_size);

#if 0
    /* the generated directory name should not be DEFAULT_USER_DATADIRNAME when
       using a custom gamedir. If it is, use the game id. This should not happen. */
    if(0 == strcmp(buffer, DEFAULT_USER_DATADIRNAME))
        snprintf(buffer, buffer_size, "%08x", game_id);
#else
    (void)game_id;
#endif

    /* done! */
    return buffer;
}

/*
 * find_root_directory()
 * Find the real root directory at a mount point
 * (e.g., "/" may have a single directory - that would be the root)
 */
char* find_root_directory(const char* mount_point, char* buffer, size_t buffer_size)
{
    if(buffer_size == 0)
        return buffer;

    PHYSFS_Stat stat;
    int dircount = 0;
    const char* dirname = NULL;
    char** list = PHYSFS_enumerateFiles(mount_point);
    char path[256];
    assertx(list != NULL);

    if(mount_point == NULL || *mount_point == '\0')
        mount_point = "/";
    bool has_trailing_slash = (mount_point[strlen(mount_point)-1] == '/');

    for(char* const* it = list; *it != NULL; it++) {
        snprintf(path, sizeof(path), (!has_trailing_slash ? "%s/%s" : "%s%s"), mount_point, *it);
        if(PHYSFS_stat(path, &stat) && stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
            dirname = *it;
            dircount++;
        }
    }

    str_cpy(buffer, "/", buffer_size);
    if(dircount == 1 && buffer_size > 1)
        str_cpy(buffer+1, dirname, buffer_size-1);

    PHYSFS_freeList(list);
    return buffer;
}






/*
 *
 * validate gamedir
 *
 */

/*
 * is_uncompressed_gamedir()
 * Checks if a folder is a valid opensurge game
 */
bool is_uncompressed_gamedir(const char* fullpath)
{
    return is_gamedir(fullpath, actual_file_exists, NULL);
}

/*
 * is_compressed_gamedir()
 * Checks if a compressed archive stores a valid opensurge game
 * The compressed archive must be of a type supported by physfs
 */
bool is_compressed_gamedir(const char* fullpath)
{
    const char PREFIX[] = "/__validate__";
    const size_t PREFIX_SIZE = sizeof(PREFIX) - 1;
    char root[256] = "";
    bool ret = false;

    assertx(PHYSFS_isInit());

    if(!PHYSFS_mount(fullpath, PREFIX, 0)) {
        const char* err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        logfile_message("%s: can't mount %s. %s", __func__, fullpath, err);
        return ret;
    }

    strcpy(root, PREFIX);
    find_root_directory(PREFIX, root + PREFIX_SIZE, sizeof(root) - PREFIX_SIZE);
    LOG("%s: testing %s", __func__, root + PREFIX_SIZE);

    ret = is_gamedir(root, virtual_file_exists, NULL);

    if(!PHYSFS_unmount(fullpath)) {
        const char* err = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        logfile_message("%s: can't unmount %s. %s", __func__, fullpath, err);
    }

    return ret;
}

/*
 * is_gamedir()
 * A helper to check if a generic root folder stores an opensurge game
 */
bool is_gamedir(const char* root, bool (*file_exists)(const char*,void*), void* context)
{
    const char* file_list[] = {
        "surge.rocks",
        "surge.prefs",
        "surge.cfg",
        "languages/english.lng",
        NULL
    };

    ALLEGRO_PATH* base_path = al_create_path_for_directory(root);
    bool valid_gamedir = false;

    /* for each file in file_list, check if it exists relative to the root (absolute path) */
    for(const char** vpath = file_list; *vpath != NULL && !valid_gamedir; vpath++) {
        ALLEGRO_PATH* path = al_clone_path(base_path);
        ALLEGRO_PATH* tail = al_create_path(*vpath);

        if(al_join_paths(path, tail)) {
            const char* fullpath = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);
            if(file_exists(fullpath, context))
                valid_gamedir = true;
        }

        al_destroy_path(tail);
        al_destroy_path(path);
    }

    al_destroy_path(base_path);
    return valid_gamedir;
}

/*
 * actual_file_exists()
 * A helper to is_gamedir() that checks if a file path exists in the filesystem
 */
bool actual_file_exists(const char* filepath, void* context)
{
    (void)context;

    return file_exists(filepath);
}

/*
 * virtual_file_exists()
 * A helper to is_gamedir() that checks if a file path exists in the
 * mounted filesystem of physfs
 */
bool virtual_file_exists(const char* filepath, void* context)
{
    (void)context;

    return PHYSFS_exists(filepath);
}





/*
 *
 * compatibility packs
 *
 */


/*
 * setup_compatibility_pack()
 * Generates and mounts the compatibility pack, which overrides
 * scripts and translations according to pre-defined rules based
 * on a compatibility version string. Ensure that the physfs I/O
 * interface (Allegro) is activated before calling this function.
 */
void setup_compatibility_pack(const char* shared_dirpath, const char* engine_version, uint32_t game_id, const char* guessed_game_title)
{
    /* we'll read files to memory */
    int file_count = 0;
    char** file_vpath = NULL;
    uint8_t** file_data = NULL;
    size_t* file_size = NULL;

    /* log */
    LOG("Will build a compatibility pack from %s", shared_dirpath);
    assertx(PHYSFS_isInit());

    /* validate */
    if(!has_pak_support())
        CRASH("Compatibility mode is not available because PhysFS has been compiled without PAK support.");



    /*
     * ----------------------
     * UPDATE TRANSLATIONS
     * ----------------------
     */

    /* scan the language files of the gamedir */
    void* file_context[] = { &file_count, &file_vpath, &file_data, &file_size };
    asset_foreach_file("languages/", ".lng", scan_translations, file_context, true);
    if(file_count == 0)
        CRASH("No language files were found!");

    /* mount the default shared data directory with higher precedence */
    if(!PHYSFS_mount(shared_dirpath, "/", 0))
        CRASH("Can't mount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

#if defined(__ANDROID__)
    /* on Android, read from the assets/ folder inside the .apk */
    PHYSFS_setRoot(shared_dirpath, "/assets");
#endif

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

    /* TODO: diff & patch? */




    /*
     * ----------------------
     * PICK SCRIPTS & MISC
     * ----------------------
     */

    /* mount the default shared data directory with higher precedence */
    if(!PHYSFS_mount(shared_dirpath, "/", 0))
        CRASH("Can't mount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

#if defined(__ANDROID__)
    /* on Android, read from the assets/ folder inside the .apk */
    PHYSFS_setRoot(shared_dirpath, "/assets");
#endif

    /* select & read scripts of the shared data directory for compatibility */
    const char** file_list = select_files_for_compatibility_pack(engine_version, game_id);
    for(const char** vpath = file_list; *vpath != NULL; vpath++) {
        int last = file_count++;

        file_vpath = reallocx(file_vpath, file_count * sizeof(*file_vpath));
        file_vpath[last] = str_dup(*vpath);

        file_data = reallocx(file_data, file_count * sizeof(*file_data));
        file_data[last] = NULL;

        file_size = reallocx(file_size, file_count * sizeof(*file_size));
        file_size[last] = 0;

        if(file_vpath[last][0] == '-') {
            LOG("Will ignore file \"%s\"...", file_vpath[last] + 1);
            free(file_vpath[last]);

            /* make the file blank, effectively removing it from the tree */
            file_vpath[last] = str_dup(*vpath + 1);
            file_data[last] = NULL;
            file_size[last] = 0;
        }
        else if(!read_file(file_vpath[last], (void**)&file_data[last], &file_size[last])) {
            WARN("Can't add file \"%s\" to the compatibility pack!", file_vpath[last]);

            /* the file probably no longer exists. Make it blank,
               effectively removing it from the tree */
            WARN("Will make \"%s\" an empty file", file_vpath[last]);
            file_data[last] = NULL;
            file_size[last] = 0;
        }
        else
            LOG("Added file \"%s\" to the compatibility pack", file_vpath[last]);
    }

    /* unmount the default shared data directory */
    if(!PHYSFS_unmount(shared_dirpath))
        CRASH("Can't unmount the shared data directory at %s. Error: %s", shared_dirpath, PHYSFSx_getLastErrorMessage());

    /* add a default surge.cfg if that file doesn't exist in the game */
    if(!PHYSFS_exists("surge.cfg")) {
        uint8_t* surge_cfg_data = NULL;
        size_t surge_cfg_size = 0;

        if(generate_surge_cfg(guessed_game_title, (void**)&surge_cfg_data, &surge_cfg_size)) {
            int last = file_count++;

            file_vpath = reallocx(file_vpath, file_count * sizeof(*file_vpath));
            file_vpath[last] = str_dup("surge.cfg");

            file_data = reallocx(file_data, file_count * sizeof(*file_data));
            file_data[last] = surge_cfg_data;

            file_size = reallocx(file_size, file_count * sizeof(*file_size));
            file_size[last] = surge_cfg_size;

            LOG("Added a default \"surge.cfg\" to the compatibility pack");
        }
        else
            WARN("Can't add a default \"surge.cfg\" to the compatibility pack");
    }



    /*
     * ----------------------
     * GENERATE PACKAGE
     * ----------------------
     */

    /* create a compatibility pack */
    void* pack_data = NULL;
    size_t pack_size = 0;
    if(!generate_pak_file((const char**)file_vpath, file_count, (const void**)file_data, file_size, &pack_data, &pack_size))
        CRASH("Can't build a compatibility pack from %s", shared_dirpath);

    /* mount the compatibility pack with higher precedence */
    if(!PHYSFS_mountMemory(pack_data, pack_size, release_pak_file, "compatibility.pak", "/", 0)) {
        release_pak_file(pack_data);
        CRASH("Can't mount the compatibility pack. Error: %s", PHYSFSx_getLastErrorMessage());
    }

    /* write the compatibility pack to secondary storage
       (for debugging purposes) */
    if(!write_file("compatibility.pak", pack_data, pack_size))
        WARN("Can't write the compatibility pack to the disk!");

    /* cleanup */
    if(file_size != NULL)
        free(file_size);

    if(file_data != NULL) {
        for(int i = file_count - 1; i >= 0; i--) {
            if(file_data[i] != NULL)
                free(file_data[i]);
        }
        free(file_data);
    }

    if(file_vpath != NULL) {
        for(int i = file_count - 1; i >= 0; i--) {
            if(file_vpath[i] != NULL)
                free(file_vpath[i]);
        }
        free(file_vpath);
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
 * Return the new size of the buffer, which is less than or equal to the old size
 */
size_t crlf_to_lf(uint8_t* data, size_t size)
{
    size_t i, j;

    for(i = j = 0; i < size; i++) {
        if(data[i] == '\r' && i+1 < size && data[i+1] == '\n')
            data[j++] = data[++i]; /* data[j++] := '\n' */
        else
            data[j++] = data[i];
    }

    for(i = j; i < size; i++)
        data[i] = '\0';

    return j;
}





/*
 *
 * .pak files
 *
 */

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

/*
 * generate_pak_file()
 * Generate a .pak archive with files stored in memory.
 * Call release_pak_file() on the output data after usage.
 */
bool generate_pak_file(const char** vpath, int file_count, const void** file_data, const size_t* file_size, void** out_pak_data, size_t* out_pak_size)
{
    uint8_t* pack_data = NULL;

    /* validation */
    if(vpath == NULL || file_count == 0) {
        WARN("No files have been added to the compatibility pack!");
        goto error;
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
    pack_data = mallocx(pack_size);

    /* open the pack file for writing */
    ALLEGRO_FILE* packf = al_open_memfile(pack_data, pack_size, "wb");
    if(NULL == packf) {
        LOG("Can't open the compatibility pack file for writing!");
        goto error;
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
    for(int i = 0; i < file_count; i++) {
        size_t n = al_fwrite(packf, file_data[i], file_size[i]);
        if(n < file_size[i]) {
            WARN("Can't add file \"%s\" to the compatibility pack!", vpath[i]);
            goto error;
        }
    }

    /* ----- */

    /* close the pack file */
    al_fclose(packf);

    /* done! */
    *out_pak_data = (void*)pack_data;
    *out_pak_size = pack_size;
    return true;

    /* error */
    error:

    if(pack_data != NULL)
        free(pack_data);

    *out_pak_data = NULL;
    *out_pak_size = 0;
    return false;
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
 * general read & write utilities
 *
 */

/*
 * read_file()
 * Read a file to memory. Returns true on success.
 * You must free() the output void* pointer on success.
 */
bool read_file(const char* vpath, void** out_file_data, size_t* out_file_size)
{
    void* buffer = NULL;
    ALLEGRO_FILE* fp = NULL;

    /* does the file exist? */
    if(PHYSFS_isInit() && !PHYSFS_exists(vpath)) {
        WARN("File \"%s\" doesn't exist", vpath);
        goto error;
    }

    /* open the file */
    if(NULL == (fp = al_fopen(vpath, "rb"))) {
        WARN("Can't open file \"%s\" for reading", vpath);
        goto error;
    }

    /* find its size */
    int64_t size = al_fsize(fp);
    if(size < 0) {
        WARN("Can't determine the size of file \"%s\"", vpath);
        goto error;
    }

    /* allocate a buffer */
    buffer = mallocx(size);

    /* read the file */
    size_t n = al_fread(fp, buffer, size);
    if(n < size) {
        WARN("Can't successfully read file \"%s\". Read %lu bytes, but expected %lu.", vpath, (unsigned long)n, (unsigned long)size);
        goto error;
    }

    /* close the file */
    al_fclose(fp);

    /* success! */
    *out_file_data = buffer;
    *out_file_size = size;
    return true;

    /* error */
    error:

    if(buffer != NULL)
        free(buffer);

     if(fp != NULL)
        al_fclose(fp);

    *out_file_data = NULL;
    *out_file_size = 0;
    return false;
}

/*
 * write_file()
 * Write a memory buffer to a file. Returns true on success.
 */
bool write_file(const char* vpath, void* file_data, size_t file_size)
{
    ALLEGRO_FILE* fp = NULL;

    /* open the file */
    if(NULL == (fp = al_fopen(vpath, "wb"))) {
        WARN("Can't open file \"%s\" for writing", vpath);
        goto error;
    }

    /* write the data */
    size_t n = al_fwrite(fp, file_data, file_size);
    if(n < file_size) {
        WARN("Can't successfully write file \"%s\". Wrote %lu bytes out of a total of %lu.", vpath, (unsigned long)n, (unsigned long)file_size);
        goto error;
    }

    /* close the file */
    al_fclose(fp);

    /* success! */
    return true;

    /* error */
    error:

    if(fp != NULL)
        al_fclose(fp);

    return false;
}
