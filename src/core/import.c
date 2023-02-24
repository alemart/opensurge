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
#include <ctype.h>
#include "import.h"
#include "global.h"
#include "stringutil.h"
#include "asset.h"

/*

The import procedure works as follows:

Copy to dest/ all files from src/ that do not match the blacklist.
Do not overwrite any files, except the ones that match the whitelist.
Ask the user about overwriting files that match the greylist.

src/ and dest/ are both Open Surge game folders.

My intent is to get the imported game in sync with this version of the engine.
I can do this automatically. On the other hand, "upgrading a game" may require
manual merging, especially when the user has modified assets of the base game.

Getting a game in sync with this version of the engine is a simpler problem
than upgrading (or downgrading) such game. Getting it in sync is the bulk of
the work of upgrading, and so this is a semi-automated way of upgrading a game.

Tip: if this Import Utility is invoked with the user path environment variable
set (see asset.c), then the files will be imported to that path rather than to
the directory of the executable (which is the default behavior). Simply put,

- dest/ is the folder of this executable or the user path environment variable.
- src/  is the folder of the imported game and is specified by the user.

*/

/* utility macros */
#define IMPORT_LOGFILE_NAME         "import_log.txt"
#define ENVIRONMENT_VARIABLE_NAME   "OPENSURGE_USER_PATH" /* FIXME: duplicated string from asset.c */

#define DRY_RUN                     0 /* don't actually copy any files; for testing purposes only */
#define WANT_SILLY_JOKE             1
#define PATH_MAXSIZE                4096

#define ALERT(...)                  message_box(0, __VA_ARGS__)
#define WARN(...)                   message_box(ALLEGRO_MESSAGEBOX_WARN, __VA_ARGS__)
#define ERROR(...)                  message_box(ALLEGRO_MESSAGEBOX_ERROR, __VA_ARGS__)
#define CONFIRM(...)                message_box(ALLEGRO_MESSAGEBOX_QUESTION | ALLEGRO_MESSAGEBOX_YES_NO, __VA_ARGS__)
#define YES                         1

#define PRINT(...)      do { \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n"); \
    if(import_logfile != NULL) { \
        al_fprintf(import_logfile, __VA_ARGS__); \
        al_fprintf(import_logfile, "\n"); \
    } \
    if(textlog != NULL) { \
        al_append_native_text_log(textlog, __VA_ARGS__); \
        al_append_native_text_log(textlog, "\n"); \
    } \
} while(0)

/*
   whitelist & blacklist

   these are the patterns to which we'll match the (relative) filepaths
   use a slash '/' as the path separator
*/
#define PREFIX(path)                "^" path
#define SUFFIX(path)                "$" path
#define EXACT(path)                 "=" path
#define IS_PREFIX(pattern)          ((pattern)[0] == '^')
#define IS_SUFFIX(pattern)          ((pattern)[0] == '$')
#define IS_EXACT(pattern)           ((pattern)[0] == '=')
#define PATTERN_PATH(pattern)       ((pattern) + 1)

static const char* WHITELIST[] = {

    EXACT("surge.prefs"),
    EXACT("surge.rocks"),
    EXACT("surge.cfg"),

    EXACT("quests/default.qst"),
    EXACT("quests/intro.qst"),

    EXACT("themes/scenes/credits.bg"),
    EXACT("themes/scenes/langselect.bg"),
    EXACT("themes/scenes/levelselect.bg"),
    EXACT("themes/scenes/options.bg"),

    EXACT("images/loading.png"),

    PREFIX("musics/"),
    PREFIX("samples/"),

    PREFIX("fonts/"), /* FIXME: should we have a fonts/overrides/ instead? */

    /* NULL-terminated list */
    NULL

};

