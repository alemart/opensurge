#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# stage_opensurge.sh
# Stage opensurge
#
set -e

echo "Staging opensurge..."
source "$(dirname "$0")/set_ndk.sh" "$@"

if [[ -z "$ABI" || -z "$PREFIX" ]]; then
    echo "ERROR: unspecified ABI parameters"
    exit 1
fi

# Stage
pushd build/parts/opensurge/build/$ABI
make install
popd

# Validate
lib=libopensurge.so
if [[ ! -f "$PREFIX/lib/$lib" ]]; then
    echo "ERROR: $lib not found!"
    exit 1
fi

# Success!
echo "opensurge for $ABI has been staged!"
