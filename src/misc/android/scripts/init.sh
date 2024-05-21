#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# init.sh
# Create the build folder
#

echo "Creating the build folder..."

mkdir -p build
pushd build

# prime area
mkdir -p apk
mkdir -p apk/lib
mkdir -p apk/res
mkdir -p apk/assets

# stage area
mkdir -p stage

mkdir -p stage/libs
mkdir -p stage/res

mkdir -p stage/sysroot
mkdir -p stage/sysroot/arm64-v8a
mkdir -p stage/sysroot/armeabi-v7a
mkdir -p stage/sysroot/x86_64
mkdir -p stage/sysroot/x86

# source & build areas for each part
mkdir -p parts

mkdir -p parts/opensurge
mkdir -p parts/opensurge/src
mkdir -p parts/opensurge/build
mkdir -p parts/opensurge/build/arm64-v8a
mkdir -p parts/opensurge/build/armeabi-v7a
mkdir -p parts/opensurge/build/x86_64
mkdir -p parts/opensurge/build/x86

mkdir -p parts/surgescript
mkdir -p parts/surgescript/src
mkdir -p parts/surgescript/build
mkdir -p parts/surgescript/build/arm64-v8a
mkdir -p parts/surgescript/build/armeabi-v7a
mkdir -p parts/surgescript/build/x86_64
mkdir -p parts/surgescript/build/x86

mkdir -p parts/allegro
mkdir -p parts/allegro/src
mkdir -p parts/allegro/build
mkdir -p parts/allegro/build/arm64-v8a
mkdir -p parts/allegro/build/armeabi-v7a
mkdir -p parts/allegro/build/x86_64
mkdir -p parts/allegro/build/x86

mkdir -p parts/deps
mkdir -p parts/deps/src
mkdir -p parts/deps/build
mkdir -p parts/deps/build/arm64-v8a
mkdir -p parts/deps/build/armeabi-v7a
mkdir -p parts/deps/build/x86_64
mkdir -p parts/deps/build/x86

mkdir -p parts/java
mkdir -p parts/java/src
mkdir -p parts/java/build

popd
