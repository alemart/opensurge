/*
 * Open Surge Engine
 * assetfs.c - assetfs virtual filesystem
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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
#include <direct.h>
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
typedef enum assetfiletype_t assetfiletype_t;
typedef enum assetpriority_t assetpriority_t;
typedef struct assetfile_t assetfile_t;
typedef struct assetdir_t assetdir_t;
typedef struct assetdirentry_t assetdirentry_t;

enum assetfiletype_t
{
    ASSET_DATA,    /* game data (usually reaonly) */
    ASSET_CONFIG,  /* configuration file */
    ASSET_CACHE    /* non-essential data (logs, etc.) */
};

enum assetpriority_t
{
    ASSET_PRIMARY,    /* asset comes from a primary source */
    ASSET_SECONDARY   /* complimentary asset for compatibility purposes */
};

struct assetfile_t
{
    char* name; /* filename.ext */
    char* fullpath; /* absolute filepath in the real filesystem */
    assetfiletype_t type; /* file type */
    assetpriority_t priority; /* asset priority */
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
static char* afs_gameid = NULL;

/* internal functions */
static bool is_valid_id(const char* str);
static bool is_sane_vpath(const char* vpath);
static bool is_asset_folder(const char* fullpath);
static bool is_writable_file(const char* fullpath);
static inline char* clone_str(const char* str);
static inline char* join_path(const char* path, const char* basename);
static inline char* pathify(const char* path);
static inline int vpathcmp(const char* vp1, const char* vp2);
static inline int vpathncmp(const char* vp1, const char* vp2, int n);
static inline int vpc(int c) { return (c == '\\') ? '/' : tolower(c); }
static void scan_default_folders(const char* gameid, const char* basedir);
static bool scan_exedir(assetdir_t* dir, assetpriority_t priority);
static void scan_folder(assetdir_t* dir, const char* abspath, assetfiletype_t type, assetpriority_t priority);
static int dircmp(const void* a, const void* b);
static int filecmp(const void* a, const void* b);
static int filematch(const void* key, const void* f);
static const char* vpathbasename(const char* vpath, int* dirname_length);
static char* dir2vpath(assetdir_t* dir);
static char* build_config_fullpath(const char* gameid, const char* vpath);
static char* build_userdata_fullpath(const char* gameid, const char* vpath);
static char* build_cache_fullpath(const char* gameid, const char* vpath);
#if !defined(_WIN32)
static int mkpath(char* path, mode_t mode);
#else
static int mkpath(char* path);
#endif

static assetdir_t* afs_mkdir(assetdir_t* parent, const char* dirname);
static assetdir_t* afs_rmdir(assetdir_t* base);
static assetdir_t* afs_finddir(assetdir_t* base, const char* dirpath);
static assetfile_t* afs_mkfile(const char* filename, const char* fullpath, assetfiletype_t type, assetpriority_t priority);
static assetfile_t* afs_rmfile(assetfile_t* file);
static void afs_storefile(assetdir_t* dir, assetfile_t* file);
static void afs_updatefile(assetfile_t* file, const char* fullpath);
static assetfile_t* afs_findfile(assetdir_t* dir, const char* vpath);
static int afs_foreach(assetdir_t* dir, const char* extension_filter, int (*callback)(const char* fullpath, void* param), void* param, bool recursive, bool* stop);
static void afs_sort(assetdir_t* base);
static bool afs_empty(assetdir_t* base);
static assetdir_t* afs_mkpath(assetdir_t* base, const char* vpath);
static bool afs_strict = true;



/* public API */

/*
 * assetfs_init()
 * Initializes the asset filesystem
 * gameid: a string (lowercase letters, numbers), or NULL if we have specified no custom gameid
 * basedir: an absolute filepath, or NULL to use GAME_DATADIR as the base directory for the assets
 * datadir: will read assets only from this path; if NULL, will look for the assets in default places
 */
void assetfs_init(const char* gameid, const char* basedir, const char* datadir)
{
    /* error? */
    if(assetfs_initialized()) {
        assetfs_fatal("assetfs_init() error: already initialized");
        return;
    }

    /* create the root */
    root = afs_mkdir(NULL, ".");

    /* get the gameid */
    gameid = !gameid && datadir && *datadir ? "unknown" : gameid; /* FIXME */
    gameid = gameid && *gameid ? gameid : GAME_UNIXNAME;
    afs_gameid = clone_str(gameid);

    /* validate basedir */
    basedir = basedir && *basedir ? basedir : GAME_DATADIR;

    /* scan the assets */
    if(is_valid_id(gameid)) {
        assetfs_log("Loading assets for %s...", gameid);
        if(datadir && *datadir) {
            /* using custom data directory */
            if(!is_asset_folder(datadir))
                assetfs_log("Custom asset folder \"%s\" is either invalid or obsolete.", datadir);
            scan_folder(root, datadir, ASSET_DATA, ASSET_PRIMARY);
        }
        else
            scan_default_folders(gameid, basedir);
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
    root = afs_rmdir(root);
    free(afs_gameid);
    afs_gameid = NULL;
}

/*
 * assetfs_fullpath()
 * Return the absolute filepath related to the given virtual path
 */
const char* assetfs_fullpath(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);
    static char path[4096] = { 0 };

    if(file == NULL) {
        assetfs_log("Can't find asset \"%s\"", vpath);
        if(is_sane_vpath(vpath)) {
            /* return an invalid path to the program */
            char* sanitized_vpath = pathify(vpath);
            snprintf(path, sizeof(path), "surge://%s", sanitized_vpath);
            free(sanitized_vpath);
            return path;
        }
        else {
            /* vpath is not 'safe' */
            return "invalid-asset";
        }
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
 * Executes a callback for each file in a virtual folder; returns the number of counted files
 * 1. extension filter may be NULL or ".png", ".ss", and so on...
 * 2. callback must return 0 to let the enumeration proceed, or non-zero to stop it
 */
int assetfs_foreach_file(const char* vpath_of_dir, const char* extension_filter, int (*callback)(const char* vpath, void* param), void* param, bool recursive)
{
    assetdir_t* dir = afs_finddir(root, vpath_of_dir);
    return (dir != NULL) ? afs_foreach(dir, extension_filter, callback, param, recursive, NULL) : 0;
}

/*
 * assetfs_initialized()
 * checks if this subsystem has been initialized
 */
bool assetfs_initialized()
{
    return (root != NULL);
}

/*
 * assetfs_use_strict()
 * Use strict mode? (default: true)
 * Non-strict mode allows empty file systems
 */
bool assetfs_use_strict(bool strict)
{
    bool old_value = afs_strict;
    afs_strict = strict;
    return old_value;
}

/*
 * assetfs_is_primary_file()
 * Checks if the file is primary, i.e., not
 * gathered from a complimentary source
 */
bool assetfs_is_primary_file(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);
    return file != NULL ? file->priority == ASSET_PRIMARY : false;
}

/*
 * assetfs_create_config_file()
 * Creates a new config file in the virtual filesystem and
 * return its fullpath in the actual filesystem
 */
const char* assetfs_create_config_file(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);

    if(file == NULL) {
        int dirlen = -1;
        char* path = pathify(vpath), *fullpath;

        if((fullpath = build_config_fullpath(afs_gameid, path)) != NULL) {
            /* Create the path in the virtual filesystem */
            file = afs_mkfile(vpathbasename(path, &dirlen), fullpath, ASSET_CONFIG, ASSET_PRIMARY);
            if(dirlen >= 0) {
                path[dirlen] = '\0';
                afs_storefile(afs_mkpath(root, path), file);
            }
            else
                afs_storefile(root, file);
            free(fullpath);

            /* Create the path in the actual filesystem */
            #if !defined(_WIN32)
            mkpath(file->fullpath, 0755);
            #else
            mkpath(file->fullpath);
            #endif
        }
        else
            assetfs_fatal("assetfs error: can't create config file \"%s\"", vpath);

        free(path);
        return file ? file->fullpath : "";
    }
    else {
        if(file->type != ASSET_CONFIG) {
            /* preserve paths. Do nothing. */
            assetfs_log("assetfs warning: expected a config file - \"%s\"", vpath);
            file->type = ASSET_CONFIG;
        }
        if(!is_writable_file(file->fullpath)) {
            /* not a writable file. Replace path. */
            char* path = pathify(vpath), *fullpath;
            if((fullpath = build_config_fullpath(afs_gameid, path)) != NULL) {
                assetfs_log("assetfs warning: not a writable file - \"%s\". Using \"%s\"", file->fullpath, fullpath);
                afs_updatefile(file, fullpath);
                free(fullpath);
            }
            else
                assetfs_log("assetfs warning: not a writable file - \"%s\"", file->fullpath);
            free(path);
        }
        return file->fullpath;
    }
}

/*
 * assetfs_create_cache_file()
 * Create a user-specific non-essential (cached) data
 */
const char* assetfs_create_cache_file(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);

