// -----------------------------------------------------------------------------
// File: hydra.ss
// Description: Hydra Boss (Waterworks)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Audio.Sound;
using SurgeEngine.Video.Screen;
using SurgeEngine.Events.Event;
using SurgeEngine.Behaviors.DirectionalMovement;
using SurgeEngine.Behaviors.CircularMovement;
using SurgeEngine.Collisions.CollisionBox;
using SurgeEngine.Collisions.CollisionBall;

// Hydra is the Waterworks Zone Boss
object "Hydra" is "entity", "awake", "boss"
{
    public onDefeat = Event();
    public hp = 5; // health points

    actor = Actor("Hydra");
    collider = CollisionBall(48);
    movement = CircularMovement();
    fall = DirectionalMovement();
    transform = Transform();
    hit = Sound("samples/bosshit.wav");
    electric = Sound("samples/electric_bulb.wav");
    appear = Sound("samples/teleport_appear.wav");
    disappear = Sound("samples/teleport_disappear.wav");
    chargeup = Sound("samples/charging_up.wav");
    aim = null;
    teleportTime = 2.0; // seconds
    explosionTime = 2.0; // seconds

    state "main"
    {
        // setup
        actor.zindex = 0.45;
        //collider.visible = true; // debug

        movement.rate = 0.25;
        movement.radius = 16;
        movement.scale = Vector2.up; // up-down movement
        movement.enabled = false;

        fall.direction = Vector2.down;
        fall.speed = 0;
        fall.enabled = false;

        // done
        state = "waiting";
    }

    state "waiting"
    {
        //
        // waiting for the boss fight
        //
        // the boss needs to be activated
        // by calling function activate()
        //
    }

    state "resting"
    {
        actor.anim = 0;
    }

    state "charging"
    {
        actor.anim = 1;
    }

    state "getting hit"
    {
        actor.anim = 2;
        if(timeout(0.5)) {
            if(hp > 0)
                teleport();
            else
                explode();
        }
    }

    state "teleporting"
    {
        if(timeout(teleportTime)) {
            transform.position = Vector2(
                Player.active.transform.position.x,
                transform.position.y
            );
            actor.visible = true;
            aim.show();
            appear.play();
            state = "resting";
        }
    }

    state "waiting to teleport"
    {
        actor.anim = 0;
        if(timeout(1.5)) {
            if(nearPlayer(Player.active, Screen.width * 0.45))
                rest(); // too close, no need to teleport
            else
                teleport();
        }
    }

    state "exploding"
    {
        actor.anim = 3;
        if(timeout(explosionTime))
            state = "defeated";
    }

    state "defeated"
    {
        actor.anim = 4;
        fall.enabled = true;
        fall.speed += Level.gravity * Time.delta;

        if(timeout(3.0)) {
            onDefeat.call();
            state = "done";
        }
    }

    state "done"
    {
    }

    // activate the boss
    fun activate()
    {
        if(state == "waiting") {
            aim = spawn("Hydra's Aim");
            movement.enabled = true;
            state = "resting";
        }
    }

    fun charge()
    {
        if(state != "charging") {
            chargeup.play();
            electric.play();
            state = "charging";
        }
    }

    fun rest()
    {
        state = "resting";
    }

    fun getHit(player)
    {
        if(state != "getting hit") {
            hp--;
            hit.play();
            player.bounceBack(actor);
            player.xsp = -player.xsp;
            state = "getting hit";
        }
    }

    fun teleport()
    {
        if(state != "teleporting") {
            disappear.play();
            aim.hide();
            actor.visible = false;
            state = "teleporting";
        }
    }

    fun restAndTeleport()
    {
        state = "waiting to teleport";
    }

    fun nearPlayer(player, threshold)
    {
        dist = Math.abs(transform.position.x - player.transform.position.x);
        return dist <= threshold;
    }

    fun explode()
    {
        if(state != "exploding") {
            Level.spawnEntity(
                "Explosion Combo",
                transform.position
            ).setSize(120, 60).setDuration(explosionTime);
            movement.enabled = false;
            aim.disappear();
            state = "exploding";
        }
    }

    fun onCollision(otherCollider)
    {
        if(state == "resting" || state == "charging" || state == "waiting to teleport") {
            if(otherCollider.entity.hasTag("player")) {
                player = otherCollider.entity;
                if(state != "charging" && player.attacking)
                    getHit(player);
                else
                    player.getHit(actor);
            }
        }
    }
}

// Hydra's Aim determine where the boss will attack
// basic states: defend, chase player, charge, attack, rest, pull back, hidden, disappearing
object "Hydra's Aim" is "private", "entity", "awake"
{
    public readonly transform = Transform();
    
    hydra = parent;
    player = Player.active;
    t = 0.0; // a value in [0, 1]
    orb = [];

    // configuration
    orbCount = 8;
    defendDuration = 3.0;
    chaseDuration = 2.0;
    chargeDuration = 2.0;
    pullBackDuration = 1.0;
    restDuration = 3.0;
    minRadius = 16;
    maxRadius = 64;

