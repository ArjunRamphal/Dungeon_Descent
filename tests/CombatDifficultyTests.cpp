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
            game.choose(game.displayChoiceFor(choice));
            break;
        }
        case Phase::Combat: {
            const float before = game.player().stats[Health];
            CombatAction action = CombatAction::StandardStrike;
            const auto intent = game.bossIntent();
            if (intent.telegraphed && (intent.counterMultiplier > 1.f || intent.drainsStrike)) {
                action = CombatAction::GuardStrike;
            } else if (intent.telegraphed && (intent.shattersOnPierce || (intent.heavyPierces && intent.damageTakenMultiplier < 1.f)) &&
                       !intent.heavyBacklash && game.strikesLeft() >= 2) {
                action = CombatAction::HeavyStrike;
            } else if (intent.appliesVenom && game.playerVenomStacks() >= 2) {
                action = CombatAction::GuardStrike;
            } else if (game.difficulty() == Difficulty::Impossible && game.room() == Room::Boss && game.classAbilityAvailable()) {
                action = CombatAction::ClassAbility;
            }
            const auto preview = game.previewCombatAction(action);
            const int strikes = game.strikesLeft();
            game.attack(action);
            if (game.enemyHealth() == 0) check(game.player().stats[Health] >= before, "A killing blow prevents retaliation on every mode");
            else if (strikes > preview.strikeCost || game.room() == Room::Boss) {
                const float healBoost = (action == CombatAction::ClassAbility && game.player().hero == Hero::Mage)
                    ? (3.f + static_cast<float>(game.floorNumber())) : 0.f;
                const float expectedHp = std::max(0.f, before + healBoost - preview.expectedCounter);
                check(std::abs(game.player().stats[Health] - expectedHp) < .001f,
                      "Counterattacks apply the exact previewed damage including boss intents and guard mitigation");
                if (preview.expectedCounter > 0.f) ++retaliations;
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

        // Verify tactical combat options improve difficult encounters and respect constraints.
        int tacticalHardWins = 0;
        for (int hero = 0; hero < 4; ++hero) {
            for (unsigned seed = 0; seed < 16; ++seed) {
                game.start(static_cast<Hero>(hero), Biome::Ice, seed, Difficulty::Hard);
                while (game.inProgress()) {
                    if (game.phase() == Phase::Complete) {
                        game.descend(game.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
                    } else if (game.phase() == Phase::Room) {
                        std::size_t choice = game.room() == Room::Library ? (hero == 2 ? 2 : hero == 0 ? 0 : 1) : 0;
                        if (game.room() == Room::Shop) choice = game.floorNumber() == 3 ? 2 : 1;
                        if (game.room() == Room::Respite) choice = 1;
                        if (game.room() == Room::Encounter) choice = 1;
                        game.choose(game.displayChoiceFor(choice));
                    } else if (game.phase() == Phase::Combat) {
                        if (game.classAbilityAvailable()) game.choose(3);
                        else if (game.combatChoiceAvailable(1) && game.enemyHealth() <= game.combatSummary().heavyDamage) game.choose(1);
                        else if (game.player().stats[Health] <= game.enemyDamage() && game.enemyHealth() > game.combatSummary().baseDamage) game.choose(2);
                        else game.choose(0);
                    } else if (game.phase() == Phase::Riddle) {
                        game.answer(game.riddle().correct);
                    } else {
                        game.advance();
                    }
                }
                if (game.phase() == Phase::Ending && game.ending() != Ending::Bad) ++tacticalHardWins;
            }
        }
        check(tacticalHardWins >= 48, "Tactical combat enables strong Hard-difficulty clears across all heroes");

        // Verify all 5 unique boss intents and live tactical combat previews.
        auto reachBoss = [&](Biome targetBiome) {
            const Biome floor1 = targetBiome == Biome::Jungle ? Biome::Jungle : Biome::Ice;
            game.start(Hero::Warrior, floor1, 7, Difficulty::Hard);
            while (game.inProgress()) {
                if (game.biome() == targetBiome && game.room() == Room::Boss && game.phase() == Phase::Combat) return;
                if (game.phase() == Phase::Complete) {
                    const Biome floor2 = targetBiome == Biome::Desert ? Biome::Desert : Biome::Ghost;
                    game.descend(game.floorNumber() == 1 ? floor2 : Biome::Lava);
                } else if (game.phase() == Phase::Room) {
                    std::size_t choice = 0;
                    if (game.room() == Room::Shop) choice = game.floorNumber() == 3 ? 2 : 1;
                    if (game.room() == Room::Respite) choice = 1;
                    if (game.room() == Room::Encounter) choice = 1;
                    game.choose(game.displayChoiceFor(choice));
                } else if (game.phase() == Phase::Combat) {
                    if (game.classAbilityAvailable()) game.choose(3);
                    else game.choose(0);
                } else if (game.phase() == Phase::Riddle) {
                    game.answer(game.riddle().correct);
                } else {
                    game.advance();
                }
            }
            throw std::runtime_error("Failed to reach boss for biome");
        };

        // 1. The Yeti (Ice): Glacial Slam on turn 3 deals 2x counterattack.
        reachBoss(Biome::Ice);
        check(game.bossIntent().name == "Gathering Frost" && !game.bossIntent().telegraphed, "Yeti turn 1 is normal strike");
        game.choose(0); // Turn 1 -> Turn 2
        game.choose(0); // Turn 2 -> Turn 3 (Glacial Slam)
        check(game.phase() == Phase::Combat && game.bossIntent().telegraphed && game.bossIntent().name == "Glacial Slam" &&
              std::abs(game.bossIntent().counterMultiplier - 2.f) < .001f, "Yeti telegraphs Glacial Slam (2x counter) on turn 3");
        {
            const auto stdPrev = game.previewCombatAction(CombatAction::StandardStrike);
            const auto guardPrev = game.previewCombatAction(CombatAction::GuardStrike);
            check(std::abs(stdPrev.expectedCounter - game.enemyDamage() * 2.f) < .001f &&
                  std::abs(guardPrev.expectedCounter - game.enemyDamage() * 2.f * 0.25f) < .001f,
                  "Live combat preview reflects Glacial Slam and Guard reduction");
        }

        // 2. Jungle Warden (Jungle): Thorned Carapace on turn 3 (1.6x unguarded, Guard bypasses thorns).
        reachBoss(Biome::Jungle);
        game.choose(0); game.choose(0);
        check(game.phase() == Phase::Combat && game.bossIntent().telegraphed && game.bossIntent().name == "Thorned Carapace" &&
              game.bossIntent().guardBypasses, "Jungle Warden telegraphs Thorned Carapace on turn 3");
        {
            const auto stdPrev = game.previewCombatAction(CombatAction::StandardStrike);
            const auto guardPrev = game.previewCombatAction(CombatAction::GuardStrike);
            check(std::abs(stdPrev.expectedCounter - game.enemyDamage() * 1.6f) < .001f &&
                  std::abs(guardPrev.expectedCounter - game.enemyDamage() * 0.25f) < .001f,
                  "Guard Strike bypasses Thorned Carapace multiplier in live preview");
        }

        // 3. Ahmose the Pharaoh (Desert): Curse of the Sands on turn 3 (1.75x + drains 1 strike unless guarded).
        reachBoss(Biome::Desert);
        game.choose(0); game.choose(0);
        check(game.phase() == Phase::Combat && game.bossIntent().telegraphed && game.bossIntent().name == "Curse of the Sands" &&
              game.bossIntent().drainsStrike, "Ahmose telegraphs Curse of the Sands on turn 3");
        {
            const int strikesBefore = game.strikesLeft();
            game.choose(0); // Unguarded hit triggers strike drain
            if (game.phase() == Phase::Combat) {
                check(game.lastCombatExchange().strikesDrained == 1 && game.strikesLeft() == strikesBefore - 2,
                      "Unguarded strike against Curse of the Sands drains 1 extra strike");
            }
        }

        // 4. Count Dracula (Ghost): Blood Siphon on turn 3 (1.5x counter + heals 2x damage dealt).
        reachBoss(Biome::Ghost);
        game.choose(0); game.choose(0);
        check(game.phase() == Phase::Combat && game.bossIntent().telegraphed && game.bossIntent().name == "Blood Siphon" &&
              std::abs(game.bossIntent().lifestealRatio - 2.f) < .001f, "Dracula telegraphs Blood Siphon on turn 3");
        {
            game.choose(0);
            if (game.phase() == Phase::Combat) {
                check(game.lastCombatExchange().enemyHealed > 0.f, "Dracula Blood Siphon heals enemy HP on unguarded counter");
            }
        }

        // 5. Malvel (Lava): Arcane Ward on turn 2 (50% dmg reduction, pierced by Heavy/Class) + Obsidian Cataclysm on turn 3 (2.25x).
        reachBoss(Biome::Lava);
        game.choose(0); // Turn 1 -> Turn 2 (Arcane Ward)
        check(game.phase() == Phase::Combat && game.bossIntent().telegraphed && game.bossIntent().name == "Arcane Ward" &&
              std::abs(game.bossIntent().damageTakenMultiplier - 0.5f) < .001f && game.bossIntent().heavyPierces,
              "Malvel activates Arcane Ward on turn 2");
        {
            const auto stdPrev = game.previewCombatAction(CombatAction::StandardStrike);
            const auto heavyPrev = game.previewCombatAction(CombatAction::HeavyStrike);
            check(heavyPrev.minDamage > stdPrev.minDamage * 3.f,
                  "Heavy Strike pierces Malvel's Arcane Ward while Standard Strike is halved");
        }
        game.choose(0); // Turn 2 -> Turn 3 (Obsidian Cataclysm)
        check(game.phase() == Phase::Combat && game.bossIntent().telegraphed && game.bossIntent().name == "Obsidian Cataclysm" &&
              std::abs(game.bossIntent().counterMultiplier - 2.25f) < .001f,
              "Malvel telegraphs Obsidian Cataclysm (2.25x counter) on turn 3");

        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
