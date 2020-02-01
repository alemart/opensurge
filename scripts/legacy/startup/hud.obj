// HUD script
// by Alexandre

//
// Provided that you use this object to handle the HUD
// (this assumption is true by default),
//
// 1) if you want to hide the HUD, run in your script
//    let "$_hide_hud = 1"
//
// 2) if you want to show it again, run
//    let "$_hide_hud = 0"
//

// ----------------------------------------------
// This object handles the HUD (Heads-Up Display)
// ----------------------------------------------
object ".default_startup.hud"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        let "$_ds_hud_zindex = 99999"
        create_child ".default_startup.hud.score"
        create_child ".default_startup.hud.time"
        create_child ".default_startup.hud.collectibles"
        create_child ".default_startup.hud.lives"
        destroy
    }
}

// Display the score
object ".default_startup.hud.score"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        set_absolute_position "-200" "10"

        change_state "go"
    }

    state "go"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "min(15, xpos() + 500*dt())" "ypos()"
        on_level_cleared "go away"

        let "$score = score()"
        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_SCORE</color> $score"
    }

    state "go away"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "xpos() - 500*dt()" "ypos()"

        let "$score = score()"
        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_SCORE</color> $score"
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}

// Display the time
object ".default_startup.hud.time"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex+0.01"

        let "$time = 0"
        let "$_ds_hud_time_minutes = 0"
        let "$_ds_hud_time_seconds = 0"
        let "$_ds_hud_time_tenths = 0"

        set_absolute_position "-400" "25"
        let "$_ds_hud_time_xpos = xpos()"
        let "$_ds_hud_time_ypos = ypos()"

        // trick to get the leading zeroes right
        create_child ".default_startup.hud.time.minutes"
        create_child ".default_startup.hud.time.seconds"
        create_child ".default_startup.hud.time.tenths"

        change_state "go"
    }

    state "go"
    {
        if "$_hide_hud" "hidden"
        on_level_cleared "go away"
        on_player_death "freeze"

        let "$time += dt()"
        let "$_ds_hud_time_minutes = floor($time / 60)"
        let "$_ds_hud_time_seconds = floor($time mod 60)"
        let "$_ds_hud_time_tenths = floor(($time * 100) mod 100)"

        set_absolute_position "min(15, xpos() + 500*dt())" "ypos()"
        let "$_ds_hud_time_xpos = xpos()"
        let "$_ds_hud_time_ypos = ypos()"

        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_TIME</color>"
    }

    state "go away"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "xpos() - 500*dt()" "ypos()"
        let "$_ds_hud_time_xpos = xpos()"
        let "$_ds_hud_time_ypos = ypos()"

        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_TIME</color>"
    }

    state "freeze"
    {
        if "$_hide_hud" "hidden"
        on_level_cleared "go away"

        set_absolute_position "min(15, xpos() + 500*dt())" "ypos()"
        let "$_ds_hud_time_xpos = xpos()"
        let "$_ds_hud_time_ypos = ypos()"

        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_TIME</color>"
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}

object ".default_startup.hud.time.minutes"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        change_state "go"
    }

    state "go"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "$_ds_hud_time_xpos" "$_ds_hud_time_ypos"
        let "$xrel = -8 * floor(log10(max(1,$_ds_hud_time_minutes)))"
        textout "hud_legacy" $xrel 0 "$HUD_TIME $_ds_hud_time_minutes'"
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}

object ".default_startup.hud.time.seconds"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        change_state "with_leading_zero"
    }

    state "with_leading_zero"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "$_ds_hud_time_xpos" "$_ds_hud_time_ypos"
        if "$_ds_hud_time_seconds >= 10" "without_leading_zero"
        textout "hud_legacy" 0 0 "$HUD_TIME   0$_ds_hud_time_seconds\""
    }

    state "without_leading_zero"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "$_ds_hud_time_xpos" "$_ds_hud_time_ypos"
        if "$_ds_hud_time_seconds < 10" "with_leading_zero"
        textout "hud_legacy" 0 0 "$HUD_TIME   $_ds_hud_time_seconds\""
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}

