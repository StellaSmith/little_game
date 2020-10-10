# little_game
A little project aimed to create a minecraft-like game.

## Building
### Windows
- Make sure you have [CMake](https://cmake.org/download/) and [Visual Studio](https://visualstudio.microsoft.com/) installed with the C++ desktop development tools.
- Clone or download the repository.
- Open the command line in the root folder of the source (where the CMakeLists.txt file is located).
- Configure the dependencies and the project: `cmake -S ./ -B build/`
- Build the dependencies and the project: `cmake --build build --target little_game --config Release --parallel`
- Alternatively you can open the project on Visual Studio and compile the game from it.<br/>
### Linux
Make sure you have `git`, `gcc`, `make` and `cmake` installed. Also make sure you have a graphical enviroment installed.<br/>

|    System    |                      Command                     |
|--------------|--------------------------------------------------|
| Arch Based   | `sudo pacman -S --needed base-devel cmake git` |
| Debian Based | `sudo apt install build-essential cmake git`              |

- Clone the repository: `git clone --depth=1 https://github.com/StellaSmith/little_game`
- Change your directory to the repository: `cd little_game/`
- Configure the dependencies and the project: `cmake -S ./ -B build/ -DCMAKE_BUILD_TYPE=Release`
- Build the dependencies and the project: `cmake --build build/ --target little_game --parallel`

## Installing and Running
- First, make sure you already compiled everything.
- Then, as administraror or root run the following:
  - On Windows: `cmake --install build/ --config Release`
  - On Linux: `cmake --install build/`
- Finally, you can run it using:
  - On Windows: `C:/Program Files/little_game/run.bat`
  - On Linux: `/opt/little_game/run.sh`

If you want to chance the install location, in the configure step, add `-DCMAKE_INSTALL_PREFIX=path/to/installation`.
For example, on Linux, use `cmake -S ./ -B build/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.little_game` to install it under your home directory.