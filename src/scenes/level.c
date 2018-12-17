/*
 * Open Surge Engine
 * level.c - code for the game levels
 * Copyright (C) 2008-2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "level.h"
#include "confirmbox.h"
#include "gameover.h"
#include "pause.h"
#include "quest.h"
#include "util/editorgrp.h"
#include "util/editorcmd.h"
#include "../core/scene.h"
#include "../core/storyboard.h"
#include "../core/global.h"
#include "../core/input.h"
#include "../core/fadefx.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/timer.h"
#include "../core/sprite.h"
#include "../core/assetfs.h"
#include "../core/hashtable.h"
#include "../core/stringutil.h"
#include "../core/logfile.h"
#include "../core/lang.h"
#include "../core/soundfactory.h"
#include "../core/nanoparser/nanoparser.h"
#include "../core/font.h"
#include "../core/prefs.h"
#include "../core/modmanager.h"
#include "../entities/actor.h"
#include "../entities/brick.h"
#include "../entities/player.h"
#include "../entities/item.h"
#include "../entities/enemy.h"
#include "../entities/camera.h"
#include "../entities/background.h"
#include "../entities/particle.h"
#include "../entities/renderqueue.h"
#include "../entities/items/flyingtext.h"
#include "../entities/entitymanager.h"
#include "../scripting/scripting.h"
#include "../scenes/editorpal.h"


/* ------------------------
 * Dialog Regions
 *
 * If the player gets inside
 * these regions, a dialog
 * box appears
 * ------------------------ */
/* constants */
#define DIALOGREGION_MAX 100

/* structure */
typedef struct {
    int rect_x, rect_y, rect_w, rect_h; /* region data */
    char title[128], message[1024]; /* dialog box data */
    int disabled; /* TRUE if this region is disabled */
} dialogregion_t;

/* internal data */
static int dialogregion_size; /* size of the vector */
static dialogregion_t dialogregion[DIALOGREGION_MAX];

/* internal methods */
static void update_dialogregions();



/* ------------------------
 * Startup objects
 * ------------------------ */
#define DEFAULT_STARTUP_OBJECT ".default_startup"
typedef struct startupobject_list_t startupobject_list_t;
struct startupobject_list_t {
    char *object_name;
    startupobject_list_t *next;
};
static startupobject_list_t *startupobject_list;
static void init_startup_object_list();
static void release_startup_object_list();
static void add_to_startup_object_list(const char *object_name);
static void spawn_startup_objects();
static int is_startup_object(const char* object_name);



/* ------------------------
 * Level
 * ------------------------ */
/* constants */
#define DEFAULT_MARGIN          (VIDEO_SCREEN_W/2)
#define MAX_POWERUPS            10
#define DLGBOX_MAXTIME          7000
#define TEAM_MAX                16
#define DEFAULT_WATERLEVEL      INFINITY
#define DEFAULT_WATERCOLOR      image_rgb(0,32,192)

/* level attributes */
static char file[1024];
static char musicfile[1024];
static char theme[1024];
static char bgtheme[1024];
static char grouptheme[1024];
static char name[128];
static char author[128];
static char version[128];
static char license[128];
static int act;
static int requires[3]; /* this level requires engine version x.y.z */
static int readonly; /* we can't activate the level editor */

/* internal data */
static float gravity;
static int level_width;/* width of this level (in pixels) */
static int level_height; /* height of this level (in pixels) */
static float level_timer;
static v2d_t spawn_point;
static music_t *music;
static sound_t *override_music;
static int block_music;
static int quit_level;
static image_t *quit_level_img;
static bgtheme_t *backgroundtheme;
static int must_load_another_level;
static int must_restart_this_level;
static int must_push_a_quest;
static char quest_to_be_pushed[1024];
static float dead_player_timeout;
static int waterlevel; /* in pixels */
static uint32 watercolor;

/* player data */
static player_t *team[TEAM_MAX]; /* players */
static int team_size; /* size of team[] */
static player_t *player; /* reference to the current player */

/* camera */
static actor_t *camera_focus;

/* scripting controlled variables */
static int level_cleared; /* display the level cleared animation */
static int jump_to_next_stage; /* jumps to the next stage in the quest */
static int wants_to_leave; /* wants to abort the level */
static int wants_to_pause; /* wants to pause the level */

/* dialog box */
static int dlgbox_active;
static uint32 dlgbox_starttime;
static actor_t *dlgbox;
static font_t *dlgbox_title, *dlgbox_message;

/* level management */
static void level_load(const char *filepath);
static void level_unload();
static int level_save(const char *filepath);
static void level_interpret_line(const char *filename, int fileline, const char *line);
static void level_interpret_parsed_line(const char *filename, int fileline, const char *identifier, int param_count, const char **param);

/* internal methods */
static int inside_screen(int x, int y, int w, int h, int margin);
static void update_level_size();
static void restart(int preserve_current_spawnpoint);
static void render_players();
static void update_music();
static void spawn_players();
static void render_level(brick_list_t *major_bricks, item_list_t *major_items, enemy_list_t *major_enemies); /* render bricks, items, enemies, players, etc. */
static void render_hud(enemy_list_t *major_enemies); /* gui / hud related */
static void render_powerups(); /* gui / hud related */
static void render_dlgbox(); /* dialog boxes */
static void update_dlgbox(); /* dialog boxes */
static void reconfigure_players_input_devices();

/* Scripting */
#define TRANSFORM_MAX_DEPTH 64
static surgescript_object_t* cached_level_ssobject = NULL;
static void update_ssobjects();
static void update_ssobject(surgescript_object_t* object, void* param);
static void late_update_ssobject(surgescript_object_t* object, void* param);
static void render_ssobjects();
static bool render_ssobject(surgescript_object_t* object, void* param);
static bool ssobject_exists(const char* object_name);
static surgescript_object_t* level_ssobject();
static surgescript_object_t* spawn_ssobject(const char* object_name, v2d_t spawn_point, int spawned_in_the_editor);
static bool save_ssobject(surgescript_object_t* object, void* param);
typedef struct ssobj_extradata_t ssobj_extradata_t;
struct ssobj_extradata_t { v2d_t spawn_point; int spawned_in_the_editor; };
static const char* hash_of_ssobj(const surgescript_object_t* object);
static v2d_t get_ssobj_spawnpoint(const surgescript_object_t* object);
static int is_ssobj_spawned_in_the_editor(const surgescript_object_t* object);
static ssobj_extradata_t* get_ssobj_extradata(const surgescript_object_t* object);
static void set_ssobj_extradata(const surgescript_object_t* object, ssobj_extradata_t extradata);
static void free_ssobj_extradata(ssobj_extradata_t* data);
HASHTABLE_GENERATE_CODE(ssobj_extradata_t);
static hashtable_ssobj_extradata_t* ssobj_extradata = NULL;



/* ------------------------
 * Level Editor
 * ------------------------ */

/* methods */
static void editor_init();
static void editor_release();
static void editor_enable();
static void editor_disable();
static void editor_update();
static void editor_render();
static int editor_is_enabled();
static int editor_want_to_activate();
static void editor_update_background();
static void editor_render_background();
static void editor_movable_platforms_path_render(brick_list_t *major_bricks);
static void editor_waterline_render(int ycoord, uint32 color);
static void editor_save();
static void editor_scroll();
static int editor_is_eraser_enabled();

/* object type */
enum editor_entity_type {
    EDT_BRICK,
    EDT_ITEM,
    EDT_ENEMY,
    EDT_GROUP,
    EDT_SSOBJ
};
#define EDITORGRP_ENTITY_TO_EDT(t) \
                ( ((t) == EDITORGRP_ENTITY_BRICK) ? EDT_BRICK : \
                ((t) == EDITORGRP_ENTITY_ITEM) ? EDT_ITEM : \
                EDT_ENEMY );



/* internal stuff */
static int editor_enabled; /* is the level editor enabled? */
static int editor_previous_video_resolution;
static int editor_previous_video_smooth;
static editorcmd_t* editor_cmd;
static v2d_t editor_camera, editor_cursor;
static enum editor_entity_type editor_cursor_entity_type;
static int editor_cursor_entity_id, editor_cursor_itemid;
static font_t *editor_cursor_font;
static font_t *editor_properties_font;
static font_t *editor_help_font;
static const char* editor_entity_class(enum editor_entity_type objtype);
static const char* editor_entity_info(enum editor_entity_type objtype, int objid);
static void editor_draw_object(enum editor_entity_type obj_type, int obj_id, v2d_t position);
static void editor_next_class();
static void editor_previous_class();
static void editor_next_entity();
static void editor_previous_entity();

/* editor: legacy items */
static int editor_item_list[] = {
    IT_RING, IT_LIFEBOX, IT_RINGBOX, IT_STARBOX, IT_SPEEDBOX, IT_GLASSESBOX, IT_TRAPBOX,
    IT_SHIELDBOX, IT_FIRESHIELDBOX, IT_THUNDERSHIELDBOX, IT_WATERSHIELDBOX,
    IT_ACIDSHIELDBOX, IT_WINDSHIELDBOX,
    IT_LOOPGREEN, IT_LOOPYELLOW,
    IT_YELLOWSPRING, IT_BYELLOWSPRING, IT_RYELLOWSPRING, IT_LYELLOWSPRING,
    IT_TRYELLOWSPRING, IT_TLYELLOWSPRING, IT_BRYELLOWSPRING, IT_BLYELLOWSPRING,
    IT_REDSPRING, IT_BREDSPRING, IT_RREDSPRING, IT_LREDSPRING,
    IT_TRREDSPRING, IT_TLREDSPRING, IT_BRREDSPRING, IT_BLREDSPRING,
    IT_BLUESPRING, IT_BBLUESPRING, IT_RBLUESPRING, IT_LBLUESPRING,
    IT_TRBLUESPRING, IT_TLBLUESPRING, IT_BRBLUESPRING, IT_BLBLUESPRING,
    IT_BLUERING, IT_SWITCH, IT_DOOR, IT_TELEPORTER, IT_BIGRING, IT_CHECKPOINT, IT_GOAL,
    IT_ENDSIGN, IT_ENDLEVEL, IT_BUMPER,
    IT_DANGER, IT_VDANGER, IT_FIREDANGER, IT_VFIREDANGER,
    IT_SPIKES, IT_CEILSPIKES, IT_LWSPIKES, IT_RWSPIKES, IT_PERSPIKES,
    IT_PERCEILSPIKES, IT_PERLWSPIKES, IT_PERRWSPIKES, IT_DNADOOR, IT_DNADOORNEON,
    IT_DNADOORCHARGE, IT_HDNADOOR, IT_HDNADOORNEON, IT_HDNADOORCHARGE,
    -1 /* -1 represents the end of this list */
};
static int editor_item_list_size; /* counted automatically */
static int editor_item_list_get_index(int item_id);
int editor_is_valid_item(int item_id); /* this is used by editorgrp */

/* editor: legacy objects (enemy type) */
static const char** editor_enemy_name;
static int editor_enemy_name_length;
int editor_enemy_name2key(const char *name);
const char* editor_enemy_key2name(int key);
static const char** editor_enemy_category;
static int editor_enemy_category_length;
static int editor_enemy_selected_category_id; /* a value between 0 and editor_enemy_category_length - 1, inclusive */
static const char* editor_enemy_selected_category();
static void editor_next_object_category();
static void editor_previous_object_category();

/* editor: SurgeScript entities */
static char** editor_ssobj; /* the names of all SurgeScript entities; a vector of strings */
static int editor_ssobj_count; /* length of editor_ssobj */
static void editor_ssobj_init();
static void editor_ssobj_release();
static void editor_ssobj_register(const char* entity_name, void* data); /* internal */
static int editor_ssobj_id(const char* entity_name); /* an index k such that editor_ssobj[k] is entity_name */
static const char* editor_ssobj_name(int entity_id); /* the inverse of editor_ssobj_id() */
static bool editor_remove_ssobj(surgescript_object_t* object, void* data);
static bool editor_pick_ssobj(surgescript_object_t* object, void* data);

/* editor: bricks */
static int* editor_brick; /* an array of all valid brick numbers */
static int editor_brick_count; /* length of editor_brick */
static bricklayer_t editor_layer; /* current layer */
static brickflip_t editor_flip; /* flip flags */
static void editor_brick_init();
static void editor_brick_release();
static int editor_brick_index(int brick_id); /* index of brick_id at editor_brick[] */
static int editor_brick_id(int index); /* the index-th valid brick - at editor_brick[] */

/* editor: grid */
#define EDITOR_GRID_W         (int)(editor_grid_size().x)
#define EDITOR_GRID_H         (int)(editor_grid_size().y)
static int editor_grid_enabled; /* is the grid enabled? */
static void editor_grid_init();
static void editor_grid_release();
static void editor_grid_update();
static v2d_t editor_grid_size(); /* returns the size of the grid */
static v2d_t editor_grid_snap(v2d_t position); /* aligns position to a cell in the grid */


/* implementing UNDO and REDO */

/* in order to implement UNDO and REDO,
 * we must register the actions of the
 * user */

/* action type */
enum editor_action_type {
    EDA_NEWOBJECT,
    EDA_DELETEOBJECT,
    EDA_CHANGESPAWN,
    EDA_RESTORESPAWN
};

/* action = action type + action properties */
typedef struct editor_action_t {
    enum editor_action_type type;
    enum editor_entity_type obj_type;
    int obj_id;
    v2d_t obj_position;
    v2d_t obj_old_position;
    bricklayer_t layer;
    brickflip_t flip;
} editor_action_t;

/* action: constructors */
static editor_action_t editor_action_entity_new(int is_new_object, enum editor_entity_type obj_type, int obj_id, v2d_t obj_position);
static editor_action_t editor_action_spawnpoint_new(int is_changing, v2d_t obj_position, v2d_t obj_old_position);

/* linked list */
typedef struct editor_action_list_t {
    editor_action_t action; /* node data */
    int in_group; /* is this element part of a group? */
    uint32 group_key; /* internal use (used only if in_group is true) */
    struct editor_action_list_t *prev, *next; /* linked list: pointers */
} editor_action_list_t;

/* data */
editor_action_list_t *editor_action_buffer;
editor_action_list_t *editor_action_buffer_head; /* sentinel */
editor_action_list_t *editor_action_buffer_cursor;
    
/* methods */
static void editor_action_init();
static void editor_action_release();
static void editor_action_undo();
static void editor_action_redo();
static void editor_action_commit(editor_action_t action);
static void editor_action_register(editor_action_t action); /* internal */
static editor_action_list_t* editor_action_delete_list(editor_action_list_t *list); /* internal */












/* level-specific functions */

/*
 * level_load()
 * Loads a level from a file
 */
void level_load(const char *filepath)
{
    char line[1024];
    const char* fullpath;
    FILE* fp;

    logfile_message("level_load(\"%s\")", filepath);
    fullpath = assetfs_fullpath(filepath);

    /* default values */
    str_cpy(file, filepath, sizeof(file)); /* it's the relative filepath we want */
    str_cpy(name, "Untitled", sizeof(name));
    strcpy(musicfile, "");
    strcpy(theme, "");
    strcpy(bgtheme, "");
    strcpy(author, "");
    strcpy(version, "");
    strcpy(license, "");
    strcpy(grouptheme, "");
    spawn_point = v2d_new(0,0);
    dialogregion_size = 0;
    act = 1;
    requires[0] = GAME_VERSION;
    requires[1] = GAME_SUB_VERSION;
    requires[2] = GAME_WIP_VERSION;
    readonly = FALSE;
    waterlevel = DEFAULT_WATERLEVEL;
    watercolor = DEFAULT_WATERCOLOR;

    /* scripting: preparing a new Level... */
    cached_level_ssobject = NULL;
    ssobj_extradata = hashtable_ssobj_extradata_t_create(free_ssobj_extradata);
    surgescript_object_call_function(scripting_util_surgeengine_component(surgescript_vm(), "LevelManager"), "onLevelLoad", NULL, 0, NULL);

    /* entity manager */
    entitymanager_init();

    /* startup objects (1) */
    init_startup_object_list();

    /* traversing the level file */
    fp = fopen(fullpath, "r");
    if(fp != NULL) {
        int ln = 0;
        while(fgets(line, sizeof(line) / sizeof(char), fp)) {
            line[strlen(line)-1] = 0; /* no newlines, please! */
            level_interpret_line(fullpath, ++ln, line);
        }
        fclose(fp);
    }
    else
        fatal_error("Can\'t open level file \"%s\".", fullpath);

    /* players */
    if(team_size == 0) {
        /* default players */
        logfile_message("Loading the default players...");
        team[team_size++] = player_create("Surge");
        team[team_size++] = player_create("Neon");
        team[team_size++] = player_create("Charge");
    }
    level_change_player(team[0]);
    spawn_players();
    camera_init();
    camera_set_position(player->actor->position);
    player_set_collectibles(0);
    surgescript_object_call_function(scripting_util_surgeengine_component(surgescript_vm(), "Player"), "__spawnPlayers", NULL, 0, NULL);

    /* startup objects (2) */
    spawn_startup_objects();

    /* load the music */
    block_music = FALSE;
    music = *musicfile ? music_load(musicfile) : NULL;

    /* misc */
    update_level_size();
    backgroundtheme = background_load(bgtheme);

    /* success! */
    logfile_message("level_load() ok");
}

