/*
 * Open Surge Engine
 * osspec.c - OS Specific Routines
 * Copyright (C) 2009-2010, 2012-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <time.h>
#include <allegro.h>
#include "global.h"
#include "osspec.h"
#include "util.h"
#include "stringutil.h"
#include "video.h"



/* uncomment to disable case insensitive filename support
 * on platforms that do not support it (like *nix)
 * (see fix_case_path() for more information).
 *
 * Disabling this feature can speed up a little bit
 * the file searching routines on platforms like *nix
 * (it has no effect under Windows, though).
 *
 * Keeping this enabled provides case insensitive filename
 * support on platforms like *nix. */
/*#define DISABLE_FIX_CASE_PATH*/



/* uncomment to disable filepath optimizations - useful
 * on distributed file systems (example: nfs).
 *
 * Disabling this feature improves memory usage just
 * a little bit. You probably want to keep this
 * option enabled.
 *
 * Keeping this enabled improves drastically the
 * speed of the game when the files are stored
 * in a distributed file system like NFS. */
/*#define DISABLE_FILEPATH_OPTIMIZATIONS*/




#ifndef __WIN32__

#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#else

#include <winalleg.h>

#endif




/* private stuff */
static char *base_dir; /* absolute path to the base directory. It can be "" (read resources from install folder and from $HOME), or some other absolute path */

static void install_filepath(char *dest, const char *relativefp, size_t dest_size);
static void home_filepath(char *dest, const char *relativefp, size_t dest_size);
static int foreach_file(const char *wildcard, int (*callback)(const char *filename, void *param), void *param, int recursive);
static int directory_exists(const char *dirpath);
/*static void create_process(const char *path, int argc, char *argv[]);*/

#ifndef __WIN32__
static struct passwd *userinfo;
#endif

static char* fix_case_path(char *filepath);
static int fix_case_path_backtrack(const char *pwd, const char *remaining_path, const char *delim, char *dest);
static void search_the_file(char *dest, const char *relativefp, size_t dest_size);

typedef struct {
    int (*callback)(const char *filename, void *param);
    void *param;
} foreach_file_helper;

static int foreach_file_callback(const char *filename, int attrib, void *param);

#ifndef DISABLE_FILEPATH_OPTIMIZATIONS
/* cache stuff (also private): it's a basic dictionary */
static void cache_init(); /* hi! :-) */
static void cache_release(); /* bye! */
static char *cache_search(const char *key); /* searches for key */
static void cache_insert(const char *key, const char *value); /* new key */
static void cache_update(const char *key, const char *value); /* will update key, if it exists */

/* the cache is implemented as a simple binary tree */
typedef struct cache_t {
    char *key, *value;
    struct cache_t *left, *right;
} cache_t;
static cache_t *cache_root;
static cache_t *cachetree_release(cache_t *node);
static cache_t *cachetree_search(cache_t *node, const char *key);
static cache_t *cachetree_insert(cache_t *node, const char *key, const char *value);
#endif


/* expandable table of strings */
typedef struct xptable_t {
    int capacity, length;
    char **data;
} xptable_t;

static xptable_t *xptable_create();
static xptable_t *xptable_destroy(xptable_t *table);
static void xptable_add(xptable_t *table, const char *str); /* amortized cost: O(1) */
static int xptable_foreach(xptable_t *table, int (*callback)(const char *str, void *param), void *param); /* returns the number of successful calls to 'callback'; the callback must return 0 to let the enumeration proceed, or non-zero to stop it */


/* url encoding routines */
/*static char hex2ch(char ch);*/
static char ch2hex(char code);
static char *url_encode(const char *str);
/*static char *url_decode(const char *str);*/


/* misc */
static void list_directory_recursively(xptable_t *table, const char *wildcard);


/* public functions */

/* 
 * osspec_init()
 * Operating System Specifics - initialization
 * basedir is the absolute path to the base directory. It can be "" (read resources from install folder and from $HOME), or some other absolute path.
 */
