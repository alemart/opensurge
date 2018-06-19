// Dialog Box Component: Avatar
// script by Alexandre
// ----------------------------------------------------------------------------
//
// Some avatar components for you to choose:
//
// .dialogbox.base.avatar
// Your basic avatar
//
//
// If you want to create avatar components, here's what you need:
//
// *) The "destroy" state must destroy this component
//
// *) A "refresh" state. in case the user wants to change the displayed avatar
//
// *) The avatar should display animation #$_dialogbox_avatar_id of the
//    DIALOGBOX_AVATAR sprite
//
// *) The avatar should be positioned relative to the dialog position,
//    $_dialogbox_base_bgx and $_dialogbox_base_bgy
//
// *) Export $_dialogbox_base_avatar_width (equals to width()) and
//    $_dialogbox_base_avatar_height (equals to height())
//

// Your basic avatar
object .dialogbox.base.avatar
{
    requires 0.2.0
    always_active
    detach_from_camera

    state main
    {
        hide
        let $_dialogbox_base_avatar_exists=1
        let $id=$_dialogbox_avatar_id
        change_state idle
    }

    state refresh
    {
        let $id=$_dialogbox_avatar_id
        change_state idle       
    }

    state destroy
    {
        let $_dialogbox_base_avatar_exists=0
        destroy
    }

    // --------------------------------------------

    state idle
    {
        set_zindex $_dialogbox_base_bgz+0.1
        set_animation DIALOGBOX_AVATAR $id

        let $_dialogbox_base_avatar_width=width()
        let $_dialogbox_base_avatar_height=height()

        let $offset_x=10
        let $offset_y=($_dialogbox_base_bgh-height())/2
        set_absolute_position $_dialogbox_base_bgx+$offset_x $_dialogbox_base_bgy+$offset_y
        show
    }
}
