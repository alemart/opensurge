// -----------------------------------------------------------------------------
// File: menubutton.ss
// Description: Menu Title widget
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;

object "MenuTitle" is "private", "entity", "awake"
{
    public readonly transform = Transform();
    actor = Actor("MenuButton");
    label = Text("menu.button");

    state "main"
    {
    }

    //
    // Property: text
    //
    fun get_text()
    {
        return label.text;
    }

    fun set_text(text)
    {
        label.text = text;
    }

    //
    // Property: visible
    //
    fun get_visible()
    {
        return actor.visible;
    }

    fun set_visible(visible)
    {
        actor.visible = label.visible = visible;
    }

    // ---------- internal ----------

    fun constructor()
    {
        actor.anim = 5;
        label.align = "center";
        label.offset = Vector2(0, -16);
    }
}