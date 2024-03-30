// -----------------------------------------------------------------------------
// File: marmotred.ss
// Description: Chain Marmot enemy script
// Author: Cody Licorish <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.Platformer;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Player;
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.Level;
using SurgeEngine.Collisions.CollisionBox;

object "ThrowingMarmot" is "behavior"
{
    public platformer = null;
    public readonly carrying = true;
    direction = 1;
    public maxDistance = 100;
    locked_player = null;
    public activeWeapon = null;

    state "main" {
      state = "waiting";
      if (platformer && platformer.direction != direction) {
        set_direction(platformer.direction);
      }
    }
    state "waiting" {
      if (platformer && platformer.direction != direction) {
        /* no checks while switching direction */
        set_direction(platformer.direction);
      } else {
        /* check if the player is on the side the marmot is facing */
        throw_check = false;
        active_player = Player.active;
        if (!active_player.dying) {
          dist_vec = active_player.transform.position.minus(
              parent.transform.position);
          dist_x = dist_vec.x;
          if (direction == -1 && dist_x < 0) {
            throw_check = true;
          } else if (direction == +1 && dist_x > 0) {
            throw_check = true;
          }
          if (throw_check) {
            if (dist_vec.length < maxDistance) {
              locked_player = active_player;
              //Console.print("throw check passed!");
              state = "cool down";
            }
          }
        } else {
          /* ignore a dying player */
        }
      }
    }
    state "cool down" {
      // await a throw request
    }
    fun setPlatformer(pl) {
      platformer = pl;
      return this;
    }
    fun get_direction() {
      return direction;
    }
    fun set_direction(dir) {
      direction = dir;
    }
    fun get_lockedOn() {
      return locked_player != null;
    }
    fun throwWeapon(name, position) {
      if (locked_player && activeWeapon == null) {
        target_point = locked_player.transform.position;
        locked_player = null;
        new_weapon = Level.spawnEntity(name, position);
        new_weapon.target = target_point;
        activeWeapon = new_weapon;
        return new_weapon;
      } else {
        /*Console.print("refusing to throw: "+
          locked_player+","+typeof(activeWeapon));*/
        return null;
      }
    }
    fun resumeSearch() {
      if (state == "cool down") {
        locked_player = null;
        state = "waiting";
        activeWeapon = null;
      }
    }
}

// Red Marmot's Chain is the chain that the baddie throws
object "RedMarmotChain" is "entity", "private", "awake"
{
    actor = Actor("RedMarmotChain");
    collider = CollisionBox(20,4);
    public transform = Transform();
    public target = null;
    speed = 10;
    direction = Vector2.right;
    velocity = Vector2.zero;
    accel = Vector2.zero;
    public returnEntity = null;
    seekTime = 10;
    public returnTime = 10;
    public returnOffset = Vector2.zero;
    returnCountdown = 0;
    state "main" {
      //collider.visible = true;
      velocity = direction.scaledBy(speed);
      /* compute acceleration needed to reach target */if (target) {
        expected_seek_time = seekTime-0.125;
        if (expected_seek_time < 0.125) {
          expected_seek_time = 0.125;
        }
        computeAccelTo(target, expected_seek_time);
      }
      state = "seek";
    }
    state "seek" {
      if (target != null) {
        velocity = velocity.plus(accel.scaledBy(Time.delta));
      }
      transform.translate(velocity.scaledBy(Time.delta));
      if (timeout(seekTime)) {
        turnAround();
      }
    }
    state "go home" {
      if (returnEntity != null) {
        new_target = returnEntity.transform.position.plus(returnOffset);
        if (returnCountdown <= 0.0) {
          /* force arrival at return entity */
          velocity = Vector2.zero;
          transform.position = new_target;
            ;
        } else {
          computeAccelTo(new_target, returnCountdown);
          velocity = velocity.plus(accel.scaledBy(Time.delta));
          transform.translate(velocity.scaledBy(Time.delta));
          returnCountdown -= Time.delta;
        }
      } else {
        /* show that the chain was deactivated */
        explosion = Level.spawnEntity("Explosion", transform.position);
        destroy();
      }
    }
    fun get_direction() {
      return direction;
    }
    fun set_direction(vec2) {
      if (state == "main") {
        // only allow setting velocity in first frame
        direction = vec2.normalized();
      }
    }
    fun get_speed() {
      return speed;
    }
    fun set_speed(value) {
      if (state == "main") {
        // only allow setting velocity in first frame
        speed = value;
      }
    }
    fun get_seekTime() {
      return seekTime-0.25;
    }
    fun set_seekTime(value) {
      if (value < 0.25) {
        seekTime = 0.25;
      } else {
        seekTime = value+0.25;
      }
    }
    fun onCollision(other) {
      if (state == "seek") {
        /* hit a player? */
        if (other.entity.hasTag("player")) {
          player = other.entity;
          //Console.print("touched player "+player.name);
          if (!player.attacking) {
            player.getHit(actor);
          } else {
            accel = accel.scaledBy(-1);
            velocity = velocity.scaledBy(-1);
          }
          turnAround();
        }
      } else if (state == "go home") {
        /* reach the home entity? */
        if (other.entity == returnEntity) {
          //Console.print("touched return entity");
          if (returnEntity.hasFunction("onReturnWeapon")) {
            returnEntity.__invoke("onReturnWeapon",[]);
          }
          destroy();
        }
      }
    }
    fun turnAround() {
      state = "go home";
      actor.anim = 1;
      returnCountdown = returnTime;
    }
    fun computeAccelTo(vec2, expected_time) {
      accel = vec2.minus(transform.position)
        .minus(velocity.scaledBy(expected_time))
        .scaledBy(2/(expected_time*expected_time));
    }
}

