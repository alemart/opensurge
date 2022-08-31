// -----------------------------------------------------------------------------
// File: salamanderboss.ss
// Description: Boss salamander script
// Author: Cody Licorish <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.Platformer;
using SurgeEngine.Behaviors.CircularMovement;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Brick;
using SurgeEngine.Camera;
using SurgeEngine.Collisions.CollisionBall;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Collisions.Sensor;
using SurgeEngine.Events.Event;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Transform;
using SurgeEngine.UI.Text;
using SurgeEngine.Vector2;
using SurgeEngine.Video.Screen;


// SalamanderBoss is the Waterworks boss
object "Salamander Boss" is "entity", "boss", "awake"
{
    public onDefeat = Event();
    public onFirstFall = Event();
    public hp = 5; // health points
    public stabilityDepth = Screen.height * 0.6875;
    public recoverTime = 2;
    public aimTime = 1;
    public boomChargeTime = 0.5;
    public ascendTime = 0.5;
    public battleDelayTime = 3;
    public upwardDriftTime = 3;

    activeWeapon = null;
    transcend = DirectionalMovement();
    nextState = "waiting";
    angryHp = 1;
    explosionTime = 2.0; // seconds
    aimStopwatch = 0.0; // seconds
    actor = Actor("Salamander Boss");
    transform = Transform();
    collider = CollisionBox(70,80);
    rollCollider = CollisionBall(37).setAnchor(0.5, 1.0);
    platformer = Platformer();
    lightningChargeSample = Sound("samples/salamander_charge.ogg");
    ascentSample = Sound("samples/salamander_ascent.ogg");
    hit = Sound("samples/bosshit.wav");
    spinChargeSample = Sound("samples/charge.wav");
    rollSample = Sound("samples/roll.wav");
    jumpSample = Sound("samples/jump.wav");
    lightningSmashSample = Sound("samples/lightning_smash.wav");
    bounceSample = Sound("samples/bumper.wav");
    floorHitSample = Sound("samples/floorhit.wav");
    chaseMargin = 64; // medium !!! (touches the player)
    firstUpFlap = true;
    rollMode = false;
    playerWasHit = false;
    upwardDriftRatio = 0.875;
    restorePlayerEvent = spawn("Salamander Restore Helper");
    wings = spawn("Salamander Boss Wing Anchor");
    dashSmoke = spawn("Salamander Dash Sparks");

    state "main"
    {
        rollCollider.enabled = false;
        // calculate the upwardDriftRatio using the equation:
        // (1 - upwardDriftRatio)*upwardDriftTime*ysp = stabilityDepth*2
        //   where stabilityDepth is related to the height that the boss
        //   during Lightning Boom travels relative to the player.
        maxFallSpeed = 720;
        if (upwardDriftTime > 0.1) {
            upwardDriftRatio = 1 - (stabilityDepth*2)/(upwardDriftTime*maxFallSpeed);
        }
        state = "waiting";
    }
    state "waiting"
    {
        // waiting for fight to start
        actor.animation.id = 6;
        wings.anim = 1;
    }
    state "take off"
    {
        onFirstFall();
        state = "take off delayed";
    }
    state "take off delayed"
    {
        if (timeout(battleDelayTime)) {
            startAscent();
        }
    }
    state "ascend"
    {
        actor.animation.id = 7;
        wings.anim = 2;
        if (timeout(ascendTime)) {
            state = "Lightning Smash";
            lightningSmashSample.play();
            transcend.direction = Vector2.down;
            transcend.speed = 400;
            transcend.enabled = true;
        }
    }
    state "Lightning Smash"
    {
        actor.animation.id = 8;
        wings.anim = 4;
        platformer.gravityMultiplier = 0.0;
    }

