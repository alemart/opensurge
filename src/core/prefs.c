/*
 * Open Surge Engine
 * prefs.c - load/save user preferences
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "darray.h"
#include "prefs.h"
#include "util.h"
#include "logfile.h"
#include "global.h"
#include "assetfs.h"
#include "whereami/whereami.h"

/* OS-specific includes */
#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

/* macros */
#define prefs_fatal fatal_error
#define prefs_log logfile_message
#define prefs_version() (GAME_VERSION_CODE)

/* Where are the prefs stored? */
#define PREFS_FILE "surge.prefs"

/* prefs structure */
typedef enum prefstype_t prefstype_t;
typedef struct prefslist_t prefslist_t;
typedef struct prefsentry_t prefsentry_t;

enum prefstype_t
{
    PREFS_NULL    = 0,
    PREFS_INT32   = 1,
    PREFS_FLOAT64 = 2,
    PREFS_STRING  = 3,
    PREFS_BOOL    = 4
};

struct prefsentry_t
{
    char* key;
    union prefsvalue_t {
        uint8_t boolean;
        int32_t integer;
        double real;
        char* text;
    } value;
    uint32_t hash;
    prefstype_t type;
};

struct prefslist_t
{
    prefsentry_t* entry;
    prefslist_t* next;
};

#define PREFS_MAXBUCKETS 31
struct prefs_t
{
    char* prefsid;
    prefslist_t* bucket[PREFS_MAXBUCKETS];
};

/* prefs file format */
typedef struct pfheader_t pfheader_t;
typedef struct pfentry_t pfentry_t;

#define PREFS_MAGIC "SURGEPREFS"
struct pfheader_t
{
    uint8_t magic[10]; /* file signature */
    uint8_t unused[2]; /* set to zeroes */
    uint32_t version_code; /* engine version */
    uint32_t prefsid_hash; /* prefsid hash */
    uint32_t entry_count; /* number of entries */
};

struct pfentry_t
{
    uint8_t type; /* entry type */
    uint32_t data_size; /* data size (in bytes) */
    /* data: [ key | \0 | value ] */
};

/* private stuff */
static int load(prefs_t* prefs);
static int save(const prefs_t* prefs);
static uint32_t hash(const char* str);
static int is_valid_id(const char* key);
static inline char* clone_str(const char* str);
static prefsentry_t* new_string_entry(const char* key, const char* value);
static prefsentry_t* new_int_entry(const char* key, int32_t value);
static prefsentry_t* new_double_entry(const char* key, double value);
static prefsentry_t* new_null_entry(const char* key);
static prefsentry_t* new_bool_entry(const char* key, uint8_t value);
static prefsentry_t* delete_entry(prefsentry_t* entry);
static prefslist_t* new_list(prefsentry_t* entry, prefslist_t* next);
static prefslist_t* delete_list(prefslist_t* list);
static prefsentry_t* prefs_find_entry(prefs_t* prefs, const char* key);
static int prefs_remove_entry(prefs_t* prefs, const char* key);
static void prefs_add_entry(prefs_t* prefs, prefsentry_t* entry);
static int prefs_count_entries(const prefs_t* prefs);
static uint16_t le16_to_cpu(const uint8_t* buf);
static void cpu_to_le16(uint16_t input, uint8_t* buf);
static uint32_t le32_to_cpu(const uint8_t* buf);
static void cpu_to_le32(uint32_t input, uint8_t* buf);
static uint64_t le64_to_cpu(const uint8_t* buf);
static void cpu_to_le64(uint64_t input, uint8_t* buf);
static uint32_t double_serialize(double input, uint8_t* buf);
static double double_deserialize(const uint8_t* buf, uint32_t size);
static inline int keycmp(const char* a, const char* b);
static size_t keylen(const char* key, size_t maxlen);
static int write_header(ALLEGRO_FILE* fp, const prefs_t* prefs);
static int write_entry(ALLEGRO_FILE* fp, const prefsentry_t* entry);
static prefsentry_t* read_entry(ALLEGRO_FILE* fp);
static int read_header(ALLEGRO_FILE* fp, pfheader_t* header);
static int validate_header(ALLEGRO_FILE* fp, const prefs_t* prefs, const pfheader_t* header);




