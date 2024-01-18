// -----------------------------------------------------------------------------
// File: play_classic_levels.ss
// Description: setup script of the Play Classic Levels scene
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Actor;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Behavior.DirectionalMovement;
using SurgeEngine.Behavior.CircularMovement;


object "Play Classic Levels" is "setup"
{
    menu = spawn("Simple Menu Builder")
           .addOption("yes", "$PLAYCLASSICLEVELS_YES", Vector2.zero)
           .addOption("no",  "$PLAYCLASSICLEVELS_NO",  Vector2.down.scaledBy(18))
           .setIcon("End of Demo - Pointer")
           .setFontName("End of Demo - Text")
           .setAlignment("center")
           .setHighlightColor("ffffff")
           .setPosition(Vector2(Screen.width / 2, 160))
           .build();

    text = spawn("Play Classic Levels - Text")
           .setText("$PLAYCLASSICLEVELS_TEXT")
           .setPosition(Vector2(Screen.width / 2, 128));

    fader = spawn("Fader");
    cutscene = spawn("Play Classic Levels - Cutscene");

    state "main"
    {
    }

    state "load next level"
    {
        if(timeout(fader.fadeTime))
            Level.loadNext();
    }

    state "abort quest"
    {
        if(timeout(fader.fadeTime))
            Level.abort();
    }

    fun onChooseMenuOption(optionId)
    {
        fader.fadeOut();

        if(optionId == "yes")
            state = "load next level";
        else
            state = "abort quest";
    }

    fun onHighlightMenuOption(optionId)
    {
        cutscene.advance();
    }

    fun constructor()
    {
        fader.fadeTime = 0.5;
        fader.fadeIn();
    }
}

object "Play Classic Levels - Text" is "private", "detached", "entity"
{
    transform = Transform();
    label = Text("End of Demo - Text");

    fun setPosition(position)
    {
        transform.position = position;
        return this;
    }

    fun setText(text)
    {
        label.text = text;
        return this;
    }

    fun constructor()
    {
        label.align = "center";
        label.maxWidth = Screen.width - 16;
    }
}


/*
 *
 * Cutscene
 *
 */

object "Play Classic Levels - Cutscene"
{
    surge = Level.spawnEntity("Play Classic Levels - Surge", Vector2(295, 215));
    neon = Level.spawnEntity("Play Classic Levels - Neon", Vector2(320, 215));
    sfx = Sound("samples/secret.wav");
    mustAdvance = false;
    counter = 0;
    step = 1;

    state "main"
    {
        if(!mustAdvance)
            return;

        mustAdvance = false;

        if(++counter % 30 == 0) {
            surge.advance();
            neon.advance();

            if(++step == 4)
                state = "cheating";
        }

        neon.disturb();
    }

    state "cheating"
    {
        if(timeout(1.0)) {
            sfx.play();
            state = "done";
        }
    }

    state "done"
    {
    }

    fun advance()
    {
        mustAdvance = true;
    }
}

object "Play Classic Levels - Zzz..." is "private", "detached", "entity"
{
    actor = Actor("Play Classic Levels - Zzz...");
    directionalMovement = DirectionalMovement();
    circularMovement = CircularMovement();

    state "main"
    {
        if(timeout(1.0))
            destroy();
    }

    fun constructor()
    {
        directionalMovement.direction = Vector2.up;
        directionalMovement.speed = 16;

        circularMovement.scale = Vector2(1,0);
        circularMovement.radius = 4;
        circularMovement.rate = 0.5;
        circularMovement.phaseOffset = Math.random() * 360;
    }
}

object "Play Classic Levels - Neon" is "private", "detached", "entity"
{
    actor = Actor("Play Classic Levels - Neon");
    transform = Transform();
    mustAdvance = false;

    state "main"
    {
        actor.anim = 0;
        state = "snoring";
    }

    state "sleeping"
    {
        actor.anim = 0;

        if(mustAdvance) {
            mustAdvance = false;
            state = "eyes open";
        }
        else if(timeout(0.5))
            state = "snoring";
    }

    state "snoring"
    {
        position = transform.position.plus(actor.actionOffset);
        zzz = Level.spawnEntity("Play Classic Levels - Zzz...", position);

        state = "sleeping";
    }

    state "disturbed"
    {
        actor.anim = 4;

        if(timeout(1.0))
            state = "sleeping";
    }

    state "eyes open"
    {
        actor.anim = 1;

        if(mustAdvance) {
            mustAdvance = false;
            state = "sitting";
        }
    }

    state "sitting"
    {
        actor.anim = 2;

        if(mustAdvance) {
            mustAdvance = false;
            state = "awake";
        }
    }

    state "awake"
    {
        actor.anim = 3;
    }

    fun advance()
    {
        mustAdvance = true;
    }

    fun disturb()
    {
        if(state == "sleeping")
            state = "disturbed";
    }

    fun constructor()
    {
        actor.zindex = 0.5;
        actor.hflip = true;
    }
}

object "Play Classic Levels - Surge" is "private", "detached", "entity"
{
    actor = Actor("Play Classic Levels - Surge");
    mustAdvance = false;

    state "main"
    {
        state = "waiting";
    }

    state "waiting"
    {
        actor.anim = 0;

        if(mustAdvance) {
            mustAdvance = false;
            state = "arms crossed";
        }
    }

    state "arms crossed"
    {
        actor.anim = 1;

        if(mustAdvance) {
            mustAdvance = false;
            state = "get up";
        }
    }

    state "get up"
    {
        actor.anim = 2;

        if(mustAdvance) {
            mustAdvance = false;
            state = "end";
        }
    }

    state "end"
    {
        actor.anim = 3;
    }

    fun advance()
    {
        mustAdvance = true;
    }

    fun constructor()
    {
        actor.zindex = 0.51;
    }
}