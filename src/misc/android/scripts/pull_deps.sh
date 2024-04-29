#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# pull_deps.sh
# Pull Allegro dependencies
#
set -e

echo "Pulling Allegro dependencies..."

function sha256check()
{
    local hash=$(sha256sum "$1" | sed 's/\s.*//')

    if [[ "$hash" != "$2" ]]; then
        echo "Can't verify $1. Expected SHA-256 checksum $2, found $hash."
        exit 1
    fi
}

pushd build/parts/deps/src

echo "Pulling libfreetype..."
wget http://downloads.sourceforge.net/project/freetype/freetype2/2.12.1/freetype-2.12.1.tar.gz
libfreetype=$(basename --suffix=.tar.gz ./freetype-*.tar.gz)
sha256check $libfreetype.tar.gz efe71fd4b8246f1b0b1b9bfca13cfff1c9ad85930340c27df469733bbb620938
tar xvzf $libfreetype.tar.gz
mv $libfreetype libfreetype
rm -f $libfreetype.tar.gz

echo "Pulling libogg..."
wget https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.gz
libogg=$(basename --suffix=.tar.gz ./libogg-*.tar.gz)
sha256check $libogg.tar.gz 0eb4b4b9420a0f51db142ba3f9c64b333f826532dc0f48c6410ae51f4799b664
tar xvzf $libogg.tar.gz
mv $libogg libogg
rm -f $libogg.tar.gz

echo "Pulling libvorbis..."
wget https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.gz
libvorbis=$(basename --suffix=.tar.gz ./libvorbis-*.tar.gz)
sha256check $libvorbis.tar.gz 0e982409a9c3fc82ee06e08205b1355e5c6aa4c36bca58146ef399621b0ce5ab
tar xvzf $libvorbis.tar.gz
mv $libvorbis libvorbis
rm -f $libvorbis.tar.gz

echo "Pulling libphysfs..."
wget https://github.com/icculus/physfs/archive/refs/tags/release-3.2.0.tar.gz -O physfs-release-3.2.0.tar.gz
libphysfs=$(basename --suffix=.tar.gz ./physfs-*.tar.gz)
sha256check $libphysfs.tar.gz 1991500eaeb8d5325e3a8361847ff3bf8e03ec89252b7915e1f25b3f8ab5d560
tar xvzf $libphysfs.tar.gz
mv $libphysfs libphysfs
rm -f $libphysfs.tar.gz

popd