    if(file == NULL) {
        int dirlen = -1;
        char* path = pathify(vpath), *fullpath;

        if((fullpath = build_cache_fullpath(afs_gameid, path)) != NULL) {
            /* Create the path in the virtual filesystem */
            file = afs_mkfile(vpathbasename(path, &dirlen), fullpath, ASSET_CACHE, ASSET_PRIMARY);
            if(dirlen >= 0) {
                path[dirlen] = '\0';
                afs_storefile(afs_mkpath(root, path), file);
            }
            else
                afs_storefile(root, file);
            free(fullpath);

            /* Create the path in the actual filesystem */
            #if !defined(_WIN32)
            mkpath(file->fullpath, 0755);
            #else
            mkpath(file->fullpath);
            #endif
        }
        else
            assetfs_fatal("assetfs error: can't create cache file \"%s\"", vpath);

        free(path);
        return file ? file->fullpath : "";
    }
    else {
        bool prefer_user_space = false;
        if(file->type != ASSET_CACHE) {
            /* wrong file type. Replace path. */
            assetfs_log("assetfs warning: expected a cache file - \"%s\"", vpath);
            file->type = ASSET_CACHE;
            prefer_user_space = true;
        }
        if(!is_writable_file(file->fullpath)) {
            /* not a writable file. Replace path. */
            assetfs_log("assetfs warning: not a writable file - \"%s\". Using user space.", file->fullpath);
            prefer_user_space = true;
        }
        if(prefer_user_space) {
            /* prefer user space. Replace path. */
            char* path = pathify(vpath), *fullpath;
            if((fullpath = build_cache_fullpath(afs_gameid, path)) != NULL) {
                afs_updatefile(file, fullpath);
                free(fullpath);
            }
            else
                assetfs_log("assetfs warning: can't create file \"%s\" in user space - \"%s\"", vpath, file->fullpath);
            free(path);
        }
        return file->fullpath;
    }
}