/* public API */

/*
 * prefs_create()
 * Creates a new prefs, with the associated prefsid (may be NULL)
 */
prefs_t* prefs_create(const char* prefsid)
{
    prefs_t* prefs;
    int i;

    /* Validate */
    prefsid = prefsid && *prefsid ? prefsid : GAME_UNIXNAME;
    if(!is_valid_id(prefsid))
        prefs_fatal("Can't create Prefs: invalid id \"%s\". Please use only lowercase letters / digits.", prefsid);

    /* Create the instance */
    prefs = mallocx(sizeof *prefs);
    prefs->prefsid = clone_str(prefsid);
    for(i = 0; i < PREFS_MAXBUCKETS; i++)
        prefs->bucket[i] = NULL;

    /* Load from the disk */
    load(prefs);
    return prefs;
}

/*
 * prefs_destroy()
 * Destroys a prefs instance
 */
prefs_t* prefs_destroy(prefs_t* prefs)
{
    /* Save to the disk */
    save(prefs);

    /* Delete the instance */
    for(int i = 0; i < PREFS_MAXBUCKETS; i++)
        delete_list(prefs->bucket[i]);
    free(prefs->prefsid);
    free(prefs);
    return NULL;
}

/*
 * prefs_set_null()
 * Sets an entry to null
 */
void prefs_set_null(prefs_t* prefs, const char* key)
{
    prefs_add_entry(prefs, new_null_entry(key));
}

/*
 * prefs_get_string()
 * Gets a string from the prefs
 */
const char* prefs_get_string(prefs_t* prefs, const char* key)
{
    prefsentry_t* entry = prefs_find_entry(prefs, key);
    return entry && entry->type == PREFS_STRING ? entry->value.text : "";
}

/*
 * prefs_set_string()
 * Sets a string to the prefs
 */
void prefs_set_string(prefs_t* prefs, const char* key, const char* value)
{
    prefs_add_entry(prefs, new_string_entry(key, value));
}

/*
 * prefs_get_int()
 * Gets an integer from the prefs
 */
int prefs_get_int(prefs_t* prefs, const char* key)
{
    prefsentry_t* entry = prefs_find_entry(prefs, key);
    return entry && entry->type == PREFS_INT32 ? entry->value.integer : 0;
}

/*
 * prefs_set_int()
 * Sets an integer to the prefs
 */
void prefs_set_int(prefs_t* prefs, const char* key, int value)
{
    prefs_add_entry(prefs, new_int_entry(key, value));
}

/*
 * prefs_get_double()
 * Gets a double from the prefs
 */
double prefs_get_double(prefs_t* prefs, const char* key)
{
    prefsentry_t* entry = prefs_find_entry(prefs, key);
    return entry && entry->type == PREFS_FLOAT64 ? entry->value.real : 0.0;
}

/*
 * prefs_set_double()
 * Sets a double to the prefs
 */
void prefs_set_double(prefs_t* prefs, const char* key, double value)
{
    prefs_add_entry(prefs, new_double_entry(key, value));
}

/*
 * prefs_get_bool()
 * Gets a boolean from the prefs
 */
bool prefs_get_bool(prefs_t* prefs, const char* key)
{
    prefsentry_t* entry = prefs_find_entry(prefs, key);
    return entry && entry->type == PREFS_BOOL ? entry->value.boolean != 0 : false;
}

/*
 * prefs_set_bool()
 * Sets a boolean to the prefs
 */
void prefs_set_bool(prefs_t* prefs, const char* key, bool value)
{
    prefs_add_entry(prefs, new_bool_entry(key, value ? 1 : 0));
}

/*
 * prefs_item_type()
 * Checks the type of an entry. Returns:
 * '\0' (null), 's' (string), 'i' (int), 'f' (double), 'b' (bool), '?' (unknown), '-' (not found)
 */
