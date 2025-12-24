/*
 * Open Surge Engine
 * web.h - Web routines
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
#include <errno.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <sys/types.h>
#else
#include <windows.h>
#endif

#if defined(__ANDROID__)
#define ALLEGRO_UNSTABLE /* required for al_android_get_jni_env(), al_android_get_activity() */
#include <allegro5/allegro.h>
#include <allegro5/allegro_android.h>
#endif

#include "web.h"
#include "video.h"
#include "logfile.h"
#include "../util/util.h"
#include "../util/stringutil.h"




/* private stuff */
static inline char ch2hex(unsigned char code);
static char *encode_uri(const char *uri);
static char *encode_uri_ex(const char *uri, const char encode_table[256]);

#if defined(__ANDROID__)
static void open_web_page(const char* safe_url);
#endif



/* public functions */

/*
 * launch_url()
 * Launches an URL using the default browser.
 * Returns true on success.
 * Useful stuff: http://www.dwheeler.com/essays/open-files-urls.html
 */
bool launch_url(const char *url)
{
    bool success = true;
    char *safe_url = encode_uri(url); /* encode the URL */

    logfile_message("Launching URL: \"%s\"...", safe_url);
    if(video_is_fullscreen())
        video_set_fullscreen(false);

    if(strncmp(safe_url, "http://", 7) == 0 || strncmp(safe_url, "https://", 8) == 0 || strncmp(safe_url, "mailto:", 7) == 0) {
#if defined(__ANDROID__)
        if(!is_tv_device()) {
            open_web_page(safe_url);
        }
        else {
            video_showmessage("Unsupported operation on TV devices");
            success = false;
        }

        (void)file_exists;
#elif defined(_WIN32)
        ShellExecuteA(NULL, "open", safe_url, NULL, NULL, SW_SHOWNORMAL);
        (void)file_exists;
#elif defined(__APPLE__) && defined(__MACH__)
        if(file_exists("/usr/bin/open")) {
            char* argv[] = { "/usr/bin/open", safe_url, NULL };
            pid_t child = fork();

            if(child == 0) {
                execv(argv[0], argv);
                exit(0);
            }
            else if(child < 0) {
                logfile_message("Can't fork process [%s]: %s", str_basename(argv[0]), strerror(errno));
                success = false;
            }
        }
        else 
            success = false;
#elif defined(__unix__) || defined(__unix)
        char* argv[5] = { NULL };

        if(file_exists("/usr/bin/xdg-open")) {
            argv[0] = "/usr/bin/xdg-open";
            argv[1] = safe_url;
            argv[2] = NULL;
        }
        else if(file_exists("/usr/bin/python")) {
            argv[0] = "/usr/bin/python";
            argv[1] = "-m";
            argv[2] = "webbrowser";
            argv[3] = safe_url;
            argv[4] = NULL;
        }
        else if(file_exists("/usr/bin/firefox")) {
            argv[0] = "/usr/bin/firefox";
            argv[1] = safe_url;
            argv[2] = NULL;
        }
        else
            success = false;

        if(argv[0]) {
            pid_t child = fork();

            if(child == 0) {
                execv(argv[0], argv);
                exit(0);
            }
            else if(child < 0) {
                logfile_message("Can't fork process [%s]: %s", str_basename(argv[0]), strerror(errno));
                success = false;
            }
        }
#else
#error "Unsupported operating system."
#endif
    }
    else {
        success = false;
        fatal_error("Can't launch URL (invalid protocol): \"%s\"", safe_url);
    }

    if(!success)
        logfile_message("Can't launch URL: \"%s\"", safe_url);

    free(safe_url);
    return success;
}

/*
 * encode_uri_component()
 * Encodes a URI component. The destination buffer should be 1 + 3x the length
 * of uri.
 */
char* encode_uri_component(const char* uri, char* dest, size_t dest_size)
{
    static char encode_table[256] = { 0 };

    /* create an encoding table */
    if(!encode_table[0]) {
        for(int i = 1; i < 256; i++) {
            encode_table[i] = !(
                (i >= '0' && i <= '9') || /* locale independent */
                (i >= 'a' && i <= 'z') ||
                (i >= 'A' && i <= 'Z') ||
                (strchr("-_.!~*'()", i) != NULL)
            );
        }
        encode_table[0] = 1;
    }

    /* encode URI component */
    char* encoded_uri_component = encode_uri_ex(uri, encode_table);
    str_cpy(dest, encoded_uri_component, dest_size);
    free(encoded_uri_component);
    return dest;
}





/* private methods */

/* converts to hex */
char ch2hex(unsigned char code) {
    static char hex[] = "0123456789ABCDEF";
    return hex[code & 0xF];
}

/* returns an encoded version of a URI */
char* encode_uri(const char* uri)
{
    static char encode_table[256] = { 0 };

    /* create an encoding table */
    if(!encode_table[0]) {
        for(int i = 1; i < 256; i++) {
            encode_table[i] = !(
                (i >= '0' && i <= '9') || /* locale independent */
                (i >= 'a' && i <= 'z') ||
                (i >= 'A' && i <= 'Z') ||
                (strchr(":/-_.*'!?=&~@#$,;()+%", i) != NULL) /* include '%' (encoded URI components) */
            );
        }
        encode_table[0] = 1;
    }

    /* encode URI */
    return encode_uri_ex(uri, encode_table);
}

/* returns an encoded version of a URI, given an encoding table */
char* encode_uri_ex(const char* uri, const char encode_table[256])
{
    char* buf = mallocx((3 * strlen(uri) + 1) * sizeof(char));
    char* p = buf;

    /* encode string */
    while(*uri) {
        if(encode_table[(unsigned char)(*uri)]) {
            *p++ = '%';
            *p++ = ch2hex((unsigned char)(*uri) / 16);
            *p++ = ch2hex((unsigned char)(*uri) % 16);
            uri++;
        }
        else
            *p++ = *uri++;
    }
    *p = '\0';

    /* done */
    return buf;
}

#if defined(__ANDROID__)

/* open a web page on Android */
void open_web_page(const char* safe_url)
{
    /* See https://liballeg.org/a5docs/trunk/platform.html#al_android_get_jni_env */
    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "openWebPage", "(Ljava/lang/String;)V");

    jstring jdata = (*env)->NewStringUTF(env, safe_url);
    (*env)->CallVoidMethod(env, activity, method_id, jdata);
    (*env)->DeleteLocalRef(env, jdata);

    (*env)->DeleteLocalRef(env, class_id);
}

#endif
