#include "DungeonRun.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace dungeon;
namespace {
void check(bool value, const std::string& message) {
    if (!value) throw std::runtime_error(message);
}
struct Profile { float health = 0, damage = 0; };
using Profiles = std::array<std::array<Profile, 9>, 4>;
bool play(DungeonRun& game, Difficulty difficulty, Hero hero, Biome first, Biome second, unsigned seed,
          bool reputation, Profiles& profiles, int& retaliations) {
    game.start(hero, first, seed, difficulty);
    for (int step = 0; step < 1500 && game.inProgress(); ++step) {
        check(game.difficulty() == difficulty, "The chosen difficulty lasts through the descent");
        switch (game.phase()) {
        case Phase::Room: {
            if (game.room() == Room::Battle || game.room() == Room::Boss) {
                const int role = game.room() == Room::Boss ? 2 : game.roomNumber() <= 3 ? 0 : 1;
                auto& profile = profiles[static_cast<int>(difficulty)][(game.floorNumber() - 1) * 3 + role];
                const Profile observed{game.enemyMaxHealth(), game.enemyDamage()};
                check(observed.health > 0 && observed.damage > 0 && std::isfinite(observed.health) && std::isfinite(observed.damage), "Monster stats are finite and positive");
                if (profile.health > 0) check(profile.health == observed.health && profile.damage == observed.damage, "Monster stats depend on mode and encounter, not hero or build");
                profile = observed;
            }
            std::size_t choice = 0;
            if (game.room() == Room::Library && !(reputation && game.floorNumber() == 3)) choice = hero == Hero::Mage ? 2 : hero == Hero::Warrior ? 0 : 1;
            if (game.room() == Room::Encounter) choice = reputation ? 0 : 1;
            if (game.room() == Room::Respite) choice = reputation && game.floorNumber() == 1 ? 0 : game.player().stats[Health] < 45.f * game.floorNumber() ? 0 : 1;
            if (game.room() == Room::Shop) {
                if (game.floorNumber() == 1) choice = reputation ? 2 : 1;
                else if (game.floorNumber() == 3) choice = 2;
                else if (game.biome() == Biome::Ghost) choice = 1;
                else choice = hero == Hero::Mage ? 2 : hero == Hero::Warrior ? 1 : 0;
            }
            game.choose(choice);
            break;
        }
        case Phase::Combat: {
            const float before = game.player().stats[Health], damage = game.enemyDamage();
            const int strikes = game.strikesLeft();
            game.attack();
            if (game.enemyHealth() == 0) check(game.player().stats[Health] >= before, "A killing blow prevents retaliation on every mode");
            else if (strikes > 1 || game.room() == Room::Boss) {
                check(std::abs(before - game.player().stats[Health] - std::min(before, damage)) < .001f, "Counterattacks apply the configured damage");
                ++retaliations;
            }
            break;
        }
        case Phase::Riddle:
            check(game.timeLimit() == 25 - 5 * game.floorNumber() + std::floor(game.player().stats[Wisdom] / 2), "Combat difficulty does not change riddle timers");
            game.answer(game.riddle().correct);
            break;
        case Phase::Complete: {
            const auto player = game.player();
            check(game.descend(game.floorNumber() == 1 ? second : Biome::Lava), "Guardian victory opens the next floor");
            check(game.player().stats == player.stats && game.player().reputation == player.reputation && game.difficulty() == difficulty, "A floor transition preserves the player and difficulty");
            break;
        }
        default: game.advance(); break;
        }
    }
    check(!game.inProgress(), "A simulated descent terminates");
    return game.phase() == Phase::Ending && game.ending() != Ending::Bad;
}
}
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Pass the assets directory");
        DungeonRun game(argv[1]);
        unsigned chestSeed = 0;
        do { game.start(Hero::Mage, Biome::Ice, chestSeed++); } while (game.room() != Room::Chest && chestSeed < 100);
        check(game.room() == Room::Chest, "Find an opening chest for non-combat checks");
        std::array<float, 6> chestStats{};
        Riddle question;
        float timer = 0;
        for (int level = 0; level < 4; ++level) {
            game.start(Hero::Mage, Biome::Ice, chestSeed - 1, static_cast<Difficulty>(level));
            check(game.player().stats == DungeonRun::makePlayer(Hero::Mage).stats, "Modes do not change starting hero stats or abilities");
            game.choose(0);
            if (level == 0) { question = game.riddle(); timer = game.timeLimit(); }
            else check(game.riddle().question == question.question && game.riddle().answers == question.answers && game.riddle().correct == question.correct && game.timeLimit() == timer,
                       "Changing difficulty does not reroll a non-combat question or its timer");
            game.answer(game.riddle().correct); game.advance(); game.advance();
            if (level == 0) chestStats = game.player().stats;
            else check(game.player().stats == chestStats && game.player().riddlesCorrect == 1, "Riddle rewards, loot and class multipliers remain identical");
        }
        const auto beforeInvalid = game.player();
        bool rejected = false;
        try { game.start(Hero::Warrior, Biome::Ice, 7, static_cast<Difficulty>(99)); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected && game.player().stats == beforeInvalid.stats && game.difficulty() == Difficulty::Impossible, "Invalid difficulty is rejected before changing a run");

        Profiles profiles{};
        std::array<int, 4> wins{};
        std::array<std::array<int, 4>, 4> heroWins{};
        int retaliations = 0;
        for (int level = 0; level < 4; ++level) for (int hero = 0; hero < 4; ++hero)
            for (int first = 0; first < 2; ++first) for (int second = 2; second < 4; ++second) for (unsigned seed = 0; seed < 128; ++seed)
                for (bool reputation : {false, true}) {
                    if (play(game, static_cast<Difficulty>(level), static_cast<Hero>(hero), static_cast<Biome>(first), static_cast<Biome>(second), seed, reputation, profiles, retaliations)) {
                        ++wins[level]; ++heroWins[level][hero];
                    }
                }
        for (int level = 0; level < 4; ++level) {
            check(wins[level] > 0, "Every difficulty has genuine winning runs");
            for (int encounter = 0; encounter < 9; ++encounter) {
                check(profiles[level][encounter].health > 0, "Exercise every floor and combat role on each difficulty");
                if (level > 0) check(profiles[level][encounter].health > profiles[level - 1][encounter].health && profiles[level][encounter].damage >= profiles[level - 1][encounter].damage,
                                     "Higher difficulties increase health and never reduce counterattack damage");
            }
            if (level > 0) check(wins[level] < wins[level - 1], "Seeded full-battle runs get harder at each tier");
            if (level < 3) for (int hero = 0; hero < 4; ++hero) check(heroWins[level][hero] > 0, "All heroes can finish Easy, Medium and Hard");
            std::cout << DungeonRun::difficultyName(static_cast<Difficulty>(level)) << ": " << wins[level] << " / 4096 full-battle runs won\n";
        }
        check(retaliations > 1000, "Exercise actual enemy damage across the tiers");
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