/*
 * assetfs_create_data_file()
 * Creates a data file. This shouldn't be used often (usually, data files should be readonly)
 * Note: prefer_user_space should be true only if you don't want to mess with the local folder
 */
const char* assetfs_create_data_file(const char* vpath, bool prefer_user_space)
{
    assetfile_t* file = afs_findfile(root, vpath);

    if(file == NULL) {
        int dirlen = -1;
        char* path = pathify(vpath), *fullpath;

        if((fullpath = build_userdata_fullpath(afs_gameid, path)) != NULL) {
            /* Create the path in the virtual filesystem */
            file = afs_mkfile(vpathbasename(path, &dirlen), fullpath, ASSET_DATA, ASSET_PRIMARY);
            if(dirlen >= 0) {
                path[dirlen] = '\0';
                afs_storefile(afs_mkpath(root, path), file);
            }
            else
                afs_storefile(root, file);
            free(fullpath);

            /* Create the path in the actual filesystem */
            #if !defined(_WIN32)
            mkpath(file->fullpath, 0755);
            #else
            mkpath(file->fullpath);
            #endif
        }
        else
            assetfs_fatal("assetfs error: can't create data file \"%s\"", vpath);

        free(path);
        return file ? file->fullpath : "";
    }
    else {
        if(file->type != ASSET_DATA) {
            /* preserve paths. Do nothing. */
            assetfs_log("assetfs warning: expected a data file - \"%s\"", vpath);
            file->type = ASSET_DATA;
        }
        if(!is_writable_file(file->fullpath)) {
            /* not a writable file. Replace path. */
            assetfs_log("assetfs warning: not a writable file - \"%s\". Using user space.", file->fullpath);
            prefer_user_space = true;
        }
        if(prefer_user_space) {
            /* prefer user space. Replace path. */
            char* path = pathify(vpath), *fullpath;
            if((fullpath = build_userdata_fullpath(afs_gameid, path)) != NULL) {
                afs_updatefile(file, fullpath);
                free(fullpath);
            }
            else
                assetfs_log("assetfs warning: can't create file \"%s\" in user space - \"%s\"", vpath, file->fullpath);
            free(path);
        }
        return file->fullpath;
    }
}

