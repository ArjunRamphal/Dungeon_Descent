#include "DungeonRun.h"
#include "SuspendSave.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace dungeon;
using namespace dungeon::portable;

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <class F>
void rejects(F fn, const std::string& message) {
    bool failed = false;
    try {
        fn();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, message);
}

std::string readBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

struct TempDir {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("DungeonDescent-suspend-test-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    ~TempDir() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }
};
} // namespace

int main(int argc, char** argv) {
    try {
        check(argc == 2, "Pass the assets directory.");
        const std::filesystem::path assets = argv[1];
        TempDir temp;
        const auto saveFile = temp.root / "profile" / "suspend_run.sav";
        SuspendSave store(saveFile);

        check(!store.exists() && !store.peek().has_value(), "Fresh profile has no suspended run");

        DungeonRun run(assets);
        check(!run.canSuspend(), "Unstarted run cannot be suspended");
        rejects([&] { store.save(run, 10.0); }, "Reject saving an unstarted run");

        run.start(Hero::Rogue, Biome::Jungle, 123456u, Difficulty::Hard);
        check(run.canSuspend(), "Active Phase::Room run can be suspended");

        // Advance into an Outcome and verify deltas + suspend save round-trip.
        while (run.room() == Room::Chest || run.room() == Room::Battle) {
            if (run.room() == Room::Chest) {
                run.choose(0);
                check(!run.canSuspend(), "Timed Phase::Riddle blocks suspending");
                rejects([&] { store.save(run, 25.0); }, "SuspendSave rejects saving during active timed riddle");
                run.answer(run.riddle().correct);
                check(run.canSuspend(), "RiddleOutcome allows suspending between riddles");
                run.advance();
                if (run.phase() == Phase::Loot) {
                    run.advance();
                    run.advance();
                }
            } else {
                run.choose(0);
                check(run.canSuspend(), "Phase::Combat allows suspending between timed riddles");
                break;
            }
        }

        // Test suspending mid-combat with class ability used or in Room phase.
        if (run.phase() == Phase::Combat) {
            run.attack(CombatAction::ClassAbility);
        } else {
            run.choose(0);
        }
        store.save(run, 142.75, true, "2026-11-21");
        check(store.exists(), "Suspend file exists after save");

        DungeonRun resumed(assets);
        const auto loaded = store.loadAndConsume(resumed);
        check(loaded.has_value(), "loadAndConsume returns saved metadata");
        check(!store.exists(), "Consume-on-resume deletes suspend file (roguelike save-and-exit)");
        check(std::abs(loaded->playTimeSeconds - 142.75) < 1e-6, "Playtime preserved across suspend save");
        check(loaded->seed == 123456u && loaded->dailyRun && loaded->dailyDate == "2026-11-21",
              "Seed and daily run flags preserved");

        check(resumed.seed() == run.seed(), "Resumed seed matches");
        check(resumed.difficulty() == run.difficulty(), "Resumed difficulty matches");
        check(resumed.biome() == run.biome() && resumed.firstBiome() == run.firstBiome(), "Resumed biome matches");
        check(resumed.floorNumber() == run.floorNumber() && resumed.roomNumber() == run.roomNumber(), "Resumed floor/room matches");
        check(resumed.phase() == run.phase(), "Resumed phase matches");
        check(resumed.player().hero == run.player().hero && resumed.player().stats == run.player().stats,
              "Resumed hero and stats match");
        check(resumed.player().reputation == run.player().reputation &&
              resumed.player().riddlesCorrect == run.player().riddlesCorrect &&
              resumed.player().riddlesAnswered == run.player().riddlesAnswered &&
              resumed.player().battlesWon == run.player().battlesWon &&
              resumed.player().battlesFought == run.player().battlesFought &&
              resumed.player().strikesLeftover == run.player().strikesLeftover,
              "Resumed counters match");
        check(resumed.route() == run.route(), "Resumed room route matches");
        check(resumed.strikesLeft() == run.strikesLeft() &&
              resumed.enemyHealth() == run.enemyHealth() &&
              resumed.classAbilityAvailable() == run.classAbilityAvailable(),
              "Resumed combat state matches");
        check(resumed.lastStatDeltas() == run.lastStatDeltas() &&
              resumed.lastReputationDelta() == run.lastReputationDelta(),
              "Resumed stat deltas match");
        check(resumed.message() == run.message(), "Resumed message matches");

        // Verify RNG state determinism after resume: identical subsequent choices produce identical outcomes.
        DungeonRun second(assets);
        const auto expectedState = run.serializeState();
        second.deserializeState(expectedState);
        int steps = 0;
        for (; steps < 1500 && run.inProgress() && resumed.inProgress(); ++steps) {
            if (run.phase() == Phase::Complete) {
                const Biome nextBiome = run.floorNumber() == 1 ? Biome::Ghost : Biome::Lava;
                check(run.descend(nextBiome) && resumed.descend(nextBiome), "Descend cleanly to next floor");
            } else if (run.phase() == Phase::Room) {
                run.choose(0);
                resumed.choose(0);
            } else if (run.phase() == Phase::Combat) {
                run.attack();
                resumed.attack();
            } else if (run.phase() == Phase::Riddle) {
                check(run.riddle().question == resumed.riddle().question &&
                      run.riddle().answers == resumed.riddle().answers,
                      "Resumed RNG produces identical riddle order and answer shuffle");
                run.answer(run.riddle().correct);
                resumed.answer(resumed.riddle().correct);
            } else {
                run.advance();
                resumed.advance();
            }
            check(run.player().stats == resumed.player().stats && run.phase() == resumed.phase(),
                  "Resumed run diverges deterministically zero steps");
        }
        check(!run.inProgress() && !resumed.inProgress(), "Simulated resumed run terminates cleanly");

        // Verify v6 Suspend Save preserves mid-combat monster trait flags (Ice Golem shattered Glacial Shell + bossTurn).
        {
            DungeonRun traitRun(assets);
            traitRun.start(Hero::Warrior, Biome::Ice, 42u, Difficulty::Impossible);
            while (traitRun.inProgress() && !(traitRun.phase() == Phase::Room && traitRun.room() == Room::Battle && traitRun.roomNumber() == 5)) {
                if (traitRun.phase() == Phase::Room) {
                    traitRun.choose(traitRun.room() == Room::Battle ? 1 : traitRun.displayChoiceFor(0));
                } else if (traitRun.phase() == Phase::Riddle) {
                    traitRun.answer(traitRun.riddle().correct);
                } else {
                    traitRun.advance();
                }
            }
            check(traitRun.enemyName() == "Ice Golem" && traitRun.enemyTraitName() == "Glacial Shell",
                  "Reach Ice Golem for v6 trait persistence test");
            traitRun.choose(0);
            check(!traitRun.enemyShellBroken() && traitRun.bossIntent().shattersOnPierce, "Ice Golem starts with intact Glacial Shell");
            traitRun.attack(CombatAction::HeavyStrike);
            check(traitRun.enemyShellBroken() && traitRun.lastCombatExchange().shellShattered && traitRun.bossTurn() == 2,
                  "Heavy Strike shatters Glacial Shell and advances monster turn");
            store.save(traitRun, 55.5);
            DungeonRun traitResumed(assets);
            const auto meta = store.loadAndConsume(traitResumed);
            check(meta.has_value(), "Load v6 mid-combat trait save");
            check(traitResumed.enemyShellBroken() == traitRun.enemyShellBroken() &&
                  traitResumed.enemyUndyingUsed() == traitRun.enemyUndyingUsed() &&
                  traitResumed.playerVenomStacks() == traitRun.playerVenomStacks() &&
                  traitResumed.bossTurn() == traitRun.bossTurn() &&
                  traitResumed.lastCombatExchange().shellShattered == traitRun.lastCombatExchange().shellShattered,
                  "v6 Suspend Save preserves shattered shell, venom stacks, undying flag, and turn counter");
        }

        // Corrupt / tampered file is rejected without deleting or overwriting; clear() removes file.
        std::ofstream bad(saveFile, std::ios::binary | std::ios::trunc);
        bad << "DUNGEON_DESCENT_SUSPEND 1 12345\napp 1.0 7 0 \ncorrupt";
        bad.close();
        DungeonRun target(assets);
        rejects([&] { store.loadAndConsume(target); }, "Tampered suspend file checksum must be rejected");
        check(store.exists() && readBytes(saveFile).find("corrupt") != std::string::npos,
              "Rejected corrupt suspend file is not silently deleted");
        store.clear();
        check(!store.exists(), "clear() removes suspend save when run ends or is discarded");

        // Concurrent saves leave no staging leftovers and always yield a valid readable save.
        DungeonRun cRun(assets);
        cRun.start(Hero::Mage, Biome::Ice, 99u, Difficulty::Medium);
        std::vector<std::future<void>> tasks;
        for (int i = 0; i < 8; ++i) {
            tasks.push_back(std::async(std::launch::async, [&, i] {
                SuspendSave concurrentStore(saveFile);
                concurrentStore.save(cRun, 10.0 + i);
            }));
        }
        for (auto& t : tasks) t.get();
        DungeonRun cLoaded(assets);
        const auto cMeta = store.loadAndConsume(cLoaded);
        check(cMeta.has_value() && cLoaded.seed() == 99u && cLoaded.player().hero == Hero::Mage,
              "Concurrent suspend saves stay valid and atomic");
        for (const auto& entry : std::filesystem::recursive_directory_iterator(temp.root)) {
            check(entry.path().filename().generic_u8string().find(".suspend-pending-") != 0,
                  "Staging directories are cleaned up");
        }

        std::cout << "SuspendSave serialization, RNG parity, timed-riddle guard, consume-on-resume, checksum rejection, and concurrency passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
