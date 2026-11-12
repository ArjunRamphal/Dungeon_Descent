#include "FloorOne.h"
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
void playRoom(FloorOne& game, bool correct = true) {
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
            game.choose(choice);
            break;
        }
        case Phase::Combat: game.attack(); break;
        case Phase::Riddle: game.answer(correct ? game.riddle().correct : (game.riddle().correct + 1) % 3); break;
        case Phase::Outcome:
        case Phase::RiddleOutcome:
        case Phase::Loot: game.advance(); break;
        default: return;
        }
    }
}
}
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Pass the assets directory");
        const std::filesystem::path assets = argv[1];
        FloorOne game(assets);
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
                check(!FloorOne::readText(assets / game.storyFile()).empty(), "Room story exists");
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

        std::cout << completed << " complete seeded runs; chest, timeout, retry, terminal-state and asset checks passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
