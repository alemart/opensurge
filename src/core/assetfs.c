/*
 * Open Surge Engine
 * assetfs.c - assetfs virtual filesystem
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "darray.h"
#include "whereami/whereami.h"
#include "assetfs.h"
#include "util.h"
#include "logfile.h"
#include "global.h"

/* OS-specific includes */
#if defined(_WIN32)
#include <windows.h>
#include <wchar.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

/* Base folder */
#define ASSETS_BASEDIR    "opensurge2d"

/* aliases */
#define assetfs_log       logfile_message
#define assetfs_fatal     fatal_error

/* Defining the filesystem */
typedef struct assetfile_t assetfile_t;
typedef struct assetdir_t assetdir_t;
typedef struct assetdirentry_t assetdirentry_t;

struct assetfile_t
{
    char* name; /* filename.ext */
    char* fullpath; /* absolute filepath in the real filesystem */
};

struct assetdirentry_t
{
    char* name; /* dirname */
    assetdir_t* contents; /* dirlink */
};

struct assetdir_t
{
    DARRAY(assetdirentry_t, dir); /* subdirs */
    DARRAY(assetfile_t*, file); /* files */
};

static assetdir_t* root = NULL;

/* internal functions */
static bool is_valid_id(const char* str);
static inline char* clone_str(const char* str);
static inline char* join_path(const char* path, const char* basename);
static inline char* pathify(const char* path);
static inline int vpathcmp(const char* vp1, const char* vp2);
static inline int vpathncmp(const char* vp1, const char* vp2, int n);
static inline int vpc(int c) { return (c == '\\') ? '/' : tolower(c); }
static bool is_asset_folder(const char* path);
static void scan_default_folders(const char* gameid);
static bool scan_exedir(assetdir_t* dir);
static void scan_folder(assetdir_t* dir, const char* abspath);
static int dircmp(const void* a, const void* b);
static int filecmp(const void* a, const void* b);
static int filematch(const void* key, const void* f);

static assetdir_t* afs_mkdir(assetdir_t* parent, const char* dirname);
static assetdir_t* afs_rmdir(assetdir_t* base);
static assetdir_t* afs_finddir(assetdir_t* base, const char* dirpath);
static assetfile_t* afs_mkfile(const char* filename, const char* fullpath);
static assetfile_t* afs_rmfile(assetfile_t* file);
static void afs_storefile(assetdir_t* dir, assetfile_t* file);
static assetfile_t* afs_findfile(assetdir_t* dir, const char* vpath);
static int afs_foreach(assetdir_t* dir, const char* extension_filter, int (*callback)(const char* fullpath, void* param), void* param, bool recursive, bool* stop);
static void afs_sort(assetdir_t* base);
static bool afs_empty(assetdir_t* base);



/* public API */

/*
 * assetfs_init()
 * Initializes the asset filesystem
 * gameid: a string (lowercase letters, numbers), or NULL if we have specified no custom gameid
 * datadir: an absolute filepath, or NULL to look for the assets in default places
 */
void assetfs_init(const char* gameid, const char* datadir)
{
    /* create the root */
    root = afs_mkdir(NULL, ".");

    /* scan the assets */
    gameid = gameid && *gameid ? gameid : GAME_UNIXNAME;
    if(is_valid_id(gameid)) {
        assetfs_log("Loading assets for %s...", gameid);
        if(datadir && *datadir) {
            if(!is_asset_folder(datadir))
                assetfs_log("Custom asset folder \"%s\" is either invalid or obsolete.", datadir);
            scan_folder(root, datadir);
        }
        else
            scan_default_folders(gameid);
    }
    else
        assetfs_fatal("Can't scan assets: invalid gameid \"%s\". Please use only lowercase letters / digits.", gameid);

    /* validate */
    if(afs_empty(root))
        assetfs_fatal("Can't load %s: assets not found.", gameid);

    /* sort the entries */
    afs_sort(root);
}

/*
 * assetfs_release()
 * Releases the asset filesystem
 */
void assetfs_release()
{
    afs_rmdir(root);
}

