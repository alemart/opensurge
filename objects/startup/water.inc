// ---------------------------------------------------------------------------
// Open Surge Engine
// http://opensurge2d.org
//
// File:   objects/default_startup/water.inc
// Desc:   water
// Author: Alexandre
// Date:   2012-02-04, 2012-02-10, 2012-02-13
// ---------------------------------------------------------------------------

object ".default_startup.water"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        create_child ".default_startup.water.level0"
        create_child ".default_startup.water.level1"
        create_child ".default_startup.water.level2"
        create_child ".default_startup.water.splash_detector"
        create_child ".default_startup.water.drown_detector"
        create_child ".default_startup.water.bubble_creator"
        create_child ".default_startup.water.shield_behavior"
        destroy
    }
}

// ---------------------------------------

object ".default_startup.water.level0"
{
    requires 0.2.0
    always_active

    state "main"
    {
        set_alpha 0.25
        set_animation SD_WATERLEVEL 0
        set_animation_frame "floor($frame) mod 2"
        set_absolute_position "camera_x() - 3*screen_width()/2 + floor($x)" waterlevel()

        let "$frame += (1.0/0.15) * dt()"
        let "$speed = 50"
        let "$x = cond($x >= screen_width(), $x - screen_width(), $x += $speed * dt())"
    }
}

object ".default_startup.water.level1"
{
    requires 0.2.0
    always_active

    state "main"
    {
        set_alpha 0.25
        set_animation SD_WATERLEVEL 0
        set_animation_frame "floor($frame) mod 2"
        set_absolute_position "camera_x() - screen_width()/2 + floor($x)" waterlevel()

        let "$frame += (1.0/0.15) * dt()"
        let "$speed = 50"
        let "$x = cond($x >= screen_width(), $x - screen_width(), $x += $speed * dt())"
    }
}

object ".default_startup.water.level2"
{
    requires 0.2.0
    always_active

    state "main"
    {
        set_alpha 0.25
        set_animation SD_WATERLEVEL 0
        set_animation_frame "floor($frame) mod 2"
        set_absolute_position "camera_x() + screen_width()/2 + floor($x)" waterlevel()

        let "$frame += (1.0/0.15) * dt()"
        let "$speed = 50"
        let "$x = cond($x >= screen_width(), $x - screen_width(), $x += $speed * dt())"
    }
}

// ---------------------------------------

object ".default_startup.water.splash_detector"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        on_player_underwater uw
        change_state nuw
    }

    state uw
    {
        on_player_underwater uw
        change_state splash
    }

    state nuw
    {
        on_player_underwater splash
    }

    state splash
    {
        set_absolute_position player_xpos() waterlevel()
        create_child .default_startup.water.splash 0 0
        on_player_underwater splash_bubbles
        change_state nuw
    }

    state splash_bubbles
    {
        create_child .default_startup.water.splash_bubbles
        change_state uw
    }
}

object ".default_startup.water.splash"
{
    requires 0.2.0
    destroy_if_far_from_play_area

    state "main"
    {
        set_zindex 1.0
        set_alpha 0.75
        set_animation SD_WATERSPLASH 0
        change_state wait
    }

    state "wait"
    {
        on_animation_finished destroy
    }

    state "destroy"
    {
        destroy
    }
}

object ".default_startup.water.splash_bubbles"
{
    requires 0.2.0
    destroy_if_far_from_play_area

    state "main"
    {
        hide
        let "$timeout = 0.5"
        set_absolute_position player_xpos()+8*player_direction() waterlevel()
        change_state "create_bubble"
    }

    state "loop"
    {
        if "($t += dt()) >= $timeout" "destroy"
        on_timeout "0.05 + 0.15 * ($t / $timeout)" "create_bubble"
    }