/*
 * level_unload()
 * Call manually after level_load() whenever
 * this level has to be released or changed
 */
void level_unload()
{
    int i;

    logfile_message("level_unload()");
    if(music != NULL) {
        music_stop();
        music_unref(music);
    }
    /*music_unref("musics/invincible.ogg");
    music_unref("musics/speed.ogg");*/

    /* releases the startup object list */
    release_startup_object_list();

    /* entity manager */
    entitymanager_release();

    /* scripting */
    surgescript_object_call_function(scripting_util_surgeengine_component(surgescript_vm(), "LevelManager"), "onLevelUnload", NULL, 0, NULL);
    ssobj_extradata = hashtable_ssobj_extradata_t_destroy(ssobj_extradata);
    cached_level_ssobject = NULL;

    /* unloading the brickset */
    logfile_message("Unloading the brickset...");
    brickset_unload();

    /* unloading the background */
    logfile_message("Unloading the background...");
    backgroundtheme = background_unload(backgroundtheme);

    /* destroying the players */
    logfile_message("Unloading the players...");
    for(i=0; i<team_size; i++)
        player_destroy(team[i]);
    team_size = 0;
    player = NULL;

    /* success! */
    logfile_message("level_unload() ok");
}


/*
 * level_save()
 * Saves the current level to a file
 * Returns TRUE on success
 */
int level_save(const char *filepath)
{
    int i;
    FILE *fp;
    const char* fullpath;
    brick_list_t *itb, *brick_list;
    item_list_t *iti, *item_list;
    enemy_list_t *ite, *object_list;
    startupobject_list_t *its;

    brick_list = entitymanager_retrieve_all_bricks();
    item_list = entitymanager_retrieve_all_items();
    object_list = entitymanager_retrieve_all_objects();

    fullpath = assetfs_create_data_file(filepath, false);

    /* open for writing */
    logfile_message("level_save(\"%s\")", fullpath);
    if(NULL == (fp=fopen(fullpath, "w"))) {
        logfile_message("Warning: could not open \"%s\" for writing.", fullpath);
        video_showmessage("Could not open \"%s\" for writing.", fullpath);
        return FALSE;
    }

    /* level disclaimer */
    fprintf(fp,
    "// ------------------------------------------------------------\n"
    "// %s %s level\n"
    "// This file was generated by the built-in level editor.\n"
    "// %s\n"
    "// ------------------------------------------------------------\n\n",
    GAME_TITLE, GAME_VERSION_STRING, GAME_WEBSITE);

    /* header */
    fprintf(fp,
    "// header\n"
    "name \"%s\"\n",
    str_addslashes(name));

    /* author */
    fprintf(fp, "author \"%s\"\n", str_addslashes(author));
    if(strcmp(license, "") != 0)
        fprintf(fp, "license \"%s\"\n", license);

    /* level attributes */
    fprintf(fp, 
    "version \"%s\"\n"
    "requires \"%d.%d.%d\"\n"
    "act %d\n"
    "theme \"%s\"\n"
    "bgtheme \"%s\"\n"
    "spawn_point %d %d\n",
    version,
    GAME_VERSION, GAME_SUB_VERSION, GAME_WIP_VERSION,
    act, theme, bgtheme,
    (int)spawn_point.x, (int)spawn_point.y);

    /* music? */
    if(strcmp(musicfile, "") != 0)
        fprintf(fp, "music \"%s\"\n", musicfile);

    /* grouptheme? */
    if(strcmp(grouptheme, "") != 0)
        fprintf(fp, "grouptheme \"%s\"\n", grouptheme);

    /* startup objects? */
    fprintf(fp, "startup");
    for(its=startupobject_list; its; its=its->next)
        fprintf(fp, " \"%s\"", str_addslashes(its->object_name));
    fprintf(fp, "\n");

    /* players */
    fprintf(fp, "players");
    for(i=0; i<team_size; i++)
        fprintf(fp, " \"%s\"", str_addslashes(team[i]->name));
    fprintf(fp, "\n");

    /* read only? */
    if(readonly)
        fprintf(fp, "readonly\n");

    /* water */
    if(waterlevel != DEFAULT_WATERLEVEL)
        fprintf(fp, "waterlevel %d\n", waterlevel);
    if(watercolor != DEFAULT_WATERCOLOR) {
        uint8 r, g, b;
        image_color2rgb(watercolor, &r, &g, &b);
        fprintf(fp, "watercolor %d %d %d\n", r, g, b);
    }

    /* dialog regions */
    fprintf(fp, "\n// dialogs\n");
    for(i=0; i<dialogregion_size; i++) {
        char title[256], message[1024];
        str_cpy(title, str_addslashes(dialogregion[i].title), sizeof(title));
        str_cpy(message, str_addslashes(dialogregion[i].message), sizeof(message));
        fprintf(fp, "dialogbox %d %d %d %d \"%s\" \"%s\"\n", dialogregion[i].rect_x, dialogregion[i].rect_y, dialogregion[i].rect_w, dialogregion[i].rect_h, title, message);
    }

    /* brick list */
    fprintf(fp, "\n// bricks\n");
    for(itb=brick_list; itb; itb=itb->next)  {
        if(brick_is_alive(itb->data)) {
            fprintf(fp,
                "brick %d %d %d%s%s%s%s\n",
                brick_id(itb->data),
                (int)(brick_spawnpoint(itb->data).x), (int)(brick_spawnpoint(itb->data).y),
                brick_layer(itb->data) != BRL_DEFAULT ? " " : "",
                brick_layer(itb->data) != BRL_DEFAULT ? brick_util_layername(brick_layer(itb->data)) : "",
                brick_flip(itb->data) != BRF_NOFLIP ? " " : "",
                brick_flip(itb->data) != BRF_NOFLIP ? brick_util_flipstr(brick_flip(itb->data)) : ""
            );
        }
    }

    /* SurgeScript entity list */
    fprintf(fp, "\n// entities\n");
    surgescript_object_traverse_tree_ex(level_ssobject(), fp, save_ssobject);

    /* item list */
    fprintf(fp, "\n// legacy items\n");
    for(iti=item_list; iti; iti=iti->next) {
        if(iti->data->state != IS_DEAD)
           fprintf(fp, "item %d %d %d\n", iti->data->type, (int)iti->data->actor->spawn_point.x, (int)iti->data->actor->spawn_point.y);
    }

    /* legacy object list */
    fprintf(fp, "\n// legacy objects\n");
    for(ite=object_list; ite; ite=ite->next) {
        if(ite->data->created_from_editor && ite->data->state != ES_DEAD)
            fprintf(fp, "object \"%s\" %d %d\n", str_addslashes(ite->data->name), (int)ite->data->actor->spawn_point.x, (int)ite->data->actor->spawn_point.y);
    }

    /* done! */
    fprintf(fp, "\n// EOF");
    fclose(fp);
    logfile_message("level_save() ok");

    brick_list = entitymanager_release_retrieved_brick_list(brick_list);
    item_list = entitymanager_release_retrieved_item_list(item_list);
    object_list = entitymanager_release_retrieved_object_list(object_list);

    return TRUE;
}

/*
 * level_interpret_line()
 * Interprets a line from the .lev file
 */
void level_interpret_line(const char *filename, int fileline, const char *line)
{
    int param_count;
    char *param[32], *identifier;
    char tmp[1024], *p, *q;
    int i, sz = sizeof(tmp)-1;

    /* skip spaces */
    for(p=(char*)line; isspace((int)*p); p++);
    if(0 == *p) return;

    /* reading the identifier */
    for(q=tmp; *p && !isspace(*p) && q<tmp+sz; *q++ = *p++); *q=0;
    if(tmp[0] == '/' && tmp[1] == '/') return; /* comment */
    if(tmp[0] == '#' && isspace(tmp[1])) return; /* comment */
    identifier = str_dup(tmp);

    /* skip spaces */
    for(; isspace((int)*p); p++);

    /* read the arguments */
    param_count = 0;
    if(0 != *p) {
        int quotes;
        while(*p && param_count<32) {
            quotes = (*p == '"') && !!(p++); /* short-circuit AND */
            for(q=tmp; *p && ((!quotes && !isspace(*p)) || (quotes && !(*p == '"' && *(p-1) != '\\'))) && q<tmp+sz; *q++ = *p++); *q=0;
            quotes = (*p == '"') && !!(p++);
            param[param_count++] = str_dup(tmp);
            for(; isspace((int)*p); p++); /* skip spaces */
        }
    }

    /* interpret the line */
    level_interpret_parsed_line(filename, fileline, identifier, param_count, (const char**)param);

    /* free the stuff */
    for(i=0; i<param_count; i++)
        free(param[i]);
    free(identifier);
}

/*
 * level_interpret_parsed_line()
 * Interprets a line parsed by level_interpret_line()
 */
void level_interpret_parsed_line(const char *filename, int fileline, const char *identifier, int param_count, const char **param)
{
    /* interpreting the command */
    if(str_icmp(identifier, "theme") == 0) {
        if(param_count == 1) {
            if(!brickset_loaded()) {
                str_cpy(theme, param[0], sizeof(theme));
                brickset_load(theme);
            }
        }
        else
            logfile_message("Level loader - command 'theme' expects one parameter: brickset filepath. Did you forget to double quote the brickset filepath?");
    }
    else if(str_icmp(identifier, "bgtheme") == 0) {
        if(param_count == 1)
            str_cpy(bgtheme, param[0], sizeof(bgtheme));
        else
            logfile_message("Level loader - command 'bgtheme' expects one parameter: background filepath. Did you forget to double quote the background filepath?");
    }
    else if(str_icmp(identifier, "grouptheme") == 0) {
        if(param_count == 1)
            str_cpy(grouptheme, param[0], sizeof(grouptheme));
        else
            logfile_message("Level loader - command 'grouptheme' expects one parameter: grouptheme filepath. Did you forget to double quote the grouptheme filepath?");
    }
    else if(str_icmp(identifier, "music") == 0) {
        if(param_count == 1)
            str_cpy(musicfile, param[0], sizeof(musicfile));
        else
            logfile_message("Level loader - command 'music' expects one parameter: music filepath. Did you forget to double quote the music filepath?");
    }
    else if(str_icmp(identifier, "name") == 0) {
        if(param_count == 1)
            str_cpy(name, param[0], sizeof(name));
        else
            logfile_message("Level loader - command 'name' expects one parameter: level name. Did you forget to double quote the level name?");
    }
    else if(str_icmp(identifier, "author") == 0) {
        if(param_count == 1)
            str_cpy(author, param[0], sizeof(name));
        else
            logfile_message("Level loader - command 'author' expects one parameter: author name. Did you forget to double quote the author name?");
    }
    else if(str_icmp(identifier, "version") == 0) {
        if(param_count == 1)
            str_cpy(version, param[0], sizeof(name));
        else
            logfile_message("Level loader - command 'version' expects one parameter: level version");
    }
    else if(str_icmp(identifier, "license") == 0) {
        if(param_count == 1)
            str_cpy(license, param[0], sizeof(license));
        else
            logfile_message("Level loader - command 'license' expects one parameter: license name. Did you forget to double quote the license parameter?");
    }
    else if(str_icmp(identifier, "requires") == 0) {
        if(param_count == 1) {
            int i;
            requires[0] = requires[1] = requires[2] = 0;
            sscanf(param[0], "%d.%d.%d", &requires[0], &requires[1], &requires[2]);
            for(i=0; i<3; i++) requires[i] = clip(requires[i], 0, 99);
            if(game_version_compare(requires[0], requires[1], requires[2]) < 0) {
                fatal_error(
                    "This level requires version %d.%d.%d or greater of the game engine.\nYours is %s\nPlease check out for new versions at %s",
                    requires[0], requires[1], requires[2], GAME_VERSION_STRING, GAME_WEBSITE
                );
            }
        }
        else
            logfile_message("Level loader - command 'requires' expects one parameter: minimum required engine version");
    }
    else if(str_icmp(identifier, "act") == 0) {
        if(param_count == 1)
            act = atoi(param[0]);
        else
            logfile_message("Level loader - command 'act' expects one parameter: act number");
    }
    else if(str_icmp(identifier, "waterlevel") == 0) {
        if(param_count == 1)
            waterlevel = atoi(param[0]);
        else
            logfile_message("Level loader - command 'waterlevel' expects one parameter: water level (y-coordinate, in pixels)");
    }
    else if(str_icmp(identifier, "watercolor") == 0) {
        if(param_count == 3)
            watercolor = image_rgb(atoi(param[0]), atoi(param[1]), atoi(param[2]));
        else
            logfile_message("Level loader - command 'watercolor' expects three parameters: red, green, blue");
    }
    else if(str_icmp(identifier, "spawn_point") == 0) {
        if(param_count == 2) {
            int x = atoi(param[0]);
            int y = atoi(param[1]);
            spawn_point = v2d_new(x, y);
        }
        else
            logfile_message("Level loader - command 'spawn_point' expects two parameters: xpos, ypos");
    }
    else if(str_icmp(identifier, "dialogbox") == 0) {
        if(param_count == 6 && dialogregion_size < DIALOGREGION_MAX) {
            dialogregion_t *d = &(dialogregion[dialogregion_size++]);
            d->disabled = FALSE;
            d->rect_x = atoi(param[0]);
            d->rect_y = atoi(param[1]);
            d->rect_w = atoi(param[2]);
            d->rect_h = atoi(param[3]);
            str_cpy(d->title, param[4], sizeof(d->title));
            str_cpy(d->message, param[5], sizeof(d->message));
        }
        else
            logfile_message("Level loader - command 'dialogbox' expects six parameters: rect_xpos, rect_ypos, rect_width, rect_height, title, message. Did you forget to double quote the message?");
    }
    else if(str_icmp(identifier, "readonly") == 0) {
        if(param_count == 0)
            readonly = TRUE;
        else
            logfile_message("Level loader - command 'readonly' expects no parameters");
    }
    else if(str_icmp(identifier, "brick") == 0) {
        if(param_count >= 3) {
            if(*theme != 0) {
                bricklayer_t layer = BRL_DEFAULT;
                brickflip_t flip = BRF_NOFLIP;
                int id = atoi(param[0]);
                int x = atoi(param[1]);
                int y = atoi(param[2]);

                for(int j = 3; j < param_count; j++) {
                    if(layer == BRL_DEFAULT && brick_util_layercode(param[j]) != BRL_DEFAULT)
                        layer = brick_util_layercode(param[j]);
                    else if(flip == BRF_NOFLIP && brick_util_flipcode(param[j]) != BRF_NOFLIP)
                        flip = brick_util_flipcode(param[j]);
                }

                if(brick_exists(id))
                    level_create_brick(id, v2d_new(x,y), layer, flip);
                else
                    logfile_message("Level loader - invalid brick: %d", id);
            }
            else
                logfile_message("Level loader - warning: cannot create a new brick if the theme is not defined");
        }
        else
            logfile_message("Level loader - command 'brick' expects three or four parameters: id, xpos, ypos [, layer_name [, flip_flags]]");
    }
    else if(str_icmp(identifier, "item") == 0) {
        if(param_count == 3) {
            int type = clip(atoi(param[0]), 0, ITEMDATA_MAX-1);
            int x = atoi(param[1]);
            int y = atoi(param[2]);
            level_create_item(type, v2d_new(x, y));
        }
        else
            logfile_message("Level loader - command 'item' expects three parameters: type, xpos, ypos");
    }
    else if(str_icmp(identifier, "enemy") == 0 || str_icmp(identifier, "object") == 0) {
        if(param_count == 3) {
            const char* name = param[0];
            int x = atoi(param[1]);
            int y = atoi(param[2]);
            if(!is_startup_object(name))
                level_create_enemy(name, v2d_new(x, y)); /* old API */
        }
        else
            logfile_message("Level loader - command '%s' expects three parameters: name, xpos, ypos", identifier);
    }
    else if(str_icmp(identifier, "entity") == 0) {
        if(param_count == 3) {
            const char* name = param[0];
            int x = atoi(param[1]);
            int y = atoi(param[2]);
            if(!is_startup_object(name)) {
                surgescript_object_t* obj = level_create_ssobject(name, v2d_new(x, y));
                if(obj != NULL) {
                    if(!surgescript_object_has_tag(obj, "entity"))
                        fatal_error("Level loader - can't spawn \"%s\": object is not an entity", name);
                }
                else
                    fatal_error("Level loader - can't spawn \"%s\": entity does not exist", name);
            }
        }
        else
            logfile_message("Level loader - command 'entity' expects three parameters: name, xpos, ypos");
    }
    else if(str_icmp(identifier, "startup") == 0) {
        if(param_count > 0) {
            for(int i = param_count - 1; i >= 0; i--)
                add_to_startup_object_list(param[i]);
        }
        else
            logfile_message("Level loader - command 'startup' expects one or more parameters: object_name1 [, object_name2 [, ... [, object_nameN] ... ] ]");
    }
    else if(str_icmp(identifier, "players") == 0) {
        if(param_count > 0) {
            for(int i = 0; i < param_count; i++) {
                if(team_size < TEAM_MAX) {
                    for(int j = 0; j < team_size; j++) {
                        if(strcmp(team[j]->name, param[i]) == 0)
                            fatal_error("Level loader - duplicate entry of player '%s' in '%s' near line %d", param[i], filename, fileline);
                    }

                    logfile_message("Loading player '%s'...", param[i]);
                    team[team_size++] = player_create(param[i]);
                }
                else
                    fatal_error("Level loader - can't have more than %d players per level in '%s' near line %d", TEAM_MAX, filename, fileline);
            }
        }
        else
            logfile_message("Level loader - command 'players' expects one or more parameters: character_name1 [, character_name2 [, ... [, character_nameN] ... ] ]");
    }
    else
        logfile_message("Level loader - unknown command '%s'\nin '%s' near line %d", identifier, filename, fileline);
}



