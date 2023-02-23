// -----------------------------------------------------------------------------
// File: item_selector.ss
// Description: item selector plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin lets the user select, move and remove items of the level. Follow
the example below to listen to events:

object "Debug Mode - Item Selector Watcher" is "debug-mode-plugin"
{
    fun onLoad(debugMode)
    {
        itemSelector = debugMode.plugin("Debug Mode - Item Selector");
        itemSelector.subscribe(this);
    }

    fun onUnload(debugMode)
    {
        itemSelector = debugMode.plugin("Debug Mode - Item Selector");
        itemSelector.unsubscribe(this);
    }

    fun onSelectItems(items)
    {
        foreach(item in items) {
            Console.print(item.name);
            Console.print(item.type);
        }

        Console.print(items.length + " item(s) selected.");
    }
}

*/

using SurgeEngine;
using SurgeEngine.Actor;
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.Camera;
using SurgeEngine.Input;
using SurgeEngine.Audio.Sound;
using SurgeEngine.UI.Text;
using SurgeEngine.Video.Screen;

object "Debug Mode - Item Selector" is "debug-mode-plugin", "debug-mode-observable", "private", "awake", "entity"
{
    observable = spawn("Debug Mode - Observable");
    trackedTouchId = -1;
    selectionOrigin = Vector2.zero;
    selectionPoint = Vector2.zero;
    selection = spawn("Debug Mode - Item Selector - Selection");
    selectedEntities = spawn("Debug Mode - Item Selector - Selected Entities");
    input = Input("default");
    vibrateSound = Sound("samples/vibrate.wav");
    warmUpTime = 0.5; // seconds
    itemPickerHeight = 0;

    state "main"
    {
        state = "ready";
    }

    state "disabled"
    {
    }

    state "ready"
    {
        selection.hide();
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
        selection.update(selectionOrigin, selectionPoint);
        selection.show();
    }

    state "selected"
    {
        selection.hide();

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

        // notify observers
        if(selectedEntities.count > 0)
            observable.notify(selectedEntities.items());

        // we're ready for a new selection
        deselect();
        state = "ready";
    }

    fun onLoad(debugMode)
    {
        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.subscribe(this);

        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPickerHeight = itemPicker.height;

        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        selection.zindex = uiSettings.zindex;
        selection.hide();

        selectedEntities.onLoad(debugMode);
    }

    fun onUnload(debugMode)
    {
        deselect();
        selectedEntities.onUnload(debugMode);

        touchInput = debugMode.plugin("Debug Mode - Touch Input");
        touchInput.unsubscribe(this);
    }

    fun subscribe(observer)
    {
        observable.subscribe(observer);
    }

    fun unsubscribe(observer)
    {
        observable.unsubscribe(observer);
    }

    fun onNotifyObserver(observer, items)
    {
        observer.onSelectItems(items);
    }

    fun onTouchBegin(touch)
    {
        if(touch.position.y < itemPickerHeight)
            return;

        if(state == "ready") {
            trackedTouchId = touch.id;

            selectionOrigin = Camera.screenToWorld(touch.position);
            selectionPoint = Camera.screenToWorld(touch.position);

            selection.update(selectionOrigin, selectionPoint);
            selection.show();

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

            // we tolerate a little bit of shaking
            if(touch.deltaPosition.length >= 1) {
                deselect();
                state = "ready";
            }

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
        vibrateSound.play();
    }

    fun deselect()
    {
        trackedTouchId = -1;
        selection.hide();
    }
}

object "Debug Mode - Item Selector - Item"
{
    type = "";
    name = "";

    fun get_type()
    {
        return type;
    }

    fun get_name()
    {
        return name;
    }

    fun init(itemType, itemName)
    {
        type = itemType;
        name = itemName;
        return this;
    }
}

object "Debug Mode - Item Selector - Selection" is "awake", "private", "entity"
{
    actor = Actor("Debug Mode - Item Selector - Selection");
    transform = Transform();

    fun update(selectionOrigin, selectionPoint)
    {
        width = selectionPoint.x - selectionOrigin.x; // may be negative
        height = selectionPoint.y - selectionOrigin.y; // may be negative

        transform.localPosition = selectionOrigin;
        transform.localScale = Vector2(width / actor.width, height / actor.height);
    }

    fun show()
    {
        actor.visible = true;
    }

    fun hide()
    {
        actor.visible = false;
    }

    fun get_zindex()
    {
        return actor.zindex;
    }

    fun set_zindex(zindex)
    {
        actor.zindex = zindex;
    }

    fun constructor()
    {
        actor.offset = actor.hotSpot; // top-left at (0,0)
        hide();
    }
}

object "Debug Mode - Item Selector - Selected Entities"// is "private", "awake", "entity"
{
    public blocked = false;
    selectedEntities = [];
    trackedTouchId = -1;
    zindex = 0.0;
    grid = null;
    trash = spawn("Debug Mode - Item Selector - Trash");
    itemPickerHeight = 0;

    fun get_count()
    {
        return selectedEntities.length;
    }

    fun items()
    {
        result = [];

        for(i = 0; i < selectedEntities.length; i++) {
            entity = selectedEntities[i];
            item = spawn("Debug Mode - Item Selector - Item").init("entity", entity.name);
            result.push(item);
        }

        return result;
    }

    fun add(entity, xe, ye, we, he)
    {
        // add selected entity
        //selectedEntity = Level.spawnEntity("Debug Mode - Item Selector - Selected Entity", Vector2(xe, ye));
        selectedEntity = spawn("Debug Mode - Item Selector - Selected Entity");
        selectedEntities.push(selectedEntity.init(
            entity,
            xe, ye, we, he,
            zindex
        ));

        // show the trash
        trash.show();
    }

