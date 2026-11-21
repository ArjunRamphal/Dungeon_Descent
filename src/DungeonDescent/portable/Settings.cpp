#include "Settings.h"
#include "FileLock.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace dungeon::portable {
namespace {
constexpr const char* Format = "DUNGEON_DESCENT_SETTINGS";

std::filesystem::path lockFile(std::filesystem::path file) {
    file += ".lock";
    return file;
}

struct PendingDir {
    std::filesystem::path directory;
    ~PendingDir() {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
};

std::filesystem::path pendingDirectory(const std::filesystem::path& parent) {
    static std::atomic<unsigned> serial{0};
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        const auto path = parent / (".settings-pending-" + std::to_string(stamp) + "-" + std::to_string(serial++));
        if (std::filesystem::create_directory(path)) return path;
    }
    throw std::runtime_error("Cannot prepare the settings directory.");
}

int clampVolume(int v) {
    return std::clamp(v, 0, 100);
}
} // namespace

SettingsStore::SettingsStore(std::filesystem::path file) : file_(std::filesystem::absolute(std::move(file))) {}

UserSettings SettingsStore::load() const {
    UserSettings out;
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockFile(file_));
    if (!std::filesystem::exists(file_)) return out;
    if (std::filesystem::file_size(file_) > 16384) return out;
    std::ifstream input(file_, std::ios::binary);
    if (!input) return out;
    std::string header, format;
    int version = 0;
    if (!std::getline(input, header)) return out;
    std::istringstream hs(header);
    if (!(hs >> format >> version) || format != Format || version != 1) return out;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line == "\r") continue;
        std::istringstream row(line);
        std::string key;
        int value = 0;
        if (!(row >> key >> value)) continue;
        if (key == "music_volume") out.musicVolume = clampVolume(value);
        else if (key == "sfx_volume") out.sfxVolume = clampVolume(value);
        else if (key == "narration_volume") out.narrationVolume = clampVolume(value);
        else if (key == "muted") out.muted = (value != 0);
        else if (key == "fullscreen") out.fullscreen = (value != 0);
    }
    return out;
}

void SettingsStore::save(const UserSettings& settings) const {
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockFile(file_));
    PendingDir pending{pendingDirectory(file_.parent_path())};
    const auto temp = pending.directory / "settings.cfg";
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    output << Format << " 1\n"
           << "music_volume " << clampVolume(settings.musicVolume) << '\n'
           << "sfx_volume " << clampVolume(settings.sfxVolume) << '\n'
           << "narration_volume " << clampVolume(settings.narrationVolume) << '\n'
           << "muted " << (settings.muted ? 1 : 0) << '\n'
           << "fullscreen " << (settings.fullscreen ? 1 : 0) << '\n';
    output.close();
    if (!output) throw std::runtime_error("Cannot write settings file.");
#ifdef _WIN32
    if (!MoveFileExW(temp.c_str(), file_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace settings file.");
#else
    std::filesystem::rename(temp, file_);
#endif
}

} // namespace dungeon::portable
