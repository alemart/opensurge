// -----------------------------------------------------------------------------
// File: player2.ss
// Description: companion for characters controlled by a human Player 2
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Input;
using SurgeEngine.Camera;
using SurgeEngine.Vector2;
using SurgeEngine.Video.Screen;

/*
 * In order to have your character(s) controlled by a human Player 2, add this
 * object to the companion list of the .chr file(s).
 *
 * Note: this object should be added as the first companion of the list, as it
 * modifies player.input.
 */
object "Player 2" is "companion"
{
    /*
     * This public property controls whether or not you want the human Player 2
     * controls enabled at any given time.
     */
    public enabled = true;

    /*
     * Whether or not the player should be focusable. Defaults to false.
     */
    public focusable = false;

    /*
     * When the human Player 2 stays offscreen for a while, it is repositioned.
     * Repositioning may be done in many ways: flying, jumping, teleporting, and
     * whatever else you can imagine. I call each of these a repositioning method.
     *
     * A repositioning method specifies HOW the Player 2 will be repositioned. You
     * can use existing repositioning methods or you can create your own!
     *
     * The repositioning code is kept separate from the Player 2 script. You should
     * NOT modify the Player 2 script in order to change the repositioning method.
     * You just change its public "reposition" property from a different script
     * (typically from a player companion).
     *
     * I included different repositioning methods at repositioning_methods.ss as
     * examples. Study them!
     */
    public repositioningMethod = spawn("Default Repositioning Method");

    player = parent;
    secondController = Input("secondary");
    ai = null;

    maxIdleTime = 10.0; // seconds
    idleTime = maxIdleTime;
    isIdle = true;

    maxOffscreenTime = 5.0; // seconds
    offscreenTime = 0.0;
    roiMargin = 256;
    lockedPosition = null;
    canLock = true;

    bgx = Level.child("Background Exchange Manager") ||
          Level.spawn("Background Exchange Manager");

    state "main"
    {
        // initialization
        ai = player.child("Follow the Leader AI");
        state = "active";
    }

    state "active"
    {
        // do nothing if disabled
        if(!enabled)
            return;

        // do nothing if the player is effectively Player 1 at this time
        if(player.hasFocus()) {
            setPlayer1Flags(player);
            return;
        }

        // Player 2 has special flags
        setPlayer2Flags(player);

        // lose a shield when hit
        // (doesn't happen by itself when the player is invulnerable)
        if(player.hit && player.invulnerable)
            player.shield = null;

        // do we need to reposition the player?
        if(needToReposition()) {
            startRepositioning();
            return;
        }
        lockIfOffscreen();

        // redirect user input
        if(readAndRedirectInput())
            idleTime = 0.0;
        else
            idleTime += Time.delta;

        // idle logic
        if(!isIdle) {
            // if we're idle for too long, enable the AI
            if(idleTime >= maxIdleTime) {
                if(enableAI(true))
                    isIdle = true;
            }
        }
        else {
            // we just received user input. We're no longer idle. Disable the AI
            if(idleTime == 0.0) {
                if(enableAI(false))
                    isIdle = false;
            }
        }
    }

    state "repositioning"
    {
        // we wait until finish() is called by the repositioning method
    }

    fun get_repositioning()
    {
        return state == "repositioning";
    }

    fun readAndRedirectInput()
    {
        /*

        A player that is not focused (i.e., a player that is not Player.active)
        does not respond to direct user input. However, it does respond to
        simulated input. So let's redirect the input of the second controller
        to the input object of Player 2 via simulateButton().

        */

        up = secondController.buttonDown("up");
        right = secondController.buttonDown("right");
        down = secondController.buttonDown("down");
        left = secondController.buttonDown("left");
        fire1 = secondController.buttonDown("fire1");
        fire2 = secondController.buttonDown("fire2");
        fire3 = secondController.buttonDown("fire3");
        fire4 = secondController.buttonDown("fire4");
        fire5 = secondController.buttonDown("fire5");
        fire6 = secondController.buttonDown("fire6");
        fire7 = secondController.buttonDown("fire7");
        fire8 = secondController.buttonDown("fire8");

        if(up)
            player.input.simulateButton("up", true);

        if(right)
            player.input.simulateButton("right", true);

        if(down)
            player.input.simulateButton("down", true);

        if(left)
            player.input.simulateButton("left", true);

        if(fire1)
            player.input.simulateButton("fire1", true);

        if(fire2)
            player.input.simulateButton("fire2", true);

        if(fire3)
            player.input.simulateButton("fire3", true);

        if(fire4)
            player.input.simulateButton("fire4", true);

        if(fire5)
            player.input.simulateButton("fire5", true);

        if(fire6)
            player.input.simulateButton("fire6", true);

        if(fire7)
            player.input.simulateButton("fire7", true);

        if(fire8)
            player.input.simulateButton("fire8", true);

        return (
            up || right || down || left ||
            fire1 || fire2 || fire3 || fire4 ||
            fire5 || fire6 || fire7 || fire8
        );
    }

