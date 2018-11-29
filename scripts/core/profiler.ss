// -----------------------------------------------------------------------------
// File: profiler.ss
// Description: SurgeScript profiler
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------
using SurgeEngine.Transform;
using SurgeEngine.Vector2;
using SurgeEngine.UI.Text;
using SurgeEngine.Level;

object "Profiler" is "entity", "awake"
{
    uiStats = spawn("Profiler.UI.Tree");
    uiDensity = spawn("Profiler.UI.Tree");
    uiTimes = spawn("Profiler.UI.Tree");
    stats = spawn("Profiler.Stats");
    refreshTime = 2.0;

    state "main"
    {
        count = System.objectCount;
        sortByDesc = spawn("Profiler.UI.Tree.SortByDesc");
        stats.refresh();

        // update stats
        uiTimes.updateUI("Time spent (ms)", stats.timespent, sortByDesc.with(stats.timespent));
        uiStats.updateUI("Profiler", stats.generic, null);
        uiDensity.updateUI("Density tree", stats.density, sortByDesc.with(stats.density));
        uiTimes.transform.position = Vector2(0, 0);
        uiStats.transform.position = Vector2(160, 0);
        uiDensity.transform.position = Vector2(320, 0);

        // done
        state = "wait";
    }

    state "wait"
    {
        if(timeout(refreshTime))
            state = "main";
    }
}

object "Profiler.Stats"
{
    generic = {};
    density = {};
    timespent = {};
    frames = 0;
    lastRefresh = 0;
    prevObjectCount = 0;
    maxDepth = 3;

    state "main"
    {
        frames++;
        generic.destroy();
        density.destroy();
        timespent.destroy();
        generic = {};
        density = {};
        timespent = {};
    }

    fun get_density()
    {
        return density;
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

        density.destroy();
        timespent.destroy();
        generic.destroy();
        computeDensity(Level, density = {}, 1, 1);
        computeTimespent(Level, timespent = {}, 1, 1);
        computeGeneric(generic = {}, objectCount, timeInterval);

        lastRefresh = Time.time;
    }

    fun computeGeneric(stats, objectCount, timeInterval)
    {
        // object count
        stats["Objects"] = objectCount;

        // spawn rate
        spawnRate = 100 * (objectCount - prevObjectCount) / prevObjectCount;
        stats["Spawn rate"] = formatDecimal(spawnRate) + "%";
        prevObjectCount = objectCount;

        // profiler object overhead
        objectDelta = System.objectCount - objectCount;
        stats["Overhead"] = objectDelta + " (" + Math.floor(100 * objectDelta / objectCount) + "%)";

        // fps rate
        fps = frames / timeInterval;
        stats["FPS"] = formatDecimal(fps);
        frames = 0;

        // time
        stats["Time"] = Math.floor(Time.time) + "s";
    }

    fun computeDensity(obj, tree, id, depth)
    {
        if(obj.__name == "Profiler") return 0;
        key = obj.__name; //hash(obj, id);
        result = 1;
        count = obj.childCount;
        for(i = 0; i < count; i++) {
            child = obj.child(i);
            if(child.__active)
                result += computeDensity(child, tree, ++id, 1+depth);
        }
        if(depth <= maxDepth)
            tree[key] = Math.max(tree[key], result);
        return result;
    }

    fun computeTimespent(obj, tree, id, depth)
    {
        if(obj.__name == "Profiler") return 0;
        key = obj.__name; //hash(obj, id);
        totalTime = 0;
        count = obj.childCount;
        for(i = 0; i < count; i++) {
            child = obj.child(i);
            if(child.__active)
                totalTime += computeTimespent(child, tree, ++id, 1+depth);
        }
        totalTime += 1000 * this.__timespent;
        if(depth <= maxDepth)
            tree[key] += totalTime;
        return totalTime;
    }

    fun hash(obj, uid)
    {
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