/*
 * assetfs_fullpath()
 * Return the absolute filepath related to the given virtual path
 */
const char* assetfs_fullpath(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);

    if(file == NULL) {
        assetfs_fatal("Can't find asset \"%s\"", vpath);
        return NULL;
    }

    return file->fullpath;
}


/*
 * assetfs_exists()
 * Checks if an asset exists in the virtual filesystem
 */
bool assetfs_exists(const char* vpath)
{
    return afs_findfile(root, vpath) != NULL;
}

/*
 * assetfs_foreach_file()
 * Executes a callback for each file in a virtual folder (path)
 * 1. extension filter may be NULL or ".png", ".ss", and so on...
 * 2. callback must return 0 to let the enumeration proceed, or non-zero to stop it
 */
int assetfs_foreach_file(const char* vpath_of_dir, const char* extension_filter, int (*callback)(const char* fullpath, void* param), void* param, bool recursive)
{
    assetdir_t* dir = afs_finddir(root, vpath_of_dir);
    return (dir != NULL) ? afs_foreach(dir, extension_filter, callback, param, recursive, NULL) : 0;
}


/* internals */

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

/* duplicates a string */
char* clone_str(const char* str)
{
    if(str != NULL)
        return strcpy(mallocx((1 + strlen(str)) * sizeof(char)), str);
    else
        return strcpy(mallocx(1 * sizeof(char)), "");
}

/* combines a path to a basename; you must free() this afterwards */
char* join_path(const char* path, const char* basename)
{
    if(path && *path) {
        char *s = mallocx((2 + strlen(path) + strlen(basename)) * sizeof(*s));
        strcpy(s, path);
#if !defined(_WIN32)
        if(s[strlen(s)-1] != '/')
            strcat(s, "/");
#else
        if(s[strlen(s)-1] != '\\')
            strcat(s, "\\");
#endif
        strcat(s, basename);
        return s;
    }
    else
        return clone_str(basename);
}

/* creates a directory at a certain location, returning it */
assetdir_t* afs_mkdir(assetdir_t* parent, const char* dirname)
{
    assetdir_t* dir = mallocx(sizeof *dir);
    assetdirentry_t self = { .name = clone_str("."), .contents = dir };
    assetdirentry_t back = { .name = clone_str(".."), .contents = parent ? parent : dir };

    darray_init(dir->file);
    darray_init(dir->dir);
    darray_push(dir->dir, self);
    darray_push(dir->dir, back);
    if(parent != NULL) {
        assetdirentry_t child = { .name = clone_str(dirname), .contents = dir };
        darray_push(parent->dir, child);
    }

    return dir;
}

/* removes the given directory */
assetdir_t* afs_rmdir(assetdir_t* dir)
{
    int i;

    for(i = 0; i < darray_length(dir->file); i++)
        afs_rmfile(dir->file[i]);
    darray_release(dir->file);

    for(i = 0; i < darray_length(dir->dir); i++) {
        if(0 != strcmp(dir->dir[i].name, ".") && 0 != strcmp(dir->dir[i].name, ".."))
            afs_rmdir(dir->dir[i].contents);
        free(dir->dir[i].name);
    }
    darray_release(dir->dir);

    free(dir);
    return NULL;
}

/* finds the virtual dir of a virtual path (returns NULL if not found) */
assetdir_t* afs_finddir(assetdir_t* dir, const char* vpath)
{
    const char* q = strpbrk(vpath, "/\\");
    if(q != NULL) {
        /* we're looking for a subfolder */
        if(q != vpath) {
            int len = q - vpath;
            for(int i = 0; i < darray_length(dir->dir); i++) {
                if(0 == vpathncmp(vpath, dir->dir[i].name, len) && len == strlen(dir->dir[i].name))
                    return afs_finddir(dir->dir[i].contents, q+1);
            }
            return NULL;
        }
        else
            return afs_finddir(dir, q+1);
    }
    else if(*vpath != '\0') {
        /* this is the folder we must look at */
        for(int i = 0; i < darray_length(dir->dir); i++) {
            if(0 == vpathcmp(vpath, dir->dir[i].name))
                return dir->dir[i].contents;
        }
        return NULL;
    }
    else {
        /* empty vpath (e.g., "images/") */
        return dir;
    }
}


