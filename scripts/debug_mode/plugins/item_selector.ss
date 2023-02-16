// -----------------------------------------------------------------------------
// File: item_selector.ss
// Description: item selector plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin lets the user select, move and remove items of the level.

*/

using SurgeEngine;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Camera;
using SurgeEngine.Input;
using SurgeEngine.Audio.Sound;

object "Debug Mode - Item Selector" is "debug-mode-plugin", "awake", "private", "entity"
{
    transform = Transform();
    input = Input("default");
    selectionOrigin = Vector2.zero;
    selectionPoint = Vector2.zero;
    selectionActor = Actor("Debug Mode - Item Selector - Selection");
    selectedEntities = spawn("Debug Mode - Item Selector - Selected Entities");
    vibrateSfx = Sound("samples/vibrate.wav");
    trackedTouchId = -1;
    zindex = 0.0;
    warmUpTime = 0.5; // seconds

    state "main"
    {
        state = "ready";
    }

    state "disabled"
    {
    }

    state "ready"
    {
        selectionActor.visible = false;
    }

    state "warming up"
    {
        // cancel the selection if the mobile gamepad is touched
        if(SurgeEngine.mobile) {
            if(
                input.buttonDown("up") ||
                input.buttonDown("right") ||
                input.buttonDown("down") ||
                input.buttonDown("left") ||
                input.buttonDown("fire1")
            ) {
                state = "ready";
                return;
            }
        }

        // warm up time
        if(timeout(warmUpTime)) {
            vibrate();
            state = "selecting";
        }
    }

    state "selecting"
    {
        width = selectionPoint.x - selectionOrigin.x; // may be negative
        height = selectionPoint.y - selectionOrigin.y; // may be negative

        transform.localScale = Vector2(width / selectionActor.width, height / selectionActor.height);
        selectionActor.visible = true;
    }

    state "selected"
    {
        selectionActor.visible = false;

        // find bounding box (xs,ys,ws,hs) of the selection
        xs = Math.min(selectionOrigin.x, selectionPoint.x);
        ys = Math.min(selectionOrigin.y, selectionPoint.y);
        ws = Math.abs(selectionOrigin.x - selectionPoint.x);
        hs = Math.abs(selectionOrigin.y - selectionPoint.y);

        // find all selected entities
        entities = Level.childrenWithTag("entity");
        foreach(entity in entities) {
            if(!entity.hasTag("private") && !entity.hasTag("detached")) {
                entityTransform = entity.child("Transform") || entity.spawn("Transform");
                entityActor = Actor(entity.__name);
                entityActor.visible = false;

                // find bounding box (xe,ye,we,he) of the entity
                // what about the scale of the entity? not considered here
                xe = entityTransform.position.x - entityActor.hotSpot.x;
                ye = entityTransform.position.y - entityActor.hotSpot.y;
                we = entityActor.width;
                he = entityActor.height;

                // is this entity selected?
                if(boundingBoxTest(xs, ys, ws, hs, xe, ye, we, he)) {
                    selectedEntities.add(
                        entity,
                        xe, ye, we, he
                    );
                }

                entityActor.destroy();
            }
        }

        // go to the selected state only if something was selected
        if(selectedEntities.count > 0) {
            Console.print("Selected " + selectedEntities.count + " entities");
        }

        // we're ready for a new selection
        deselect();
        state = "ready";
    }

    fun onLoad(debugMode)
    {
        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.subscribe(this);

        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex;

        selectionActor.zindex = uiSettings.zindex;
        selectionActor.offset = selectionActor.hotSpot;
        selectionActor.visible = false;

        selectedEntities.onLoad(debugMode);
    }

    fun onUnload(debugMode)
    {
        deselect();
        selectedEntities.onUnload(debugMode);

        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.unsubscribe(this);
    }

    fun onTouchBegin(touch)
    {
        if(touch.position.y < 32)
            return;

        if(state == "ready") {
            trackedTouchId = touch.id;
            selectionActor.visible = true;

            selectionOrigin = Camera.screenToWorld(touch.position);
            selectionPoint = Camera.screenToWorld(touch.position);

            transform.position = selectionOrigin;
            transform.localScale = Vector2.zero;

            state = "warming up";
        }
    }

    fun onTouchEnd(touch)
    {
        if(trackedTouchId != touch.id)
            return;

        if(state == "warming up") {
            deselect();
            state = "ready";
        }
        else if(state == "selecting") {
            selectedEntities.blocked = false;
            state = "selected";
        }
    }

