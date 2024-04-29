#!/bin/bash
#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#
# git_pull.sh
# Download a project from a Git repository
#
set -e

# Usage
function usage()
{
    echo "Usage: git_pull.sh --repository <location> [--commit <hash>] [--tag <tag>] [--branch <branch>]"
}

# Read positional arguments
unset git_repository
unset git_commit
unset git_tag
unset git_branch
while [[ "$#" -gt 0 ]]; do
    case "$1" in
        --repository) git_repository=$2; shift;;
        --commit) git_commit=$2; shift;;
        --tag) git_tag=$2; shift;;
        --branch) git_branch=$2; shift;;
        *) echo "Invalid argument: $1"; usage; exit 1;;
    esac
    shift
done

# Validate
if [[ -z "$git_repository" ]]; then
    echo "Unspecified git repository"
    usage
    exit 1
fi

# Download the project
echo "Pulling from $git_repository..."

git init
git remote add origin "$git_repository"
git pull origin ${git_branch:-master}

if [[ -n "$git_commit" ]]; then
    git checkout "$git_commit"
elif [[ -n "$git_tag" ]]; then
    git fetch --tags
    git checkout "tags/$git_tag"
fi
