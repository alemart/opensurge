// -----------------------------------------------------------------------------
// File: profiler.ss
// Description: SurgeScript profiler
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.UI.Text;

//object "Application" { profiler = spawn("Profiler"); state "main" {} }
object "Profiler" is "entity", "awake"
{
    uiStats = spawn("Profiler.UI.Tree");
    uiDescendants = spawn("Profiler.UI.Tree");
    uiTimes = spawn("Profiler.UI.Tree");
    stats = spawn("Profiler.Stats");
    refreshTime = 2.0;

    state "main"
    {
        count = System.objectCount;
        sortByDesc = spawn("Profiler.UI.Tree.SortByDesc");
        stats.refresh();

        // update stats
        uiDescendants.updateUI("Hierarchy", stats.descendants, sortByDesc.with(stats.descendants));
        uiTimes.updateUI("Time spent", stats.timespent, sortByDesc.with(stats.timespent));
        uiStats.updateUI("Profiler", stats.generic, null);
        Console.print(System.objectCount);
        uiStats.transform.position = Vector2(311, 0);
        uiTimes.transform.position = Vector2(160, 0);
        uiDescendants.transform.position = Vector2(0, 0);

        // done
        state = "wait";
    }

    state "wait"
    {
        if(timeout(refreshTime))
            state = "main";
    }

    fun constructor()
    {
        uiStats.transform.position = Vector2(311, 0);
        uiTimes.transform.position = Vector2(160, 0);
        uiDescendants.transform.position = Vector2(0, 0);
    }
}

object "Profiler.Stats"
{
    generic = {};
    descendants = {};
    timespent = {};
    frames = 0;
    lastRefresh = 0;
    avgObjectCount = 0;
    prevObjectCount = 0;

    state "main"
    {
        frames++;
        avgObjectCount += System.objectCount;
        generic.destroy();
        descendants.destroy();
        timespent.destroy();
        generic = {};
        descendants = {};
        timespent = {};
        Console.print(System.__timespent);
    }

    fun get_descendants()
    {
        return descendants;
    }

    fun get_timespent()
    {
        return timespent;
    }

    fun get_generic()
    {
        return generic;
    }

    fun refresh()
    {
        timeInterval = Math.max(Time.time - lastRefresh, 0.0001);
        objectCount = System.objectCount;

        descendants.destroy();
        timespent.destroy();
        generic.destroy();
        computeDescendants(System, descendants = {}, 1, 1);
        computeTimespent(System, timespent = {}, 1, 1);
        computeGeneric(generic = {}, objectCount, timeInterval);

        lastRefresh = Time.time;
    }

    fun computeGeneric(stats, objectCount, timeInterval)
    {
        // object count
        avgObjectCount = avgObjectCount / frames;
        stats["Objects"] = objectCount;
        //stats["Objects"] = Math.round(avgObjectCount);
        avgObjectCount = 0;

        // spawn rate
        spawnRate = 100 * (objectCount - prevObjectCount) / prevObjectCount;
        stats["Spawn rate"] = formatDecimal(spawnRate) + "%";
        prevObjectCount = objectCount;

        // profiler overhead
        objectDelta = System.objectCount - objectCount;
        stats["Overhead"] = objectDelta + " (" + Math.floor(100 * objectDelta / objectCount) + "%)";

        // fps rate
        fps = frames / timeInterval;
        stats["FPS"] = formatDecimal(fps);
        frames = 0;

        // time
        stats["Time"] = Math.floor(Time.time) + "s";
    }

    fun computeDescendants(obj, tree, id, depth)
    {
        //if(depth > 10) return 0; // stack overflow?
        if(obj.__name == "Profiler") return 0;
        key = hash(obj, id);
        descendantCount = 1;
        count = obj.childCount;
        for(i = 0; i < count; i++)
            descendantCount += computeDescendants(obj.child(i), tree, ++id, 1+depth);
        tree[key] = descendantCount;
        return descendantCount;
    }

    fun computeTimespent(obj, tree, id, depth)
    {
        //if(depth > 10) return 0; // stack overflow?
        if(obj.__name == "Profiler") return 0;
        key = hash(obj, id);
        totalTime = 0;;
        count = obj.childCount;
        for(i = 0; i < count; i++)
            totalTime += computeTimespent(obj.child(i), tree, ++id, 1+depth);
        tree[key] = this.__timespent + totalTime;
        return tree[key];
    }

    fun hash(obj, uid)
    {
        //return String.concat(obj.__name, uid.toString());//obj.__name + "/" + uid;
        return obj.__name + "/" + uid;
    }

    fun formatDecimal(num)
    {
        str = (Math.round(num * 100) * 0.01).toString();
        j = str.indexOf(".");
        return j >= 0 ? str.substr(0, j+3) : str;
    }
}

object "Profiler.UI.Tree" is "entity", "detached", "private", "awake"
{
    transform = Transform();
    text = Text("default_ttf");

    fun constructor()
    {
        transform.position = Vector2(0, 0);
        text.zindex = Math.infinity;
    }

    fun updateUI(title, tree, order)
    {
        str = "<color=ffff00>" + title + "</color>\n";
        keys = tree.keys().sort(order);
        length = keys.length;
        for(i = 0; i < length; i++) {
            key = keys[i];
            str += key + ": " + tree[key] + "\n";
            //Console.print(key + ": " + tree[key]);
        }
        text.text = str;
        keys.destroy();
    }

    fun get_transform()
    {
        return transform;
    }
}

object "Profiler.UI.Tree.SortByDesc"
{
    tree = null;
    fun with(dict) { tree = dict; return this; }
    fun call(a, b) { return tree[b] - tree[a]; }
}