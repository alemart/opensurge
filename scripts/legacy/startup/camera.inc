// -----------------------------------
// Improved Camera System
// script by Alexandre
// date: 2011-01-14, 2011-01-15
// -----------------------------------

// the built-in camera of the engine focus exactly
// on the hotspot of the player (or on the hotspot
// of the object which has the focus). We'll use
// that camera to build a more fluid scrolling
// system, which isn't always locked on the player.

// this object implements the camera system
object ".default_startup.camera"
{
    requires 0.2.0
    always_active

    state "main"
    {
        // this object is obsolete.
        destroy

        hide
        set_animation SD_CAMERA 2
        set_zindex 1.1
        set_alpha 0.75
        create_child ".default_startup.camera.x" 0 0 "cam_x"
        create_child ".default_startup.camera.y" 0 0 "cam_y"
        create_child ".default_startup.camera.lookupdown" 0 0 "cam_lookupdown"
        create_child ".default_startup.camera.focus_player" 0 0 "cam_focusplayer"
        set_absolute_position player_xpos() player_ypos()
        let "$enabled = 1"
        observe_active_player
        request_camera_focus
        change_state "wait"
    }

    state "wait"
    {
        on_timeout 0.5 "loop"
    }

    state "loop"
    {
        // is the camera enabled?
        unless "$enabled" "loop"
        on_level_cleared "loop"
        on_player_death "loop"

        // update position
        change_child_state "cam_x" "@update"
        change_child_state "cam_y" "@update_air"
        change_child_state "cam_y" "@update_gnd"
        change_child_state "cam_x" "@get_position"
        change_child_state "cam_y" "@get_position"
        change_child_state "cam_lookupdown" "@get_position"
        set_absolute_position "$_cam_x" "$_cam_y + ($offy = $_cam_offy)"

        // the code below isn't really required, but
        // it helps to make this object play
        // nicely with others which might
        // request the focus of the camera

        // has something else requested the focus
        // of the camera?
        on_camera_focus "loop"

        // something else = the active player? Is
        // the camera focusing the active player?
        change_child_state "cam_focusplayer" "@check"
        unless "$_focusing_the_player" "lost_focus" // no, let's just wait.

        // yes, the active player is the focus of
        // the camera! but wait... I want to be the focus!
        change_child_state "cam_focusplayer" "@restore_focus" // this will make my state be changed to "restore_focus" immediately,
                                                              // not in the next frame (which would be the case if we called change_state)
    }

    state "lost_focus"
    {
        unless "$enabled" "lost_focus"

        // this state isn't really required, but
        // it helps to make this object play
        // nicely with others which might
        // request the focus of the camera as well

        change_child_state "cam_focusplayer" "@check"
        unless "$_focusing_the_player" "lost_focus"
        change_child_state "cam_focusplayer" "@restore_focus"
    }

    state "restore_focus"
    {
        unless "$enabled" "restore_focus"

        change_child_state "cam_x" "@reset"
        change_child_state "cam_y" "@reset"
        change_child_state "cam_lookupdown" "@reset"
        change_child_state "cam_x" "@get_position"
        change_child_state "cam_y" "@get_position"
        change_child_state "cam_lookupdown" "@get_position"
        set_absolute_position "$_cam_x" "$_cam_y + ($offy = $_cam_offy)"

        request_camera_focus
        change_state "loop"
    }

    // ---- public methods ----
    state "@get_position"
    {
        let "$_cam_x = xpos()"
        let "$_cam_y = ypos() - $offy"
        return_to_previous_state
    }

    state "@disable"
    {
        let "$enabled = 0"
        drop_camera_focus
        return_to_previous_state
    }

    state "@enable"
    {
        let "$enabled = 1"
        request_camera_focus
        return_to_previous_state
    }
}

// this object is responsible for moving the camera
// on the x-axis
object ".default_startup.camera.x"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        set_animation SD_CAMERA 0
        set_zindex 1
        observe_active_player
        set_absolute_position player_xpos() player_ypos()
        //let "$left_border = screen_width() * 0.475" // 152
        //let "$right_border = screen_width() * 0.525" // 168
        let "$left_border = screen_width() * 0.45" // 144
        let "$right_border = screen_width() * 0.5" // 160
        let "$l = screen_width() / 2" // 160
        change_state "wait"
    }

    state "wait"
    {
        set_absolute_position xpos() player_ypos()
    }

    // ---- public methods ----
    state "@get_position"
    {
        let "$_cam_x = xpos()"
        return_to_previous_state
    }

    state "@update"
    {
        // retrieve the position of the camera
        change_parent_state "@get_position"
        let "$cx = $_cam_x"

        // retrieve the position and the instantaneous velocity of the player
        let "$pv = player_xpos() - $px"
        let "$px = player_xpos()"

        // math: get the offset
        let "$should_move = (($l+$px-$cx < $left_border) or ($l+$px-$cx > $right_border)) and (abs($pv) > 0) and (sign($pv) == sign($px - $cx))"
        let "$offset = cond($px>$cx, $right_border, $left_border) - $l"

        // attach to the player
        set_absolute_position xpos() player_ypos()
        unless "$should_move" "wait"
        set_absolute_position "player_xpos() - $offset" player_ypos()

        // done
        return_to_previous_state
    }

    state "@reset"
    {
        set_absolute_position player_xpos() player_ypos()
        return_to_previous_state
    }
}