    fun clear()
    {
        // destroy the selected entities
        foreach(selectedEntity in selectedEntities)
            selectedEntity.destroy();

        selectedEntities.clear();

        // hide the trash
        trash.hide();
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

    fun onLoad(debugMode)
    {
        grid = debugMode.plugin("Debug Mode - Grid System");

        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPickerHeight = itemPicker.height;

        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        zindex = uiSettings.zindex + 10;
        trash.zindex = uiSettings.zindex + 100;

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

    fun onTapScreen(position)
    {
        if(position.y < itemPickerHeight)
            return;

        // we're done with the selection
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

        // remove entities
        if(trash.isWithinBounds(touch.position)) {
            trash.playSound();
            trash.hide();
            clear();
            return;
        }

        // reposition entities
        foreach(selectedEntity in selectedEntities)
            selectedEntity.snapToGrid(grid);
    }

    fun onTouchMove(touch)
    {
        if(trackedTouchId != touch.id)
            return;

        if(blocked)
            return;

        moveAll(touch.deltaPosition);

        if(trash.isWithinBounds(touch.position))
            trash.highlight();
        else
            trash.dehighlight();
    }

    fun onCameraMove(deltaPosition)
    {
        moveAll(deltaPosition);
    }
}

object "Debug Mode - Item Selector - Selected Entity" is "private", "awake", "entity"
{
    transform = Transform();
    text = Text(SurgeEngine.mobile ? "BoxyBold" : "GoodNeighbors");
    selection = spawn("Debug Mode - Item Selector - Selected Entity - Selection");
    entityName = "";
    entityActor = null;

    fun move(deltaPosition)
    {
        transform.translate(deltaPosition);
    }

    fun snapToGrid(grid)
    {
        transform.position = grid.snapToGrid(transform.position);
    }

    fun respawn()
    {
        // we respawn the object in order to change the spawn point
        entityPosition = transform.position.plus(entityActor.offset);
        entity = Level.spawnEntity(entityName, entityPosition);

        // FIXME public properties are not preserved...
        // FIXME the ID of the entity is lost...
    }

    fun init(entity, x, y, w, h, zindex)
    {
        transform.position = Vector2(x, y);

        entityTransform = entity.child("Transform") || entity.spawn("Transform");
        entityPosition = entityTransform.position;

        entityName = entity.__name;
        entityActor = Actor(entityName);
        entityActor.offset = entityPosition.minus(transform.position);
        entityActor.zindex = zindex;

        text.text = entityName;
        text.offset = Vector2.up.scaledBy(text.size.y);
        text.zindex = zindex + 2;

        selection.init(w, h, zindex + 1);
        entity.destroy(); // temporarily destroy the entity

        return this;
    }

    fun get_name()
    {
        return entityName;
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

object "Debug Mode - Item Selector - Trash" is "private", "detached", "awake", "entity"
{
    actor = Actor("Debug Mode - Item Selector - Trash");
    transform = Transform();
    sound = Sound("samples/trash_empty.wav");
    appearTime = 0.33;
    highlightTime = 0.25;
    x0 = 0; x1 = 0; y = 0;
    idle = 0; highlighted = 1;
    t = 0; s = 0;

    state "main"
    {
        x0 = Screen.width + actor.width;
        x1 = x0 - 2 * actor.width;
        y = Screen.height / 2;

        transform.position = Vector2(x0, y);

        state = "hidden";
    }

    state "hidden"
    {
        actor.visible = false;
        //_resize();
    }

    state "visible"
    {
        actor.visible = true;
        _resize();
    }

    state "appearing"
    {
        actor.visible = true;
        _resize();

        x = Math.smoothstep(x0, x1, t);
        transform.position = Vector2(x, y);
        t += Time.delta / appearTime;

        if(timeout(appearTime))
            state = "visible";
    }

    state "disappearing"
    {
        actor.visible = true;
        _resize();

        x = Math.smoothstep(x1, x0, t);
        transform.position = Vector2(x, y);
        t += Time.delta / appearTime;

        if(timeout(appearTime))
            state = "hidden";
    }

    fun show()
    {
        if(state == "appearing" || state == "visible")
            return;

        s = t = 0;
        actor.anim = idle;
        state = "appearing";
    }

    fun hide()
    {
        if(state == "disappearing" || state == "hidden")
            return;

        s = t = 0;
        actor.anim = idle;
        state = "disappearing";
    }

    fun highlight()
    {
        s = t = 0;
        actor.anim = highlighted;
    }

    fun dehighlight()
    {
        s = t = 0;
        actor.anim = idle;
    }

    fun playSound()
    {
        sound.play();
    }

    fun get_zindex()
    {
        return actor.zindex;
    }

    fun set_zindex(zindex)
    {
        actor.zindex = zindex;
    }

    fun isWithinBounds(positionInScreenSpace)
    {
        rx = transform.position.x - actor.hotSpot.x;
        ry = transform.position.y - actor.hotSpot.y;
        rw = actor.width;
        rh = actor.height;

        dx = positionInScreenSpace.x - rx;
        dy = positionInScreenSpace.y - ry;

        return (dx >= 0 && dx < rw) && (dy >= 0 && dy < rh);
    }

    fun _resize()
    {
        if(s >= 1)
            return;

        targetScale = actor.anim == highlighted ? 1.25 : 1.0;
        scale = Math.lerp(transform.localScale.x, targetScale, s);
        transform.localScale = Vector2(scale, scale);
        s += Time.delta / highlightTime;
    }
}