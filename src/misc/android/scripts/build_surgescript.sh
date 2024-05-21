#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# build_surgescript.sh
# Build SurgeScript
#
set -e

echo "Building SurgeScript..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$API" || -z "$PREFIX" || -z "$NDK_TOOLCHAIN_CMAKE" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# libsurgescript

pushd build/parts/surgescript/build/$ABI

cmake ../../src \
    "-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_CMAKE" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    "-DCMAKE_INSTALL_PREFIX=$PREFIX" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_VERBOSE_MAKEFILE=on \
    -DWANT_EXECUTABLE=off \
    -DWANT_SHARED=off \
    -DWANT_STATIC=on

make

popd

# Success!
echo "SurgeScript has been built for $ABI!"
