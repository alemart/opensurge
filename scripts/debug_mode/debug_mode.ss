// -----------------------------------------------------------------------------
// File: debug_mode.ss
// Description: Debug Mode
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

/*

The Debug Mode lets you edit elements of the level. It may be accessed:
- on Mobile: via the Mobile Menu at the Pause Screen
- on Desktop: press Shift+F12 during gameplay

You leave it by pressing the back button on your smartphone / gamepad or the
ESC key on your keyboard.

You can extend the Debug Mode by creating your own plugins. Doing so is as
simple as creating objects tagged "debug-mode-plugin" and saving them to the
debug_mode/plugins/ folder. Example:

// File: debug_mode/plugins/my_plugin.ss
object "Debug Mode - My Plugin" is "debug-mode-plugin"
{
    debugMode = parent; // this is the "Debug Mode" object

    state "main"
    {
        Console.print("Hello from My Plugin!");
    }
}

In order to keep compatibility with the official releases of the engine, do not
change the scripts of the Debug Mode directly. Create your own plugins instead.

Visible elements should be rendered in screen space. This means that you should
add the tags "detached", "private", "entity" to the objects you wish to see.
Study the plugins of the Debug Mode for practical examples.

*/
using SurgeEngine.Level;

// The Debug Mode object manages its plugins
object "Debug Mode" is "detached", "private", "entity"
{
    plugins = [];
    indicesOfPluginsScheduledForRemoval = [];

    state "main"
    {
        // load all plugins
        pluginNames = System.tags.select("debug-mode-plugin");
        for(i = 0; i < pluginNames.length; i++) {
            plugin = spawn(pluginNames[i]);
            plugins.push(plugin);
        }

        // done!
        state = "active";
    }

    state "active"
    {
        // do nothing
        if(indicesOfPluginsScheduledForRemoval.length == 0)
            return;

        // remove plugins
        for(i = 0; i < indicesOfPluginsScheduledForRemoval.length; i++) {
            index = indicesOfPluginsScheduledForRemoval[i];
            plugins[index].destroy();

            if(plugins.length > 1)
                plugins[index] = plugins.pop();
            else
                plugins.clear();
        }

        indicesOfPluginsScheduledForRemoval.clear();
    }

    state "exit"
    {
        // exit debug mode
        destroy(); // this will automatically unload all plugins
    }



    // exit the debug mode
    fun exit()
    {
        // skip duplicate calls
        if(state == "exit")
            return;

        // for each plugin, call plugin.onExitDebugMode()
        for(i = plugins.length - 1; i >= 0; i--) {
            if(plugins[i].hasFunction("onExitDebugMode"))
                plugins[i].onExitDebugMode();
        }

        // exit
        state = "exit";
    }

    // get a plugin
    fun plugin(pluginName)
    {
        for(i = plugins.length - 1; i >= 0; i--) {
            if(plugins[i].__name == pluginName)
                return plugins[i];
        }

        Application.crash("Can't find Debug Mode Plugin: " + pluginName);
        return null;
    }

    // check if a plugin is in use
    fun hasPlugin(pluginName)
    {
        for(i = plugins.length - 1; i >= 0; i--) {
            if(plugins[i].__name == pluginName)
                return true;
        }

        return false;
    }

    // remove a plugin
    fun removePlugin(pluginName)
    {
        for(i = plugins.length - 1; i >= 0; i--) {
            if(plugins[i].__name == pluginName) {

                // don't allow duplicates (this function may be called repeatedly)
                if(indicesOfPluginsScheduledForRemoval.indexOf(i) < 0)
                    indicesOfPluginsScheduledForRemoval.push(i);

                break;

            }
        }
    }

    // internal use
    fun constructor()
    {
        // this object must be unique and a child of Level
        assert(parent === Level);
        assert(parent.children(this.__name).length == 1);
    }

    fun destructor()
    {
        if(state != "exit")
            Application.crash("Use exit() to leave the debug mode!");
    }
}