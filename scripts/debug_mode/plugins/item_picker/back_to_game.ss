// -----------------------------------------------------------------------------
// File: back_to_game.ss
// Description: "back to game" button for the item picker plugin (Debug Mode)
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

object "Debug Mode - Item Picker - Back To Game" is "debug-mode-plugin"
{
    debugMode = null;

    fun onLoad(debugModeObject)
    {
        debugMode = debugModeObject;

        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.subscribe(this);

        // add to the main menu
        itemPicker
            .add(back())
        ;
    }

    fun onUnload(debugMode)
    {
        itemPicker = debugMode.plugin("Debug Mode - Item Picker");
        itemPicker.unsubscribe(this);
    }

    fun back()
    {
        return spawn("Debug Mode - Item Picker - Carousel Item Builder - Actor")
            .setName("Debug Mode - Item Picker - Back") // sprite name
            .setType("back-to-game") // item type
        ;
    }

    fun onPickItem(item)
    {
        if(item.type == "back-to-game")
            debugMode.exit();
    }
}