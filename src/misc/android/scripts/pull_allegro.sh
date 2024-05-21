#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# pull_allegro.sh
# Pull Allegro
#
set -e

echo "Pulling Allegro..."

src_dir="$(realpath "$(dirname "$0")/../src/cpp/allegro")"
git_pull="$(realpath "$(dirname "$0")/git_pull.sh")"

pushd build/parts/allegro/src

if [[ ! -d "$src_dir" ]]; then

    # remote copy
    "$git_pull" \
        --repository https://github.com/liballeg/allegro5.git \
        --commit c4bc6dcb72f4a338c870de75f800d245c241e6c1 \
    ;

else

    # local copy
    pushd "$src_dir"
    tar cf - --exclude=.git . | ( popd && tar xvf - )
    popd

fi

popd
