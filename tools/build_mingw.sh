#!/bin/sh

###
# Feel free to modify this file to use the proper environment for your case
###

# arch=i686
arch=x86_64
toolchain=/usr/$arch-w64-mingw32
target_host=$arch-w64-mingw32
build_type=Release

###
# Build tools
###
set -e
mkdir -p build

if [ ! -e build/.tools_timestamp ] || [ -n "$(find recipes/vgame_tools tools/ -newer build/.tools_timestamp -print -quit)" ]; then
    conan create recipes/vgame_tools vgame_tools/latest@ -u -b missing -s compiler.cppstd=20
    touch -m build/.tools_timestamp
fi

read _ _ GCC_VERSION <<< $($target_host-cc --version)
GCC_VERSION=$(sed -E 's/([0-9]+\.[0-9]+).*?/\1/' <<< $GCC_VERSION)
mingw_profile="[settings]
build_type=$build_type
os=Windows
arch=$arch
compiler=gcc
compiler.version=$GCC_VERSION
compiler.cppstd=20
compiler.libcxx=libstdc++11
compiler.threads=win32
compiler.exception=seh"
cat <<< "$mingw_profile" > build/mingw64_profile

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
export CC="$target_host-cc"
export CXX="$target_host-c++"
export STRIP="$target_host-strip"
export RC="$target_host-windres"

# statically link against libgcc, libstdc++, and libwinpthread
export LDFLAGS="-Wl,-push-state -Wl,-Bstatic,--whole-archive -Wl,-as-needed -static-libgcc -static-libstdc++ -lwinpthread -Wl,-pop-state"

echo Using mingw toolchain at $toolchain 1>&2
echo Targeting $target_host 1>&2

if [ ! -e build/.install_timestamp ] || [ -n "$(find recipes/vgame/ -newer build/.install_timestamp -print -quit)" ]; then
    conan install recipes/vgame/ vgame/latest@ -if build/ -of build/ -pr:b ./mingw64_profile -u -b missing
    touch -m build/.install_timestamp
fi

exec conan build recipes/vgame -bf build/
