// -----------------------------------------------------------------------------
// File: pipes.ss
// Description: pipe system script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Camera;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Brick;
using SurgeEngine.Video.Screen;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Collisions.CollisionBall;

object "Pipe Up" is "entity", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager);
    public zindex = 1.0;

    state "main"
    {
    }

    fun onPipeActivate(player)
    {
        pipeManager.goUp(player);
    }
}

object "Pipe Right" is "entity", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager);
    public zindex = 1.0;

    state "main"
    {
    }

    fun onPipeActivate(player)
    {
        pipeManager.goRight(player);
    }
}

object "Pipe Down" is "entity", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager);
    public zindex = 1.0;

    state "main"
    {
    }

    fun onPipeActivate(player)
    {
        pipeManager.goDown(player);
    }
}

object "Pipe Left" is "entity", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager);
    public zindex = 1.0;

    state "main"
    {
    }

    fun onPipeActivate(player)
    {
        pipeManager.goLeft(player);
    }
}

object "Pipe Out" is "entity", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager);
    brick = Brick("Pipe Out");
    collider = CollisionBall(80);
    public zindex = 1.0;

    state "main"
    {
        state = "blocked";
    }

    state "blocked"
    {
        brick.enabled = true;

        if(wantUnblocked())
            state = "unblocked";
    }

    state "unblocked"
    {
        brick.enabled = false;

        if(!wantUnblocked())
            state = "cooldown";
    }

    state "cooldown"
    {
        if(timeout(0.5)) {
            if(wantUnblocked())
                state = "unblocked";
            else
                state = "blocked";
        }
    }

    fun onPipeActivate(player)
    {
        pipeManager.getOut(player);
    }

    fun wantUnblocked()
    {
        // we will unblock the exit if there is a player near Pipe Out & inside the tube
        for(i = 0; i < Player.count; i++) {
            player = Player[i];

            if(collider.collidesWith(player.collider)) {
                if(pipeManager.isInsidePipe(player))
                    return true;
            }
        }

        return false;
    }

    fun constructor()
    {
        brick.type = "solid";
        brick.enabled = true;
    }
}

object "Pipe In" is "entity", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager).setAsEntrance();
    nearestSensor = null;
    public zindex = 1.0;

    state "main"
    {
        if(timeout(0.5)) {
            nearestSensor = findNearestSensor();
            state = "idle";
        }
    }

    state "idle"
    {
    }

    fun onPipeActivate(player)
    {
        // nothing to do
        if(nearestSensor == null)
            return;

        // get in the pipe
        pipeManager.getIn(player);

        // guess the direction to go
        offset = nearestSensor.position.minus(pipeSensor.position);
        if(Math.abs(offset.x) >= Math.abs(offset.y)) {
            if(offset.x >= 0)
                pipeManager.goRight(player);
            else
                pipeManager.goLeft(player);
        }
        else {
            if(offset.y >= 0)
                pipeManager.goDown(player);
            else
                pipeManager.goUp(player);
        }
    }

    fun findNearestSensor()
    {
        bestSensor = null;
        bestDistance = Math.infinity;
        myPosition = pipeSensor.position;

        sensors = Level.findObjects("Pipe Sensor");
        foreach(sensor in sensors) {
            if(sensor != pipeSensor && !sensor.isEntrance()) {
                d = distance(sensor.position, myPosition);
                if(d < bestDistance) {
                    bestSensor = sensor;
                    bestDistance = d;
                }
            }
        }

        return bestSensor;
    }

    fun distance(a, b)
    {
        // Manhattan distance
        v = b.minus(a);
        return Math.abs(v.x) + Math.abs(v.y);
    }
}

object "Pipe Sensor" is "private", "entity"
{
    transform = Transform();
    collider = CollisionBox(28, 28);
    pipeManager = null;
    entrance = false;

    state "main"
    {
    }
    
    fun setManager(manager)
    {
        pipeManager = manager;
        return this;
    }

    fun setAsEntrance()
    {
        entrance = true;
        return this;
    }

    fun isEntrance()
    {
        return entrance;
    }

    fun get_position()
    {
        return transform.position;
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            //if(entrance && !player.hasFocus()) return; // FIXME
            insidePipe = pipeManager.isInsidePipe(player);
            if((entrance && !insidePipe) || (!entrance && insidePipe)) {
                // set the center of the player.collider to the center of this.collider
                player.transform.position = transform.position
                    .minus(player.collider.center)
                    .plus(player.transform.position)
                ;
                parent.onPipeActivate(player);
            }
        }
    }

    fun constructor()
    {
        //collider.visible = true;
    }
}

