// -----------------------------------------------------------------------------
// File: follow_the_leader_ai.ss
// Description: Artificial Intelligence for CPU-controlled characters
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Camera;
using SurgeEngine.Video.Screen;
using SurgeEngine.Collisions.Sensor;

/*
 * In order to have your character(s) controlled by Artificial Intelligence (AI),
 * add this object to the companion list of the .chr file(s).
 */
object "Follow the Leader AI" is "companion", "private", "awake", "entity"
{
    /*
     * This public property controls whether or not you want the AI to be enabled
     * at any given time.
     */
    public enabled = true;

    /*
     * Whether or not the follower should be focusable. Defaults to false.
     */
    public focusable = false;

    /*
     * When the follower stays away from the leader for a while, it is repositioned.
     * Repositioning may be done in many ways: flying, jumping, teleporting, and
     * whatever else you can imagine. I call each of these a repositioning method.
     *
     * A repositioning method specifies HOW the follower will be repositioned. You
     * can use existing repositioning methods or you can create your own!
     *
     * The repositioning code is kept separate from the Follow the Leader AI script.
     * You should NOT modify the Follow the Leader AI script in order to change the
     * repositioning method. You just change its public "reposition" property from
     * a different script (typically from a player companion).
     *
     * I included different repositioning methods at repositioning_methods.ss as
     * examples. Study them!
     */
    public repositioningMethod = spawn("Default Repositioning Method");

    chardist = 32;
    xfarmargin = 64;
    xnearmargin = 4;
    xmargin = 48;
    ymargin = 32;
    xpullmargin = 64;
    pullSpeed = 60.0; // px/s

    follower = parent;
    leader = parent;
    deltaIndex = 0;

    maxOffscreenTime = 5.0; // seconds
    offscreenTime = 0.0;
    roiMargin = 256;
    lockedPosition = null;
    canLock = true;

    bufferedInput = spawn("Follow the Leader AI - Input Buffer");
    bufferedState = spawn("Follow the Leader AI - State Buffer");
    jumper = spawn("Follow the Leader AI - Jumper").setPlayer(follower);
    platformSensor = spawn("Follow the Leader AI - Platform Sensor");

    state "main"
    {
        // check if enabled
        if(!enabled) {
            state = "disabled";
            return;
        }

        // check if the leader has changed
        if(!leader.hasFocus()) {
            leader = Player.active;
            bufferedInput.fill(leader.input);
            bufferedState.fill(leader);
            deltaIndex = findDeltaIndex();
        }

        // no need to do anything if I am the leader
        if(follower == leader) {
            setPlayer1Flags(follower);
            return;
        }

        // followers have special flags
        setPlayer2Flags(follower);

        // buffer the state of the leader
        bufferedState.store(leader);

        // imitate with a delay the input of the leader
        bufferedInput.store(leader.input);
        bufferedInput.load(follower.input);

        // no need to do anything else if the physics system is disabled
        if(follower.frozen)
            return;

        // lose a shield when hit
        // (doesn't happen by itself when the player is invulnerable)
        if(follower.hit && follower.invulnerable)
            follower.shield = null;

        // modify the jump
        if(bufferedInput.jump) {
            if(!follower.midair)
                jump(); // let's hold the jump button for a little while
            else
                follower.input.simulateButton("fire1", false); // cancel
        }

        // the follower is either charging or probably stuck on a ramp
        if(stuckOnARamp() || bufferedState.charging) {
            // we'll charge and release
            state = "prepare charging"; // if the follower has no charging ability, this looks weird
            return;
        }

        // the follower is pushing a wall, but the leader isn't
        if(follower.pushing && !bufferedState.pushing) {
            // make sure that the direction of the follower is towards the player
            dx = leader.transform.position.x - follower.transform.position.x;
            if(follower.direction * dx >= 0 && Math.abs(dx) > 16) {
                // if the leader isn't pushing a wall as well,
                // this is a sign that the follower may be stuck
                jump(); // get unstuck
            }
        }

        // follow the leader horizontally
        if(!followHorizontally())
            return;

        // follow the leader vertically
        followVertically();
    }

