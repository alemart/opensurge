/*
 * Open Surge Engine
 * import.c - a utility to import Open Surge games into this version of the engine
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
#include <allegro5/allegro_native_dialog.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "import.h"
#include "global.h"
#include "stringutil.h"
#include "asset.h"

#define ALERT(...)      message_box(0, __VA_ARGS__)
#define WARN(...)       message_box(ALLEGRO_MESSAGEBOX_WARN, __VA_ARGS__)
#define ERROR(...)      message_box(ALLEGRO_MESSAGEBOX_ERROR, __VA_ARGS__)
#define CONFIRM(...)    message_box(ALLEGRO_MESSAGEBOX_QUESTION | ALLEGRO_MESSAGEBOX_YES_NO, __VA_ARGS__)
#define YES             1

#define PRINT(...)      do { \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n"); \
    if(textlog != NULL) { \
        al_append_native_text_log(textlog, __VA_ARGS__); \
        al_append_native_text_log(textlog, "\n"); \
    } \
} while(0)

static const char TITLE_WIZARD[] = "Open Surge Import Wizard";
static const char UNAVAILABLE_ERROR[] = "The import utility isn't available in this platform.";
static const char BACKUP_MESSAGE[] = "\"I declare that I made a backup of my game. My backup is stored safely and I can access it now and in the future.\"";
static const char COMPLETED_MESSAGE[] = "" \
    "Your game is now in sync with version " GAME_VERSION_STRING " of the engine.\n"
    "\n" \
    "It's possible that you'll see some of your changes missing. If this happens, you'll have to adjust a few things.\n"
    "\n" \
    "As a rule of thumb, KEEP YOUR ASSETS SEPARATE FROM THOSE OF THE BASE GAME.\n" \
    "\n" \
    "If you've been following the above rule of thumb, keeping your game in sync with the latest versions of the engine will be straightforward. If not, this is a good time to fix things:\n" \
    "\n" \
    "- Do not modify the images of the base game. If you want to change them, create new images and replace the sprites using the sprites/overrides/ folder.\n"
    "\n" \
    "- Do not modify the texts of the base game. If you want to change them, store your texts in the languages/extends/ folder.\n"
    "\n" \
    "- Do not modify the SurgeScript objects of the base game. If you want changed functionality, clone the objects and change their name.\n"
    "\n" \
    "- Do not modify the characters and the bricksets of the base game. Create your own.\n"
    "\n" \
    "- Audio files may be modified directly.\n"
    "\n" \
    "For more information, read the article on how to upgrade the engine at the Open Surge Wiki: " GAME_WEBSITE \
"";

static inline bool is_import_utility_available();
static inline bool init_dialog();
static int message_box(int flags, const char* format, ...);
static bool is_valid_gamedir(ALLEGRO_FS_ENTRY* dir);
static char* fullpath_of(ALLEGRO_FS_ENTRY* dir, const char* filename, char* dest, size_t dest_size);
static void import_game_ex(const char* gamedir, ALLEGRO_TEXTLOG* textlog);
static void import_files(ALLEGRO_FS_ENTRY* dest, ALLEGRO_FS_ENTRY* src, ALLEGRO_TEXTLOG* textlog);



/*
 * import_game()
 * Import an Open Surge game located at the given absolute path
 */
void import_game(const char* gamedir)
{
    /* call the underlying import function */
    import_game_ex(gamedir, NULL);
}


/*
 * import_wizard()
 * Open Surge Import Wizard
 */
void import_wizard()
{
    ALLEGRO_FILECHOOSER* dialog = NULL;
    const char* gamedir = NULL;

    /* initialize Allegro and its native dialog addon */
    if(!init_dialog()) {
        ERROR("Can't initialize Allegro's native dialogs addon. Try the command-line (run this executable with --help)");
        return;
    }

    /* check if the import wizard is available in this platform */
    if(!is_import_utility_available()) {
        ERROR("%s", UNAVAILABLE_ERROR);
        return;
    }

    /* welcome message */
    ALERT(
        "Welcome to the %s!\n"
        "\n"
        "I will help you import your Open Surge game into this version of the engine (%s).\n"
        "\n"
        "As soon as you import your game, it will be in sync with this version of the engine.",
        TITLE_WIZARD, GAME_VERSION_STRING
    );

    /* ask for a clean build */
    if(YES != CONFIRM(
        "The %s should only be invoked from a clean build of the engine, which you can get at %s. Do not invoke this utility from your MOD.\n"
        "\n"
        "I will alter the files of the current directory, but I don't want you to lose any of your hard work. Backup your stuff.\n"
        "\n"
        "Are you sure you want to continue?",
        TITLE_WIZARD, GAME_WEBSITE
    ))
        goto done;

    /*
        ask for a backup, even though we technically don't require it*

        * assuming that this utility was invoked from a clean build of the engine,
          which should be the case, but may not be

        (users should have a backup anyway!)
    */
    if(YES != CONFIRM("%s\n\nAgree?", BACKUP_MESSAGE))
        goto done;

#if 1
    int repetitions = 3;
    do {
        ALERT("Good. Now I want you to confirm it %d more times, just for fun :)", repetitions);
        for(int i = 1; i <= repetitions; i++) {
            if(YES != CONFIRM("%s\n\n%d / %d", BACKUP_MESSAGE, i, repetitions))
                goto done;
        }
        repetitions *= 2;
    } while(YES == CONFIRM("Alright, gotcha. I love to have fun.\n\nWanna confirm some more?"));
    ALERT("Fine.");
#endif

    /* locate the gamedir */
    ALERT("Now I want you to point out to me where your game is. Where is its folder?");

    for(int i = 1;; i++) {

        if(dialog != NULL)
            al_destroy_native_file_dialog(dialog);

        dialog = al_create_native_file_dialog(
            NULL,
            "Locate the game",
            "",
            ALLEGRO_FILECHOOSER_FOLDER /* | ALLEGRO_FILECHOOSER_FILE_MUST_EXIST */
        );

        if(dialog == NULL) {
            ERROR("Can't create a file dialog");
            goto done;
        }

        if(!al_show_native_file_dialog(NULL, dialog)) {
            ERROR("Can't show a file dialog");
            goto done;
        }

        if(0 == al_get_native_file_dialog_count(dialog)) {
            ERROR("The import was cancelled.");
            goto done;
        }

        gamedir = al_get_native_file_dialog_path(dialog, 0);
        ALLEGRO_FS_ENTRY* e = al_create_fs_entry(gamedir);
        bool valid_gamedir = is_valid_gamedir(e);
        al_destroy_fs_entry(e);

        if(valid_gamedir)
            break;

        WARN("Not a valid Open Surge game!");

        const int MAX_ATTEMPTS = 3;
        if(i >= MAX_ATTEMPTS)
            goto done;

    }

    /* are you sure? */
    if(YES != CONFIRM(
        "I will import %s.\n\nAre you sure you want to continue?",
        gamedir
    ))
        goto done;

    /* create a text log and import the game */
    ALLEGRO_TEXTLOG* textlog = al_open_native_text_log(TITLE_WIZARD, 0);
    import_game_ex(gamedir, textlog);
    WARN("%s", COMPLETED_MESSAGE);
    al_close_native_text_log(textlog);

done:
    ALERT("See you later!");

    /* clean up */
    if(dialog != NULL)
        al_destroy_native_file_dialog(dialog);
}