/*
 * assetfs_is_config_file()
 * Checks if the given file is a config file
 */
bool assetfs_is_config_file(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);
    return file != NULL && file->type == ASSET_CONFIG;
}

/*
 * assetfs_is_cache_file()
 * Checks if the given file is a cache file
 */
bool assetfs_is_cache_file(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);
    return file != NULL && file->type == ASSET_CACHE;
}

/*
 * assetfs_is_data_file()
 * Checks if the given file is a data file
 */
bool assetfs_is_data_file(const char* vpath)
{
    assetfile_t* file = afs_findfile(root, vpath);
    return file != NULL && file->type == ASSET_DATA;
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

/* checks the validity of a writable virtual path */
bool is_sane_vpath(const char* vpath)
{
    /* sanity */
    if(strstr(vpath, "../") != NULL || strstr(vpath, "/..") != NULL ||
    strstr(vpath, "..\\") != NULL || strstr(vpath, "\\..") != NULL ||
    strstr(vpath, "//") != NULL || strstr(vpath, "\\\\") != NULL ||
    strstr(vpath, "~") != NULL || strstr(vpath, ":") != NULL ||
    vpath[0] == '/' || vpath[0] == '\\')
        return false;

    return true;
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
#if 0
        for(int i = 0; i < darray_length(filedir->file) && !file; i++) {
            if(vpathcmp(filedir->file[i]->name, filename) == 0)
                file = filedir->file[i];
        }
#else
        assetfile_t** entry = bsearch(filename, filedir->file, darray_length(filedir->file), sizeof(assetfile_t*), filematch);
        file = (entry != NULL) ? *entry : NULL;
#endif
    }

    /* done */
    free(path);
    return file;
}

/* list files */
int afs_foreach(assetdir_t* dir, const char* extension_filter, int (*callback)(const char* vpath, void* param), void* param, bool recursive, bool* stop)
{
    char* dirpath = dir2vpath(dir);
    int count = 0;
    const char* q;

    /* foreach file */
    for(int i = 0; i < darray_length(dir->file); i++) {
        if(!extension_filter || ((q = strrchr(dir->file[i]->name, '.')) && 0 == vpathcmp(q, extension_filter))) {
            char* vpath = join_path(dirpath, dir->file[i]->name);
            count++;
            if(0 != callback(vpath, param)) {
                /* stop the enumeration */
                if(stop != NULL)
                    *stop = true;
                free(vpath);
                free(dirpath);
                return count;
            }
            free(vpath);
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
                    free(dirpath);
                    return count;
                }
            }
        }
    }

    /* done! */
    free(dirpath);
    return count;
}