    fun enableAI(enabled)
    {
        if(ai === null || ai.repositioning)
            return false;

        ai.enabled = enabled;
        return true;
    }

    fun setPlayer2Flags(player)
    {
        player.immortal = true;
        player.invulnerable = true;
        player.secondary = false; // unlike an AI-controlled Player 2
        player.focusable = focusable;
    }

    fun setPlayer1Flags(player)
    {
        player.immortal = false;
        player.invulnerable = false;
        player.secondary = false;
        player.focusable = true;
    }

    fun isOffscreen(player, extraMargin)
    {
        collider = player.collider;
        xmargin = collider.width / 2;
        ymargin = collider.height / 2;

        topleft = Vector2(collider.left - xmargin, collider.top - ymargin);
        topleft = Camera.worldToScreen(topleft);

        bottomright = Vector2(collider.right + xmargin, collider.bottom + ymargin);
        bottomright = Camera.worldToScreen(bottomright);

        top = topleft.y;
        left = topleft.x;
        bottom = bottomright.y;
        right = bottomright.x;

        return right < -extraMargin || left > Screen.width + extraMargin || bottom < -extraMargin || top > Screen.height + extraMargin;
    }

    fun needToReposition()
    {
        // no need to do anything if the player is being repositioned
        if(state == "repositioning")
            return false;

        // is the player offscreen? for how long?
        if(isOffscreen(player, 0))
            offscreenTime += Time.delta;
        else
            offscreenTime = 0.0;

        // reposition if offscreen for too long
        if(offscreenTime >= maxOffscreenTime) {
            if(!player.dying && !player.frozen) {
                offscreenTime = 0.0;
                return true;
            }
        }

        // no need to reposition
        return false;
    }

    fun startRepositioning()
    {
        // validate
        if(state == "repositioning")
            return;

        // change the state
        state = "repositioning";
        player.restore(); // undo charging, etc.
        lockedPosition = null;
        canLock = false;

        // give up control to the repositioning method
        follower = player;
        leader = Player.active;
        finish = spawn("Player 2 - Finish Repositioning Functor");
        assert(typeof repositioningMethod == "object" && repositioningMethod.hasTag("player-repositioning-method"));
        repositioningMethod.call(finish, follower, leader, this);
    }

    fun finishRepositioning()
    {
        // validate
        if(state != "repositioning")
            return;

        // ensure a valid state
        player.frozen = false;
        player.input.enabled = true;
        player.layer = Player.active.layer;

        // go back to the main loop
        state = "active";
    }

    fun lockIfOffscreen()
    {
        // if the player is too offscreen, we'll lock it to its position,
        // so that it doesn't die unexpectedly
        if(state == "repositioning")
            return;

        if(!canLock) {
            lockedPosition = null;
            if(!isOffscreen(player, 0))
                canLock = true;
        }
        else if(lockedPosition === null) {
            if(isOffscreen(player, roiMargin))
                lockedPosition = player.transform.position.scaledBy(1);
        }
        else {
            player.transform.position = lockedPosition;
            if(!isOffscreen(player, roiMargin))
                lockedPosition = null;
        }
    }

    fun updateBackground()
    {
        leader = Player.active;
        assert(leader !== player);

        distance = player.transform.position.distanceTo(leader.transform.position);

        if(state == "repositioning" || distance < 64) {
            backgroundOfLeader = bgx.backgroundOfPlayer(leader);
            bgx.changeBackgroundOfPlayer(player, backgroundOfLeader);
        }
    }

    fun lateUpdate()
    {
        // do nothing if disabled
        if(!enabled)
            return;

        // do nothing if the player is effectively Player 1 at this time
        if(player.hasFocus())
            return;

        // background awareness
        updateBackground();
    }

    fun constructor()
    {
        assert(player.hasTag("player"));
    }
}

object "Player 2 - Finish Repositioning Functor"
{
    player2 = parent;

    fun call()
    {
        player2.finishRepositioning();
    }
}