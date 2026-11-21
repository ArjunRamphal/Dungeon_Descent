#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace dungeon {
class DungeonRun;
}

namespace dungeon::portable {

struct SuspendedRun {
    double playTimeSeconds = 0.0;
    std::uint32_t seed = 0;
    bool dailyRun = false;
    std::string dailyDate;
    std::string runState;
};

// Single-slot atomic suspend-run store with per-file locking.
// Lives in the game data directory (e.g., alongside leaderboards.tsv).
class SuspendSave {
public:
    explicit SuspendSave(std::filesystem::path file);

    const std::filesystem::path& file() const { return file_; }

    bool exists() const;
    void save(const DungeonRun& run, double playTimeSeconds, bool dailyRun = false, const std::string& dailyDate = {});
    std::optional<SuspendedRun> loadAndConsume(DungeonRun& run);
    std::optional<SuspendedRun> peek() const;
    void clear() const;

private:
    std::filesystem::path file_;
};

} // namespace dungeon::portable
