#!/bin/bash

set -e -o pipefail

case "$1" in
    "debug")
        declare -r build_type="Debug";
    ;;
    "release")
        declare -r build_type="Release";
    ;;
    *)
        echo "Unkown build_type $build_type" > /dev/stderr;
        exit 1;
    ;;
esac;

declare -r build_directory="$PWD/build/$build_type";
declare -r generators_directory="$build_directory/generators";

if [ ! -v "CONAN_HOME" ]; then
    export CONAN_HOME="$PWD/.conan";
fi;

if ! command -v conan >& /dev/null; then
    python -m venv "$PWD/.venv";
    pip install conan;
fi;

export CC=$(command -v cc)
export CXX=$(command -v c++)
export CMAKE_GENERATOR=Ninja

if [ ! -e "$CONAN_HOME" ]; then
    conan profile detect;
fi;

if [[ ! -e "$generators_directory" ]]; then
    nice conan install . -pr:h default \
        -s:h compiler.cppstd=20 \
        -b missing \
        -s "build_type=$build_type" \
        -o "boost/*:without_cobalt=True" \
        -o "boost/*:without_cobalt_io=True" \
        -u;
fi;


# shellcheck source=/dev/null
source "$generators_directory/conanbuild.sh";
if [[ ! -e "$build_directory/CMakeCache.txt" ]]; then
    nice cmake --preset "conan-$1";
fi;

nice cmake --build --preset "conan-$1" --parallel;

# shellcheck source=/dev/null
source "$generators_directory/conanrun.sh";
nice cmake --install "$build_directory" --prefix "$PWD/export" --parallel $(nproc);
