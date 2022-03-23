#!/bin/sh

set -e

cmake -S tools/ -B build/tools-build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/tools-build/ --parallel
cmake --install build/tools-build/ --prefix build/tools-install/

export PATH=build/tools-install/bin:$PATH

cmake -S ./ -B build/ -DCMAKE_BUILD_TYPE=Release
exec cmake --build build/ --parallel