char prefs_item_type(prefs_t* prefs, const char* key)
{
    prefsentry_t* entry = prefs_find_entry(prefs, key);

    /* the entry exists */
    if(entry != NULL) {
        switch(entry->type) {
            case PREFS_NULL: return '\0';
            case PREFS_INT32: return 'i';
            case PREFS_FLOAT64: return 'f';
            case PREFS_STRING: return 's';
            case PREFS_BOOL: return 'b';
            default: return '?';
        }
    }

    /* not found */
    return '-';
}

/*
 * prefs_has_item()
 * Checks if the given item exists
 */
bool prefs_has_item(prefs_t* prefs, const char* key)
{
    return prefs_find_entry(prefs, key) != NULL;
}

/*
 * prefs_delete_item()
 * Deletes an item. Returns true on success
 */
bool prefs_delete_item(prefs_t* prefs, const char* key)
{
    return prefs_remove_entry(prefs, key) != 0;
}

/*
 * prefs_clear()
 * Clears all entries from the prefs
 */
void prefs_clear(prefs_t* prefs)
{
    for(int i = 0; i < PREFS_MAXBUCKETS; i++)
        prefs->bucket[i] = delete_list(prefs->bucket[i]);
}

/*
 * prefs_id()
 * Gets the prefsid (string)
 */
const char* prefs_id(const prefs_t* prefs)
{
    return prefs->prefsid;
}

/*
 * prefs_save()
 * Saves the prefs to the disk
 */
void prefs_save(const prefs_t* prefs)
{
    save(prefs);
}





/* internal stuff */

/* prefs entry: utilities */
prefsentry_t* new_string_entry(const char* key, const char* value)
{
    prefsentry_t* entry = mallocx(sizeof *entry);

    entry->key = clone_str(key);
    entry->value.text = clone_str(value);
    entry->hash = hash(entry->key);
    entry->type = PREFS_STRING;

    return entry;
}

prefsentry_t* new_int_entry(const char* key, int32_t value)
{
    prefsentry_t* entry = mallocx(sizeof *entry);

    entry->key = clone_str(key);
    entry->value.integer = value;
    entry->hash = hash(entry->key);
    entry->type = PREFS_INT32;

    return entry;
}

prefsentry_t* new_double_entry(const char* key, double value)
{
    prefsentry_t* entry = mallocx(sizeof *entry);

    entry->key = clone_str(key);
    entry->value.real = value;
    entry->hash = hash(entry->key);
    entry->type = PREFS_FLOAT64;

    return entry;
}

prefsentry_t* new_null_entry(const char* key)
{
    prefsentry_t* entry = mallocx(sizeof *entry);

    entry->key = clone_str(key);
    entry->value.real = 0.0;
    entry->hash = hash(entry->key);
    entry->type = PREFS_NULL;

    return entry;
}

prefsentry_t* new_bool_entry(const char* key, uint8_t value)
{
    prefsentry_t* entry = mallocx(sizeof *entry);

    entry->key = clone_str(key);
    entry->value.boolean = value;
    entry->hash = hash(entry->key);
    entry->type = PREFS_BOOL;

    return entry;
}

prefsentry_t* delete_entry(prefsentry_t* entry)
{
    if(entry->type == PREFS_STRING)
        free(entry->value.text);
    free(entry->key);
    free(entry);
    return NULL;
}

/* prefs list: utilities */
prefslist_t* new_list(prefsentry_t* entry, prefslist_t* next)
{
    prefslist_t* list = mallocx(sizeof *list);
    list->entry = entry;
    list->next = next;
    return list;
}

prefslist_t* delete_list(prefslist_t* list)
{
    while(list != NULL) {
        prefslist_t* next = list->next;
        delete_entry(list->entry);
        free(list);
        list = next;
    }
    return NULL;
}

