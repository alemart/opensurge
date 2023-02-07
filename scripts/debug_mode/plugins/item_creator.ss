// -----------------------------------------------------------------------------
// File: item_creator.ss
// Description: item creator plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Actor;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

object "Debug Mode - Item Creator" is "debug-mode-plugin", "private", "awake", "entity"
{
    debugMode = parent;
    transform = Transform();
    input = Input("default");
    actionButton = "fire1";
    itemType = "";
    itemName = "";
    itemPreview = spawn("Debug Mode - Item Creator - Preview");
    positionUpdater = null;

    state "main"
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        itemPreview.zindex = uiSettings.zindex;

        worldScroller = debugMode.plugin("Debug Mode - World Scroller");
        positionUpdater = worldScroller.spawn("Debug Mode - Item Creator - Position Updater").setItemCreator(this);

        state = "ready";
    }

    state "ready"
    {
        // create the item if the action button is pressed
        if(input.buttonPressed(actionButton)) {

            if(itemType == "entity")
                createEntity(itemName);

        }
    }

    fun createEntity(entityName)
    {
        Level.spawnEntity(entityName, transform.position);
    }

    fun onPickItem(name)
    {
        itemType = "entity";
        itemName = name;

        itemPreview.setEntity(itemName);
    }

    fun onPositionUpdate(position)
    {
        transform.position = position;
    }
}

object "Debug Mode - Item Creator - Preview" is "private", "awake", "entity"
{
    public zindex = 0.0;
    actor = null;

    fun setEntity(entityName)
    {
        if(actor !== null)
            actor.destroy();

        actor = Actor(entityName);
        actor.visible = true;
        actor.alpha = 0.67;
        actor.zindex = zindex;
    }
}

// lateUpdate() hack
// the position of the item creator should be updated AFTER the world scroller is updated
object "Debug Mode - Item Creator - Position Updater" is "private", "awake", "entity"
{
    worldScroller = parent;
    itemCreator = null;

    state "main"
    {
        itemCreator.onPositionUpdate(worldScroller.position);
    }

    fun setItemCreator(obj)
    {
        itemCreator = obj;
        return this;
    }
}