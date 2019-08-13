// -----------------------------------------------------------------------------
// File: score_text.ss
// Description: a tiny text that appears when the player gains score
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.UI.Text;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

object "Score Text" is "entity", "private", "disposable"
{
    text = Text("Tiny");
    transform = Transform();
    speed = 90; // pixels per second
    timeToLive = 1.0; // seconds

    state "main"
    {
        transform.move(0, -speed * Time.delta);
        if(timeout(timeToLive * 0.5))
            state = "waiting";
    }

    state "waiting"
    {
        if(timeout(timeToLive * 0.5))
            destroy();
    }

    fun setText(str)
    {
        text.text = String(str);
        return this;
    }

    fun setOffset(x, y)
    {
        text.offset = Vector2(x, y);
        return this;
    }

    fun constructor()
    {
        text.zindex = 0.91;
        text.align = "center";
        text.text = "0";
    }
}