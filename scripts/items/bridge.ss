// -----------------------------------------------------------------------------
// File: bridge.ss
// Description: script for bridges
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Player;
using SurgeEngine.Actor;
using SurgeEngine.Brick;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Collisions.CollisionBox;

// Bridge
object "Bridge" is "entity", "gimmick"
{
    public length = 12;
    public anim = 1;

    bridgeCollider = CollisionBox(1, 1);
    transform = Transform();
    elements = [];
    leftCorner = null;
    rightCorner = null;
    prevMaxDepth = 0;
    smoothMaxDepth = 0;
    prevIndex = 0;
    smoothIndex = 0;
    timer = 0;

    state "main"
    {
        setup();
        state = "idle";
    }

    state "idle"
    {
        deepestElementIndex = -1;
        maxDepth = 0;
        numPlayers = 0;

        // compute the maximum depth of all bridge elements
        // according to the position of the players
        for(i = 0; i < Player.count; i++) { // for each player
            player = Player[i];
            if(!player.midair && bridgeCollider.collidesWith(player.collider)) { // player on the bridge
                numPlayers++;
                feet = Vector2(player.collider.center.x, player.collider.bottom);
                for(j = 0; j < elements.length; j++) {
                    if(elements[j].gotCollision(feet)) { // player on bridge element j
                        depth = elements[j].depth;
                        if(depth > maxDepth) {
                            deepestElementIndex = j;
                            maxDepth = depth;
                        }
                        break;
                    }
                }
            }
        }

        // smoothing the bridge
        if(deepestElementIndex >= 0) {
            if(Math.abs(prevMaxDepth - maxDepth) >= 1) {
                prevMaxDepth = maxDepth;
                timer = 0;
            }
            else if(Math.abs(prevIndex - deepestElementIndex) >= 1) {
                prevIndex = deepestElementIndex;
                timer = 0;
            }
            timer += Time.delta / 0.25;
            smoothMaxDepth = Math.lerp(smoothMaxDepth, maxDepth, timer);
            smoothIndex = (smoothIndex >= 0 && (numPlayers > 1 || Player.active.jumping)) ?
                Math.smoothstep(smoothIndex, deepestElementIndex, timer) :
                deepestElementIndex;
        }
        else {
            smoothIndex = deepestElementIndex;
            smoothMaxDepth = maxDepth;
        }

        // position the bridge elements
        for(j = 0; j < elements.length; j++)
            elements[j].updatePosition(smoothIndex, smoothMaxDepth);
    }

    fun setup()
    {
        // create the bridge elements
        length = Math.clamp(length, 6, 24); // at most 24 elements (entity not awake)
        for(i = 0; i < length; i++) {
            e = spawn("Bridge Element").setElement(i, length, anim);
            elements.push(e);
        }

        // create the corners
        leftCorner = spawn("Bridge Corner").setCorner(-1, length, anim);
        rightCorner = spawn("Bridge Corner").setCorner(length, length, anim);

        // setup the bridge collider
        bridgeCollider.width = elements[0].width * length;
        bridgeCollider.height = elements[0].height;
        bridgeCollider.setAnchor(0.5, 0.5);
        //bridgeCollider.visible = true;
    }

    fun clear()
    {
        // clear bridge
        if(leftCorner != null) {
            leftCorner.destroy();
            leftCorner = null;
        }

        if(rightCorner != null) {
            rightCorner.destroy();
            rightCorner = null;
        }

        for(i = 0; i < elements.length; i++) {
            elements[i].destroy();
            elements[i] = null;
        }
        elements = [];
    }
}

// Bridge Element
object "Bridge Element" is "entity", "private"
{
    public readonly depth = 0;
    public readonly index = 0;

    transform = Transform();
    actor = Actor("Bridge Element");
    brick = Brick("Bridge Element");
    collider = CollisionBox(actor.width, actor.height).setAnchor(0, 8 / actor.height);
    length = 1;
    offx = 0;
    offy = 0;
    timer = 0;
    ninety = Math.deg2rad(90);
    originalOffset = Vector2.zero;

    state "main"
    {
        //collider.visible = true;
    }

    fun setElement(idx, len, anim)
    {
        // set the animation
        actor.anim = anim;

        // set the element index
        index = idx;

        // store the length of the bridge
        length = len;

        // set the position of the element
        offx = (index - length / 2) * actor.width;
        originalOffset = Vector2(offx, 0);
        transform.localPosition = originalOffset;

        // compute its depth
        d1 = index + 1;
        d2 = length - index;
        depth = Math.min(d1, d2) * 2;

        // make this call chainable
        return this;
    }

    fun updatePosition(deepestElementIndex, maxDepth)
    {
        // if a player is on the bridge...
        if(deepestElementIndex >= 0) {
            // compute the depth factor of this element
            f1 = (index + 1) / (deepestElementIndex + 1);
            f2 = (length - index) / (length - deepestElementIndex);
            factor = Math.min(f1, f2);

            // update timer
            timer += Time.delta / 0.33;
            if(timer > 1)
                timer = 1;

            // update graphics
            targetFrame = Math.floor((timer * factor * factor) * (actor.animation.frameCount - 1));
            actor.animation.frame = targetFrame;

            // update position
            offy = maxDepth * Math.sin(ninety * factor);
            y = Math.smoothstep(0, offy, timer);
            dy = y - transform.localPosition.y;
            transform.move(0, dy);

            // move player(s)
            if(dy > 0.0) {
                for(j = 0; j < Player.count; j++) {
                    player = Player[j];
                    if(player.ysp >= 0 && collider.collidesWith(player.collider)) {
                        ddy = player == Player.active ? Math.max(0, dy - 0.7) : dy;
                        player.transform.move(0, ddy);
                    }
                }
            }
        }
        else if(timer > 0) {
            // update timer
            timer -= Time.delta / 0.25;
            if(timer < 0)
                timer = 0;

            // update graphics
            targetFrame = Math.floor((timer * timer) * (actor.animation.frameCount - 1));
            actor.animation.frame = Math.min(targetFrame, actor.animation.frame);

            // update position
            y = Math.smoothstep(0, offy, timer);
            dy = y - transform.localPosition.y;
            transform.move(0, dy);
        }
        else {
            actor.animation.frame = 0;
            transform.localPosition = originalOffset;
        }
    }

    fun gotCollision(position)
    {
        return collider.contains(position);
    }

    fun get_width()
    {
        return actor.width;
    }

    fun get_height()
    {
        return actor.height;
    }

    fun constructor()
    {
        brick.type = "cloud";
    }
}

// Bridge Corner
object "Bridge Corner" is "entity", "private"
{
    actor = null;

    state "main"
    {
    }

    fun setCorner(index, length, anim)
    {
        // configure the actor
        actor = Actor((index > 0) ? "Bridge Corner Right" : "Bridge Corner Left");
        actor.anim = anim;
        actor.zindex = 0.51; // appear in front of the player
        actor.offset = Vector2((index - length / 2) * actor.width, 0);

        // return the object
        return this;
    }
}