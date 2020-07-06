# little_game
A little project aimed to create a minecraft-like game.

## Building
### Windows
Make sure you have CMake and Visual Studio installed with the C++ desktop development tools installed.<br/>
Clone or download the source.<br/>
Open the command line in the root folder of the source.<br/>
Type `cmake -S . -B build`, this will download the dependencies and create the visual studio solution.<br/>
Type `cmake --build build --target little_game --config Release --parallel`, this will start compiling the dependencies and the game.<br/>
Finally run it typing `.\build\Release\little_game`.<br/>

Alternatively you can open the project on Visual Studio, compile and run the game from it.<br/>
### Linux
Make sure you have gcc, make and cmake installed.<br/>

|    System    |               Command               |
|--------------|-------------------------------------|
| Arch Based   | `pacman -S base-devel cmake`        |
| Debian Based | `apt install build-essential cmake` |

```bash
git clone --depth=1 https://github.com/StellaSmith/little_game
cd little_game
# Configure the project, downloads the dependencies and generates the makefiles
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release      
cmake --build build --target little_game --parallel # 
./build/little_game
```
