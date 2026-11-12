# Single-executable builds

The **Standalone | x64** configuration produces `dist/Dungeon_Descent.exe`. Copy that file to another folder or another Windows x64 computer to play. SFML, its audio/font dependencies and the Visual C++ runtime are statically linked. The artwork, fonts, animation frames, music, story/riddle files and dependency notices are embedded in the executable.

The EXE still uses Windows' own system libraries and graphics/audio drivers. It does not require SFML DLLs or a Visual C++ Redistributable installation.

## Build in Visual Studio

1. Install Visual Studio 2022 with **Desktop development with C++**, MSVC v143, a Windows SDK and **C++ CMake tools for Windows**. A separate CMake 3.24+ installation on PATH also works.
2. Install Git for Windows with `git.exe` on PATH.
3. Open `Dungeon_Descent.sln` and select **Standalone | x64** in the toolbar.
4. Use **Build > Build Solution**, then **Ctrl+F5** to run.
5. Share `dist/Dungeon_Descent.exe`.

The first build downloads SFML 3.0.0 and its pinned dependencies and compiles them from source. This takes longer than the normal development build. Later builds reuse them. Before publishing to `dist`, the script checks that the executable can load its embedded assets from an isolated folder and repair a damaged cache. Build failures are shown in the Visual Studio Build output; fix the first reported error before launching.

The same build can be started from regular PowerShell in the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\Build-Standalone.ps1
.\dist\Dungeon_Descent.exe
```

Add `-Rebuild` to clean and rebuild the generated targets. `-Clean` cleans the standalone targets and removes the published EXE. Dependencies remain downloaded under `build/standalone` for reuse. Both `build` and `dist` are excluded from Git.

## Asset cache

The game streams music and loads its original assets from paths. To support those APIs, the standalone executable prepares its embedded files under:

```text
%LOCALAPPDATA%\DungeonDescent\assets\<asset-version>\
```

Nothing needs to be installed or kept beside the EXE. The current assets occupy approximately **280 MiB**, so the EXE is larger and the cache needs about that much additional disk space. First launch takes a little longer while it prepares the cache.

Local leaderboard scores are saved separately at `%LOCALAPPDATA%\DungeonDescent\leaderboards.tsv`. They survive asset-cache cleanup and replacement EXEs; the EXE still needs no companion files in its distribution folder. See [leaderboards](LEADERBOARDS.md) for an alternate data directory and backup details.

Each asset version has a content-derived directory. Files are checked on launch, reused when intact, and recreated if missing or damaged. Simultaneous launches coordinate through an operating-system lock. An interrupted extraction cannot be mistaken for a completed cache. Older versions keep separate directories; the cache can be deleted while the game is closed and will be recreated next time.

For an alternate writable cache location, set `DUNGEON_DESCENT_CACHE` before launching. This is also useful for verification:

```powershell
$env:DUNGEON_DESCENT_CACHE = "$env:TEMP\DungeonDescent-check"
$game = Start-Process .\dist\Dungeon_Descent.exe -ArgumentList '--verify-assets' -Wait -PassThru
$game.ExitCode  # 0 means the embedded files and riddle banks loaded successfully
```

`--assets <folder>` remains available as an explicit development override. Normal standalone launches always use the embedded assets, even if another asset folder is nearby.

## CMake

From an **x64 Native Tools Command Prompt for VS 2022**:

```bat
cmake -S . -B build/standalone-manual -G Ninja -DCMAKE_BUILD_TYPE=Release -DDD_STANDALONE=ON
cmake --build build/standalone-manual --target Dungeon_Descent --parallel
ctest --test-dir build/standalone-manual --output-on-failure
```

The manual build produces `build/standalone-manual/Dungeon_Descent.exe`. With `DD_STANDALONE=ON`, Windows CMake requires MSVC x64, fetches the pinned SFML source, turns off shared libraries and builds SFML and its dependencies with the same static runtime (`/MT`). It does not reuse the bundled SDK's precompiled libraries, which use a different runtime setting. A post-link import check rejects SFML, codec or Visual C++ runtime DLL dependencies.

Asset changes regenerate the embedded package automatically. The native C++ asset packer needs no Python installation. Original dependency notices are included under `licenses` in the embedded cache; their sources are listed in [third-party information](../third_party/README.md).

## Verification

The asset tests cover exact binary/text round trips, Unicode paths, deterministic packaging, cache reuse and repair, concurrent launches, corrupt archives, invalid paths and symlink handling. The standalone integration test copies only the executable into an isolated directory, verifies the embedded asset/riddle banks, damages the cache and verifies repair.

With `DD_TEST_GRAPHICS=ON`, an additional integration test launches that isolated executable with the real renderer and saves `standalone-check/Standalone_Startup.png`. Linux supports the same embedded-asset path for development checks; its system-library dependencies are separate from the Windows static runtime build.

Validation for this change: the GCC 14 / SFML 3.0.0 standalone build passed all seven tests on Linux with software OpenGL, including the full three-floor render/input suite and both isolated-executable checks. A second Linux build fetched and compiled the pinned font/audio dependencies from source with CMake 4.4, then passed the same asset-isolation and repair check. The Windows project XML, configuration/launch mapping and DLL-import rejection were checked separately. Windows compilation, runtime imports and audible playback still need verification on Windows.
