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

object "Profiler" is "entity", "awake", "special"
{
    uiStats = spawn("Profiler.UI.Tree");
    uiDensity = spawn("Profiler.UI.Tree");
    uiTimes = spawn("Profiler.UI.Tree");
    stats = spawn("Profiler.Stats");
    sortByDesc = spawn("Profiler.UI.Tree.SortByDesc");
    refreshTime = 2.0;

    state "main"
    {
        uiTimes.transform.position = Vector2(0, 0);
        uiDensity.transform.position = Vector2(160, 0);
        uiStats.transform.position = Vector2(310, 0);

        // wait a frame
        state = "update";
    }

    state "update"
    {
        // recompute stats
        stats.refresh();

        // display stats
        uiTimes.updateUI("Time (avg / sum)", stats.timespent, sortByDesc.with(stats.timespent));
        uiDensity.updateUI("Density tree", stats.density, sortByDesc.with(stats.density));
        uiStats.updateUI("General", stats.generic, null);

        // done
        state = "wait";
    }

    state "wait"
    {
        if(timeout(refreshTime))
            state = "update";
    }
}

object "Profiler.Stats"
{
    public readonly generic = {};
    public readonly density = {};
    public readonly timespent = {};
    frames = 0;
    lastRefresh = 0;
    prevObjectCount = 0;
    root = null;
    maxDepth = 3; // increase to inspect more objects (slower)

    state "main"
    {
        // change to inspect a particular object
        root = Level;
        //root = Level.findObject("Default HUD");

        // count the number of frames
        state = "counting frames";
    }

    state "counting frames"
    {
        frames++;
    }

    fun refresh()
    {
        timeInterval = Math.max(Time.time - lastRefresh, 0.0001);
        objectCount = System.objectCount;

        density.destroy();
        timespent.destroy();
        generic.destroy();
        computeDensity(root, density = {}, 1, 1);
        computeTimespent(root, timespent = {}, count = {}, 1, 1);
        computeGeneric(generic = {}, objectCount, timeInterval);

        // compute relative times
        maxTimeSpent = timespent[root.__name];
        if(maxTimeSpent > 0) {
            maxAvgTimeSpent = maxTimeSpent / count[root.__name]; // count[root] is usually 1
            foreach(entry in timespent) {
                key = entry.key;
                value = entry.value;
                avg = value / count[key];

                value *= 100 / maxTimeSpent;
                avg *= 100 / maxAvgTimeSpent;

                timespent[key] = formatDecimal(avg) + " / " + formatDecimal(value);
            }
        }

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
        stats["Overhead"] = objectDelta + " (" + formatDecimal(100 * objectDelta / objectCount) + "%)";

        // garbage collection
        stats["Garbage"] = System.gc.objectCount;

        // fps rate
        fps = frames / timeInterval;
        stats["FPS"] = formatDecimal(fps);
        frames = 0;

        // time
        stats["Time"] = Math.floor(Time.time) + "s";
    }

    fun computeDensity(obj, tree, id, depth)
    {
        key = obj.__name; //hash(obj, id);
        result = 1;
        n = obj.__childCount;
        for(i = 0; i < n; i++) {
            child = obj.child(i);
            //if(child.__active)
                result += computeDensity(child, tree, ++id, 1+depth);
        }
        if(depth <= maxDepth)
            tree[key] += result;
        return result;
    }

    fun computeTimespent(obj, tree, countTree, id, depth)
    {
        key = obj.__name; //hash(obj, id);
        totalTime = 0;
        n = obj.__childCount;
        for(i = 0; i < n; i++) {
            child = obj.child(i);
            if(child.__active)
                totalTime += computeTimespent(child, tree, countTree, ++id, 1+depth);
        }
        totalTime += this.__timespent;
        if(depth <= maxDepth) {
            tree[key] += totalTime;
            countTree[key] += 1;
        }
        return totalTime;
    }

    fun hash(obj, uid)
    {
        return obj.__name + "/" + uid;
    }

    fun formatDecimal(num)
    {
        if(typeof(num) !== "number") return num;
        str = (Math.round(num * 100) * 0.01).toString();
        j = str.indexOf(".");
        return j >= 0 ? str.substr(0, j+3) : str;
    }
}

object "Profiler.UI.Tree" is "entity", "detached", "private", "awake"
{
    public readonly transform = Transform();
    text = Text("BoxyBold");

    fun constructor()
    {
        transform.position = Vector2(0, 0);
        text.zindex = Math.infinity;
        text.maxWidth = 160;
    }

    fun updateUI(title, tree, order)
    {
        str = "<color=ffff00>" + title + "</color>\n";
        keys = tree.keys().sort(order);
        length = keys.length;
        for(i = 0; i < length; i++) {
            key = keys[i];
            value = formatDecimal(tree[key]);
            str += key + ": " + value + "\n";
        }
        text.text = str;
        keys.destroy();
    }

    fun formatDecimal(num)
    {
        if(typeof(num) !== "number") return num;
        str = (Math.round(num * 100) * 0.01).toString();
        j = str.indexOf(".");
        return j >= 0 ? str.substr(0, j+3) : str;
    }
}

object "Profiler.UI.Tree.SortByDesc"
{
    tree = null;
    fun with(dict) { tree = dict; return this; }
    fun call(a, b) { return f(tree[b]) - f(tree[a]); }
    fun f(s) { return (typeof(s) === "number") ? s : ((j = s.indexOf(" ")) >= 0 ? Number(s.substr(0, j)) : Number(s)); }
}