// Red Marmot is a baddie that walks around
object "RedMarmot" is "entity", "enemy"
{
    public transform = Transform();
    actor = Actor("RedMarmot");
    collider = CollisionBox(20,20);
    enemy = Enemy();
    platformer = Platformer().walk();
    carrythrower = spawn("ThrowingMarmot")
        .setPlatformer(platformer);

    state "main"
    {
        platformer.speed = 30;
        actor.anim = 1;
        state = "waiting";
        //collider.visible = true;
        collider.setAnchor(0.5,1.5);
        carrythrower.maxDistance = 150;
    }
    
    state "waiting"
    {
        checkAnimation();
        if (carrythrower.lockedOn) {
          state = "prepare throw";
          platformer.stop();
          /* switch to throwing animation */{
            actor.anim += 2;
          }
        }
    }

    state "prepare throw"
    {
      // wait for end of throwing animation
      if (actor.animation.finished) {
        position = transform.position.translatedBy(0,-20);
        new_weapon = carrythrower.throwWeapon("RedMarmotChain", position);
        if (new_weapon) {
          //Console.print("weapon away!");
          // only initialize the chain if new
          if (platformer.direction > 0) {
            new_weapon.direction = Vector2(+1,0);
          } else {
            new_weapon.direction = Vector2(-1,0);
          }
          new_weapon.speed = 50;
          new_weapon.seekTime = 1;
          new_weapon.returnTime = 1;
          new_weapon.returnEntity = this;
          new_weapon.returnOffset = Vector2(0,-20);
          /* switch to thrown animation */{
            actor.anim += 2;
          }
        }/* else {
          Console.print("nothing to throw");
        }*/
        state = "has thrown";
      }
    }

    state "has thrown"
    {
      if (actor.animation.finished) {
        state = "wait return";
        platformer.walk();
        actor.anim = 0;
      }
    }

    state "wait return" {
      // nothing to do here
    }

    fun checkAnimation() {
        if (platformer.leftWall || platformer.leftLedge) {
          // switch to right-facing animation
          actor.anim = 1;
        } else if (platformer.rightWall || platformer.rightLedge){
          // switch to left-facing animation
          actor.anim = 2;
        }
    }

    fun onReturnWeapon() {
      if (state == "wait return" || state == "has thrown") {
        state = "waiting";
        carrythrower.resumeSearch();
        if (platformer.direction > 0) {
          // switch to right-facing animation
          actor.anim = 1;
        } else {
          // switch to left-facing animation
          actor.anim = 2;
        }
      }
    }

    fun onEnemyDestroy(player) {
      if (carrythrower.activeWeapon) {
        carrythrower.activeWeapon.returnEntity = null;
      }
    }
}