    state "falling"
    {
        // actually trying to fall faster than Surge
        actor.animation.id = 8;
        platformer.enabled = true;
        platformer.gravityMultiplier = 1.0;
        player = Player.active;
        if (player.transform.position.y < transform.position.y - stabilityDepth) {
            platformer.gravityMultiplier = 0.0;
            platformer.forceJump(0);
            transcend.direction = Vector2.down;
            transcend.speed = Player.active.ysp;
            transcend.enabled = true;
            state = "aiming";
            aimStopwatch = aimTime;
            if (!playerWasHit) {
                lightningChargeSample.play();
            }
        }
    }
    state "aiming"
    {
        dt = Time.delta;
        playerPosition = Player.active.transform.position;
        transcend.speed = Player.active.ysp;
        xdiff = playerPosition.x - transform.position.x;
        if (aimStopwatch <= 0) {
            platformer.stop();
            state = "air charging";
        }
        else {
            aimStopwatch -= dt;
            if (xdiff < -chaseMargin) {
                platformer.walkLeft();
            }
            else if (xdiff > +chaseMargin) {
                platformer.walkRight();
            }
            else {
                platformer.stop();
            }
        }
    }
    state "air charging"
    {
        lookAtPlayer();
        transcend.speed = Player.active.ysp;
        if (timeout(boomChargeTime)) {
            state = "Lightning Boom";
        }
    }
    state "Lightning Boom"
    {
        lookAtPlayer();
        transcend.speed = Player.active.ysp;
        lightningChargeSample.stop();
        if (!playerWasHit) {
            activeWeapon = spawn("Salamander Lightning Boom");
            activeWeapon.transform.localPosition = Vector2(0,-0.5*actor.height);
        }
        state = "drift upward";
    }
    state "drift upward"
    {
        lookAtPlayer();
        dt = Time.delta;
        player = Player.active;
        transcend.speed = 0.875 * player.ysp;
        wings.anim = 3;
        if (activeWeapon && playerWasHit) {
            activeWeapon.cancel();
            activeWeapon = null;
        }
        if (isFarAbovePlayer() || (!player.midair) || (!platformer.midair)) {
            startLanding();
        }
    }
    state "up landing"
    {
        if (!platformer.midair) {
            startRecovery(true);
        }
        else if (firstUpFlap || (platformer.falling && belowPlayer())) {
            platformer.forceJump(300);
            ascentSample.play();
            firstUpFlap = false;
            wings.anim = 2;
            wings.frame = 0;
        }
        if (!belowPlayer()) {
            state = "up landing trial";
        }
    }
    state "up landing trial"
    {
        if (!platformer.midair) {
            startRecovery(true);
        }
        else if (farBelowPlayer()) {
            firstUpFlap = true;
            state = "up landing";
        }
    }
    state "landing"
    {
        wings.anim = 4;
        if (!platformer.midair) {
            startRecovery(true);
        }
    }
    state "recover"
    {
        actor.animation.id = 1;
        wings.anim = 0;
        if (timeout(recoverTime)) {
            if (hp <= angryHp) {
                state = "angry";
            }
            else {
                state = "charging";
            }
        }
    }
    state "charging"
    {
        actor.animation.id = 3;
        if (actor.animation.finished) {
            platformer.jump();
            jumpSample.play();
            state = "Jump";
        }
    }
    state "Jump"
    {
        actor.animation.id = 6;
        wings.anim = 2;
        if (platformer.falling) {
            platformer.enabled = false;
            state = "Lightning Soar";
        }
    }
    state "Lightning Soar"
    {
        startAscent();
    }

    state "angry"
    {
        actor.animation.id = 5;
        if (actor.animation.finished) {
            collider.enabled = false;
            rollCollider.enabled = true;
            rollMode = true;
            platformer.jumpSpeed = 100;
            platformer.stop();
            platformer.speed = 300;
            bounceCount = 0;
            spinChargeSample.play();
            state = "Spin Charge";
        }
    }
    state "Spin Charge"
    {
        actor.animation.id = 9;
        wings.visible = false;
        if (timeout(0.5)) {
            state = "Roll";
            rollSample.play();
            // FIXME restore to just `platformer.walk()` when
            //   SurgeEngine.Behavior.Platformer allows setting direction
            //   directly
            if (get_direction() < 0) {
                platformer.walkLeft();
            }
            else {
                platformer.walkRight();
            }
            platformer.walk();
        }
    }
    state "Roll"
    {
        actor.animation.id = 4;
        wings.visible = false;
        if (platformer.speed < 20) {
            platformer.stop();
            state = "roll recover";
        }
        if (isTouchingWall()) {
            rollBounce();
        }
        platformer.speed -= 50*Time.delta;
    }
    state "roll recover"
    {
        collider.enabled = true;
        wings.visible = true;
        rollCollider.enabled = false;
        rollCollider.visible = false;
        actor.animation.id = 1;
        oldSpeed = platformer.speed;
        newSpeed = Math.max(0, oldSpeed - 100*Time.delta);
        platformer.speed = newSpeed;
        if (newSpeed <= 0) {
            startRecovery(false);
        }
    }


    state "getting hit"
    {
        actor.animation.id = 2;
        if (actor.animation.finished) {
            state = nextState;
        }
    }

    // exploding (a la Giant Wolf)
    state "exploding"
    {
        if(timeout(explosionTime)) {
            state = "defeated";
        }
    }

