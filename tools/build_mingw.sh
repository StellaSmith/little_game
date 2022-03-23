#!/bin/sh


###
# Build tools
###

set -e

cmake -S tools/ -B build/tools-build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/tools-build/ --parallel
cmake --install build/tools-build/ --prefix build/tools-install/

export PATH=build/tools-install/bin:$PATH

###
# Feel free to modify this file to use the proper environment for your case
###

# arch=i686
arch=x86_64
bindir=build
c=gcc
cxx=g++
toolchain=/usr/$arch-w64-mingw32
target_host=$arch-w64-mingw32
build_type=Release
 
export CONAN_BASH_PATH=$(realpath $(command -v sh))
# export CONAN_SYSREQUIRES_MODE=enabled
export CONAN_CMAKE_SYSROOT="$toolchain"
export CONAN_CMAKE_FIND_ROOT_PATH="$toolchain"
export CONAN_CMAKE_SYSROOT="$toolchain"
export CONAN_CMAKE_SYSTEM_NAME="Windows"
export CONAN_CMAKE_SYSTEM_PROCESSOR=$arch
export CHOST="$target_host"
export AR="$target_host-ar"
export AS="$target_host-as"
export RANLIB="$target_host-ranlib"
export CC="$target_host-$c"
export CXX="$target_host-$cxx"
export STRIP="$target_host-strip"
export RC="$target_host-windres"

# statically link against libgcc, libstdc++, and libwinpthread
export LDFLAGS="-static-libgcc -static-libstdc++ -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive"

echo Using mingw toolchain at $toolchain 1>&2
echo Targeting $target_host 1>&2

# x: print executed commands
# e: exit on error
# set -xe

cmake -S . -B $bindir $generator -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_PROCESSOR=$arch -DCMAKE_FIND_ROOT_PATH=$toolchain
exec cmake --build $bindir --parallel $(nproc)