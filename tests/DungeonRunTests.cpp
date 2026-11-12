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
void playRoom(DungeonRun& game, bool correct = true, bool storyChoices = false, bool merciful = true) {
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
            if (game.room() == Room::Shop) choice = game.floorNumber() == 3 ? 2 : 1;
            if (game.room() == Room::Encounter && !merciful) choice = 1;
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
void finishFloor(DungeonRun& game, bool storyChoices = false, bool merciful = true) {
    for (int i = 0; i < 9 && game.inProgress() && game.phase() != Phase::Complete; ++i)
        playRoom(game, true, storyChoices, merciful);
    check(game.phase() == (game.floorNumber() == 3 ? Phase::Ending : Phase::Complete), "Complete the current floor");
}
bool playPerfectRun(DungeonRun& game, Hero hero, Biome first, Biome second, unsigned seed) {
    game.start(hero, first, seed);
    for (int step = 0; step < 900 && game.inProgress(); ++step) {
        if (game.phase() == Phase::Complete) game.descend(game.floorNumber() == 1 ? second : Biome::Lava);
        else if (game.phase() == Phase::Room) {
            std::size_t choice = 0;
            if (game.room() == Room::Library && game.floorNumber() < 3)
                choice = hero == Hero::Mage ? 2 : hero == Hero::Rogue ? 1 : 0;
            if (game.room() == Room::Respite) choice = game.floorNumber() == 1 ? 0 : 1;
            if (game.room() == Room::Shop) choice = game.floorNumber() == 2 ? 1 : 2;
            game.choose(choice);
        } else if (game.phase() == Phase::Combat) game.attack();
        else if (game.phase() == Phase::Riddle) game.answer(game.riddle().correct);
        else game.advance();
    }
    return game.phase() == Phase::Ending && game.ending() == Ending::Perfect;
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

        // A losing run is terminal; extra input cannot revive or reset it.
        bool sawDefeat = false;
        game.start(Hero::Warrior, Biome::Jungle, 3);
        for (int i = 0; i < 9 && game.phase() != Phase::Defeat; ++i) playRoom(game, false);
        sawDefeat = game.phase() == Phase::Defeat;
        check(sawDefeat, "A losing run reaches defeat");
        const auto failedPlayer = game.player();
        game.advance(); check(game.phase() == Phase::Defeat, "Defeat cannot advance to another floor");
        game.choose(0); game.attack(); game.answer(0);
        check(!game.inProgress() && !game.descend(Biome::Desert) && game.player().stats == failedPlayer.stats, "Game over cannot resume a failed descent");

        // Floor 2 must be reached through a real first-floor victory, with all
        // player state preserved. Cover every hero and both choices on each floor.
        int threeFloorRuns = 0, hints = 0;
        std::set<std::array<Room, 3>> secondFloorOrders, thirdFloorOrders;
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
                check(game.canDescend() && game.inProgress(), "Second guardian opens the final floor without ending the run");
                const Player carriedToLava = game.player();
                check(!game.descend(Biome::Desert) && !game.descend(Biome::Ice), "Only Malvel's domain follows floor 2");
                check(game.descend(Biome::Lava), "Enter the third floor");
                check(game.player().stats == carriedToLava.stats && game.player().reputation == carriedToLava.reputation &&
                      game.player().battlesWon == carriedToLava.battlesWon && game.player().battlesFought == carriedToLava.battlesFought &&
                      game.player().riddlesAnswered == 16 && game.player().hero == carriedToLava.hero,
                      "The final floor carries the exact existing player");
                check(game.riddles().size() == 35 && game.floorNumber() == 3 && game.biome() == Biome::Lava, "Load the 35 original hard riddles on floor 3");
                check(!game.canDescend() && !game.descend(Biome::Lava), "Cannot reenter or skip the final floor");
                const std::array<Room, 6> thirdTail = {Room::Chest, Room::Battle, Room::Shop, Room::Encounter, Room::Respite, Room::Boss};
                check(std::equal(thirdTail.begin(), thirdTail.end(), game.route().begin() + 3), "Original third-floor route");
                thirdFloorOrders.insert({game.route()[0], game.route()[1], game.route()[2]});
                questions.clear();
                while (game.inProgress()) {
                    check(std::filesystem::exists(assets / game.image()), "Floor 3 artwork exists: " + game.image());
                    check(!DungeonRun::readText(assets / game.storyFile()).empty(), "Floor 3 story exists");
                    check(std::filesystem::exists(assets / "music" / game.musicFile()) && std::filesystem::exists(assets / game.mapImage()), "Lava map and music exist");
                    check(game.enemyMaxHealth() == (game.room() == Room::Boss ? 567.f : 126.f), "Original third-floor enemy health");
                    if (game.room() == Room::Battle) {
                        const auto story = DungeonRun::readText(assets / game.storyFile());
                        check(story.find(game.roomNumber() == 5 ? "knight" : "dragon") != std::string::npos, "Lava enemy artwork matches its story");
                    }
                    if (game.phase() == Phase::Riddle) {
                        questions.insert(game.riddle().question);
                        check(game.timeLimit() == 10 + std::floor(game.player().stats[Wisdom] / 2), "Floor 3 uses the hard timer base");
                        const float health = game.player().stats[Health];
                        game.answer(game.riddle().correct);
                        const float multiplier = game.player().hero == Hero::Mage ? 3.f : 1.5f;
                        check(std::abs(game.player().stats[Health] - health - 2.25f * multiplier) < .001f, "Hard-riddle rewards use floor and class scaling");
                    } else if (game.phase() == Phase::Room) {
                        std::size_t choice = game.room() == Room::Respite ? 1 : game.room() == Room::Shop ? 2 : 0;
                        if (game.room() == Room::Library && game.player().hero == Hero::Mage) choice = 2;
                        if (game.room() == Room::Library && game.player().hero == Hero::Rogue) choice = 1;
                        game.choose(choice);
                    } else if (game.phase() == Phase::Combat) {
                        const float health = game.player().stats[Health];
                        game.attack();
                        if (game.phase() == Phase::Combat)
                            check(std::abs(health - game.player().stats[Health] - (game.room() == Room::Boss ? 15.f : game.roomNumber() == 5 ? 12.f : 9.f)) < .001f,
                                  "Lava enemies retaliate with original floor-scaled damage");
                    } else game.advance();
                }
                check(game.phase() == Phase::Ending && game.ending() == Ending::True, "Complete all three floors and earn the true ending");
                check(questions.size() == 8 && game.player().riddlesAnswered == 24 && game.player().riddlesCorrect == 24 && game.player().battlesFought == 9,
                      "Final results contain 24 riddles and nine attempted battles");
                check(game.enemyHealth() == 0 && game.player().battlesWon > carriedToLava.battlesWon, "Malvel's actual defeat counts as a battle victory");
                check(game.message() == DungeonRun::readText(assets / "textfiles/ending/trueending.txt") && std::filesystem::exists(assets / game.image()), "Original true ending text and art");
                const auto finished = game.player();
                game.attack(); game.advance(); game.answer(0); game.choose(0); game.tick(10000);
                check(!game.canDescend() && !game.descend(Biome::Lava) && game.player().stats == finished.stats && game.player().battlesWon == finished.battlesWon,
                      "Ending is terminal and cannot grant duplicate rewards");
                ++threeFloorRuns;
            }
        check(secondFloorOrders.size() == 6 && thirdFloorOrders.size() == 6 && hints > 0, "Exercise every opening order on every floor and observation hints");

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
        // If this strong hero survives, repeat with poor answers to reach defeat.
        if (game.phase() == Phase::Complete) {
            game.start(Hero::Warrior, Biome::Ice, 3); finishFloor(game);
            game.descend(Biome::Desert);
            while (game.phase() != Phase::Defeat && game.phase() != Phase::Complete) playRoom(game, false);
        }
        check(game.phase() == Phase::Defeat, "A failed second-floor route reaches defeat");
        check(!game.inProgress() && !game.canDescend(), "Failed second-floor descent ends the run");
        game.start(Hero::Warrior, Biome::Jungle, 1);
        check(game.player().stats == DungeonRun::makePlayer(Hero::Warrior).stats && game.floorNumber() == 1 && game.player().reputation == 0, "New descent resets class ability exactly once");

        game.start(Hero::Mage, Biome::Ice, 9); finishFloor(game);
        game.descend(Biome::Ghost); finishFloor(game); game.descend(Biome::Lava);
        const DungeonRun lavaEntry = game;

        // Story choices and both sealed chests on the final floor award once.
        while (game.room() != Room::Library) playRoom(game);
        const auto beforeDiary = game.player();
        game.choose(0); game.choose(0);
        check(game.player().reputation == beforeDiary.reputation + 1 &&
              std::abs(game.player().stats[Strength] - beforeDiary.stats[Strength] - 3.f) < .001f &&
              game.message().find(DungeonRun::readText(assets / "textfiles/biomes/lava/library/lavalibrarydiary.txt")) != std::string::npos,
              "The Princess Diary gives its original story, fixed stat reward and one reputation");
        for (const bool timeout : {false, true}) {
            game = lavaEntry;
            while (game.room() != Room::Chest) playRoom(game);
            game.choose(0);
            const auto before = game.player();
            if (timeout) game.tick(10000);
            else for (std::size_t i = 0; i < 3; ++i) if (i != game.riddle().correct && game.answerAvailable(i)) { game.answer(i); break; }
            check(std::abs(before.stats[Health] - game.player().stats[Health] - (timeout ? 1.5f : 4.5f)) < .001f, "Hard-riddle failure and timeout penalties");
            game.advance();
            check(game.player().reputation == before.reputation && game.phase() == Phase::Room, "Failed lava seal grants no orb or reputation");
        }
        game = lavaEntry;
        for (int target : {0, 4}) {
            while (target == 0 ? game.room() != Room::Chest : game.roomNumber() < target) playRoom(game);
            game.choose(0); game.answer(game.riddle().correct); game.advance();
            const auto before = game.player();
            const bool opening = game.roomNumber() <= 3;
            game.advance();
            check(std::abs(game.player().stats[Health] - before.stats[Health] - 18.f) < .001f &&
                  game.player().reputation == before.reputation + (opening ? 1 : 0), "Both lava chests give scaled loot; only the opening orb gives reputation");
            if (opening) check(game.message().find("Malvel") != std::string::npos, "Orb reveals Malvel's presence");
            const auto claimed = game.player();
            game.advance();
            check(game.player().stats == claimed.stats && game.player().reputation == claimed.reputation, "Lava loot cannot be claimed twice");
        }
        while (game.roomNumber() < 6) playRoom(game);
        for (std::size_t choice = 0; choice < 3; ++choice) {
            auto shop = game;
            const auto before = shop.player();
            shop.choose(choice);
            if (choice == 0) check(std::abs(shop.player().stats[Health] - before.stats[Health] - 7.5f) < .001f, "Emberheart retains its fixed individual reward");
            if (choice == 1) check(std::abs(shop.player().stats[Strength] - before.stats[Strength] - 3.f) < .001f, "Corebound retains its fixed individual reward");
            if (choice == 2) check(std::abs(shop.player().stats[Health] - before.stats[Health] - 18.f) < .001f, "Inferno grants scaled all-stat rewards");
        }
        playRoom(game);
        for (std::size_t choice = 0; choice < 2; ++choice) {
            auto encounter = game;
            const auto before = encounter.player();
            encounter.choose(choice);
            check(encounter.player().reputation == before.reputation + (choice == 0 ? 1 : 0), "Threatening minions is the reputation choice");
            if (choice == 0) check(encounter.message().find(DungeonRun::readText(assets / "textfiles/biomes/lava/encounter/lavaminionfree.txt")) != std::string::npos, "Threatened minions reveal their original story");
            else check(std::abs(encounter.player().stats[Health] - before.stats[Health] - 18.f) < .001f, "Killing minions awards scaled stats");
        }

        // All three endings are reachable through real runs, without test-only
        // player mutation. Low reputation still wins, but misses the true ending.
        game.start(Hero::Mage, Biome::Jungle, 15); finishFloor(game, false, false);
        game.descend(Biome::Ghost); finishFloor(game, false, false);
        game.descend(Biome::Lava); finishFloor(game, false, false);
        check(game.phase() == Phase::Ending && game.ending() == Ending::Good && game.player().reputation < 5 &&
              game.message() == DungeonRun::readText(assets / "textfiles/ending/goodending.txt") && std::filesystem::exists(assets / game.image()),
              "A real low-reputation victory produces the original good ending");
        game.start(Hero::Mage, Biome::Ice, 9);
        for (int floor = 1; floor <= 2; ++floor) {
            while (game.inProgress() && game.phase() != Phase::Complete) {
                if (game.phase() == Phase::Room) {
                    const std::size_t choice = game.room() == Room::Battle ? 1 : game.room() == Room::Library ? 2 : 0;
                    game.choose(choice);
                } else if (game.phase() == Phase::Riddle) game.answer(game.riddle().correct);
                else if (game.phase() == Phase::Combat) game.attack();
                else game.advance();
            }
            check(game.phase() == Phase::Complete, "Cautious route reaches the final floor");
            check(game.descend(floor == 1 ? Biome::Ghost : Biome::Lava), "Continue the cautious route");
        }
        while (game.inProgress()) {
            if (game.phase() == Phase::Room) {
                std::size_t choice = game.room() == Room::Battle ? 1 : game.room() == Room::Library ? 3 : 0;
                game.choose(choice);
            } else if (game.phase() == Phase::Riddle) {
                for (std::size_t i = 0; i < 3; ++i) if (i != game.riddle().correct && game.answerAvailable(i)) { game.answer(i); break; }
            } else if (game.phase() == Phase::Combat) game.attack();
            else game.advance();
        }
        check(game.phase() == Phase::Ending && game.ending() == Ending::Bad && game.enemyHealth() > 0 &&
              game.message() == DungeonRun::readText(assets / "textfiles/ending/badending.txt") && std::filesystem::exists(assets / game.image()),
              "Losing the final battle produces the original bad ending (phase " + std::to_string(static_cast<int>(game.phase())) + ")");
        Player thresholds;
        thresholds.reputation = 5; thresholds.riddlesCorrect = 14; thresholds.battlesWon = 4;
        check(DungeonRun::resolveEnding(true, thresholds) == Ending::True, "Fourteen riddles meets the original true-ending threshold");
        thresholds.riddlesCorrect = 13; thresholds.battlesWon = 5;
        check(DungeonRun::resolveEnding(true, thresholds) == Ending::True, "Five battles is an alternative true-ending threshold");
        thresholds.battlesWon = 4;
        check(DungeonRun::resolveEnding(true, thresholds) == Ending::Good, "Thirteen riddles and four battles is below the threshold");
        thresholds.reputation = 4; thresholds.riddlesCorrect = 24; thresholds.battlesWon = 9;
        check(DungeonRun::resolveEnding(true, thresholds) == Ending::Good, "High performance cannot replace reputation");
        check(DungeonRun::resolveEnding(false, thresholds) == Ending::Bad, "Losing Malvel always gives the bad ending");

        Player perfect;
        perfect.battlesFought = perfect.battlesWon = 9;
        perfect.riddlesAnswered = perfect.riddlesCorrect = 24;
        perfect.reputation = 9;
        check(DungeonRun::resolveEnding(true, perfect) == Ending::Perfect, "A 100% run takes priority over the true ending");
        check(DungeonRun::resolveEnding(false, perfect) == Ending::Bad, "Perfect counters cannot replace defeating Malvel");
        for (int missed = 0; missed < 5; ++missed) {
            auto incomplete = perfect;
            if (missed == 0) incomplete.battlesFought = incomplete.battlesWon = 8;
            if (missed == 1) incomplete.battlesWon = 8;
            if (missed == 2) incomplete.riddlesAnswered = incomplete.riddlesCorrect = 23;
            if (missed == 3) incomplete.riddlesCorrect = 23;
            if (missed == 4) incomplete.reputation = 8;
            check(DungeonRun::resolveEnding(true, incomplete) == Ending::True,
                  "Skipping/losing a fight, missing/failing a riddle, or missing reputation keeps the true ending");
        }

        // Prove that the unlock can actually be earned with every hero and
        // biome pairing; no counters or stats are injected into these runs.
        int perfectRuns = 0;
        for (int hero = 0; hero < 4; ++hero) for (int first = 0; first < 2; ++first) for (int second = 2; second < 4; ++second) {
            bool unlocked = false;
            for (unsigned seed = 0; seed < 200 && !unlocked; ++seed)
                unlocked = playPerfectRun(game, static_cast<Hero>(hero), static_cast<Biome>(first), static_cast<Biome>(second), seed);
            check(unlocked, "Perfect ending must be reachable: hero " + std::to_string(hero) + " first " + std::to_string(first) + " second " + std::to_string(second));
            check(game.player().battlesFought == 9 && game.player().battlesWon == 9 && game.player().riddlesAnswered == 24 &&
                  game.player().riddlesCorrect == 24 && game.player().reputation == 9, "Perfect runs earn all required totals");
            check(game.endingName() == "A Crown in Ashes" && game.image() == "images/ending/perfectending.jpg" &&
                  game.storyFile() == "textfiles/ending/perfectending.txt" && std::filesystem::exists(assets / game.image()), "Perfect ending uses its named artwork and story");
            check(game.message() == DungeonRun::readText(assets / game.storyFile()) && game.message().find("The princess is dead.") != std::string::npos,
                  "The new ending's full story resolves Sophie's death");
            const auto finished = game.player();
            game.attack(); game.advance(); game.choose(0); game.answer(0); game.tick(10000);
            check(!game.inProgress() && !game.canDescend() && !game.descend(Biome::Lava) &&
                  game.ending() == Ending::Perfect && game.player().stats == finished.stats && game.player().battlesWon == 9,
                  "Perfect ending is terminal and cannot award a tenth victory");
            ++perfectRuns;
        }

        std::cout << completed << " first-floor runs, " << threeFloorRuns << " complete three-floor runs and " << perfectRuns
                  << " earned perfect endings; all four endings, carryover, routes, rewards, hints, timeouts and assets passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
