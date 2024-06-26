#!/bin/sh

set -ex -o pipefail

###
# Feel free to modify this file to use the proper environment for your case
###

# arch=i686
arch=x86_64
os=Windows
toolchain=/usr/${arch}-w64-mingw32
target_host=${arch}-w64-mingw32
build_type=Debug

cc=${target_host}-gcc
cxx=${target_host}-g++
as=${target_host}-as
ar=${target_host}-ar
ranlib=${target_host}-ranlib
strip=${target_host}-strip
windres=${target_host}-windres

GCC_INFO=$(${cc} --verbose 2>&1)
GCC_VERSION=$(echo "${GCC_INFO}" | sed -nr 's/^gcc version ([0-9]+\.[0-9]+).*?/\1/p')
THREAD_MODEL=$(echo "${GCC_INFO}" | sed -nr 's/^Thread model: (.*)$/\1/p')

mkdir -p build/cross/${target_host}
cat >build/cross/${target_host}/profile <<EOF
[settings]
build_type=${build_type}
os=${os}
arch=${arch}
compiler=gcc
compiler.version=${GCC_VERSION}
compiler.cppstd=gnu20
compiler.libcxx=libstdc++11
compiler.threads=${THREAD_MODEL}
compiler.exception=seh
[conf]
tools.build:sysroot=
tools.cmake.cmaketoolchain:system_name=${os}
tools.cmake.cmaketoolchain:system_processor=${arch}
tools.build:compiler_executables={"c": "${cc}", "cxx": "${cxx}", "asm": "${as}", "rc": "${windres}"}
[options]
boost/*:i18n_backend_iconv=libiconv
boost/*:without_stacktrace=True
[buildenv]
CHOST=${target_host}
AR=${ar}
AS=${as}
RANLIB=${ranlib}
CC=${cc}
CXX=${cxx}
STRIP=${strip}
RC=${windres}
EOF
# LDFLAGS=-Wl,-push-state -Wl,-Bstatic,--whole-archive -Wl,-as-needed -static-libgcc -static-libstdc++ $([ "${THREAD_MODEL}" = "posix" ] && echo "-lwinpthread") -Wl,-pop-state
# CXXFLAGS=-fext-numeric-literals

# statically link against libgcc, libstdc++, and libwinpthread

echo Using mingw toolchain at ${toolchain} 1>&2
echo Targeting ${target_host} 1>&2

exec conan build . -pr:b default -s:b compiler.cppstd=gnu20 -pr:h build/cross/${target_host}/profile -b missing