static const char* BLACKLIST[] = {

    SUFFIX(".exe"),
    EXACT("opensurge"),
    EXACT("logfile.txt"),
    EXACT(IMPORT_LOGFILE_NAME),

    EXACT("CMakeLists.txt"),
    PREFIX("src/"),
    PREFIX("build/"),

    EXACT("CHANGES.md"),
    EXACT("CONTRIBUTING.md"),
    EXACT("README.md"),
    EXACT("LICENSE"),
    EXACT("surge.png"),
    EXACT("logo.png"),

    /* deleted files from previous builds */
    EXACT("preferences.dat"),

    /* NULL-terminated list */
    NULL

};

/* the greylist should have few matching files, because we'll ask the user about each one of them */
static const char* GREYLIST[] = {

    /* users sometimes stick with outdated mappings;
       a manual merge could be appropriate */
    EXACT("inputs/default.in"),

    /* NULL-terminated list */
    NULL

};

/* instructions & tips */
#define SUCCESSFUL_IMPORT_1 "" \
    "Your game is now in sync with version " GAME_VERSION_STRING " of the engine.\n" \
    "\n" \
    "It's possible that you'll see some of your changes missing. If this happens, you'll have to adjust a few things.\n" \
    "\n" \
    "As a rule of thumb, KEEP YOUR ASSETS SEPARATE FROM THOSE OF THE BASE GAME.\n" \
    "\n" \
    "If you've been following the above rule of thumb, upgrading your game to the latest versions of the engine will be straightforward. If not, this is a good time to fix things.\n" \
""

#define SUCCESSFUL_IMPORT_2 "" \
    "Tips:\n" \
    "\n" \
    "- Do not modify the images of the base game. If you want to change them, create new images and replace the sprites using the sprites/overrides/ folder.\n" \
    "\n" \
    "- Do not modify the texts of the base game. If you want different texts, store your changes in the languages/extends/ folder.\n" \
    "\n" \
    "- Do not modify the SurgeScript objects of the base game. If you want changed functionality, clone the objects, change their name and save them as separate files.\n" \
    "\n" \
    "- Do not modify the characters/levels/bricksets of the base game. Clone them before you remix, or create your own.\n" \
""

#define SUCCESSFUL_IMPORT_3 "" \
    "More tips:\n" \
    "\n" \
    "- If you have modified the input controls, manually merge your changes. Look especially at inputs/default.in.\n" \
    "\n" \
    "- If you'd like to know which files you have previously changed, you may run a diff between the folder of your MOD and the folder of a clean build of the version of the engine you were using.\n" \
    "\n" \
    "- If you have modified the source code of the engine (C language), your changes no longer apply. You may redo them.\n" \
    "\n" \
    "- You can modify the audio files directly. These files were all imported.\n" \
""

#define SUCCESSFUL_IMPORT_4 "" \
    "Again: keep your assets separate from those of the base game. This is what you need to know in a nutshell.\n" \
    "\n" \
    "The logfile can give you insights in case of need. For more information, read the article on how to upgrade the engine at the Open Surge Wiki: " GAME_WEBSITE \
""

#define FOREACH_SUCCESSFUL_IMPORT_MESSAGE(F) \
    F(SUCCESSFUL_IMPORT_1) \
    F(SUCCESSFUL_IMPORT_2) \
    F(SUCCESSFUL_IMPORT_3) \
    F(SUCCESSFUL_IMPORT_4)

#define GLUE(x) x "\n"
#define TELL(x) WARN("%s", x);

/* strings */
static const char TITLE_WIZARD[] = "Open Surge Import Wizard";
static const char UNAVAILABLE_ERROR[] = "Define environment variable " ENVIRONMENT_VARIABLE_NAME " before invoking this import utility.";
static const char INVALID_DIRECTORY_ERROR[] = "Not a valid Open Surge game directory!";
static const char BACKUP_MESSAGE[] = "\"I declare that I made a backup of my game. My backup is stored safely and I can access it now and in the future.\"";
static const char SUCCESSFUL_IMPORT[] = FOREACH_SUCCESSFUL_IMPORT_MESSAGE(GLUE);
static const char UNSUCCESSFUL_IMPORT[] = "Something went wrong.\n\nPlease review the logs at " IMPORT_LOGFILE_NAME ", double check the permissions of your filesystem and try again.";

