#pragma once

#include "../sfml/DungeonRun.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dungeon::portable {

struct MilestoneInfo {
    std::string id;
    std::string title;
    std::string description;
    bool unlocked = false;
};

struct CodexData {
    std::uint32_t unlockedEndingsMask = 0; // Bit 0=Good, 1=True, 2=Bad, 3=Perfect
    std::uint32_t totalRuns = 0;
    std::uint32_t totalVictories = 0;
    std::uint32_t totalDefeats = 0;
    std::uint32_t dungeonDeaths = 0;
    std::uint32_t impossibleVictories = 0;
    bool evilRouteCompleted = false;
    std::uint32_t noAbilityDifficultyMask = 0; // Bit 0=Easy, 1=Medium, 2=Hard, 3=Impossible
    std::array<std::uint32_t, 4> heroVictories{};
    std::array<int, 4> heroBestScore{};
    std::uint32_t totalRiddlesAnswered = 0;
    std::uint32_t totalRiddlesCorrect = 0;
    int bestRiddleStreak = 0;
    int fastestVictorySeconds = 0; // 0 = none recorded yet
    int bestGauntletScore = 0;
    int bestGauntletStreak = 0;

    bool endingUnlocked(Ending ending) const;
    int unlockedEndingCount() const;
    std::vector<MilestoneInfo> milestones() const;
};

// Atomic, file-locked persistent Codex / Career Stats & Ending Gallery store.
class CodexStore {
public:
    explicit CodexStore(std::filesystem::path file);

    const std::filesystem::path& file() const { return file_; }

    CodexData load() const;
    CodexData recordCompletedRun(const DungeonRun& run, int playTimeSeconds) const;
    CodexData recordGauntletResult(int score, int bestStreak) const;

private:
    CodexData readUnlocked() const;
    void writeUnlocked(const CodexData& data) const;
    std::filesystem::path file_;
};

} // namespace dungeon::portable