    state "main"
    {
        // create orbs
        for(i = 0; i < orbCount; i++) {
            obj = spawn("Hydra's Orb").setup(i, orbCount);
            orb.push(obj);
        }

        // done
        state = "defending";
    }

    state "defending"
    {
        transform.localPosition = Vector2.zero;
        if(timeout(defendDuration)) {
            if(hydra.nearPlayer(Player.active, Screen.width))
                chasePlayer(Player.active);
            else
                hydra.teleport();
        }
    }

    state "chasing player"
    {
        // update timer
        t += Time.delta / chaseDuration;
        if(t >= 1.0)
            state = "charging attack";

        // update orbs
        t = Math.clamp(t, 0, 1);
        for(i = 0; i < orbCount; i++)
            orb[i].radius = minRadius + (maxRadius - minRadius) * (1 - t);

        // update position
        transform.position = Vector2(
            Math.smoothstep(transform.position.x, player.transform.position.x, t),
            transform.position.y
        );
    }

    state "charging attack"
    {
        if(timeout(chargeDuration))
            state = "attacking";
    }

    state "attacking"
    {
        Level.spawnEntity("Hydra's Lighting Strike", transform.position);
        hydra.restAndTeleport();
        state = "resting";
    }

    state "resting"
    {
        if(timeout(restDuration))
            pullBack();
    }

    state "pulling back"
    {
        // update timer
        t += Time.delta / pullBackDuration;
        if(t >= 1.0)
            state = "defending";

        // update orbs
        t = Math.clamp(t, 0, 1);
        for(i = 0; i < orbCount; i++)
            orb[i].radius = minRadius + (maxRadius - minRadius) * t;

        // update position
        transform.localPosition = Vector2(
            Math.smoothstep(transform.localPosition.x, 0.0, t),
            0.0
        );
    }

    state "hidden"
    {
        // do nothing
    }

    state "disappearing"
    {
        // do nothing
    }

    fun pullBack()
    {
        t = 0;
        state = "pulling back";
    }

    fun chasePlayer(p)
    {
        t = 0;
        player = p;
        hydra.charge();
        state = "chasing player";
    }

    fun disappear()
    {
        if(state != "disappearing") {
            for(i = 0; i < orbCount; i++)
                orb[i].disappear();
            state = "disappearing";
        }
    }

    // hide orbs
    fun hide()
    {
        state = "hidden";
        for(i = 0; i < orbCount; i++)
            orb[i].hide();
    }

    // show orbs
    fun show()
    {
        state = "defending";
        for(i = 0; i < orbCount; i++) {
            orb[i].radius = maxRadius;
            orb[i].show();
        }
    }
}

// Hydra's Orb is an orb that damages
// the player if touched
object "Hydra's Orb" is "private", "entity", "awake"
{
    public radius = 64;

    actor = Actor("Hydra's Orb");
    movement = CircularMovement();
    fall = DirectionalMovement();
    collider = CollisionBall(8);

    state "main"
    {
        // setup
        movement.rate = 1.0;
        fall.enabled = false;
        fall.direction = Vector2.down;
        fall.speed = 0;
        actor.zindex = 0.451;
        state = "active";
    }

    state "active"
    {
        // orb: circular motion
        movement.radius = radius;
        movement.center = parent.transform.position;
    }

    state "disappearing"
    {
        // be affected by gravity
        fall.speed += Level.gravity * Time.delta;
    }

    fun setup(id, orbCount) // 0 <= id < orbCount
    {
        actor.anim = id % 2;
        movement.phaseOffset = 360 * id / orbCount;
        return this;
    }

    fun show()
    {
        actor.visible = true;
    }

    fun hide()
    {
        actor.visible = false;
    }

    fun disappear()
    {
        if(state != "disappearing") {
            movement.enabled = false;
            fall.enabled = true;
            state = "disappearing";
        }
    }

    fun onCollision(otherCollider)
    {
        if(state != "disappearing" && actor.visible) {
            if(otherCollider.entity.hasTag("player")) {
                player = otherCollider.entity;
                player.getHit(actor);
            }
        }
    }
}

// Hydra's Lighting Strike
// damages the player if touched
object "Hydra's Lighting Strike" is "private", "entity", "awake"
{
    actor = Actor("Hydra's Lighting Strike");
    collider = CollisionBox(128, 480);
    transform = Transform();
    discharge = Sound("samples/discharge.wav");
    duration = 0.5; // seconds
    scale = 1.0;

    state "main"
    {
        actor.zindex = 1.0;
        discharge.play();
        state = "disappearing";
    }

    state "disappearing"
    {
        transform.localScale = Vector2(scale, 1);
        scale -= Time.delta / duration;
        if(scale <= 0)
            destroy();
    }

    fun onCollision(otherCollider)
    {
        if(otherCollider.entity.hasTag("player")) {
            player = otherCollider.entity;
            if(scale > 0.3)
                player.getHit(actor);
        }
    }
}