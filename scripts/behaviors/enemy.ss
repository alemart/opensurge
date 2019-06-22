// -----------------------------------------------------------------------------
// File: enemy.ss
// Description: enemy behavior script
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*
 * A baddie is an entity that is a simple enemy:
 *
 * It will hurt the player if touched, unless the player is attacking
 * (jumping, rolling, etc.) In this case the baddie will be destroyed
 * with an explosion, giving the player a certain score.
 *
 * Object "Enemy" in this script codifies that behavior, but is not
 * a concrete baddie itself. You may use it to script your own baddies.
 *
 * HOW TO SCRIPT A BADDIE:
 *
 * 0. Make sure you have the graphics and the sprite file (.spr) ready,
 *    before you begin with SurgeScript
 * 1. Your object should be tagged (at least): "entity", "enemy"
 * 2. Spawn an Actor for the graphics and an Enemy object for the behavior.
 *
 *    Example:
 *

using SurgeEngine.Actor;
using SurgeEngine.Behavior.Enemy;
using SurgeEngine.Behavior.Platformer;

object "My Baddie" is "entity", "enemy"
{
    actor = Actor("My Baddie"); // handles the graphics
    enemy = Enemy(); // handles the behavior
    platformer = Platformer().walk(); // make it walk

    state "main"
    {
        // you may give it some movement
        // using a platformer, or do whatever
        // you want (see the SurgeScript docs,
        // available at our website)
    }
}

 *
 * 3. optionally, change the properties of the Enemy behavior. Example:
 *

object "My Baddie" is "entity", "enemy"
{
    actor = Actor("My Baddie"); // handles the graphics
    enemy = Enemy(); // handles the behavior

    // ...

    fun constructor()
    {
        enemy.score = 200;
    }
}

 *
 * 4. additionally, you may write functions onEnemyAttack(player) and
 *    onEnemyDestroy(player) if you want to catch those events. Example:
 *

object "My Baddie" is "entity", "enemy"
{
    actor = Actor("My Baddie");
    enemy = Enemy();

    state "main"
    {
        // ... your code ...
    }

    fun onEnemyAttack(player)
    {
        // the player has just
        // got hit by the enemy
    }

    fun onEnemyDestroy(player)
    {
        // the enemy has just been
        // destroyed by the player
    }
}

 *
 * 5. Have fun!!!
 *
 */

// -----------------------------------------------------------------------------
// The following code makes the Enemy behavior work.
// Think twice before modifying this code!
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Collisions.CollisionBox;

object "Enemy" is "private", "entity", "behavior"
{
    public score = 100; // how many points should be gained once the player defeats this baddie
    public invincible = false; // should this baddie hit the player even if the player is attacking?
    public enabled = true; // is this behavior enabled?
    public readonly collider = CollisionBox(32, 32);
    sfx = Sound("samples/destroy.wav");
    transform = Transform();
    skipAutodetect = false;
    actor = null;

    state "main"
    {
        actor = sibling("Actor");
        state = skipAutodetect ? "idle" : "autodetect";
    }

    state "autodetect"
    {
        // autodetect collider size
        if(actor != null) {
            hotspot = actor.animation.hotspot;
            collider.width = actor.width * 0.8;
            collider.height = actor.height * 0.8;
            collider.setAnchor(
                hotspot.x / actor.width,
                hotspot.y / actor.height
            );
        }

        // done
        state = "idle";
    }

    state "idle"
    {
    }

    fun constructor()
    {
        // behavior validation
        if(!parent.hasTag("entity") || !parent.hasTag("enemy"))
            Application.crash("Object \"" + parent.__name + "\" must be tagged \"entity\", \"enemy\" to use " + this.__name + ".");
    }

    fun onCollision(otherCollider)
    {
        if(enabled && otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(player.attacking && (!invincible || player.invincible)) {
                // impact the player
                player.score += Math.max(0, score);
                if(player.midair) {
                    if(actor != null)
                        player.bounceBack(actor);
                    else
                        player.bounce(null);
                }

                // destroy the enemy
                Level.spawnEntity("Explosion", collider.center);
                sfx.play();

                // notify & destroy parent
                if(parent.hasFunction("onEnemyDestroy"))
                    parent.onEnemyDestroy(player);
                parent.destroy();
            }
            else if(!player.invincible) {
                // hit the player
                player.hit(actor);

                // notify parent
                if(parent.hasFunction("onEnemyAttack"))
                    parent.onEnemyAttack(player);
            }
        }
    }



    // --- MODIFIERS ---

    // set the boundaries of the collider (all coordinates in pixels,
    // relative to the parent object. Example: setBounds(-8, -16, 8, 16))
    // this is autodetected, but can be manually adjusted as well
    fun setBounds(left, top, right, bottom)
    {
        x1 = Math.min(left, right);
        y1 = Math.min(top, bottom);
        x2 = left + right - x1;
        y2 = top + bottom - y1;
        if(x1 == x2 || y1 == y2) // invalid coordinates
            return this;

        collider.width = x2 - x1;
        collider.height = y2 - y1;
        collider.setAnchor(
            -x1 / (x2 - x1),
            -y1 / (y2 - y1)
        );

        skipAutodetect = true;
        return this;
    }
}