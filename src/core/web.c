/*
 * Open Surge Engine
 * web.h - Web routines
 * Copyright (C) 2012, 2013, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "web.h"
#include "util.h"
#include "video.h"
#include "logfile.h"

#if !defined(_WIN32)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <windows.h>
#endif



/* private stuff */
static bool file_exists(const char *filepath);
static inline char ch2hex(int code);
static char *url_encode(const char *str);
/*static char *url_decode(const char *str);*/
/*static char hex2ch(char ch);*/




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
    char *safe_url = url_encode(url); /* it's VERY important to sanitize the URL... */

    if(video_is_fullscreen())
        video_changemode(video_get_resolution(), video_is_smooth(), FALSE);

    if(strncmp(safe_url, "http://", 7) == 0 || strncmp(safe_url, "https://", 8) == 0 || strncmp(safe_url, "ftp://", 6) == 0 || strncmp(safe_url, "mailto:", 7) == 0) {
#if defined(_WIN32)
        ShellExecute(NULL, "open", safe_url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__) && defined(__MACH__)
        char *safe_cmd = mallocx(sizeof(char) * (strlen(safe_url) + 32));
        sprintf(safe_cmd, "open \"%s\"", safe_url);
        success = (system(safe_cmd) >= 0);
        free(safe_cmd);
#elif defined(__unix__) || defined(__unix)
        char *safe_cmd = mallocx(sizeof(char) * (strlen(safe_url) + 32));
        *safe_cmd = 0;

        if(file_exists("/usr/bin/xdg-open"))
            sprintf(safe_cmd, "xdg-open \"%s\"", safe_url);
        else if(file_exists("/usr/bin/firefox"))
            sprintf(safe_cmd, "firefox \"%s\"", safe_url);
        else 
            success = false;

        if(*safe_cmd)
            success = (system(safe_cmd) >= 0);

        free(safe_cmd);
#else
#error "Unsupported operating system."
#endif
    }
    else {
        success = false;
        fatal_error("Can't launch url: invalid protocol (the valid ones are: http, https, ftp, mailto).\n%s", safe_url);
    }

    if(!success)
        logfile_message("Can't launch url: \"%s\"", safe_url);

    free(safe_url);
    return success;
}



/* private methods */

/* checks if the given absolute filepath exists */
bool file_exists(const char *filepath)
{
#if !defined(_WIN32)
    struct stat st;
    return (stat(filepath, &st) == 0);
#else
    FILE* fp = fopen(filepath, "rb");
    bool valid = false;
    if(fp != NULL) {
        valid = true;
        fclose(fp);
    }
    return valid;
#endif
}

/* converts ch from hex */
/*char hex2ch(char ch) {
    return isdigit(ch) ? (ch - '0') : ((char)toupper(ch) - 'A' + 10);
}*/

/* converts code to hex */
char ch2hex(int code) {
    static char hex[] = "0123456789ABCDEF";
    return hex[code & 0xF];
}

/* returns an url-encoded version of str */
char *url_encode(const char *str) {
    const char *p;
    char *buf = mallocx(sizeof(char) * (strlen(str) * 3 + 1)), *q = buf;

    for(p = str; *p; p++) {
        if(isalnum(*p) || *p == '-' || *p == '#' || *p == '_' || *p == '.' || *p == '~' || *p == ':' || *p == '?' || *p == '&' || *p == '/' || *p == '=' || *p == '+' || *p == '@')
            *q++ = *p; /* safety: we ensure that *p != '\\', *p != '\"' */
        else if(*p == ' ') 
            *q++ = '+';
        else 
            *q++ = '%', *q++ = ch2hex(*p / 16), *q++ = ch2hex(*p % 16);
    }

    *q = 0;
    return buf;
}

/* returns an url-decoded version of str */
/*char *url_decode(const char *str) {
    const char *p;
    char *buf = mallocx(sizeof(char) * (strlen(str) + 1)), *q = buf;

    for(p = str; *p; p++) {
        if(*p == '%') {
            if(*(p+1) && *(p+2)) {
                *q++ = (hex2ch(*(p+1)) << 4) | hex2ch(*(p+2));
                p += 2;
            }
        }
        else if(*p == '+')
            *q++ = ' ';
        else
            *q++ = *p;
    }

    *q = 0;
    return buf;
}*/