/* prefs: CRUD operations */
prefsentry_t* prefs_find_entry(prefs_t* prefs, const char* key)
{
    /* returns NULL if the key has not been found */
    uint32_t h = hash(key);
    prefslist_t* prev = NULL;

    for(prefslist_t* l = prefs->bucket[h % PREFS_MAXBUCKETS]; l != NULL; l = l->next) {
        /* found the key */
        if(l->entry->hash == h && 0 == keycmp(l->entry->key, key)) {
            /* move the entry to the front of the list */
            if(prev != NULL) {
                prev->next = l->next;
                l->next = prefs->bucket[h % PREFS_MAXBUCKETS];
                prefs->bucket[h % PREFS_MAXBUCKETS] = l;
            }
            return l->entry;
        }
        prev = l;
    }

    /* not found */
    return NULL;
}

int prefs_remove_entry(prefs_t* prefs, const char* key)
{
    /* returns 0 if there was no such key, non-zero otherwise */
    uint32_t h = hash(key);
    prefslist_t* prev = NULL;

    for(prefslist_t* l = prefs->bucket[h % PREFS_MAXBUCKETS]; l != NULL; l = l->next) {
        /* found the key */
        if(l->entry->hash == h && 0 == keycmp(l->entry->key, key)) {
            if(prev == NULL)
                prefs->bucket[h % PREFS_MAXBUCKETS] = l->next;
            else
                prev->next = l->next;
            l->next = NULL;
            delete_list(l);
            return 1;
        }
        prev = l;
    }

    /* not found */
    return 0;
}

void prefs_add_entry(prefs_t* prefs, prefsentry_t* entry)
{
    uint32_t h = hash(entry->key);
    prefslist_t* l = new_list(entry, NULL);

    /* No duplicate keys are allowed */
    prefs_remove_entry(prefs, entry->key);

    /* setup new entry */
    l->next = prefs->bucket[h % PREFS_MAXBUCKETS];
    prefs->bucket[h % PREFS_MAXBUCKETS] = l;
}

int prefs_count_entries(const prefs_t* prefs)
{
    int count = 0;

    for(int i = 0; i < PREFS_MAXBUCKETS; i++) {
        for(prefslist_t* l = prefs->bucket[i]; l != NULL; l = l->next)
            count++;
    }

    return count;
}

/* endianess conversion */
uint32_t le32_to_cpu(const uint8_t* buf)
{
    return
        ((uint32_t)buf[0]) |
        (((uint32_t)buf[1]) << 8) |
        (((uint32_t)buf[2]) << 16) |
        (((uint32_t)buf[3]) << 24)
    ;
}

void cpu_to_le32(uint32_t input, uint8_t* buf)
{
    buf[0] = (input & 0x000000FF);
    buf[1] = (input & 0x0000FF00) >> 8;
    buf[2] = (input & 0x00FF0000) >> 16;
    buf[3] = (input & 0xFF000000) >> 24;
}

uint16_t le16_to_cpu(const uint8_t* buf)
{
    return
        ((uint16_t)buf[0]) |
        (((uint16_t)buf[1]) << 8)
    ;
}

void cpu_to_le16(uint16_t input, uint8_t* buf)
{
    buf[0] = (input & 0x00FF);
    buf[1] = (input & 0xFF00) >> 8;
}

uint64_t le64_to_cpu(const uint8_t* buf)
{
    return
        ((uint64_t)buf[0]) |
        (((uint64_t)buf[1]) << 8) |
        (((uint64_t)buf[2]) << 16) |
        (((uint64_t)buf[3]) << 24) |
        (((uint64_t)buf[4]) << 32) |
        (((uint64_t)buf[5]) << 40) |
        (((uint64_t)buf[6]) << 48) |
        (((uint64_t)buf[7]) << 56)
    ;
}

void cpu_to_le64(uint64_t input, uint8_t* buf)
{
    buf[0] = (input & 0x00000000000000FFLL);
    buf[1] = (input & 0x000000000000FF00LL) >> 8;
    buf[2] = (input & 0x0000000000FF0000LL) >> 16;
    buf[3] = (input & 0x00000000FF000000LL) >> 24;
    buf[4] = (input & 0x000000FF00000000LL) >> 32;
    buf[5] = (input & 0x0000FF0000000000LL) >> 40;
    buf[6] = (input & 0x00FF000000000000LL) >> 48;
    buf[7] = (input & 0xFF00000000000000LL) >> 56;
}