/* scene functions */

/*
 * level_init()
 * Initializes the scene
 */
void level_init(void *path_to_lev_file)
{
    const char *filepath = (const char*)path_to_lev_file;
    int i;

    logfile_message("level_init()");
    video_display_loading_screen();

    /* main init */
    gravity = 787.5;
    level_width = level_height = 0;
    level_timer = 0;
    dialogregion_size = 0;
    override_music = NULL;
    quit_level = FALSE;
    quit_level_img = image_create(image_width(video_get_backbuffer()), image_height(video_get_backbuffer()));
    backgroundtheme = NULL;
    must_load_another_level = FALSE;
    must_restart_this_level = FALSE;
    must_push_a_quest = FALSE;
    dead_player_timeout = 0;
    team_size = 0;
    for(i=0; i<TEAM_MAX; i++)
        team[i] = NULL;
    player = NULL;
    music = NULL;

    /* scripting controlled variables */
    level_cleared = FALSE;
    jump_to_next_stage = FALSE;

    /* helpers */
    particle_init();
    music_stop();

    /* level init */
    level_load(filepath);
    spawn_players();

    /* dialog box */
    dlgbox_active = FALSE;
    dlgbox_starttime = 0;
    dlgbox = actor_create();
    dlgbox->position.y = VIDEO_SCREEN_H;
    actor_change_animation(dlgbox, sprite_get_animation("SD_DIALOGBOX", 0));
    dlgbox_title = font_create("sans");
    dlgbox_message = font_create("sans");

    /* editor */
    editor_init();

    /* done! */
    logfile_message("level_init() ok");
}


/*
 * level_update()
 * Updates the scene (this one runs
 * every cycle of the program)
 */
void level_update()
{
    int i, cbox;
    int got_dying_player = FALSE;
    int block_quit = FALSE, block_pause = FALSE;
    float dt = timer_get_delta();
    brick_list_t *major_bricks, *bnode;
    item_list_t *major_items, *inode;
    enemy_list_t *major_enemies, *enode;
    v2d_t cam = level_editmode() ? editor_camera : camera_get_position();

    entitymanager_remove_dead_bricks();
    entitymanager_remove_dead_items();
    entitymanager_remove_dead_objects();

    /* next stage in the quest... */
    if(jump_to_next_stage) {
        jump_to_next_stage = FALSE;
        scenestack_pop();
        return;
    }

    /* must load another level? */
    if(must_load_another_level) {
        must_load_another_level = FALSE;
        restart(FALSE);
        return;
    }

    /* must restart the current level? */
    if(must_restart_this_level) {
        must_restart_this_level = FALSE;
        restart(TRUE);
        return;
    }

    /* must push a quest? */
    if(must_push_a_quest) {
        must_push_a_quest = FALSE;
        scenestack_pop();
        quest_setlevel(quest_currentlevel() - 1);
        scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)quest_to_be_pushed);
        return;
    }

    /* music */
    update_music();

    /* level editor */
    if(editor_is_enabled()) {
        entitymanager_set_active_region(
            (int)cam.x - VIDEO_SCREEN_W/2 - DEFAULT_MARGIN,
            (int)cam.y - VIDEO_SCREEN_H/2 - DEFAULT_MARGIN,
            VIDEO_SCREEN_W + 2*DEFAULT_MARGIN,
            VIDEO_SCREEN_H + 2*DEFAULT_MARGIN
        );
        editor_update();
        return;
    }

    /* should we quit due to scripting? */
    if(!surgescript_vm_is_active(surgescript_vm())) {
        game_quit();
        return;
    }

    /* displaying message: "do you really want to quit?" */
    block_quit = FALSE;
    for(i=0; i<team_size && !block_quit; i++)
        block_quit = player_is_dying(team[i]);

    if(wants_to_leave && !block_quit) {
        char op[3][512];
        confirmboxdata_t cbd = { op[0], op[1], op[2] };

        wants_to_leave = FALSE;
        image_blit(video_get_backbuffer(), quit_level_img, 0, 0, 0, 0, image_width(quit_level_img), image_height(quit_level_img));
        music_pause();

        lang_getstring("CBOX_QUIT_QUESTION", op[0], sizeof(op[0]));
        lang_getstring("CBOX_QUIT_OPTION1", op[1], sizeof(op[1]));
        lang_getstring("CBOX_QUIT_OPTION2", op[2], sizeof(op[2]));

        scenestack_push(storyboard_get_scene(SCENE_CONFIRMBOX), (void*)&cbd);
        return;
    }

    cbox = confirmbox_selected_option();
    if(cbox == 1)
        quit_level = TRUE;
    else if(cbox == 2)
        music_resume();

    if(quit_level) {
        music_stop();
        if(fadefx_over()) {
            scenestack_pop();
            quest_abort();
            return;
        }
        fadefx_out(image_rgb(0,0,0), 1.0);
        return;
    }

    /* pause game */
    block_pause = block_pause || (level_timer < 1.0f);
    for(i=0; i<team_size; i++)
        block_pause = block_pause || player_is_dying(team[i]);

    if(wants_to_pause && !block_pause) {
        wants_to_pause = FALSE;
        sound_play(SFX_PAUSE);
        scenestack_push(storyboard_get_scene(SCENE_PAUSE), NULL);
        return;
    }

    /* open level editor */
    if(editor_want_to_activate()) {
        if(readonly) {
            video_showmessage("No way!");
            sound_play(SFX_DENY);
        }
        else {
            editor_enable();
            return;
        }
    }

    /* got dying player? */
    for(i = 0; i < team_size; i++) {
        if(player_is_dying(team[i]))
            got_dying_player = TRUE;
    }

    /* -------------------------------------- */
    /* updating the entities */
    /* -------------------------------------- */

    /* getting the major entities */
    entitymanager_set_active_region(
        (int)cam.x - VIDEO_SCREEN_W/2 - (DEFAULT_MARGIN*3)/2,
        (int)cam.y - VIDEO_SCREEN_H/2 - (DEFAULT_MARGIN*3)/2,
        VIDEO_SCREEN_W + (DEFAULT_MARGIN*3),
        VIDEO_SCREEN_H + (DEFAULT_MARGIN*3)
    );

    major_enemies = entitymanager_retrieve_active_objects();
    major_items = entitymanager_retrieve_active_items();

    entitymanager_set_active_region(
        (int)cam.x - VIDEO_SCREEN_W/2 - DEFAULT_MARGIN,
        (int)cam.y - VIDEO_SCREEN_H/2 - DEFAULT_MARGIN,
        VIDEO_SCREEN_W + 2*DEFAULT_MARGIN,
        VIDEO_SCREEN_H + 2*DEFAULT_MARGIN
    );

    major_bricks = entitymanager_retrieve_active_bricks();

    /* update background */
    background_update(backgroundtheme);

    /* update items */
    for(inode = major_items; inode != NULL; inode = inode->next) {
        float x = inode->data->actor->position.x;
        float y = inode->data->actor->position.y;
        float w = image_width(actor_image(inode->data->actor));
        float h = image_height(actor_image(inode->data->actor));
        int inside_playarea = inside_screen(x, y, w, h, DEFAULT_MARGIN);
        int always_active = inode->data->always_active;

        if(inside_playarea || always_active) {
            /* update this item */
            item_update(inode->data, team, team_size, major_bricks, major_items, major_enemies);
        }
        else if(!inside_playarea) {
            /* this item is outside the screen... (and it's not always active) */
            if(!inode->data->preserve)
                inode->data->state = IS_DEAD;
            else if(!inside_screen(inode->data->actor->spawn_point.x, inode->data->actor->spawn_point.y, w, h, DEFAULT_MARGIN))
                inode->data->actor->position = inode->data->actor->spawn_point;
        }
    }

    /* update objects */
    for(enode = major_enemies; enode != NULL; enode = enode->next) {
        float x = enode->data->actor->position.x;
        float y = enode->data->actor->position.y;
        float w = image_width(actor_image(enode->data->actor));
        float h = image_height(actor_image(enode->data->actor));
        int always_active = enode->data->always_active;
        int inside_playarea = inside_screen(x, y, w, h, DEFAULT_MARGIN);

        if(inside_playarea || always_active) {
            /* update this object */
            enemy_update(enode->data, team, team_size, major_bricks, major_items, major_enemies);
        }
        else if(!inside_playarea) {
            /* this object is outside the play area... (and it's not always active) */
            if(!enode->data->preserve)
                enode->data->state = ES_DEAD;
            else if(!inside_screen(enode->data->actor->spawn_point.x, enode->data->actor->spawn_point.y, w, h, DEFAULT_MARGIN))
                enode->data->actor->position = enode->data->actor->spawn_point;
        }
    }

    /* update players */
    for(i=0; i<team_size; i++) {
        float x = team[i]->actor->position.x;
        float y = team[i]->actor->position.y;
        float w = image_width(actor_image(team[i]->actor));
        float h = image_height(actor_image(team[i]->actor));
        float hy = team[i]->actor->hot_spot.y;

        /* somebody is hurt! show it to the user */
        if(team[i] != player) {
            if(player_is_getting_hit(team[i]) || player_is_dying(team[i]))
                level_change_player(team[i]);
        }

        /* updating... */
        if(entitymanager_get_number_of_bricks() > 0) {
            if(inside_screen(x, y, w, h, DEFAULT_MARGIN/4) || player_is_dying(team[i]) || team[i]->actor->position.y < 0) {
                if(!got_dying_player || player_is_dying(team[i]) || player_is_getting_hit(team[i]))
                    player_update(team[i], team, team_size, major_bricks, major_items, major_enemies);
            }

            /* pitfall */
            if(team[i]->actor->position.y > level_height-(h-hy)) {
                if(inside_screen(x,y,w,h,DEFAULT_MARGIN/4))
                    player_kill(team[i]);
            }
        }
    }

    /* someone is dying */
    if(got_dying_player) {
        /* fade out the music */
        music_set_volume(music_get_volume() - 0.5*dt);

        /* restart the level */
        if(((dead_player_timeout += dt) >= 2.5f)) {
            if(player_get_lives() > 1) {
                /* restart the level! */
                if(fadefx_over()) {
                    player_set_lives(player_get_lives()-1);
                    restart(TRUE);
                    return;
                }
                fadefx_out(image_rgb(0,0,0), 1.0);
            }
            else {
                /* game over */
                scenestack_pop();
                scenestack_push(storyboard_get_scene(SCENE_GAMEOVER), NULL);
                return;
            }
        }
    }

    /* some objects are attached to the player... */
    for(enode = major_enemies; enode != NULL; enode = enode->next) {
        float x = enode->data->actor->position.x;
        float y = enode->data->actor->position.y;
        float w = image_width(actor_image(enode->data->actor));
        float h = image_height(actor_image(enode->data->actor));
        int always_active = enode->data->always_active;
        int inside_playarea = inside_screen(x, y, w, h, DEFAULT_MARGIN);

        if(inside_playarea || always_active) {
            if(enode->data->attached_to_player) {
                enode->data->actor->position = enemy_get_observed_player(enode->data)->actor->position;
                enode->data->actor->position = v2d_add(enode->data->actor->position, enode->data->attached_to_player_offset);
                enode->data->attached_to_player = FALSE;
            }
        }
    }

    /* update bricks */
    for(bnode = major_bricks; bnode != NULL; bnode = bnode->next) {
        /* update this brick */
        brick_update(bnode->data, team, team_size, major_bricks, major_items, major_enemies);
    }

    /* update camera */
    if(level_cleared)
        camera_move_to(v2d_add(camera_focus->position, v2d_new(0, -90)), 0.17);
    else if(!got_dying_player)
        camera_move_to(camera_focus->position, 0.0f); /* the camera will be locked on its focus (usually, the player) */
    camera_update();

    /* level timer */
    if(!got_dying_player && !level_cleared)
        level_timer += timer_get_delta();

    /* update scripts */
    update_ssobjects();

    /* update particles */
    particle_update_all(major_bricks);

    /* update dialog box */
    update_dialogregions();
    update_dlgbox();

    /* "ungetting" major entities */
    entitymanager_release_retrieved_brick_list(major_bricks);
    entitymanager_release_retrieved_item_list(major_items);
    entitymanager_release_retrieved_object_list(major_enemies);
}



/*
 * level_render()
 * Rendering function
 */
void level_render()
{
    brick_list_t *major_bricks;
    item_list_t *major_items;
    enemy_list_t *major_enemies;

    /* very important, if we restart the level */
    if(level_timer < 0.05f)
        return;

    /* quit... */
    if(quit_level) {
        image_blit(quit_level_img, video_get_backbuffer(), 0, 0, 0, 0, image_width(quit_level_img), image_height(quit_level_img));
        return;
    }

    /* render the level editor? */
    if(editor_is_enabled()) {
        editor_render();
        return;
    }

    /* --------------------------- */

    major_bricks = entitymanager_retrieve_active_bricks();
    major_items = entitymanager_retrieve_active_items();
    major_enemies = entitymanager_retrieve_active_objects();

    /* render level */
    render_level(major_bricks, major_items, major_enemies);

    /* render the built-in HUD */
    render_hud(major_enemies);

    entitymanager_release_retrieved_brick_list(major_bricks);
    entitymanager_release_retrieved_item_list(major_items);
    entitymanager_release_retrieved_object_list(major_enemies);
}



/*
 * level_release()
 * Releases the scene
 */
void level_release()
{
    logfile_message("level_release()");

    image_destroy(quit_level_img);
    particle_release();
    level_unload();
    camera_release();
    editor_release();
    prefs_save(modmanager_prefs());

    font_destroy(dlgbox_title);
    font_destroy(dlgbox_message);
    actor_destroy(dlgbox);

    logfile_message("level_release() ok");
}