/* creates a new virtual file */
assetfile_t* afs_mkfile(const char* filename, const char* fullpath, assetfiletype_t type, assetpriority_t priority)
{
    assetfile_t* file = mallocx(sizeof *file);
    file->name = clone_str(filename);
    file->fullpath = clone_str(fullpath);
    file->type = type;
    file->priority = priority;
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

/* updates the fullpath of a virtual file */
void afs_updatefile(assetfile_t* file, const char* fullpath)
{
    free(file->fullpath);
    file->fullpath = clone_str(fullpath);
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

/* mkpath(): give the absolute path of a file
   this will create the filepath for you */
#if !defined(_WIN32)
static int mkpath(char* path, mode_t mode)
{
    char* p;

    /* sanity check */
    if(path == NULL || *path == '\0')
        return 0;

    /* make path */
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
#else
static int mkpath(char* path)
{
    char* p;

    /* sanity check */
    if(path == NULL || *path == '\0')
        return 0;

    /* skip volume; begin as \folder1\folder2\file... */
    p = strstr(path, ":\\");
    if(p == NULL) {
        if(strncmp(path, "\\\\", 2) == 0 && isalnum(path[2])) {
            p = strchr(path+2, '\\');
            if(p == NULL) {
                assetfs_fatal("Can't mkpath \"%s\": invalid path", path);
                return -1;
            }
        }
        else {
            assetfs_fatal("Can't mkpath \"%s\": not an absolute path", path);
            return -1;
        }
    }
    else
        p = p+1;

    /* make path */
    for(p = strchr(p+1, '\\'); p != NULL; p = strchr(p+1, '\\')) {
        *p = '\0';
        if(_mkdir(path) != 0) {
            if(errno != EEXIST) {
                *p = '\\';
                assetfs_log("Can't mkpath \"%s\": %s", path, strerror(errno));
                return -1;
            }
        }
        *p = '\\';
    }

    return 0;
}
#endif




/* checks if a certain folder (given its absolute path) is a valid
 * opensurge asset folder */
bool is_asset_folder(const char* fullpath)
{
    char* fpath = join_path(fullpath, "surge.rocks");
    bool valid = false;
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    struct stat st;
    valid = (stat(fpath, &st) == 0);
#else
    FILE* fp = fopen_utf8(fpath, "rb");
    if(fp != NULL) {
        valid = true;
        fclose(fp);
    }
#endif
    if(!valid)
        assetfs_log("Not an asset folder: \"%s\"", fullpath);
    free(fpath);
    return valid;
}

/* checks if a certain file (given its absolute path) is writable */
bool is_writable_file(const char* fullpath)
{
    FILE* fp = fopen_utf8(fullpath, "r+b");
    bool writable = false;
    if(fp != NULL) {
        writable = true;
        fclose(fp);
    }
    return writable;
}



/* scans the default folders that store game assets */
void scan_default_folders(const char* gameid, const char* basedir)
{
#if defined(_WIN32)
    assetfs_log("Scanning assets...");

    /* scan asset folder: <exedir> */
    if(!scan_exedir(root, ASSET_PRIMARY)) {
        const char* path = ".";
        assetfs_log("Can't find the application folder: scanning the working dir");
        if(is_asset_folder(path))
            scan_folder(root, path, ASSET_DATA, ASSET_PRIMARY);
    }
#elif defined(__APPLE__) && defined(__MACH__)
    /* FIXME: untested */
    char* userdatadir = build_userdata_fullpath(gameid, "");
    char* configdir = build_config_fullpath(gameid, "");
    char* cachedir = build_cache_fullpath(gameid, "");
    bool must_scan_basedir = true;
    assetfs_log("Scanning assets...");

    /* scan user-specific config & cache files (must come 1st) */
    if(configdir != NULL) {
        mkpath(configdir, 0755);
        scan_folder(root, configdir, ASSET_CONFIG, ASSET_PRIMARY);
        free(configdir);
    }
    if(cachedir != NULL) {
        mkpath(cachedir, 0755);
        scan_folder(root, cachedir, ASSET_CACHE, ASSET_PRIMARY);
        free(cachedir);
    }

    /* scan primary asset folder: <exedir>/../Resources */
    if(scan_exedir(root, ASSET_PRIMARY))
        must_scan_basedir = false;

    /* scan additional asset folder: ~/Library/<basedir>/<gameid> */
    if(userdatadir != NULL) {
        mkpath(userdatadir, 0755); /* userdatadir ends with '/' */
        scan_folder(root, userdatadir, ASSET_DATA, must_scan_basedir ? ASSET_PRIMARY : ASSET_SECONDARY);
        free(userdatadir);
    }
    else
        assetfs_log("Can't find the userdata directory: additional game assets may not be loaded");

    /* scan <basedir> */
    if(must_scan_basedir) {
        if(is_asset_folder(basedir))
            scan_folder(root, basedir, ASSET_DATA, ASSET_SECONDARY);
        else if(afs_strict)
            assetfs_fatal("Can't load %s: assets not found in %s", gameid, basedir);
    }
#elif defined(__unix__) || defined(__unix)
    char* userdatadir = build_userdata_fullpath(gameid, "");
    char* configdir = build_config_fullpath(gameid, "");
    char* cachedir = build_cache_fullpath(gameid, "");
    bool must_scan_basedir = true;
    assetfs_log("Scanning assets...");

    /* scan user-specific config & cache files (must come 1st) */
    if(configdir != NULL) {
        mkpath(configdir, 0755);
        scan_folder(root, configdir, ASSET_CONFIG, ASSET_PRIMARY);
        free(configdir);
    }
    if(cachedir != NULL) {
        mkpath(cachedir, 0755);
        scan_folder(root, cachedir, ASSET_CACHE, ASSET_PRIMARY);
        free(cachedir);
    }

    /* scan primary asset folder: <exedir> */
    if(scan_exedir(root, ASSET_PRIMARY))
        must_scan_basedir = false;

    /* scan additional asset folder: $XDG_DATA_HOME/<basedir>/<gameid> */
    if(userdatadir != NULL) {
        mkpath(userdatadir, 0755); /* userdatadir ends with '/' */
        scan_folder(root, userdatadir, ASSET_DATA, must_scan_basedir ? ASSET_PRIMARY : ASSET_SECONDARY);
        free(userdatadir);
    }
    else
        assetfs_log("Can't find the userdata directory: additional game assets may not be loaded");

    /* scan <basedir> */
    if(must_scan_basedir) {
        if(is_asset_folder(basedir))
            scan_folder(root, basedir, ASSET_DATA, ASSET_SECONDARY);
        else if(afs_strict)
            assetfs_fatal("Can't load %s: assets not found in %s", gameid, basedir);
    }
#else
#error "Unsupported operating system."
#endif
}

/* scan the <exedir>; returns true if it's an asset folder */
bool scan_exedir(assetdir_t* dir, assetpriority_t priority)
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
            scan_folder(dir, data_path, ASSET_DATA, priority);
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
            scan_folder(dir, exedir, ASSET_DATA, priority);
        }
        free(exedir);
    }
    else
        assetfs_log("Can't find the application folder: game assets may not be loaded");

    return success;
#endif
}