// this object is responsible for moving the camera
// on the y-axis
object ".default_startup.camera.y"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        set_animation SD_CAMERA 1
        set_zindex 1
        create_child ".default_startup.camera.y.air_checker" 0 0 "air_checker"
        observe_active_player
        set_absolute_position player_xpos() player_ypos()
        let "$top_border = screen_height() * 0.285"
        let "$bottom_border = screen_height() * 0.571"
        let "$ground_level = screen_height() * 0.5" //0.429"
        let "$l = screen_height() / 2"
        change_state "wait"
    }

    state "wait"
    {
        set_absolute_position player_xpos() ypos()
    }

    // ---- public methods ----
    state "@get_position"
    {
        let "$_cam_y = ypos()"

        change_closest_object_state ".default_startup.camera.lookupdown" "@get_position"
        let "$_cam_y = cond(abs($_cam_offy) <> 0, player_ypos() + ($l - $ground_level), $_cam_y)" // gambiarra

        return_to_previous_state
    }

    state "@update_gnd"
    {
        // am I in the air?
        change_child_state "air_checker" "@check"
        if "$_in_the_air" "wait"
        //on_button_down "fire1" "wait" // bug!

        // cute stuff
        set_alpha 1.0
        set_absolute_position player_xpos() ypos()

        // retrieve the position of the camera
        change_parent_state "@get_position"
        let "$cy = $_cam_y"

        // retrieve the position and the instantaneous velocity of the player
        //let "$pv = player_ypos() - $py" // zero?
        let "$py = player_ypos()"

        // goto $ground_level
        let "$dt = 0.07"
        let "$ds = ($py + ($l - $ground_level)) - $cy"
        let "$vel = $ds / $dt"
        let "$y = cond((abs($ds) > 1) and ($dt > 0.00001), $cy + $vel * dt(), $py + ($l - $ground_level))"
        set_absolute_position player_xpos() $y

        // done
        return_to_previous_state
    }

    state "@update_air"
    {
        // am I in the air?
        change_child_state "air_checker" "@check"
        if "not($_in_the_air)" "wait" // this technique simulates an "on_player_NOT_in_the_air" command
        
        // cute stuff
        set_alpha 0.5
        set_absolute_position player_xpos() ypos()

        // retrieve the position of the camera
        change_parent_state "@get_position"
        let "$cy = $_cam_y"

        // retrieve the position and the instantaneous velocity of the player
        let "$pv = player_ypos() - $py"
        let "$py = player_ypos()"

        // math: get the offset
        let "$should_move = (($l+$py-$cy < $top_border) or ($l+$py-$cy > $bottom_border)) and (abs($pv) > 0) and (sign($pv) == sign($py - $cy))"
        let "$offset = cond($py>$cy, $bottom_border, $top_border) - $l"

        // attach to the player
        set_absolute_position player_xpos() ypos()
        unless "$should_move" "wait"
        set_absolute_position player_xpos() "player_ypos() - $offset"

        // done
        return_to_previous_state
    }

    state "@reset"
    {
        let "$py = player_ypos()"
        set_absolute_position player_xpos() "player_ypos() + ($l - $ground_level)"
        return_to_previous_state
    }
}

// this object is responsible for implementing
// the "look up/down" functionality
object ".default_startup.camera.lookupdown"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        observe_active_player
        let "$top = -screen_height() * 0.4" //0.464"
        let "$bottom = screen_height() * 0.4" //393"
        let "$speed = screen_height() * 0.535"
        let "$timeout = 2.0" // seconds until the camera scrolls
        let "$y = 0"
        change_state "loop"
    }

    state "loop"
    {
        // reset timer
        let "$t = 0"

        // move the camera back to its neutral position
        let "$is_neutral = (abs($y) <= 2)"
        let "$y = cond($is_neutral, 0, $y - sign($y) * $speed * dt())"

        // look up/down
        unless "$is_neutral" "loop"
        on_player_lookup "charge_up"
        on_player_duck "charge_down"
    }

    state "charge_up"
    {
        if "($t += dt()) >= $timeout" "move_up"
        on_player_lookup "charge_up"
        change_state "loop"
    }

    state "charge_down"
    {
        if "($t += dt()) >= $timeout" "move_down"
        on_player_duck "charge_down"
        change_state "loop"
    }

    state "move_up"
    {
        let "$y = max($y - $speed * dt(), $top)"
        on_player_lookup "move_up"
        change_state "loop"
    }

    state "move_down"
    {
        let "$y = min($y + $speed * dt(), $bottom)"
        on_player_duck "move_down"
        change_state "loop"
    }

    // ---- public methods ----
    state "@get_position"
    {
        let "$_cam_offy = $y"
        return_to_previous_state
    }

    state "@reset"
    {
        let "$y = 0"
        return_to_previous_state
    }
}



// ---------------- utilities -----------------


// this object tells whether the player is
// in the air or not
object ".default_startup.camera.y.air_checker"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        observe_active_player
        change_state "wait"
    }

    state "wait"
    {
    }

    // ---- public methods ----
    state "@check"
    {
        let "$_in_the_air = 1"
        on_player_in_the_air "wait"
        let "$_in_the_air = 0"
        return_to_previous_state
    }
}

// this object tells whether the camera is focusing
// the active player or not
object ".default_startup.camera.focus_player"
{
    requires 0.2.0
    always_active

    state "main"
    {
        hide
        observe_active_player
        change_state "wait"
    }

    state "wait"
    {
    }

    // ---- public methods ----
    state "@check"
    {
        let "$_focusing_the_player = 1"
        on_camera_focus_player "wait"
        let "$_focusing_the_player = 0"
        return_to_previous_state
    }

    state "@restore_focus"
    {
        change_parent_state "restore_focus" // this will be done immediately
        return_to_previous_state
    }
}
