// -----------------------------------------------------------------------------
// File: giant_wolf.ss
// Description: Giant Wolf Boss (Sunshine Paradise)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;
using SurgeEngine.Events.Event;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Collisions.CollisionBall;
using SurgeEngine.Collisions.Sensor;

// Giant Wolf is the Sunshine Paradise boss
object "Giant Wolf" is "entity", "boss", "awake"
{
    public onDefeat = Event();
    public hp = 4; // health points

    actor = Actor("Giant Wolf's Body");
    head = spawn("Giant Wolf's Head");
    leftHand = spawn("Giant Wolf's Hand").setLeft();
    rightHand = spawn("Giant Wolf's Hand").setRight();
    transform = Transform();

    basePosition = 0;
    angryHp = 1;
    withdrawal = DirectionalMovement();
    explosionTime = 2.0; // seconds
    walkTimer = 0;
    chase = DirectionalMovement();
    chaseMargin = 112; // easy !!! (should not touch the player)

    state "main"
    {
        // setup
        actor.zindex = 0.4;
        basePosition = transform.position.y + 8; // offset it a bit

        chase.direction = Vector2.left;
        chase.speed = 60;
        chase.enabled = false;

        withdrawal.direction = Vector2.down;
        withdrawal.speed = 60;
        withdrawal.enabled = false;

        // done
        state = "waiting";
    }

    state "waiting"
    {
        //
        // waiting for the boss fight
        //
        // the boss needs to be activated
        // by calling function activate()
        //
    }

    // chase the player and punch
    state "chasing player"
    {
        if(walkTowards(Player.active)) {
            leftHand.punch();
            rightHand.punch();
        }
    }

    // punch all the time
    state "angry"
    {
        chaseMargin = 80; // an angry boss is a harder boss
        walkTowards(Player.active);
        leftHand.punch();
        rightHand.punch();
    }

    // exploding
    state "exploding"
    {
        if(timeout(explosionTime))
            state = "defeated";
    }

    // defeated
    state "defeated"
    {
        withdrawal.enabled = true;
        if(timeout(actor.height * 2.0 / withdrawal.speed)) {
            onDefeat.call();
            state = "done";
        }
    }

    // done
    state "done"
    {
    }



    // the boss got hit
    fun getHit()
    {
        if(hp > 0) {
            // lift hands
            leftHand.lift();
            rightHand.lift();

            // deciding the next state
            // according to the hp
            hp--;
            if(hp == 0) {
                chase.enabled = false;
                leftHand.stop();
                rightHand.stop();
                head.explode(explosionTime);
                state = "exploding";
            }
            else if(hp <= angryHp) {
                head.getAngry();
                state = "angry";
            }
            else
                state = "chasing player";
        }
    }

    // is the boss angry?
    fun isAngry()
    {
        return (state == "angry");
    }

    // is the boss activated?
    fun isActivated()
    {
        return (state != "main" && state != "waiting");
    }

    // activate the boss
    fun activate()
    {
        if(state == "waiting")
            state = "chasing player";
    }

    // Walk towards the player. Returns true if the
    // player is within a "punch" area, false otherwise
    fun walkTowards(player)
    {
        inPunchArea = false;

        // skip movement?
        chase.enabled = false;
        if(!rightHand.isIdle() && !isAngry())
            return false;

        // chase the player
        // (movement on the x-axis)
        playerX = player.transform.position.x;
        xpos = transform.position.x;
        if(xpos < playerX - chaseMargin) {
            chase.direction = Vector2.right;
            chase.enabled = true;
        }
        else if(xpos > playerX + chaseMargin) {
            chase.direction = Vector2.left;
            chase.enabled = true;
        }
        else if(Math.abs(xpos - playerX) > chaseMargin / 2)
            inPunchArea = true;
        else
            chase.enabled = true;

        // walking effect
        // (movement on the y-axis)
        walkTimer += Time.delta;
        walk = Math.sin(9.5 * walkTimer);
        ypos = basePosition + 4.0 * Math.abs(walk);
        transform.position = Vector2(xpos, ypos);

        // done
        return inPunchArea;
    }
}

// the head can be hit by the player
object "Giant Wolf's Head" is "private", "entity", "awake"
{
    wolf = parent;
    actor = Actor("Giant Wolf's Head");
    collider = CollisionBall(64);
    transform = Transform();
    hit = Sound("samples/bosshit.wav");
    roar = Sound("samples/roar.ogg");
    cry = Sound("samples/growlmod.wav");
    nextState = "";
    roared = false;
    explosionTime = 0.0;

    state "main"
    {
        actor.zindex = 0.41;
        actor.anim = 0;
        transform.localPosition = Vector2(0, -216);
        nextState = "eyes open";
        state = nextState;

        // debug
        /*
        collider.visible = true;
        */
    }

    state "eyes open"
    {
        actor.anim = 0;
    }

