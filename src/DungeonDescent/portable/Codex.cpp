#include "Codex.h"
#include "FileLock.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace dungeon::portable {
namespace {
constexpr const char* Format = "DUNGEON_DESCENT_CODEX";

std::uint32_t endingBit(Ending ending) {
    switch (ending) {
    case Ending::Good: return 1u << 0;
    case Ending::True: return 1u << 1;
    case Ending::Bad: return 1u << 2;
    case Ending::Perfect: return 1u << 3;
    case Ending::None: return 0u;
    }
    return 0u;
}

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
        const auto path = parent / (".codex-pending-" + std::to_string(stamp) + "-" + std::to_string(serial++));
        if (std::filesystem::create_directory(path)) return path;
    }
    throw std::runtime_error("Cannot prepare the codex directory.");
}
} // namespace

bool CodexData::endingUnlocked(Ending ending) const {
    const auto bit = endingBit(ending);
    return bit != 0 && (unlockedEndingsMask & bit) != 0;
}

int CodexData::unlockedEndingCount() const {
    int count = 0;
    for (int i = 0; i < 4; ++i) {
        if ((unlockedEndingsMask & (1u << i)) != 0) ++count;
    }
    return count;
}

std::vector<MilestoneInfo> CodexData::milestones() const {
    const bool allFourHeroes = heroVictories[0] > 0 && heroVictories[1] > 0 &&
                               heroVictories[2] > 0 && heroVictories[3] > 0;
    return {
        {"first_victory", "Slayer of Malvel", "Defeat Malvel and reach any victorious ending.", totalVictories > 0},
        {"true_ending", "Justice for Falkreath", "Unlock the True Ending.", endingUnlocked(Ending::True)},
        {"crown_in_ashes", "A Crown in Ashes", "Earn the Perfect Ending (9/9 battles, 24/24 riddles, 9/9 rep).", endingUnlocked(Ending::Perfect)},
        {"all_endings", "Chronicler of Aethoria", "Unlock all four branching endings.", unlockedEndingCount() == 4},
        {"all_heroes", "Guild Champion", "Defeat Malvel with Warrior, Ranger, Mage and Rogue.", allFourHeroes},
        {"impossible_clear", "Defying the Impossible", "Defeat Malvel on Impossible difficulty.", impossibleVictories > 0},
        {"riddle_savant", "Riddle Savant", "Reach a riddle streak of 8+ correct answers in a row.", bestRiddleStreak >= 8},
        {"gauntlet_master", "Gauntlet Scholar", "Score 10+ correct answers in a Riddle Gauntlet.", bestGauntletScore >= 10}
    };
}

CodexStore::CodexStore(std::filesystem::path file) : file_(std::filesystem::absolute(std::move(file))) {}

CodexData CodexStore::readUnlocked() const {
    CodexData data;
    if (!std::filesystem::exists(file_)) return data;
    if (std::filesystem::file_size(file_) > 65536) return data;
    std::ifstream input(file_, std::ios::binary);
    if (!input) return data;
    std::string header, format;
    int version = 0;
    if (!std::getline(input, header)) return data;
    std::istringstream hs(header);
    if (!(hs >> format >> version) || format != Format || version != 1) return data;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line == "\r") continue;
        std::istringstream row(line);
        std::string key;
        if (!(row >> key)) continue;
        if (key == "endings_mask") row >> data.unlockedEndingsMask;
        else if (key == "total_runs") row >> data.totalRuns;
        else if (key == "total_victories") row >> data.totalVictories;
        else if (key == "total_defeats") row >> data.totalDefeats;
        else if (key == "impossible_victories") row >> data.impossibleVictories;
        else if (key == "hero_victories") row >> data.heroVictories[0] >> data.heroVictories[1] >> data.heroVictories[2] >> data.heroVictories[3];
        else if (key == "hero_best_score") row >> data.heroBestScore[0] >> data.heroBestScore[1] >> data.heroBestScore[2] >> data.heroBestScore[3];
        else if (key == "riddles_answered") row >> data.totalRiddlesAnswered;
        else if (key == "riddles_correct") row >> data.totalRiddlesCorrect;
        else if (key == "best_riddle_streak") row >> data.bestRiddleStreak;
        else if (key == "fastest_victory") row >> data.fastestVictorySeconds;
        else if (key == "best_gauntlet_score") row >> data.bestGauntletScore;
        else if (key == "best_gauntlet_streak") row >> data.bestGauntletStreak;
    }
    return data;
}

