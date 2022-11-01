#!/bin/sh

###
# Feel free to modify this file to use the proper environment for your case
###

# arch=i686
arch=x86_64
os=Windows
toolchain=/usr/${arch}-w64-mingw32
target_host=${arch}-w64-mingw32
build_type=Release
cc=${target_host}-cc
cxx=${target_host}-c++

###
# Build tools
###
set -e
mkdir -p build/cross/${target_host}/

if [ ! -e build/vgame_tools/.timestamp ] || [ -n "$(find recipes/vgame_tools tools/ -newer build/vgame_tools/.timestamp -print -quit)" ]; then
    echo "#### exporting tools ####"
    conan install -if build/vgame_tools -b missing recipes/vgame_tools/ vgame_tools/latest@ -pr:b default -pr:h default -s:h compiler.cppstd=20 -s:b compiler.cppstd=20 &&
    conan export -l  build/vgame_tools/conan.lock recipes/vgame_tools/ vgame_tools/latest@ &&
    touch -m build/vgame_tools/.timestamp
fi

set +e

GCC_INFO=$(${cc} --verbose 2>&1)
GCC_VERSION=$(sed -nr 's/^gcc version ([0-9]+\.[0-9]+).*?/\1/p' <<< ${GCC_INFO})
THREAD_MODEL=$(sed -nr 's/^Thread model: (.*)$/\1/p' <<< ${GCC_INFO})

cat <<< "[settings]
build_type=${build_type}
os=${os}
arch=${arch}
compiler=gcc
compiler.version=${GCC_VERSION}
compiler.cppstd=gnu20
compiler.libcxx=libstdc++11
compiler.threads=${THREAD_MODEL}
compiler.exception=seh
[options]
boost:i18n_backend_iconv=libiconv
boost:without_stacktrace=True
[env]
CONAN_BASH_PATH=$(realpath $(command -v sh))
CONAN_CMAKE_SYSROOT=${toolchain}
CONAN_CMAKE_FIND_ROOT_PATH=${toolchain}
CONAN_CMAKE_SYSTEM_NAME=${os}
CONAN_CMAKE_SYSTEM_PROCESSOR=${arch}
CHOST=$target_host
AR=$target_host-ar
AS=$target_host-as
RANLIB=$target_host-ranlib
CC=${cc}
CXX=${cxx}
STRIP=$target_host-strip
RC=$target_host-windres
CXXFLAGS=-fext-numeric-literals
LDFLAGS=-Wl,-push-state -Wl,-Bstatic,--whole-archive -Wl,-as-needed -static-libgcc -static-libstdc++ $([ "${THREAD_MODEL}" = "posix" ] && echo "-lwinpthread") -Wl,-pop-state
" > build/cross/${target_host}/profile

# statically link against libgcc, libstdc++, and libwinpthread

echo Using mingw toolchain at ${toolchain} 1>&2
echo Targeting ${target_host} 1>&2


set -e
if [ ! -e build/cross/${target_host}/.timestamp ] || [ -n "$(find . ! -path './.git/*' ! -exec git check-ignore -q {} ';' -newer build/cross/${target_host}/.timestamp -print -quit)" ]; then
    echo "#### building game ####"
    conan install recipes/vgame/ vgame/latest@ -if build/cross/${target_host}/ -pr:b default -s:b compiler.cppstd=gnu20 -pr:h build/cross/${target_host}/profile -b missing
    conan build recipes/vgame/ -bf build/cross/${target_host}/
    touch -m build/cross/${target_host}/.timestamp
fi

echo "#### updating compile_commands.json ####"
rm -f build/compile_commands.json;
ln -s $(realpath build/cross/${target_host}/compile_commands.json) build/