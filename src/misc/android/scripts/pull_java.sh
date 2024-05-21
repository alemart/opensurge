#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# pull_java.sh
# Pull the Java part
#
set -e

echo "Pulling the Java part..."

src_dir="$(realpath "$(dirname "$0")/../src")"

pushd build/parts/java/src

# local copy
pushd "$src_dir"
tar cf - --exclude=cpp . | ( popd && tar xvf - )
popd

popd
