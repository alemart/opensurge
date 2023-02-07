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

Plugins may optionally implement functions init() and release() for
initialization and deinitialization of resources, respectively.

*/
using SurgeEngine.Level;
using SurgeEngine.Vector2;
using SurgeEngine.Transform;

// The Debug Mode object manages its plugins
object "Debug Mode" is "detached", "private", "entity"
{
    transform = Transform();
    plugins = [];
    indicesOfPluginsScheduledForRemoval = [];
    cleared = false;

    state "main"
    {
        // load all plugins
        pluginNames = System.tags.select("debug-mode-plugin");
        for(i = 0; i < pluginNames.length; i++) {
            plugin = spawn(pluginNames[i]);
            plugins.push(plugin);
        }

        // initialize all plugins
        foreach(plugin in plugins) {
            if(plugin.hasFunction("init"))
                plugin.init();
        }

        // done!
        state = "active";
    }

    state "active"
    {
        // do nothing
        if(indicesOfPluginsScheduledForRemoval.length == 0)
            return;

        // release plugins
        for(i = 0; i < indicesOfPluginsScheduledForRemoval.length; i++) {
            index = indicesOfPluginsScheduledForRemoval[i];
            plugin = plugins[index];

            if(plugin.hasFunction("release"))
                plugin.release();
        }

        // unload plugins
        for(i = 0; i < indicesOfPluginsScheduledForRemoval.length; i++) {
            index = indicesOfPluginsScheduledForRemoval[i];

            // unload plugin
            plugins[index].destroy();

            // remove from the plugins array
            if(plugins.length > 1)
                plugins[index] = plugins.pop(); // length -= 1
            else
                plugins.clear();
        }

        // no more plugins scheduled for removal
        indicesOfPluginsScheduledForRemoval.clear();
    }

    state "exit"
    {
        // release all plugins
        foreach(plugin in plugins) {
            if(plugin.hasFunction("release"))
                plugin.release();
        }

        // unload all plugins
        foreach(plugin in plugins)
            plugin.destroy();
        plugins.clear();

        // exit debug mode
        cleared = true;
        destroy();
    }



    // exit the debug mode
    fun exit()
    {
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

        // just to make sure...
        transform.position = Vector2.zero;
    }

    fun destructor()
    {
        // did the user destroy() the Debug Mode object?
        if(!cleared) {
            // plugin.release() is not being called...
            Console.print("Use exit() to leave the debug mode!");
        }

        // note: the engine may be closed while the debug mode is activated
    }
}