/*
 * level_create_particle()
 * Creates a new particle.
 * Warning: image will be free'd internally.
 */
void level_create_particle(image_t *image, v2d_t position, v2d_t speed, int destroy_on_brick)
{
    if(editor_is_enabled()) {
        /* no, you can't create a new particle! */
        image_destroy(image);
    }
    else
        particle_add(image, position, speed, destroy_on_brick);
}

/*
 * level_file()
 * Returns the relative path of the level file
 */
const char* level_file()
{
    return file;
}

/*
 * level_name()
 * Returns the level name
 */
const char* level_name()
{
    return name;
}

/*
 * level_version()
 * Returns the version of the level
 */
const char* level_version()
{
    return version;
}

/*
 * level_author()
 * Returns the author of the level
 */
const char* level_author()
{
    return author;
}

/*
 * level_license()
 * Returns the level license
 */
const char* level_license()
{
    return license;
}

/*
 * level_act()
 * Returns the current act number
 */
int level_act()
{
    return act;
}

/*
 * level_player()
 * Returns the current player
 */
player_t* level_player()
{
    return player;
}

/*
 * level_persist()
 * Persists (saves) the current level
 * Returns TRUE on success
 */
int level_persist()
{
    return level_save(file);
}

/*
 * level_change()
 * Changes the level.
 * Make sure that 'level' is the current scene.
 */
void level_change(const char* path_to_lev_file)
{
    str_cpy(file, path_to_lev_file, sizeof(file));
    must_load_another_level = TRUE;
    logfile_message("Changing level to '%s'...", path_to_lev_file);
}


/*
 * level_change_player()
 * Changes the current player (character switching)
 */
void level_change_player(player_t *new_player)
{
    int i, player_id = -1;

    for(i=0; i<team_size && player_id < 0; i++)
        player_id = (team[i] == new_player) ? i : player_id;
    
    if(player_id >= 0) {
        player = team[player_id];
        level_set_camera_focus(player->actor);
        reconfigure_players_input_devices();
    }
}

/*
 * level_get_player_by_name()
 * Get a player by its name
 * Returns NULL if there is no such player
 */
player_t* level_get_player_by_name(const char* name)
{
    /* simple search */
    for(int i = 0; i < team_size; i++) {
        if(strcmp(team[i]->name, name) == 0)
            return team[i];
    }

    /* not found */
    return NULL;
}

/*
 * level_get_player_by_id()
 * Get a player by its ID (index on team[] array)
 * Returns NULL if there is no such player
 */
player_t* level_get_player_by_id(int id)
{
    if(id >= 0 && id < team_size)
        return team[id];
    else
        return NULL;
}

/*
 * level_create_brick()
 * Creates and adds a brick to the level. This function
 * returns a pointer to the created brick.
 */
brick_t* level_create_brick(int id, v2d_t position, bricklayer_t layer, brickflip_t flip)
{
    brick_t *brick = brick_create(id, position, layer, flip);
    entitymanager_store_brick(brick);
    return brick;
}



/*
 * level_create_item()
 * Creates and adds an item to the level. Returns the
 * created item.
 */
item_t* level_create_item(int id, v2d_t position)
{
    item_t *item = item_create(id);
    item->actor->spawn_point = position;
    item->actor->position = position;
    entitymanager_store_item(item);
    return item;
}



/*
 * level_create_enemy()
 * Creates and adds an enemy to the level. Returns the
 * created enemy.
 */
enemy_t* level_create_enemy(const char *name, v2d_t position)
{
    enemy_t *object = enemy_create(name);
    object->actor->spawn_point = position;
    object->actor->position = position;
    entitymanager_store_object(object);
    return object;
}


/*
 * level_create_ssobject()
 * Creates a SurgeScript object and adds it to the level.
 * Returns the new object, or NULL if the object couldn't be found
 */
surgescript_object_t* level_create_ssobject(const char* object_name, v2d_t position)
{
    if(ssobject_exists(object_name)) {
        surgescript_vm_t* vm = surgescript_vm();
        int spawned_in_the_editor =
            /* note: objects not created with this function (e.g., via scripting)
               will not have this flag set to true */
            surgescript_tagsystem_has_tag(surgescript_vm_tagsystem(vm), object_name, "entity") &&
            !is_startup_object(object_name)
        ;
        return spawn_ssobject(object_name, position, spawned_in_the_editor);
    }
    else
        return NULL;
}


/*
 * level_gravity()
 * Returns the gravity of the level
 */
float level_gravity()
{
    return gravity;
}


/*
 * level_add_to_score()
 * Adds a value to the player's score.
 * It also creates a flying text that
 * shows that score.
 */
void level_add_to_score(int score)
{
    item_t *flyingtext;
    char buf[80];
    int h = image_height(actor_image(player->actor));

    player_set_score(player_get_score() + score);

    sprintf(buf, "%d", score);
    flyingtext = level_create_item(IT_FLYINGTEXT, v2d_add(player->actor->position, v2d_new(0,-h/2)));
    flyingtext_set_text(flyingtext, buf);
}


/*
 * level_create_animal()
 * Creates a random animal
 */
item_t* level_create_animal(v2d_t position)
{
    item_t *animal = level_create_item(IT_ANIMAL, position);
    return animal;
}



/*
 * level_set_camera_focus()
 * Sets a new focus to the camera
 */
void level_set_camera_focus(actor_t *act)
{
    camera_focus = act;
}


/*
 * level_get_camera_focus()
 * Gets the actor being focused by the camera
 */
actor_t* level_get_camera_focus()
{
    return camera_focus;
}


/*
 * level_editmode()
 * Is the level editor activated?
 */
int level_editmode()
{
    return editor_is_enabled();
}


/*
 * level_size()
 * Returns the size of the level
 */
v2d_t level_size()
{
    return v2d_new(level_width, level_height);
}


/*
 * level_override_music()
 * Stops the music while the given sample is playing.
 * After it gets finished, the music gets played again.
 */
void level_override_music(sound_t *sample)
{
    music_stop();
    override_music = sample;
    sound_play(override_music);
}

/*
 * level_music()
 * The music of the level (may be NULL)
 */
music_t* level_music()
{
    return music;
}



/*
 * level_set_spawn_point()
 * Defines a new spawn point
 */
void level_set_spawn_point(v2d_t newpos)
{
    spawn_point = newpos;
}


/*
 * level_clear()
 * Call this when the player clears this level
 * If end_sign is NULL, the camera will be focused on the active player
 */
void level_clear(actor_t *end_sign)
{
    int i;

    if(level_cleared)
        return;

    /* ignore input and focus the camera on the end sign */
    for(i=0; i<team_size; i++)
        input_ignore(team[i]->actor->input);

    /* block music */
    block_music = TRUE;

    /* set the focus */
    if(end_sign != NULL)
        level_set_camera_focus(end_sign);
    else
        level_set_camera_focus(player->actor);

    /* success! */
    level_hide_dialogbox();
    level_cleared = TRUE;
}


/*
 * level_call_dialogbox()
 * Calls a dialog box
 */
void level_call_dialogbox(const char *title, const char *message)
{
    if(dlgbox_active && strcmp(font_get_text(dlgbox_title), title) == 0 && strcmp(font_get_text(dlgbox_message), message) == 0)
        return;

    dlgbox_active = TRUE;
    dlgbox_starttime = timer_get_ticks();
    font_set_text(dlgbox_title, "%s", title);
    font_set_text(dlgbox_message, "%s", message);
    font_set_width(dlgbox_message, 260);
}



/*
 * level_hide_dialogbox()
 * Hides the current dialog box (if any)
 */
void level_hide_dialogbox()
{
    dlgbox_active = FALSE;
}


/*
 * level_inside_screen()
 * Returns TRUE if a given region is
 * inside the screen position (camera-related)
 */
int level_inside_screen(int x, int y, int w, int h)
{
    return inside_screen(x,y,w,h,DEFAULT_MARGIN);
}


/*
 * level_has_been_cleared()
 * Has this level been cleared?
 */
int level_has_been_cleared()
{
    return level_cleared;
}


/*
 * level_jump_to_next_stage()
 * Jumps to the next stage in the quest
 */
void level_jump_to_next_stage()
{
    jump_to_next_stage = TRUE;
}


/*
 * level_ask_to_leave()
 * Asks permission for the user to leave the level
 */
void level_ask_to_leave()
{
    wants_to_leave = TRUE;
}

/*
 * level_pause()
 * Pauses the level
 */
void level_pause()
{
    wants_to_pause = TRUE;
}

/*
 * level_restart()
 * Restarts the current level
 */
void level_restart()
{
    must_restart_this_level = TRUE; /* schedules restart */
}

/*
 * level_waterlevel()
 * Returns the level of the water (the y-coordinate where the water begins)
 */
int level_waterlevel()
{
    return waterlevel;
}

/*
 * level_set_waterlevel()
 * Sets a new waterlevel
 */
void level_set_waterlevel(int ycoord)
{
    waterlevel = ycoord;
}

/*
 * level_watercolor()
 * Returns the color of the water
 */
uint32 level_watercolor()
{
    return watercolor;
}

/*
 * level_set_watercolor()
 * Sets a new watercolor
 */
void level_set_watercolor(uint32 color)
{
    watercolor = color;
}

/*
 * level_push_quest()
 * Pops this level, pushes a new quest.
 * So, you'll have at least two quests on the scene stack,
 * stored in consecutive positions.
 */
void level_push_quest(const char* path_to_qst_file)
{
    /* schedules a quest push */
    must_push_a_quest = TRUE;
    str_cpy(quest_to_be_pushed, path_to_qst_file, sizeof(quest_to_be_pushed));
}

/*
 * level_abort()
 * Pops this level, and also pops the current quest (if any)
 */
void level_abort()
{
    /* schedules a quest pop */
    quest_abort();
    level_jump_to_next_stage();
}










/* private functions */


/* renders the entities of the level: bricks,
 * enemies, items, players, etc. */
void render_level(brick_list_t *major_bricks, item_list_t *major_items, enemy_list_t *major_enemies)
{
    brick_list_t *bnode;
    item_list_t *inode;
    enemy_list_t *enode;

    /* starting up the render queue... */
    renderqueue_begin( camera_get_position() );

        /* render background */
        if(!editor_is_enabled())
            renderqueue_enqueue_background(backgroundtheme);

        /* render bricks */
        for(bnode=major_bricks; bnode; bnode=bnode->next)
            renderqueue_enqueue_brick(bnode->data);

        /* render items */
        for(inode=major_items; inode; inode=inode->next)
            renderqueue_enqueue_item(inode->data);

        /* render legacy objects */
        for(enode=major_enemies; enode; enode=enode->next)
            renderqueue_enqueue_object(enode->data);

        /* render surgescript objects */
        render_ssobjects();

        /* render particles */
        renderqueue_enqueue_particles();

        /* render the players */
        render_players();

        /* render the water */
        if(!editor_is_enabled())
            renderqueue_enqueue_water();

        /* render foreground */
        if(!editor_is_enabled())
            renderqueue_enqueue_foreground(backgroundtheme);

    /* okay, enough! let's render */
    renderqueue_end();
}

/* true if a given region is inside the screen position */
int inside_screen(int x, int y, int w, int h, int margin)
{
    v2d_t cam = level_editmode() ? editor_camera : camera_get_position();
    float a[4] = { x, y, x+w, y+h };
    float b[4] = {
        cam.x-VIDEO_SCREEN_W/2 - margin,
        cam.y-VIDEO_SCREEN_H/2 - margin,
        cam.x+VIDEO_SCREEN_W/2 + margin,
        cam.y+VIDEO_SCREEN_H/2 + margin
    };
    return bounding_box(a,b);
}

/* calculates the size of the
 * current level */
void update_level_size()
{
    int max_x, max_y;
    v2d_t bottomright;
    brick_list_t *p, *brick_list;

    max_x = max_y = -INFINITY;

    brick_list = entitymanager_retrieve_all_bricks();
    for(p=brick_list; p; p=p->next) {
        if(brick_type(p->data) != BRK_PASSABLE) {
            bottomright = v2d_add(brick_spawnpoint(p->data), brick_size(p->data));
            max_x = max(max_x, (int)bottomright.x);
            max_y = max(max_y, (int)bottomright.y);
        }
    }
    brick_list = entitymanager_release_retrieved_brick_list(brick_list);

    level_width = max(max_x, VIDEO_SCREEN_W);
    level_height = max(max_y, VIDEO_SCREEN_H);
}

/* restarts the level preserving
 * the current spawn point */
void restart(int preserve_current_spawnpoint)
{
    v2d_t sp = spawn_point;

    level_release();
    level_init((void*)file);

    if(preserve_current_spawnpoint) {
        spawn_point = sp;
        spawn_players();
    }
}


/* reconfigures the input devices of the
 * players after switching characters */
void reconfigure_players_input_devices()
{
    int i;

    for(i=0; i<team_size; i++) {
        if(NULL == team[i]->actor->input)
            team[i]->actor->input = input_create_user(NULL);

        if(team[i] == player) {
            input_restore(team[i]->actor->input);
            input_simulate_button_down(team[i]->actor->input, IB_FIRE2); /* bugfix (character switching) */
        }
        else
            input_ignore(team[i]->actor->input);
    }
}


/* renders the players */
void render_players()
{
    for(int i=team_size-1; i>=0; i--) {
        if(team[i] != player)
            renderqueue_enqueue_player(team[i]);
    }
    renderqueue_enqueue_player(player);
}


/* updates the music */
void update_music()
{
    if(override_music && !sound_is_playing(override_music))
        override_music = NULL;

    if(music != NULL && !block_music) {
        if(!override_music && !music_is_playing()) {
            if(music_current() == NULL || (music_current() == music && !music_is_paused()))
                music_play(music, INFINITY);
        }
    }
}


/* puts the players at the spawn point */
void spawn_players()
{
    int i, j;

    for(i = 0; i < team_size; i++) {
        j = ((int)spawn_point.x <= level_width/2) ? (team_size-1)-i : i;
        team[i]->actor->mirror = ((int)spawn_point.x <= level_width/2) ? IF_NONE : IF_HFLIP;
        team[i]->actor->spawn_point.x = team[i]->actor->position.x = spawn_point.x + 15 * j;
        team[i]->actor->spawn_point.y = team[i]->actor->position.y = spawn_point.y;
    }
}


/* renders the built-in hud */
void render_hud(enemy_list_t *major_enemies)
{
    v2d_t fixedcam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    /* powerups */
    if(!level_cleared)
        render_powerups();

    /* dialog box */
    render_dlgbox(fixedcam);
}


/* renders the dialog box */
void render_dlgbox(v2d_t camera_position)
{
    actor_render(dlgbox, camera_position);
    font_render(dlgbox_title, camera_position);
    font_render(dlgbox_message, camera_position);
}

/* updates the dialog box */
void update_dlgbox()
{
    float speed = VIDEO_SCREEN_H / 2; /* y speed */
    float dt = timer_get_delta();
    uint32 t = timer_get_ticks();

    if(dlgbox_active) {
        if(t >= dlgbox_starttime + DLGBOX_MAXTIME) {
            dlgbox_active = FALSE;
            return;
        }
        dlgbox->position.x = (VIDEO_SCREEN_W - image_width(actor_image(dlgbox)))/2;
        dlgbox->position.y = max(dlgbox->position.y - speed*dt, VIDEO_SCREEN_H - image_height(actor_image(dlgbox))*1.3f);

    }
    else {
        dlgbox->position.y = min(dlgbox->position.y + speed*dt, VIDEO_SCREEN_H);
    }

    font_set_position(dlgbox_title, v2d_add(dlgbox->position, v2d_new(7, 8)));
    font_set_position(dlgbox_message, v2d_add(dlgbox->position, v2d_new(7, 20)));
}








/* camera facade */
void level_lock_camera(int x1, int y1, int x2, int y2) /* the camera can show any point in this rectangle */
{
    camera_lock(x1+VIDEO_SCREEN_W/2, y1+VIDEO_SCREEN_H/2, x2-VIDEO_SCREEN_W/2, y2-VIDEO_SCREEN_H/2);
}

void level_unlock_camera()
{
    camera_unlock();
}

