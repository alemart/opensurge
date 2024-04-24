#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# build_deps.sh
# Build Allegro dependencies
#
set -e

echo "Building Allegro dependencies..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$API" || -z "$HOST" || -z "$PREFIX" || -z "$NDK_TOOLCHAIN_CMAKE" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

pushd build/parts/deps/build/$ABI

# -----

# Build libfreetype
mkdir -p libfreetype
pushd libfreetype

../../../src/libfreetype/configure \
    --host=$HOST \
    "--prefix=$PREFIX" \
    --enable-static --disable-shared \
    --without-png --without-zlib --without-harfbuzz --without-bzip2 --without-brotli \
    CFLAGS="-fPIC -DPIC"

make

popd

# Build libogg
mkdir -p libogg
pushd libogg

cmake ../../../src/libogg \
    "-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_CMAKE" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    "-DCMAKE_INSTALL_PREFIX=$PREFIX" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_VERBOSE_MAKEFILE=on \
    -DBUILD_SHARED_LIBS=OFF

make

# make install should be run in a separate staging phase, but libvorbis below
# depends on this. Ideally, libogg and libvorbis would be split into different
# parts. Doing so seems like overkill, though?
make install

popd

# Build libvorbis
mkdir -p libvorbis
pushd libvorbis

cmake ../../../src/libvorbis \
    "-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_CMAKE" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    "-DCMAKE_INSTALL_PREFIX=$PREFIX" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_VERBOSE_MAKEFILE=on \
    -DBUILD_SHARED_LIBS=OFF \
    "-DOGG_LIBRARY=$PREFIX/lib/libogg.a" \
    "-DOGG_INCLUDE_DIR=$PREFIX/include"

make

popd

# Build libphysfs
mkdir -p libphysfs
pushd libphysfs

cmake ../../../src/libphysfs \
    "-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_CMAKE" \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API \
    "-DCMAKE_INSTALL_PREFIX=$PREFIX" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_VERBOSE_MAKEFILE=on \
    -DPHYSFS_BUILD_SHARED=ON \
    -DPHYSFS_BUILD_STATIC=OFF

make

popd

# -----

popd

# Success!
echo "The Allegro dependencies have been built for $ABI!"