void osspec_init(const char *basedir)
{
    /* base directory */
    base_dir = (basedir ? str_dup(basedir) : NULL);
    if(base_dir != NULL && !directory_exists(base_dir)) {
        fprintf(stderr, "ERROR: invalid base directory \"%s\"\n", base_dir);
        free(base_dir);
        exit(1);
    }

    /* stuff related to the $HOME folder */
#ifndef __WIN32__
    if(base_dir == NULL) {
        /* retrieving user data */
        if(NULL != (userinfo = getpwuid(getuid()))) {
            char *subdirs[] = {   /* subfolders at $HOME/.$GAME_UNIXNAME/ */
                "",
                "characters",
                "config",
                "fonts",
                "images",
                "languages",
                "levels",
                "licenses",
                "musics",
                "objects",
                "quests",
                "samples",
                "screenshots",
                "sprites",
                "themes",
                "ttf",
                NULL /* end of list */
            }, **p, tmp[1024];

            /* creating sub-directories */
            for(p = subdirs; *p; p++) {
                home_filepath(tmp, *p, sizeof(tmp));
                mkdir(tmp, 0755);
            }
        }
        else
            fprintf(stderr, "WARNING: couldn't obtain information about your user. User-specific data may not work.\n");
    }
    else
        userinfo = NULL;
#endif

#ifndef DISABLE_FILEPATH_OPTIMIZATIONS
    /* initializing the cache */
    cache_init();
#endif
}


/* 
 * osspec_release()
 * Operating System Specifics - release
 */
void osspec_release()
{
#ifndef DISABLE_FILEPATH_OPTIMIZATIONS
    /* releasing the cache */
    cache_release();
#endif

    /* base directory */
    if(base_dir != NULL)
        free(base_dir);
}


/*
 * filepath_exists()
 * Returns TRUE if the given file exists
 * or FALSE otherwise
 */
int filepath_exists(const char *filepath)
{
    return exists(filepath);
}



/*
 * directory_exists()
 * Returns TRUE if the given directory exists
 * or FALSE otherwise
 */
int directory_exists(const char *dirpath)
{
    return file_exists(dirpath, FA_DIREC | FA_HIDDEN | FA_RDONLY, NULL);
}


/*
 * foreach_resource()
 * Traverses a directory, calling callback on each resource file
 * wildcard must be a resource path, e.g.: "images / *.png"
 * Note: callback must return 0 to let the enumeration proceed, or non-zero to stop it
 * Returns the number of successful calls to callback.
 */
int foreach_resource(const char *wildcard, int (*callback)(const char *filename, void *param), void *param, int recursive)
{
    static char abs_path[2][1024];
    int j, max_paths, sum = 0;

    /* official and $HOME filepaths */
    install_filepath(abs_path[0], wildcard, sizeof(abs_path[0]));
    home_filepath(abs_path[1], wildcard, sizeof(abs_path[1]));
    max_paths = (strcmp(abs_path[0], abs_path[1]) == 0) ? 1 : 2;

    /* reading the parse tree */
    for(j=0; j<max_paths; j++)
        sum += foreach_file(abs_path[j], callback, param, recursive);

    /* done! ;) */
    return sum;
}


/*
 * resource_filepath()
 * Similar to install_filepath() and home_filepath(), but this routine
 * searches the specified file both in the home directory and in the
 * game directory
 */
void resource_filepath(char *dest, const char *relativefp, size_t dest_size, resfp_t mode)
{
    switch(mode) {
        /* I'll read the file */
        case RESFP_READ:
        {

#ifndef DISABLE_FILEPATH_OPTIMIZATIONS

            /* optimizations: without this, the game could become terribly slow
             * when the files are distributed over a network (example: nfs) */
            char *path;
            if(is_relative_filename(relativefp)) {
                if(NULL == (path=cache_search(relativefp))) {
                    /* I'll have to search the file... */
                    search_the_file(dest, relativefp, dest_size);

                    /* store the resulting filepath in the memory */
                    cache_insert(relativefp, dest);
                }
                else
                    str_cpy(dest, path, dest_size);
            }
            else
                search_the_file(dest, relativefp, dest_size);

#else

            search_the_file(dest, relativefp, dest_size);

#endif
            break;

        }

        /* I'll write to the file */
        case RESFP_WRITE:
        {
            FILE *fp;
            struct al_ffblk info;
            install_filepath(dest, relativefp, dest_size);

            if(al_findfirst(dest, &info, FA_HIDDEN) == 0) {
                /* the file exists AND it's NOT read-only */
                al_findclose(&info);
            }
            else {

                /* the file does not exist OR it is read-only */
                if(!filepath_exists(dest)) {

                    /* it doesn't exist */
                    if(NULL != (fp = fopen(dest, "w"))) {
                        /* is it writable? this shouldn't happen */
                        fclose(fp);
                        delete_file(dest);
                    }
                    else {
                        /* it's not writable */
                        home_filepath(dest, relativefp, dest_size);
                    }

                }
                else {
                    /* the file exists, but it's read-only */
                    home_filepath(dest, relativefp, dest_size);
                }

            }

#ifndef DISABLE_FILEPATH_OPTIMIZATIONS
            /* this is sooo important... */
            if(cache_search(relativefp) != NULL)
                cache_update(relativefp, dest);
            else
                cache_insert(relativefp, dest);
#endif
            break;
        }
    }
}



