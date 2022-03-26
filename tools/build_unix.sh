#!/bin/sh

set -e

if [ ! -e build/tools-build/.timestamp ] || [ -n "$(find tools/ -newer build/tools-build/.timestamp -print -quit)" ]; then
    cmake -S tools/ -B build/tools-build/ -DCMAKE_BUILD_TYPE=Release
    cmake --build build/tools-build/ --parallel
    cmake --install build/tools-build/ --prefix build/tools-install/
    touch -m build/tools-build/.timestamp
fi

export PATH=build/tools-install/bin:$PATH

cmake -S ./ -B build/ -DCMAKE_BUILD_TYPE=Release
exec cmake --build build/ --parallel
