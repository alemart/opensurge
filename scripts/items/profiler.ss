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
    alternationTime = 8.0;
    counter = 0;

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
        cycle = Math.ceil(alternationTime / refreshTime);
        wantAvg = ((counter++) % (2 * cycle)) < cycle;
        stats.refresh(wantAvg);

        // display stats
        uiTimes.updateUI(wantAvg ? "Time (avg)" : "Time (sum)", stats.timespent, sortByDesc.with(stats.timespent));
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
    maxDepth = 16; // increase to inspect more objects (slower)

    state "main"
    {
        // change to inspect a particular object
        root = Level;
        //root = Level.findObject("Default HUD");
        //root = System;

        // count the number of frames
        state = "counting frames";
    }

    state "counting frames"
    {
        frames++;
    }

    fun refresh(wantAverageTimes)
    {
        timeInterval = Math.max(Time.time - lastRefresh, 0.0001);
        objectCount = System.objectCount;

        density.destroy();
        timespent.destroy();
        generic.destroy();
        computeDensity(root, density = {}, 1, 1);
        computeTimespent(root, absTimeSpent = {}, count = {}, 1, 1);
        computeGeneric(generic = {}, objectCount, timeInterval);

        // compute relative times
        timespent = {};
        maxTimeSpent = absTimeSpent[root.__name];
        if(maxTimeSpent > 0) {
            maxAvgTimeSpent = maxTimeSpent / count[root.__name]; // count[root] is usually 1
            foreach(entry in absTimeSpent) {
                key = entry.key;
                val = entry.value;
                cnt = count[key];

                if(wantAverageTimes) {
                    avg = val / cnt;
                    avg *= 100 / maxAvgTimeSpent;
                    timespent[key] = avg;
                }
                else {
                    val *= 100 / maxTimeSpent;
                    key = "[" + cnt + "] " + key;
                    timespent[key] = val;
                }
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
        stats["FPS"] = formatDecimal(frames / timeInterval);
        stats["FPS Inv."] = frames > 0 ? timeInterval / frames : 0;
        frames = 0;

        // time
        stats["Time"] = Math.floor(Time.time) + "s";
    }

    fun computeDensity(obj, tree, id, depth)
    {
        result = 1;
        key = obj.__name;
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
        totalTime = 0;
        key = obj.__name;
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
            value = tree[key];
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