/*
 * create_process()
 * Creates a new process;
 *     path is the absolute path to the executable file
 *     argc is argument count
 *     argv[] contains the command line arguments
 *
 * NOTE: argv[0] also contains the absolute path of the executable
 */
/*
void create_process(const char *path, int argc, char *argv[])
{
#ifndef __WIN32__
    pid_t pid;

    argv[argc] = NULL;
    if(0 == (pid=fork()))
        execv(path, argv);
#else
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char cmd[10240]="";
    int i, is_file;

    for(i=0; i<argc; i++) {
        is_file = filepath_exists(argv[i]);
        if(is_file) strcat(cmd, "\"");
        strcat(cmd, argv[i]);
        strcat(cmd, is_file ? "\" " : " ");
    }
    cmd[strlen(cmd)-1] = '\0';

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if(!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBox(NULL, "Couldn't CreateProcess()", "Error", MB_OK);
        MessageBox(NULL, cmd, "Command Line", MB_OK);
        exit(1);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#endif
}
*/


/*
 * basename()
 * Finds out the filename portion of a completely specified file path
 */
char *basename(const char *path)
{
    return get_filename(path);
}




/*
 * launch_url()
 * Launches an URL using the default browser.
 * Returns TRUE on success.
 * Useful stuff: http://www.dwheeler.com/essays/open-files-urls.html
 */
int launch_url(const char *url)
{
    int ret = TRUE;
    char *safe_url = url_encode(url); /* it's VERY important to sanitize the URL... */

    if(video_is_fullscreen())
        video_changemode(video_get_resolution(), video_is_smooth(), FALSE);

    if(strncmp(safe_url, "http://", 7) == 0 || strncmp(safe_url, "https://", 8) == 0 || strncmp(safe_url, "ftp://", 6) == 0 || strncmp(safe_url, "mailto:", 7) == 0) {
#ifdef __WIN32__
        ShellExecute(NULL, "open", safe_url, NULL, NULL, SW_SHOWNORMAL);
#elif __APPLE__
        char *safe_cmd = mallocx(sizeof(char) * (strlen(safe_url) + 32));
        *safe_cmd = 0;

        if(filepath_exists("/usr/bin/open"))
            sprintf(safe_cmd, "open \"%s\"", safe_url);
        else 
            ret = FALSE; /* failure */

        if(*safe_cmd)
            ret = system(safe_cmd) >= 0;

        free(safe_cmd);
#else
        char *safe_cmd = mallocx(sizeof(char) * (strlen(safe_url) + 32));
        *safe_cmd = 0;

        if(filepath_exists("/usr/bin/xdg-open"))
            sprintf(safe_cmd, "xdg-open \"%s\"", safe_url);
        else if(filepath_exists("/usr/bin/firefox"))
            sprintf(safe_cmd, "firefox \"%s\"", safe_url);
        else 
            ret = FALSE; /* failure */

        if(*safe_cmd)
            ret = system(safe_cmd) >= 0;

        free(safe_cmd);
#endif
    }
    else {
        ret = FALSE;
        fatal_error("Can't launch url: invalid protocol (valid ones are: http, https, ftp, mailto).\n%s", safe_url);
    }

    free(safe_url);
    return ret;
}






/* private methods */


/*
 * foreach_file()
 * Traverses a directory, calling callback on each file
 * wildcard must be an absolute path, e.g.: "/ usr / share / opensurge / images / *.png"
 * Note: callback must return 0 to let the enumeration proceed, or non-zero to stop it
 * Returns the number of successful calls to callback.
 */
