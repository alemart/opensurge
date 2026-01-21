#!/usr/bin/env node
// -----------------------------------------------------------------------------
// File: releases2appdata.js
// Description: converts a Release Notes file into an appdata XML block
// Author: Alexandre Martins <http://opensurge2d.org>
// License: MIT
// -----------------------------------------------------------------------------

// How to use:
// ./releases2appdata.js < /path/to/CHANGELOG.md
const readline = require("readline");
const rl = readline.createInterface(process.stdin);
parse(rl).then(printXML);

// Parses CHANGELOG.md
function parse(rl)
{
    const match = (s, r) => { const x = r.exec(s) || []; return k => x[k] || ""; };
    let releases = [];
    let release = null;

    return new Promise((resolve, reject) => {
        rl.on("line", line => {
            // new release
            if(/\d\.\d.*\d{4}\s*$/.test(line)) {
                const param = match(line, /(\d\.\d(\.\d(\.\d)?)?)\W*(.*)$/);
                release = {
                    version: param(1),
                    date: new Date(param(4).replace(/(\d)(st|nd|rd|th)/gi, "$1").trim()),
                    items: []
                };
                releases.push(release);
            }

            // release item
            else if(release && /^\s*\*/.test(line)) {
                const param = match(line, /^\s*\*\s+(.*)$/);
                release.items.push(param(1).replace(/(\W)/g, (m, x) => ({
                    "&": "&amp;",
                    "<": "&lt;",
                    ">": "&gt;",
                })[x] || x).trim());
            }
        });

        rl.on("close", () => {
            resolve(releases);
        });
    });
}

// Converts to XML
function printXML(releases)
{
    const day = d => d.toISOString().substr(0, 10);
    const indent = n => s => (new Array(n).fill("  ").join("")) + s;
    const print = n => { const i = indent(n); return s => console.log(i(s)) };
    const p1 = print(1), p2 = print(2), p3 = print(3), p4 = print(4), p5 = print(5);

    p1(`<releases>`);
    for(const release of releases) {
        p2(`<release version="${release.version}" date="${day(release.date)}">`);
        p3(`<description>`);
        p4(`<ul>`)
        for(const item of release.items)
            p5(`<li>${item}</li>`);
        p4(`</ul>`)
        p3(`</description>`);
        p2(`</release>`);
    }
    p1(`</releases>`);
}