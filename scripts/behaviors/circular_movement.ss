// -----------------------------------------------------------------------------
// File: circular_movement.ss
// Description: circular movement behavior
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*
 * This behavior makes the entity perform a Circular
 * Movement on the 2D plane. A Circular Movement has
 * a radius, given in pixels, a movement rate, given
 * in cycles per second, and other parameters.
 *
 * Example:

using SurgeEngine.Actor;
using SurgeEngine.Behaviors.DirectionalMovement;

object "Simple Ball" is "entity"
{
    actor = Actor("Simple Ball");
    movement = CircularMovement();

    state "main"
    {
        movement.radius = 128; // using a radius of 128 pixels
        movement.rate = 0.25;  // it takes 4 seconds to complete one cycle
    }
}

 *
 * Parameters:
 *
 * - enabled: boolean. Indicates whether the movement is enabled or not (defaults to true).
 * - radius: number. A positive number given in pixels (e.g., 128 means a radius of 128 pixels).
 * - rate: number. A positive number given in cycles per second (1.0 means one cycle per second).
 * - clockwise: boolean. Indicates whether the movement is clockwise or counter-clockwise.
 * - scale: Vector2 object. Used to distort the circle. Vector2(1.0, 1.0) means no distortion (default).
 * - phaseOffset: number. A value given in degrees. Defaults to zero (180 means opposite phase).
 * - phase: number, readonly. A value given in degrees that indicates the current phase of the movement.
 * - entity: object, readonly. The entity associated with this behavior.
 */

 // -----------------------------------------------------------------------------
// The following code makes CircularMovement work.
// Think twice before modifying this code!
// -----------------------------------------------------------------------------
using SurgeEngine.Vector2;

object "CircularMovement" is "behavior"
{
    public readonly entity = parent;

    transform = null;
    radius = 0.0;
    rate = 0.0;
    angle = 0.0;
    sign = 1.0;
    scale = Vector2(1.0, 1.0);
    offset = 0.0;
    offsetDeg = 0;
    twopi = 2.0 * Math.pi;
    cachedV2 = null;

    state "main"
    {
        // setup
        if(transform == null)
            transform = entity.child("Transform") || entity.spawn("Transform");

        // move
        if(rate != 0) {
            dt = Time.delta;
            w = sign * twopi * rate;
            rw = radius * w;
            angle += w * dt;
            transform.move(
                -rw * Math.cos(angle + offset) * scale.x * dt,
                 rw * Math.sin(angle + offset) * scale.y * dt
            );
        }
    }

    state "disabled"
    {
        // no movement
    }

    // constructor
    fun constructor()
    {
        // behavior validation
        if(!entity.hasTag("entity"))
            Application.crash("Object \"" + entity.__name + "\" must be tagged \"entity\" to use " + this.__name + ".");
    }

    // enable/disable movement
    fun get_enabled()
    {
        return state != "disabled";
    }

    fun set_enabled(enabled)
    {
        state = enabled ? "main" : "disabled";
    }

    // phaseOffset, given in degrees
    fun get_phaseOffset()
    {
        return offsetDeg;
    }

    fun set_phaseOffset(deg)
    {
        offsetDeg = deg;
        offset = Math.deg2rad(deg);
    }

    // clockwise movement?
    fun get_clockwise()
    {
        return (sign < 0.0);
    }

    fun set_clockwise(cw)
    {
        sign = cw ? -1.0 : 1.0;
    }

    // movement phase, in degrees
    fun get_phase()
    {
        return (Math.rad2deg(angle) + 360) % 360;
    }

    fun set_phase(deg)
    {
        angle = Math.deg2rad(deg);
    }

    // movement rate, given in cycles per second
    fun get_rate()
    {
        return rate;
    }

    fun set_rate(r)
    {
        rate = Math.max(r, 0);
    }

    // movement radius, given in pixels
    fun get_radius()
    {
        return radius;
    }

    fun set_radius(r)
    {
        radius = Math.max(r, 0);
    }

    // movement scale, a Vector2 object
    fun get_scale()
    {
        return scale;
    }

    fun set_scale(v2)
    {
        if(v2 !== cachedV2) {
            if(typeof(v2) == "object" && v2.__name == "Vector2") {
                scale = v2.scaledBy(1.0); // clone object
                cachedV2 = v2;
            }
        }
    }
}