# little_game
A little project aimed to create a minecraft-like game.

[![Build Linux](https://github.com/StellaSmith/little_game/actions/workflows/linux.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/linux.yml)
[![Build Win32](https://github.com/StellaSmith/little_game/actions/workflows/win32.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/win32.yml)
[![Build MinGW-w64](https://github.com/StellaSmith/little_game/actions/workflows/mingw.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/mingw.yml)
[![Build MSYS2](https://github.com/StellaSmith/little_game/actions/workflows/msys2.yml/badge.svg)](https://github.com/StellaSmith/little_game/actions/workflows/msys2.yml)

## Building
### Windows
- Make sure you have [Python](https://www.python.org/downloads/windows/) and [Visual Studio](https://visualstudio.microsoft.com/) installed with the C++ desktop development tools.
- Clone or download the repository.
- Open the PowerShell in the root folder of the source (where the CMakeLists.txt file is located).
  ```powershell
  # Make sure we have conan installed
  pip install --update conan
  # Add the bincrafters conan repository for SDL2
  conan remote add bincrafters https://bincrafters.jfrog.io/artifactory/api/conan/conan
  # Install and build dependencies
  mkdir build
  pushd build
  conan install .. --build=missing
  popd
  # Activate virtual enviroment
  .\build\activate.ps1
  # Configure the project
  cmake -S . -B build
  # And finally, building
  cmake --build build --target little_game --config Release --parallel
  # Install on default destination (C:\Program Files\little_game)
  cmake --install build --config Release
  # Or install under your user
  cmake --install build --config Release --prefix %AppData%\.little_game
  # Deactivate the virtual enviroment
  .\build\deactivate.ps1
  # Now you can run the game by
  C:\Program Files\little_game\bin\little_game
  # Or
  %AppData%\.little_game\bin\little_game
  ```
### Linux
- Make sure you have a graphical enviroment installed (X or Wayland).
- Open a terminal
  ```bash
  # Make sure we have python, git and a build enviroment installed
  #  Arch Based
  sudo pacman -S --needed python git base-devel
  #  Debian Based
  sudo apt install python3 python3-pip git build-essential
  # Clone the repository
  git clone --depth=1 https://github.com/StellaSmith/little_game
  cd little_game/
  # Make sure we have conan installed
  sudo python3 -m pip install --update conan
  # Add the bincrafters conan repository for SDL2
  conan remote add bincrafters https://bincrafters.jfrog.io/artifactory/api/conan/conan
  # Install and build dependencies
  mkdir build
  pushd build
  conan install .. --build=missing
  popd
  # Activate virtual enviroment
  source build/activate.sh
  # Configure the project
  cmake -S ./ -B build/ -DCMAKE_BUILD_TYPE=Release
  # And finally, building
  # If this fails to link in Arch (/usr/bin/ld: cannot find -laudio), install aur/nas
  cmake --build build/ --target little_game --parallel
  # Install on default destination (/opt/little_game)
  sudo cmake --install build/
  # Or install under your user
  cmake --install build/ --prefix ~/.little_game
  # Deactivate the virtual enviroment
  source build/deactivate.sh
  # Now you can run the game by
  /opt/little_game/bin/little_game
  # Or
  ~/.little_game/bin/little_game
  ```