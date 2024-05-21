#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# stage_allegro.sh
# Stage Allegro
#
set -e

echo "Staging Allegro..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$PREFIX" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# Stage
pushd build/parts/allegro/build/$ABI
make install
popd

# Stage allegro-release.aar
ALLEGRO_AAR=build/parts/allegro/build/$ABI/lib/allegro-release.aar
if [[ ! -f "$ALLEGRO_AAR" ]]; then
    echo "ERROR: Allegro AAR not found at $ALLEGRO_AAR"
    exit 1
fi
cp -v "$ALLEGRO_AAR" build/stage/libs

# Validate
for lib in liballegro.so \
    liballegro_main.so \
    liballegro_image.so \
    liballegro_primitives.so \
    liballegro_color.so \
    liballegro_font.so \
    liballegro_ttf.so \
    liballegro_acodec.so \
    liballegro_audio.so \
    liballegro_dialog.so \
    liballegro_memfile.so \
    liballegro_physfs.so
do
    if [[ ! -f "$PREFIX/lib/$lib" ]]; then
        echo "ERROR: $lib not found!"
        exit 1
    fi
done

# Success!
echo "Allegro for $ABI has been staged!"
