// -----------------------------------------------------------------------------
// File: skaterbug.ss
// Description: Skaterbug script
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
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Collisions.Sensor;

//
// Skaterbug is a friend that hops over water,
// carrying the player.
//
// Properties:
// - direction: string. It can be "right" or "left".
//
object "Skaterbug" is "entity", "friend", "gimmick"
{
    actor = Actor("Skaterbug");
    brick = Brick("Skaterbug Mask");
    sfx = Sound("samples/skaterbug.wav");
    collider = CollisionBox(24, 32).setAnchor(0.5, 1);
    transform = Transform();
    jumpVelocity = Vector2(180, jumpSpeed(64)); // in px/s
    restTime = 0.5; // seconds between jumps

    direction = "right";
    dir = 1;
    xsp = 0;
    ysp = 0;
    baselevel = 0;
    player = null; // the player that is being carried
    jumpTime = -2 * jumpVelocity.y / Level.gravity; // time it takes to jump
    leftSensor = Sensor(-actor.width * 0.4, -16, 1, actor.height * 2);
    rightSensor = Sensor(actor.width * 0.4, -16, 1, actor.height * 2);

    state "main"
    {
        // setup
        brick.type = "cloud";
        actor.zindex = 0.51;
        baselevel = transform.position.y;
        state = "active";

        // debug
        //collider.visible = leftSensor.visible = rightSensor.visible = true;
    }

    state "active"
    {
        // jump if a player appears
        if(isCarryingPlayer())
            jump();

        // move it!
        move();
    }

    state "jumping"
    {
        // move
        move();

        // wait a bit between jumps
        if(timeout(jumpTime + restTime))
            state = "active";
    }

    fun onCollision(otherCollider)
    {
        // detect player
        if(otherCollider.entity.hasTag("player"))
            player = otherCollider.entity;
    }

    fun move()
    {
        // initialize variables
        xpos = transform.position.x;
        ypos = transform.position.y;
        carrying = isCarryingPlayer();

        // move entity
        if(ypos < baselevel || ysp < 0) {
            dt = Time.delta;
            xsp = noWall() ? jumpVelocity.x * dir : 0;
            ysp += Level.gravity * dt;
            transform.translateBy(xsp * dt, ysp * dt);
            actor.anim = 1;
        }
        else if(ypos > baselevel) {
            xsp = ysp = 0;
            transform.translateBy(0, baselevel - ypos);
            actor.anim = 0;
        }

        // move the player, if applicable
        if(carrying) {
            dx = Math.floor(transform.position.x) - Math.floor(xpos);
            dy = Math.floor(transform.position.y) - Math.floor(ypos);
            player.moveBy(dx, dy);
        }
        else if(player != null && player.jumping && ysp != 0) {
            if(xsp > 0)
                player.xsp = Math.max(player.xsp, xsp);
            else if(xsp < 0)
                player.xsp = Math.min(player.xsp, xsp);
            player = null;
        }
    }

    fun jump()
    {
        if(canJump()) {
            ysp = jumpVelocity.y;
            sfx.play();
            state = "jumping";
        }
    }

    fun canJump()
    {
        return (transform.position.y >= baselevel) && noWall();
    }

    fun isCarryingPlayer()
    {
        return (player != null) && !player.midair &&
            collider.collidesWith(player.collider);
    }

    fun noWall()
    {
        sensor = (dir > 0) ? rightSensor : leftSensor;
        return (sensor.status == null);
    }

    fun jumpSpeed(jumpHeight)
    {
        // Torricelli's
        return -Math.sqrt(2 * Level.gravity * Math.abs(jumpHeight));
    }

    //
    // Property: direction (must be "left" or "right")
    //
    fun set_direction(direction)
    {
        direction = String(direction);
        dir = (direction.toLowerCase() == "right") ? 1 : -1;
        actor.hflip = (dir < 0);
    }

    fun get_direction()
    {
        return direction;
    }
}