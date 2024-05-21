#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# stage_java.sh
# Stage the Java part
#
set -e

echo "Staging the Java part..."

# Stage
cp -v build/parts/java/build/classes.dex build/stage/
cp -v build/parts/java/src/AndroidManifest.xml build/stage/
cp -v -r build/parts/java/src/res build/stage/

# Success!
echo "The Java part has been staged!"
