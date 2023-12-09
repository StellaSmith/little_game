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

if [[ ! -e "$generators_directory" ]]; then
    conan install . -pr:h default -s:h compiler.cppstd=20 -b missing -s "build_type=$build_type"  -u;
fi;


# shellcheck source=/dev/null
source "$generators_directory/conanbuild.sh";
if [[ ! -e "$build_directory/CMakeCache.txt" ]]; then
    cmake --preset "conan-$1"
fi;

cmake --build --preset "conan-$1" --parallel;

# shellcheck source=/dev/null
source "$generators_directory/conanrun.sh";
cmake --install "$build_directory" --prefix "$PWD/install";
