// ---------------------------------------------------------------------------
// Open Surge Engine
// http://opensnc.sourceforge.net
//
// File:   objects/default_startup/opening_animation.inc
// Desc:   opening animation
// Author: Alexandre
// Date:   2011-01-01
// ---------------------------------------------------------------------------

object ".default_startup.opening_animation"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide

        set_absolute_position 0 0
        let "$_ds_op_zindex = 9999"

        create_child ".default_startup.opening_animation.blackscreen"
        create_child ".default_startup.opening_animation.playermovement"
        create_child ".default_startup.opening_animation.background"
        create_child ".default_startup.opening_animation.topstrip"
        create_child ".default_startup.opening_animation.leftstrip" 0 0
        create_child ".default_startup.opening_animation.leftstrip" 0 240
        create_child ".default_startup.opening_animation.leftstrip" 0 480
        create_child ".default_startup.opening_animation.bottomstrip"
        create_child ".default_startup.opening_animation.title"
        create_child ".default_startup.opening_animation.subtext"
        create_child ".default_startup.opening_animation.subtexthideoverlay"
        create_child ".default_startup.opening_animation.act"
        destroy
    }
}

object ".default_startup.opening_animation.blackscreen"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        set_animation "SD_HUDBLACKSCREEN" 0
        set_zindex "$_ds_op_zindex - 0.01"
        set_absolute_position 0 0
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 1.5 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.playermovement"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        on_timeout 1.0 "cooldown"
    }

    state "cooldown"
    {
        disable_player_movement
        on_timeout 2.0 "destroy"
    }

    state "destroy"
    {
        enable_player_movement
        destroy
    }
}

object ".default_startup.opening_animation.background"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        set_animation "SD_HUDBACKGROUND" 0
        set_zindex "$_ds_op_zindex"
        set_absolute_position 0 240
        let "$speed = 1440"
        change_state "appear"
    }

    state "appear"
    {
        set_absolute_position "xpos()" "max(0, ypos()-$speed*dt())"
        on_timeout 3.5 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()" "ypos()+$speed*dt()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.topstrip"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        set_animation "SD_HUDTOPSTRIP" 0
        set_zindex "$_ds_op_zindex + 0.01"
        set_absolute_position 0 -60
        let "$speed = 960"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.5 "appear"
    }

    state "appear"
    {
        set_absolute_position "xpos()" "min(-10, ypos()+$speed*dt())"
        on_timeout 3.0 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()" "ypos()-$speed*dt()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.leftstrip"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        set_animation "SD_HUDLEFTSTRIP" 0
        set_zindex "$_ds_op_zindex + 0.02"
        set_absolute_position -90 "ypos()"
        let "$speed = 900"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.625 "appear"
    }

    state "appear"
    {
        set_absolute_position "min(0, xpos()+$speed*dt())" "ypos() - 50*dt()"
        on_timeout 2.375 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()-$speed*dt()" "ypos()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.bottomstrip"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        set_animation "SD_HUDBOTTOMSTRIP" 0
        set_zindex "$_ds_op_zindex + 0.03"
        set_absolute_position 320 180
        let "$speed = 1440"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.825 "appear"
    }

    state "appear"
    {
        set_absolute_position "max(0, xpos()-$speed*dt())" "ypos()"
        on_timeout 2.175 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()+$speed*dt()" "ypos()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.title"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_op_zindex + 0.04"
        set_absolute_position 320 64
        let "$speed = 960"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.825 "appear"
    }

    state "appear"
    {
        set_absolute_position "max(100, xpos()-$speed*dt())" "ypos()"
        textout "hud_title" 1 1 "<color=222222>$LEVEL_NAME</color>" 210
        textout "hud_title" 0 0 "$LEVEL_NAME" 210
        on_timeout 2.175 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()+$speed*dt()" "ypos()"
        textout "hud_title" 1 1 "<color=222222>$LEVEL_NAME</color>" 210
        textout "hud_title" 0 0 "$LEVEL_NAME" 210
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.subtext"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_op_zindex + 0.04"
        set_absolute_position 370 208
        let "$speed = 1440"
        let "$x = 30"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.825 "appear"
    }

    state "appear"
    {
        set_absolute_position "max(50, xpos()-$speed*dt())" "ypos()"
        let "$x -= 100*dt()"
        textout "hud_subtext" "1+min($x, 0)" 1 "<color=222222>Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!!</color>"
        textout "hud_subtext" "min($x, 0)" 0 "Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!! Surge!!! Neon!!! Charge!!!"
        on_timeout 2.175 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()+$speed*dt()" "ypos()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.opening_animation.subtexthideoverlay"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        set_animation "SD_HUDSUBTEXTHIDEOVERLAY" 0
        set_zindex "$_ds_op_zindex + 0.045"
        set_absolute_position -90 "ypos()"
        let "$speed = 900"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.625 "appear"
    }

    state "appear"
    {
        set_absolute_position "min(0, xpos()+$speed*dt())" "ypos()"
        on_timeout 2.375 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()-$speed*dt()" "ypos()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}


object ".default_startup.opening_animation.act"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        execute "destroy" if "act() < 1 or act() > 3"
        let "$anim = clamp(act()-1, 0, 2)"
        set_animation "SD_LEVELACT" "$anim"
        set_zindex "$_ds_op_zindex + 0.05"
        set_absolute_position 320 165
        let "$speed = 1440"
        change_state "cooldown"
    }

    state "cooldown"
    {
        on_timeout 0.825 "appear"
    }

    state "appear"
    {
        set_absolute_position "max(265, xpos()-$speed*dt())" "ypos()"
        on_timeout 2.175 "disappear"
    }

    state "disappear"
    {
        set_absolute_position "xpos()+$speed*dt()" "ypos()"
        on_timeout 1.0 "destroy"
    }

    state "destroy"
    {
        destroy
    }
}
