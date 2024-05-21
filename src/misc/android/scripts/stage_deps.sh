#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# stage_deps.sh
# Stage Allegro dependencies
#
set -e

echo "Staging Allegro dependencies..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$PREFIX" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# Stage
for lib in libfreetype libogg libvorbis libphysfs; do
    pushd build/parts/deps/build/$ABI/$lib
    make install
    popd
done

# Validate
for lib in libfreetype.a libogg.a libvorbis.a libvorbisfile.a libphysfs.so; do
    if [[ ! -f "$PREFIX/lib/$lib" ]]; then
        echo "ERROR: $lib not found!"
        exit 1
    fi
done

# Success!
echo "The Allegro dependencies for $ABI have been staged!"