object ".default_startup.hud.time.tenths"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        change_state "with_leading_zero"
    }

    state "with_leading_zero"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "$_ds_hud_time_xpos" "$_ds_hud_time_ypos"
        if "$_ds_hud_time_tenths >= 10" "without_leading_zero"
        textout "hud_legacy" 0 0 "$HUD_TIME      0$_ds_hud_time_tenths"
    }

    state "without_leading_zero"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "$_ds_hud_time_xpos" "$_ds_hud_time_ypos"
        if "$_ds_hud_time_tenths < 10" "with_leading_zero"
        textout "hud_legacy" 0 0 "$HUD_TIME      $_ds_hud_time_tenths"
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}


// Display the amount of collectibles
object ".default_startup.hud.collectibles"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        set_absolute_position "-600" "40"
        let "$blink_interval = 0.3"

        change_state "normal"
    }

    state "normal"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "min(15, xpos() + 500*dt())" "ypos()"
        on_level_cleared "go away"

        let "$collectibles = collectibles()"
        if "$collectibles == 0" "danger"
        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_POWER</color> $collectibles"
    }

    state "danger"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "min(15, xpos() + 500*dt())" "ypos()"
        on_level_cleared "go away"

        let "$collectibles = collectibles()"
        if "$collectibles > 0" "normal"
        textout "hud_legacy" 0 0 "<color=$COLOR_HUDDANGER>$HUD_POWER</color> $collectibles"

        on_timeout "$blink_interval" "blinking_danger"
    }

    state "blinking_danger"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "min(15, xpos() + 500*dt())" "ypos()"
        on_level_cleared "go away"

        let "$collectibles = collectibles()"
        if "$collectibles > 0" "normal"
        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_POWER</color> $collectibles"

        on_timeout "$blink_interval" "danger"
    }

    state "go away"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "xpos() - 500*dt()" "ypos()"

        let "$collectibles = collectibles()"
        textout "hud_legacy" 0 0 "<color=$COLOR_HUD>$HUD_POWER</color> $collectibles"
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}

// Display the number of lives
object ".default_startup.hud.lives"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        set_absolute_position "15" "screen_height()+400"
        set_absolute_position xpos()+hotspot_x() ypos()+hotspot_y() // fix hotspot
        create_child ".default_startup.hud.lives.counter"
        change_state "go"
    }

    state "Surge"
    {
        let "$anim_id = 0"
        change_state "go"
    }

    state "Neon"
    {
        let "$anim_id = 1"
        change_state "go"
    }

    state "Charge"
    {
        let "$anim_id = 2"
        change_state "go"
    }

    state "Tux"
    {
        let "$anim_id = 3"
        change_state "go"
    }

    state "go"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "xpos()" "max(screen_height()-15, ypos() - 500*dt())"
        let "$_ds_hud_lives_xpos = xpos() - hotspot_x()"
        let "$_ds_hud_lives_ypos = ypos() - hotspot_y()"
        on_level_cleared "go away"

        show
        set_animation "SD_HUDHEAD" "$anim_id"

        observe_active_player
        on_observed_player "Surge" "Surge"
        on_observed_player "Neon" "Neon"
        on_observed_player "Charge" "Charge"
        on_observed_player "Tux" "Tux"
    }

    state "go away"
    {
        if "$_hide_hud" "hidden"
        set_absolute_position "xpos()" "ypos() + 500*dt()"
        let "$_ds_hud_lives_xpos = xpos() - hotspot_x()"
        let "$_ds_hud_lives_ypos = ypos() - hotspot_y()"
    }

    state "hidden"
    {
        hide
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}

object ".default_startup.hud.lives.counter"
{
    requires 0.2.0
    always_active
    detach_from_camera

    state "main"
    {
        hide
        set_zindex "$_ds_hud_zindex"
        on_timeout 0.2 "go"
    }

    state "go"
    {
        if "$_hide_hud" "hidden"
        let "$lives = lives()"

        set_absolute_position "$_ds_hud_lives_xpos" "$_ds_hud_lives_ypos"
        textout "hud_legacy" 20 9 "x$lives"
    }

    state "hidden"
    {
        if "$_hide_hud" "hidden"
        return_to_previous_state
    }
}
