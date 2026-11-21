#include "Codex.h"
#include "DungeonRun.h"
#include "Settings.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace dungeon;
using namespace dungeon::portable;

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct TempDir {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("DungeonDescent-codex-settings-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }
};

void completeRun(DungeonRun& run, Hero hero, Difficulty diff, bool perfectLike) {
    run.start(hero, Biome::Ice, 12, diff);
    for (int step = 0; step < 1500 && run.inProgress(); ++step) {
        if (run.phase() == Phase::Complete) {
            run.descend(run.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
        } else if (run.phase() == Phase::Room) {
            std::size_t choice = 0;
            if (run.room() == Room::Library) choice = hero == Hero::Mage ? 2 : hero == Hero::Rogue ? 1 : 0;
            if (run.room() == Room::Respite) choice = perfectLike && run.floorNumber() == 1 ? 0 : 1;
            if (run.room() == Room::Shop) choice = run.floorNumber() == 3 ? 2 : 1;
            run.choose(choice);
        } else if (run.phase() == Phase::Combat) {
            if (run.classAbilityAvailable()) run.attack(CombatAction::ClassAbility);
            else if (run.bossIntent().telegraphed && run.bossIntent().counterMultiplier > 1.f) run.attack(CombatAction::GuardStrike);
            else run.attack();
        } else if (run.phase() == Phase::Riddle) {
            run.answer(run.riddle().correct);
        } else {
            run.advance();
        }
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Pass the assets directory.");
        const std::filesystem::path assets = argv[1];
        TempDir temp;

        // 1. SettingsStore defaults, save/load round-trip, clamping, and concurrency.
        {
            const auto file = temp.root / "settings.cfg";
            SettingsStore store(file);
            const auto defaults = store.load();
            check(defaults.musicVolume == 25 && defaults.sfxVolume == 80 && defaults.narrationVolume == 60 &&
                  !defaults.muted && defaults.fullscreen, "Fresh SettingsStore returns expected defaults");

            UserSettings custom;
            custom.musicVolume = 150;
            custom.sfxVolume = -20;
            custom.narrationVolume = 45;
            custom.muted = true;
            custom.fullscreen = false;
            store.save(custom);

            const auto reloaded = SettingsStore(file).load();
            check(reloaded.musicVolume == 100 && reloaded.sfxVolume == 0 && reloaded.narrationVolume == 45 &&
                  reloaded.muted && !reloaded.fullscreen, "SettingsStore clamps volumes to 0..100 and persists flags");

            std::vector<std::future<void>> tasks;
            for (int i = 0; i < 8; ++i) {
                tasks.push_back(std::async(std::launch::async, [&, i] {
                    UserSettings s;
                    s.musicVolume = i * 10;
                    s.sfxVolume = 50 + i;
                    s.narrationVolume = 60;
                    s.muted = (i % 2) == 0;
                    s.fullscreen = true;
                    SettingsStore(file).save(s);
                }));
            }
            for (auto& t : tasks) t.get();
            const auto afterConcurrent = store.load();
            check(afterConcurrent.musicVolume >= 0 && afterConcurrent.musicVolume <= 100 &&
                  afterConcurrent.narrationVolume == 60, "Concurrent settings saves remain valid");

            // Corrupt file falls back cleanly to defaults without crashing.
            std::ofstream corrupt(file, std::ios::binary | std::ios::trunc);
            corrupt << "GARBAGE\nnot valid\n";
            corrupt.close();
            const auto recovered = store.load();
            check(recovered.musicVolume == 25 && recovered.sfxVolume == 80, "Corrupt settings file falls back to defaults");
        }

        // 2. CodexStore persistence, ending unlocks, milestones, and Gauntlet records.
        {
            const auto file = temp.root / "codex.tsv";
            CodexStore store(file);
            const auto empty = store.load();
            check(empty.totalRuns == 0 && empty.unlockedEndingCount() == 0 && empty.milestones().size() == 8,
                  "Fresh Codex starts empty with 8 defined milestones");
            for (const auto& m : empty.milestones()) {
                check(!m.unlocked, "No milestone unlocked initially: " + m.id);
            }

            DungeonRun run(assets);
            completeRun(run, Hero::Mage, Difficulty::Easy, false);
            check(run.phase() == Phase::Ending && run.ending() != Ending::Bad, "Simulated run reaches a victory ending");
            const auto afterFirst = store.recordCompletedRun(run, 185);
            check(afterFirst.totalRuns == 1 && afterFirst.totalVictories == 1 && afterFirst.totalDefeats == 0 &&
                  afterFirst.endingUnlocked(run.ending()) && afterFirst.unlockedEndingCount() == 1 &&
                  afterFirst.heroVictories[static_cast<std::size_t>(Hero::Mage)] == 1 &&
                  afterFirst.heroBestScore[static_cast<std::size_t>(Hero::Mage)] == run.score() &&
                  afterFirst.fastestVictorySeconds == 185 &&
                  afterFirst.bestRiddleStreak >= 5,
                  "Codex records victory stats, ending unlock, hero score, fastest clear, and riddle streak");

            const auto milestones = afterFirst.milestones();
            auto hasMilestone = [&](const std::string& id) {
                for (const auto& m : milestones) if (m.id == id) return m.unlocked;
                return false;
            };
            check(hasMilestone("first_victory"), "First Victory milestone unlocked");
            check(hasMilestone("riddle_savant"), "Riddle Savant (8+ streak) milestone unlocked");
            check(!hasMilestone("impossible_clear"), "Impossible milestone remains locked");

            // Record Gauntlet results and verify max retention.
            const auto afterG1 = store.recordGauntletResult(14, 9);
            const auto afterG2 = store.recordGauntletResult(8, 12);
            check(afterG2.bestGauntletScore == 14 && afterG2.bestGauntletStreak == 12,
                  "Gauntlet records keep highest score and best streak independently");
            bool gauntletMilestone = false;
            for (const auto& m : afterG2.milestones()) if (m.id == "gauntlet_master") gauntletMilestone = m.unlocked;
            check(gauntletMilestone, "Gauntlet Scholar (10+ Gauntlet score) milestone unlocked");
            check(afterG1.bestGauntletScore == 14, "First Gauntlet record persisted");

            // Reload from a fresh store instance.
            const auto reloaded = CodexStore(file).load();
            check(reloaded.totalRuns == 1 && reloaded.totalVictories == 1 &&
                  reloaded.bestGauntletScore == 14 && reloaded.bestGauntletStreak == 12 &&
                  reloaded.fastestVictorySeconds == 185 && reloaded.endingUnlocked(run.ending()),
                  "Codex data survives fresh store reload");
        }

        // Verify staging directories are cleaned up.
        for (const auto& entry : std::filesystem::recursive_directory_iterator(temp.root)) {
            const auto name = entry.path().filename().generic_u8string();
            check(name.find(".settings-pending-") != 0 && name.find(".codex-pending-") != 0,
                  "Settings and Codex staging directories are cleaned up");
        }

        std::cout << "SettingsStore and CodexStore persistence, clamping, milestones, ending unlocks, Gauntlet records, and concurrency passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
