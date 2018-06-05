// ---------------------------------------------------------------------------
// Open Surge Engine
// http://opensnc.sourceforge.net'
//
// File:   objects/default_startup/hud.inc
// Desc:   "you got through act X" animation
// Author: Alexandre
// Date:   2010-12-20, 2011-01-02
// ---------------------------------------------------------------------------

//
// In case you want to create an object that gives a
// secret bonus, add some score to the global variable
// called $_secretbonus
//
// Example: run in your script
// let "$_secretbonus += 2000"
//

// ----------------------------------------------
// This object handles the "level cleared"
// animation
// ----------------------------------------------
object ".default_startup.level_cleared"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide

        let "$_ds_levelcleared_zindex = 9999"
        let "$_ds_levelcleared_ringbonus = 0"
        let "$_ds_levelcleared_timebonus = 0"
        let "$_ds_levelcleared_secretbonus = 0"
        let "$_ds_levelcleared_totalbonus = 0"

        let "$time = 0"
        let "$_secretbonus = 0"

        change_state "wait"
    }

    state "wait"
    {
        let "$time += dt()"
        on_level_cleared "cleared"
    }

    state "cleared"
    {
        let "$_ds_levelcleared_ringbonus = 100 * collectibles()"
        let "$_ds_levelcleared_timebonus = 10 * max(600 - floor($time), 0)"
        let "$_ds_levelcleared_secretbonus = $_secretbonus"
        let "$_ds_levelcleared_totalbonus = 0"
        create_child ".default_startup.level_cleared.handle_music"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.5 "create stuff"
    }

    state "create stuff"
    {
        create_child ".default_startup.level_cleared.handle_fadeout"
        create_child ".default_startup.level_cleared.handle_bonus"
        create_child ".default_startup.level_cleared.title"
        create_child ".default_startup.level_cleared.title2"
        create_child ".default_startup.level_cleared.ringbonus"
        create_child ".default_startup.level_cleared.ringbonus_counter"
        create_child ".default_startup.level_cleared.timebonus"
        create_child ".default_startup.level_cleared.timebonus_counter"
        create_child ".default_startup.level_cleared.secretbonus"
        create_child ".default_startup.level_cleared.secretbonus_counter"
        create_child ".default_startup.level_cleared.totalbonus"
        create_child ".default_startup.level_cleared.totalbonus_counter"
        destroy
    }
}

// takes care of the goal song
object ".default_startup.level_cleared.handle_music"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        let "$vol = music_volume()"
        change_state "fade_out"
    }

    state "fade_out"
    {
        set_music_volume "max(0, music_volume() - 1.0*dt())"
        on_timeout 1.5 "goal"
    }

    state "goal"
    {
        set_music_volume "$vol"
        play_music "musics/goal.ogg"
        change_state "wait"
    }

    state "wait"
    {
        // music "double-click" bugfix
        on_timeout "music_duration() * 0.95" "end"
    }

    state "end"
    {
        play_music "samples/silence.ogg" -1
        destroy
    }
}

// responsible for fading out the screen
object ".default_startup.level_cleared.handle_fadeout"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        let "$_ds_levelcleared_fadeout = 0"
        change_state "wait"
    }

    state "wait"
    {
        if "$_ds_levelcleared_fadeout" "go"
    }

    state "go"
    {
        let "$_fade_in = 0"
        let "$_fade_duration = 1.0"
        create_child ".fade_effect"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.1 "next_level"
    }

    state "next_level"
    {
        next_level
    }
}

// this object counts all the bonuses the user may have got
object ".default_startup.level_cleared.handle_bonus"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        change_state "wait"
    }

    state "wait"
    {
        on_timeout 3.85 "count"
    }

    state "count"
    {
        play_sample "bonus count"
        let "$d = 100"

        let "$_ds_levelcleared_totalbonus += min($d, $_ds_levelcleared_ringbonus)"
        let "$_ds_levelcleared_totalbonus += min($d, $_ds_levelcleared_timebonus)"
        let "$_ds_levelcleared_totalbonus += min($d, $_ds_levelcleared_secretbonus)"

        let "$_ds_levelcleared_ringbonus = max(0, $_ds_levelcleared_ringbonus-$d)"
        let "$_ds_levelcleared_timebonus = max(0, $_ds_levelcleared_timebonus-$d)"
        let "$_ds_levelcleared_secretbonus = max(0, $_ds_levelcleared_secretbonus-$d)"

        if "($_ds_levelcleared_ringbonus == 0) and ($_ds_levelcleared_timebonus == 0) and ($_ds_levelcleared_secretbonus == 0)" "cash"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.04 "count"
    }

    state "cash"
    {
        play_sample "cash"
        add_to_score "$_ds_levelcleared_totalbonus"
        change_state "wait a bit"
    }

    state "wait a bit"
    {
        on_timeout 2.5 "end"
    }

    state "end"
    {
        let "$_ds_levelcleared_fadeout = 1"
        destroy
    }
}

