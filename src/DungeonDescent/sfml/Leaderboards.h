#pragma once
#include "DungeonRun.h"

namespace dungeon {
struct LeaderboardEntry {
    std::uint64_t id = 0;
    std::string initials;
    int score = 0;
    Hero hero = Hero::Warrior;
    Ending ending = Ending::None;
    std::uint32_t seed = 0;
    int playTimeSeconds = 0;
};

class Leaderboards {
public:
    static constexpr std::size_t PageSize = 10;
    static constexpr std::size_t Limit = PageSize;
    using Tables = std::array<std::vector<LeaderboardEntry>, 4>;
    struct Submission {
        Tables tables;
        std::uint64_t id;
        std::optional<std::size_t> rank; // One-based rank among all saved entries for this difficulty.
    };
    explicit Leaderboards(std::filesystem::path file);
    Tables load() const;
    Submission submit(Difficulty difficulty, std::string initials, int score, Hero hero, Ending ending,
                      std::uint32_t seed = 0, int playTimeSeconds = 0) const;
    static std::filesystem::path dataDirectory();
    static bool validInitials(const std::string& initials);
private:
    struct Data { Tables tables; std::uint64_t nextId = 1; };
    Data read() const; // Caller holds the per-file lock.
    void write(const Data& data) const;
    std::filesystem::path file_;
};
} // namespace dungeon