    state "getting hit"
    {
        actor.anim = 2;
        if(actor.animation.finished)
            state = nextState;
    }

    state "angry"
    {
        actor.anim = 3;
        if(!roared) {
            roar.play();
            roared = true;
        }
    }

    state "exploding"
    {
        if(timeout(explosionTime)) {
            cry.play();
            state = "defeated";
        }
    }

    state "defeated"
    {
        actor.anim = 1;
    }

    fun getAngry()
    {
        nextState = "angry";
        if(state == "eyes open")
            state = nextState;
    }

    fun explode(durationInSeconds)
    {
        // get hit & explode
        nextState = "exploding";
        state = "getting hit";

        // spawn Explosion Combo
        explosionTime = durationInSeconds;
        diameter = collider.radius * 2;
        Level.spawnEntity(
            "Explosion Combo",
            transform.position.translatedBy(0, diameter / 4)
        ).setSize(diameter, diameter).setDuration(explosionTime);
    }

    fun onCollision(otherCollider)
    {
        if(wolf.isActivated() && state != "defeated") {
            if(otherCollider.entity.hasTag("player")) {
                player = otherCollider.entity;
                if(player.attacking) {
                    hit.play();
                    wolf.getHit();
                    player.bounceBack(actor);
                    player.xsp = -player.xsp;
                    state = "getting hit";
                }
            }
        }
    }
}

// the hand has 3 basic actions: shake, punch and lift
object "Giant Wolf's Hand" is "private", "entity", "awake"
{
    wolf = parent;
    actor = Actor("Giant Wolf's Hand");
    brick = Brick("Giant Wolf's Hand Mask");
    collider = CollisionBox(48, 16);
    sensor = Sensor(-24, -2, 48, 1);
    transform = Transform();
    offset = Vector2(64, -80);
    prevX = 0;
    punchSpeed = 360;
    liftSpeed = punchSpeed / 2;
    shakeTime = 1.0;
    restTime = 2.5; // very easy

    state "main"
    {
        actor.zindex = 0.5;
        brick.type = "cloud";
        prevX = transform.position.x;
        state = "idle";

        // debug
        /*
        collider.visible = true;
        sensor.visible = true;
        */
    }

    state "idle"
    {
        prevX = transform.position.x;
    }

    state "shaking"
    {
        if(timeout(shakeTime) || (wolf.isAngry() && timeout(0.3 * shakeTime))) {
            shakeY = 0;
            brick.enabled = true;
            state = "idle";
        }
        else {
            shakeY = 4 * Math.cos(60 * Time.time);
            brick.enabled = false;
        }

        transform.localPosition = Vector2(
            transform.localPosition.x,
            offset.y + shakeY
        );
    }

    state "punching"
    {
        brick.enabled = true;
        if(sensor.status !== null) {
            // hit the ground
            Level.spawnEntity("Giant Wolf's Hand Impact", transform.position);
            state = "resting";
        }
        else if(transform.localPosition.y >= Screen.height)
            state = "resting"; // no bricks outside the screen
        else
            transform.move(0, punchSpeed * Time.delta);
    }

    state "resting"
    {
        if(timeout(restTime) || wolf.isAngry())
            state = "lifting";
    }

    state "lifting"
    {
        transform.move(0, -liftSpeed * Time.delta);

        // the hand has been lifted
        if(transform.localPosition.y <= offset.y) {
            transform.localPosition = Vector2(
                transform.localPosition.x,
                offset.y
            );
            state = "shaking";
        }
    }

    state "stopped"
    {
        actor.zindex = 0.41;
        brick.enabled = false;
    }

    // set this to be the right hand
    fun setRight()
    {
        actor.hflip = false;
        transform.localPosition = offset;
        return this;
    }

    // set this to be the left hand
    fun setLeft()
    {
        actor.hflip = true;
        transform.localPosition = Vector2(-offset.x, offset.y);
        return this;
    }

    // punch
    fun punch()
    {
        if(state == "idle")
            state = "punching";
    }

    // lift the hand
    fun lift()
    {
        if(state == "resting") {
            brick.enabled = false; // so the player can't hit the boss too much
            state = "lifting";
        }
    }

    // the boss has been defeated; just stop
    fun stop()
    {
        state = "stopped";
    }

    // is the hand idle? (i.e., not shaking, punching or being lifted)
    fun isIdle()
    {
        return (state == "idle");
    }

    // handle collisions
    fun onOverlap(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(state == "punching")
                player.getHit(actor);
        }
    }
}

// hand impact: special effect
object "Giant Wolf's Hand Impact" is "private", "entity", "disposable"
{
    actor = Actor("Giant Wolf's Hand Impact");
    hit = Sound("samples/floorhit.wav");

    state "main"
    {
        hit.play();
        actor.zindex = 0.51;
        state = "playing";
    }

    state "playing"
    {
        if(actor.animation.finished)
            destroy();
    }
}