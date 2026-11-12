#include "DungeonRun.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>

using namespace dungeon;
namespace {
enum class Style { Power, Reputation, Survival };
struct Totals {
    int runs = 0, first = 0, second = 0, wins = 0, allBattles = 0, perfect = 0;
    int malvel = 0;
    std::array<int, 3> bosses{}, attacks{};
    double healthAtMalvel = 0, healthAfterWin = 0;
};
std::size_t choose(const DungeonRun& run, Style style) {
    const auto hero = run.player().hero;
    const int floor = run.floorNumber();
    const bool reputation = style == Style::Reputation;
    switch (run.room()) {
    case Room::Library:
        if (reputation && floor == 3) return 0;
        return hero == Hero::Mage ? 2 : hero == Hero::Warrior ? 0 : 1;
    case Room::Encounter: return reputation ? 0 : 1;
    case Room::Respite:
        if (reputation && floor == 1) return 0;
        return run.player().stats[Health] < 45.f * floor ? 0 : 1;
    case Room::Shop:
        if (floor == 1) return reputation ? 2 : 1;
        if (floor == 3) return 2;
        if (run.biome() == Biome::Ghost) return 1;
        return hero == Hero::Mage ? 2 : hero == Hero::Warrior ? 1 : 0;
    case Room::Battle:
        if (style == Style::Survival) {
            const auto primary = hero == Hero::Warrior ? Strength : hero == Hero::Ranger ? Accuracy : hero == Hero::Mage ? Wisdom : Agility;
            const float damage = std::max(1.f, run.player().stats[primary]) * (1 + std::min(35.f, run.player().stats[Accuracy]) / 35);
            const float hits = std::ceil(run.enemyMaxHealth() / damage);
            const float available = 3 + std::floor(run.player().stats[Agility] / 3);
            if (hits > available || run.player().stats[Health] - (hits - 1) * run.enemyDamage() < 25.f * floor) return 1;
        }
        return 0;
    default: return 0;
    }
}
void play(DungeonRun& run, Difficulty difficulty, Hero hero, Biome first, Biome second,
          unsigned seed, int accuracy, Style style, Totals& totals) {
    run.start(hero, first, seed, difficulty);
    std::mt19937 answers(seed ^ (0x85ebca6bu * (1 + static_cast<unsigned>(hero))) ^ (0xc2b2ae35u * static_cast<unsigned>(second)));
    ++totals.runs;
    for (int step = 0; step < 1500 && run.inProgress(); ++step) {
        switch (run.phase()) {
        case Phase::Room:
            run.choose(choose(run, style));
            if (run.room() == Room::Boss && run.phase() == Phase::Combat) {
                ++totals.bosses[run.floorNumber() - 1];
                if (run.floorNumber() == 3) { ++totals.malvel; totals.healthAtMalvel += run.player().stats[Health]; }
            }
            break;
        case Phase::Combat:
            if (run.room() == Room::Boss) ++totals.attacks[run.floorNumber() - 1];
            run.attack();
            break;
        case Phase::Riddle:
            if (std::uniform_int_distribution<int>(0, 99)(answers) < accuracy) run.answer(run.riddle().correct);
            else for (std::size_t i = 0; i < 3; ++i) if (i != run.riddle().correct && run.answerAvailable(i)) { run.answer(i); break; }
            break;
        case Phase::Complete:
            if (run.floorNumber() == 1) ++totals.first;
            else ++totals.second;
            if (!run.descend(run.floorNumber() == 1 ? second : Biome::Lava)) throw std::runtime_error("Blocked valid descent");
            break;
        default: run.advance(); break;
        }
    }
    if (run.inProgress()) throw std::runtime_error("Simulation exceeded its step budget");
    if (run.phase() == Phase::Ending && run.ending() != Ending::Bad) {
        ++totals.wins;
        if (run.player().battlesWon == 9) ++totals.allBattles;
        if (run.ending() == Ending::Perfect) ++totals.perfect;
        totals.healthAfterWin += run.player().stats[Health];
    }
}
double mean(double value, int count) { return count == 0 ? 0 : value / count; }
double percent(int value, int count) { return 100 * mean(value, count); }
}
int main(int argc, char** argv) {
    try {
        if (argc < 2 || argc > 4) throw std::runtime_error("Usage: dungeon_balance ASSETS [SEEDS_PER_ROUTE=500] [RIDDLE_ACCURACY=100]");
        const int seeds = argc >= 3 ? std::stoi(argv[2]) : 500;
        const int accuracy = argc >= 4 ? std::stoi(argv[3]) : 100;
        if (seeds < 1 || seeds > 100000 || accuracy < 0 || accuracy > 100) throw std::runtime_error("Invalid seed count or answer accuracy");
        DungeonRun run(argv[1]);
        std::cout << "difficulty,hero,style,accuracy,seeds_per_route,runs,floor1_pct,floor2_pct,win_pct,all9_pct,perfect_pct,boss1_attacks,boss2_attacks,boss3_attacks,malvel_entry_hp,win_hp\n";
        std::cout << std::fixed << std::setprecision(2);
        for (int difficulty = 0; difficulty < 4; ++difficulty) for (int hero = 0; hero < 4; ++hero) for (int style = 0; style < 3; ++style) {
            Totals totals;
            for (int first = 0; first < 2; ++first) for (int second = 2; second < 4; ++second)
                for (unsigned seed = 0; seed < static_cast<unsigned>(seeds); ++seed)
                    play(run, static_cast<Difficulty>(difficulty), static_cast<Hero>(hero), static_cast<Biome>(first), static_cast<Biome>(second), seed, accuracy, static_cast<Style>(style), totals);
            std::cout << DungeonRun::difficultyName(static_cast<Difficulty>(difficulty)) << ',' << DungeonRun::heroName(static_cast<Hero>(hero)) << ','
                      << std::array<const char*, 3>{"power", "reputation", "survival"}[style] << ',' << accuracy << ',' << seeds << ',' << totals.runs << ','
                      << percent(totals.first, totals.runs) << ',' << percent(totals.second, totals.runs) << ',' << percent(totals.wins, totals.runs) << ','
                      << percent(totals.allBattles, totals.runs) << ',' << percent(totals.perfect, totals.runs) << ','
                      << mean(totals.attacks[0], totals.bosses[0]) << ',' << mean(totals.attacks[1], totals.bosses[1]) << ',' << mean(totals.attacks[2], totals.bosses[2]) << ','
                      << mean(totals.healthAtMalvel, totals.malvel) << ',' << mean(totals.healthAfterWin, totals.wins) << '\n';
        }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
