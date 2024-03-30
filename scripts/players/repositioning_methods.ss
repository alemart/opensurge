// -----------------------------------------------------------------------------
// File: repositioning_methods.ss
// Description: repositioning methods for characters
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Camera;
using SurgeEngine.Collisions.Sensor;
using SurgeEngine.Video.Screen;
using SurgeEngine.Events.Event;

/*
 * Repositioning methods are used with characters that are not controlled by
 * Player 1 (e.g., a CPU-controlled character that takes the role of Player 2).
 * When these characters stay offscreen for a little while, they are repositioned
 * (respawned) using a repositioning method.
 *
 * Repositioning methods may be anything you can imagine: teleporting, flying,
 * jumping, you name it. I included a few methods as examples. You can create
 * your own, or you can extend the existing ones by setting the onStart() and
 * the onFinish() events that they trigger.
 *
 * Study the "Follow the Leader AI" script (follow_the_leader_ai.ss) to see an
 * example of how the repositioning methods are used in practice.
 */

/*
 * The Simple Repositioning Method moves the follower to the top of the screen.
 */
object "Simple Repositioning Method" is "player-repositioning-method"
{
    /*
     * The onStart event is triggered as soon as the repositioning method
     * starts functioning.
     */
    public onStart = Event();

    /*
     * The onFinish event is triggered as soon as the repositioning method
     * gives up the control of the player.
     */
    public onFinish = Event();

    // the AI will call this function when it intends to reposition the follower
    fun call(finish, follower, leader, ai)
    {
        // trigger the onStart event
        onStart();

        // reposition the follower
        follower.transform.position = leader.transform.position.translatedBy(0, -192);
        follower.gsp = follower.xsp = follower.ysp = 0;
        follower.restore();

        // after repositioning the follower, we call finish() in order to give the
        // control back to the AI
        finish();

        // trigger the onFinish event
        onFinish();
    }
}

/*
 * The No Repositioning Method makes the follower stand still. The leader has
 * to keep the follower in sight.
 */
object "No Repositioning Method" is "player-repositioning-method"
{
    /*
     * The onStart event is triggered as soon as the repositioning method
     * starts functioning.
     */
    public onStart = Event();

    /*
     * The onFinish event is triggered as soon as the repositioning method
     * gives up the control of the player.
     */
    public onFinish = Event();

    finish = null;
    follower = null;
    leader = null;

    state "main"
    {
        // wait for the repositioning method to be called
        // -- do nothing --
    }

    state "waiting"
    {
        // wait for the follower to get onscreen
        if(isOffscreen(follower))
            return;

        // the follower is not offscreen. We'll "reposition" it to the same place
        // (i.e., it stands still)
        ;

        // reset and unfreeze the follower
        follower.gsp = 0;
        follower.xsp = 0;
        follower.ysp = 0;
        follower.restore();
        follower.frozen = false;

        // return to the main state
        state = "main";

        // after repositioning the follower, we call finish() in order to give the
        // control back to the AI
        finish();

        // trigger the onFinish event
        onFinish();
    }

    fun isOffscreen(player)
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

        return right < 0 || left > Screen.width || bottom < 0 || top > Screen.height;
    }

    // the AI will call this function when it intends to reposition the follower
    fun call(_finish, _follower, _leader, _ai)
    {
        finish = _finish;
        follower = _follower;
        leader = _leader;

        // freeze the follower
        follower.frozen = true;

        // trigger the onStart event
        onStart();

        // just wait
        state = "waiting";
    }
}

/*
 * The Flying Repositioning Method makes the follower fly towards the leader.
 */
object "Flying Repositioning Method" is "player-repositioning-method"
{
    /*
     * The animation number of the character that corresponds to flying.
     */
    public flyingAnim = 20;

    /*
     * The onStart event is triggered as soon as the repositioning method
     * starts functioning.
     */
    public onStart = Event();

    /*
     * The onFinish event is triggered as soon as the repositioning method
     * gives up the control of the player.
     */
    public onFinish = Event();

    finish = null;
    follower = null;
    leader = null;

    deltaIndex = 0;
    landingDistance = 8;
    teleportOffset = Vector2(0, -192);
    teleportDistance = Math.max(Screen.width, Screen.height) + 32;
    bufferedState = spawn("Follow the Leader AI - State Buffer");

    state "main"
    {
        // wait for the repositioning method to be called
        // -- do nothing --
    }

