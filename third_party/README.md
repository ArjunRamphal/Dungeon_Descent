# Third-party components

## SFML 3.0.0

`SFML-3.0.0/` contains the SDK that was already bundled with Dungeon Descent. It was moved from the repository root without changing its files or version.

The bundle includes headers, import and static libraries, runtime DLLs, debug symbols, documentation and examples. Its runtime DLLs target x64. The application uses its graphics, window, system and audio modules.

- [SFML 3.0.0 download information](https://www.sfml-dev.org/download/sfml/3.0.0/)
- [SFML Visual Studio setup](https://www.sfml-dev.org/tutorials/3.0/getting-started/visual-studio/)
- [SFML licence information](https://www.sfml-dev.org/license/)

The SDK files retain their original copyright and licence notices, including those in the headers. The example asset notices remain in `SFML-3.0.0/examples/asset_licenses.md`.

The game's own images, fonts, music and story files are maintained separately under `assets/`.

## Standalone runtime

The standalone build compiles SFML from commit `7f1162dfea4969bc17417563ac55d93b72e84c1e` (3.0.0) with static libraries and the static MSVC runtime. SFML's build fetches FreeType 2.13.2, Ogg 1.3.5, Vorbis 1.3.7 and FLAC 1.4.3 from their pinned upstream releases. It also uses its bundled miniaudio, minimp3, stb image loaders and graphics headers, which retain their upstream notices in SFML's sources.

`notices/` contains the SFML, FreeType and Xiph dependency licence texts and the FreeType acknowledgement. These are packaged inside the executable and extracted to `licenses/` in the asset cache, so a standalone distribution retains those notices. The SDK's precompiled libraries remain available for the normal Debug/Release builds.
