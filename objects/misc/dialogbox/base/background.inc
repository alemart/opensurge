// Dialog Box Component: Background
// script by Alexandre
// ----------------------------------------------------------------------------
//
// Some background components for you to choose:
//
// .dialogbox.base.background
// A simple dialog background with no animation.
//
// .dialogbox.base.background[slide_from_bottom]
// The dialog will slide from the bottom of the screen.
//
// .dialogbox.base.background[slide_from_top]
// The dialog will slide from the top of the screen.
//
//
// If you want to create new background components, this is what
// you need to implement:
//
// *) A "destroy" state, which will destroy the background
//
// *) A "disappear" state, which will start the disappearing animation
//
// *) A "hide" state, which will hide the dialog box
//
// *) A "show" state, which will show the dialog box after it's been hidden
//
// *) Once the appearing animation is completed, change
//    its parent state to ".dialogbox.base.background:on_appear"
//
// *) Once the disappearing animation is completed, change
//    its parent state to ".dialogbox.base.background:on_disappear"
//
// *) Export the position of the dialog box to the globals
//    $_dialogbox_base_bgx and $_dialogbox_base_bgy
//
// *) Export the width() and the height() of the dialog box to
//    $_dialogbox_base_bgw and $_dialogbox_base_bgh
//
// *) Export zindex() to $_dialogbox_base_bgz
//

// A simple background
object .dialogbox.base.background
{
    requires 0.2.0
    always_active
    detach_from_camera

    state main
    {
        set_zindex 99999
        set_animation DIALOGBOX_BACKGROUND 0
        set_alpha 0.9

        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=screen_height()-height()-10
        let $_dialogbox_base_bgw=width()
        let $_dialogbox_base_bgh=height()
        let $_dialogbox_base_bgz=zindex()
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy

        change_state appearance_is_complete
    }

    state disappear
    {
        set_absolute_position xpos() $_dialogbox_base_bgy=screen_height()
        change_state disappearance_is_complete
    }

    state destroy
    {
        destroy
    }

    state hide
    {
        set_absolute_position $_dialogbox_base_bgx=infinity() $_dialogbox_base_bgy=infinity()
        return_to_previous_state
    }

    state show
    {
        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=screen_height()-height()-10
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy
        return_to_previous_state
    }

    // --------------------------------------------

    state appearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_appear
        change_state wait
    }

    state disappearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_disappear
        change_state wait
    }

    state wait
    {
    }
}

// This background slides from the bottom of the screen
object .dialogbox.base.background[slide_from_bottom]
{
    requires 0.2.0
    always_active
    detach_from_camera

    state main
    {
        set_zindex 99999
        set_animation DIALOGBOX_BACKGROUND 0
        set_alpha 0.9

        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=screen_height()
        let $_dialogbox_base_bgw=width()
        let $_dialogbox_base_bgh=height()
        let $_dialogbox_base_bgz=zindex()
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy

        let $speed=screen_height()
        change_state appearing
    }

    state disappear
    {
        change_state disappearing
    }

    state destroy
    {
        destroy
    }

    state hide
    {
        set_absolute_position $_dialogbox_base_bgx=infinity() $_dialogbox_base_bgy=infinity()
        return_to_previous_state
    }

    state show
    {
        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=screen_height()-height()-10
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy
        return_to_previous_state
    }

    // --------------------------------------------

    state appearing
    {
        let $_dialogbox_base_bgy=max(screen_height()-height()-10,ypos()-$speed*dt())
        set_absolute_position $_dialogbox_base_bgx=xpos() $_dialogbox_base_bgy
        if ypos()<=screen_height()-height()-10 appearance_is_complete
    }

    state disappearing
    {
        let $_dialogbox_base_bgy=ypos()+$speed*dt()
        set_absolute_position $_dialogbox_base_bgx=xpos() $_dialogbox_base_bgy
        if ypos()>=screen_height() disappearance_is_complete
    }

    state appearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_appear
        change_state wait
    }

    state disappearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_disappear
        change_state wait
    }

    state wait
    {
    }
}

// This background slides from the top of the screen
object .dialogbox.base.background[slide_from_top]
{
    requires 0.2.0
    always_active
    detach_from_camera

    state main
    {
        set_zindex 99999
        set_animation DIALOGBOX_BACKGROUND 0
        set_alpha 0.9

        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=-height()
        let $_dialogbox_base_bgw=width()
        let $_dialogbox_base_bgh=height()
        let $_dialogbox_base_bgz=zindex()
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy

        let $speed=screen_height()
        change_state appearing
    }

    state disappear
    {
        change_state disappearing
    }

    state destroy
    {
        destroy
    }

    state hide
    {
        set_absolute_position $_dialogbox_base_bgx=infinity() $_dialogbox_base_bgy=infinity()
        return_to_previous_state
    }

    state show
    {
        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=10
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy
        return_to_previous_state
    }

    // --------------------------------------------

    state appearing
    {
        let $_dialogbox_base_bgy=min(10,ypos()+$speed*dt())
        set_absolute_position $_dialogbox_base_bgx=xpos() $_dialogbox_base_bgy
        if ypos()>=10 appearance_is_complete
    }

    state disappearing
    {
        let $_dialogbox_base_bgy=ypos()-$speed*dt()
        set_absolute_position $_dialogbox_base_bgx=xpos() $_dialogbox_base_bgy
        if ypos()<=-height() disappearance_is_complete
    }

    state appearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_appear
        change_state wait
    }

    state disappearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_disappear
        change_state wait
    }

    state wait
    {
    }
}

// component of the dialog box system
object .dialogbox.base.background[cutscene]
{
    requires 0.2.0
    always_active
    detach_from_camera

    state main
    {
        set_zindex 99999
        set_animation DIALOGBOX_BACKGROUND 0
        set_alpha 0.9

        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=-height()
        let $_dialogbox_base_bgw=width()
        let $_dialogbox_base_bgh=height()
        let $_dialogbox_base_bgz=zindex()
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy

        let $speed=screen_height()
        change_state appearing
    }

    state disappear
    {
        change_state disappearing
    }

    state destroy
    {
        destroy
    }

    state hide
    {
        set_absolute_position $_dialogbox_base_bgx=infinity() $_dialogbox_base_bgy=infinity()
        return_to_previous_state
    }

    state show
    {
        let $_dialogbox_base_bgx=(screen_width()-width())/2
        let $_dialogbox_base_bgy=40
        set_absolute_position $_dialogbox_base_bgx $_dialogbox_base_bgy
        return_to_previous_state
    }

    // --------------------------------------------

    state appearing
    {
        let $_dialogbox_base_bgy=min(40,ypos()+$speed*dt())
        set_absolute_position $_dialogbox_base_bgx=xpos() $_dialogbox_base_bgy
        if ypos()>=40 appearance_is_complete
    }

    state disappearing
    {
        let $_dialogbox_base_bgy=ypos()-$speed*dt()
        set_absolute_position $_dialogbox_base_bgx=xpos() $_dialogbox_base_bgy
        if ypos()<=-height() disappearance_is_complete
    }

    state appearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_appear
        change_state wait
    }

    state disappearance_is_complete
    {
        change_parent_state .dialogbox.base.background:on_disappear
        change_state wait
    }

    state wait
    {
    }
}