    state "prepare charging"
    {
        // check if enabled
        if(!enabled) {
            state = "disabled";
            return;
        }

        // buffer the state of the leader
        bufferedState.store(leader);

        // ensure that all input is stopped
        // not really needed*, because the input is cleared automatically each frame
        // * unless some other script is changing this input object, which should not
        //   be the case as it could interfere with this
        follower.input.simulateButton("up", false);
        follower.input.simulateButton("right", false);
        follower.input.simulateButton("down", false);
        follower.input.simulateButton("left", false);
        follower.input.simulateButton("fire1", false);
        //follower.input.enabled = false;

        // wait for the follower to stop
        if(!follower.midair && Math.abs(follower.gsp) < 1) {
            follower.gsp = 0;
            state = "adjust direction";
            return;
        }

        // the follower is taking too long to stop
        if(timeout(2.0))
            state = "main"; // cancel the charging
    }

    state "adjust direction"
    {
        // pick a direction
        desiredDirection = leader.charging ? leader.direction :
                           Math.sign(leader.transform.position.x - follower.transform.position.x);

        // adjust the direction of the follower
        if(follower.direction != desiredDirection) {
            if(desiredDirection > 0)
                follower.input.simulateButton("right", true);
            else
                follower.input.simulateButton("left", true);
        }
        else
            state = "ducking";
    }

    state "ducking"
    {
        follower.gsp = 0;
        follower.input.simulateButton("down", true);
        state = "charging";
    }

    state "charging"
    {
        // check if enabled
        if(!enabled) {
            state = "disabled";
            return;
        }

        // buffer the state of the leader
        bufferedState.store(leader);

        // hold down + jump
        follower.input.simulateButton("down", true);
        follower.input.simulateButton("fire1", true);

        // keep pressing jump (nice effect)
        if(follower.charging) {
            pressed = Math.random() < 0.1 && Math.sin(6.2832 * Time.time) > 0;
            follower.input.simulateButton("fire1", pressed);
        }

        // release
        if(timeout(0.5) && !bufferedState.charging) {
            // let go of the controls
            follower.input.simulateButton("down", false);
            follower.input.simulateButton("fire1", false);

            // if the leader was charging, the follower may jump if we do not clear the buffer
            bufferedInput.clear();

            // go to the boosting state
            state = "boosting";
        }

        // note: if the follower doesn't have the charging ability,
        // this does nothing
    }

    state "boosting"
    {
        // go back to the main state
        if(
            leader.gsp * follower.gsp < 0 ||
            !leader.rolling || !follower.rolling ||
            leader.midair || follower.midair ||
            leader.frozen || follower.frozen
        ) {
            state = "main";
            return;
        }

        // artificially change the gsp to make things look nicer
        follower.gsp = leader.gsp;
    }

    state "disabled"
    {
        if(enabled)
            state = "main";
    }

    state "repositioning"
    {
        // we wait until finish() is called by the repositioning method
    }

    fun get_repositioning()
    {
        return state == "repositioning";
    }

    fun setPlayer2Flags(player)
    {
        player.immortal = true;
        player.invulnerable = true;
        player.secondary = true;
        player.focusable = focusable && !this.repositioning;
    }

    fun setPlayer1Flags(player)
    {
        player.immortal = false;
        player.invulnerable = false;
        player.secondary = false;
        player.focusable = true;
    }