    fun onTouchMove(touch)
    {
        if(trackedTouchId != touch.id)
            return;

        if(state == "warming up") {
            deselect();
            state = "ready";
        }
        else if(state == "selecting") {
            selectedEntities.blocked = true;
            selectionPoint = Camera.screenToWorld(touch.position);
        }
    }

    fun boundingBoxTest(x1, y1, w1, h1, x2, y2, w2, h2) {
        dx = x2 - x1;
        dy = y2 - y1;

        return !(dx > w1 || dx < -w2 || dy > h1 || dy < -h2);
    }

    fun vibrate()
    {
        // well... this workaround signals that something has happened
        //if(SurgeEngine.mobile)
        vibrateSfx.play();
    }

    fun deselect()
    {
        trackedTouchId = -1;
        selectionActor.visible = false;
    }
}

object "Debug Mode - Item Selector - Selected Entities" is "private", "awake", "entity"
{
    public blocked = false;
    selectedEntities = [];
    trackedTouchId = -1;
    zindex = 0.0;

    fun onLoad(debugMode)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex + 200;

        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.subscribe(this);

        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.subscribe(this);

        camera = debugMode.plugin("Debug Mode - Camera");
        camera.subscribe(this);
    }

    fun onUnload(debugMode)
    {
        respawnAll();
        clear();

        camera = debugMode.plugin("Debug Mode - Camera");
        camera.unsubscribe(this);

        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.unsubscribe(this);

        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.unsubscribe(this);
    }

    fun get_count()
    {
        return selectedEntities.length;
    }

    fun add(entity, xe, ye, we, he)
    {
        // add selected entity
        selectedEntity = Level.spawnEntity("Debug Mode - Item Selector - Selected Entity", Vector2(xe, ye));
        selectedEntities.push(selectedEntity.init(
            entity,
            xe, ye, we, he,
            zindex
        ));
    }

    fun clear()
    {
        foreach(selectedEntity in selectedEntities)
            selectedEntity.destroy();

        selectedEntities.clear();
    }

    fun respawnAll()
    {
        // we do this to change the spawn point of all selected entities
        foreach(selectedEntity in selectedEntities)
            selectedEntity.respawn();
    }

    fun moveAll(deltaPosition)
    {
        foreach(selectedEntity in selectedEntities)
            selectedEntity.move(deltaPosition);
    }

    fun onTapScreen(position)
    {
        respawnAll();
        clear();
    }

    fun onTouchBegin(touch)
    {
        if(trackedTouchId != -1)
            return;

        trackedTouchId = touch.id;
    }

    fun onTouchEnd(touch)
    {
        if(trackedTouchId != touch.id)
            return;

        trackedTouchId = -1;
    }

    fun onTouchMove(touch)
    {
        if(trackedTouchId != touch.id)
            return;

        if(blocked)
            return;

        moveAll(touch.deltaPosition);
    }

    fun onCameraMove(deltaPosition)
    {
        moveAll(deltaPosition);
    }
}

object "Debug Mode - Item Selector - Selected Entity" is "private", "awake", "entity"
{
    transform = Transform();
    selection = spawn("Debug Mode - Item Selector - Selected Entity - Selection");
    entityName = "";
    entityActor = null;

    fun move(deltaPosition)
    {
        transform.translate(deltaPosition);
    }

    fun respawn()
    {
        // we respawn the object in order to change the spawn point
        entityPosition = transform.position.plus(entityActor.offset);
        entity = Level.spawnEntity(entityName, entityPosition);

        // FIXME public properties are not preserved...
    }

    fun init(entity, x, y, w, h, zindex)
    {
        entityTransform = entity.child("Transform") || entity.spawn("Transform");
        entityPosition = entityTransform.position;

        entityName = entity.__name;
        entityActor = Actor(entityName);
        entityActor.offset = entityPosition.minus(transform.position);
        entityActor.zindex = zindex;

        transform.position = Vector2(x, y);
        selection.init(w, h, zindex + 0.001);
        entity.destroy(); // temporarily destroy the entity

        return this;
    }
}

object "Debug Mode - Item Selector - Selected Entity - Selection" is "private", "awake", "entity"
{
    actor = Actor("Debug Mode - Item Selector - Selection");
    transform = Transform();

    fun init(width, height, zindex)
    {
        actor.zindex = zindex;
        actor.offset = actor.hotSpot; // top-left at (0,0)

        xscale = width / actor.width;
        yscale = height / actor.height;
        transform.scaleBy(xscale, yscale);

        return this;
    }
}