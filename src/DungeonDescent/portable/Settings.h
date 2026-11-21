#pragma once

#include <filesystem>

namespace dungeon::portable {

struct UserSettings {
    int musicVolume = 25;      // 0..100
    int sfxVolume = 80;        // 0..100
    int narrationVolume = 60;  // 0..100
    bool muted = false;
    bool fullscreen = true;
};

// Atomic, file-locked persistent user settings store (settings.cfg in data directory).
class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path file);

    const std::filesystem::path& file() const { return file_; }

    UserSettings load() const;
    void save(const UserSettings& settings) const;

private:
    std::filesystem::path file_;
};

} // namespace dungeon::portable
