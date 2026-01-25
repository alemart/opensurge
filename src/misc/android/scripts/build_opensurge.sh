#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# build_opensurge.sh
# Build opensurge
#
set -e

echo "Building opensurge..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$API" || -z "$PREFIX" || -z "$NDK_TOOLCHAIN_CMAKE" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# Usage
function usage()
{
    echo "Usage: build_opensurge.sh [-v <build-version-string>]"
}

# Read positional arguments
unset GAME_BUILD_VERSION
while getopts "a:m:N:v:" OPTION; do
    case $OPTION in
        v) GAME_BUILD_VERSION="$OPTARG";;
        a|m|N) ;;
        ?) echo "Invalid argument"; usage; exit 1;;
    esac
done

# Set defaults
GAME_BUILD_VERSION="${GAME_BUILD_VERSION:-unofficial}"
echo "Using GAME_BUILD_VERSION = $GAME_BUILD_VERSION"

# Build
pushd build/parts/opensurge/build/$ABI

cmake ../../src \
    "-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_CMAKE" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    "-DCMAKE_INSTALL_PREFIX=$PREFIX" \
    "-DCMAKE_FIND_ROOT_PATH=$PREFIX" \
    "-DGAME_BINDIR=$PREFIX/lib" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_VERBOSE_MAKEFILE=on \
    -DALLEGRO_STATIC=off -DALLEGRO_MONOLITH=off \
    -DSURGESCRIPT_STATIC=on \
    -DWANT_GLES=on -DWANT_BETTER_GAMEPAD=off \
    "-DGAME_BUILD_VERSION=$GAME_BUILD_VERSION" \
    -DWANT_BUILD_DATE=on

make

popd

# Success!
echo "opensurge has been built for $ABI!"