/* private functions */
static inline bool is_import_utility_available();
static inline bool init_allegro();
static inline bool init_dialog();
static int message_box(int flags, const char* format, ...);
static bool is_valid_gamedir(ALLEGRO_FS_ENTRY* dir);
static char* fullpath_of(ALLEGRO_FS_ENTRY* dir, const char* filename, char* dest, size_t dest_size);
static bool import_game_ex(const char* gamedir);
static int import_files(ALLEGRO_FS_ENTRY* dest, ALLEGRO_FS_ENTRY* src);
static int import_file(ALLEGRO_FS_ENTRY* e, void* extra);
static bool is_match(const char* relative_path, const char** pattern_list);
static bool copy_file(ALLEGRO_FS_ENTRY* dest, ALLEGRO_FS_ENTRY* src);
static int my_for_each_fs_entry(ALLEGRO_FS_ENTRY *dir, int (*callback)(ALLEGRO_FS_ENTRY *dir, void *extra), void *extra);
static bool make_directory_for_file(ALLEGRO_FS_ENTRY* e);
static int pathcmp(const char* a, const char* b);
static ALLEGRO_FILE* open_import_logfile(const char* path_to_directory);
static ALLEGRO_FILE* close_import_logfile(ALLEGRO_FILE* fp);

/* outputs for the import log */
static ALLEGRO_FILE* import_logfile = NULL;
static ALLEGRO_TEXTLOG* textlog = NULL;




/* ----- public ----- */


/*
 * import_game()
 * Import an Open Surge game located at the given absolute path
 */
void import_game(const char* gamedir)
{
    /* initialize the outputs for the import log */
    import_logfile = NULL;
    textlog = NULL;

    /* initialize Allegro */
    if(!init_allegro()) {
        ERROR("Can't initialize Allegro");
        return;
    }

    /* call the underlying import function */
    if(import_game_ex(gamedir))
        PRINT("%s", SUCCESSFUL_IMPORT);
    else
        PRINT("%s", UNSUCCESSFUL_IMPORT);
}


/*
 * import_wizard()
 * The Open Surge Import Wizard is a graphical utility that calls the
 * underlying Import Utility, which will import an Open Surge game
 */
void import_wizard()
{
    ALLEGRO_FILECHOOSER* dialog = NULL;
    const char* gamedir = NULL;

    /* initialize the outputs for the import log */
    import_logfile = NULL;
    textlog = NULL;

    /* initialize Allegro */
    if(!init_allegro()) {
        ERROR("Can't initialize Allegro");
        return;
    }

    /* initialize Allegro's native dialog addon */
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
        "As soon as you import your game, it will be in sync with this version.",
        TITLE_WIZARD, GAME_VERSION_STRING
    );

    /* ask for a clean build */
    if(YES != CONFIRM(
        "The %s should only be invoked from a clean build of the engine, which you can get at %s.\n"
        "\n"
        "Do not invoke me from your MOD. I will alter some files of this build. Also, backup your stuff, because I don't want you to lose any of your hard work.\n"
        "\n"
        "Are you sure you want to continue?",
        TITLE_WIZARD, GAME_WEBSITE
    ))
        goto done;

    /*
        ask for a backup, even though we technically don't require it*

        * assuming that this utility was invoked from a clean build of the engine,
          which should be the case. If the user accidentally invokes this from his
          or her MOD, then a backup is required!

        (users should have a backup anyway!)
    */
    if(YES != CONFIRM("%s\n\nAgree?", BACKUP_MESSAGE))
        goto done;

#if WANT_SILLY_JOKE
    int repetitions = 3;
    do {

        ALERT("Good.");
        ALERT("Now I want you to confirm it to me %d more times, just for fun :)", repetitions);

        for(int i = 1; i <= repetitions; i++) {
            if(YES != CONFIRM("%s\n\n%d / %d", BACKUP_MESSAGE, i, repetitions)) {
                ALERT("Wrong answer!");
                goto done;
            }
        }

        repetitions *= 2;

        /* are you paying attention?!?! */
    } while(YES == CONFIRM("Alright, gotcha.\n\nWanna confirm some more?"));
    ALERT("Fine.");
