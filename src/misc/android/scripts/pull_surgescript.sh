#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# pull_surgescript.sh
# Pull SurgeScript
#
set -e

echo "Pulling SurgeScript..."

src_dir="$(realpath "$(dirname "$0")/../src/cpp/surgescript")"
git_pull="$(realpath "$(dirname "$0")/git_pull.sh")"

pushd build/parts/surgescript/src

if [[ ! -d "$src_dir" ]]; then

    # remote copy
    "$git_pull" \
        --repository https://github.com/alemart/surgescript.git \
        --commit 8c88a7afdbc732d153507b0afa295f31e70cdbd0 \
        `# --tag v0.6.0` \
    ;

else

    # local copy
    pushd "$src_dir"
    tar cf - --exclude=.git . | ( popd && tar xvf - )
    popd

fi

popd
