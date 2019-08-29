// -----------------------------------------------------------------------------
// File: lady_bugsy.ss
// Description: Lady Bugsy enemy script
// Author: Alexandre Martins <http://opensurge2d.org> (based on Celdecea's Lady Bugsy)
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.Platformer;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Collisions.CollisionBall;

// Lady Bugsy is a baddie that moves and shoots slime bullets
object "Lady Bugsy" is "entity", "enemy"
{
    actor = Actor("Lady Bugsy");
    enemy = Enemy();
    transform = Transform();
    platformer = Platformer();
    bullets = 0; // number of bullets
    sfx = Sound("samples/shot.wav");
    gunSpot = Vector2(0.5, -0.37); // relative to the hot spot

    state "main"
    {
        platformer.speed = 30;
        state = "moving";
    }

    state "moving"
    {
        actor.anim = 1;
        platformer.walk();

        if(timeout(2.5)) {
            bullets = 3;
            shoot();
        }
    }

    state "shooting"
    {
        actor.anim = 2;
        platformer.stop();

        if(actor.animation.finished) {
            bullets--;
            state = (bullets > 0) ? "cooldown" : "moving";
        }
    }

    state "cooldown"
    {
        actor.anim = 0;
        if(timeout(0.5))
            shoot();
    }

    fun shoot()
    {
        // create bullet
        bulletDirection = (platformer.direction > 0) ? Vector2.right : Vector2.left;
        bulletPosition = transform.position.translatedBy(
            actor.width * gunSpot.x * bulletDirection.x,
            actor.height * gunSpot.y
        );
        Level.spawnEntity("Lady Bugsy Bullet", bulletPosition).setDirection(bulletDirection);

        // play sound
        sfx.play();

        // change state
        state = "shooting";
    }
}

object "Lady Bugsy Bullet" is "disposable", "private", "entity"
{
    actor = Actor("Lady Bugsy Bullet");
    movement = DirectionalMovement();
    collider = CollisionBall(4.0);

    state "main"
    {
        movement.speed = 60;
    }

    fun setDirection(direction)
    {
        movement.direction = direction;
        actor.hflip = (direction.x < 0);
        return this;
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            player.hit(actor);
        }
    }
}