int level_is_camera_locked()
{
    return camera_is_locked();
}




/* dialog regions */


/* update_dialogregions(): updates all the dialog regions.
 * This checks if the player enters one region, and if he/she does,
 * this function shows the corresponding dialog box */
void update_dialogregions()
{
    int i;
    float a[4], b[4];

    if(level_timer < 2.0)
        return;

    a[0] = player->actor->position.x;
    a[1] = player->actor->position.y;
    a[2] = a[0] + image_width(actor_image(player->actor));
    a[3] = a[1] + image_height(actor_image(player->actor));

    for(i=0; i<dialogregion_size; i++) {
        if(dialogregion[i].disabled)
            continue;

        b[0] = dialogregion[i].rect_x;
        b[1] = dialogregion[i].rect_y;
        b[2] = b[0]+dialogregion[i].rect_w;
        b[3] = b[1]+dialogregion[i].rect_h;

        if(bounding_box(a, b)) {
            dialogregion[i].disabled = TRUE;
            level_call_dialogbox(dialogregion[i].title, dialogregion[i].message);
            break;
        }
    }
}





/* startup objects */

/* initializes the startup object list */
void init_startup_object_list()
{
    startupobject_list = NULL;
}

/* releases the startup object list */
void release_startup_object_list()
{
    startupobject_list_t *me, *next;

    for(me=startupobject_list; me; me=next) {
        next = me->next;
        free(me->object_name);
        free(me);
    }
}


/* adds a new object to the startup object list */
/* (actually it inserts the new object in the first position of the linked list) */
void add_to_startup_object_list(const char *object_name)
{
    startupobject_list_t *first_node = mallocx(sizeof *first_node);
    first_node->object_name = str_dup(object_name);
    first_node->next = startupobject_list;
    startupobject_list = first_node;
}

/* spawns the startup objects */
void spawn_startup_objects()
{
    startupobject_list_t *me;

    if(startupobject_list == NULL)
        add_to_startup_object_list(DEFAULT_STARTUP_OBJECT);

    for(me=startupobject_list; me; me=me->next) {
        /* try to create an object using the new API.
           if failure, use the old API. */
        if(!level_create_ssobject(me->object_name, v2d_new(0, 0))) {
            enemy_t* e = level_create_enemy(me->object_name, v2d_new(0, 0));
            e->created_from_editor = FALSE;
        }
    }
}

/* check if object_name is in the startup object list */
int is_startup_object(const char* object_name)
{
    for(startupobject_list_t* me = startupobject_list; me != NULL; me = me->next) {
        if(str_icmp(object_name, me->object_name) == 0)
            return TRUE;
    }

    if(str_icmp(object_name, DEFAULT_STARTUP_OBJECT) == 0)
        return TRUE;

    return FALSE;
}


/* misc */

/* render powerups */
void render_powerups()
{
    image_t *icon[MAX_POWERUPS]; /* icons */
    int visible[MAX_POWERUPS]; /* is icon[i] visible? */
    int i, c = 0; /* c is the icon count */
    float t = timer_get_ticks() * 0.001;

    for(i=0; i<MAX_POWERUPS; i++)
        visible[i] = TRUE;

    if(player) {
        if(player->got_glasses)
            icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 6) , 0 );

        switch (player_shield_type(player))
        {
            case SH_SHIELD:
                icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 7) , 0 );
                break;
            case SH_FIRESHIELD:
                icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 11) , 0 );
                break;
            case SH_THUNDERSHIELD:
                icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 12) , 0 );
                break;
            case SH_WATERSHIELD:
                icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 13) , 0 );
                break;
            case SH_ACIDSHIELD:
                icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 14) , 0 );
                break;
            case SH_WINDSHIELD:
                icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 15) , 0 );
                break;
            case SH_NONE:
                break;
        }

        if(player_is_invincible(player)) {
            icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 4) , 0 );
            if(player->invtimer >= PLAYER_MAX_INVINCIBILITY*0.75) { /* it blinks */
                /* we want something that blinks faster as player->invtimer tends to PLAYER_MAX_INVINCIBLITY */
                float x = ((PLAYER_MAX_INVINCIBILITY-player->invtimer)/(PLAYER_MAX_INVINCIBILITY*0.25)); /* 1 = x --> 0 */
                visible[c-1] = sin( (0.5*PI*t) / (x+0.1) ) >= 0;
            }
        }

        if(player_is_ultrafast(player)) {
            icon[c++] = sprite_get_image( sprite_get_animation("SD_ICON", 5) , 0 );
            if(player->speedshoes_timer >= PLAYER_MAX_SPEEDSHOES*0.75) { /* it blinks */
                /* we want something that blinks faster as player->speedshoes_timer tends to PLAYER_MAX_SPEEDSHOES */
                float x = ((PLAYER_MAX_SPEEDSHOES-player->speedshoes_timer)/(PLAYER_MAX_SPEEDSHOES*0.25)); /* 1 = x --> 0 */
                visible[c-1] = sin( (0.5*PI*t) / (x+0.1) ) >= 0;
            }
        }
    }

    for(i=0; i<c; i++) {
        if(visible[i])
            image_draw(icon[i], video_get_backbuffer(), VIDEO_SCREEN_W - image_width(icon[i]) * (i+1) - 5*i - 15, 10, IF_NONE);
    }
}


/* scripting */

/* update surgescript */
void update_ssobjects()
{
    surgescript_vm_t* vm = surgescript_vm();
    if(surgescript_vm_is_active(vm)) {
        v2d_t origin[TRANSFORM_MAX_DEPTH] = { [0] = v2d_new(0, 0) };
        surgescript_vm_update_ex(vm, origin, update_ssobject, late_update_ssobject);
    }
}

void update_ssobject(surgescript_object_t* object, void* param)
{
    /* initialization */
    int depth = surgescript_object_depth(object);
    if(depth < TRANSFORM_MAX_DEPTH) {
        /* get the transform origin */
        v2d_t origin = ((v2d_t*)param)[depth];

        /* are we dealing with a level entity? */
        if(surgescript_object_has_tag(object, "entity")) {
            surgescript_transform_t transform;
            surgescript_object_peek_transform(object, &transform);
            surgescript_transform_apply2d(&transform, &origin.x, &origin.y);

            /* check whether the entity should be updated, disposed or what */
            if(
                level_inside_screen(origin.x, origin.y, 1, 1) ||
                surgescript_object_has_tag(object, "awake") ||
                surgescript_object_has_tag(object, "detached")
            )
                surgescript_object_set_active(object, true);
            else if(!surgescript_object_has_tag(object, "disposable"))
                surgescript_object_set_active(object, false);
            else
                surgescript_object_kill(object);
        }

        /* set the transform origin for the next depth level */
        if(1 + depth < TRANSFORM_MAX_DEPTH)
            ((v2d_t*)param)[1 + depth] = origin;
    }
    else
        fatal_error("Scripting Error: TRANSFORM_MAX_DEPTH (%d) has been exceeded by \"%s\".", TRANSFORM_MAX_DEPTH, surgescript_object_name(object));
}

void late_update_ssobject(surgescript_object_t* object, void* param)
{
    //struct ssaux_t* ssaux = (struct ssaux_t*)param;
    //if(surgescript_object_transform_changed(object)
        //--ssaux->origin_top;
    if(!surgescript_object_is_active(object) && surgescript_object_has_tag(object, "entity"))
        surgescript_object_set_active(object, true); /* the object may reawaken in the future */
}

/* render objects */

void render_ssobjects()
{
    surgescript_vm_t* vm = surgescript_vm();
    if(surgescript_vm_is_active(vm)) {
        surgescript_object_t* root = surgescript_vm_root_object(vm);
        surgescript_object_traverse_tree_ex(root, surgescript_vm_programpool(vm), render_ssobject);
    }
}

bool render_ssobject(surgescript_object_t* object, void* param)
{
    surgescript_programpool_t* pool = (surgescript_programpool_t*)param;
    if(surgescript_object_is_active(object) && !surgescript_object_is_killed(object)) {
        if(editor_is_enabled()) {
            if(surgescript_object_has_tag(object, "entity") && !surgescript_object_has_tag(object, "private"))
                renderqueue_enqueue_ssobject_debug(object);
            return true;
        }
        else if(surgescript_programpool_exists(pool, surgescript_object_name(object), "render")) {
            renderqueue_enqueue_ssobject(object);
            return true;
        }
        else
            return true;
    }
    else
        return false;
}


/* SurgeScript object utilities */

/* does the specified object exist? */
bool ssobject_exists(const char* object_name)
{
    surgescript_vm_t* vm = surgescript_vm();
    surgescript_programpool_t* pool = surgescript_vm_programpool(vm);
    return surgescript_programpool_exists(pool, object_name, "state:main");
}

/* get the Level object (SurgeScript) */
surgescript_object_t* level_ssobject()
{
    if(cached_level_ssobject == NULL)
        cached_level_ssobject = scripting_util_surgeengine_component(surgescript_vm(), "Level");
    return cached_level_ssobject;
}

/* spawns a ssobject */
surgescript_object_t* spawn_ssobject(const char* object_name, v2d_t spawn_point, int spawned_in_the_editor)
{
    if(ssobject_exists(object_name)) {
        /* create the object by invoking Level.spawn */
        surgescript_vm_t* vm = surgescript_vm();
        surgescript_objectmanager_t* manager = surgescript_vm_objectmanager(vm);
        surgescript_transform_t* transform = NULL;
        surgescript_object_t* object = NULL;
        surgescript_var_t* tmp = surgescript_var_set_string(surgescript_var_create(), object_name);
        surgescript_var_t* ret = surgescript_var_create();
        const surgescript_var_t* param[] = { tmp };

        surgescript_object_call_function(level_ssobject(), "spawn", param, 1, ret);
        object = surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(ret));
        surgescript_var_destroy(ret);
        surgescript_var_destroy(tmp);

        /* set the spawn point */
        transform = surgescript_object_transform(object);
        surgescript_transform_translate2d(transform, spawn_point.x, spawn_point.y);

        /* save the editor-related data (entities only) */
        if(surgescript_object_has_tag(object, "entity")) {
            ssobj_extradata_t extradata = {
                .spawn_point = spawn_point,
                .spawned_in_the_editor = spawned_in_the_editor
            };
            set_ssobj_extradata(object, extradata);
        }

        /* done! */
        return object;
    }
    else {
        fatal_error("Can't spawn level object \"%s\": object does not exist.");
        return NULL;
    }
}

/* writes an object declaration to a file */
bool save_ssobject(surgescript_object_t* object, void* param)
{
    if(surgescript_object_is_killed(object))
        return false;

    if(is_ssobj_spawned_in_the_editor(object)) {
        FILE* fp = (FILE*)param;
        const char* object_name = surgescript_object_name(object);
        v2d_t spawn_point = get_ssobj_spawnpoint(object);
        fprintf(fp, "entity \"%s\" %d %d\n", str_addslashes(object_name), (int)spawn_point.x, (int)spawn_point.y);
    }

    return true;
}



/*
                                                                                                              
                                                                                                              
                                                                                                              
                                                                  MZZM                                        
                                                       MMMMMMMMMMMZOOZM MMMMMMMMM                             
             MM                                       M=...........MOOOM=.........M       MM          MM      
             MM                             MM          M7......77MOOOOOOM7.....77M             MM    MM      
             MM                             MM          M=......7MOOOOOMM=.....77M              MM    MM      
             MM    MM     MM     MMMMM    MMMMMM        M=......7MOOOMM==....77M          MM  MMMMMM  MM      
             MM    MM     MM    MMMMMMMM  MMMMMM        M=......7MOOOM==....77M           MM  MMMMMM  MM      
             MM    MM     MM   MM           MM         MM=......7MOM==....77MOOOM         MM    MM    MM      
             MM    MM     MM   MMMM         MM        MZM=......7MM==....77MOOOOOM        MM    MM    MM      
             MM    MM     MM      MMMMMM    MM      MZOOM=......7==....77MOOOOOOOOOM      MM    MM    MM      
     MM      MM    MM     MM           MM   MM       MZOM=......7....77MOOOOOOOOOOM       MM    MM            
      MM    MM      MM   MMM   MM     MMM   MM        MZM=..........MMMOOOOOOOOOOM        MM    MM            
        MMMM          MMM MM     MMMMMM      MMM        M=.........M..MOOOOOOOOM          MM     MMM  MM      
                                                        M=........7MMMOMMMOMMMMMM                             
                                                        M=......77MM..M...........M                           
                                                        M=....77MOM..MM..OM..MM..M                            
                                                        M=...77MOOM..MM..MM..MM..M                            
                                                         M.77M  MM...M..MM..MM...M                            
                                                          MMM    MMMMOMM  MM  MMM                             
                                                                   MM                                         
                                                                                                              
*/





/* Level Editor */



/*
 * editor_init()
 * Initializes the level editor data
 */
void editor_init()
{
    logfile_message("editor_init()");

    /* intializing... */
    editor_enabled = FALSE;
    editor_item_list_size = -1;
    while(editor_item_list[++editor_item_list_size] >= 0) { }
    editor_cursor_entity_type = EDT_BRICK;
    editor_cursor_entity_id = 0;
    /*editor_previous_video_resolution = video_get_resolution();
    editor_previous_video_smooth = video_is_smooth();*/
    editor_enemy_name = objects_get_list_of_names(&editor_enemy_name_length);
    editor_enemy_selected_category_id = 0;
    editor_enemy_category = objects_get_list_of_categories(&editor_enemy_category_length);

    /* creating objects */
    editor_cmd = editorcmd_create();
    editor_cursor_font = font_create("default");
    editor_properties_font = font_create("default");
    editor_help_font = font_create("default");

    /* grid */
    editor_grid_init();

    /* bricks */
    editor_brick_init();

    /* groups */
    editorgrp_init(grouptheme);

    /* SurgeScript entities */
    editor_ssobj_init();

    /* done */
    logfile_message("editor_init() ok");
}


/*
 * editor_release()
 * Releases the level editor data
 */
void editor_release()
{
    logfile_message("editor_release()");

    /* grid */
    editor_grid_release();

    /* groups */
    editorgrp_release();

    /* bricks */
    editor_brick_release();

    /* SurgeScript entities */
    editor_ssobj_release();

    /* destroying objects */
    editorcmd_destroy(editor_cmd);
    font_destroy(editor_properties_font);
    font_destroy(editor_cursor_font);
    font_destroy(editor_help_font);

    /* releasing... */
    editor_enabled = FALSE;
    logfile_message("editor_release() ok");
}


/*
 * editor_update()
 * Updates the level editor
 */
