// -----------------------------------------------------------------------------
// File: surgescript.ss
// Description: a simple SurgeScript application
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

using SurgeEngine.Level;
using SurgeEngine.Input;
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.UI.Text;

// Object "SurgeScript Demo App" is in the setup list of level surgescript.lev.
// This means that the object is spawned when the level is loaded.
object "SurgeScript Demo App"
{
    // this object has the following components:
    message = spawn("SurgeScript Demo App - Message");
    sprite = spawn("SurgeScript Demo App - Sprite");
    quit = spawn("SurgeScript Demo App - Quit");

    state "main"
    {
    }
}

object "SurgeScript Demo App - Message"
{
    state "main"
    {
        // display a message...
        Console.print("Welcome to SurgeScript " + SurgeScript.version + "!");
        state = "done";
    }

    state "done"
    {
        // ...and then do nothing
    }
}

object "SurgeScript Demo App - Quit"
{
    input = Input("default");
    actionButton = "fire1";
    pauseButton = "fire3";
    backButton = "fire4";

    state "main"
    {
        // quit the app if the action, the pause or the back button is pressed
        if(input.buttonPressed(actionButton) || input.buttonPressed(pauseButton) || input.buttonPressed(backButton))
            Level.abort();
    }
}

object "SurgeScript Demo App - Sprite" is "private", "detached", "entity"
{
    actor = Actor("Surge");
    transform = Transform();
    text = spawn("SurgeScript Demo App - Sprite - Text"); // spawn a child object
    winning = 17;

    state "main"
    {
    }

    fun constructor()
    {
        // set the initial position and animation of the sprite
        transform.position = Vector2(213, 132); // position on the screen
        actor.anim = winning;
    }
}

object "SurgeScript Demo App - Sprite - Text" is "private", "detached", "entity"
{
    text = Text("GoodNeighbors");
    lineBreak = "\n";

    state "main"
    {
    }

    fun constructor()
    {
        // set up the text to be displayed
        text.text = "SurgeScript" + lineBreak + "version " + SurgeScript.version;
        text.align = "center";
        text.offset = Vector2(0, -68); // an offset relative to the parent object, i.e., the Sprite
    }
}