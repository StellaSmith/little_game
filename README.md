# little_game
A little project aimed to create a minecraft-like game.

## Building
### Windows
- Make sure you have [CMake](https://cmake.org/download/), [Python 3](https://www.python.org/downloads/windows/) and [Visual Studio](https://visualstudio.microsoft.com/) installed with the C++ desktop development tools installed.
- Clone or download the source.
- Open the command line in the root folder of the source (where the CMakeLists.txt file is located).
- Type `cmake -S . -B build`, this will download the dependencies and create the visual studio solution.
- Type `cmake --build build --target little_game --config Release --parallel`, this will start compiling the dependencies and the game.
- Finally run it typing `.\build\Release\little_game`.

Alternatively you can open the project on Visual Studio, compile and run the game from it.<br/>
### Linux
Make sure you have gcc, make, cmake and python installed. Also make sure you have a graphical enviroment.<br/>

|    System    |                      Command                     |
|--------------|--------------------------------------------------|
| Arch Based   | `pacman -S --needed base-devel cmake git python` |
| Debian Based | `apt install build-essential cmake`              |

Download the source using git or get the [zip file](https://github.com/StellaSmith/little_game/archive/master.zip)
```bash
git clone --depth=1 https://github.com/StellaSmith/little_game
```
Then configure the project, this will also download some required libraries like SDL2 and GLM
```bash
cd little_game
cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
```
Finally, compile and run the program using
```bash
cmake --build build/ --target little_game --parallel
./build/little_game
```