/* finds a virtual file in a virtual dir (returns NULL if not found) */
assetfile_t* afs_findfile(assetdir_t* dir, const char* vpath)
{
    assetfile_t* file = NULL;
    assetdir_t* filedir = NULL;
    char* path = pathify(vpath), *filename, *q;

    /* find filename & its directory */
    if(NULL != (q = strrchr(path, '/'))) {
        *q = '\0';
        filename = q+1;
        filedir = afs_finddir(dir, path);
    }
    else {
        filename = path;
        filedir = dir;
    }

    /* locate the file */
    if(filedir != NULL) {
        assetfile_t** entry = bsearch(filename, filedir->file, darray_length(filedir->file), sizeof(assetfile_t*), filematch);
        file = (entry != NULL) ? *entry : NULL;
    }

    /* done */
    free(path);
    return file;
}

/* list files */
int afs_foreach(assetdir_t* dir, const char* extension_filter, int (*callback)(const char* fullpath, void* param), void* param, bool recursive, bool* stop)
{
    int count = 0;
    const char* q;

    /* foreach file */
    for(int i = 0; i < darray_length(dir->file); i++) {
        if(!extension_filter || ((q = strrchr(dir->file[i]->name, '.')) && 0 == vpathcmp(q, extension_filter))) {
            count++;
            if(0 != callback(dir->file[i]->fullpath, param)) {
                /* stop the enumeration */
                if(stop != NULL)
                    *stop = true;
                return count;
            }
        }
    }

    /* foreach folder */
    if(recursive) {
        for(int j = 0; j < darray_length(dir->dir); j++) {
            if(0 != strcmp(dir->dir[j].name, ".") && 0 != strcmp(dir->dir[j].name, "..")) {
                bool failed = false;
                count += afs_foreach(dir->dir[j].contents, extension_filter, callback, param, recursive, &failed);
                if(failed) {
                    /* stop the enumeration */
                    if(stop != NULL)
                        *stop = true;
                    return count;
                }
            }
        }
    }

    /* done! */
    return count;
}



/* creates a new virtual file */
assetfile_t* afs_mkfile(const char* filename, const char* fullpath)
{
    assetfile_t* file = mallocx(sizeof *file);
    file->name = clone_str(filename);
    file->fullpath = clone_str(fullpath);
    return file;
}

/* removes an existing virtual file */
assetfile_t* afs_rmfile(assetfile_t* file)
{
    free(file->fullpath);
    free(file->name);
    free(file);
    return NULL;
}

/* stores a virtual file in a virtual directory */
void afs_storefile(assetdir_t* dir, assetfile_t* file)
{
    darray_push(dir->file, file);
}

/* replace backslashes by slashes; you'll have to free this string afterwards */
char* pathify(const char* path)
{
    const char* p = path;
    char* buf = mallocx((1 + strlen(path)) * sizeof(*buf));
    char* q = buf, c;

    while((c = *(p++)))
        *(q++) = (c == '\\') ? '/' : c;
    *q = '\0';

    return buf;
}

/* returns 0 if the two vpaths are equal */
int vpathcmp(const char* vp1, const char* vp2)
{
    int delta = 0;
    const unsigned char* a = (const unsigned char*)vp1;
    const unsigned char* b = (const unsigned char*)vp2;
    while(0 == (delta = vpc(*a) - vpc(*b)) && *a++ && *b++);
    return delta;
}

/* compares two vpaths up to n characters */
int vpathncmp(const char* vp1, const char* vp2, int n)
{
    int delta = 0;
    const unsigned char* a = (const unsigned char*)vp1;
    const unsigned char* b = (const unsigned char*)vp2;
    while(n-- > 0 && 0 == (delta = vpc(*a) - vpc(*b)) && *a++ && *b++);
    return delta;
}




/* OS-specific functions */
#if !defined(_WIN32)
/* recursive mkdir(): give the absolute path of a file
   this will create the filepath for you */