int foreach_file(const char *wildcard, int (*callback)(const char *filename, void *param), void *param, int recursive)
{
    if(recursive) {
        int sum = 0;
        xptable_t *table = xptable_create();
        list_directory_recursively(table, wildcard);
        sum += xptable_foreach(table, callback, param);
        xptable_destroy(table);
        return sum;
    }
    else {
        int deny_flags = FA_DIREC | FA_LABEL;
        foreach_file_helper h = { callback, param };
        return for_each_file_ex(wildcard, 0, deny_flags, foreach_file_callback, (void*)(&h));
    }
}

/*
 * install_filepath()
 * Converts a relative filepath into an
 * absolute filepath (in relation to the installation folder).
 */
void install_filepath(char *dest, const char *relativefp, size_t dest_size)
{
    /* did we receive a relative filepath? */
    if(is_relative_filename(relativefp)) {
        /* we don't need to deal with any strange base directory, do we? */
        if(base_dir == NULL) {
            static char executable_name[1024] = "";
            if(strcmp(executable_name, "") == 0)
                get_executable_name(executable_name, sizeof(executable_name));
#ifndef __WIN32__
            char *tmp = mallocx(sizeof(char) * (1 + max(strlen(executable_name), strlen(GAME_UNIX_EXECDIR))));
            strcpy(tmp, executable_name);
            tmp[ strlen(GAME_UNIX_EXECDIR) ] = '\0';
            if(strcmp(tmp, GAME_UNIX_EXECDIR) != 0) {
                str_cpy(dest, executable_name, dest_size);
                replace_filename(dest, dest, relativefp, dest_size);
            }
            else
                snprintf(dest, dest_size, "%s/%s", GAME_UNIX_INSTALLDIR, relativefp);
            free(tmp);
#else
            str_cpy(dest, executable_name, dest_size);
            replace_filename(dest, dest, relativefp, dest_size);
#endif
        }
        else {
            const char *dummy = "/" GAME_UNIXNAME;
            char *tmp = mallocx(sizeof(char) * (1 + strlen(base_dir) + strlen(dummy)));
            strcpy(tmp, base_dir);
            strcat(tmp, dummy);
            str_cpy(dest, tmp, dest_size);
            free(tmp);
            replace_filename(dest, dest, relativefp, dest_size);
        }
    }
    else
        str_cpy(dest, relativefp, dest_size); /* relativefp is already an absolute filepath */

    /* adjustments */
    fix_filename_slashes(dest);
    canonicalize_filename(dest, dest, dest_size);
    fix_case_path(dest);
}



/*
 * home_filepath()
 * Similar to install_filepath(), but this routine considers
 * the $HOME/.$GAME_UNIXNAME/ directory instead
 */
void home_filepath(char *dest, const char *relativefp, size_t dest_size)
{
#ifndef __WIN32__

    if(userinfo && base_dir == NULL) {
        snprintf(dest, dest_size, "%s/.%s/%s", userinfo->pw_dir, GAME_UNIXNAME, relativefp);
        fix_filename_slashes(dest);
        canonicalize_filename(dest, dest, dest_size);
        fix_case_path(dest);
    }
    else
        install_filepath(dest, relativefp, dest_size);

#else

    /* home_filepath() is not applicable on Windows */
    install_filepath(dest, relativefp, dest_size);

#endif
}






/* helps foreach_file() */
int foreach_file_callback(const char *filename, int attrib, void *param)
{
    foreach_file_helper *h = (foreach_file_helper*)param;
    return h->callback(filename, h->param);
}