    state "create_bubble"
    {
        let "$_waterairbubble_anim = 4*random(2)"
        create_child .default_startup.water.bubble -10+random(20) 10+(40*$t)+random(20)
        let "$_waterairbubble_anim = 4*random(2)"
        create_child .default_startup.water.bubble -10+random(20) 10+(40*$t)+random(20)
        let "$_waterairbubble_anim = 4*random(2)"
        create_child .default_startup.water.bubble -10+random(20) 10+(40*$t)+random(20)
        let "$_waterairbubble_anim = 4*random(2)"
        create_child .default_startup.water.bubble -10+random(20) 10+(40*$t)+random(20)
        let "$_waterairbubble_anim = 4*random(2)"
        create_child .default_startup.water.bubble -10+random(20) 10+(40*$t)+random(20)
        change_state "loop"
    }

    state "destroy"
    {
        destroy
    }
}

// ---------------------------------------

object ".default_startup.water.drown_detector"
{
    requires 0.2.0
    detach_from_camera
    always_active

    state "main"
    {
        let "$seconds = 5" // time to show the counter

        hide
        set_zindex 1.1
        set_animation SD_WATERCOUNTER 0
        set_absolute_position screen_width()/2 screen_height()/2
        change_state idle
    }

    state idle
    {
        unless "player_seconds_remaining_to_drown() <= $seconds" idle
        let "$x = 0.0"
        play_music "musics/drowning.ogg"
        change_state display
    }

    state display
    {
        // nice effect
        let "$x = cond($x >= 1.0, $x - 1.0, $x) + dt()"
        set_scale 0.5+2*$x 0.5+2*$x
        set_animation_frame "clamp(ceil(player_seconds_remaining_to_drown()), 0, $seconds)"
        on_player_drown destroy

        // no longer underwater?
        hide
        if "player_seconds_remaining_to_drown() > $seconds" restore
        show
        on_player_underwater display
        hide
        change_state restore
    }

    state restore
    {
        play_level_music
        change_state idle
    }

    state destroy
    {
        hide
        if "($t+=dt())<1.5" destroy
        play_music "musics/silence.ogg" -1
        destroy
    }
}

// ---------------------------------------

object ".default_startup.water.bubble_creator"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        on_player_underwater uw
        change_state nuw
    }

    state uw
    {
        on_water_shield uw // don't create bubbles if watershield
        attach_to_player 8*player_direction() -13
        on_player_drown drowned
        on_timeout "1.0+random(100)*0.01" create_bubble
        on_player_underwater uw
        change_state nuw
    }

    state nuw
    {
        on_player_underwater uw
    }

    state create_bubble
    {
        let "$_waterairbubble_anim = 0"
        create_child .default_startup.water.bubble 0 0
        if "random(4) > 0" uw
        create_child .default_startup.water.bubble player_direction()*(3+random(3)) -2+random(5)
        change_state uw
    }

    state drowned
    {
        attach_to_player 0 -15
        on_timeout 0.1 create_bubble2
    }

    state create_bubble2
    {
        let "$_waterairbubble_anim = cond(random(2)==0,0,3)"
        create_child .default_startup.water.bubble 0 0
        if "random(2) == 0" drowned
        let "$_waterairbubble_anim = cond(random(2)==0,0,3)"
        create_child .default_startup.water.bubble 0 -2
        change_state drowned
    }
}

object ".default_startup.water.bubble"
{
    requires 0.2.0
    destroy_if_far_from_play_area

    state "main"
    {
        hide
        let $amp=2+random(2)
        set_zindex 0.99
        set_alpha 0.5
        set_animation SD_WATERAIRBUBBLE $_waterairbubble_anim
        change_state go
    }

    state "go"
    {
        if "ypos() - hotspot_y()/2 <= waterlevel()" destroy
        show
        move "($amp*(2*pi())) * sin(2*pi()*($t+=dt()))" "-30" // the chain rule
    }

    state "destroy"
    {
        destroy
    }
}

// ------------------------------------------

object ".default_startup.water.shield_behavior"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        on_player_underwater uw
    }

    state "uw"
    {
        on_fire_shield delete_shield
        on_thunder_shield delete_shield
        on_player_underwater uw
        change_state main
    }

    state "delete_shield"
    {
        let $x=player_xspeed()
        let $y=player_yspeed()
        hit_player
        set_player_xspeed $x
        set_player_yspeed $y
        change_state main
    }
}