static int mkpath(char* path, mode_t mode)
{   
    char* p;

    if(path == NULL || *path == '\0')
        return 0;

    for(p = strchr(path+1, '/'); p != NULL; p = strchr(p+1, '/')) {
        *p = '\0';
        if(mkdir(path, mode) != 0) {
            if(errno != EEXIST) {
                *p = '/';
                assetfs_log("Can't mkpath \"%s\": %s", path, strerror(errno));
                return -1;
            }
        }
        *p = '/';
    }

    return 0;
}
#endif


/* checks if a certain folder (given its absolute path) is a valid
 * opensurge asset folder */
bool is_asset_folder(const char* path)
{
    char* fpath = join_path(path, "surge.rocks");
    bool valid = false;
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    struct stat st;
    valid = (stat(fpath, &st) == 0);
#else
    FILE* fp = fopen(fpath, "rb");
    if(fp != NULL) {
        valid = true;
        fclose(fp);
    }
#endif
    if(!valid)
        assetfs_log("Not an asset folder: \"%s\"", path);
    free(fpath);
    return valid;
}



/* scans the default folders that store game assets */
void scan_default_folders(const char* gameid)
{
#if defined(_WIN32)
    assetfs_log("Scanning assets...");

    /* scan asset folder: <exedir> */
    if(!scan_exedir(root)) {
        const char* path = ".";
        assetfs_log("Can't find the application folder: scanning the working dir");
        if(is_asset_folder(path))
            scan_folder(root, path);
    }
#elif defined(__APPLE__) && defined(__MACH__)
    /* FIXME: untested */
    struct passwd* userinfo = NULL;
    bool must_scan_unixdir = true;
    assetfs_log("Scanning assets...");

    /* scan primary asset folder: <exedir>/../Resources */
    if(strcmp(gameid, GAME_UNIXNAME) == 0) {
        if(scan_exedir(root))
            must_scan_unixdir = false;
    }

    /* scan additional asset folder: ~/Library/<basedir>/<gameid> */
    if(NULL != (userinfo = getpwuid(getuid()))) {
        const char* path = "/Library/" ASSETS_BASEDIR "/", *p;
        DARRAY(char, buf);
        darray_init(buf);

        for(p = userinfo->pw_dir; *p; p++)
            darray_push(buf, *p);
        for(p = path; *p; p++)
            darray_push(buf, *p);
        for(p = gameid; *p; p++)
            darray_push(buf, *p);
        darray_push(buf, '/'); /* used by mkpath() */
        darray_push(buf, '\0');

        mkpath((char*)buf, 0755);
        scan_folder(root, (const char*)buf);
        darray_release(buf);
    }
    else
        assetfs_log("Can't find home directory: additional game assets may not be loaded");

    /* fill-in any missing files (if custom gameid) for compatibility purposes */
    if(strcmp(gameid, GAME_UNIXNAME) != 0) {
        if(scan_exedir(root))
            must_scan_unixdir = false;
    }

    /* scan <unixdir> */
    if(must_scan_unixdir) {
        if(is_asset_folder(GAME_DATADIR))
            scan_folder(root, GAME_DATADIR);
        else
            assetfs_fatal("Can't load %s: assets not found in %s", gameid, GAME_DATADIR);
    }
#elif defined(__unix__) || defined(__unix)
    const char *home = NULL, *path = NULL;
    bool must_scan_unixdir = true;
    assetfs_log("Scanning assets...");

    /* scan primary asset folder: <exedir> */
    if(strcmp(gameid, GAME_UNIXNAME) == 0) { /* gameid is provisory (should come from the command-line) */
        if(scan_exedir(root))
            must_scan_unixdir = false;
    }

    /* scan additional asset folder: $XDG_DATA_HOME/<basedir>/<gameid> */
    if(NULL == (home = getenv("XDG_DATA_HOME"))) {
        struct passwd* userinfo = NULL;
        if(NULL != (userinfo = getpwuid(getuid()))) {
            home = userinfo->pw_dir;
            path = "/.local/share/" ASSETS_BASEDIR "/";
        }
    }
    else
        path = "/" ASSETS_BASEDIR "/";

    if(home != NULL && path != NULL) {
        const char* p;
        DARRAY(char, buf);
        darray_init(buf);

        for(p = home; *p; p++)
            darray_push(buf, *p);
        for(p = path; *p; p++)
            darray_push(buf, *p);
        for(p = gameid; *p; p++)
            darray_push(buf, *p);
        darray_push(buf, '/'); /* used by mkpath() */
        darray_push(buf, '\0');

        mkpath((char*)buf, 0755);
        scan_folder(root, (const char*)buf);
        darray_release(buf);
    }

    /* fill-in any missing files (if custom gameid) for compatibility purposes */
    if(strcmp(gameid, GAME_UNIXNAME) != 0) {
        if(scan_exedir(root))
            must_scan_unixdir = false;
    }

    /* scan <unixdir> */
    if(must_scan_unixdir) {
        if(is_asset_folder(GAME_DATADIR))
            scan_folder(root, GAME_DATADIR);
        else
            assetfs_fatal("Can't load %s: assets not found in %s", gameid, GAME_DATADIR);
    }
#else
#error "Unsupported operating system."
#endif
}

