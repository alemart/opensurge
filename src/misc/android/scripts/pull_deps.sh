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

pushd build/parts/deps/src

echo "Pulling libfreetype..."
wget http://downloads.sourceforge.net/project/freetype/freetype2/2.12.1/freetype-2.12.1.tar.gz
libfreetype=$(basename --suffix=.tar.gz ./freetype-*.tar.gz)
tar xvzf $libfreetype.tar.gz
mv $libfreetype libfreetype

echo "Pulling libogg..."
wget https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.gz
libogg=$(basename --suffix=.tar.gz ./libogg-*.tar.gz)
tar xvzf $libogg.tar.gz
mv $libogg libogg

echo "Pulling libvorbis..."
wget https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.gz
libvorbis=$(basename --suffix=.tar.gz ./libvorbis-*.tar.gz)
tar xvzf $libvorbis.tar.gz
mv $libvorbis libvorbis

echo "Pulling libphysfs..."
wget https://github.com/icculus/physfs/archive/refs/tags/release-3.2.0.tar.gz -O physfs-release-3.2.0.tar.gz
libphysfs=$(basename --suffix=.tar.gz ./physfs-*.tar.gz)
tar xvzf $libphysfs.tar.gz
mv $libphysfs libphysfs

popd