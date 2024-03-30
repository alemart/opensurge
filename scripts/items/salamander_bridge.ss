// -----------------------------------------------------------------------------
// File: salamander_bridge.ss
// Description: script for bridges
// Author: Cody Licorish
// License: MIT
// -----------------------------------------------------------------------------

using SurgeEngine.Actor;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Brick;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Level;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;

// Salamander Bridge
object "Salamander Bridge" is "entity", "gimmick", "awake", "boss bridge"
{
    public length = 6;
    public recoverTime = 3.5;
    elements = [];
    box = null;
    transform = Transform();

    state "main"
    {
        setup();
        state = "idle";
    }
    state "idle"
    {
    }
    state "collapsing"
    {
        if (timeout(recoverTime)) {
            foreach (e in elements) {
                e.fix();
            }
            state = "idle";
        }
    }

    fun setup()
    {
        length = Math.clamp(length,3,12);
        for (i = 0; i < length; ++i) {
            e = spawn("Salamander Bridge Element");
            e.setup(i, length);
            elements.push(e);
        }
        box = CollisionBox(64*length, 16);
        box.setAnchor(0.5,1);
    }
    fun collapse(point)
    {
        if (box && box.contains(point)) {
            index = (point.x - box.left)/64 - 0.5;
            // NOTE: index should be a real number, not an integer
            foreach (e in elements) {
                e.collapse(index);
            }

            // play sound
            sfx = Sound("samples/break.wav");
            sfx.play();

            state = "collapsing";
        }
        else {
            //Console.print("Salamander Bridge untouched by "+point.toString());
        }
    }
}

object "Salamander Bridge Actor" is "entity", "private", "awake"
{
    public printing = false;
    actor = Actor("Salamander Bridge Element");
    transform = Transform();
    angular = 0;
    angle_total = 0;

    state "idle"
    {
    }
    state "collapsing"
    {
        dt = Time.delta;
        transform.rotate(dt * angular);
        if (printing) {
            Console.print(dt+" * "+angular);
            Console.print(transform.angle);
        }
    }

    fun constructor()
    {
        actor.anim = 1;
    }
    fun get_visible()
    {
        return actor.visible;
    }
    fun set_visible(value)
    {
        actor.visible = value;
    }
    fun fix()
    {
        transform.angle = 0;
        actor.anim = 1;
        actor.alpha = 1.0;
        actor.visible = true;
        state = "idle";
    }
    fun collapse(rotate_velocity)
    {
        // actor.visible = true;
        actor.anim = 0;
        actor.alpha = 0.5;
        angular = rotate_velocity;
        state = "collapsing";
    }
}

object "Salamander Bridge Element" is "entity", "private"
{
    actor = spawn("Salamander Bridge Actor");
    brick = Brick("Salamander Bridge Collision Mask");
    transform = Transform();
    index = 0;
    length = 0;
    ysp = 0;

    state "main"
    {
        fix();
    }
    state "idle"
    {
    }
    state "collapsing"
    {
        dt = Time.delta;
        ysp += Level.gravity*dt;
        transform.translateBy(0, ysp*dt);
        if (timeout(2.0)) {
            actor.visible = false;
            state = "hidden";
        }
    }
    state "hidden"
    {
    }

    fun constructor()
    {
        brick.type = "cloud";
    }
    fun setup(i, l)
    {
        index = i;
        length = l;
        //actor.printing = (index == 0);
    }
    fun fix()
    {
        x = (index-length*0.5)*64+32;
        //Console.print("SBE "+x+" for "+index+"/"+length);
        transform.localPosition = Vector2(x, 0);
        actor.fix();
        brick.enabled = true;
        state = "idle";
    }
    fun collapse(i)
    {
        brick.enabled = false;
        here_distance = (i - index);
        angular = -Math.sign(here_distance)*180/(1+Math.abs(here_distance));
        actor.collapse(angular);
        ysp = 20/(1+Math.abs(here_distance));
        //Console.print("SBE angle "+angular+" for "+index+"/"+length);
        state = "collapsing";
    }
}