/* scan the <exedir>; returns true if it's an asset folder */
bool scan_exedir(assetdir_t* dir)
{
#if defined(__APPLE__) && defined(__MACH__)
    int len, dirlen = 0;
    bool success = false;

    if((len = wai_getExecutablePath(NULL, 0, NULL)) >= 0) {
        char* data_path = NULL;
        char* exedir = mallocx((1 + len) * sizeof(*exedir));
        wai_getExecutablePath(exedir, len, &dirlen);
        exedir[dirlen] = '\0';
        data_path = join_path(exedir, "../Resources"); /* use realpath() */
        if(is_asset_folder(data_path)) {
            success = true;
            scan_folder(dir, data_path);
        }
        free(exedir);
        free(data_path);
    }
    else
        assetfs_log("Can't find the application folder: game assets may not be loaded");

    return success;
#else
    int len, dirlen = 0;
    bool success = false;

    if((len = wai_getExecutablePath(NULL, 0, NULL)) >= 0) {
        char* exedir = mallocx((1 + len) * sizeof(*exedir));
        wai_getExecutablePath(exedir, len, &dirlen);
        exedir[dirlen] = '\0';
        if(is_asset_folder(exedir)) {
            success = true;
            scan_folder(dir, exedir);
        }
        free(exedir);
    }
    else
        assetfs_log("Can't find the application folder: game assets may not be loaded");

    return success;
#endif
}

/* scan a specific asset folder */
void scan_folder(assetdir_t* folder, const char* abspath)
{
    /* for debugging purposes */
    if(folder == root)
        assetfs_log("Scanning \"%s\"...", abspath);

    do {
#if defined(_WIN32)
    WIN32_FIND_DATAW fd; HANDLE h;
    char* dirpath = join_path(abspath, "*");
    int size = MultiByteToWideChar(CP_UTF8, 0, dirpath, -1, NULL, 0);
    if(size > 0) {
        wchar_t* dirpathw = mallocx(size * sizeof(*dirpathw));
        MultiByteToWideChar(CP_UTF8, 0, dirpath, -1, dirpathw, size);
        if((h = FindFirstFileW(dirpathw, &fd)) != INVALID_HANDLE_VALUE) {
            do {
                if((size = WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, NULL, 0, NULL, NULL)) > 0) {
                    char* filename = mallocx(size * sizeof(*filename));
                    WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, filename, size, NULL, NULL);
                    if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        /* recurse on directories */
                        if(0 != strcmp(filename, ".") && 0 != strcmp(filename, "..")) {
                            char* path = join_path(abspath, filename);
                            assetdir_t* subfolder = afs_finddir(folder, filename);
                            subfolder = subfolder ? subfolder : afs_mkdir(folder, filename);
                            scan_folder(subfolder, path);
                            free(path);
                        }
                    }
                    else {
                        /* scan the asset */
                        if(!afs_findfile(folder, filename)) { /* no duplicate files */
                            char* path = join_path(abspath, filename);
                            afs_storefile(folder, afs_mkfile(filename, path));
                            free(path);
                        }
                    }
                    free(filename);
                }
            } while(FindNextFileW(h, &fd));
            if(!FindClose(h))
                assetfs_log("Can't close \"%s\": error code %d", abspath, GetLastError());
        }
        else
            assetfs_log("Can't scan \"%s\": error code %d", abspath, GetLastError());
        free(dirpathw);
    }
    else
        assetfs_log("Can't scan \"%s\": MultiByteToWideChar error (%d)", abspath, GetLastError());
    free(dirpath);
