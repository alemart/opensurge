#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# stage_surgescript.sh
# Stage SurgeScript
#
set -e

echo "Staging SurgeScript..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$PREFIX" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# Stage
pushd build/parts/surgescript/build/$ABI
make install
popd

# Validate
lib=libsurgescript-static.a
if [[ ! -f "$PREFIX/lib/$lib" ]]; then
    echo "ERROR: $lib not found!"
    exit 1
fi

# Success!
echo "SurgeScript for $ABI has been staged!"