/* Jenkins' hash function */
uint32_t hash(const char* str)
{
    const unsigned char* p = (const unsigned char*)str;
    uint32_t hash = 0;

    if(p != NULL) {
        while(*p) {
            hash += *(p++);
            hash += hash << 10;
            hash ^= hash >> 6;
        }
        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;
    }

    return hash;
}

/* double to binary */
uint32_t double_serialize(double input, uint8_t* buf)
{
    uint32_t size = 0;

    if(!isinf(input) && !isnan(input)) {
        int exp = 0;
        double mant = frexp(input, &exp);
        int64_t mant64 = (int64_t)(mant * INT64_MAX);
        int16_t exp16 = (int16_t)exp;
        *buf = 'x';
        cpu_to_le64(*((uint64_t*)&mant64), buf+1);
        cpu_to_le16(*((uint16_t*)&exp16), buf+9);
        size = 11;
    }
    else if(isnan(input))
        memcpy(buf, "nan", (size = 3));
    else if(input > 0.0)
        memcpy(buf, "+inf", (size = 4));
    else
        memcpy(buf, "-inf", (size = 4));

    return size;
}

double double_deserialize(const uint8_t* buf, uint32_t size)
{
    static const double ZERO = 0.0;
    double value = 0.0;

    if(*buf == 'x' && size == 11) {
        uint64_t mant64 = le64_to_cpu(buf+1);
        uint16_t exp16 = le16_to_cpu(buf+9);
        int exp = (int)(*((int16_t*)&exp16));
        double mant = (double)(*((int64_t*)&mant64)) / INT64_MAX;
        value = ldexp(mant, exp);
    }
    else if(memcmp(buf, "nan", 3) == 0)
        value = NAN;
    else if(memcmp(buf, "+inf", 4) == 0)
        value = 1.0 / ZERO;
    else if(memcmp(buf, "-inf", 4) == 0)
        value = -1.0 / ZERO;

    return value;
}

/* key string comparison */
int keycmp(const char* a, const char* b)
{
    return strcmp(a, b);
}

/* key length (at most maxlen bytes) */
size_t keylen(const char* key, size_t maxlen)
{
    size_t i = 0;
    while(key[i] && i < maxlen)
        i++;
    return i;
}

/* validates an ID: only lowercase alphanumeric characters are accepted */
int is_valid_id(const char* str)
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

    return 0;
}

/* duplicates a string */
char* clone_str(const char* str)
{
    if(str != NULL)
        return strcpy(mallocx((1 + strlen(str)) * sizeof(char)), str);
    else
        return strcpy(mallocx(1 * sizeof(char)), "");
}





/* save & load routines */

/* read an entry from fp
 * returns NULL on corrupt file */