    fun followHorizontally()
    {
        followerPosition = follower.transform.position;
        leaderPosition = bufferedState.position;

        // skip if the leader is in ceiling mode (135,225) or close to it (90,135] U [225,270)
        if(bufferedState.slope > 90 && bufferedState.slope < 270)
            return false;

        // if the leader is pushing a wall, let's push together!
        if(bufferedState.pushing) {
            if(bufferedState.direction > 0)
                moveRight();
            else
                moveLeft();

            return true;
        }

        // compute the x-position of the leader with an offset
        xtarget = leaderPosition.x; xnear = 0;
        if(Math.abs(leader.gsp) < 240) {
            for(d = chardist; d >= 4; d /= 2) {
                baseOffset = d * deltaIndex;
                xoffset = baseOffset * -bufferedState.direction;
                xoffsetB = baseOffset * -leader.direction;

                // this offset is undesirable if the leader is standing on a small platform
                if(leader.slope != 0 || (
                    platformSensor.hitTest(leader.collider.center.x + xoffsetB, leader.collider.bottom + 8, follower.layer) &&
                    platformSensor.hitTest(leader.collider.center.x - xoffsetB, leader.collider.bottom + 8, follower.layer)
                )) {
                    xtarget += xoffset;
                    xnear = leader.slope == 0 && d < chardist ? xnearmargin : 0;
                    break;
                }
            }
        }

        // move horizontally
        xdist = xtarget - followerPosition.x;
        if(xdist > xmargin) {
            moveRight();
        }
        else if(xdist < -xmargin) {
            moveLeft();
        }
        else if(leader.direction != follower.direction) {
            if(!leader.midair && leader.gsp * follower.gsp <= 0) {
                if(xdist > xnear)
                    moveRight();
                else if(xdist < -xnear)
                    moveLeft();
            }
        }

        // make it jump if it's going too fast near a ledge
        if(
            !follower.midair && !follower.rolling &&
            follower.slope == 0 && Math.abs(follower.gsp) >= 120 &&
            follower.transform.position.y >= bufferedState.position.y - 32 &&
            leader.transform.position.y <= follower.transform.position.y + 16
        ) {
            dx = follower.collider.right - follower.collider.center.x;
            if(!platformSensor.hitTest(
                follower.collider.center.x + dx * follower.direction,
                follower.collider.bottom + 8,
                follower.layer
            ))
                jump();
        }

        // done
        return true;
    }

    fun followVertically()
    {
        followerPosition = follower.transform.position;
        leaderPosition = bufferedState.position;

        // skip if the follower is too far away (horizontally) from the leader
        if(Math.abs(leaderPosition.x - followerPosition.x) > xfarmargin)
            return false;

        // the leader is above the follower by a margin
        if(leaderPosition.y < followerPosition.y - ymargin) {
            if(!follower.midair && Math.abs(follower.gsp) < 240) {
                if(leader.slope < 90 || leader.slope > 270) {
                    // let's make the follower jump
                    if(Math.random() < 0.03)
                        jump();
                }
            }
        }

        // done
        return true;
    }

    fun moveRight()
    {
        // press the appropriate buttons on the controller
        follower.input.simulateButton("right", true);
        follower.input.simulateButton("left", false);

        // pull the follower towards the leader in order to make the movement
        // more robust when it comes to small platforms (and generally also)
        dx = bufferedState.position.x - follower.transform.position.x;
        if((follower.jumping || Math.abs(dx) > xpullmargin) && follower.direction > 0 && follower.xsp > 15 && !follower.pushing)
            follower.transform.translateBy(pullSpeed * Time.delta, 0);
    }

    fun moveLeft()
    {
        // press the appropriate buttons on the controller
        follower.input.simulateButton("left", true);
        follower.input.simulateButton("right", false);

        // pull the follower towards the leader in order to make the movement
        // more robust when it comes to small platforms (and generally also)
        dx = bufferedState.position.x - follower.transform.position.x;
        if((follower.jumping || Math.abs(dx) > xpullmargin) && follower.direction < 0 && follower.xsp < -15 && !follower.pushing)
            follower.transform.translateBy(-pullSpeed * Time.delta, 0);
    }

    fun jump()
    {
        jumper.jump();
    }