void editor_update()
{
    v2d_t topleft = v2d_subtract(editor_camera, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
    item_list_t *it, *major_items;
    brick_list_t *major_bricks;
    enemy_list_t *major_enemies;
    int pick_object, delete_object = FALSE;
    int selected_item;
    v2d_t pos;

    /* mouse cursor */
    editor_cursor = editorcmd_mousepos(editor_cmd);

    /* disable the level editor */
    if(editorcmd_is_triggered(editor_cmd, "quit")) {
        editor_disable();
        return;
    }

    /* save the level */
    if(editorcmd_is_triggered(editor_cmd, "save")) {
        editor_save();
        return;
    }

    /* reload level */
    if(editorcmd_is_triggered(editor_cmd, "reload")) {
        confirmboxdata_t cbd = { "Reload the level?", "YES", "NO" };
        scenestack_push(storyboard_get_scene(SCENE_CONFIRMBOX), (void*)&cbd);
        return;
    }

    if(1 == confirmbox_selected_option()) {
        v2d_t cam = editor_camera;
        editor_disable();
        editor_release();
        level_unload();
        level_load(file);
        editor_init();
        editor_enable();
        editor_camera = cam;
        level_cleared = FALSE;
        jump_to_next_stage = FALSE;
        spawn_players();
        return;
    }

    /* help */
    if(editorcmd_is_triggered(editor_cmd, "help")) {
        scenestack_push(storyboard_get_scene(SCENE_EDITORHELP), NULL);
        return;
    }

    /* open palette */
    if(editorcmd_is_triggered(editor_cmd, "open-brick-palette")) {
        if(editor_brick_count > 0) {
            editorpal_config_t config = {
                .type = EDITORPAL_BRICK,
                .brick = {
                    .id = (const int*)editor_brick,
                    .count = editor_brick_count
                }
            };
            scenestack_push(storyboard_get_scene(SCENE_EDITORPAL), &config);
            editor_cursor_entity_type = EDT_BRICK; editor_next_entity();
            return;
        }
        else
            sound_play(SFX_DENY);
    }
    else if(editorcmd_is_triggered(editor_cmd, "open-entity-palette")) {
        if(editor_ssobj_count > 0) {
            editorpal_config_t config = {
                .type = EDITORPAL_SSOBJ,
                .ssobj = {
                    .name = (const char**)editor_ssobj,
                    .count = editor_ssobj_count
                }
            };
            scenestack_push(storyboard_get_scene(SCENE_EDITORPAL), &config);
            editor_cursor_entity_type = EDT_SSOBJ; editor_next_entity();
            return;           
        }
        else
            sound_play(SFX_DENY);
    }

    if(-1 < (selected_item = editorpal_selected_item())) {
        if(editor_cursor_entity_type == EDT_BRICK) {
            editor_cursor_entity_id = editor_brick_id(selected_item);
            editor_flip = BRF_NOFLIP;
            /*editor_layer = BRL_DEFAULT;*/ /* is this desirable? */
        }
        else if(editor_cursor_entity_type == EDT_SSOBJ)
            editor_cursor_entity_id = selected_item;
    }

    /* ----------------------------------------- */

    /* getting major entities */
    major_enemies = entitymanager_retrieve_active_objects();
    major_items = entitymanager_retrieve_active_items();
    major_bricks = entitymanager_retrieve_active_bricks();

    /* update items */
    for(it=major_items; it!=NULL; it=it->next)
        item_update(it->data, team, team_size, major_bricks, major_items, major_enemies);

    /* change class / entity / object category */
    if(editorcmd_is_triggered(editor_cmd, "next-category")) {
        if(editor_cursor_entity_type == EDT_ENEMY) /* change category: legacy objects */
            editor_next_object_category();
    }
    else if(editorcmd_is_triggered(editor_cmd, "previous-category")) {
        if(editor_cursor_entity_type == EDT_ENEMY)
            editor_previous_object_category();
    }
    else if(editorcmd_is_triggered(editor_cmd, "next-class"))
        editor_next_class();
    else if(editorcmd_is_triggered(editor_cmd, "previous-class"))
        editor_previous_class();
    else if(editorcmd_is_triggered(editor_cmd, "next-item"))
        editor_next_entity();
    else if(editorcmd_is_triggered(editor_cmd, "previous-item"))
        editor_previous_entity();

    /* change brick layer */
    if(editorcmd_is_triggered(editor_cmd, "layer-next")) {
        if(editor_cursor_entity_type == EDT_BRICK)
            editor_layer = (editor_layer + 1) % 3;
        else
            sound_play(SFX_DENY);
    }
    else if(editorcmd_is_triggered(editor_cmd, "layer-previous")) {
        if(editor_cursor_entity_type == EDT_BRICK)
            editor_layer = (editor_layer + 2) % 3;
        else
            sound_play(SFX_DENY);
    }

    /* change brick flip mode */
    if(editorcmd_is_triggered(editor_cmd, "flip-next")) {
        if(editor_cursor_entity_type == EDT_BRICK) {
            int delta = (3 + editor_flip) / 2;
            editor_flip = (editor_flip + delta) & BRF_VHFLIP;
        }
        else
            sound_play(SFX_DENY);
    }
    else if(editorcmd_is_triggered(editor_cmd, "flip-previous")) {
        if(editor_cursor_entity_type == EDT_BRICK) {
            int delta = 2 + editor_flip + editor_flip / 2;
            editor_flip = (editor_flip + delta) & BRF_VHFLIP;
        }
        else
            sound_play(SFX_DENY);
    }

    /* new spawn point */
    if(editorcmd_is_triggered(editor_cmd, "change-spawnpoint")) {
        v2d_t nsp = editor_grid_snap(editor_cursor);
        editor_action_t eda = editor_action_spawnpoint_new(TRUE, nsp, spawn_point);
        editor_action_commit(eda);
        editor_action_register(eda);
    }

    /* put item */
    if(editorcmd_is_triggered(editor_cmd, "put-item")) {
        editor_action_t eda = editor_action_entity_new(TRUE, editor_cursor_entity_type, editor_cursor_entity_id, editor_grid_snap(editor_cursor));
        editor_action_commit(eda);
        editor_action_register(eda);
    }

    /* pick or delete item */
    pick_object = editorcmd_is_triggered(editor_cmd, "pick-item");
    delete_object = editorcmd_is_triggered(editor_cmd, "delete-item") || editor_is_eraser_enabled();
    if(pick_object || delete_object) {
        brick_list_t *itb;
        item_list_t *iti;
        enemy_list_t *ite;

        switch(editor_cursor_entity_type) {
            /* brick */
            case EDT_BRICK: {
                brick_t *candidate = NULL;

                for(itb=major_bricks;itb;itb=itb->next) {
                    v2d_t brk_topleft = brick_position(itb->data);
                    v2d_t brk_bottomright = v2d_add(brk_topleft, brick_size(itb->data));
                    float a[4] = { brk_topleft.x, brk_topleft.y, brk_bottomright.x, brk_bottomright.y };
                    float b[4] = { editor_cursor.x+topleft.x , editor_cursor.y+topleft.y , editor_cursor.x+topleft.x+1 , editor_cursor.y+topleft.y+1 };
                    if(bounding_box(a,b)) {
                        if(candidate == NULL || brick_zindex(itb->data) >= brick_zindex(candidate))
                            candidate = itb->data;
                    }
                }

                if(candidate != NULL) {
                    if(pick_object) {
                        editor_cursor_entity_id = brick_id(candidate);
                        editor_layer = brick_layer(candidate);
                        editor_flip = brick_flip(candidate);
                    }
                    else {
                        editor_action_t eda = editor_action_entity_new(FALSE, EDT_BRICK, brick_id(candidate), brick_position(candidate));
                        editor_action_commit(eda);
                        editor_action_register(eda);
                    }
                }

                break;
            }

            /* item */
            case EDT_ITEM: {
                item_t *candidate = NULL;

                for(iti=major_items;iti;iti=iti->next) {
                    float a[4] = {iti->data->actor->position.x-iti->data->actor->hot_spot.x, iti->data->actor->position.y-iti->data->actor->hot_spot.y, iti->data->actor->position.x-iti->data->actor->hot_spot.x + image_width(actor_image(iti->data->actor)), iti->data->actor->position.y-iti->data->actor->hot_spot.y + image_height(actor_image(iti->data->actor))};
                    float b[4] = { editor_cursor.x+topleft.x , editor_cursor.y+topleft.y , editor_cursor.x+topleft.x+1 , editor_cursor.y+topleft.y+1 };

                    if(bounding_box(a,b)) {
                        if(candidate == NULL || !iti->data->bring_to_back)
                            candidate = iti->data;
                    }
                }

                if(candidate != NULL) {
                    if(pick_object) {
                        int index = editor_item_list_get_index(candidate->type);
                        if(index >= 0) {
                            editor_cursor_itemid = index;
                            editor_cursor_entity_id = editor_item_list[index];
                        }
                    }
                    else {
                        editor_action_t eda = editor_action_entity_new(FALSE, EDT_ITEM, candidate->type, candidate->actor->position);
                        editor_action_commit(eda);
                        editor_action_register(eda);
                    }
                }

                break;
            }

            /* enemy */
            case EDT_ENEMY: {
                enemy_t *candidate = NULL;
                int candidate_key = 0;

                for(ite=major_enemies;ite;ite=ite->next) {
                    float a[4] = {ite->data->actor->position.x-ite->data->actor->hot_spot.x, ite->data->actor->position.y-ite->data->actor->hot_spot.y, ite->data->actor->position.x-ite->data->actor->hot_spot.x + image_width(actor_image(ite->data->actor)), ite->data->actor->position.y-ite->data->actor->hot_spot.y + image_height(actor_image(ite->data->actor))};
                    float b[4] = { editor_cursor.x+topleft.x , editor_cursor.y+topleft.y , editor_cursor.x+topleft.x+1 , editor_cursor.y+topleft.y+1 };
                    int mykey = editor_enemy_name2key(ite->data->name);
                    if(mykey >= 0 && bounding_box(a,b)) {
                        if(candidate == NULL || ite->data->zindex >= candidate->zindex) {
                            candidate = ite->data;
                            candidate_key = mykey;
                        }
                    }
                }

                if(candidate != NULL) {
                    if(pick_object) {
                        editor_cursor_entity_id = candidate_key;
                    }
                    else {
                        editor_action_t eda = editor_action_entity_new(FALSE, EDT_ENEMY, candidate_key, candidate->actor->position);
                        editor_action_commit(eda);
                        editor_action_register(eda);
                    }
                }

                break;
            }

            /* can't pick-up/delete a group */
            case EDT_GROUP:
                break;

            /* SurgeScript entity */
            case EDT_SSOBJ: {
                surgescript_vm_t* vm = surgescript_vm();
                surgescript_object_t* root = surgescript_vm_root_object(vm);
                surgescript_object_t* ssobject = NULL;
                surgescript_object_traverse_tree_ex(root, &ssobject, editor_pick_ssobj);
                if(ssobject != NULL) {
                    int ssobj_id = editor_ssobj_id(surgescript_object_name(ssobject));
                    if(!pick_object) {
                        editor_action_t eda = editor_action_entity_new(FALSE, EDT_SSOBJ, ssobj_id, scripting_util_world_position(ssobject));
                        editor_action_commit(eda);
                        editor_action_register(eda);
                    }
                    else
                        editor_cursor_entity_id = ssobj_id;
                }
                break;               
            }
        }
    }

    /* undo & redo */
    if(editorcmd_is_triggered(editor_cmd, "undo"))
        editor_action_undo();
    else if(editorcmd_is_triggered(editor_cmd, "redo"))
        editor_action_redo();

    /* background */
    editor_update_background();

    /* grid */
    editor_grid_update();

    /* scrolling */
    editor_scroll();

    /* cursor coordinates */
    font_set_text(editor_cursor_font, "%d,%d", (int)editor_grid_snap(editor_cursor).x, (int)editor_grid_snap(editor_cursor).y);
    pos.x = clip((int)editor_grid_snap(editor_cursor).x - (editor_camera.x - VIDEO_SCREEN_W/2), 10, VIDEO_SCREEN_W-font_get_textsize(editor_cursor_font).x-10);
    pos.y = clip((int)editor_grid_snap(editor_cursor).y - (editor_camera.y - VIDEO_SCREEN_H/2) - 2 * font_get_textsize(editor_cursor_font).y, 10, VIDEO_SCREEN_H-10);
    font_set_position(editor_cursor_font, pos);

    /* help label */
    font_set_text(editor_help_font, "<color=ff8060>F1</color>: help");
    font_set_position(editor_help_font, v2d_new(VIDEO_SCREEN_W - font_get_textsize(editor_help_font).x - 8, 8));
    font_set_visible(editor_help_font, video_get_window_size().x > 512);

    /* object properties */
    font_set_position(editor_properties_font, v2d_new(8, 8));
    font_set_text(
        editor_properties_font,
        "<color=ff8060>%s</color> <color=ffffff>%s</color>",
        editor_entity_class(editor_cursor_entity_type),
        editor_entity_info(editor_cursor_entity_type, editor_cursor_entity_id)
    );

    /* "ungetting" major entities */
    entitymanager_release_retrieved_brick_list(major_bricks);
    entitymanager_release_retrieved_item_list(major_items);
    entitymanager_release_retrieved_object_list(major_enemies);
}



/*
 * editor_render()
 * Renders the level editor
 */
void editor_render()
{
    brick_list_t *major_bricks;
    item_list_t *major_items;
    enemy_list_t *major_enemies;
    image_t *cursor;
    v2d_t topleft = v2d_subtract(editor_camera, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));

    major_bricks = entitymanager_retrieve_active_bricks();
    major_items = entitymanager_retrieve_active_items();
    major_enemies = entitymanager_retrieve_active_objects();




    /* background */
    editor_render_background();

    /* path of the movable platforms */
    editor_movable_platforms_path_render(major_bricks);

    /* render level */
    render_level(major_bricks, major_items, major_enemies);

    /* draw editor water line */
    editor_waterline_render((int)(waterlevel - topleft.y), image_rgb(255, 255, 255));

    /* top bar */
    image_rectfill(video_get_backbuffer(), 0, 0, VIDEO_SCREEN_W, 24, image_rgb(40, 44, 52));
    font_render(editor_properties_font, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
    font_render(editor_help_font, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));

    /* mouse cursor */
    if(!editor_is_eraser_enabled()) {
        /* drawing the object */
        editor_draw_object(editor_cursor_entity_type, editor_cursor_entity_id, v2d_subtract(editor_grid_snap(editor_cursor), topleft));

        /* drawing the cursor arrow */
        cursor = sprite_get_image(sprite_get_animation("SD_ARROW", 0), 0);
        if(editor_layer == BRL_DEFAULT || (editor_cursor_entity_type != EDT_BRICK && editor_cursor_entity_type != EDT_GROUP))
            image_draw(cursor, video_get_backbuffer(), (int)editor_cursor.x, (int)editor_cursor.y, IF_NONE);
        else
            image_draw_translit(cursor, video_get_backbuffer(), (int)editor_cursor.x, (int)editor_cursor.y, brick_util_layercolor(editor_layer), 0.5f, IF_NONE);

        /* cursor coordinates */
        font_render(editor_cursor_font, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
    }
    else {
        /* drawing an eraser */
        cursor = sprite_get_image(sprite_get_animation("SD_ERASER", 0), 0);
        image_draw(cursor, video_get_backbuffer(), (int)editor_cursor.x - image_width(cursor)/2, (int)editor_cursor.y - image_height(cursor)/2, IF_NONE);
    }





    entitymanager_release_retrieved_brick_list(major_bricks);
    entitymanager_release_retrieved_item_list(major_items);
    entitymanager_release_retrieved_object_list(major_enemies);
}



/*
 * editor_enable()
 * Enables the level editor
 */
void editor_enable()
{
    logfile_message("editor_enable()");

    /* activating the editor */
    editor_action_init();
    editor_enabled = TRUE;
    editor_camera.x = (int)camera_get_position().x;
    editor_camera.y = (int)camera_get_position().y;
    editor_cursor = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);
    video_showmessage("Welcome to the Level Editor!");

    /* changing the video resolution */
    editor_previous_video_resolution = video_get_resolution();
    editor_previous_video_smooth = video_is_smooth();
    video_changemode(VIDEORESOLUTION_EDT, FALSE, video_is_fullscreen());

    logfile_message("editor_enable() ok");
}


/*
 * editor_disable()
 * Disables the level editor
 */
void editor_disable()
{
    logfile_message("editor_disable()");

    /* disabling the level editor */
    update_level_size();
    editor_action_release();
    editor_enabled = FALSE;

    /* restoring the video resolution */
    video_changemode(editor_previous_video_resolution, editor_previous_video_smooth, video_is_fullscreen());

    logfile_message("editor_disable() ok");
}


/*
 * editor_is_enabled()
 * Is the level editor activated?
 */
int editor_is_enabled()
{
    return editor_enabled;
}

/*
 * editor_want_to_activate()
 * The level editor wants to be activated!
 */
int editor_want_to_activate()
{
    return editorcmd_is_triggered(editor_cmd, "enter");
}

/*
 * editor_update_background()
 * Updates the background of the level editor
 */
void editor_update_background()
{
    background_update(backgroundtheme);
}

/*
 * editor_render_background()
 * Renders the background of the level editor
 */
void editor_render_background()
{
    image_rectfill(video_get_backbuffer(), 0, 0, VIDEO_SCREEN_W, VIDEO_SCREEN_H, image_rgb(40, 44, 52));
    background_render_bg(backgroundtheme, editor_camera); /* FIXME? no render_fg */
}


/*
 * editor_movable_platforms_path_render()
 * Renders the path of the movable platforms
 */
void editor_movable_platforms_path_render(brick_list_t *major_bricks)
{
    brick_list_t *it;

    for(it=major_bricks; it; it=it->next)
        brick_render_path(it->data, editor_camera);
}