prefsentry_t* read_entry(ALLEGRO_FILE* fp)
{
    prefsentry_t* entry = NULL;
    pfentry_t pfentry;

    /* read type & data size */
    if(sizeof(pfentry.type) == al_fread(fp, &pfentry.type, sizeof(pfentry.type))) {
        uint8_t buf[sizeof(pfentry.data_size)];

        if(sizeof(buf) == al_fread(fp, buf, sizeof(buf))) {
            pfentry.data_size = le32_to_cpu(buf);

            /* check if the type is known */
            switch((prefstype_t)pfentry.type) {
                case PREFS_NULL: {
                    uint8_t* data = mallocx(1 + pfentry.data_size);
                    data[pfentry.data_size] = '\0';

                    if(pfentry.data_size == al_fread(fp, data, pfentry.data_size)) {
                        const char* key = (const char*)data;
                        const uint8_t* value_buf = (data + keylen(key, pfentry.data_size - 1) + 1);
                        int value_size = pfentry.data_size - (value_buf - data);
                        if(value_size == 0)
                            entry = new_null_entry(key);
                    }

                    free(data);
                    break;
                }

                case PREFS_INT32: {
                    uint8_t* data = mallocx(1 + pfentry.data_size);
                    data[pfentry.data_size] = '\0';

                    if(pfentry.data_size == al_fread(fp, data, pfentry.data_size)) {
                        const char* key = (const char*)data;
                        const uint8_t* value_buf = (data + keylen(key, pfentry.data_size - 1) + 1);
                        int value_size = pfentry.data_size - (value_buf - data);
                        if(value_size == sizeof(int32_t)) {
                            uint32_t value = le32_to_cpu(value_buf);
                            entry = new_int_entry(key, *((int32_t*)&value));
                        }
                    }

                    free(data);
                    break;
                }

                case PREFS_FLOAT64: {
                    uint8_t* data = mallocx(1 + pfentry.data_size);
                    data[pfentry.data_size] = '\0';

                    if(pfentry.data_size == al_fread(fp, data, pfentry.data_size)) {
                        const char* key = (const char*)data;
                        const uint8_t* value_buf = (data + keylen(key, pfentry.data_size - 1) + 1);
                        int value_size = pfentry.data_size - (value_buf - data);
                        if(value_size > 0) {
                            double value = double_deserialize(value_buf, value_size);
                            entry = new_double_entry(key, value);
                        }
                    }

                    free(data);
                    break;
                }

                case PREFS_STRING: {
                    uint8_t* data = mallocx(1 + pfentry.data_size);
                    data[pfentry.data_size] = '\0';

                    if(pfentry.data_size == al_fread(fp, data, pfentry.data_size)) {
                        const char* key = (const char*)data;
                        const uint8_t* value_buf = (data + keylen(key, pfentry.data_size - 1) + 1);
                        int value_size = pfentry.data_size - (value_buf - data);
                        if(value_size >= 0) {
                            const char* value = (const char*)value_buf;
                            entry = new_string_entry(key, value);
                        }
                    }

                    free(data);
                    break;
                }

                case PREFS_BOOL: {
                    uint8_t* data = mallocx(1 + pfentry.data_size);
                    data[pfentry.data_size] = '\0';

                    if(pfentry.data_size == al_fread(fp, data, pfentry.data_size)) {
                        const char* key = (const char*)data;
                        const uint8_t* value_buf = (data + keylen(key, pfentry.data_size - 1) + 1);
                        int value_size = pfentry.data_size - (value_buf - data);
                        if(value_size == sizeof(uint8_t)) {
                            uint8_t value = *value_buf;
                            entry = new_bool_entry(key, value != 0);
                        }
                    }

                    free(data);
                    break;
                }

                default: {
                    /* unknown type: ignore field */
                    uint8_t* data = mallocx(1 + pfentry.data_size);
                    data[pfentry.data_size] = '\0';

                    if(pfentry.data_size == al_fread(fp, data, pfentry.data_size)) {
                        const char* key = (const char*)data;
                        entry = new_null_entry(key);
                    }

                    free(data);
                    break;
                }
            }
        }
    }

    /* error? */
    if(al_ferror(fp) != 0 || entry == NULL) {
        prefs_log("Prefs reading error (%d) near byte %ld", al_ferror(fp), al_ftell(fp));
        al_fclearerr(fp);
    }

    /* done */
    return entry;
}