// title LED
object ".default_startup.level_cleared.title"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position -112 50
        set_zindex "$_ds_levelcleared_zindex"
        change_state "move"
    }

    state "move"
    {
        set_absolute_position "min(160, xpos()+1280*dt())" "ypos()"
        textout_centre "hud_levelend" 0 0 "$HUD_CLEARED_LINE1"
    }
}

object ".default_startup.level_cleared.title2"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position 432 75
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.35 "move"
    }

    state "move"
    {
        if "act() == 1" "move1"
        if "act() == 2" "move2"
        if "act() == 3" "move3"
        change_state "move1"
    }

    state "move1"
    {
        set_absolute_position "max(160, xpos()-1280*dt())" "ypos()"
        textout_centre "hud_levelend" 0 0 "$HUD_CLEARED_LINE2 <color=ffdd00>$HUD_CLEARED_LINE2_ACT 1</color>"
    }

    state "move2"
    {
        set_absolute_position "max(160, xpos()-1280*dt())" "ypos()"
        textout_centre "hud_levelend" 0 0 "$HUD_CLEARED_LINE2 <color=ff2211>$HUD_CLEARED_LINE2_ACT 2</color>"
    }

    state "move3"
    {
        set_absolute_position "max(160, xpos()-1280*dt())" "ypos()"
        textout_centre "hud_levelend" 0 0 "$HUD_CLEARED_LINE2 <color=0022ff>$HUD_CLEARED_LINE2_ACT 3</color>"
    }
}

// ring bonus LED
object ".default_startup.level_cleared.ringbonus"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position -112 120
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.5 "move"
    }

    state "move"
    {
        set_absolute_position "min(150, xpos()+640*dt())" "ypos()"
        textout_right "hud" 0 0 "<color=ffff00>$HUD_CLEARED_COLLECTIBLES</color>"
    }
}

object ".default_startup.level_cleared.ringbonus_counter"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position 432 120
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.5 "move"
    }

    state "move"
    {
        set_absolute_position "max(220, xpos()-640*dt())" "ypos()"
        textout_right "hud" 0 0 "$_ds_levelcleared_ringbonus"
    }
}

// time bonus LED
object ".default_startup.level_cleared.timebonus"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position -112 135
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.75 "move"
    }

    state "move"
    {
        set_absolute_position "min(150, xpos()+640*dt())" "ypos()"
        textout_right "hud" 0 0 "<color=ffff00>$HUD_CLEARED_TIME</color>"
    }
}

object ".default_startup.level_cleared.timebonus_counter"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position 432 135
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.75 "move"
    }

    state "move"
    {
        set_absolute_position "max(220, xpos()-640*dt())" "ypos()"
        textout_right "hud" 0 0 "$_ds_levelcleared_timebonus"
    }
}

// secret bonus LED
object ".default_startup.level_cleared.secretbonus"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position -112 150
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.0 "move"
    }

    state "move"
    {
        set_absolute_position "min(150, xpos()+640*dt())" "ypos()"
        textout_right "hud" 0 0 "<color=ffff00>$HUD_CLEARED_SECRET</color>"
    }
}

object ".default_startup.level_cleared.secretbonus_counter"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position 432 150
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.0 "move"
    }

    state "move"
    {
        set_absolute_position "max(220, xpos()-640*dt())" "ypos()"
        textout_right "hud" 0 0 "$_ds_levelcleared_secretbonus"
    }
}

// total bonus LED
object ".default_startup.level_cleared.totalbonus"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position -112 180
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.25 "move"
    }

    state "move"
    {
        set_absolute_position "min(150, xpos()+640*dt())" "ypos()"
        textout_right "hud" 0 0 "<color=ffff00>$HUD_CLEARED_TOTAL</color>"
    }
}

object ".default_startup.level_cleared.totalbonus_counter"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_absolute_position 432 180
        set_zindex "$_ds_levelcleared_zindex"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.25 "move"
    }

    state "move"
    {
        set_absolute_position "max(220, xpos()-640*dt())" "ypos()"
        textout_right "hud" 0 0 "$_ds_levelcleared_totalbonus"
    }
}

