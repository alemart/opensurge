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
    speed = -180.0; // pixels per second
    timeToLive = 0.5; // seconds
    deceleration = speed / timeToLive;

    state "main"
    {
        // move
        transform.translateBy(0, speed * Time.delta);

        // decelerate
        speed -= deceleration * Time.delta;
        if(speed >= 0.0)
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