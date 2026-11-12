# Development setup

## Windows / existing Visual Studio solution

Install Visual Studio 2022 with **Desktop development with C++**, MSVC v143 and a Windows SDK. Open `Dungeon_Descent.sln`, select **x64**, build and run. The bundled SFML 3.0.0 SDK supplies the headers, import libraries and runtime DLLs. Its x86 configuration is unsupported.

The active project is now native C++ and compiles only `StartScreen.cpp`, `sfml/GameApp.cpp` and `sfml/FloorOne.cpp`. C++/CLI, .NET and Windows Forms are not required. The original forms and classes remain in source as references for porting subsequent floors.

From Developer PowerShell:

```powershell
msbuild Dungeon_Descent.sln /m /p:Configuration=Release /p:Platform=x64
.\build\x64\Release\Dungeon_Descent.exe
```

| Configuration | Output |
| --- | --- |
| Debug x64 | `build/x64/Debug/Dungeon_Descent.exe` |
| Release x64 | `build/x64/Release/Dungeon_Descent.exe` |

MSBuild copies the original runtime folders (`fonts`, `images`, `music`, `textfiles`, `candle`) and SFML DLLs beside the executable. Keep these together when sharing the game. Rebuild after asset changes.

## CMake / Windows

CMake 3.24 or later can use the same bundled MSVC x64 SDK:

```powershell
cmake -S . -B build/cmake -A x64
cmake --build build/cmake --config Release
ctest --test-dir build/cmake -C Release --output-on-failure
.\build\cmake\Release\Dungeon_Descent.exe
```

CMake copies assets into an `assets` folder beside the executable and copies the required runtime DLLs. Use `-DDD_FETCH_SFML=ON` to build the pinned SFML 3.0.0 source instead, for example with another supported compiler. This needs Git and network access for SFML's dependencies.

## CMake / Linux

The bundled DLLs and `.lib` files are Windows binaries. On Debian/Ubuntu, install build tools and SFML's development dependencies:

```sh
sudo apt install build-essential cmake ninja-build git \
  libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
  libfreetype-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev
cmake -S . -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DDD_FETCH_SFML=ON
cmake --build build/linux
ctest --test-dir build/linux --output-on-failure
./build/linux/Dungeon_Descent
```

Alternatively, use an installed SFML 3.x package and pass `-DSFML_DIR=/path/to/lib/cmake/SFML`. The game targets the 3.0.0 API. See SFML's [CMake guide](https://www.sfml-dev.org/tutorials/3.0/getting-started/cmake/) for platform prerequisites.

`DD_COPY_ASSETS=OFF` avoids copying assets during local development. Asset discovery can find the repository's `assets` directory when the executable is built below the repository root.

## Assets and launch options

No working-directory change is required. Startup searches for an `assets` folder beside the executable, the MSBuild runtime layout, and nearby repository asset folders. Paths use `std::filesystem`, including Windows Unicode executable paths.

```sh
Dungeon_Descent --assets /path/to/assets
Dungeon_Descent --skip-intro
Dungeon_Descent --no-audio
Dungeon_Descent --smoke-test --screenshot FloorOne.png
```

`--smoke-test` opens a first-floor scene without audio, renders five frames, optionally saves its canvas and exits. It requires a graphical display. Missing required images, fonts, story text or riddle data produces a startup/error message and `DungeonDescent-error.log`; optional music failures do not end gameplay.

## Tests

Rules tests need only a C++17 compiler and CMake, without SFML or a display:

```sh
cmake -S . -B build/rules -DDD_BUILD_GAME=OFF
cmake --build build/rules
ctest --test-dir build/rules --output-on-failure
```

They exercise 800 successful seeded routes across all four heroes and both biomes, every opening room permutation, assets, eight unique riddles per floor, chest gating, single rewards, timeout/incorrect penalties, defeated states and room retries. This checks a successful strategy; it does not promise victory for every choice or random strike roll.

Enable the optional render/input checks with `-DDD_TEST_GRAPHICS=ON` in a game build. Run CTest with a graphical display, or `xvfb-run -a ctest --test-dir build/linux --output-on-failure` on headless Linux. They traverse both biome routes using the real SFML renderer and event handler, check scaled mouse coordinates, keyboard choices, overlays and paused timers, and save screenshots under `ui-captures` in the build directory.

Validation during this implementation: GCC 14 / SFML 3.0.0 build, all rules tests and the SFML render/input tests passed on Linux with software OpenGL. The Windows MSVC project paths and XML were checked; Windows compilation and audible playback still require verification on Windows.

## Troubleshooting

| Problem | Check |
| --- | --- |
| Build asks for x64 | Select the x64 platform for the bundled SDK. |
| A header, import library or DLL is missing | Restore `third_party/SFML-3.0.0` and rebuild. |
| Assets cannot be located | Keep the copied runtime folders together or supply `--assets`. |
| Asset edits do not appear | Rebuild to rerun the copy step. |
| No music | Press `V`, check system volume, and check the original `music` folder. Use `--no-audio` for environments without an audio device. |
| A riddle appears frozen | Close the map/lore/pause overlay and focus the game window. |
| Display cannot be opened on Linux | Run in a desktop session or use Xvfb for verification. |
| Error during launch | Read the message and `DungeonDescent-error.log`. |

Build output, logs and personal Visual Studio settings should stay out of Git. The bundled SDK and historical assets retain their notices.
