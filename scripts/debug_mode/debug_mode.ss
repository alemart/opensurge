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

Plugins may optionally implement functions onLoad() and onUnload() for
initialization and deinitialization of resources, respectively. Example:

// File: debug_mode/plugins/my_other_plugin.ss
object "Debug Mode - My Other Plugin" is "debug-mode-plugin"
{
    fun onLoad(debugMode) // debugMode is the "Debug Mode" object below
    {
        Console.print("Loading this plugin");
    }

    fun onUnload(debugMode)
    {
        Console.print("Unloading this plugin");
    }
}

Plugins can also interact with each other. I'll let you figure this out.

Study how the existing plugins are made. Some are simple and short. The
possibilities are limitless!

Happy hacking! ;)

*/

using SurgeEngine.Level;
using SurgeEngine.Vector2;

// The Debug Mode object manages its plugins
object "Debug Mode"
{
    plugins = [];
    indicesOfPluginsScheduledForRemoval = [];
    cleared = false;

    state "main"
    {
        // spawn all plugins in ascending order
        pluginNames = System.tags.select("debug-mode-plugin").sort(null);
        for(i = 0; i < pluginNames.length; i++) {
            plugin = spawn(pluginNames[i]);
            plugins.push(plugin);
        }

        // load all plugins
        // (ideally we would have a dependency graph)
        foreach(plugin in plugins)
            _loadPlugin(plugin);

        // done!
        state = "active";
    }

    state "active"
    {
        // do nothing
        if(indicesOfPluginsScheduledForRemoval.length == 0)
            return;

        // unload plugins
        for(i = 0; i < indicesOfPluginsScheduledForRemoval.length; i++) {
            index = indicesOfPluginsScheduledForRemoval[i];
            _unloadPlugin(plugins[index]);
        }

        // destroy plugins
        for(i = 0; i < indicesOfPluginsScheduledForRemoval.length; i++) {
            index = indicesOfPluginsScheduledForRemoval[i];
            plugins[index].destroy();

            // remove from the plugins array
            if(index < plugins.length - 1)
                plugins[index] = plugins.pop(); // length -= 1
            else
                plugins.pop();
        }

        // no more plugins scheduled for removal
        indicesOfPluginsScheduledForRemoval.clear();
    }

    state "exit"
    {
        // unload all plugins
        foreach(plugin in plugins)
            _unloadPlugin(plugin);

        // destroy all plugins
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
        // exit() may be called twice or more...
        state = "exit"; // don't destroy() the object right away
    }

    // get a plugin
    fun plugin(pluginName)
    {
        plugin = this.child(pluginName);
        if(plugin !== null && plugins.indexOf(plugin) >= 0)
            return plugin;

        Application.crash("Can't find Debug Mode Plugin: " + pluginName);
        return null;
    }

    // check if a plugin is in use
    fun hasPlugin(pluginName)
    {
        plugin = this.child(pluginName);
        return (plugin !== null && plugins.indexOf(plugin) >= 0);
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

        // enable the Debug Mode flag
        Level.debugMode = true;
    }

    fun destructor()
    {
        // disable the Debug Mode flag
        Level.debugMode = false;

        // did the user destroy() the Debug Mode object?
        if(!cleared) {
            // plugin.onUnload() is not being called...
            Console.print("Use exit() to leave the debug mode!");
        }

        // note: the engine may be closed while the debug mode is activated
    }

    fun _loadPlugin(plugin)
    {
        if(plugin.findObjectWithTag("debug-mode-plugin") !== null)
            Application.crash("Debug Mode: nested plugins are not acceptable.");

        uiComponents = plugin.findObjectsWithTag("debug-mode-ui-component");
        foreach(uiComponent in uiComponents) {
            if(uiComponent.hasFunction("onLoad"))
                uiComponent.onLoad(this);
        }

        if(plugin.hasFunction("onLoad"))
            plugin.onLoad(this);
    }

    fun _unloadPlugin(plugin)
    {
        if(plugin.hasFunction("onUnload"))
            plugin.onUnload(this);

        uiComponents = plugin.findObjectsWithTag("debug-mode-ui-component");
        foreach(uiComponent in uiComponents) {
            if(uiComponent.hasFunction("onUnload"))
                uiComponent.onUnload(this);
        }
    }
}
