#include "AssetBundle.h"
#include <cstdlib>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#else
extern "C" const unsigned char dd_assets_begin[], dd_assets_end[];
#endif

namespace dungeon::portable {
Bytes embeddedAssets() {
#ifdef _WIN32
    const auto resource = FindResourceW(nullptr, MAKEINTRESOURCEW(101), MAKEINTRESOURCEW(10));
    const auto size = resource ? SizeofResource(nullptr, resource) : 0;
    const auto loaded = resource ? LoadResource(nullptr, resource) : nullptr;
    const auto data = loaded ? static_cast<const unsigned char*>(LockResource(loaded)) : nullptr;
    if (!data || !size) throw std::runtime_error("Embedded game assets are missing from this executable.");
    return {data, size};
#else
    const auto size = reinterpret_cast<std::uintptr_t>(dd_assets_end) - reinterpret_cast<std::uintptr_t>(dd_assets_begin);
    return {dd_assets_begin, size};
#endif
}

std::filesystem::path defaultCacheDirectory() {
#ifdef _WIN32
    std::wstring overridePath(32768, L'\0');
    const auto length = GetEnvironmentVariableW(L"DUNGEON_DESCENT_CACHE", overridePath.data(), static_cast<DWORD>(overridePath.size()));
    if (length && length < overridePath.size()) { overridePath.resize(length); return overridePath; }
    PWSTR folder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &folder))) {
        const auto result = std::filesystem::path(folder) / "DungeonDescent" / "assets";
        CoTaskMemFree(folder);
        return result;
    }
#else
    if (const auto path = std::getenv("DUNGEON_DESCENT_CACHE"); path && *path) return path;
    if (const auto path = std::getenv("XDG_CACHE_HOME"); path && *path) return std::filesystem::path(path) / "DungeonDescent" / "assets";
    if (const auto path = std::getenv("HOME"); path && *path) return std::filesystem::path(path) / ".cache" / "DungeonDescent" / "assets";
#endif
    return std::filesystem::temp_directory_path() / "DungeonDescent" / "assets";
}
} // namespace dungeon::portable
