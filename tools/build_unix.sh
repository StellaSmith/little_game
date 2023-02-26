#!/bin/sh

set -e -o pipefail

if [ ! -e build/vgame_tools/.timestamp ] || [ -n "$(find tools/ -newer build/vgame_tools/.timestamp -print -quit)" ]; then
    echo "#### exporting tools ####"
    conan install -if build/vgame_tools -b missing recipes/vgame_tools/ vgame_tools/latest@ -pr:b default -pr:h default -s:h compiler.cppstd=20 -s:b compiler.cppstd=20 &&
    conan export -l  build/vgame_tools/conan.lock recipes/vgame_tools/ vgame_tools/latest@ &&
    touch -m build/vgame_tools/.timestamp
fi

if [ ! -e build/vgame/.timestamp ] || [ -n "$(find ./ ! -path './.git/*' ! -exec git check-ignore -q {} ';' -newer build/vgame/.timestamp -print -quit)" ]; then
    echo "#### building game ####"
    conan install recipes/vgame/ vgame/latest@ -b missing -of build/vgame -if build/vgame -pr:b default -pr:h default -s:h compiler.cppstd=20 -s:b compiler.cppstd=20 \
        -s build_type=Debug -o vgame:with_opengl=False -o vgame:with_vulkan=True &&
    conan build recipes/vgame/ -bf build/vgame -sf ./ &&
    touch -m build/vgame/.timestamp
fi

echo "#### updating compile_commands.json ####"
rm -f build/compile_commands.json;
ln -s $(realpath build/vgame/compile_commands.json) build/