#endif

    /* locate the gamedir */
    ALERT("Now I want you to point out to me where your game is. Where is its folder?");

    for(int i = 1;; i++) {

        if(dialog != NULL)
            al_destroy_native_file_dialog(dialog);

        dialog = al_create_native_file_dialog(
            NULL,
            "Where is the folder of the game?",
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

        WARN("%s\n\n%s", INVALID_DIRECTORY_ERROR, gamedir);

        const int MAX_ATTEMPTS = 3;
        if(i >= MAX_ATTEMPTS) {
            ALERT("Enough of that.");
            goto done;
        }

    }

    /* are you sure? */
    if(YES != CONFIRM(
        "I will import %s.\n\nAre you sure you want to continue?",
        gamedir
    ))
        goto done;

    /* create a text log and import the game */
    textlog = al_open_native_text_log(TITLE_WIZARD, ALLEGRO_TEXTLOG_MONOSPACE);

    if(import_game_ex(gamedir)) {
        { FOREACH_SUCCESSFUL_IMPORT_MESSAGE(TELL); }
        PRINT("%s", SUCCESSFUL_IMPORT);
    }
    else {
        ERROR("%s", UNSUCCESSFUL_IMPORT);
        PRINT("%s", UNSUCCESSFUL_IMPORT);
    }

    al_close_native_text_log(textlog);
    textlog = NULL;

done:
    ALERT("See you later!");

    /* clean up */
    if(dialog != NULL)
        al_destroy_native_file_dialog(dialog);
}



/* ----- private ----- */

/* initialize Allegro */
bool init_allegro()
{
    if(!al_is_system_installed()) {
        if(!al_init())
            return false;
    }

    return true;
}

/* initialize Allegro's native dialog addon */
bool init_dialog()
{
    if(!al_is_native_dialog_addon_initialized()) {
        if(!al_init_native_dialog_addon())
            return false;
    }

    return true;
}

/* is the import utility is available? */
bool is_import_utility_available()
{
#if defined(_WIN32) || (defined(GAME_RUNINPLACE) && (GAME_RUNINPLACE))
    return true;
#else
    return (NULL != getenv(ENVIRONMENT_VARIABLE_NAME));
#endif
}

