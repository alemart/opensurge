#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# assemble_prepare.sh
# Prepare APK assemblage
#
set -e

echo "Preparing APK assemblage..."

# Validate
if [[ ! -d "build/stage/sysroot/arm64-v8a" ]]; then
    echo "Can't find arm64-v8a in build/stage/sysroot/. Is it in the ABI_LIST?"
    exit 1
fi

# Copying files from the stage area to build/apk/
cp -v build/stage/AndroidManifest.xml build/apk/
cp -v build/stage/classes.dex build/apk/
cp -v -r build/stage/res/* build/apk/res/

# Copying native libraries to build/apk/lib/
for abi in arm64-v8a armeabi-v7a x86_64 x86; do
    if [[ -d "build/stage/sysroot/$abi/usr/lib" ]]; then
        mkdir -p build/apk/lib/$abi/
        cp -v build/stage/sysroot/$abi/usr/lib/*.so build/apk/lib/$abi/
    fi
done

# Copying game assets to build/apk/assets/
cp -v -r build/stage/sysroot/arm64-v8a/usr/share/games/opensurge/* build/apk/assets/
rm -rf build/apk/assets/src

if [[ -z "$(ls -A build/apk/assets)" ]]; then
    echo "Can't copy game assets to build/apk/assets/."
    exit 1
fi