/*
 * editor_waterline_render()
 * Renders the line of the water
 */
void editor_waterline_render(int ycoord, uint32 color)
{
    int x, x0 = 19 - (timer_get_ticks() / 25) % 20;
    image_t *buf = video_get_backbuffer();

    for(x=x0-10; x<VIDEO_SCREEN_W; x+=20)
        image_line(buf, x, ycoord, x+10, ycoord, color);
}


/*
 * editor_save()
 * Saves the level
 */
void editor_save()
{
    if(level_save(file)) {
        sound_play(SFX_SAVE);
        video_showmessage("Level saved.");
    }
    else {
        sound_play(SFX_DENY);
        video_showmessage("Can't save the level. Please check the logs...");
    }
}


/*
 * editor_scroll()
 * Scrolling - "free-look mode"
 */
void editor_scroll()
{
    v2d_t camera_direction = v2d_new(0, 0);
    float camera_speed = 750.0f;
    float dt = timer_get_delta();

    /* input */
    if(editorcmd_is_triggered(editor_cmd, "UP"))
        camera_direction.y -= 5.0f;
    if(editorcmd_is_triggered(editor_cmd, "RIGHT"))
        camera_direction.x += 5.0f;
    if(editorcmd_is_triggered(editor_cmd, "DOWN"))
        camera_direction.y += 5.0f;
    if(editorcmd_is_triggered(editor_cmd, "LEFT"))
        camera_direction.x -= 5.0f;
    if(editorcmd_is_triggered(editor_cmd, "up"))
        camera_direction.y -= 1.0f;
    if(editorcmd_is_triggered(editor_cmd, "right"))
        camera_direction.x += 1.0f;
    if(editorcmd_is_triggered(editor_cmd, "down"))
        camera_direction.y += 1.0f;
    if(editorcmd_is_triggered(editor_cmd, "left"))
        camera_direction.x -= 1.0f;

    /* scroll */
    if(v2d_magnitude(camera_direction) > EPSILON)
        editor_camera = v2d_add(v2d_multiply(camera_direction, camera_speed * dt), editor_camera);

    /* the camera mustn't go off the bounds */
    editor_camera.x = (int)max(editor_camera.x, VIDEO_SCREEN_W/2);
    editor_camera.y = (int)max(editor_camera.y, VIDEO_SCREEN_H/2);
    camera_set_position(editor_camera);
}

/*
 * editor_is_eraser_enabled()
 * Is the eraser enabled?
 */
int editor_is_eraser_enabled()
{
    const float hold_time = 0.57f; /* the eraser is activated after this amount of seconds */
    static float timer = 0.0f;

    /* group mode? will erase bricks */
    if(editor_cursor_entity_type == EDT_GROUP) {
        if(editorcmd_is_triggered(editor_cmd, "erase-area")) {
            while(editor_cursor_entity_type != EDT_BRICK)
                editor_next_class();
        }
    }

    /* check if the eraser is enabled */
    if(editorcmd_is_triggered(editor_cmd, "erase-area")) {
        timer += timer_get_delta();
        return timer >= hold_time;
    }
    else {
        timer = 0.0f;
        return FALSE;
    }
}


/* private stuff (level editor) */



/* returns a string containing a text
 * that corresponds to the given editor class
 * object id */
const char *editor_entity_class(enum editor_entity_type objtype)
{
    switch(objtype) {
        case EDT_BRICK:
            return "brick";

        case EDT_GROUP:
            return "brick group";

        case EDT_SSOBJ:
            return "entity";

        case EDT_ITEM:
            return "legacy item";

        case EDT_ENEMY:
            return "legacy object";
    }

    return "unknown";
}


/* returns a string containing data
 * about a given object */
const char *editor_entity_info(enum editor_entity_type objtype, int objid)
{
    static char buf[128];
    *buf = 0;

    switch(objtype) {
        case EDT_BRICK: {
            if(brick_exists(objid)) {
                brick_t *x = brick_create(objid, v2d_new(0,0), BRL_DEFAULT, BRF_NOFLIP);
                sprintf(buf,
                    "%4d %10s %12s    %3dx%-3d    z=%.2f    %6s",
                    objid,
                    brick_util_behaviorname(brick_behavior(x)),
                    brick_util_typename(brick_type(x)),
                    (int)brick_size(x).x,
                    (int)brick_size(x).y,
                    brick_zindex(x),
                    brick_util_flipstr(editor_flip)
                );
                brick_destroy(x);
            }
            else
                strcpy(buf, "<missing>");
            break;
        }

        case EDT_ITEM: {
            sprintf(buf, "%2d", objid);
            break;
        }

        case EDT_ENEMY: {
            int len = 0;
            if(strcmp(editor_enemy_selected_category(), "*") != 0) {
                sprintf(buf, "[%s] ", editor_enemy_selected_category());
                len = strlen(buf);
            }
            str_cpy(buf + len, editor_enemy_key2name(editor_cursor_entity_id), sizeof(buf) - len);
            break;
        }

        case EDT_GROUP: {
            break;
        }

        case EDT_SSOBJ: {
            str_cpy(buf, editor_ssobj_name(objid), sizeof(buf));
            break;
        }
    }

    return buf;
}

/* returns the name of the selected category */
const char* editor_enemy_selected_category()
{
    return editor_enemy_category[editor_enemy_selected_category_id];
}

/* next object category */
void editor_next_object_category()
{
    editor_enemy_selected_category_id = (editor_enemy_selected_category_id + 1) % editor_enemy_category_length;

    /* drop the entity if it's not in the correct category */
    editor_next_entity();
    editor_previous_entity();
}

/* previous object category */
void editor_previous_object_category()
{
    editor_enemy_selected_category_id = ((editor_enemy_selected_category_id - 1) + editor_enemy_category_length) % editor_enemy_category_length;

    /* drop the entity if it's not in the correct category */
    editor_next_entity();
    editor_previous_entity();
}


/* jump to next class */
void editor_next_class()
{
    editor_cursor_entity_type =
    (editor_cursor_entity_type == EDT_BRICK) ? EDT_SSOBJ :
    (editor_cursor_entity_type == EDT_SSOBJ) ? EDT_ITEM  :
    (editor_cursor_entity_type == EDT_ITEM)  ? EDT_ENEMY :
    (editor_cursor_entity_type == EDT_ENEMY) ? EDT_GROUP :
    (editor_cursor_entity_type == EDT_GROUP) ? EDT_BRICK :
    editor_cursor_entity_type;

    editor_cursor_entity_id = 0;
    editor_cursor_itemid = 0;

    if(editor_cursor_entity_type == EDT_GROUP && editorgrp_group_count() == 0)
        editor_next_class();

    if(editor_cursor_entity_type == EDT_ENEMY && editor_enemy_name_length == 0)
        editor_next_class();

    if(editor_cursor_entity_type == EDT_SSOBJ && editor_ssobj_count == 0)
        editor_next_class();

    if(editor_cursor_entity_type == EDT_BRICK && !brick_exists(editor_cursor_entity_id))
        editor_next_entity(); /* it's guaranteed that we'll always have a brick (see brickset_load) */
}


/* jump to previous class */
void editor_previous_class()
{
    editor_cursor_entity_type =
    (editor_cursor_entity_type == EDT_BRICK) ? EDT_GROUP :
    (editor_cursor_entity_type == EDT_SSOBJ) ? EDT_BRICK :
    (editor_cursor_entity_type == EDT_ITEM)  ? EDT_SSOBJ :
    (editor_cursor_entity_type == EDT_ENEMY) ? EDT_ITEM  :
    (editor_cursor_entity_type == EDT_GROUP) ? EDT_ENEMY :
    editor_cursor_entity_type;

    editor_cursor_entity_id = 0;
    editor_cursor_itemid = 0;

    if(editor_cursor_entity_type == EDT_GROUP && editorgrp_group_count() == 0)
        editor_previous_class();

    if(editor_cursor_entity_type == EDT_ENEMY && editor_enemy_name_length == 0)
        editor_previous_class();

    if(editor_cursor_entity_type == EDT_SSOBJ && editor_ssobj_count == 0)
        editor_previous_class();

    if(editor_cursor_entity_type == EDT_BRICK && !brick_exists(editor_cursor_entity_id))
        editor_previous_entity();
}


/* select the next object */
void editor_next_entity()
{
    switch(editor_cursor_entity_type) {
        /* brick */
        case EDT_BRICK: {
            editor_cursor_entity_id = editor_brick_id(
                (editor_brick_index(editor_cursor_entity_id) + 1) % editor_brick_count
            );
            break;
        }

        /* group */
        case EDT_GROUP: {
            int size = editorgrp_group_count();
            editor_cursor_entity_id = (editor_cursor_entity_id + 1) % size;
            break;
        }

        /* SurgeScript entity */
        case EDT_SSOBJ: {
            int size = editor_ssobj_count;
            editor_cursor_entity_id = (editor_cursor_entity_id + 1) % size;
            break;
        }

        /* item */
        case EDT_ITEM: {
            int size = editor_item_list_size;
            editor_cursor_itemid = (editor_cursor_itemid + 1) % size;
            editor_cursor_entity_id = editor_item_list[editor_cursor_itemid];
            break;
        }

        /* enemy */
        case EDT_ENEMY: {
            enemy_t *enemy;
            int size = editor_enemy_name_length;
            editor_cursor_entity_id = (editor_cursor_entity_id + 1) % size;

            enemy = enemy_create(editor_enemy_key2name(editor_cursor_entity_id));
            if(!enemy_belongs_to_category(enemy, editor_enemy_selected_category())) {
                enemy_destroy(enemy);
                editor_next_entity(); /* invalid category? */
            }
            else
                enemy_destroy(enemy);

            break;
        }
    }
}


/* select the previous object */
void editor_previous_entity()
{
    switch(editor_cursor_entity_type) {
        /* brick */
        case EDT_BRICK: {
            editor_cursor_entity_id = editor_brick_id(
                ((editor_brick_index(editor_cursor_entity_id) - 1) + editor_brick_count) % editor_brick_count
            );
            break;
        }

        /* group */
        case EDT_GROUP: {
            int size = editorgrp_group_count();
            editor_cursor_entity_id = ((editor_cursor_entity_id - 1) + size) % size;
            break;
        }

        /* SurgeScript entity */
        case EDT_SSOBJ: {
            int size = editor_ssobj_count;
            editor_cursor_entity_id = ((editor_cursor_entity_id - 1) + size) % size;
            break;
        }

        /* item */
        case EDT_ITEM: {
            int size = editor_item_list_size;
            editor_cursor_itemid = ((editor_cursor_itemid - 1) + size) % size;
            editor_cursor_entity_id = editor_item_list[editor_cursor_itemid];
            break;
        }

        /* enemy */
        case EDT_ENEMY: {
            enemy_t *enemy;
            int size = editor_enemy_name_length;
            editor_cursor_entity_id = ((editor_cursor_entity_id - 1) + size) % size;

            enemy = enemy_create(editor_enemy_key2name(editor_cursor_entity_id));
            if(!enemy_belongs_to_category(enemy, editor_enemy_selected_category())) {
                enemy_destroy(enemy);
                editor_previous_entity(); /* invalid category? */
            }
            else
                enemy_destroy(enemy);

            break;
        }
    }
}


/* returns the index of item_id on
 * editor_item_list or -1 if the search fails */
int editor_item_list_get_index(int item_id)
{
    int i;

    for(i=0; i<editor_item_list_size; i++) {
        if(item_id == editor_item_list[i])
            return i;
    }

    return -1;
}


/*
 * editor_is_valid_item()
 * Is the given item valid to be used in
 * the level editor?
 */
int editor_is_valid_item(int item_id)
{
    return (editor_item_list_get_index(item_id) != -1);
}



/* draws the given object at [position] */
void editor_draw_object(enum editor_entity_type obj_type, int obj_id, v2d_t position)
{
    const image_t *cursor = NULL;
    v2d_t offset = v2d_new(0, 0);
    float alpha = 0.75f;
    int flags = IF_NONE;

    /* getting the image of the current object */
    switch(obj_type) {
        case EDT_BRICK: {
            if(brick_exists(obj_id)) {
                cursor = brick_image_preview(obj_id);
                flags = brick_image_flags(editor_flip);
            }
            break;
        }

        case EDT_ITEM: {
            item_t *item = item_create(obj_id);
            if(item != NULL) {
                cursor = actor_image(item->actor);
                offset = item->actor->hot_spot;
                item_destroy(item);
            }
            break;
        }

        case EDT_ENEMY: {
            enemy_t *enemy = enemy_create(editor_enemy_key2name(obj_id));
            if(enemy != NULL) {
                cursor = actor_image(enemy->actor);
                offset = enemy->actor->hot_spot;
                enemy_destroy(enemy);
            }
            break;
        }

        case EDT_GROUP: {
            editorgrp_entity_list_t *list, *it;
            list = editorgrp_get_group(obj_id);
            for(it=list; it; it=it->next) {
                enum editor_entity_type my_type = EDITORGRP_ENTITY_TO_EDT(it->entity.type);
                editor_draw_object(my_type, it->entity.id, v2d_add(position, it->entity.position));
            }
            break;
        }

        case EDT_SSOBJ: {
            const animation_t* anim = NULL;
            const char* object_name = editor_ssobj_name(obj_id);
            if(sprite_animation_exists(object_name, 0))
                anim = sprite_get_animation(object_name, 0);
            else
                anim = sprite_get_animation(NULL, 0);
            cursor = sprite_get_image(anim, 0);
            offset = anim->hot_spot;
            break;
        }
    }

    /* drawing the object */
    if(cursor != NULL)
        image_draw_trans(cursor, video_get_backbuffer(), (int)(position.x-offset.x), (int)(position.y-offset.y), alpha, flags);
}


/* level editor: enemy name list */
int editor_enemy_name2key(const char *name)
{
    int i;

    for(i=0; i<editor_enemy_name_length; i++) {
        if(strcmp(name, editor_enemy_name[i]) == 0)
            return i;
    }

    return -1; /* not found */
}

const char* editor_enemy_key2name(int key)
{
    key = clip(key, 0, editor_enemy_name_length-1);
    return editor_enemy_name[key];
}


/* level editor: SurgeScript entities */
void editor_ssobj_init()
{
    surgescript_vm_t* vm = surgescript_vm();
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    int fill_counter = 0;

    editor_ssobj = NULL;
    editor_ssobj_count = 0;

    /* count SurgeScript objects tagged as "entity" */
    surgescript_tagsystem_foreach_tagged_object(tag_system, "entity", &editor_ssobj_count, editor_ssobj_register);

    /* register entities */
    editor_ssobj = mallocx(editor_ssobj_count * sizeof(*editor_ssobj));
    surgescript_tagsystem_foreach_tagged_object(tag_system, "entity", &fill_counter, editor_ssobj_register);
}

void editor_ssobj_register(const char* entity_name, void* data)
{
    surgescript_vm_t* vm = surgescript_vm();
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    if(!surgescript_tagsystem_has_tag(tag_system, entity_name, "private")) {
        int *counter = (int*)data;
        if(editor_ssobj != NULL)
            editor_ssobj[*counter] = str_dup(entity_name);
        (*counter)++;
    }
}

void editor_ssobj_release()
{
    for(int i = 0; i < editor_ssobj_count; i++)
        free(editor_ssobj[i]);
    free(editor_ssobj);
    editor_ssobj = NULL;
}

/* associates an integer to each SurgeScript entity */
int editor_ssobj_id(const char* entity_name)
{
    /* find i such that editor_ssobj[i] == entity_name */
    for(int i = 0; i < editor_ssobj_count; i++) {
        if(strcmp(entity_name, editor_ssobj[i]) == 0)
            return i;
    }

    /* not found */
    return -1;
}

const char* editor_ssobj_name(int entity_id)
{
    int id = clip(entity_id, 0, editor_ssobj_count - 1);
    return editor_ssobj[id];
}



