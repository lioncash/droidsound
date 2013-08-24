#!/bin/bash

MYDIR="$(dirname $0)"
TARGET="$MYDIR"

if [[ $# -ne 1 ]]; then
        echo "Give path to SC68 source tree svn checkout"
        exit 1;
fi
SC68DIR="$1"

if [[ ! -e "$SC68DIR/INSTALL" ]]; then
        echo "The given SC68 source directory '$SC68DIR' must have the 'INSTALL' file.";
        exit 1;
fi

if [[ ! -e "$SC68DIR/libsc68/sc68/trap68.h" ]]; then
        echo "Build SC68 tree once to generate trap68.h"
        exit 1;
fi

echo "Copying SVN checkout..."
for p in "libsc68" "file68" "unice68"; do
    rm -rf "$TARGET/$p"
    git rm -rf "$TARGET/$p"
    cp -a "$SC68DIR/$p" "$TARGET"
done

echo "Removing SVN and autotools specific files, editor backups, etc."
find "$TARGET" -name '.svn' -type d -prune -exec rm -rf '{}' ';'
find "$TARGET" -name '*.in' -type f -exec rm -rf '{}' ';'
find "$TARGET" -name '*.am' -type f -exec rm -rf '{}' ';'
find "$TARGET" -name 'configure' -type f -exec rm -rf '{}' ';'
find "$TARGET" -name 'autom4te.cache' -type d -prune -exec rm -rf '{}' ';'
find "$TARGET" -name '*~' -type f -exec rm -rf '{}' ';'

echo "Adding new file set to git..."
git add "$TARGET"
