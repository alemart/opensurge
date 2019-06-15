// Dialog Box Component: Button
// script by Alexandre
// ----------------------------------------------------------------------------
//
// Some button components for you to choose:
//
// .dialogbox.base.button
// Your basic button
//
//
// If you want to create button components, here's what you need:
//
// *) The "destroy" state must destroy this component
//
// *) The "hidden" state must hide this component
//
// *) The "continue" state must show a continue icon (the message
//    of the dialog box isn't over yet, but the box got full)
//
// *) The "stop" state must display a stop icon (the message
//    of the dialog box has been fully shown)
//

// Your basic button
object .dialogbox.base.button
{
    requires 0.2.0
    detach_from_camera
    always_active

    state main
    {
        set_zindex $_dialogbox_base_bgz+0.2
        change_state hidden
    }

    state destroy
    {
        destroy
    }

    state hidden
    {
        set_animation DIALOGBOX_BUTTON 0
        set_absolute_position $_dialogbox_base_bgx+($_dialogbox_base_bgw-width())/2 $_dialogbox_base_bgy+$_dialogbox_base_bgh-3
    }

    state continue
    {
        let $anim=1
        change_state blink
    }

    state stop
    {
        set_animation DIALOGBOX_BUTTON 2
        set_absolute_position $_dialogbox_base_bgx+($_dialogbox_base_bgw-width())/2 $_dialogbox_base_bgy+$_dialogbox_base_bgh-3
        on_button_pressed fire1 :stop
    }

    // --------------------------------------

    state :continue
    {
        play_sample samples/select_2.wav
        change_state hidden
    }

    state :stop
    {
        play_sample samples/select_2.wav
        change_state hidden
    }

    state flip
    {
        let "$anim = 1 - $anim"
        on_button_pressed fire1 :continue
        return_to_previous_state
    }

    state blink
    {
        set_animation DIALOGBOX_BUTTON $anim
        set_absolute_position $_dialogbox_base_bgx+($_dialogbox_base_bgw-width())/2 $_dialogbox_base_bgy+$_dialogbox_base_bgh-3
        on_button_pressed fire1 :continue
        on_timeout 0.5 flip
    }
}