/* level editor: bricks */
void editor_brick_init()
{
    /* layer & flip flags */
    editor_layer = BRL_DEFAULT;
    editor_flip = BRF_NOFLIP;

    /* which are the valid bricks? */
    if(brickset_loaded()) {
        int i, j;

        /* count valid bricks */
        editor_brick_count = 0;
        for(i = 0; i < brickset_size(); i++)
            editor_brick_count += brick_exists(i) ? 1 : 0;

        /* read valid bricks */
        editor_brick = mallocx(editor_brick_count * sizeof(*editor_brick));
        for(i = j = 0; i < brickset_size(); i++) {
            if(brick_exists(i))
                editor_brick[j++] = i;
        }
    }
    else {
        editor_brick = NULL;
        editor_brick_count = 0;
    }
}

void editor_brick_release()
{
    if(editor_brick != NULL)
        free(editor_brick);

    editor_brick = NULL;
    editor_brick_count = 0;
}

/* index of brick_id at editor_brick[], or -1 if not found */
int editor_brick_index(int brick_id)
{
    if(editor_brick != NULL) {
        int begin = 0, mid = 0;
        int end = editor_brick_count - 1;
        while(begin <= end) {
            mid = (begin + end) / 2;
            if(brick_id < editor_brick[mid])
                end = mid - 1;
            else if(brick_id > editor_brick[mid])
                begin = mid + 1;
            else
                return mid;
        }
        return -1;
    }
    else
        return -1;    
}

/* the index-th valid brick - at editor_brick[] */
int editor_brick_id(int index)
{
    if(editor_brick != NULL) {
        index = clip(index, 0, editor_brick_count - 1);
        return editor_brick[index];
    }
    else
        return 0;
}




/* level editor: grid */

/* initializes the grid module */
void editor_grid_init()
{
    editor_grid_enabled = TRUE;
}

/* releases the grid module */
void editor_grid_release()
{
}

/* updates the grid module */
void editor_grid_update()
{
    if(editorcmd_is_triggered(editor_cmd, "snap-to-grid")) {
        editor_grid_enabled = !editor_grid_enabled;
        video_showmessage("Snap to grid: %s", editor_grid_enabled ? "ON" : "OFF");
    }
}

/* returns the size of the grid */
v2d_t editor_grid_size()
{
    if(!editor_grid_enabled)
        return v2d_new(1,1);
    else
        return v2d_new(8,8);
}

/* aligns position to a cell in the grid */
v2d_t editor_grid_snap(v2d_t position)
{
    v2d_t topleft = v2d_subtract(editor_camera, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));

    int w = EDITOR_GRID_W;
    int h = EDITOR_GRID_H;
    int cx = (int)topleft.x % w;
    int cy = (int)topleft.y % h;

    int xpos = -cx + ((int)position.x / w) * w;
    int ypos = -cy + ((int)position.y / h) * h;

    return v2d_add(topleft, v2d_new(xpos, ypos));
}


/* level editor actions */



/* action: constructor (entity) */
editor_action_t editor_action_entity_new(int is_new_object, enum editor_entity_type obj_type, int obj_id, v2d_t obj_position)
{
    editor_action_t o;
    o.type = is_new_object ? EDA_NEWOBJECT : EDA_DELETEOBJECT;
    o.obj_type = obj_type;
    o.obj_id = obj_id;
    o.obj_position = obj_position;
    o.obj_old_position = obj_position;
    o.layer = editor_layer;
    o.flip = editor_flip;

    /* are we removing a brick? Store its layer & flip flags */
    if(!is_new_object && obj_type == EDT_BRICK) {
        brick_list_t *it, *brick_list = entitymanager_retrieve_all_bricks();
        for(it=brick_list; it; it=it->next) {
            if(brick_id(it->data) == o.obj_id) {
                float dist = v2d_magnitude(v2d_subtract(brick_position(it->data), o.obj_position));
                if(dist < EPSILON) {
                    o.layer = brick_layer(it->data);
                    o.flip = brick_flip(it->data);
                    brick_kill(it->data);
                }
            }
        }
    }

    return o;
}

/* action: constructor (spawn point) */
editor_action_t editor_action_spawnpoint_new(int is_changing, v2d_t obj_position, v2d_t obj_old_position)
{
    editor_action_t o;
    o.type = is_changing ? EDA_CHANGESPAWN : EDA_RESTORESPAWN;
    o.obj_id = 0;
    o.obj_position = obj_position;
    o.obj_old_position = obj_old_position;
    o.obj_type = EDT_ITEM;
    o.layer = editor_layer;
    o.flip = editor_flip;
    return o;
}

/* initializes the editor_action module */
void editor_action_init()
{
    /* linked list */
    editor_action_buffer_head = mallocx(sizeof *editor_action_buffer_head);
    editor_action_buffer_head->in_group = FALSE;
    editor_action_buffer_head->prev = NULL;
    editor_action_buffer_head->next = NULL;
    editor_action_buffer = editor_action_buffer_head;
    editor_action_buffer_cursor = editor_action_buffer_head;
}


/* releases the editor_action module */
void editor_action_release()
{
    /* linked list */
    editor_action_buffer_head = editor_action_delete_list(editor_action_buffer_head);
    editor_action_buffer = NULL;
    editor_action_buffer_cursor = NULL;
}

/* registers a new editor_action */
void editor_action_register(editor_action_t action)
{
    /* ugly, but these fancy group stuff
     * shouldn't be availiable on the interface */
    static int registering_group = FALSE;
    static uint32 group_key;

    if(action.obj_type != EDT_GROUP) {
        editor_action_list_t *c, *it, *node;

        /* creating new node */
        node = mallocx(sizeof *node);
        node->action = action;
        node->in_group = registering_group;
        if(node->in_group)
            node->group_key = group_key;

        /* cursor */
        c = editor_action_buffer_cursor;
        if(c != NULL)
            c->next = editor_action_delete_list(c->next);

        /* inserting the node into the linked list */
        it = editor_action_buffer;
        while(it->next != NULL)
            it = it->next;
        it->next = node;
        node->prev = it;
        node->next = NULL;

        /* updating the cursor */
        editor_action_buffer_cursor = node;
    }
    else {
        static uint32 auto_increment = 0xbeef; /* dummy value */
        editorgrp_entity_list_t *list, *it;

        /* registering a group of objects */
        registering_group = TRUE;
        group_key = auto_increment++;
        list = editorgrp_get_group(action.obj_id);
        for(it=list; it; it=it->next) {
            editor_action_t a;
            editorgrp_entity_t e = it->entity;
            enum editor_entity_type my_type = EDITORGRP_ENTITY_TO_EDT(e.type);
            a = editor_action_entity_new(TRUE, my_type, e.id, v2d_add(e.position, action.obj_position));
            editor_action_register(a);
        }
        registering_group = FALSE;
    }
}

/* deletes an editor_action list */
editor_action_list_t* editor_action_delete_list(editor_action_list_t *list)
{
    editor_action_list_t *p, *next;

    p = list;
    while(p != NULL) {
        next = p->next;
        free(p);
        p = next;
    }

    return NULL;
}

/* undo */
void editor_action_undo()
{
    editor_action_list_t *p;
    editor_action_t a;

    if(editor_action_buffer_cursor != editor_action_buffer_head) {
        /* moving the cursor */
        p = editor_action_buffer_cursor;
        editor_action_buffer_cursor = editor_action_buffer_cursor->prev;

        /* UNDOing a group? */
        if(p->in_group && p->prev && p->prev->in_group && p->group_key == p->prev->group_key)
            editor_action_undo();

        /* undo */
        a = p->action;
        a.type = /* reverse of a.type ??? */
        (a.type == EDA_NEWOBJECT) ? EDA_DELETEOBJECT :
        (a.type == EDA_DELETEOBJECT) ? EDA_NEWOBJECT :
        (a.type == EDA_CHANGESPAWN) ? EDA_RESTORESPAWN :
        (a.type == EDA_RESTORESPAWN) ? EDA_CHANGESPAWN :
        a.type;
        editor_action_commit(a);
    }
    else
        video_showmessage("Already at oldest change.");
}


/* redo */
void editor_action_redo()
{
    editor_action_list_t *p;
    editor_action_t a;

    if(editor_action_buffer_cursor->next != NULL) {
        /* moving the cursor */
        editor_action_buffer_cursor = editor_action_buffer_cursor->next;
        p = editor_action_buffer_cursor;

        /* REDOing a group? */
        if(p->in_group && p->next && p->next->in_group && p->group_key == p->next->group_key)
            editor_action_redo();
        
        /* redo */
        a = p->action;
        editor_action_commit(a);
    }
    else
        video_showmessage("Already at newest change.");
}


/* commit action */
void editor_action_commit(editor_action_t action)
{
    if(action.type == EDA_NEWOBJECT) {
        /* new object */
        switch(action.obj_type) {
            case EDT_BRICK: {
                /* new brick */
                level_create_brick(action.obj_id, action.obj_position, action.layer, action.flip);
                break;
            }

            case EDT_ITEM: {
                /* new item */
                level_create_item(action.obj_id, action.obj_position);
                break;
            }

            case EDT_ENEMY: {
                /* new enemy */
                level_create_enemy(editor_enemy_key2name(action.obj_id), action.obj_position);
                break;
            }

            case EDT_SSOBJ: {
                /* new SurgeScript object */
                level_create_ssobject(editor_ssobj_name(action.obj_id), action.obj_position);
                break;
            }

            case EDT_GROUP: {
                /* new group of objects */
                editorgrp_entity_list_t *list, *it;
                list = editorgrp_get_group(action.obj_id);
                for(it=list; it; it=it->next) {
                    editor_action_t a;
                    editorgrp_entity_t e = it->entity;
                    enum editor_entity_type my_type = EDITORGRP_ENTITY_TO_EDT(e.type);

                    /* e.layer (flip) has higher precedence than editor_layer (flip) */
                    bricklayer_t old_layer = editor_layer;
                    brickflip_t old_flip = editor_flip;

                    editor_layer = (e.layer != BRL_DEFAULT) ? e.layer : editor_layer;
                    editor_flip = (e.flip != BRF_NOFLIP) ? e.flip : editor_flip;
                    a = editor_action_entity_new(TRUE, my_type, e.id, v2d_add(e.position, action.obj_position));
                    editor_action_commit(a);

                    editor_flip = old_flip;
                    editor_layer = old_layer;
                }
                break;
            }
        }
    }
    else if(action.type == EDA_DELETEOBJECT) {
        /* delete object */
        switch(action.obj_type) {
            case EDT_BRICK: {
                /* delete brick */
                brick_list_t *brick_list = entitymanager_retrieve_all_bricks(); /* FIXME: retrieve relevant entities only? */
                for(brick_list_t *it = brick_list; it != NULL; it = it->next) {
                    if(brick_id(it->data) == action.obj_id) {
                        float dist = v2d_magnitude(v2d_subtract(brick_position(it->data), action.obj_position));
                        if(dist < EPSILON)
                            brick_kill(it->data);
                    }
                }
                entitymanager_release_retrieved_brick_list(brick_list);
                break;
            }

            case EDT_ITEM: {
                /* delete legacy item */
                item_list_t *item_list = entitymanager_retrieve_all_items();
                for(item_list_t *it = item_list; it != NULL; it = it->next) {
                    if(it->data->type == action.obj_id) {
                        float dist = v2d_magnitude(v2d_subtract(it->data->actor->position, action.obj_position));
                        if(dist < EPSILON)
                            it->data->state = IS_DEAD;
                    }
                }
                entitymanager_release_retrieved_item_list(item_list);
                break;
            }

            case EDT_ENEMY: {
                /* delete legacy object */
                enemy_list_t *enemy_list = entitymanager_retrieve_all_objects();
                for(enemy_list_t *it = enemy_list; it != NULL; it = it->next) {
                    if(editor_enemy_name2key(it->data->name) == action.obj_id) {
                        float dist = v2d_magnitude(v2d_subtract(it->data->actor->position, action.obj_position));
                        if(dist < EPSILON)
                            it->data->state = ES_DEAD;
                    }
                }
                entitymanager_release_retrieved_object_list(enemy_list);
                break;
            }

            case EDT_GROUP: {
                /* can't delete a group directly */
                break;
            }

            case EDT_SSOBJ: {
                /* delete SurgeScript entity */
                surgescript_vm_t* vm = surgescript_vm();
                surgescript_object_t* root = surgescript_vm_root_object(vm);
                surgescript_object_traverse_tree_ex(root, &action, editor_remove_ssobj);
                break;
            }
        }
    }
    else if(action.type == EDA_CHANGESPAWN) {
        /* change spawn point */
        level_set_spawn_point(action.obj_position);
        spawn_players();
    }
    else if(action.type == EDA_RESTORESPAWN) {
        /* restore spawn point */
        level_set_spawn_point(action.obj_old_position);
        spawn_players();
    }
}

bool editor_remove_ssobj(surgescript_object_t* object, void* data)
{
    if(surgescript_object_is_active(object)) {
        if(surgescript_object_has_tag(object, "entity")) {
            const char* object_name = surgescript_object_name(object);
            editor_action_t *action = (editor_action_t*)data;
            if(editor_ssobj_id(object_name) == action->obj_id) {
                v2d_t delta = v2d_subtract(scripting_util_world_position(object), action->obj_position);
                if(v2d_magnitude(delta) < EPSILON)
                    surgescript_object_kill(object);
            }
        }

        return true;
    }
    else
        return false;
}

bool editor_pick_ssobj(surgescript_object_t* object, void* data)
{
    if(surgescript_object_is_active(object)) {
        if(surgescript_object_has_tag(object, "entity")) {
            v2d_t topleft = v2d_subtract(editor_camera, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
            float a[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float b[4] = { editor_cursor.x + topleft.x , editor_cursor.y + topleft.y , editor_cursor.x + topleft.x + 1 , editor_cursor.y + topleft.y + 1 };

            /* find the bounding box of the entity */
            const char* name = surgescript_object_name(object);
            const animation_t* anim = sprite_animation_exists(name, 0) ? sprite_get_animation(name, 0) : sprite_get_animation(NULL, 0);
            const image_t* img = sprite_get_image(anim, 0);
            v2d_t hot_spot = anim->hot_spot;
            v2d_t worldpos = scripting_util_world_position(object);
            a[0] = worldpos.x - hot_spot.x;
            a[1] = worldpos.y - hot_spot.y;
            a[2] = a[0] + image_width(img);
            a[3] = a[1] + image_height(img);

            /* got collision between the cursor and the entity */
            if(bounding_box(a, b)) {
                surgescript_object_t** result = (surgescript_object_t**)data;
                if(NULL == *result || scripting_util_object_zindex(object) >= scripting_util_object_zindex(*result))
                    *result = object;
            }
        }

        return true;
    }
    else
        return false;
}



/* extradata: extra metadata for SurgeScript objects */
const char* hash_of_ssobj(const surgescript_object_t* object)
{
    /* TODO: use a faster key type */
    static char buf[24] = { [23] = 0 };
    char* p = buf + 23;
    surgescript_objecthandle_t h = surgescript_object_handle(object);
    do { *--p = "0123456789abcdef"[h & 0xF]; } while(h /= 16);
    return p;
}

v2d_t get_ssobj_spawnpoint(const surgescript_object_t* object)
{
    ssobj_extradata_t* data = get_ssobj_extradata(object);
    return data != NULL ? data->spawn_point : v2d_new(0, 0);
}

int is_ssobj_spawned_in_the_editor(const surgescript_object_t* object)
{
    ssobj_extradata_t* data = get_ssobj_extradata(object);
    return data != NULL ? data->spawned_in_the_editor : FALSE;
}

/* Get the editor data of a given object
   It may return NULL (if no such data exists) */
ssobj_extradata_t* get_ssobj_extradata(const surgescript_object_t* object)
{
    const char* hash = hash_of_ssobj(object);
    return hashtable_ssobj_extradata_t_find(ssobj_extradata, hash); /* may be NULL */
}

void set_ssobj_extradata(const surgescript_object_t* object, ssobj_extradata_t extradata)
{
    const char* hash = hash_of_ssobj(object);
    ssobj_extradata_t* data = hashtable_ssobj_extradata_t_find(ssobj_extradata, hash);
    if(data == NULL) {
        data = mallocx(sizeof *data);
        *data = extradata;
        hashtable_ssobj_extradata_t_add(ssobj_extradata, hash, data);
    }
    else
        *data = extradata; /*data->spawn_point = extradata.spawn_point;*/
}

void free_ssobj_extradata(ssobj_extradata_t* data)
{
    free(data);
}
