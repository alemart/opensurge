// -----------------------------------------------------------------------------
// File: menubutton.ss
// Description: Menu Button script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.UI.Text;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Audio.Sound;

//
// MenuButton
// A button that you can use for your own purposes
//
// How to use:
//
// using SurgeEngine.Vector2;
// object "MenuButtonTest" {
//     btn = spawn("MenuButton");
//
//     state "main" {
//         if(timeout(5.0))
//             btn.press(); // will press the button
//         if(btn.pressed)
//             Console.print("The button was pressed");
//     }
//
//     fun constructor() {
//         btn.transform.position = Vector2(210, 120);
//         btn.text = "Click me";
//         btn.focus(); // optional
//     }
// }
//
// Properties:
// transform: the transform of the button, Transform object, read-only
// focus: does this button have focus? bool
// text: the text of the button, string
// pressed: whether this button is pressed or not, bool
// sound: sound to be played when pressed, Sound object \ null
//
// Functions:
// press(): press the button (plays nice animation)
// focus(): enable focus
// blur(): disable focus
//
object "MenuButton" is "private", "entity", "awake"
{
    public readonly transform = Transform();
    public sound = Sound("samples/select.wav"); // may be changed
    actor = Actor("MenuButton");
    hand = Actor("SelectHand64");
    label = Text("menu.button");

    // main
    state "main"
    {
    }

    // clicking animation
    state "pressing"
    {
        c = actor.animation.frameCount;
        f = actor.animation.frame;
        j = f < c/2 ? f : c - f - 1;
        label.offset = Vector2(j, j-16);
        hand.offset = Vector2(5 - actor.width/2 + j, 20 - actor.height + j);
        if(actor.animation.finished) {
            actor.anim = 0;
            label.offset = Vector2(0, -16);
            hand.offset = Vector2(5 - actor.width/2, 20 - actor.height);
            state = "pressed";
        }
    }

    // used for 'pressed' property
    state "pressed"
    {
        state = "main";
    }

    //
    // press(): will press the button
    //
    fun press()
    {
        if(state == "main") {
            actor.anim = 2;
            hand.animation.speedFactor = 0;
            if(sound != null)
                sound.play();
            state = "pressing";
        }
    }

    //
    // Property: pressed (read-only)
    //
    fun get_pressed()
    {
        return state == "pressed";
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
    // Property: focus
    //
    fun get_focus()
    {
        return hand.visible;
    }

    fun set_focus(focus)
    {
        hand.visible = focus;
        hand.animation.speedFactor = 1.0;
    }

    fun focus()
    {
        this.focus = true;
    }

    fun blur()
    {
        this.focus = false;
    }

    // ---------- internal ----------

    fun constructor()
    {
        hand.anim = 0;
        hand.visible = false;
        hand.zindex = 0.51;
        hand.offset = Vector2(5 - actor.width/2, 20 - actor.height);
        label.align = "center";
        label.offset = Vector2(0, -16);
    }
}