/* display a message box */
int message_box(int flags, const char* format, ...)
{
    char buffer[4096];
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    /* According to the Allegro manual, al_show_native_message_box() may be called
       without Allegro being initialized. */
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
    const char* files[] = { "surge.prefs", "surge.rocks", "surge.cfg" };
    for(int i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        char path[PATH_MAXSIZE];
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
bool import_game_ex(const char* gamedir)
{
    ALLEGRO_FS_ENTRY* src = NULL;
    ALLEGRO_FS_ENTRY* dest = NULL;
    int error_count = -1;

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

    /* create the src entry corresponding to gamedir */
    if(NULL == (src = al_create_fs_entry(gamedir))) {
        PRINT("Can't create src fs entry for %s", gamedir);
        goto exit;
    }

    /* validate the source directory (i.e., gamedir) */
    if(!is_valid_gamedir(src)) {
        PRINT("%s %s", INVALID_DIRECTORY_ERROR, gamedir);
        goto exit;
    }

    /* find the destination directory */
    char destdir[PATH_MAXSIZE];
    asset_user_datadir(destdir, sizeof(destdir)); /* depends on ENVIRONMENT_VARIABLE_NAME */

    /* create the dest entry */
    if(NULL == (dest = al_create_fs_entry(destdir))) {
        PRINT("Can't create dest fs entry for %s", destdir);
        goto exit;
    }

    /* validate the destination directory (it may not be the folder of this executable) */
    if(!is_valid_gamedir(dest)) {
        PRINT("%s %s", INVALID_DIRECTORY_ERROR, destdir);
        goto exit;
    }

    /* check if src != dest */
    if(0 == pathcmp(al_get_fs_entry_name(src), al_get_fs_entry_name(dest))) {
        PRINT("No need to import a game into its own folder");
        goto exit;
    }

    /* open the import logfile */
    import_logfile = open_import_logfile(al_get_fs_entry_name(dest));

    /* print headers */
    PRINT("Open Surge Import Utility");
    PRINT("Engine version: %s", GAME_VERSION_STRING);
    PRINT("Importing: %s", al_get_fs_entry_name(src));
    PRINT("Destination: %s", al_get_fs_entry_name(dest));

    /* import files */
    error_count = import_files(dest, src);

    /* done! */
    PRINT(" ");
    PRINT("Done!");

    /* show error count */
    if(error_count > 1)
        PRINT("%d errors have occurred.", error_count);
    else if(error_count == 1)
        PRINT("1 error has occurred.");

    /* close the import logfile */
    import_logfile = close_import_logfile(import_logfile);

exit:
    /* clean up */
    if(src != NULL)
        al_destroy_fs_entry(src);

    if(dest != NULL)
        al_destroy_fs_entry(dest);

    /* return success if no errors have occurred */
    return error_count == 0;
}

/* import files from src/ into dest/, returning the number of errors */
int import_files(ALLEGRO_FS_ENTRY* dest, ALLEGRO_FS_ENTRY* src)
{
    const char* dest_dir = al_get_fs_entry_name(dest);
    const char* src_dir = al_get_fs_entry_name(src);
    int error_count = 0;

    /* prepare iteration */
    ALLEGRO_PATH* src_path = al_create_path_for_directory(src_dir);
    ALLEGRO_PATH* dest_path = al_create_path_for_directory(dest_dir);

    /* call import_file() for each entry of the src folder */
    void* extra[] = { src_path, dest_path, &error_count };
    my_for_each_fs_entry(src, import_file, extra);

    /* clean up */
    al_destroy_path(dest_path);
    al_destroy_path(src_path);

    /* done! */
    return error_count;
}

/* import a file, if applicable */
int import_file(ALLEGRO_FS_ENTRY* e, void* extra)
{
    const ALLEGRO_PATH* src_path = (ALLEGRO_PATH*)(((void**)extra)[0]);
    const ALLEGRO_PATH* dest_path = (ALLEGRO_PATH*)(((void**)extra)[1]);
    int* error_count = (int*)(((void**)extra)[2]);
    int result = ALLEGRO_FOR_EACH_FS_ENTRY_OK; /* continue iteration */

    /* find e_path, the absolute path of the current entry e */
    bool is_dir = (al_get_fs_entry_mode(e) & ALLEGRO_FILEMODE_ISDIR) != 0;
    ALLEGRO_PATH* e_path = (is_dir ? al_create_path_for_directory : al_create_path)(al_get_fs_entry_name(e));

    /* make e_path relative to src_path */
    ALLEGRO_PATH* relative_path = al_clone_path(e_path);
    int root_cnt = al_get_path_num_components(src_path);
    while(root_cnt-- > 0)
        al_remove_path_component(relative_path, 0);
    al_set_path_drive(relative_path, NULL);

    /* find d_path, the absolute path in the destination folder corresponding to e_path */
    ALLEGRO_PATH* d_path = al_clone_path(dest_path);
    if(al_join_paths(d_path, relative_path)) {

        /* use a consistent path separator across platforms */
        const char* vpath = al_path_cstr(relative_path, '/');
        vpath = *vpath ? vpath : "/";

        /* will we import the file? */
        bool import = false;

        /* match the file */
        if(is_dir) {

            /* ignore directories */
            PRINT(" ");
            PRINT("Scanning %s", vpath);
            PRINT(" ");

        }
        else if(is_match(vpath, BLACKLIST)) {

            /* ignore files in the blacklist */
            PRINT("    Ignoring %s", vpath);

        }
        else if(al_filename_exists(al_path_cstr(d_path, ALLEGRO_NATIVE_PATH_SEP)) && is_match(vpath, GREYLIST)) {

            /* ask the user about overwriting files in the greylist */
            if(YES != CONFIRM("Use updated file \"%s\" of the base game?\n\nDefault answer: yes", vpath)) {
                PRINT("    Importing %s", vpath);
                import = true;
            }
            else {
                PRINT("    Skipping %s", vpath);
            }

        }
        else if(al_filename_exists(al_path_cstr(d_path, ALLEGRO_NATIVE_PATH_SEP)) && !is_match(vpath, WHITELIST)) {

            /* skip existing files, except those that are in the whitelist */
            PRINT("    Skipping %s", vpath);

        }
        else {

            /* import all other files */
            PRINT("    Importing %s", vpath);
            import = true;

        }

        /* import the file */
        if(import) {
            ALLEGRO_FS_ENTRY* d = al_create_fs_entry(al_path_cstr(d_path, ALLEGRO_NATIVE_PATH_SEP));

            if(!copy_file(d, e)) {
                PRINT("!   ERROR: can't copy %s", vpath);
                ++(*error_count);
            }

            al_destroy_fs_entry(d);
        }

    }
    else {

        /* this shouldn't happen */
        PRINT("ERROR: something went wrong\n%s\n%s\n", al_path_cstr(relative_path, ALLEGRO_NATIVE_PATH_SEP), al_path_cstr(dest_path, ALLEGRO_NATIVE_PATH_SEP));
        result = ALLEGRO_FOR_EACH_FS_ENTRY_STOP;

    }

    /* clean up */
    al_destroy_path(d_path);
    al_destroy_path(relative_path);
    al_destroy_path(e_path);

    /* done */
    return result;
    
    /* note: if we have a prefix match on a directory, we don't need to recurse... */
}

/* Checks if relative_path matches any entry of the NULL-terminated pattern_list
   We assume that the path separator of relative_path is always a slash '/', regardless of the underlying platform */
bool is_match(const char* relative_path, const char** pattern_list)
{
    bool match = false;

    /* for each pattern of pattern_list, look for a match */
    for(const char* pattern = *pattern_list; pattern != NULL && !match; pattern = *(++pattern_list)) {
        const char* pattern_path = PATTERN_PATH(pattern);

        if(IS_EXACT(pattern))
            match = (0 == str_icmp(relative_path, pattern_path));
        else if(IS_PREFIX(pattern))
            match = str_istartswith(relative_path, pattern_path);
        else if(IS_SUFFIX(pattern))
            match = str_iendswith(relative_path, pattern_path);
    }

    /* done */
    return match;
}

/* similar to al_for_each_fs_entry(), except that files are scanned first and folders later */
int my_for_each_fs_entry(ALLEGRO_FS_ENTRY *dir, int (*callback)(ALLEGRO_FS_ENTRY *dir, void *extra), void *extra)
{
    /* call the callback on dir, which may be the "root" folder (i.e., gamedir) */
    int result = callback(dir, extra);
    if(result != ALLEGRO_FOR_EACH_FS_ENTRY_OK) /* don't recurse */
        return result;

    /* open directory */
    if(dir == NULL || !al_open_directory(dir)) {
        al_set_errno(ENOENT); /* no such file or directory */
        return ALLEGRO_FOR_EACH_FS_ENTRY_ERROR;
    }

    /* for each entry */
    for(ALLEGRO_FS_ENTRY* entry = al_read_directory(dir); entry != NULL; entry = al_read_directory(dir)) {

        /* skip directories */
        int mode = al_get_fs_entry_mode(entry);
        if(mode & ALLEGRO_FILEMODE_ISDIR)
            continue;

        /* call the callback */
        int result = callback(entry, extra);

        /* clean up */
        al_destroy_fs_entry(entry);

        /* stop iteration? */
        if(result == ALLEGRO_FOR_EACH_FS_ENTRY_STOP || result == ALLEGRO_FOR_EACH_FS_ENTRY_ERROR) {
            al_close_directory(dir);
            return result;
        }

    }

    /* re-open directory */
    if(!al_close_directory(dir) || !al_open_directory(dir)) {
        al_set_errno(ENOENT); /* no such file or directory */
        return ALLEGRO_FOR_EACH_FS_ENTRY_ERROR;
    }

    /* for each entry (again) */
    for(ALLEGRO_FS_ENTRY* entry = al_read_directory(dir); entry != NULL; entry = al_read_directory(dir)) {

        /* skip non-directories */
        int mode = al_get_fs_entry_mode(entry);
        if(!(mode & ALLEGRO_FILEMODE_ISDIR))
            continue;

        /* recurse */
        my_for_each_fs_entry(entry, callback, extra);

        /* clean up */
        al_destroy_fs_entry(entry);

        /* stop iteration? */
        if(result == ALLEGRO_FOR_EACH_FS_ENTRY_STOP || result == ALLEGRO_FOR_EACH_FS_ENTRY_ERROR) {
            al_close_directory(dir);
            return result;
        }

    }

    /* done! */
    al_close_directory(dir);
    return ALLEGRO_FOR_EACH_FS_ENTRY_OK;
}

/* copy file src to dest using Allegro's File I/O */
bool copy_file(ALLEGRO_FS_ENTRY* dest, ALLEGRO_FS_ENTRY* src)
{
#if !(DRY_RUN)

    /* create the directory that will store dest, if needed */
    if(!make_directory_for_file(dest))
        return false;

    /* open the files */
    ALLEGRO_FILE* file_read = al_open_fs_entry(src, "rb");
    if(NULL == file_read)
        return false;

    ALLEGRO_FILE* file_write = al_open_fs_entry(dest, "wb");
    if(NULL == file_write) {
        al_fclose(file_read);
        return false;
    }

    /* copy src to dest */
    unsigned char buffer[4096];
    int num_bytes;
    bool success = true;

    while(!al_feof(file_read) && success) {
        /* expect to read sizeof(buffer) bytes from the source file,
           except (probably) at the end of it */
        if(sizeof(buffer) != (num_bytes = al_fread(file_read, buffer, sizeof(buffer))))
            success = !al_ferror(file_read);

        /* expect to write all the bytes we've just read */
        if(success && (num_bytes != al_fwrite(file_write, buffer, num_bytes)))
            success = false;
    }

    /* close the files */
    al_fclose(file_write);
    al_fclose(file_read);

    /* done! */
    return success;

#else

    (void)dest;
    (void)src;
    (void)make_directory_for_file;

    return true;

#endif
}

/* create a directory (and parent directories, if needed) to store a file */
bool make_directory_for_file(ALLEGRO_FS_ENTRY* e)
{
    ALLEGRO_PATH* path = al_create_path(al_get_fs_entry_name(e));
    al_set_path_filename(path, NULL);

    /* al_make_directory() returns true (success) if the directory already exists */
    bool result = al_make_directory(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));

    al_destroy_path(path);
    return result;
}

/* compare paths to directories a/ and b/ */
int pathcmp(const char* a, const char* b)
{
    ALLEGRO_PATH* p = al_create_path_for_directory(a);
    ALLEGRO_PATH* q = al_create_path_for_directory(b);

    int result = str_icmp(
        al_path_cstr(p, ALLEGRO_NATIVE_PATH_SEP),
        al_path_cstr(q, ALLEGRO_NATIVE_PATH_SEP)
    );

    al_destroy_path(q);
    al_destroy_path(p);

    return result;
}

/* open an import logfile for output given the absolute path to its directory */
ALLEGRO_FILE* open_import_logfile(const char* path_to_directory)
{
    ALLEGRO_PATH* path = al_create_path_for_directory(path_to_directory);
    al_set_path_filename(path, IMPORT_LOGFILE_NAME);

    ALLEGRO_FILE* fp = al_fopen(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP), "w");

    al_destroy_path(path);
    return fp;
}

/* close a previously open import logfile */
ALLEGRO_FILE* close_import_logfile(ALLEGRO_FILE* fp)
{
    if(fp != NULL)
        al_fclose(fp);

    return NULL;
}