int read_header(ALLEGRO_FILE* fp, pfheader_t* header)
{
    uint8_t buf32[sizeof(uint32_t)];

    /* read file signature */
    if(sizeof(header->magic) != al_fread(fp, header->magic, sizeof(header->magic))) {
        prefs_log("Can't read prefs file signature");
        return 0;
    }

    /* discard unused bytes */
    if(sizeof(header->unused) != al_fread(fp, header->unused, sizeof(header->unused))) {
        prefs_log("Can't read prefs file header: invalid format");
        return 0;
    }

    /* read version code */
    if(sizeof(buf32) != al_fread(fp, buf32, sizeof(buf32))) {
        prefs_log("Can't read prefs file header: expected version code");
        return 0;
    }
    header->version_code = le32_to_cpu(buf32);

    /* read prefsid hash */
    if(sizeof(buf32) != al_fread(fp, buf32, sizeof(buf32))) {
        prefs_log("Can't read prefs file header: expected prefs hash");
        return 0;
    }
    header->prefsid_hash = le32_to_cpu(buf32);

    /* read entry count */
    if(sizeof(buf32) != al_fread(fp, buf32, sizeof(buf32))) {
        prefs_log("Can't read prefs file header: expected entry count");
        return 0;
    }
    header->entry_count = le32_to_cpu(buf32);

    /* done */
    return 1;
}

int validate_header(ALLEGRO_FILE* fp, const prefs_t* prefs, const pfheader_t* header)
{
    /* check magic */
    if(0 != memcmp(header->magic, PREFS_MAGIC, sizeof(header->magic))) {
        prefs_log("Invalid prefs file signature");
        return 0;
    }

    /* validate version */
    if(header->version_code > prefs_version())
        prefs_log("Found newer version of prefs file: engine upgrade is advised");

    /* verify hash */
    if(hash(prefs->prefsid) != header->prefsid_hash) {
        prefs_log("Invalid prefs file hash");
        return 0;
    }

    /* done */
    return 1;
}

int write_entry(ALLEGRO_FILE* fp, const prefsentry_t* entry)
{
    int success = 0;

    /* write the entry */
    if(entry != NULL) {
        pfentry_t pfentry = { .type = entry->type };

        if(sizeof(pfentry.type) == al_fwrite(fp, &pfentry.type, sizeof(pfentry.type))) {
            uint32_t key_size = 1 + strlen(entry->key); /* key + '\0' */
            uint8_t data_size[sizeof(pfentry.data_size)]; /* little endian */

            switch(entry->type) {
                case PREFS_NULL: {
                    uint32_t value_size = 0;
                    cpu_to_le32(key_size + value_size, data_size);
                    if(sizeof(data_size) == al_fwrite(fp, data_size, sizeof(data_size)))
                        success = (key_size == al_fwrite(fp, entry->key, key_size));
                    break;
                }

                case PREFS_INT32: {
                    uint32_t value_size = sizeof(int32_t);
                    cpu_to_le32(key_size + value_size, data_size);
                    if(sizeof(data_size) == al_fwrite(fp, data_size, sizeof(data_size))) {
                        if(key_size == al_fwrite(fp, entry->key, key_size)) {
                            uint8_t value_buf[sizeof(int32_t)];
                            cpu_to_le32(*((uint32_t*)&(entry->value.integer)), value_buf);
                            success = (value_size == al_fwrite(fp, value_buf, value_size));
                        }
                    }
                    break;
                }

                case PREFS_FLOAT64: {
                    uint8_t value_buf[32];
                    uint32_t value_size = double_serialize(entry->value.real, value_buf);
                    cpu_to_le32(key_size + value_size, data_size);
                    if(sizeof(data_size) == al_fwrite(fp, data_size, sizeof(data_size))) {
                        if(key_size == al_fwrite(fp, entry->key, key_size))
                            success = (value_size == al_fwrite(fp, value_buf, value_size));
                    }
                    break;
                }

                case PREFS_STRING: {
                    uint32_t value_size = strlen(entry->value.text);
                    cpu_to_le32(key_size + value_size, data_size);
                    if(sizeof(data_size) == al_fwrite(fp, data_size, sizeof(data_size))) {
                        if(key_size == al_fwrite(fp, entry->key, key_size)) {
                            const uint8_t* value_buf = (const uint8_t*)(entry->value.text);
                            success = (value_size == al_fwrite(fp, value_buf, value_size));
                        }
                    }
                    break;
                }

                case PREFS_BOOL: {
                    uint32_t value_size = sizeof(uint8_t);
                    cpu_to_le32(key_size + value_size, data_size);
                    if(sizeof(data_size) == al_fwrite(fp, data_size, sizeof(data_size))) {
                        if(key_size == al_fwrite(fp, entry->key, key_size)) {
                            uint8_t value = entry->value.boolean;
                            success = (value_size == al_fwrite(fp, &value, value_size));
                        }
                    }
                    break;
                }
            }
        }
    }

    /* error? */
    if(al_ferror(fp) != 0 || !success) {
        prefs_log("Prefs writing error (%d) near byte %ld", al_ferror(fp), al_ftell(fp));
        al_fclearerr(fp);
    }

    /* done */
    return success;
}

