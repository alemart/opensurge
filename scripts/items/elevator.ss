// -----------------------------------------------------------------------------
// File: elevator.ss
// Description: elevator script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Player;
using SurgeEngine.Level;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Collisions.CollisionBox;

//
// Elevator API
//
// Properties:
// - anim: number. Will render the specified animation number of the "Elevator" sprite.
// - speed: number. The speed of the elevator, in px/s.
// - transform: Transform object, read-only.
// - collider: Collider object, read-only.
//
// Functions:
// - addListener(obj). Adds a listener object (obj) to this elevator. This will call:
//   obj.onElevatorActivate(this) whenever this elevator is activated, and
//   obj.onElevatorComplete(this) whenever this elevator completes its path.
//
object "Elevator" is "entity", "gimmick"
{
    public readonly collider = CollisionBox(128, 8);
    public readonly transform = Transform();
    public speed = 60; // in px/s
    lineCollider = CollisionBox(128, 1);
    actor = Actor("Elevator");
    brick = Brick("Elevator");
    cables = [];
    listeners = [];
    sign = 0;

    state "main"
    {
        setup();
        state = "idle";
    }

    state "idle"
    {
        // wait for the player(s)
        for(i = 0; i < Player.count; i++) {
            player = Player(i);
            if(collider.collidesWith(player.collider)) {
                state = "active";
                notifyListeners();
                return;
            }
        }
    }

    state "active"
    {
        // check if there's more room to go
        if(!findOverlappingCable()) {
            state = "done";
            fixPositions();
            notifyListeners();
            return;
        }

        // move the elevator
        oldY = Math.floor(transform.localPosition.y);
        transform.move(0, sign * speed * Time.delta);
        dy = Math.floor(transform.localPosition.y) - oldY;

        // move the player(s)
        for(i = 0; i < Player.count; i++) {
            player = Player(i);
            if(collider.collidesWith(player.collider))
                player.transform.move(0, dy);
        }
    }

    state "done"
    {
    }

    fun constructor()
    {
        this.anim = 0;
        collider.setAnchor(0, 1);
        //collider.visible = true;
        lineCollider.setAnchor(0, 0);
    }

    fun setup()
    {
        // setup the elevator
        cables = Level.findObjects("Elevator Cable");
        sign = shouldMoveUp() ? -1.0 : 1.0;
    }

    fun findOverlappingCable()
    {
        for(i = 0; i < cables.length; i++) {
            if(lineCollider.collidesWith(cables[i].collider))
                return cables[i];
        }

        return null;
    }

    fun shouldMoveUp()
    {
        return true; // FIXME
        
        moveUp = true;

        if((cable = findOverlappingCable())) {
            transform.move(0, -cable.collider.height);
            lineCollider.setAnchor(0, 0); // redo cache
            moveUp = (findOverlappingCable() != null);
            transform.move(0, cable.collider.height);
        }

        return moveUp;
    }

    fun fixPositions()
    {
        oldy = Math.round(transform.position.y - sign * speed * Time.delta);
        transform.position = Vector2(transform.position.x, oldy + (oldy % 2));
        for(i = 0; i < Player.count; i++) {
            player = Player(i);
            if(collider.collidesWith(player.collider)) {
                oldy = Math.round(player.transform.position.y - sign * speed * Time.delta);
                player.transform.position = Vector2(player.transform.position.x, oldy + (oldy % 2));
            }
        }
    }

    fun notifyListeners()
    {
        for(j = 0; j < listeners.length; j++) {
            if(state == "active")
                listeners[j].onElevatorActivate(this);
            else if(state == "done")
                listeners[j].onElevatorComplete(this);
        }
    }

    fun onReset()
    {
        state = "idle";
    }

    fun onLeaveEditor()
    {
        setup();
        state = "idle";
    }



    //
    // public API
    //

    fun get_anim()
    {
        return actor.anim;
    }

    fun set_anim(id)
    {
        actor.anim = id;
    }

    fun addListener(obj)
    {
        if(listeners.indexOf(obj) < 0)
            listeners.push(obj);
    }
}

object "Elevator Cable" is "entity", "gimmick"
{
    public readonly collider = CollisionBox(8, 128);

    state "main"
    {
        collider.setAnchor(0, 0);
    }
}