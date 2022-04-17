#!/bin/sh

set -e

mkdir -p build
if [ ! -e build/.tools_timestamp ] || [ -n "$(find tools/ -newer build/.tools_timestamp -print -quit)" ]; then
    conan create recipes/vgame_tools/ vgame_tools/latest@ -u -b missing -s compiler.cppstd=20
    touch -m build/.tools_timestamp
fi

if [ ! -e build/.install_timestamp ] || [ -n "$(find recipes/vgame/ -newer build/.install_timestamp -print -quit)" ]; then
    conan install recipes/vgame/ vgame/latest@ -if build/ -of build/ -u -b missing -s compiler.cppstd=gnu20
    touch -m build/.install_timestamp
fi

exec conan build recipes/vgame -bf build/
