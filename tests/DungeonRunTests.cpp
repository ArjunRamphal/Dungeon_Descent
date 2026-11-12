#include "DungeonRun.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace dungeon;

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void playRoom(DungeonRun& game, bool correct = true, bool storyChoices = false) {
    const int room = game.roomNumber();
    for (int steps = 0; steps < 100 && game.roomNumber() == room; ++steps) {
        switch (game.phase()) {
        case Phase::Room: {
            std::size_t choice = 0;
            if (game.room() == Room::Library) {
                if (game.player().hero == Hero::Mage) choice = 2;
                else if (game.player().hero == Hero::Rogue) choice = 1;
            }
            // A generous growth route exercises every fight, including the boss.
            if (game.room() == Room::Respite) choice = 1;
            if (game.room() == Room::Shop) choice = 1;
            if (storyChoices && game.room() == Room::Respite) choice = 0;
            if (storyChoices && game.room() == Room::Shop) choice = game.floorNumber() == 1 ? 2 : 0;
            game.choose(choice);
            break;
        }
        case Phase::Combat: game.attack(); break;
        case Phase::Riddle:
            if (correct) game.answer(game.riddle().correct);
            else for (std::size_t i = 0; i < 3; ++i) if (i != game.riddle().correct && game.answerAvailable(i)) { game.answer(i); break; }
            break;
        case Phase::Outcome:
        case Phase::RiddleOutcome:
        case Phase::Loot: game.advance(); break;
        default: return;
        }
    }
}
void finishFloor(DungeonRun& game, bool storyChoices = false) {
    for (int i = 0; i < 9 && game.phase() != Phase::Complete && game.phase() != Phase::Defeat; ++i)
        playRoom(game, true, storyChoices);
    check(game.phase() == Phase::Complete, "Complete the current floor");
}
}
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Pass the assets directory");
        const std::filesystem::path assets = argv[1];
        DungeonRun game(assets);
        check(game.riddles().size() == 30, "Load all 30 original riddles");
        int completed = 0;
        std::set<std::array<Room, 3>> permutations;
        for (int hero = 0; hero < 4; ++hero) for (int biome = 0; biome < 2; ++biome) for (unsigned seed = 0; seed < 100; ++seed) {
            game.start(static_cast<Hero>(hero), static_cast<Biome>(biome), seed);
            permutations.insert({game.route()[0], game.route()[1], game.route()[2]});
            check(game.roomNumber() == 1 && game.player().riddlesAnswered == 0, "New run resets progress");
            std::set<std::string> questions;
            for (int room = 1; room <= 9; ++room) {
                check(game.roomNumber() == room, "Visit all nine rooms in order");
                check(std::filesystem::exists(assets / game.image()), "Room artwork exists: " + game.image());
                check(!DungeonRun::readText(assets / game.storyFile()).empty(), "Room story exists");
                if (room < 9) questions.insert(game.riddle().question);
                playRoom(game);
                if (game.phase() == Phase::Defeat) break;
            }
            check(game.phase() == Phase::Complete, "Winning route failed: hero " + std::to_string(hero) + " seed " + std::to_string(seed));
            check(game.player().riddlesAnswered == 8 && game.player().riddlesCorrect == 8, "Eight non-boss riddles");
            check(questions.size() == 8, "Riddles do not repeat within a floor");
            check(game.player().battlesFought == 3 && game.player().battlesWon >= 1, "Three battles attempted and boss defeated");
            const auto finalStats = game.player().stats;
            game.advance(); game.attack(); game.answer(0); game.choose(0);
            check(game.phase() == Phase::Complete && game.player().stats == finalStats, "Terminal state ignores extra input");
            ++completed;
        }
        check(permutations.size() == 6, "All six opening room orders are reachable");

        // Failed chest riddles never award loot; timeout resolves only once.
        for (bool timeout : {false, true}) {
            game.start(Hero::Warrior, Biome::Ice, 12);
            while (game.room() != Room::Chest) playRoom(game);
            game.choose(0);
            check(game.phase() == Phase::Riddle, "Chest is sealed before its riddle");
            const auto before = game.player().stats;
            if (timeout) game.tick(10000); else game.answer((game.riddle().correct + 1) % 3);
            const auto after = game.player().stats;
            const int count = game.player().riddlesAnswered;
            game.tick(10000); game.answer(game.riddle().correct);
            check(game.player().riddlesAnswered == count, "A riddle resolves only once");
            check(after[Strength] < before[Strength], "Failure applies a stat penalty");
            const int room = game.roomNumber();
            game.advance();
            check(game.roomNumber() == room + 1 && game.player().stats == after, "Locked chest cannot award treasure");
        }
        game.start(Hero::Mage, Biome::Jungle, 12);
        while (game.room() != Room::Chest) playRoom(game);
        game.choose(0); game.tick(-5); game.tick(NAN);
        check(game.timeLeft() == game.timeLimit(), "Invalid deltas do not alter timers");
        game.answer(game.riddle().correct); game.advance();
        check(game.phase() == Phase::Loot, "Correct chest answer unlocks treasure");
        const auto beforeLoot = game.player().stats;
        game.advance();
        check(std::abs(game.player().stats[Health] - beforeLoot[Health] - 6) < .001f, "Mage chest health multiplier");
        const auto afterLoot = game.player().stats;
        game.advance();
        check(game.player().stats == afterLoot, "Chest reward is claimed once");

        // A losing run must stop; retry restores its room-entry checkpoint.
        bool sawDefeat = false;
        game.start(Hero::Warrior, Biome::Jungle, 3);
        for (int i = 0; i < 9 && game.phase() != Phase::Defeat; ++i) playRoom(game, false);
        sawDefeat = game.phase() == Phase::Defeat;
        check(sawDefeat, "A losing run reaches defeat");
        const int failedRoom = game.roomNumber();
        game.advance(); check(game.phase() == Phase::Defeat, "Defeat cannot advance to another floor");
        game.retryRoom();
        check(game.phase() == Phase::Room && game.roomNumber() == failedRoom && game.player().stats[Health] > 0, "Retry restores the same room alive");

        // Floor 2 must be reached through a real first-floor victory, with all
        // player state preserved. Cover every hero and both choices on each floor.
        int twoFloorRuns = 0, hints = 0;
        std::set<std::array<Room, 3>> secondFloorOrders;
        game.start(Hero::Warrior, Biome::Ice, 1);
        check(!game.descend(Biome::Desert) && game.floorNumber() == 1, "Cannot skip the first guardian");
        for (int hero = 0; hero < 4; ++hero) for (int first = 0; first < 2; ++first)
            for (int second = 2; second < 4; ++second) for (unsigned seed = 0; seed < 100; ++seed) {
                game.start(static_cast<Hero>(hero), static_cast<Biome>(first), seed);
                finishFloor(game);
                const Player carried = game.player();
                check(game.canDescend(), "First guardian unlocks descent");
                check(!game.descend(Biome::Ice) && game.player().stats == carried.stats, "Reject invalid next biome without changing player");
                check(game.descend(static_cast<Biome>(second)), "Enter floor 2");
                check(game.floorNumber() == 2 && game.roomNumber() == 1, "Second floor resets local room progress");
                check(game.player().stats == carried.stats && game.player().hero == carried.hero &&
                      game.player().reputation == carried.reputation && game.player().riddlesAnswered == 8 &&
                      game.player().battlesFought == carried.battlesFought, "Stats, health, class and counters carry unchanged");
                check(game.riddles().size() == 37, "Load all 37 original medium riddles");
                check(!game.descend(Biome::Ghost), "A second descent cannot reset floor 2");
                const std::array<Room, 6> tail = {Room::Shop, Room::Chest, Room::Encounter, Room::Respite, Room::Battle, Room::Boss};
                check(std::equal(tail.begin(), tail.end(), game.route().begin() + 3), "Original second-floor route");
                secondFloorOrders.insert({game.route()[0], game.route()[1], game.route()[2]});
                std::set<std::string> questions;
                while (game.phase() != Phase::Complete && game.phase() != Phase::Defeat) {
                    check(std::filesystem::exists(assets / game.image()), "Floor 2 artwork exists: " + game.image());
                    check(!DungeonRun::readText(assets / game.storyFile()).empty(), "Floor 2 story exists");
                    if (game.biome() == Biome::Desert && game.room() == Room::Battle) {
                        const auto story = DungeonRun::readText(assets / game.storyFile());
                        check(game.roomNumber() == 8 ? story.find("scorpion") != std::string::npos : story.find("Sandwyrm") != std::string::npos,
                              "Desert enemy dialogue matches the scorpion/Sandwyrm artwork");
                    }
                    check(std::filesystem::exists(assets / "music" / game.musicFile()), "Biome music exists");
                    check(std::filesystem::exists(assets / game.mapImage()), "Second-floor map exists");
                    if (game.room() == Room::Boss) check(game.enemyMaxHealth() == 306, "Original floor 2 guardian health");
                    else check(game.enemyMaxHealth() == 68, "Original floor 2 ordinary enemy health");
                    if (game.roomNumber() < 9) questions.insert(game.riddle().question);
                    // Inspect actual riddle hints before resolving this room.
                    const int room = game.roomNumber();
                    while (game.roomNumber() == room && game.phase() != Phase::Complete && game.phase() != Phase::Defeat) {
                        if (game.phase() == Phase::Riddle) {
                            check(game.timeLimit() == 15 + std::floor(game.player().stats[Wisdom] / 2), "Floor 2 timer uses medium base time");
                            check(game.answerAvailable(game.riddle().correct), "Observation never hides the correct answer");
                            if (game.hasObservationHint()) {
                                ++hints;
                                int available = 0;
                                for (std::size_t i = 0; i < 3; ++i) {
                                    if (game.answerAvailable(i)) ++available;
                                    else { const int answered = game.player().riddlesAnswered; game.answer(i); check(game.player().riddlesAnswered == answered, "Hidden answer cannot be selected"); }
                                }
                                check(available == 2, "Observation removes exactly one false answer");
                            }
                            game.answer(game.riddle().correct);
                        } else if (game.phase() == Phase::Room) {
                            std::size_t choice = game.room() == Room::Respite || game.room() == Room::Shop ? 1 : 0;
                            if (game.room() == Room::Library && game.player().hero == Hero::Mage) choice = 2;
                            if (game.room() == Room::Library && game.player().hero == Hero::Rogue) choice = 1;
                            game.choose(choice);
                        } else if (game.phase() == Phase::Combat) game.attack();
                        else game.advance();
                    }
                }
                check(game.phase() == Phase::Complete, "Two-floor strategy failed: hero " + std::to_string(hero) + " second " + std::to_string(second) + " seed " + std::to_string(seed));
                check(questions.size() == 8 && game.player().riddlesAnswered == 16 && game.player().riddlesCorrect == 16, "Sixteen unique-per-floor riddles answered");
                check(game.player().battlesFought == 6 && game.player().reputation == carried.reputation + 3, "Second-floor battles and three reputation sources");
                check(game.message().find("Malvel") != std::string::npos, "Original guardian revelation is presented");
                check(!game.canDescend() && !game.descend(Biome::Desert), "Run ends after the second guardian");
                ++twoFloorRuns;
            }
        check(secondFloorOrders.size() == 6 && hints > 0, "Exercise opening orders and high-observation hints on floor 2");

        // Both second-floor chests need a solved seal. The vision and its
        // reputation reward are granted once, only by room five's chest.
        for (const bool timeout : {false, true}) {
            game.start(Hero::Mage, Biome::Jungle, 5); finishFloor(game);
            check(game.descend(Biome::Ghost), "Reach floor 2 for penalty checks");
            while (game.roomNumber() < 5) playRoom(game);
            game.choose(0);
            const auto before = game.player();
            if (timeout) game.tick(10000);
            else for (std::size_t i = 0; i < 3; ++i) if (i != game.riddle().correct && game.answerAvailable(i)) { game.answer(i); break; }
            check(std::abs(game.player().stats[Health] - (before.stats[Health] - (timeout ? 1.f : 3.f))) < .001f, "Floor 2 failure penalty doubles");
            game.advance();
            check(game.roomNumber() == 6 && game.player().reputation == before.reputation, "Failed seal denies vision and reputation");
        }
        game.start(Hero::Mage, Biome::Ice, 9); finishFloor(game);
        auto retryBaseline = game.player();
        game.descend(Biome::Desert);
        while (game.roomNumber() < 5) playRoom(game);
        game.choose(0); game.answer(game.riddle().correct); game.advance();
        const auto beforeVision = game.player();
        game.advance();
        check(game.player().reputation == beforeVision.reputation + 1 &&
              std::abs(game.player().stats[Health] - beforeVision.stats[Health] - 12) < .001f &&
              game.message().find("Sophie") != std::string::npos, "Vision chest applies scaled reward and original revelation");
        const auto visionStats = game.player().stats;
        game.choose(0); game.answer(0); game.advance();
        check(game.roomNumber() == 6 && game.player().stats == visionStats, "Vision cannot be claimed twice");
        game.choose(1); // Take the vial rather than saving the butler.
        check(game.player().reputation == beforeVision.reputation + 1, "Selfish encounter grants no reputation");
        game.advance(); game.tick(10000); game.advance();
        while (game.phase() != Phase::Defeat && game.phase() != Phase::Complete) playRoom(game, false);
        // If this strong hero survives, repeat a fresh run with poor answers to
        // reach defeat before exercising the floor checkpoint.
        if (game.phase() == Phase::Complete) {
            game.start(Hero::Warrior, Biome::Ice, 3); finishFloor(game);
            retryBaseline = game.player();
            game.descend(Biome::Desert);
            while (game.phase() != Phase::Defeat && game.phase() != Phase::Complete) playRoom(game, false);
        }
        check(game.phase() == Phase::Defeat, "A failed second-floor route reaches defeat");
        game.retryFloor();
        check(game.floorNumber() == 2 && game.roomNumber() == 1 && game.phase() == Phase::Room && game.player().riddlesAnswered == 8, "Retry floor 2 preserves first-floor progress");
        check(game.player().stats == retryBaseline.stats && game.player().reputation == retryBaseline.reputation &&
              game.player().battlesWon == retryBaseline.battlesWon && game.player().battlesFought == retryBaseline.battlesFought,
              "Floor retry restores the exact carried player and removes attempted-floor rewards");
        game.start(Hero::Warrior, Biome::Jungle, 1);
        check(game.player().stats == DungeonRun::makePlayer(Hero::Warrior).stats && game.floorNumber() == 1 && game.player().reputation == 0, "New descent resets class ability exactly once");

        std::cout << completed << " first-floor runs and " << twoFloorRuns << " complete two-floor runs; carryover, routes, rewards, hints, timeouts, retries and assets passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
