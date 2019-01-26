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
    transform = Transform();
    actor = Actor("MenuButton");
    label = Text("GoodNeighborsLarge");

    state "main"
    {
    }

    //
    // Property: transform (read-only)
    //
    fun get_transform()
    {
        return transform;
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

    // ---------- internal ----------

    fun constructor()
    {
        actor.anim = 5;
        label.align = "center";
        label.offset = Vector2(0, -16);
    }
}