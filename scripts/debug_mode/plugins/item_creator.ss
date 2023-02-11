// -----------------------------------------------------------------------------
// File: item_creator.ss
// Description: item creator plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin lets the user put items on the level.

*/

using SurgeEngine.Actor;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

object "Debug Mode - Item Creator" is "debug-mode-plugin", "private", "awake", "entity"
{
    transform = Transform();
    input = Input("default");
    actionButton = "fire1";
    itemType = "";
    itemName = "";
    itemPreview = spawn("Debug Mode - Item Creator - Preview");
    camera = null; // camera of the debug mode (plugin)



    state "main"
    {
        // follow the camera
        transform.position = camera.position;

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

    fun onLoad(debugMode)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        itemPreview.zindex = uiSettings.zindex;

        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        camera = debugMode.plugin("Debug Mode - Camera");
    }

    fun onUnload(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.unsubscribe(this);
    }

    fun onPickItem(item)
    {
        itemType = item.type;
        itemName = item.name;

        if(itemType == "entity")
            itemPreview.setEntity(itemName);
        else
            itemPreview.setNull();
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

    fun setNull()
    {
        if(actor !== null)
            actor.destroy();
    }
}