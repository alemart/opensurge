#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# build_java.sh
# Build the Java part
#
set -e

echo "Building the Java part..."
source "$(dirname "$0")/set_sdk.sh" "$@"

# Extract classes.jar from the Allegro AAR
echo "Obtaining classes.jar from Allegro..."
pushd build/stage/libs

if [[ ! -e "allegro-release.aar" ]]; then
    echo "ERROR: can't find allegro-release.aar in the staging area"
    exit 1
fi

unzip -o allegro-release.aar classes.jar

popd

# Generate R.java
echo "Generating R.java..."
mkdir -p build/parts/java/build/src
"$SDK_BUILD_TOOLS/aapt" package -f \
    -m -J build/parts/java/build/src \
    -S build/parts/java/src/res \
    -M build/parts/java/src/AndroidManifest.xml \
    -I "$SDK_PLATFORM/android.jar"

# Compile Java files
echo "Compiling Java files..."
"$JAVA_HOME/bin/javac" \
    -d build/parts/java/build/obj \
    -source 1.7 \
    -target 1.7 \
    -classpath "$SDK_PLATFORM/android.jar:build/stage/libs/classes.jar" \
    -sourcepath build/parts/java/src/java/org/opensurge2d/surgeengine \
    build/parts/java/build/src/org/opensurge2d/surgeengine/R.java \
    build/parts/java/src/java/org/opensurge2d/surgeengine/*.java

# Generate the DEX bytecode
echo "Generating DEX bytecode..."
"$SDK_BUILD_TOOLS/d8" \
    --output build/parts/java/build \
    --lib "$SDK_PLATFORM/android.jar" \
    build/stage/libs/classes.jar \
    build/parts/java/build/obj/org/opensurge2d/surgeengine/*.class

# Success!
echo "The Java part has been built!"
