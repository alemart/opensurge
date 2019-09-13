// -----------------------------------------------------------------------------
// File: harrier.ss
// Description: Swooping Harrier enemy script
// Author: Cody Licorish <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Vector2;
using SurgeEngine.Behaviors.Enemy;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Collisions.Sensor;
using SurgeEngine.Transform;
using SurgeEngine.Player;

// Swoop Harrier is a baddie that glides, then swoops on sight
object "SwoopHarrier" is "entity", "enemy"
{
    actor = Actor("SwoopHarrier");
    enemy = Enemy();
    movement = DirectionalMovement();
    floorSensor = Sensor(0, 128, 1, 32);
    rightWallSensor = Sensor(24, 0, 1, 128);
    leftWallSensor = Sensor(-24, 0, 1, 128);
    ownDirectionX = Vector2.right.x;
    target_point = null;
    transform = Transform();
    public maxDistance = 150;
    accel = Vector2.zero;
    public swoopSpeed = 200;
    public ascendSpeed = 30; //20;
    public flySpeed = 120;
    public trapCheck = 1;
    swoop_time = 1;
    last_frame_switch = 50;

    state "main"
    {
        movement.direction = Vector2.right;
        movement.speed = flySpeed;
        //floorSensor.visible = true;
        //rightWallSensor.visible = true;
        //leftWallSensor.visible = false;
        state = "fly";
        checkDirection();
    }
    state "fly" {
        movement.speed = flySpeed;
        checkDirection() || searchPlayer();
    }
    state "ascend"
    {
        movement.speed = ascendSpeed;
        checkDirection();
    }
    state "prepare swoop"
    {
        if (actor.animation.finished) {
            movement.speed = swoopSpeed;
            movement.direction = Vector2.down;
            if (Math.abs(target_point.minus(transform.position).x) < 16) {
                state = "quick drop";
            } else {
                state = "swoop";
                computeAccelTo(target_point, swoop_time);
            }
        }
    }
    state "swoop"
    {
        actor.anim = 6;
        /* accelerate the directional movement */{
            velocity = movement.direction.scaledBy(movement.speed)
              .plus(accel.scaledBy(Time.delta));
            movement.direction = velocity.normalized();
            movement.speed = velocity.length;
            if (ownDirectionX > 0) {
              /* compute right swoop angle */
              transform.angle = -velocity.angle;
              actor.hflip = false;
            } else {
              /* compute left swoop angle */
              transform.angle = 180-velocity.angle;
              actor.hflip = true;
            }
        }
        if (timeout(swoop_time)) {
            state = "chill";
            transform.angle = 0;
            set_direction(Vector2(ownDirectionX, 0));
        }
    }
    state "chill" {
        /* decelerate */
        actor.anim = 7;
        next_speed = movement.speed - swoopSpeed*2*Time.delta;
        if (next_speed <= 0.5) {
            /* done */
            last_frame_switch = 0;
            checkDirection();
        } else {
            movement.speed = next_speed;
        }
    }
    state "quick drop"
    {
        if (transform.position.y >= target_point.y) {
            /* done */
            checkDirection();
        }
    }
    state "slow hover"
    {
        /* just sit there */
        actor.anim = 5;
        searchPlayer();
        if (timeout(5)) {
            state = "slow hover turnaround";
        }
    }
    state "slow hover turnaround"
    {
        ownDirectionX = -ownDirectionX;
        actor.hflip = (ownDirectionX < 0);
        state = "slow hover";
    }

    fun set_swoopTime(t) {
        if (typeof(t) != "number") {
            swoop_time = 1.0;
        } else if (t < 1.0) {
            swoop_time = 1.0;
        } else swoop_time = 1.0;
    }
    fun get_swoopTime() {
        return swoop_time;
    }
    fun set_direction(vec2) {
        movement.direction = vec2;
        if (vec2.x > 0)
            ownDirectionX = 1;
        else if (vec2.x < 0)
            ownDirectionX = -1;
        checkActor();
    }
    fun checkActor() {
        next_anim = 0;
        y = movement.direction.y;
        if (ownDirectionX > 0) {
            next_anim = 0;
        } else if (ownDirectionX < 0) {
            next_anim = 1;
        }
        if (y < 0) {
            /* ascending */
            next_anim += 2;
        }
        actor.anim = next_anim;
        actor.hflip = ((next_anim%2) == 1);
        return;
    }
    fun clearFrameSwitch() {
        last_frame_switch = trapCheck+50;
    }
    fun checkDirection() {
        if (state == "ascend") {
            clearFrameSwitch();
            if (!(leftWallSensor.status || rightWallSensor.status)) {
                if (floorSensor.status) {
                    state = "fly";
                    set_direction(Vector2(ownDirectionX,0));
                } else {
                    state = "slow hover";
                    set_direction(Vector2.zero);
                    return true;
                }
            } else return true;
        }
        if (ownDirectionX > 0) {
            if (rightWallSensor.status || (!floorSensor.status)) {
                if (last_frame_switch < trapCheck) {
                    state = "ascend";
                    set_direction(Vector2.up);
                    movement.speed = ascendSpeed;
                    return true;
                } else {
                    state = "fly";
                    set_direction(Vector2.left);
                    movement.speed = flySpeed;
                    //rightWallSensor.visible = false;
                    //leftWallSensor.visible = true;
                    last_frame_switch = 0;
                    return true;
                }
            }
        } else {
            if (leftWallSensor.status || (!floorSensor.status)) {
                if (last_frame_switch < trapCheck) {
                    state = "ascend";
                    set_direction(Vector2.up);
                    movement.speed = ascendSpeed;
                    return true;
                } else {
                    state = "fly";
                    set_direction(Vector2.right);
                    movement.speed = flySpeed;
                    //leftWallSensor.visible = false;
                    //rightWallSensor.visible = true;
                    last_frame_switch = 0;
                    return true;
                }
            }
        }
        last_frame_switch += Time.delta;
        return false;
    }
    fun searchPlayer() {
        active_player = Player.active;
        if (!active_player.dying) {
            swoop_check = false;
            dist_vec = active_player.transform.position.minus(
                transform.position);
            dist_x = dist_vec.x;
            if (ownDirectionX == -1 && dist_x < 0) {
                swoop_check = true;
            } else if (ownDirectionX == +1 && dist_x > 0) {
                swoop_check = true;
            }
            if (swoop_check) {
                if (dist_vec.length < maxDistance) {
                    prepareSwoop(active_player);
                }
            }
        } else {
            /* ignore a dying player */
        }
        return true;
    }
    fun prepareSwoop(player_lock) {
        actor.anim = 4;
        movement.speed = 0;
        state = "prepare swoop";
        target_point = player_lock.transform.position;
        return;
    }
    fun computeAccelTo(vec2, expected_time) {
      velocity = movement.direction.scaledBy(movement.speed);
      accel = vec2.minus(transform.position)
          .minus(velocity.scaledBy(expected_time))
          .scaledBy(2/(expected_time*expected_time));
    }
}
