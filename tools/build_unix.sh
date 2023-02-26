#!/bin/bash

set -e -o pipefail

mkdir -p build/vgame{_tools,}

if [ ! -e build/vgame_tools/.timestamp ] || ! conan list -c "vgame_tools" | grep ERROR >/dev/null 2>&1 || [ -n "$(find tools/ -newer build/vgame_tools/.timestamp -print -quit)" ]; then
    echo "#### exporting tools ####"
    conan export --build-require recipes/vgame_tools/ --version none
    touch -m build/vgame_tools/.timestamp
fi

if [ ! -e build/vgame/.timestamp ] || [ -n "$(find ./ ! -path './.git/*' ! -exec git check-ignore -q {} ';' -newer build/vgame/.timestamp -print -quit)" ]; then
    echo "#### building game ####"
    conan install -u recipes/vgame/ --version none -b missing -of build/vgame -pr:b default -pr:h default -s build_type=Debug
    conan build recipes/vgame/ -of build/vgame
    touch -m build/vgame/.timestamp
fi

echo "#### updating compile_commands.json ####"
rm -f build/compile_commands.json
ln -s "$(realpath build/vgame/compile_commands.json)" build/