/* scan a specific asset folder */
void scan_folder(assetdir_t* folder, const char* abspath, assetfiletype_t type, assetpriority_t priority)
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
                            scan_folder(subfolder, path, type, priority);
                            free(path);
                        }
                    }
                    else {
                        /* scan the asset */
                        assetfile_t* file = afs_findfile(folder, filename);
                        if(file == NULL) {
                            char* path = join_path(abspath, filename);
                            afs_storefile(folder, afs_mkfile(filename, path, type, priority));
                            free(path);
                        }
                        else if(file->priority != ASSET_PRIMARY && priority == ASSET_PRIMARY) {
                            char* path = join_path(abspath, filename);
                            afs_updatefile(file, path);
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
                is_file = (d->d_type == DT_REG) && (d->d_type != DT_LNK);
            }
            else {
            #endif
                char* path = join_path(abspath, d->d_name);
                struct stat st;
                lstat(path, &st);
                is_dir = S_ISDIR(st.st_mode) && !(S_ISLNK(st.st_mode));
                is_file = S_ISREG(st.st_mode) && !(S_ISLNK(st.st_mode));
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
                    scan_folder(subfolder, path, type, priority);
                    free(path);
                }
            }

            /* scan the asset */
            else if(is_file) {
                assetfile_t* file = afs_findfile(folder, d->d_name);
                if(file == NULL) {
                    char* path = join_path(abspath, d->d_name);
                    afs_storefile(folder, afs_mkfile(d->d_name, path, type, priority));
                    free(path);
                }
                else if(file->priority != ASSET_PRIMARY && priority == ASSET_PRIMARY) {
                    char* path = join_path(abspath, d->d_name);
                    afs_updatefile(file, path);
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

    /* for bsearch() */
    afs_sort(folder);
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

/* mkpath, where vpath is a virtual path to a directory */
assetdir_t* afs_mkpath(assetdir_t* base, const char* vpath)
{
    char* path = pathify(vpath), *p;
    assetdir_t* dir;

    if((p = strchr(path, '/')) != NULL) {
        *p = '\0';
        dir = afs_mkpath(afs_mkdir(base, path), p+1);
    }
    else if(*path != '\0') /* && NULL == strchr(path, '.'))*/
        dir = afs_mkdir(base, path);
    else
        dir = base;

    free(path);
    return dir;
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

/* basename */
const char* vpathbasename(const char* vpath, int* dirname_length)
{
    const char* p = strrchr(vpath, '/');

    if(p != NULL) {
        if(dirname_length != NULL)
            *dirname_length = p - vpath;
        return p+1;
    }
    else {
        if(dirname_length != NULL)
            *dirname_length = -1;
        return vpath;
    }
}

/* get the vpath of a given directory; you'll need to free() the returned string */
char* dir2vpath(assetdir_t* dir)
{
    if(dir != root) {
        assetdir_t* parent = afs_finddir(dir, "..");
        if(parent != NULL) {
            const char* dirname = "";
            char* parent_path, *path;

            /* find dirname */
            for(int i = 0; i < darray_length(parent->dir); i++) {
                if(parent->dir[i].contents == dir) {
                    dirname = parent->dir[i].name;
                    break;
                }
            }

            /* build path */
            parent_path = dir2vpath(parent);
            if(*parent_path != '\0')
                path = join_path(parent_path, dirname);
            else
                path = clone_str(dirname);
            free(parent_path);

            /* done */
            return path;
        }
        else
            return pathify(""); /* this shouldn't happen */
    }
    else
        return pathify("");
}

/* The absolute filepath of a configuration file
   You'll have to free() this. Returns NULL on error */
char* build_config_fullpath(const char* gameid, const char* vpath)
{
    char* result = NULL;

    /* sanity check */
    if(!is_sane_vpath(vpath)) {
        assetfs_fatal("Can't build path for \"%s\": invalid path", vpath);
        return NULL;
    }

    /* OS-specific routines */
    if(vpath != NULL) {
#if defined(_WIN32)
        /* return <exedir>\<vpath> */
        int len, dirlen = 0;
        if((len = wai_getExecutablePath(NULL, 0, NULL)) >= 0) {
            char* path = mallocx((1 + len) * sizeof(*path));
            const char* p;
            DARRAY(char, buf);
            darray_init(buf);

            wai_getExecutablePath(path, len, &dirlen);
            path[dirlen] = '\0';
            for(p = path; *p; p++)
                darray_push(buf, *p);
            darray_push(buf, '\\');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '/' ? *p : '\\');
            darray_push(buf, '\0');

            mkpath((char*)buf);
            result = clone_str((const char*)buf);
            darray_release(buf);
            free(path);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find exedir", vpath);
#elif defined(__APPLE__) && defined(__MACH__)
        /* return ~/Library/Application Support/<basedir>/<gameid>/<vpath> */
        struct passwd* userinfo = NULL;
        if(NULL != (userinfo = getpwuid(getuid()))) {
            const char* path = "/Library/Application Support/" ASSETS_BASEDIR "/", *p;
            DARRAY(char, buf);
            darray_init(buf);

            for(p = userinfo->pw_dir; *p; p++)
                darray_push(buf, *p);
            for(p = path; *p; p++)
                darray_push(buf, *p);
            for(p = gameid; *p; p++)
                darray_push(buf, *p);
            darray_push(buf, '/');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '\\' ? *p : '/');
            darray_push(buf, '\0');

            mkpath((char*)buf, 0755);
            result = clone_str((const char*)buf);
            darray_release(buf);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find home directory", vpath);
#elif defined(__unix__) || defined(__unix)
        /* return $XDG_CONFIG_HOME/<basedir>/<gameid>/<vpath> */
        const char *home = NULL, *path = NULL;
        if(NULL == (home = getenv("XDG_CONFIG_HOME"))) {
            struct passwd* userinfo = NULL;
            if(NULL != (userinfo = getpwuid(getuid()))) {
                home = userinfo->pw_dir;
                path = "/.config/" ASSETS_BASEDIR "/";
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
            darray_push(buf, '/');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '\\' ? *p : '/');
            darray_push(buf, '\0');

            mkpath((char*)buf, 0755);
            result = clone_str((const char*)buf);
            darray_release(buf);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find the config directory", vpath);
#else
        /* Unknown system */
        ;
#endif

        /* error? */
        if(result == NULL) {
            assetfs_log("Can't find the config directory");
            return NULL;
        }
    }
    else
        result = build_config_fullpath(gameid, "");

    /* done! */
    return result;
}

/* The absolute filepath of a user-specific (readonly) data file
   You'll have to free() this. Returns NULL on error */
char* build_userdata_fullpath(const char* gameid, const char* vpath)
{
    char* result = NULL;

    /* sanity check */
    if(!is_sane_vpath(vpath)) {
        assetfs_fatal("Can't build path for \"%s\": invalid path", vpath);
        return NULL;
    }

    /* OS-specific routines */
    if(vpath != NULL) {
#if defined(_WIN32)
        /* return <exedir>\<vpath> */
        int len, dirlen = 0;
        if((len = wai_getExecutablePath(NULL, 0, NULL)) >= 0) {
            char* path = mallocx((1 + len) * sizeof(*path));
            const char* p;
            DARRAY(char, buf);
            darray_init(buf);

            wai_getExecutablePath(path, len, &dirlen);
            path[dirlen] = '\0';
            for(p = path; *p; p++)
                darray_push(buf, *p);
            darray_push(buf, '\\');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '/' ? *p : '\\');
            darray_push(buf, '\0');

            mkpath((char*)buf);
            result = clone_str((const char*)buf);
            darray_release(buf);
            free(path);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find exedir", vpath);
#elif defined(__APPLE__) && defined(__MACH__)
        /* return ~/Library/<basedir>/<gameid>/<vpath> */
        struct passwd* userinfo = NULL;
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
            darray_push(buf, '/');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '\\' ? *p : '/');
            darray_push(buf, '\0');

            mkpath((char*)buf, 0755);
            result = clone_str((const char*)buf);
            darray_release(buf);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find home directory", vpath);
#elif defined(__unix__) || defined(__unix)
        /* return $XDG_DATA_HOME/<basedir>/<gameid>/<vpath> */
        const char *home = NULL, *path = NULL;
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
            darray_push(buf, '/');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '\\' ? *p : '/');
            darray_push(buf, '\0');

            mkpath((char*)buf, 0755);
            result = clone_str((const char*)buf);
            darray_release(buf);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find the userdata directory", vpath);
#else
        /* Unknown system */
        ;
#endif

        /* error? */
        if(result == NULL) {
            assetfs_log("Can't find the userdata directory");
            return NULL;
        }
    }
    else
        result = build_userdata_fullpath(gameid, "");

    /* done! */
    return result;
}

/* The absolute filepath of a user-specific (non-essential, cached and writable) data file
   You'll have to free() this. Returns NULL on error */
char* build_cache_fullpath(const char* gameid, const char* vpath)
{
    char* result = NULL;

    /* sanity check */
    if(!is_sane_vpath(vpath)) {
        assetfs_fatal("Can't build path for \"%s\": invalid path", vpath);
        return NULL;
    }

    /* OS-specific routines */
    if(vpath != NULL) {
#if defined(_WIN32)
        /* return <exedir>\<vpath> */
        int len, dirlen = 0;
        if((len = wai_getExecutablePath(NULL, 0, NULL)) >= 0) {
            char* path = mallocx((1 + len) * sizeof(*path));
            const char* p;
            DARRAY(char, buf);
            darray_init(buf);

            wai_getExecutablePath(path, len, &dirlen);
            path[dirlen] = '\0';
            for(p = path; *p; p++)
                darray_push(buf, *p);
            darray_push(buf, '\\');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '/' ? *p : '\\');
            darray_push(buf, '\0');

            mkpath((char*)buf);
            result = clone_str((const char*)buf);
            darray_release(buf);
            free(path);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find exedir", vpath);
#elif defined(__APPLE__) && defined(__MACH__)
        /* return ~/Library/Caches/<basedir>/<gameid>/<vpath> */
        struct passwd* userinfo = NULL;
        if(NULL != (userinfo = getpwuid(getuid()))) {
            const char* path = "/Library/Caches/" ASSETS_BASEDIR "/", *p;
            DARRAY(char, buf);
            darray_init(buf);

            for(p = userinfo->pw_dir; *p; p++)
                darray_push(buf, *p);
            for(p = path; *p; p++)
                darray_push(buf, *p);
            for(p = gameid; *p; p++)
                darray_push(buf, *p);
            darray_push(buf, '/');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '\\' ? *p : '/');
            darray_push(buf, '\0');

            mkpath((char*)buf, 0755);
            result = clone_str((const char*)buf);
            darray_release(buf);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find home directory", vpath);
#elif defined(__unix__) || defined(__unix)
        /* return $XDG_CACHE_HOME/<basedir>/<gameid>/<vpath> */
        const char *home = NULL, *path = NULL;
        if(NULL == (home = getenv("XDG_CACHE_HOME"))) {
            struct passwd* userinfo = NULL;
            if(NULL != (userinfo = getpwuid(getuid()))) {
                home = userinfo->pw_dir;
                path = "/.cache/" ASSETS_BASEDIR "/";
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
            darray_push(buf, '/');
            for(p = vpath; *p; p++)
                darray_push(buf, *p != '\\' ? *p : '/');
            darray_push(buf, '\0');

            mkpath((char*)buf, 0755);
            result = clone_str((const char*)buf);
            darray_release(buf);
        }
        else
            assetfs_log("Can't build path for \"%s\": can't find the cache directory", vpath);

#else
        /* Unknown system */
        ;
#endif

        /* error? */
        if(result == NULL) {
            assetfs_log("Can't find the cache directory");
            return NULL;
        }
    }
    else
        result = build_cache_fullpath(gameid, "");

    /* done! */
    return result;
}