    fun needToReposition()
    {
        // no need to do anything if the follower is being repositioned
        if(state == "repositioning")
            return false;

        // is the follower offscreen? for how long?
        if(isOffscreen(follower, 0))
            offscreenTime += Time.delta;
        else
            offscreenTime = 0.0;

        // reposition if offscreen for too long
        if(offscreenTime >= maxOffscreenTime) {
            if(!follower.dying && !follower.frozen) {
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
        follower.restore(); // undo charging, etc.
        lockedPosition = null;
        canLock = false;

        // give up control to the repositioning method
        finish = spawn("Follow the Leader AI - Finish Repositioning Functor");
        assert(typeof repositioningMethod == "object" && repositioningMethod.hasTag("player-repositioning-method"));
        repositioningMethod.call(finish, follower, leader, this);
    }

    fun finishRepositioning()
    {
        // validate
        if(state != "repositioning")
            return;

        // fill the buffers with the current state
        bufferedInput.fill(leader.input);
        bufferedState.fill(leader);

        // ensure a valid state
        follower.frozen = false;
        follower.input.enabled = true;
        follower.layer = leader.layer;

        // run the AI
        state = "main";
    }

    fun lockIfOffscreen()
    {
        // if the follower is too offscreen, we'll lock it to its position,
        // so that it doesn't die unexpectedly
        if(state == "repositioning")
            return;

        if(!canLock) {
            lockedPosition = null;
            if(!isOffscreen(follower, 0))
                canLock = true;
        }
        else if(lockedPosition === null) {
            if(isOffscreen(follower, roiMargin))
                lockedPosition = follower.transform.position.scaledBy(1);
        }
        else {
            follower.transform.position = lockedPosition;
            if(!isOffscreen(follower, roiMargin))
                lockedPosition = null;
        }
    }

    fun stuckOnARamp()
    {
        // check if stuck on a ramp or loop
        return !follower.midair && Math.abs(follower.gsp) < 7.5 &&
               (follower.hlockTime > 0.0 || Math.abs(Math.cos(Math.deg2rad(follower.slope)) - 0.707) < 0.16) &&
               Math.abs(follower.transform.position.x - leader.transform.position.x) > xnearmargin;
    }

    fun findDeltaIndex()
    {
        followerIndex = 0;
        leaderIndex = 0;

        n = Player.count;
        for(i = 0; i < n; i++) {
            player = Player[i];

            if(player == follower)
                followerIndex = i;

            if(player == leader)
                leaderIndex = i;
        }

        // find min x >= 0 such that (leaderIndex + x) mod n = followerIndex
        return ((followerIndex - leaderIndex) + n) % n;
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

    fun lateUpdate()
    {
        // do nothing if the AI is disabled
        if(!enabled)
            return;

        // do nothing if I'm the leader at this time
        if(follower == leader)
            return;

        // do nothing while repositioning
        if(state == "repositioning")
            return;

        // reposition the follower
        if(needToReposition()) {
            startRepositioning();
            return;
        }

        // lock if offscreen
        lockIfOffscreen();
    }

    fun constructor()
    {
        assert(follower.hasTag("player"));
    }
}

object "Follow the Leader AI - Finish Repositioning Functor"
{
    ai = parent;

    fun call()
    {
        ai.finishRepositioning();
    }
}

object "Follow the Leader AI - Jumper"
{
    ai = parent;
    player = null;

    state "main"
    {
    }

    // hold the jump button for a little while to ensure that the jump isn't cut short
    state "jumping"
    {
        if(timeout(0.5) || player.frozen || !player.input.enabled || !ai.enabled) {
            player.input.simulateButton("fire1", false);
            state = "main";
            return;
        }

        player.input.simulateButton("fire1", true);
    }

    fun jump()
    {
        player.input.simulateButton("fire1", true);
        state = "jumping";
    }

    fun setPlayer(p)
    {
        player = p;
        return this;
    }
}

object "Follow the Leader AI - Platform Sensor" is "private", "awake", "entity"
{
    transform = Transform();
    sensor = Sensor(0, 0, 1, 1);

    fun hitTest(xpos, ypos, layer)
    {
        currentPosition = transform.position;
        transform.translateBy(xpos - currentPosition.x, ypos - currentPosition.y);

        sensor.layer = layer;
        sensor.onTransformChange();

        return sensor.status !== null;
    }
}

object "Follow the Leader AI - Input Buffer"
{
    maxSamples = 15; // target fps / 60
    upBuffer = [];
    rightBuffer = [];
    downBuffer = [];
    leftBuffer = [];
    jumpBuffer = [];
    head = 0;
    tail = 1;
    initialized = false;

    fun get_up()
    {
        return upBuffer[tail];
    }

    fun get_right()
    {
        return rightBuffer[tail];
    }

    fun get_down()
    {
        return downBuffer[tail];
    }

    fun get_left()
    {
        return leftBuffer[tail];
    }

    fun get_jump()
    {
        return jumpBuffer[tail];
    }

    fun load(input)
    {
        input.simulateButton("up", this.up);
        input.simulateButton("right", this.right);
        input.simulateButton("down", this.down);
        input.simulateButton("left", this.left);
        input.simulateButton("fire1", this.jump);
    }

    fun store(input)
    {
        if(!initialized) {
            fill(input);
            initialized = true;
            return;
        }

        head = (head + 1) % maxSamples;
        upBuffer[head] = input.buttonDown("up");
        rightBuffer[head] = input.buttonDown("right");
        downBuffer[head] = input.buttonDown("down");
        leftBuffer[head] = input.buttonDown("left");
        jumpBuffer[head] = input.buttonPressed("fire1");

        tail = (tail + 1) % maxSamples;
    }

    fun fill(input)
    {
        up = input.buttonDown("up");
        right = input.buttonDown("right");
        down = input.buttonDown("down");
        left = input.buttonDown("left");
        jump = input.buttonDown("fire1");

        for(i = 0; i < maxSamples; i++) {
            upBuffer[i] = up;
            rightBuffer[i] = right;
            downBuffer[i] = down;
            leftBuffer[i] = left;
            jumpBuffer[i] = jump;
        }
    }

    fun clear()
    {
        for(i = 0; i < maxSamples; i++) {
            upBuffer[i] = false;
            rightBuffer[i] = false;
            downBuffer[i] = false;
            leftBuffer[i] = false;
            jumpBuffer[i] = false;
        }
    }

    fun constructor()
    {
        assert(maxSamples > 1);

        for(i = 0; i < maxSamples; i++) {
            upBuffer.push(false);
            rightBuffer.push(false);
            downBuffer.push(false);
            leftBuffer.push(false);
            jumpBuffer.push(false);
        }
    }
}

object "Follow the Leader AI - State Buffer"
{
    maxSamples = 15; // target fps / 60
    xposBuffer = [];
    yposBuffer = [];
    directionBuffer = [];
    pushingBuffer = [];
    chargingBuffer = [];
    jumpingBuffer = [];
    midairBuffer = [];
    slopeBuffer = [];
    head = 0;
    tail = 1;
    initialized = false;

    fun get_position()
    {
        xpos = xposBuffer[tail];
        ypos = yposBuffer[tail];

        return Vector2(xpos, ypos);
    }

    fun get_direction()
    {
        return directionBuffer[tail];
    }

    fun get_pushing()
    {
        return pushingBuffer[tail];
    }

    fun get_charging()
    {
        //return chargingBuffer[tail];

        // the following adjustment makes things look nicer
        n = maxSamples;
        dist = Math.floor(n / 3);
        adjustedTail = (tail + (n - dist)) % n;

        return chargingBuffer[adjustedTail];
    }

    fun get_jumping()
    {
        return jumpingBuffer[tail];
    }

    fun get_midair()
    {
        return midairBuffer[tail];
    }

    fun get_slope()
    {
        return slopeBuffer[tail];
    }

    fun load(player)
    {
        ;
    }

    fun store(player)
    {
        if(!initialized) {
            fill(player);
            initialized = true;
            return;
        }

        if(!wantToBuffer(player))
            return;

        head = (head + 1) % maxSamples;
        xposBuffer[head] = player.transform.position.x;
        yposBuffer[head] = player.transform.position.y;
        directionBuffer[head] = player.direction;
        pushingBuffer[head] = player.pushing;
        chargingBuffer[head] = player.charging;
        jumpingBuffer[head] = player.jumping;
        midairBuffer[head] = player.midair;
        slopeBuffer[head] = player.slope;

        tail = (tail + 1) % maxSamples;
    }

    fun fill(player)
    {
        xpos = player.transform.position.x;
        ypos = player.transform.position.y;
        direction = player.direction;
        pushing = false;
        charging = false;
        jumping = false;
        midair = false;
        slope = 0;

        for(i = 0; i < maxSamples; i++) {
            xposBuffer[i] = xpos;
            yposBuffer[i] = ypos;
            directionBuffer[i] = direction;
            pushingBuffer[i] = pushing;
            chargingBuffer[i] = charging;
            jumpingBuffer[i] = jumping;
            midairBuffer[i] = midair;
            slopeBuffer[i] = slope;
        }
    }

    fun wantToBuffer(player)
    {
        return !player.dying && (!player.frozen || player.rolling /* tubes */);
    }

    fun constructor()
    {
        assert(maxSamples > 1);

        for(i = 0; i < maxSamples; i++) {
            xposBuffer.push(0);
            yposBuffer.push(0);
            directionBuffer.push(1);
            pushingBuffer.push(false);
            chargingBuffer.push(false);
            jumpingBuffer.push(false);
            midairBuffer.push(false);
        }
    }
}