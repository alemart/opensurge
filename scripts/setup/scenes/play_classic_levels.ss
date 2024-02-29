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
using SurgeTheRabbit;
using SurgeTheRabbit.Settings;


object "Play Classic Levels" is "setup"
{
    menu = spawn("Simple Menu Builder")
           .addOption("yes", "$PLAYCLASSICLEVELS_YES", Vector2.zero)
           .addOption("no",  "$PLAYCLASSICLEVELS_NO",  Vector2.down.scaledBy(18))
           .setBackOption("no")
           .setIcon("Play Classic Levels - Pointer")
           .setFontName("End of Demo - Text")
           .setAlignment("center")
           .setHighlightColor("ffffff")
           .setPosition(Vector2(Screen.width / 2, 160))
           .build();

    text = spawn("Play Classic Levels - Text")
           .setText("$PLAYCLASSICLEVELS_TEXT")
           .setPosition(Vector2(Screen.width / 2, 128));

    fader = spawn("Fader");
    interactiveAnimation = spawn("Play Classic Levels - Interactive Animation");

    state "main"
    {
    }

    state "wait before accept"
    {
        if(timeout(1.5)) {
            fader.fadeOut();
            state = "accept";
        }
    }

    state "accept"
    {
        if(timeout(fader.fadeTime)) {
            Level.next += 1;
            Level.load("quests/classic.qst");
        }
    }

    state "decline"
    {
        if(timeout(fader.fadeTime))
            Level.loadNext();
    }

    fun onChooseMenuOption(optionId)
    {
        if(optionId == "yes") {
            interactiveAnimation.finish();
            state = "wait before accept";
        }
        else {
            fader.fadeOut();
            state = "decline";
        }
    }

    fun onHighlightMenuOption(optionId)
    {
        interactiveAnimation.advance();
    }

    fun constructor()
    {
        fader.fadeTime = 0.5;
        fader.fadeIn();

        Settings.wantNeonAsPlayer2 = false;
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
 * Interactive Animation
 *
 */

object "Play Classic Levels - Interactive Animation"
{
    surge = Level.spawnEntity("Play Classic Levels - Surge", Vector2(275, 215));
    neon = Level.spawnEntity("Play Classic Levels - Neon", Vector2(300, 215));
    sfx = Sound("samples/secret.wav");
    mustAdvance = false;
    isFinished = false;
    counter = 0;
    step = 1;

    state "main"
    {
        if(isFinished) {
            surge.walk();
            state = "finished";
            return;
        }

        if(!mustAdvance)
            return;

        mustAdvance = false;

        if(++counter % 10 == 0) {
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
            Settings.wantNeonAsPlayer2 = true;
            sfx.play();
            state = "cheat ready";
        }
    }

    state "cheat ready"
    {
        if(isFinished) {
            surge.walk();
            neon.walk();

            state = "finished";
        }
    }

    state "finished"
    {
    }

    fun advance()
    {
        mustAdvance = true;
    }

    fun finish()
    {
        isFinished = true;
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
    directionalMovement = DirectionalMovement();
    transform = Transform();
    mustAdvance = false;
    isSleeping = true;

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
        actor.anim = 5;

        if(mustAdvance) {
            mustAdvance = false;
            state = "eyes open";
        }
        else if(timeout(1.0))
            state = "sleeping";
    }

    state "eyes open" // sort of open ;)
    {
        actor.anim = 1;
        isSleeping = false;

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

    state "walking"
    {
        if(!timeout(0.5))
            return;

        actor.anim = 4;
        actor.hflip = false;
        directionalMovement.enabled = true;
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

    fun walk()
    {
        if(!isSleeping)
            state = "walking";
    }

    fun constructor()
    {
        actor.zindex = 0.5;
        actor.hflip = true;

        directionalMovement.enabled = false;
        directionalMovement.direction = Vector2.right;
        directionalMovement.speed = 100;
    }
}

object "Play Classic Levels - Surge" is "private", "detached", "entity"
{
    actor = Actor("Play Classic Levels - Surge");
    directionalMovement = DirectionalMovement();
    mustAdvance = false;

    state "main"
    {
        actor.anim = 0;
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
            state = "displeased";
        }
    }

    state "displeased"
    {
        actor.anim = 3;
    }

    state "walking"
    {
        actor.anim = 4;
        directionalMovement.enabled = true;
    }

    fun advance()
    {
        mustAdvance = true;
    }

    fun walk()
    {
        state = "walking";
    }

    fun constructor()
    {
        actor.zindex = 0.51;

        directionalMovement.enabled = false;
        directionalMovement.direction = Vector2.right;
        directionalMovement.speed = 100;
    }
}