/* lists the files whose names are matched by wildcard. We go deep inside every directory we find. */
void list_directory_recursively(xptable_t *table, const char *wildcard)
{
    struct al_ffblk info;
    char *q, *current_dir = str_dup(wildcard), *curinga = basename(wildcard), *ilovesurge;

    /* sweet strings */
    if(NULL != (q = strstr(current_dir, curinga)))
        *q = 0;

    ilovesurge = mallocx((strlen(current_dir) + 2) * sizeof(*ilovesurge));
    strcpy(ilovesurge, current_dir);
    strcat(ilovesurge, "*"); /* example: "/home/alexandre/.opensurge/sprites/ *" */

    /* list the files matching the wildcard */
    if(al_findfirst(wildcard, &info, FA_ALL/* & ~FA_LABEL & ~FA_DIREC*/) == 0) {
        do {
            if(!(info.attrib & FA_DIREC) && !(info.attrib & FA_LABEL)) { /* why do I need to check this? can't trust Allegro on this one? */
                char *file = mallocx((strlen(current_dir) + strlen(info.name) + 1) * sizeof(*file));
                strcpy(file, current_dir);
                strcat(file, info.name);
                xptable_add(table, file);
                free(file);
            }
        } while(al_findnext(&info) == 0);
        al_findclose(&info);
    }

    /* look inside the directories, recursively */
    if(al_findfirst(ilovesurge, &info, FA_ALL/*FA_DIREC*/) == 0) { /* FA_DIREC doesn't work on BSD?? */
        do {
            if((info.attrib & FA_DIREC) && strcmp(info.name, "") != 0 && strcmp(info.name, ".") != 0 && strcmp(info.name, "..") != 0) {
                char *new_wildcard = mallocx((strlen(current_dir) + strlen(info.name) + strlen(curinga) + 2) * sizeof(*new_wildcard));
                strcpy(new_wildcard, current_dir);
                strcat(new_wildcard, info.name);
                strcat(new_wildcard, "/");
                strcat(new_wildcard, curinga);
                list_directory_recursively(table, new_wildcard);
                free(new_wildcard);
            }
        } while(al_findnext(&info) == 0);
        al_findclose(&info);
    }

    free(ilovesurge);
    free(current_dir);
}

/* backtracking routine used in fix_case_path()
 * returns TRUE iff a solution is found */
int fix_case_path_backtrack(const char *pwd, const char *remaining_path, const char *delim, char *dest)
{
    char *query, *pos, *p, *tmp, *new_pwd;
    const char *q;
    struct al_ffblk info;
    int ret = FALSE;

    if(NULL != (pos = strchr(remaining_path, *delim))) {
        /* if remaning_path is "my/example/query", then
           query is equal to "my" */
        query = mallocx(sizeof(char) * (pos-remaining_path+1));
        for(p=query,q=remaining_path; q!=pos;)
            *p++ = *q++;
        *p = 0;

        /* next folder... */
        tmp = mallocx(sizeof(char) * (strlen(pwd)+2));
        sprintf(tmp, "%s*", pwd);
        if(al_findfirst(tmp, &info, FA_ALL) == 0) {
            do {
                if(str_icmp(query, info.name) == 0) {
                    new_pwd = mallocx(sizeof(char) * (strlen(pwd)+strlen(query)+strlen(delim)+1));
                    sprintf(new_pwd, "%s%s%s", pwd, info.name, delim);
                    ret = fix_case_path_backtrack(new_pwd, pos+1, delim, dest);
                    free(new_pwd);
                    if(ret) break;
                }
            }
            while(al_findnext(&info) == 0);
            al_findclose(&info);
        }
        free(tmp);

        /* releasing resources */
        free(query);
    }
    else {
        /* no more subdirectories */
        tmp = mallocx(sizeof(char) * (strlen(pwd)+2));
        sprintf(tmp, "%s*", pwd);
        if(al_findfirst(tmp, &info, FA_ALL) == 0) {
            do {
                if(str_icmp(remaining_path, info.name) == 0) {
                    ret = TRUE;
                    sprintf(dest, "%s%s", pwd, info.name);
                    break;
                }
            }
            while(al_findnext(&info) == 0);
            al_findclose(&info);
        }
        free(tmp);
    }

    /* done */
    return ret;
}


/* Case-insensitive filename support for all platforms.
 *
 * If the user requests for the file "LEVELS/MyLevel.lev", but
 * only "levels/mylevel.lev" exists, the valid filepath will
 * be used. This routine does nothing on Windows.
 *
 * filepath may be modified during the process. A copy of it
 * is returned. */
char* fix_case_path(char *filepath)
{
#if !defined(DISABLE_FIX_CASE_PATH) && !defined(__WIN32__)
    char *tmp;
    const char delimiter[] = "/";
    int solved = FALSE;

    if(!filepath_exists(filepath)) {
        fix_filename_slashes(filepath);
        tmp = mallocx(sizeof(char) * (strlen(filepath)+1));

        if(*filepath == *delimiter)
            solved = fix_case_path_backtrack(delimiter, filepath+1, delimiter, tmp);
        else
            solved = fix_case_path_backtrack("", filepath, delimiter, tmp);

        if(solved)
            strcpy(filepath, tmp);

        free(tmp);
    }
#endif

    return filepath;
}


