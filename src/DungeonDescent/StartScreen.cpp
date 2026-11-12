#include "sfml/GameApp.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef DD_EMBED_ASSETS
#include "portable/AssetBundle.h"
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#ifndef DD_EMBED_ASSETS
namespace {
std::filesystem::path executableDirectory(const char* argv0) {
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length > 0 && length < buffer.size()) { buffer.resize(length); return std::filesystem::path(buffer).parent_path(); }
#elif defined(__linux__)
    std::error_code error;
    const auto executable = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) return executable.parent_path();
#endif
    return std::filesystem::absolute(argv0).parent_path();
}
std::filesystem::path findAssets(const std::filesystem::path& executable) {
    std::vector<std::filesystem::path> candidates{executable / "assets", executable, std::filesystem::current_path() / "assets"};
    auto parent = executable;
    for (int i = 0; i < 5; ++i) { candidates.push_back(parent / "assets"); parent = parent.parent_path(); }
    for (const auto& path : candidates)
        if (std::filesystem::exists(path / "fonts/Arial.ttf") && std::filesystem::exists(path / "textfiles/riddles/1/EasyRiddles.txt"))
            return std::filesystem::canonical(path);
    throw std::runtime_error("Game assets not found. Keep the assets folder beside the executable, or pass --assets <folder>.");
}
}
#endif
int main(int argc, char** argv) {
    [[maybe_unused]] bool quietErrors = false;
    try {
        dungeon::LaunchOptions options;
        bool verifyAssets = false;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--assets" && i + 1 < argc) options.assets = std::filesystem::absolute(argv[++i]);
            else if (arg == "--data-dir" && i + 1 < argc) options.dataDirectory = std::filesystem::absolute(argv[++i]);
            else if (arg == "--no-audio") options.audio = false;
            else if (arg == "--skip-intro") options.skipIntro = true;
            else if (arg == "--verify-assets") { verifyAssets = true; quietErrors = true; }
            else if (arg == "--smoke-test") { options.smoke = true; options.audio = false; quietErrors = true; }
            else if (arg == "--screenshot" && i + 1 < argc) options.screenshot = std::filesystem::absolute(argv[++i]);
            else if (arg == "--help") {
                std::cout << "Dungeon Descent: --assets <folder> --data-dir <folder> --no-audio --skip-intro\n"
                             "Verification: --verify-assets --smoke-test [--screenshot <png>]\n";
                return 0;
            } else throw std::runtime_error("Unknown or incomplete option: " + arg);
        }
        if (options.assets.empty()) {
#ifdef DD_EMBED_ASSETS
            dungeon::portable::AssetBundle bundle(dungeon::portable::embeddedAssets());
            options.assets = bundle.materialize(dungeon::portable::defaultCacheDirectory());
#else
            options.assets = findAssets(executableDirectory(argv[0]));
#endif
        }
        if (verifyAssets) {
            dungeon::DungeonRun rules(options.assets);
            if (!std::filesystem::is_regular_file(options.assets / "fonts/Arial.ttf")) throw std::runtime_error("Game fonts are missing.");
            std::cout << "Game assets verified: " << options.assets.string() << '\n';
            return 0;
        }
        return dungeon::GameApp(std::move(options)).run();
    } catch (const std::exception& error) {
        std::cerr << "Dungeon Descent: " << error.what() << '\n';
        std::ofstream("DungeonDescent-error.log") << error.what() << '\n';
#ifdef _WIN32
        if (!quietErrors) MessageBoxA(nullptr, error.what(), "Dungeon Descent / startup error", MB_OK | MB_ICONERROR);
#endif
        return 1;
    }
}