    // defeated (a la Giant Wolf)
    state "defeated"
    {
        onDefeat.call();
        state = "done";
        replacement =
            Level.spawnEntity("Salamander Boss Defeated", transform.position);
        replacement.hflip = actor.hflip;
        actor.visible = false;
        wings.visible = false;
    }

    // done (a la Giant Wolf)
    state "done"
    {
    }


    fun constructor()
    {
        collider.visible = false;
        platformer.enabled = false;
        platformer.speed = 140;
        platformer.stop();
        collider.setAnchor(0.5,1.0);
        transcend.enabled = false;
    }

    fun activate()
    {
        if(state == "waiting")
            state = "take off";
    }
    fun get_position()
    {
        return transform.position;
    }
    fun startRecovery(withLanding)
    {
        state = "recover";
        playerWasHit = false;
        if (withLanding) {
            floorHitSample.play();
        }
    }
    fun startLanding()
    {
        playerWasHit = false;
        if (activeWeapon) {
            activeWeapon.cancel();
            activeWeapon = null;
        }
        transcend.enabled = false;
        platformer.gravityMultiplier = 1.0;
        if (belowPlayer()) {
            // edge case
            firstUpFlap = true;
            state = "up landing";
        }
        else {
            state = "landing";
        }
    }
    fun belowPlayer()
    {
        return transform.position.y > Player.active.transform.position.y;
    }
    fun restorePlayer()
    {
        if (rollMode)
            /* do not restore, and */return;
        playerWasHit = true;
        // start a timer that will eventually springify the player.
        restorePlayerEvent.call();
    }
    fun isFarAbovePlayer()
    {
        player = Player.active;
        stableHeight = transform.position.y + stabilityDepth;
        return player.transform.position.y > stableHeight;
    }
    fun farBelowPlayer()
    {
        player = Player.active;
        stableHeight = transform.position.y - stabilityDepth;
        return player.transform.position.y < stableHeight;
    }
    fun onReturnStrike()
    {
        if (hp == 0)
            return;
        hit.play();
        hp--;
        if (hp <= 0) {
            // done
            explode();
        }
        else if (hp <= angryHp) {
            nextState = "angry";
            state = "getting hit";
            transcend.enabled = false;
            platformer.enabled = true;
        }
        else {
            nextState = "charging";
            state = "getting hit";
        }
    }
    fun lookAtPlayer()
    {
        playerPosition = Player.active.transform.position;
        xdiff = playerPosition.x - transform.position.x;
        if (xdiff < -chaseMargin) {
            // FIXME replace with `platformer.direction = -1` when
            //   SurgeEngine.Behavior.Platformer allows setting direction
            //   directly
            actor.hflip = true;
        }
        else if (xdiff > +chaseMargin) {
            // FIXME replace with `platformer.direction = +1` when
            //   SurgeEngine.Behavior.Platformer allows setting direction
            //   directly
            actor.hflip = false;
        }
        else {
            /* do nothing */;
        }
    }
    fun collidesWith(otherCollider)
    {
        return otherCollider.collidesWith(collider);
    }
    fun explode()
    {
        // get hit & explode
        nextState = "exploding";
        state = "getting hit";

        // spawn Explosion Combo
        diameter = collider.width * 2;
        Level.spawnEntity(
            "Explosion Combo",
            collider.center
        ).setSize(diameter, diameter).setDuration(explosionTime);
        platformer.stop();
    }
    fun harmless()
    {
      return state == "take off delayed"
          || state == "take off"
          || canBeHit();
    }
    fun onCollision(other)
    {
        if (hp <= 0)
            return;
        e = other.entity;
        /* hit a player? */
        if (e.hasTag("player")) {
            player = other.entity;
            if (player.attacking && canBeHit()) {
                onReturnStrike();
                if (player.midair) {
                    player.bounceBack(actor);
                }
                else {
                    player.gsp = -player.gsp;
                }
            }
            else if (state == "getting hit") {
                /* do nothing */
            }
            else if (harmless()) {
                /* do nothing */;
            }
            else if (canBeBounced(player)) {
                rollBounce();
            }
            else if (!playerWasHit) {
                player.getHit(get_falling() ? null : actor);
                restorePlayer();
            }
        }
        else if (e.hasTag("boss bridge")) {
            if (state != "Lightning Smash")
                return;
            e.collapse(transform.position);
            acceptFalling();
        }
    }
    fun acceptFalling()
    {
        state = "falling";
    }
    fun canBeBounced(player)
    {
        return false;
    }
    fun rollBounce()
    {
        platformer.jump();
        bounceSample.play();
    }
    fun isTouchingWall()
    {
        if (platformer.direction < 0)
            return platformer.leftWall;
        else if (platformer.direction > 0)
            return platformer.rightWall;
    }
    fun canBeHit()
    {
        return (state == "recover") || (state == "roll recover");
    }
    fun get_falling()
    {
        if (platformer.enabled)
            return platformer.falling;
        else if (transcend.enabled)
            return transcend.speed > 0 && transcend.direction.y > 0;
        else
            return false;
    }
    fun get_ascending()
    {
        if (platformer.enabled)
            return false;
        else if (transcend.enabled)
            return transcend.speed > 0 && transcend.direction.y < 0;
        else
            return false;
    }
    fun get_midair()
    {
        if (platformer.enabled)
            return platformer.midair;
        else
            return true;
    }
    fun get_preparingAttack() //TODO let tail use this property
    {
        return state == "charging" || state == "angry";
    }
    fun startAscent()
    {
        playerWasHit = false;
        platformer.enabled = false;
        transcend.direction = Vector2.up;
        transcend.speed = 300;
        transcend.enabled = true;
        state = "ascend";
        ascentSample.play();
    }
    fun translate(offset)
    {
        transform.translate(offset);
    }
    fun get_charging()
    {
        return state == "Spin Charge";
    }
    fun get_collider()
    {
        return collider;
    }
    fun get_direction()
    {
        // FIXME restore to `return platformer.direction` when
        //   SurgeEngine.Behavior.Platformer allows setting direction
        //   directly
        return actor.hflip ? -1 : +1;
    }
}

