#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# build_allegro.sh
# Build Allegro
#
set -e

echo "Building Allegro..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$API" || -z "$PREFIX" || -z "$NDK_TOOLCHAIN_CMAKE" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# Build
pushd build/parts/allegro/build/$ABI

ln -s "$PREFIX" deps

cmake ../../src \
    "-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_CMAKE" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    "-DCMAKE_INSTALL_PREFIX=$PREFIX" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_VERBOSE_MAKEFILE=on \
    -DWANT_MONOLITH=off -DSHARED=on -DPREFER_STATIC_DEPS=on \
    -DWANT_GLES3=on -DWANT_MP3=off -DWANT_MODAUDIO=off \
    -DWANT_EXAMPLES=off -DWANT_DEMO=off -DWANT_TESTS=off \
    -DWANT_DOCS=off -DWANT_ANDROID_LEGACY=off

make

popd

# Success!
echo "Allegro has been built for $ABI!"