void CodexStore::writeUnlocked(const CodexData& data) const {
    PendingDir pending{pendingDirectory(file_.parent_path())};
    const auto temp = pending.directory / "codex.tsv";
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    output << Format << " 1\n"
           << "endings_mask " << data.unlockedEndingsMask << '\n'
           << "total_runs " << data.totalRuns << '\n'
           << "total_victories " << data.totalVictories << '\n'
           << "total_defeats " << data.totalDefeats << '\n'
           << "impossible_victories " << data.impossibleVictories << '\n'
           << "hero_victories " << data.heroVictories[0] << ' ' << data.heroVictories[1] << ' '
                                << data.heroVictories[2] << ' ' << data.heroVictories[3] << '\n'
           << "hero_best_score " << data.heroBestScore[0] << ' ' << data.heroBestScore[1] << ' '
                                 << data.heroBestScore[2] << ' ' << data.heroBestScore[3] << '\n'
           << "riddles_answered " << data.totalRiddlesAnswered << '\n'
           << "riddles_correct " << data.totalRiddlesCorrect << '\n'
           << "best_riddle_streak " << data.bestRiddleStreak << '\n'
           << "fastest_victory " << data.fastestVictorySeconds << '\n'
           << "best_gauntlet_score " << data.bestGauntletScore << '\n'
           << "best_gauntlet_streak " << data.bestGauntletStreak << '\n';
    output.close();
    if (!output) throw std::runtime_error("Cannot write codex file.");
#ifdef _WIN32
    if (!MoveFileExW(temp.c_str(), file_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace codex file.");
#else
    std::filesystem::rename(temp, file_);
#endif
}

CodexData CodexStore::load() const {
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockFile(file_));
    return readUnlocked();
}

CodexData CodexStore::recordCompletedRun(const DungeonRun& run, int playTimeSeconds) const {
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockFile(file_));
    CodexData data = readUnlocked();
    ++data.totalRuns;
    const auto& p = run.player();
    const auto heroIdx = static_cast<std::size_t>(p.hero);
    data.totalRiddlesAnswered += static_cast<std::uint32_t>(std::max(0, p.riddlesAnswered));
    data.totalRiddlesCorrect += static_cast<std::uint32_t>(std::max(0, p.riddlesCorrect));
    data.bestRiddleStreak = std::max(data.bestRiddleStreak, p.bestRiddleStreak);
    if (heroIdx < 4) {
        data.heroBestScore[heroIdx] = std::max(data.heroBestScore[heroIdx], run.score());
    }
    if (run.phase() == Phase::Ending && run.ending() != Ending::None) {
        data.unlockedEndingsMask |= endingBit(run.ending());
        if (run.ending() != Ending::Bad) {
            ++data.totalVictories;
            if (heroIdx < 4) ++data.heroVictories[heroIdx];
            if (run.difficulty() == Difficulty::Impossible) ++data.impossibleVictories;
            if (playTimeSeconds > 0 && (data.fastestVictorySeconds == 0 || playTimeSeconds < data.fastestVictorySeconds)) {
                data.fastestVictorySeconds = playTimeSeconds;
            }
        } else {
            ++data.totalDefeats;
        }
    } else if (run.phase() == Phase::Defeat) {
        ++data.totalDefeats;
    }
    writeUnlocked(data);
    return data;
}

CodexData CodexStore::recordGauntletResult(int score, int bestStreak) const {
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockFile(file_));
    CodexData data = readUnlocked();
    data.bestGauntletScore = std::max(data.bestGauntletScore, std::max(0, score));
    data.bestGauntletStreak = std::max(data.bestGauntletStreak, std::max(0, bestStreak));
    data.bestRiddleStreak = std::max(data.bestRiddleStreak, std::max(0, bestStreak));
    writeUnlocked(data);
    return data;
}

} // namespace dungeon::portable
