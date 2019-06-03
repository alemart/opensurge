// -----------------------------------------------------------------------------
// File: pipes.ss
// Description: pipe system script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Brick;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

object "Pipe Up" is "entity", "gimmick", "special"
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

object "Pipe Right" is "entity", "gimmick", "special"
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

object "Pipe Down" is "entity", "gimmick", "special"
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

object "Pipe Left" is "entity", "gimmick", "special"
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

object "Pipe Out" is "entity", "gimmick", "special"
{
    pipeManager = Level.child("Pipe Manager") || Level.spawn("Pipe Manager");
    pipeSensor = spawn("Pipe Sensor").setManager(pipeManager);
    brick = Brick("Pipe Out");
    playerCollider = null;
    isBlocked = { };
    public zindex = 1.0;

    state "main"
    {
        brick.enabled = (isBlocked[Player.active.name] || false);
    }

    state "block"
    {
        if(!playerCollider.collidesWith(pipeSensor.collider)) {
            player = playerCollider.entity;
            isBlocked[player.name] = true;
            state = "main";
        }
    }

    fun onPipeActivate(player)
    {
        pipeManager.getOut(player);
        playerCollider = player.collider;
        state = "block";
    }

    fun constructor()
    {
        brick.type = "solid";
        brick.enabled = false;
    }
}

object "Pipe In" is "entity", "gimmick", "special"
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
        if(nearestSensor != null) {
            // get in the pipe
            pipeManager.getIn(player);

            // guess the direction to go
            offset = nearestSensor.transform.position.minus(pipeSensor.transform.position);
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
    }

    fun findNearestSensor()
    {
        bestSensor = null;
        bestDistance = Math.infinity;
        myPosition = pipeSensor.transform.position;

        sensors = Level.findObjects("Pipe Sensor");
        foreach(sensor in sensors) {
            if(sensor != pipeSensor && !sensor.isEntrance()) {
                d = distance(sensor.transform.position, myPosition);
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
    public readonly transform = Transform();
    public readonly collider = CollisionBox(28, 28);
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

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            //if(entrance && !player.hasFocus()) return; // FIXME
            insidePipe = pipeManager.isInsidePipe(player);
            if((entrance && !insidePipe) || (!entrance && insidePipe)) {
                player.transform.position = transform.position
                    .plus(player.animation.hotspot)
                    .minus(ballCenter(player))
                ;
                parent.onPipeActivate(player);
            }
        }
    }

    fun ballCenter(player)
    {
        player.springify(); // hack: unroll to adjust collider
        return Vector2(player.width / 2, Math.floor(player.height - player.collider.height * 0.3));
    }

    fun constructor()
    {
        //collider.visible = true;
    }
}

object "Pipe Manager" is "private", "awake", "entity"
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
            travelers[player.name] = spawn("Pipe Traveler").setPlayer(player);
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
        if(!travelers.has(player.name))
            travelers[player.name] = spawn("Pipe Traveler").setPlayer(player);

        return travelers[player.name];
    }

    //
    // public API
    //
    fun getIn(player)
    {
        if(!isInsidePipe(player))
            pipeInSfx.play();

        travelerOf(player).getIn();
    }

    fun getOut(player)
    {
        if(isInsidePipe(player))
            pipeOutSfx.play();

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

object "Pipe Traveler" is "private", "awake", "entity"
{
    player = null;
    rollAnimation = 18;
    pipeManager = parent;
    throwSpeed = Vector2.zero;

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
        player.speed = throwSpeed.x;
        player.ysp = throwSpeed.y;
        player.roll();
        player.anim = rollAnimation;
        state = "stopped";
    }

    state "up"
    {
        player.transform.move(0, -pipeManager.speed * Time.delta);
        player.anim = rollAnimation;
        player.frozen = true;
    }
 
    state "right"
    {
        player.transform.move(pipeManager.speed * Time.delta, 0);
        player.anim = rollAnimation;
        player.frozen = true;
    }
 
    state "down"
    {
        player.transform.move(0, pipeManager.speed * Time.delta);
        player.anim = rollAnimation;
        player.frozen = true;
    }
    
    state "left"
    {
        player.transform.move(-pipeManager.speed * Time.delta, 0);
        player.anim = rollAnimation;
        player.frozen = true;
    }

    fun setPlayer(myPlayer)
    {
        player = myPlayer;
        return this;
    }

    fun computeThrowSpeed(direction)
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
            player.frozen = true;
            state = "in";
        }
    }

    fun getOut()
    {
        if(isTraveling()) {
            throwSpeed = computeThrowSpeed(state);
            player.frozen = false;
            state = "out";
        }
    }

    fun goUp()
    {
        state = "up";
    }

    fun goRight()
    {
        player.speed = player.ysp = 0;
        state = "right";
    }

    fun goDown()
    {
        player.speed = player.ysp = 0;
        state = "down";
    }

    fun goLeft()
    {
        player.speed = player.ysp = 0;
        state = "left";
    }

    fun isTraveling()
    {
        return player.frozen && state != "stopped";
    }
}