object "Salamander Dash Sparks" is "entity", "private"
{
    actor = Actor("Salamander Dash Sparks");
    transform = Transform();
    state "main"
    {
        actor.zindex = 0.49;
        actor.animation.id = 2;
        actor.hflip = (parent.direction < 0);
        if (parent.charging) {
            transform.localPosition = actor.actionOffset.scaledBy(-1);
            state = "charging";
        }
    }
    state "charging"
    {
        actor.animation.id = 0;
        if (!parent.charging) {
            state = "release";
        }
    }
    state "release"
    {
        actor.animation.id = 2;
        if (parent.charging) {
            actor.hflip = (parent.direction < 0);
            transform.localPosition = actor.actionOffset.scaledBy(-1);
            state = "charging";
        }
        else if (actor.animation.finished) {
            state = "main";
        }
    }
}

object "Salamander Lightning Boom" is "entity", "private"
{
    public transform = Transform();
    sample = Sound("samples/salamander_boom.ogg");
    actor = Actor("Salamander Lightning Boom");
    collider = CollisionBall(96);
    decayTime = 3/12;
    decayRemaining = 0;

    state "main"
    {
        sample.volume = 1;
        sample.play();
        state = "idle";
    }
    state "idle"
    {
    }
    state "cool down"
    {
        decayRemaining -= Time.delta;
        sample.volume = Math.max(0, decayRemaining/decayTime);
        if (decayRemaining <= 0) {
            destroy();
        }
    }
    fun onCollision(other) {
        e = other.entity;
        /* hit a player? */
        if (e.hasTag("player")) {
            player = other.entity;
            if (player.shield == "thunder") {
                /* do nothing */;
                player.xsp = -player.xsp;
            }
            else {
                player.getHit(actor);
                collider.enabled = false;
                parent.restorePlayer();
            }
        }
    }
    fun cancel()
    {
        actor.animation.id = 1;
        decayTime = actor.animation.frameCount / actor.animation.fps;
        decayRemaining = decayTime;
        state = "cool down";
    }
}

object "Salamander Boss Wing Anchor" is "private", "entity"
{
    public anim = 0;
    public visible = true;
    //actor = Actor("Salamander Boss Wing Anchor");
    transform = Transform();
    parentActor = parent.child("Actor");
    rightWing = spawn("Salamander Boss Wing").setDirection(+1);
    leftWing = spawn("Salamander Boss Wing").setDirection(-1);
    tail = spawn("Salamander Boss Tail");