object "Pipe Manager"
{
    public speed = 600; // in px/s
    pipeInSfx = Sound("samples/pipe_in.wav");
    pipeOutSfx = Sound("samples/pipe_out.wav");
    pipeSfx = null;
    travelers = {};

    //
    // states
    //
    state "main"
    {
        // for each player, create a Pipe Traveler
        for(i = 0; i < Player.count; i++) {
            player = Player[i];
            travelers[player.id] = spawn("Pipe Traveler").setPlayer(player);
        }

        // done
        state = "idle";
    }

    state "idle"
    {
    }

    //
    // functions
    //
    fun travelerOf(player)
    {
        if(!travelers.has(player.id))
            travelers[player.id] = spawn("Pipe Traveler").setPlayer(player);

        return travelers[player.id];
    }

    fun isOffscreen(player)
    {
        extraMargin = 64;

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

    //
    // public API
    //
    fun getIn(player)
    {
        if(!isInsidePipe(player)) {
            if(!isOffscreen(player))
                pipeInSfx.play();
        }

        travelerOf(player).getIn();
    }

    fun getOut(player)
    {
        if(isInsidePipe(player)) {
            if(!isOffscreen(player))
                pipeOutSfx.play();
        }

        travelerOf(player).getOut();
    }

    fun goUp(player)
    {
        travelerOf(player).goUp();
    }

    fun goRight(player)
    {
        travelerOf(player).goRight();
    }

    fun goDown(player)
    {
        travelerOf(player).goDown();
    }

    fun goLeft(player)
    {
        travelerOf(player).goLeft();
    }

    fun isInsidePipe(player)
    {
        return travelerOf(player).isTraveling();
    }
}

object "Pipe Traveler"
{
    player = null; // each Pipe Traveler is linked to a player
    rollAnimation = 18;
    pipeManager = parent;
    throwVelocity = Vector2.zero;

    state "main"
    {
        assert(player != null);
        state = "stopped";
    }

    state "stopped"
    {
    }

    state "in"
    {
        player.speed = player.ysp = 0;
        player.anim = rollAnimation;
        player.frozen = true;
    }

    state "out"
    {
        // throw player
        player.speed = throwVelocity.x;
        player.ysp = throwVelocity.y;
        player.roll();
        player.anim = rollAnimation;
        state = "stopped";
    }

    state "up"
    {
        player.transform.translateBy(0, -pipeManager.speed * Time.delta);
        player.anim = rollAnimation;
        player.frozen = true;
    }
 
    state "right"
    {
        player.transform.translateBy(pipeManager.speed * Time.delta, 0);
        player.anim = rollAnimation;
        player.frozen = true;
    }
 
    state "down"
    {
        player.transform.translateBy(0, pipeManager.speed * Time.delta);
        player.anim = rollAnimation;
        player.frozen = true;
    }
    
    state "left"
    {
        player.transform.translateBy(-pipeManager.speed * Time.delta, 0);
        player.anim = rollAnimation;
        player.frozen = true;
    }

    fun setPlayer(myPlayer)
    {
        player = myPlayer;
        return this;
    }

    fun computeThrowVelocity(direction)
    {
        if(direction == "up")
            return Vector2.up.scaledBy(pipeManager.speed);
        else if(direction == "right")
            return Vector2.right.scaledBy(pipeManager.speed);
        else if(direction == "down")
            return Vector2.down.scaledBy(pipeManager.speed);
        else if(direction == "left")
            return Vector2.left.scaledBy(pipeManager.speed);
        else
            return Vector2.zero;
    }

    fun getIn()
    {
        if(!isTraveling()) {
            player.roll(); // will shrink sensors
            player.frozen = true;
            state = "in";
        }
    }

    fun getOut()
    {
        if(isTraveling()) {
            throwVelocity = computeThrowVelocity(state);
            player.frozen = false;
            state = "out";
        }
    }

    fun goUp()
    {
        if(isTraveling()) {
            player.speed = player.ysp = 0;
            state = "up";
        }
    }

    fun goRight()
    {
        if(isTraveling()) {
            player.speed = player.ysp = 0;
            state = "right";
        }
    }

    fun goDown()
    {
        if(isTraveling()) {
            player.speed = player.ysp = 0;
            state = "down";
        }
    }

    fun goLeft()
    {
        if(isTraveling()) {
            player.speed = player.ysp = 0;
            state = "left";
        }
    }

    fun isTraveling()
    {
        return player.frozen && state != "stopped";
    }
}