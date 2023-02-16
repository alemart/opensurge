// -----------------------------------------------------------------------------
// File: camera_menu.ss
// Description: camera menu for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Camera Menu" is "debug-mode-plugin"
{
    select = spawn("Debug Mode - Tag Selector");
    camera = null;

    fun onLoad(debugMode)
    {
        camera = debugMode.plugin("Debug Mode - Camera");

        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        // add to the main menu
        itemPicker
            .add(menu())
        ;
    }

    fun onUnload(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.unsubscribe(this);
    }

    fun menu()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Camera") // sprite name
            .setType("open-camera-menu") // item type
        ;
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("close-camera-menu") // item type
        ;
    }

    fun button(spriteName)
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName(spriteName) // entityName is also the name of a sprite
            .setType("change-camera-mode") // item type
        ;
    }

    fun openCameraMenu(itemPicker)
    {
        itemPicker.push()
            .add(back())
            .add(button("Debug Mode - Item Picker - Camera - Player Mode"))
            .add(button("Debug Mode - Item Picker - Camera - Free Look Mode"))
            .add(button("Debug Mode - Item Picker - Camera - Scroll Mode"))
        ;
    }

    fun closeCameraMenu(itemPicker)
    {
        itemPicker.pop();
    }

    fun onPickItem(item)
    {
        if(item.type == "open-camera-menu") {
            openCameraMenu(item.picker);
        }
        else if(item.type == "close-camera-menu") {
            closeCameraMenu(item.picker);
        }
        else if(item.type == "change-camera-mode") {

            if(item.name == "Debug Mode - Item Picker - Camera - Player Mode")
                camera.mode = "player";
            else if(item.name == "Debug Mode - Item Picker - Camera - Free Look Mode")
                camera.mode = "free-look";
            else if(item.name == "Debug Mode - Item Picker - Camera - Scroll Mode")
                camera.mode = "scroll";

        }
    }
}