/* ----- private ----- */

/* initialize Allegro and its native dialog addon */
bool init_dialog()
{
    if(!al_is_system_installed()) {
        if(!al_init())
            return false;
    }

    if(!al_is_native_dialog_addon_initialized()) {
        if(!al_init_native_dialog_addon())
            return false;
    }

    return true;
}

/* is the import utility is available in this platform? */
bool is_import_utility_available()
{
#if defined(_WIN32) || (defined(GAME_RUNINPLACE) && (GAME_RUNINPLACE))
    return true;
#else
    return false;
#endif
}

/* display a message box */
int message_box(int flags, const char* format, ...)
{
    char buffer[8192];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    return al_show_native_message_box(NULL, TITLE_WIZARD, TITLE_WIZARD, buffer, NULL, flags);
}

/* is gamedir a valid opensurge game directory? */
bool is_valid_gamedir(ALLEGRO_FS_ENTRY* dir)
{
    /* check if it exists */
    if(!al_fs_entry_exists(dir))
        return false;

    /* check permissions */
    uint32_t mode = al_get_fs_entry_mode(dir);
    if(!(mode & ALLEGRO_FILEMODE_ISDIR))
        return false;
    else if(!(mode & ALLEGRO_FILEMODE_READ))
        return false;

    /* look for one of the following files in dir */
    const char* files[] = { "surge.rocks", "surge.prefs" };
    for(int i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char path[4096];
        fullpath_of(dir, files[i], path, sizeof(path));

        /* the file exists */
        if(al_filename_exists(path))
            return true;
    }

    /* not a valid folder */
    return false;
}

/* Copies dir/filename to dest. Returns dest */
char* fullpath_of(ALLEGRO_FS_ENTRY* dir, const char* filename, char* dest, size_t dest_size)
{
    const char* gamedir = al_get_fs_entry_name(dir);
    ALLEGRO_PATH* path = al_create_path_for_directory(gamedir);

    al_set_path_filename(path, filename);
    const char* fullpath = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);
    str_cpy(dest, fullpath, dest_size);

    al_destroy_path(path);
    return dest;
}

/* Import an Open Surge game located at the given absolute path */
void import_game_ex(const char* gamedir, ALLEGRO_TEXTLOG* textlog)
{
    ALLEGRO_FS_ENTRY* root = NULL;

    /* check if the import utility is available in this platform */
    if(!is_import_utility_available()) {
        PRINT("%s", UNAVAILABLE_ERROR);
        goto exit;
    }

    /* check if the asset subsystem is uninitialized */
    if(asset_is_init()) {
        PRINT("Can't import anything if the asset subsystem is initialized");
        goto exit;
    }

    /* create the root entry */
    if(NULL == (root = al_create_fs_entry(gamedir))) {
        PRINT("Can't create fs entry for %s", gamedir);
        goto exit;
    }

    /* validate gamedir */
    if(!is_valid_gamedir(root)) {
        PRINT("Not a valid Open Surge game directory: %s", gamedir);
        goto exit;
    }

    /* print headers */
    PRINT("Open Surge Import Utility");
    PRINT("Engine version: %s", GAME_VERSION_STRING);
    PRINT("Importing: %s", gamedir);
    PRINT("Destination: %s", "");
    PRINT(" ");

    /* import files */
    (void)import_files;

    /* done! */
    PRINT("Done!");
    PRINT(" ");

    if(textlog == NULL)
        PRINT("%s", COMPLETED_MESSAGE);

exit:
    /* clean up */
    if(root != NULL)
        al_destroy_fs_entry(root);
}

/* import files from src into dest */
void import_files(ALLEGRO_FS_ENTRY* dest, ALLEGRO_FS_ENTRY* src, ALLEGRO_TEXTLOG* textlog)
{
    /*

    The import procedure works as follows:

    Copy into dest all files from src that do not match the blacklist.
    Do not overwrite any files, except the ones that match the whitelist.

    src and dest are both Open Surge game folders

    Intent: make the imported game be in sync with this version of the engine.

    */
}