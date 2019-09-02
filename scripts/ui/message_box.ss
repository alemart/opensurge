// -----------------------------------------------------------------------------
// File: message_box.ss
// Description: simple Message Box
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Video.Screen;

//
// Message Box
//
// Properties:
// - text: string. Text to be displayed.
// - time: number. Time to display the message box, in seconds.
//
// Functions:
// - disappear(). Makes the message box disappear (call when visible).
//
// Usage example:
//   mb = Level.spawn("Message Box");
//   mb.text = "Hello, world!";
//
object "Message Box" is "detached", "private", "entity"
{
    public text = ""; // string to be displayed
    public time = 10.0; // time in seconds

    box = Actor("Message Box");
    txt = Text("dialogbox");
    spd = Screen.height;
    pad = 8;
    transform = Transform();
    controller = null;

    state "main"
    {
        box.visible = true;
        txt.text = String(text);
        transform.position = Vector2((Screen.width - box.width) / 2, Screen.height);
        controller.show(this);
        state = "appearing";
    }

    state "appearing"
    {
        // move
        transform.move(0, -spd * Time.delta);

        // are we done?
        ymin = Screen.height - box.height - pad;
        if(transform.position.y <= ymin) {
            transform.position = Vector2(transform.position.x, ymin);
            state = "waiting";
        }
    }

    state "waiting"
    {
        if(timeout(time))
            disappear();
    }

    state "disappearing"
    {
        // move
        transform.move(0, spd * Time.delta);

        // are we done?
        ymax = Screen.height;
        if(transform.position.y >= ymax)
            state = "done";
    }

    state "done"
    {
        controller.hide(this);
        destroy();
    }

    fun constructor()
    {
        this.zindex = 1001.0;
        box.visible = false;
        txt.offset = Vector2(8, 5);
        txt.maxWidth = box.width - txt.offset.x * 2;
        controller = Level.child("Message Box Controller") || Level.spawn("Message Box Controller");
    }

    fun set_zindex(zindex)
    {
        box.zindex = zindex;
        txt.zindex = zindex + 0.01;
    }

    fun get_zindex()
    {
        return box.zindex;
    }

    /* it appears automatically
    fun appear()
    {
        state = "appearing";
    }*/

    fun disappear()
    {
        if(state == "appearing" || state == "waiting") {
            this.zindex -= 0.1;
            state = "disappearing";
        }
    }
}

// Only one Message Box should be
// active at any given time
object "Message Box Controller"
{
    msgbox = null; // active Message Box object

    state "main"
    {
        if(Level.cleared)
            hide(msgbox);
    }

    fun show(mb)
    {
        if(msgbox != null && msgbox != mb)
            msgbox.disappear();
        msgbox = mb;
    }

    fun hide(mb)
    {
        if(msgbox != null && msgbox == mb) {
            msgbox.disappear();
            msgbox = null;
        }
    }
}