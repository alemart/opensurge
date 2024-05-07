/*
 * Open Surge Engine
 * modloader.c - a helper scene that loads a MOD
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#if !defined(__ANDROID__)
#include <allegro5/allegro.h>
#else
#define ALLEGRO_UNSTABLE /* al_android_open_fd() */
#include <allegro5/allegro.h>
#include <allegro5/allegro_android.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "modloader.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../core/asset.h"
#include "../core/audio.h"
#include "../core/video.h"
#include "../core/engine.h"
#include "../core/scene.h"
#include "../core/lang.h"
#include "../core/storyboard.h"
#include "../core/commandline.h"
#include "../core/logfile.h"
#include "../entities/sfx.h"

static const char EXIT_LEVEL[] = "levels/scenes/thanks_for_playing.lev";
static commandline_t args;

#if defined(__ANDROID__)
static const char* find_absolute_filepath(const char* content_uri);
static bool open_file_at_uri(const char* uri, ALLEGRO_FILE** out_f);
static const char* url_encoded_basename(const char* url);
static bool download_to_cache(ALLEGRO_FILE* f, const char* destination_path, void (*on_progress)(double,void*), void* context);
static bool need_to_download_to_cache(ALLEGRO_FILE* f, const char* destination_path);
static void show_download_progress(double percentage, void* context);
#endif



/*
 * modloader_init()
 * Initializes the restart scene
 */
void modloader_init(void* ctx)
{
    /* set the command line arguments */
    if(ctx != NULL)
        args = *((const commandline_t*)ctx);
    else
        args = commandline_parse(0, NULL);

    /* show the exit scene */
    if(asset_exists(EXIT_LEVEL) && args.gamedir[0] != '\0') {
#if !defined(__ANDROID__)
        if(asset_is_valid_gamedir(args.gamedir, NULL)) {
#else
        if(1) { /* download the gamedir to cache first, before checking if it's valid */
#endif
            scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)EXIT_LEVEL);
        }
    }
}


/*
 * modloader_release()
 * Releases the restart scene
 */
void modloader_release()
{
    ;
}


/*
 * modloader_update()
 * Updates the restart scene
 */
void modloader_update()
{
    bool success = true;
    bool is_legacy_gamedir = false;

#if defined(__ANDROID__)
    /* get an absolute filepath if gamedir is a content:// URI */
    if(strncmp(args.gamedir, "content://", 10) == 0) {
        const char* fullpath = find_absolute_filepath(args.gamedir);

        if(fullpath != NULL)
            str_cpy(args.gamedir, fullpath, sizeof(args.gamedir));
        else
            success = false;
    }
#endif

    /* validate the gamedir, if any */
    if(success && args.gamedir[0] != '\0')
        success = asset_is_valid_gamedir(args.gamedir, &is_legacy_gamedir);

    /* are... you... ready??? */
    if(success) {
        /* restart the engine on success */
        engine_restart(&args);
    }
    else {
        /* display an error message */
        sound_play(SFX_DENY);

        if(!is_legacy_gamedir)
            alert("%s", lang_get("OPTIONS_PLAYMOD_NOTAGAME"));
        else
            alert("%s", lang_get("OPTIONS_PLAYMOD_LEGACYERROR"));
    }

    /* return to the previous scene */
    scenestack_pop();
}


/*
 * modloader_render()
 * Renders the restart scene
 */
void modloader_render()
{
    ;
}




/*
 *
 * Android-specific
 *
 */

#if defined(__ANDROID__)

/* find an absolute path equivalent to an openable document URI;
   returns a statically allocated string on success or NULL on error */
const char* find_absolute_filepath(const char* content_uri)
{
    const char* filename = url_encoded_basename(content_uri);
    const char* path_to_game = NULL;
    static char cache_path[1024];
    ALLEGRO_FILE* f = NULL;

    /* create relative path "games/$filename" */
    char relative_path[1024] = "games/";
    size_t len = strlen(relative_path);
    str_cpy(relative_path + len, filename, sizeof(relative_path) - len);
    assertx(0 == strcmp(relative_path + len, filename), "filename too long");
    cache_path[0] = '\0';

    if(!open_file_at_uri(content_uri, &f)) {
        /* can't open the file */
        sound_play(SFX_DENY);
        alert("%s", "Can't open the selected file! Make sure you have the necessary permissions.");
    }
    else if('\0' == *(asset_cache_path(relative_path, cache_path, sizeof(cache_path)))) {
        /* this shouldn't happen */
        sound_play(SFX_DENY);
        alert("%s %s", "Can't find the application cache!", filename);
    }
    else {
        /* download the game to the application cache */
        logfile_message("Path at the cache: \"%s\"", cache_path);

        if(need_to_download_to_cache(f, cache_path)) {
            logfile_message("The game is not yet cached. We'll cache it.");

            if(download_to_cache(f, cache_path, show_download_progress, video_display_loading_screen_ex)) {
                logfile_message("The game is now cached!");
                path_to_game = cache_path;
            }
            else {
                sound_play(SFX_DENY);
                alert("%s", "Can't open the game! You may clear the application cache to get extra storage space.");
            }
        }
        else {
            logfile_message("The game is already cached");
            show_download_progress(1.0, video_display_loading_screen_ex);
            path_to_game = cache_path;
        }
    }

    /* close the file */
    if(f != NULL)
        al_fclose(f);

    /* make sure the path points to a valid opensurge game */
    if(path_to_game != NULL && !asset_is_valid_gamedir(path_to_game, NULL)) {

        /* remove the downloaded file from cache */
        logfile_message("Not a valid gamedir: %s", path_to_game);

        if(0 != remove(path_to_game)) {
            const char* err = strerror(errno);
            logfile_message("Error deleting file from cache. %s", err);
        }

        /* not a valid gamedir */
        path_to_game = NULL;

    }

    /* done! */
    return path_to_game;
}