int write_header(ALLEGRO_FILE* fp, const prefs_t* prefs)
{
    uint8_t buf32[sizeof(uint32_t)];
    pfheader_t header;
    int success = 1;

    /* compute header data */
    memcpy(header.magic, PREFS_MAGIC, sizeof(header.magic));
    memset(header.unused, 0, sizeof(header.unused));
    header.version_code = prefs_version();
    header.prefsid_hash = hash(prefs->prefsid);
    header.entry_count = prefs_count_entries(prefs);

    /* save header */
    success = success && (sizeof(header.magic) == al_fwrite(fp, header.magic, sizeof(header.magic)));
    success = success && (sizeof(header.unused) == al_fwrite(fp, header.unused, sizeof(header.unused)));
    cpu_to_le32(header.version_code, buf32);
    success = success && (sizeof(buf32) == al_fwrite(fp, buf32, sizeof(buf32)));
    cpu_to_le32(header.prefsid_hash, buf32);
    success = success && (sizeof(buf32) == al_fwrite(fp, buf32, sizeof(buf32)));
    cpu_to_le32(header.entry_count, buf32);
    success = success && (sizeof(buf32) == al_fwrite(fp, buf32, sizeof(buf32)));

    /* error? */
    if(al_ferror(fp) != 0 || !success) {
        prefs_log("Prefs header writing error (%d) near byte %ld", al_ferror(fp), al_ftell(fp));
        al_fclearerr(fp);
    }

    /* done */
    return success;
}

/* load prefs from the disk */
int load(prefs_t* prefs)
{
    const char* filename = assetfs_create_config_file(PREFS_FILE);
    ALLEGRO_FILE* fp = al_fopen(filename, "rb");
    int success = 0;

    prefs_log("Loading prefs from file \"%s\"...", filename);

    /* load file */
    if(fp != NULL) {
        pfheader_t header;
        if(read_header(fp, &header)) {
            if(validate_header(fp, prefs, &header)) {
                int good = 1;
                for(int i = 0; i < header.entry_count && good; i++) {
                    prefsentry_t* entry = read_entry(fp);
                    if(entry != NULL)
                        prefs_add_entry(prefs, entry);
                    else
                        good = 0;
                }
                success = good;
            }
        }
        al_fclose(fp);
    }

    /* error? */
    if(!success) {
        prefs_log(!fp ? "Can't read prefs file!" : "Prefs file is corrupt.");
        prefs_clear(prefs);
    }

    /* done */
    return success;
}

/* save prefs to the disk */
int save(const prefs_t* prefs)
{
    const char* filename = assetfs_create_config_file(PREFS_FILE);
    ALLEGRO_FILE* fp = al_fopen(filename, "wb");
    int success = 0;

    prefs_log("Saving prefs to file \"%s\"...", filename);

    /* save file */
    if(fp != NULL) {
        if(write_header(fp, prefs)) {
            int good = 1;
            for(int i = 0; i < PREFS_MAXBUCKETS && good; i++) {
                for(prefslist_t* l = prefs->bucket[i]; l != NULL && good; l = l->next)
                    good = good && write_entry(fp, l->entry);
            }
            success = good;
        }
        al_fclose(fp);
    }
    else
        prefs_log("Can't open prefs file for writing!");

    /* error? */
    if(!success)
        prefs_log("Can't save prefs to file.");

    /* done */
    return success;
}