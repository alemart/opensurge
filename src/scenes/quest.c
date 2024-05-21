/*
 * Open Surge Engine
 * quest.c - quest scene
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

#include <stdbool.h>
#include <string.h>
#include "quest.h"
#include "level.h"
#include "../core/scene.h"
#include "../core/quest.h"
#include "../core/logfile.h"
#include "../core/storyboard.h"
#include "../core/asset.h"
#include "../core/video.h"
#include "../util/util.h"
#include "../util/stringutil.h"

/* private data */
#define STACK_MAX 25 /* maximum number of active quests */
static int top = -1;
static struct queststack_t {
    quest_t* quest;
    int next_level;
    bool abort_quest;
} stack[STACK_MAX]; /* a stack of quests */

#define WARN(fmt, ...) do { \
    video_showmessage(fmt, __VA_ARGS__); \
    logfile_message("[Quest scene] " fmt, __VA_ARGS__); \
} while(0)

static void push_builtin_scene(const char *str);



/* public scene functions */

/*
 * quest_init()
 * Initializes the quest scene.
 */
void quest_init(void *path_to_qst_file)
{
    const char* filepath = (const char*)path_to_qst_file;

    if(++top >= STACK_MAX)
        fatal_error("Do you have a circular dependency in your quests? The quest stack can't hold more than %d quests.", STACK_MAX);

    stack[top].quest = quest_load(filepath);
    stack[top].next_level = 0;
    stack[top].abort_quest = false;

    logfile_message("Pushed quest \"%s\" onto the quest stack...", quest_file(stack[top].quest));
}

/*
 * quest_release()
 * Releases the quest scene
 */
void quest_release()
{
    if(top >= 0) {
        logfile_message("Popping quest \"%s\" from the quest stack...", quest_file(stack[top].quest));
        quest_unload(stack[top].quest);
        top--;
    }

    logfile_message("The quest has been released.");
}

/*
 * quest_render()
 * This function does nothing
 */
void quest_render()
{
    ;
}

/*
 * quest_update()
 * Updates the quest manager
 */
void quest_update()
{
    if(top < 0) {

        /* empty stack: this shouldn't happen */
        logfile_message("ERROR: empty quest stack");
        scenestack_pop();
        return;

    }
    else if(stack[top].abort_quest) {

        /* aborted quest */
        const quest_t* quest = stack[top].quest;
        logfile_message("Quest \"%s\" has been aborted.", quest_file(quest));
        scenestack_pop();
        return;

    }
    else if(stack[top].next_level >= quest_entry_count(stack[top].quest)) {

        /* cleared quest */
        const quest_t* quest = stack[top].quest;
        logfile_message("Quest \"%s\" has been cleared!", quest_file(quest));
        scenestack_pop();
        return;

    }
    else {

        /* go to the next level */
        const quest_t* quest = stack[top].quest;
        int index = stack[top].next_level++;
        const char* path = quest_entry_path(quest, index);

        if(quest_entry_is_level(quest, index)) {

            /* load a .lev file */
            if(asset_exists(path))
                scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)path);
            else
                WARN("Can't load \"%s\"", path);

            return;

        }
        else if(quest_entry_is_quest(quest, index)) {

            /*
               load a .qst file

               There is a caveat when loading a quest: if the player aborts it
               (via the pause menu for example), the previous quest will be
               resumed - and in the next entry. Loading a quest within another
               quest is not the same as loading a sequence of levels.

               We do not resolve circular dependencies when loading .qst files.
               If a circular dependency exists, the limited stack size will
               make the program crash eventually.

               What would we do if we found a circular dependency? Warn the
               user? Crash the engine as we already do? We already have a large
               enough stack size anyway.

               Even if we search for circular dependencies by reading the .qst
               files, we cannot guarantee that there are no such dependencies:
               quests may also be loaded via scripting. This won't be a problem
               in practice as long as we pick a large enough stack size.
            */
            if(asset_exists(path))
                scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)path);
            else
                WARN("Can't load \"%s\"", path);

            return;

        }
        else if(quest_entry_is_builtin_scene(quest, index)) {

            /* load a built-in scene */
            push_builtin_scene(path);
            return;

        }
        else {

            /* this shouldn't happen. fail silently */
            logfile_message("ERROR - unknown quest entry: %s", path);
            scenestack_pop();
            return;

        }
    }
}

/*
 * quest_abort()
 * Aborts the current quest. It will be popped from the stack
 */
void quest_abort()
{
    if(top >= 0)
        stack[top].abort_quest = true;
}

/*
 * quest_set_next_level()
 * Jumps to the given level, 0 <= lev <= n
 * If set to n (level_count), the quest will be cleared
 */
void quest_set_next_level(int id)
{
    if(top >= 0) {
        const quest_t* quest = stack[top].quest;
        int n = quest_entry_count(quest);

        stack[top].next_level = clip(id, 0, n);
    }
}

/*
 * quest_next_level()
 * id of the current level, 0 <= id <= n,
 * where n is the number of levels of the quest
 */
int quest_next_level()
{
    return top >= 0 ? stack[top].next_level : 0;
}

/*
 * quest_current()
 * Returns the current quest, or NULL if no quest is active
 */
const quest_t* quest_current()
{
    return top >= 0 ? stack[top].quest : NULL;
}




/* private */

void push_builtin_scene(const char *str)
{
    if(str_icmp(str, "<options>") == 0)
        scenestack_push(storyboard_get_scene(SCENE_OPTIONS), NULL);
    else if(str_icmp(str, "<language_select>") == 0)
        scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);
    else if(str_icmp(str, "<credits>") == 0)
        scenestack_push(storyboard_get_scene(SCENE_CREDITS), NULL);
    else if(str_icmp(str, "<stage_select>") == 0)
        scenestack_push(storyboard_get_scene(SCENE_STAGESELECT), NULL);
    else
        fatal_error("Quest error: unrecognized symbol \"%s\"", str);
}
