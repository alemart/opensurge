// -----------------------------------------------------------------------------
// File: item_selector.ss
// Description: item selector plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin lets the user select items of the level.

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
    vibrateSfx = Sound("samples/vibrate.wav");
    touchId = -1;
    zindex = 0.0;
    warmUpTime = 1.0; // seconds

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
        if(timeout(warmUpTime)) {
            vibrate();
            state = "selecting";
        }

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
            }
        }
    }

    state "selecting"
    {
        width = selectionPoint.x - selectionOrigin.x; // may be negative
        height = selectionPoint.y - selectionOrigin.y; // may be negative

        transform.localScale = Vector2(width / selectionActor.width, height / selectionActor.height);
        selectionActor.visible = true;
    }

    state "just selected"
    {
        Console.print("Selected");
        selectedEntities = [];

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
                    selectedEntities.push({
                        "rect": [xe, ye, we, he],
                        "initialLocation": entityTransform.position.scaledBy(1),
                        "entityId": entity
                    });
                }

                entityActor.destroy();
            }
        }

        // go to the selected state only if something was selected
        if(selectedEntities.length > 0) {
            Console.print("Selected " + selectedEntities.length + " entities");
            foreach(e in selectedEntities) Console.print(e["entityId"].__name);
            state = "selected";
            return;
        }

        // nothing was selected.
        state = "ready";
    }

    state "selected"
    {
        selectionActor.visible = false;
        touchId = -1;
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
    }

    fun onUnload(debugMode)
    {
        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.unsubscribe(this);
    }

    fun onTouchBegin(touch)
    {
        if(state == "ready") {
            touchId = touch.id;
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
        if(touchId != touch.id)
            return;

        if(state == "warming up" || state == "selecting") {
            touchId = -1;
            selectionActor.visible = false;
            state = state == "selecting" ? "just selected" : "ready";
            return;
        }
    }

    fun onTouchMove(touch)
    {
        if(touchId != touch.id)
            return;

        if(state == "warming up") {
            touchId = -1;
            selectionActor.visible = false;
            state = "ready";
            return;
        }

        selectionPoint = Camera.screenToWorld(touch.position);
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
}