    state "main"
    {
        transform.localPosition = parentActor.actionOffset;
        hflip = parentActor.hflip;
        //actor.hflip = hflip;
        //actor.visible = visible;
        rightWing.visible = visible;
        rightWing.hflip = hflip;
        rightWing.anim = anim;
        leftWing.visible = visible;
        leftWing.hflip = hflip;
        leftWing.anim = anim;
        tail.visible = visible;
        tail.hflip = hflip;
    }
    fun constructor()
    {
        //actor.zindex = 0.55;
    }
    fun get_frame()
    {
        return Math.min(rightWing.frame, leftWing.frame);
    }
    fun set_frame(v)
    {
        rightWing.frame = v;
        leftWing.frame = v;
    }
    fun get_midair()
    {
        return parent.midair;
    }
    fun get_actionY()
    {
        return parentActor.actionSpot.y;
    }
    fun get_falling()
    {
        return parent.falling;
    }
    fun get_ascending()
    {
        return parent.ascending;
    }
    fun get_preparingAttack()
    {
        return parent.preparingAttack;
    }
}
object "Salamander Boss Tail" is "private", "entity"
{
    actor = Actor("Salamander Boss Tail");
    transform = Transform();
    state "main"
    {
        if (parent.ascending) {
            // ascending
            actor.anim = 1;
        }
        else if (parent.falling) {
            // descending
            actor.anim = 2;
        }
        else if (parent.midair) {
            // hanging
            actor.anim = 3;
        }
        else if (parent.preparingAttack) {
            // flat but charging
            actor.anim = 4;
        }
        else {
            // flat
            actor.anim = 0;
        }
        transform.localPosition = actor.actionOffset.scaledBy(-1)
            .translatedBy(actor.hflip ? +7 : -7, Math.min(39,102-parent.actionY));
    }
    fun constructor()
    {
        actor.zindex = 0.49;
    }
    fun get_hflip()
    {
        return actor.hflip;
    }
    fun set_hflip(v)
    {
        actor.hflip = v;
    }
    fun get_visible()
    {
        return actor.visible;
    }
    fun set_visible(v)
    {
        actor.visible = v;
    }
}
object "Salamander Boss Wing" is "private", "entity"
{
    actor = Actor("Salamander Boss Wing");
    transform = Transform();
    side = false;
    hflip = false;

    state "main"
    {
        transform.localPosition = actor.actionOffset.scaledBy(-1).translatedBy(side ? -3 : +3,0);
    }
    fun constructor()
    {
        actor.zindex = 0.495;
    }
    fun setDirection(dir)
    {
        side = dir < 0;
        actor.hflip = side;
        return this;
    }
    fun get_hflip()
    {
        return hflip;
    }
    fun set_hflip(v)
    {
        hflip = v;
        set_anim(get_anim());
    }
    fun get_visible()
    {
        return actor.visible;
    }
    fun set_visible(v)
    {
        actor.visible = v;
    }
    fun get_frame()
    {
        return actor.animation.frame;
    }
    fun set_frame(v)
    {
        actor.animation.frame = v;
    }
    fun get_offset()
    {
        return (side || hflip) && (!(side && hflip)) ? 1 : 0;
    }
    fun get_anim()
    {
        return (actor.anim-get_offset())/2;
    }
    fun set_anim(v)
    {
        actor.anim = v*2+get_offset();
    }
}

object "Salamander Invisible Wall" is "entity"
{
    brick = Brick("Salamander Invisible Wall Mask");

    state "main"
    {
        brick.enabled = false;
    }
    state "active"
    {
        brick.enabled = true;
    }
    fun activate()
    {
        state = "active";
    }
    fun deactivate()
    {
        state = "main";
    }
}

object "Salamander Restore Helper" is "entity", "private"
{
    timer = 0.0;
    state "main"
    {
    }
    state "waiting"
    {
        timer -= Time.delta;
        if (timer <= 0.0) {
            state = "main";
            player = Player.active;
            if (player.hit && player.midair) {
                player.springify();
            }
        }
    }

    fun call()
    {
        timer = 1.0;
        state = "waiting";
    }
}

object "Salamander Boss Defeated" is "entity", "private"
{
    public transform = Transform();
    actor = Actor("Salamander Boss Defeated");
    platformer = Platformer();
    collider = CollisionBox(30,40).setAnchor(0.5,1);
    stopX = 0;

    state "main"
    {
        actor.animation.id = 0;
        if (actor.animation.finished) {
            state = "gasp";
        }
    }
    state "gasp"
    {
        actor.animation.id = 3;
        if (actor.animation.finished) {
            platformer.walkRight();
            state = "cry walk";
        }
    }
    state "cry walk"
    {
        actor.animation.id = 1;
    }
    state "cry near stop"
    {
        if (transform.position.x >= stopX) {
            state = "stop";
            platformer.stop();
        }
    }
    state "stop"
    {
        actor.animation.id = 2;
    }
    fun constructor() {
        platformer.speed = 40;
        platformer.stop();
    }
    fun get_hflip()
    {
        return actor.hflip;
    }
    fun set_hflip(v)
    {
        actor.hflip = v;
    }
    fun onCollision(other)
    {
        if (other.entity.__name == "Salamander Exploding Wall") {
            stopX = transform.position.x + 88;
            state = "cry near stop";
            collider.enabled = false;
        }
    }
}
