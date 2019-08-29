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
    public time = 10.0; // time in seconds

    box = Actor("Message Box");
    txt = Text("dialogbox");
    spd = Screen.height / 2;
    pad = 16;
    transform = Transform();

    state "main"
    {
        transform.position = Vector2((Screen.width - box.width) / 2, Screen.height);
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
            state = "disappearing";
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
        destroy();
    }

    fun constructor()
    {
        box.zindex = 1001.0;
        txt.zindex = box.zindex + 0.1;
        txt.offset = Vector2(8, 8);
        txt.maxWidth = box.width - 16;
    }

    fun set_text(text)
    {
        txt.text = String(text);
    }

    fun get_text()
    {
        return txt.text;
    }

    /* it appears automatically
    fun appear()
    {
        state = "appearing";
    }*/

    fun disappear()
    {
        if(state == "appearing" || state == "waiting")
            state = "disappearing";
    }
}