/* open a file given a Universal Resource Identifier (URI) */
bool open_file_at_uri(const char* uri, ALLEGRO_FILE** out_f)
{
    int fd = al_android_open_fd(uri, "r");

    if(fd < 0) {
        logfile_message("%s al_android_open_fd failed fd=%d uri=%s", __func__, fd, uri);
        *out_f = NULL;
        return false;
    }

    if(NULL == (*out_f = al_fopen_fd(fd, "rb"))) {
        logfile_message("%s al_fopen_fd failed", __func__);
        close(fd);
        return false;
    }

    return true;
}

/* get the basename of a URL-encoded string */
const char* url_encoded_basename(const char* url)
{
    const char* SLASH = "%2F";

    for(const char *p = NULL; NULL != (p = strstr(url, SLASH));)
        url = p+3;

    return url; /* XXX is (*url == '\0') possible? */
}

/* copy an open file stream f to the application cache */
bool download_to_cache(ALLEGRO_FILE* f, const char* destination_path, void (*on_progress)(double,void*), void* context)
{
    /* since we're operating on the application cache, we have
       write permissions to open the destination path for writing */
    enum { BUFFER_SIZE = 4096, MEGABYTE = 1048576, PROGRESS_CHUNK = 1 * MEGABYTE };
    STATIC_ASSERTX(PROGRESS_CHUNK % BUFFER_SIZE == 0, validate_progress_chunk);

    uint8_t buffer[BUFFER_SIZE];
    size_t n;
    bool error;
    FILE* f_copy;
    int64_t total_bytes;
    int64_t bytes_written;

    /* open filepath for writing */
    if(NULL == (f_copy = fopen(destination_path, "wb"))) {
        const char* err = strerror(errno);
        sound_play(SFX_DENY);
        alert("%s %s", "Can't write a cached copy!", err);
        return false;
    }

    /* copy the file */
    total_bytes = al_fsize(f);
    bytes_written = 0;
    error = false;

    on_progress(0.0, context);
    while(!error && 0 < (n = al_fread(f, buffer, sizeof(buffer)))) {
        error = (n != fwrite(buffer, 1, n, f_copy));

        /* show progress */
        if(0 == ((bytes_written += n) % PROGRESS_CHUNK)) {
            int64_t fraction = (bytes_written << 10) / total_bytes;
            double percentage = (double)fraction / 1024.0;

            on_progress(percentage, context);
        }
    }
    on_progress(1.0, context);

    /* error checking */
    if(error) {
        sound_play(SFX_DENY);
        alert("%s", "Can't copy the file to the application cache! Make sure there is enough storage space in your device.");

        if(al_ferror(f) != 0)
            alert("READ ERROR: %s", al_ferrmsg(f));

        if(ferror(f_copy) != 0)
            alert("WRITE ERROR: %d", ferror(f_copy));
    }

    /* close the copy */
    fclose(f_copy);

    /* done! */
    return !error;
}

/* do we need to download file f to destination_path at the application cache? */
bool need_to_download_to_cache(ALLEGRO_FILE* f, const char* destination_path)
{
#if 1
    /* Simple heuristic: compare the size of the files. This is not always
       correct, but it is probably correct. We want this routine to be fast.
       Computing a checksum takes time; the file is expected to have a size of
       hundreds of megabytes. Users can clear the cache to force new downloads. */
    struct stat buf;

    if(0 != stat(destination_path, &buf)) {
        const char* err = strerror(errno);
        logfile_message("can't stat \"%s\": %s", destination_path, err);
        return true; /* file not found (possibly) */
    }

    int64_t f_size = al_fsize(f);
    int64_t d_size = buf.st_size;

    return (f_size != d_size);
#else
    /* always download to cache */
    return true;
#endif
}

/* show download progress */
void show_download_progress(double percentage, void* context)
{
    void (*fn)(double) = (void (*)(double))context;

    if(fn != NULL)
        fn(percentage);
}

#endif