#else
    DIR* dir = opendir(abspath);
    if(dir != NULL) {
        struct dirent* d;
        while((d = readdir(dir))) {
            bool is_dir = false;
            bool is_file = false;

            /* check entry type */
            #ifdef _DIRENT_HAVE_D_TYPE
            if(d->d_type != DT_UNKNOWN) {
                is_dir = (d->d_type == DT_DIR) && (d->d_type != DT_LNK); /* d'uh */
                is_file = (d->d_type == DT_REG) || (d->d_type == DT_LNK);
            }
            else {
            #endif
                char* path = join_path(abspath, d->d_name);
                struct stat st;
                lstat(path, &st);
                is_dir = S_ISDIR(st.st_mode) && !(S_ISLNK(st.st_mode));
                is_file = S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
                free(path);
            #ifdef _DIRENT_HAVE_D_TYPE
            }
            #endif

            /* recurse on directories */
            if(is_dir) {
                if(0 != strcmp(d->d_name, ".") && 0 != strcmp(d->d_name, "..")) {
                    char* path = join_path(abspath, d->d_name);
                    assetdir_t* subfolder = afs_finddir(folder, d->d_name);
                    subfolder = subfolder ? subfolder : afs_mkdir(folder, d->d_name);
                    scan_folder(subfolder, path);
                    free(path);
                }
            }

            /* scan the asset */
            else if(is_file) {
                if(!afs_findfile(folder, d->d_name)) { /* no duplicate files */
                    char* path = join_path(abspath, d->d_name);
                    afs_storefile(folder, afs_mkfile(d->d_name, path));
                    free(path);
                }
            }
        }
        if(closedir(dir))
            assetfs_log("Can't close \"%s\": %s", abspath, strerror(errno));
    }
    else
        assetfs_log("Can't scan \"%s\": %s", abspath, strerror(errno));
#endif
    } while(0);
}

/* checks if a directory is empty */
bool afs_empty(assetdir_t* base)
{
    int i;

    if(darray_length(base->file) > 0)
        return false;

    if(darray_length(base->dir) > 2) /* skip "." and ".." */
        return false;

    for(i = 0; i < darray_length(base->dir); i++) { /* just in case... */
        if(0 != strcmp(base->dir[i].name, ".") && 0 != strcmp(base->dir[i].name, ".."))
            return false;
    }

    return true;
}

/* sort the entries of the directory, recursively */
void afs_sort(assetdir_t* base)
{
    int i;

    qsort(base->file, darray_length(base->file), sizeof(assetfile_t*), filecmp);
    qsort(base->dir, darray_length(base->dir), sizeof(assetdirentry_t), dircmp);
    for(i = 0; i < darray_length(base->dir); i++) {
        if(0 != strcmp(base->dir[i].name, ".") && 0 != strcmp(base->dir[i].name, ".."))
            afs_sort(base->dir[i].contents);
    }
}

/* compare two assetdirentry_t*'s */
int dircmp(const void* a, const void* b)
{
    return vpathcmp(((assetdirentry_t*)a)->name, ((assetdirentry_t*)b)->name);
}

/* compare two assetfile_t*'s */
int filecmp(const void* a, const void* b)
{
    return vpathcmp((*((assetfile_t**)a))->name, (*((assetfile_t**)b))->name);
}

/* checks if the assetfile_t* matches the key */
int filematch(const void* key, const void* f)
{
    return vpathcmp((const char*)key, (*((assetfile_t**)f))->name);
}
