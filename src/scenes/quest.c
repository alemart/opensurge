/*
 * Open Surge Engine
 * quest.c - quest scene
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

#include <string.h>
#include "quest.h"
#include "level.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/scene.h"
#include "../core/quest.h"
#include "../core/logfile.h"
#include "../core/storyboard.h"

/* private data */
#define STACK_MAX 32 /* maximum number of simultaneous quests */
static int top = -1;
static struct {
    quest_t* current_quest;
    int next_level;
    int abort_quest;
} stack[STACK_MAX]; /* one can stack quests */

static void push_appropriate_scene(const char *str);



/* public scene functions */

/*
 * quest_init()
 * Initializes the quest scene.
 */
void quest_init(void *path_to_qst_file)
{
    const char* filepath = (const char*)path_to_qst_file;

    if(++top >= STACK_MAX)
        fatal_error("The quest stack can't hold more than %d quests.", STACK_MAX);

    stack[top].current_quest = quest_load(filepath);
    stack[top].next_level = 0;
    stack[top].abort_quest = FALSE;

    logfile_message("Pushed quest \"%s\" (\"%s\") onto the quest stack...", stack[top].current_quest->file, stack[top].current_quest->name);
}

/*
 * quest_release()
 * Releases the quest scene
 */
void quest_release()
{
    quest_unload(stack[top--].current_quest);
    logfile_message("The quest has been released.");
}

/*
 * quest_render()
 * Actually, this function does nothing
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
    /* invalid quest */
    if(stack[top].current_quest->level_count == 0)
        fatal_error("Quest '%s' has no levels.", stack[top].current_quest->file);

    /* quest manager */
    if(stack[top].next_level < stack[top].current_quest->level_count && !stack[top].abort_quest) {
        /* next level... */
        const char *path_to_lev_file = stack[top].current_quest->level_path[stack[top].next_level++];
        push_appropriate_scene(path_to_lev_file);
    }
    else {
        /* the user has cleared (or exited) the quest! */
        logfile_message(
            "Quest '%s' has been %s Popping...",
            stack[top].current_quest->file,
            (stack[top].abort_quest ? "aborted." : "cleared!")
        );
        scenestack_pop();
    }
}

/*
 * quest_abort()
 * Aborts the current quest, popping it from the stack
 */
void quest_abort()
{
    if(top >= 0)
        stack[top].abort_quest = TRUE;
}

/*
 * quest_set_next_level()
 * Jumps to the given level, 0 <= lev <= n
 * If set to n (level_count), the quest will be cleared
 */
void quest_set_next_level(int id)
{
    if(top >= 0) {
        const quest_t* quest = stack[top].current_quest;
        stack[top].next_level = clip(id, 0, quest->level_count);
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
    return top >= 0 ? stack[top].current_quest : NULL;
}





/* ------ private --------- */
void push_appropriate_scene(const char *str)
{
    if(str[0] == '<' && str[strlen(str)-1] == '>') {
        /* not a level? TODO: at the moment, the engine can't handle those scenes repeatedly on the stack */
        if(str_icmp(str, "<options>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_OPTIONS), NULL);
        else if(str_icmp(str, "<language_select>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);
        else if(str_icmp(str, "<credits>") == 0)
            scenestack_push(storyboard_get_scene(SCENE_CREDITS), NULL);
        else
            fatal_error("Quest error: unrecognized symbol '%s'", str);
    }
    else {
        /* just a regular level */
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)str);
    }
}