    state "flying"
    {
        // we'll fly towards the leader

        // find the leader
        // we use Player.active instead of the leader received as a parameter
        // in call() because who the leader is may change during this flight
        leader = Player.active;

        // buffer the state of the leader
        bufferedState.store(leader);

        // read the position of the follower
        followerPosition = follower.transform.position;

        // find the target position, based on the buffered position of the leader
        dx = leader.winning ? 16 * deltaIndex * -bufferedState.direction : 0;
        targetPosition = (dx == 0) ? bufferedState.position : bufferedState.position.translatedBy(dx, 0);

        // do nothing if the leader dies while the follower is being repositioned
        if(leader.dying)
            return;

        // land on the ground
        if(!bufferedState.midair && !leader.dying && !leader.frozen) {
            if(followerPosition.distanceTo(targetPosition) <= landingDistance) {
                land();
                return;
            }
        }

        // teleport if too far away from the CURRENT position of the leader
        if(followerPosition.distanceTo(leader.transform.position) >= teleportDistance) {
            teleport();
            return;
        }

        // compute velocity
        dx = targetPosition.x - followerPosition.x;
        dy = targetPosition.y - followerPosition.y;
        xvel1 = Math.abs(dx * 60.0);
        xvel2 = Math.min(720.0, Math.floor(xvel1 * 0.0625)) + Math.abs(leader.xsp);
        xvel = Math.signum(dx) * Math.min(xvel1, xvel2);
        yvel = Math.signum(dy) * (Math.abs(dy) >= 2 ? 60.0 : 0.0);

        // move
        follower.transform.translateBy(xvel * Time.delta, yvel * Time.delta);

        // animate
        follower.angle = 0;
        follower.anim = flyingAnim;
        follower.hflip = (follower.direction * Math.sign(dx) < 0);
    }

    fun land()
    {
        // stop flying
        state = "main";

        // reset speeds & enable physics
        follower.gsp = 0;
        follower.xsp = 0;
        follower.ysp = 0;
        follower.restore();
        follower.frozen = false;
        follower.hflip = false;

        // give back the control to the AI
        finish();

        // trigger the onFinish event
        onFinish();
    }

    fun teleport()
    {
        follower.transform.position = leader.transform.position.plus(teleportOffset);
        bufferedState.fill(leader);
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

    // the AI will call this function when it intends to reposition the follower
    fun call(_finish, _follower, _leader, _ai)
    {
        finish = _finish;
        follower = _follower;
        leader = _leader;

        // trigger the onStart event
        onStart();

        // prepare the follower and start flying
        follower.frozen = true;
        deltaIndex = findDeltaIndex();
        teleport();
        state = "flying";
    }
}

/*
 * The Default Repositioning Method makes the follower jump from the
 * bottom of the screen. It adds some logic to prevent the follower from
 * getting stuck in a wall.
 */
object "Default Repositioning Method" is "player-repositioning-method", "private", "awake", "entity"
{
    /*
     * The animation number of the character that corresponds to jumping.
     */
    public jumpingAnim = 3;

    /*
     * The onStart event is triggered as soon as the repositioning method
     * starts functioning.
     */
    public onStart = Event();

    /*
     * The onFinish event is triggered as soon as the repositioning method
     * gives up the control of the player.
     */
    public onFinish = Event();

    leader = null;
    follower = null;
    finish = null;

    sensor = Sensor(-10, 0, 20, 1);
    transform = Transform();

    xsp = 0;
    ysp = 0;
    dy = 150;

    state "main"
    {
        // wait for the repositioning method to be called
        // -- do nothing --
    }

    state "waiting"
    {
        // wait and make the follower jump when the leader isn't running too much
        if(!leader.frozen && !leader.midair && !leader.rolling && Math.abs(leader.gsp) < 300) {
            state = "prepare to jump";
        }
    }

    state "prepare to jump"
    {
        follower.restore();
        follower.frozen = true;
        follower.input.enabled = false;
        follower.visible = true;
        follower.angle = 0;
        follower.transform.position = leader.transform.position.translatedBy(0, dy);

        grv = Level.gravity;
        jumpSpeed = -Math.sqrt(2 * grv * (1.5 * dy));

        ysp = jumpSpeed;
        xsp = leader.speed / 2;

        state = "jumping";
    }

    state "jumping"
    {
        transform.position = follower.transform.position;

        // handle jumping
        handleJumping();

        // end of jumping
        if(ysp >= 0) {

            // don't restore the physics unless the sensor is cleared
            // (avoid getting crushed)
            if(sensor.status === null) {
                state = "restore physics";
            }

            // the follower got stuck in a wall
            else {
                state = "falling";
            }

        }
    }

    state "falling"
    {
        // handle jumping
        handleJumping();

        // let's try again
        if(follower.transform.position.y - leader.transform.position.y > dy) {
            follower.visible = false;
            state = "cooldown";
        }
    }

    state "cooldown"
    {
        if(timeout(3.0))
            state = "waiting";
    }

    state "restore physics"
    {
        // restore physics
        follower.gsp = 0;
        follower.xsp = xsp;
        follower.ysp = ysp;
        follower.frozen = false;
        follower.restore();

        // we're done
        sensor.enabled = false;
        state = "main";

        // give the control back to the AI
        finish();

        // trigger the onFinish event
        onFinish();
    }

    fun handleJumping()
    {
        // animation
        follower.anim = jumpingAnim;

        // gravity
        dt = Time.delta;
        grv = Level.gravity;
        ysp += grv * dt;

        // move
        follower.transform.translateBy(xsp * dt, ysp * dt);
    }

    fun constructor()
    {
        sensor.enabled = false;
    }

    // the AI will call this function when it intends to reposition the follower
    fun call(_finish, _follower, _leader, _ai)
    {
        finish = _finish;
        follower = _follower;
        leader = _leader;

        // trigger the onStart event
        onStart();

        // prepare the follower
        follower.restore();
        follower.frozen = true;
        follower.visible = false;
        sensor.enabled = true;
        state = "waiting";
    }
}
