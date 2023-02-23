// -----------------------------------------------------------------------------
// File: item_creator.ss
// Description: item creator plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

This plugin lets the user put items on the level.

Properties:
- mode: string. One of the following: "mobile", "mouse".

*/

using SurgeEngine;
using SurgeEngine.Actor;
using SurgeEngine.Input;
using SurgeEngine.Level;
using SurgeEngine.Player;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;
using SurgeEngine.UI.Text;
using SurgeEngine.Input.Mouse;
using SurgeEngine.Camera;

object "Debug Mode - Item Creator" is "debug-mode-plugin", "private", "awake", "entity"
{
    transform = Transform();
    input = Input("default");
    actionButton = "fire1";
    itemType = "";
    itemName = "";
    itemPreview = spawn("Debug Mode - Item Creator - Preview");
    camera = null; // camera of the debug mode (plugin)
    gridSystem = null;
    itemPickerHeight = 0;



    state "main"
    {
    }

    state "mobile"
    {
        // follow the camera
        transform.position = camera.position;

        // create the item if the action button is pressed
        if(input.buttonPressed(actionButton)) {

            createItem(transform.position);

        }
    }

    state "mouse"
    {
        // follow the mouse
        transform.position = gridSystem.snapToGrid(Camera.screenToWorld(Mouse.position));

        // create the item when the user taps the screen
        // onTapScreen() ...
    }

    fun onTapScreen(position)
    {
        if(state == "mouse") {
            if(position.y >= itemPickerHeight)
                createItem(transform.position);
        }
    }

    fun createItem(position)
    {
        if(itemType == "entity") {
            entityName = itemName;
            Level.spawnEntity(entityName, position);
        }
    }

    fun onLoad(debugMode)
    {
        uiSettings = debugMode.plugin("Debug Mode - UI Settings");
        itemPreview.zindex = uiSettings.zindex + 1000;

        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);
        itemPickerHeight = itemPicker.height;

        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.subscribe(this);

        camera = debugMode.plugin("Debug Mode - Camera");
        gridSystem = debugMode.plugin("Debug Mode - Grid System");
    }

    fun onUnload(debugMode)
    {
        tapDetector = debugMode.plugin("Debug Mode - Tap Detector");
        tapDetector.unsubscribe(this);

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

    fun constructor()
    {
        if(SurgeEngine.mobile)
            state = "mobile";
        else
            state = "mouse";
    }
}

object "Debug Mode - Item Creator - Preview" is "private", "awake", "entity"
{
    public zindex = 0.0;
    transform = Transform();
    text = Text("GoodNeighbors");
    actor = null;

    state "main"
    {
        xpos = Math.floor(transform.position.x);
        ypos = Math.floor(transform.position.y);

        text.text = xpos + " , " + ypos;
        text.zindex = zindex;
    }

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

        actor = null;
    }

    fun constructor()
    {
        text.align = "center";
        text.offset = Vector2(0, 16);
    }
}