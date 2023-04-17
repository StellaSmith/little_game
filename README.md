# little_game
A little project aimed to create a minecraft-like game.

[![Build Linux](https://github.com/StellaSmith/little_game/actions/workflows/linux.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/linux.yml)
[![Build Win32](https://github.com/StellaSmith/little_game/actions/workflows/win32.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/win32.yml)
[![Build MinGW-w64](https://github.com/StellaSmith/little_game/actions/workflows/mingw.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/mingw.yml)
[![Build MSYS2](https://github.com/StellaSmith/little_game/actions/workflows/msys2.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/msys2.yml)

## Building
### Build Dependencies
Programs needed in order to build, links are included for the windows installers.
- C & C++ compilers
  - [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- [Python](https://www.python.org/downloads/)
- [CMake](https://cmake.org/download/)

The rest of the build tools should be provided by conan
## Building
```bash
cd little_game
mkdir build
# Make sure we have conan installed
python -m pip install --user --update conan
# Finally, building
conan build . --build missing
```
The last step will also download and build dependecies if required.