/* auxiliary routine for resource_filepath(): given any filepath (relative or absolute),
 * finds the absolute path (either in the home directory or in the game directory) */
void search_the_file(char *dest, const char *relativefp, size_t dest_size)
{
    char *home_path = mallocx(dest_size * sizeof(*dest));
    char *install_path = mallocx(dest_size * sizeof(*dest));

    home_filepath(home_path, relativefp, dest_size);
    install_filepath(install_path, relativefp, dest_size);

    if(filepath_exists(home_path) || directory_exists(home_path)) {
        if(filepath_exists(install_path) || directory_exists(install_path)) {
            if(difftime(file_time(install_path), file_time(home_path)) > 0)
                strcpy(dest, install_path);
            else
                strcpy(dest, home_path);
        }
        else
            strcpy(dest, home_path);
    }
    else
        strcpy(dest, install_path);

    free(install_path);
    free(home_path);
}








/* url encoding-decoding routines */

/* converts ch from hex */
/*char hex2ch(char ch) {
    return isdigit(ch) ? (ch - '0') : ((char)toupper(ch) - 'A' + 10);
}*/

/* converts code to hex */
char ch2hex(char code) {
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
            *q++ = '%', *q++ = ch2hex(*p >> 4), *q++ = ch2hex(*p & 0xF);
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



#ifndef DISABLE_FILEPATH_OPTIMIZATIONS
/* ------- cache interface -------- */

/* initializes the cache */
void cache_init()
{
    cache_root = NULL;
}

/* releases the cache */
void cache_release()
{
    cache_root = cachetree_release(cache_root);
}

/* finds a string in the dictionary */
char *cache_search(const char *key)
{
    cache_t *node = cachetree_search(cache_root, key);
    return node ? node->value : NULL;
}

/* inserts a string into the dictionary */
void cache_insert(const char *key, const char *value)
{
    cache_root = cachetree_insert(cache_root, key, value);
}

/* will update an entry of the dictionary, if that entry exists */
void cache_update(const char *key, const char *value)
{
    cache_t *node = cachetree_search(cache_root, key);
    if(node != NULL) {
        free(node->value);
        node->value = str_dup(value);
    }
}

/* ------ cache implementation --------- */
cache_t *cachetree_release(cache_t *node)
{
    if(node) {
        node->left = cachetree_release(node->left);
        node->right = cachetree_release(node->right);
        free(node->key);
        free(node->value);
        free(node);
    }

    return NULL;
}

cache_t *cachetree_search(cache_t *node, const char *key)
{
    int cmp;

    if(node) {
        cmp = strcmp(key, node->key);

        if(cmp < 0)
            return cachetree_search(node->left, key);
        else if(cmp > 0)
            return cachetree_search(node->right, key);
        else
            return node;
    }
    else
        return NULL;
}

cache_t *cachetree_insert(cache_t *node, const char *key, const char *value)
{
    int cmp;
    cache_t *t;

    if(node) {
        cmp = strcmp(key, node->key);

        if(cmp < 0)
            node->left = cachetree_insert(node->left, key, value);
        else if(cmp > 0)
            node->right = cachetree_insert(node->right, key, value);

        return node;
    }
    else {
        t = mallocx(sizeof *t);
        t->key = str_dup(key);
        t->value = str_dup(value);
        t->left = t->right = NULL;
        return t;
    }
}
#endif





/* ----------- expandable table of strings ---------------- */

xptable_t *xptable_create()
{
    xptable_t *table = mallocx(sizeof *table);
    table->capacity = 32;
    table->length = 0;
    table->data = malloc(table->capacity * sizeof(char*));
    return table;
}

xptable_t *xptable_destroy(xptable_t *table)
{
    int i;

    for(i=0; i<table->length; i++)
        free(table->data[i]);
    free(table->data);
    free(table);

    return NULL;
}

void xptable_add(xptable_t *table, const char *str)
{
    if(table->length == table->capacity) {
        table->capacity *= 2;
        table->data = reallocx(table->data, table->capacity * sizeof(char*));
    }

    table->data[ (table->length)++ ] = str_dup(str);
}

int xptable_foreach(xptable_t *table, int (*callback)(const char *str, void *param), void *param)
{
    int i;

    for(i=0; i<table->length; i++) {
        if(callback(table->data[i], param) != 0)
            return i;
    }

    return table->length;
}
