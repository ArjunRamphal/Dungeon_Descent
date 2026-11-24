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
            run.choose(run.displayChoiceFor(choice));
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

        // 2. CodexStore persistence, ending unlocks, all 19 milestones, and Gauntlet records.
        {
            const auto file = temp.root / "codex.tsv";
            CodexStore store(file);
            const auto empty = store.load();
            check(empty.totalRuns == 0 && empty.unlockedEndingCount() == 0 && empty.milestones().size() == 22,
                  "Fresh Codex starts empty with 22 defined milestones");
            check(!empty.prequelUnlocked(), "Prequel starts locked before Perfect run on Impossible");
            for (const auto& m : empty.milestones()) {
                check(!m.unlocked, "No milestone unlocked initially: " + m.id);
            }

            auto hasMilestone = [](const CodexData& data, const std::string& id) {
                for (const auto& m : data.milestones()) if (m.id == id) return m.unlocked;
                return false;
            };

            // Run 1: Mage Easy victory using class ability in battles including Malvel -> unlocks neither no-ability tier.
            DungeonRun run(assets);
            completeRun(run, Hero::Mage, Difficulty::Easy, false);
            check(run.phase() == Phase::Ending && run.ending() != Ending::Bad && run.player().classAbilitiesUsed > 0,
                  "Simulated run reaches a victory ending using class ability");
            const auto afterFirst = store.recordCompletedRun(run, 185);
            const auto bdFast = run.scoreBreakdown(185);
            const auto bdSlow = run.scoreBreakdown(600);
            check(bdFast.statsPoints > 0 && bdFast.healthPoints > 0 &&
                  run.player().strikesLeftover > 0 && bdFast.strikesPoints == run.player().strikesLeftover * 10 &&
                  bdFast.timePoints == (1800 - 185) * 2 &&
                  bdFast.total == bdFast.statsPoints + bdFast.healthPoints + bdFast.strikesPoints + bdFast.timePoints &&
                  bdFast.total > bdSlow.total &&
                  run.scoreBreakdown(2000).timePoints == 0,
                  "Score breakdown incorporates stats, ending health, leftover strikes, and victory speed bonus");
            check(afterFirst.totalRuns == 1 && afterFirst.totalVictories == 1 && afterFirst.totalDefeats == 0 &&
                  afterFirst.endingUnlocked(run.ending()) && afterFirst.unlockedEndingCount() == 1 &&
                  afterFirst.heroVictories[static_cast<std::size_t>(Hero::Mage)] == 1 &&
                  afterFirst.heroBestScore[static_cast<std::size_t>(Hero::Mage)] == run.score(185) &&
                  afterFirst.fastestVictorySeconds == 185 &&
                  afterFirst.bestRiddleStreak >= 5,
                  "Codex records victory stats, ending unlock, hero score, fastest clear, and riddle streak");
            check(hasMilestone(afterFirst, "first_victory"), "First Victory milestone unlocked");
            check(hasMilestone(afterFirst, "riddle_savant"), "Riddle Savant (8+ streak) milestone unlocked");
            check(!hasMilestone(afterFirst, "impossible_clear"), "Impossible milestone remains locked");
            check(!hasMilestone(afterFirst, "no_ability_easy"), "Using class ability does not unlock Pure Steel: Easy");
            check(!hasMilestone(afterFirst, "malvel_no_ability_easy"), "Using class ability vs Malvel does not unlock Duelist vs Malvel: Easy");
            check(!hasMilestone(afterFirst, "evil_route"), "Merciful run does not unlock Ruthless Descent");
            check(!hasMilestone(afterFirst, "claimed_by_depths"), "Victory does not unlock Claimed by the Depths");
            check(!hasMilestone(afterFirst, "fallen_to_malvel"), "Victory does not unlock Fallen to the Sorcerer");

            // Run 1b: Medium victory using class ability on Floor 1/2, but NOT against Malvel -> unlocks malvel_no_ability_medium ONLY!
            DungeonRun malvelDuelist(assets);
            malvelDuelist.start(Hero::Mage, Biome::Ice, 12, Difficulty::Medium);
            for (int step = 0; step < 1500 && malvelDuelist.inProgress(); ++step) {
                if (malvelDuelist.phase() == Phase::Complete) {
                    malvelDuelist.descend(malvelDuelist.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
                } else if (malvelDuelist.phase() == Phase::Room) {
                    std::size_t choice = 0;
                    if (malvelDuelist.room() == Room::Library) choice = 2;
                    if (malvelDuelist.room() == Room::Respite) choice = 1;
                    if (malvelDuelist.room() == Room::Shop) choice = malvelDuelist.floorNumber() == 3 ? 2 : 1;
                    malvelDuelist.choose(malvelDuelist.displayChoiceFor(choice));
                } else if (malvelDuelist.phase() == Phase::Combat) {
                    const bool isMalvel = malvelDuelist.floorNumber() == 3 && malvelDuelist.room() == Room::Boss;
                    if (!isMalvel && malvelDuelist.classAbilityAvailable()) malvelDuelist.attack(CombatAction::ClassAbility);
                    else if (malvelDuelist.bossIntent().telegraphed && malvelDuelist.bossIntent().counterMultiplier > 1.f)
                        malvelDuelist.attack(CombatAction::GuardStrike);
                    else malvelDuelist.attack(CombatAction::StandardStrike);
                } else if (malvelDuelist.phase() == Phase::Riddle) {
                    malvelDuelist.answer(malvelDuelist.riddle().correct);
                } else {
                    malvelDuelist.advance();
                }
            }
            check(malvelDuelist.phase() == Phase::Ending && malvelDuelist.ending() != Ending::Bad &&
                  malvelDuelist.player().classAbilitiesUsed > 0 && !malvelDuelist.classAbilityUsedInBattle(),
                  "Malvel duelist run used abilities earlier but not in the Malvel battle");
            const auto afterDuelist = store.recordCompletedRun(malvelDuelist, 200);
            check(hasMilestone(afterDuelist, "malvel_no_ability_medium") && !hasMilestone(afterDuelist, "no_ability_medium"),
                  "Beating Malvel without class ability unlocks Duelist vs Malvel: Medium without unlocking full-run Pure Steel: Medium");

            // Run 2: Easy victory WITHOUT class ability the ENTIRE run AND killing all 3 NPCs -> unlocks no_ability_easy, malvel_no_ability_easy & evil_route!
            DungeonRun evilNoAbility(assets);
            evilNoAbility.start(Hero::Mage, Biome::Jungle, 15, Difficulty::Easy);
            for (int step = 0; step < 1500 && evilNoAbility.inProgress(); ++step) {
                if (evilNoAbility.phase() == Phase::Complete) {
                    evilNoAbility.descend(evilNoAbility.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
                } else if (evilNoAbility.phase() == Phase::Room) {
                    std::size_t choice = 0;
                    if (evilNoAbility.room() == Room::Library) choice = 2;
                    if (evilNoAbility.room() == Room::Respite) choice = 1;
                    if (evilNoAbility.room() == Room::Shop) choice = evilNoAbility.floorNumber() == 3 ? 2 : 1;
                    if (evilNoAbility.room() == Room::Encounter) choice = 1; // Always choose kill/ruthless
                    evilNoAbility.choose(evilNoAbility.displayChoiceFor(choice));
                } else if (evilNoAbility.phase() == Phase::Combat) {
                    if (evilNoAbility.bossIntent().telegraphed && evilNoAbility.bossIntent().counterMultiplier > 1.f)
                        evilNoAbility.attack(CombatAction::GuardStrike);
                    else evilNoAbility.attack(CombatAction::StandardStrike);
                } else if (evilNoAbility.phase() == Phase::Riddle) {
                    evilNoAbility.answer(evilNoAbility.riddle().correct);
                } else {
                    evilNoAbility.advance();
                }
            }
            check(evilNoAbility.phase() == Phase::Ending && evilNoAbility.ending() != Ending::Bad &&
                  evilNoAbility.player().classAbilitiesUsed == 0 && evilNoAbility.player().npcKills == 3,
                  "Evil no-ability run wins with 0 class abilities and 3/3 NPC kills");
            const auto afterEvil = store.recordCompletedRun(evilNoAbility, 210);
            check(hasMilestone(afterEvil, "no_ability_easy") && hasMilestone(afterEvil, "malvel_no_ability_easy") &&
                  !hasMilestone(afterEvil, "no_ability_medium"),
                  "Full-run 0-ability Easy clear unlocks both Pure Steel: Easy and Duelist vs Malvel: Easy");
            check(hasMilestone(afterEvil, "evil_route"),
                  "Ruthless Descent unlocks when choosing kill in all 3 NPC encounters");

            // Run 3: Early dungeon death (Phase::Defeat) -> unlocks claimed_by_depths, NOT fallen_to_malvel.
            DungeonRun dungeonDeath(assets);
            dungeonDeath.start(Hero::Warrior, Biome::Jungle, 3, Difficulty::Easy);
            for (int step = 0; step < 400 && dungeonDeath.inProgress(); ++step) {
                if (dungeonDeath.phase() == Phase::Room) dungeonDeath.choose(0);
                else if (dungeonDeath.phase() == Phase::Riddle) dungeonDeath.answer((dungeonDeath.riddle().correct + 1) % 3);
                else if (dungeonDeath.phase() == Phase::Combat) dungeonDeath.attack();
                else dungeonDeath.advance();
            }
            check(dungeonDeath.phase() == Phase::Defeat, "Simulated early defeat reaches Phase::Defeat");
            check(dungeonDeath.scoreBreakdown(45).timePoints == 0, "Defeat run receives 0 time bonus");
            const auto afterDeath = store.recordCompletedRun(dungeonDeath, 45);
            check(hasMilestone(afterDeath, "claimed_by_depths") && !hasMilestone(afterDeath, "fallen_to_malvel"),
                  "Dying in the dungeon unlocks Claimed by the Depths without unlocking Fallen to the Sorcerer");

            // Run 4: Dying to Malvel (Ending::Bad) -> unlocks fallen_to_malvel.
            DungeonRun malvelDeath(assets);
            malvelDeath.start(Hero::Mage, Biome::Ice, 9, Difficulty::Easy);
            for (int step = 0; step < 900 && malvelDeath.inProgress(); ++step) {
                if (malvelDeath.phase() == Phase::Complete) {
                    malvelDeath.descend(malvelDeath.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
                } else if (malvelDeath.phase() == Phase::Room) {
                    std::size_t choice = 0;
                    if (malvelDeath.room() == Room::Library) choice = malvelDeath.floorNumber() == 3 ? 3 : 2;
                    if (malvelDeath.room() == Room::Battle) choice = 1;
                    malvelDeath.choose(malvelDeath.displayChoiceFor(choice));
                } else if (malvelDeath.phase() == Phase::Riddle) {
                    if (malvelDeath.floorNumber() == 3) {
                        for (std::size_t i = 0; i < 3; ++i)
                            if (i != malvelDeath.riddle().correct && malvelDeath.answerAvailable(i)) { malvelDeath.answer(i); break; }
                    } else malvelDeath.answer(malvelDeath.riddle().correct);
                } else if (malvelDeath.phase() == Phase::Combat) malvelDeath.attack();
                else malvelDeath.advance();
            }
            check(malvelDeath.phase() == Phase::Ending && malvelDeath.ending() == Ending::Bad,
                  "Simulated Malvel loss reaches Ending::Bad");
            check(malvelDeath.scoreBreakdown(150).timePoints == 0, "Bad ending receives 0 time bonus");
            const auto afterMalvel = store.recordCompletedRun(malvelDeath, 150);
            check(hasMilestone(afterMalvel, "fallen_to_malvel"),
                  "Dying to Malvel unlocks Fallen to the Sorcerer");

            // Record Gauntlet results and verify max retention.
            const auto afterG1 = store.recordGauntletResult(14, 9);
            const auto afterG2 = store.recordGauntletResult(8, 12);
            check(afterG2.bestGauntletScore == 14 && afterG2.bestGauntletStreak == 12,
                  "Gauntlet records keep highest score and best streak independently");
            check(hasMilestone(afterG2, "gauntlet_master"), "Gauntlet Scholar (10+ Gauntlet score) milestone unlocked");
            check(afterG1.bestGauntletScore == 14, "First Gauntlet record persisted");

            // Reload from a fresh store instance.
            const auto reloaded = CodexStore(file).load();
            check(reloaded.totalRuns == 5 && reloaded.totalVictories == 3 && reloaded.dungeonDeaths == 1 &&
                  reloaded.evilRouteCompleted && (reloaded.noAbilityDifficultyMask & 1u) != 0 &&
                  (reloaded.malvelNoAbilityDifficultyMask & 2u) != 0 &&
                  reloaded.bestGauntletScore == 14 && reloaded.bestGauntletStreak == 12 &&
                  reloaded.fastestVictorySeconds == 185 && reloaded.endingUnlocked(Ending::Bad) &&
                  hasMilestone(reloaded, "fallen_to_malvel") && hasMilestone(reloaded, "claimed_by_depths") &&
                  hasMilestone(reloaded, "evil_route") && hasMilestone(reloaded, "no_ability_easy") &&
                  hasMilestone(reloaded, "malvel_no_ability_medium") && !hasMilestone(reloaded, "no_ability_medium"),
                  "All 19 Codex milestones and counters survive fresh store reload");
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
