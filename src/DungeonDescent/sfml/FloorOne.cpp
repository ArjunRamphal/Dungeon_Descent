#include "FloorOne.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace dungeon {
namespace {
std::string number(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}
}

std::string FloorOne::readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Missing story file: " + path.string());
    std::ostringstream out;
    out << file.rdbuf();
    std::string text = out.str();
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);
    return text;
}

FloorOne::FloorOne(const std::filesystem::path& assets) : assets_(assets) {
    std::istringstream input(readText(assets / "textfiles/riddles/1/EasyRiddles.txt"));
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        Riddle r;
        r.question = line;
        for (auto& answer : r.answers) {
            if (!std::getline(input, answer) || answer.empty())
                throw std::runtime_error("Incomplete four-line riddle in EasyRiddles.txt");
        }
        riddles_.push_back(std::move(r)); // First answer in the source file is correct.
    }
    if (riddles_.size() < 8) throw std::runtime_error("Floor 1 needs at least eight riddles.");
    sourceRiddles_ = riddles_;
}

Player FloorOne::makePlayer(Hero hero) {
    Player p;
    p.hero = hero;
    switch (hero) {
    case Hero::Warrior: p.stats = {7, 1, 0, 3, 35, 2}; break;
    case Hero::Ranger: p.stats = {2, 2, 4, 2, 30, 8}; break;
    case Hero::Mage: p.stats = {1, 7, 3, 1, 40, 1}; break;
    case Hero::Rogue: p.stats = {2, 0, 4, 8, 30, 5}; break;
    }
    return p; // Includes the original automatic +3 class ability, applied once.
}

void FloorOne::start(Hero hero, Biome biome, std::uint32_t seed) {
    rng_.seed(seed);
    riddles_ = sourceRiddles_;
    player_ = makePlayer(hero);
    biome_ = biome;
    roomIndex_ = 0;
    rooms_ = {Room::Library, Room::Chest, Room::Battle, Room::Encounter,
              Room::Battle, Room::Respite, Room::Library, Room::Shop, Room::Boss};
    std::shuffle(rooms_.begin(), rooms_.begin() + 3, rng_);
    riddleOrder_.resize(riddles_.size());
    std::iota(riddleOrder_.begin(), riddleOrder_.end(), 0);
    std::shuffle(riddleOrder_.begin(), riddleOrder_.end(), rng_);
    enterRoom();
}

void FloorOne::enterRoom() {
    checkpoint_ = player_;
    phase_ = Phase::Room;
    chestUnlocked_ = false;
    enemyHealth_ = 0;
    strikesLeft_ = 0;
    timeLeft_ = timeLimit_ = 0;
    message_.clear();
}

void FloorOne::addStat(Stat stat, float amount) {
    const auto hero = player_.hero;
    const bool bonus = hero == Hero::Mage ||
        (hero == Hero::Warrior && stat == Strength) ||
        (hero == Hero::Ranger && stat == Accuracy) ||
        (hero == Hero::Rogue && stat == Agility);
    player_.stats[stat] += amount * (bonus ? 1.5f : 1.f);
}

void FloorOne::addAll(float amount) {
    for (std::size_t s = 0; s < player_.stats.size(); ++s) {
        float multiplier = 1;
        if (player_.hero == Hero::Mage) multiplier = s == Health ? 3.f : 1.5f;
        else if (s == Health || (player_.hero == Hero::Warrior && s == Strength) ||
                 (player_.hero == Hero::Ranger && s == Accuracy) ||
                 (player_.hero == Hero::Rogue && s == Agility)) multiplier = 1.5f;
        player_.stats[s] += amount * multiplier;
    }
}

void FloorOne::subtractAll(float amount) {
    for (auto& stat : player_.stats) stat = std::max(0.f, stat - amount);
}

void FloorOne::outcome(std::string text) {
    message_ = std::move(text);
    phase_ = player_.stats[Health] <= 0 ? Phase::Defeat : Phase::Outcome;
}

std::vector<std::string> FloorOne::choices() const {
    if (phase_ != Phase::Room) return {};
    switch (room()) {
    case Room::Library: return {"The Art of Warfare  /  Strength +2", "The Whispering Winds  /  Agility +2",
                               "The Ancient Lore  /  Wisdom +2", "The Heart of the Forest  /  Observation +2"};
    case Room::Chest: return {"Read the seal's riddle"};
    case Room::Battle: return {"Confront the creature", "Sneak past"};
    case Room::Encounter: return {"Free the warrior  /  Reputation +1", "Take his vitality  /  All stats +1"};
    case Room::Respite: return {"Drink from the spring  /  Health +10", "Search the room  /  All stats +2"};
    case Room::Shop: return {"Health Potion  /  Health +5", "Stat Increase Potion  /  All stats +1", "Book of Knowledge  /  Reputation +1"};
    case Room::Boss: return {"Face the guardian"};
    }
    return {};
}

void FloorOne::choose(std::size_t choice) {
    if (phase_ != Phase::Room || choice >= choices().size()) return;
    switch (room()) {
    case Room::Chest: beginRiddle(); return;
    case Room::Library: {
        const std::array<Stat, 4> stats = {Strength, Agility, Wisdom, Observation};
        const auto stat = stats[choice];
        const float before = player_.stats[stat];
        addStat(stat, 2);
        outcome("You study the book. " + statName(stat) + " increased by " + number(player_.stats[stat] - before) + ".");
        return;
    }
    case Room::Encounter:
        if (choice == 0) {
            ++player_.reputation;
            outcome(readText(assets_ / ("textfiles/biomes/" + biomeKey() + "/encounter/" + biomeKey() + "prisonfree.txt")) + "\n\nReputation +1.");
        } else {
            addAll(1);
            outcome("You take the warrior's vitality. All stats increase by 1, with your class bonuses.");
        }
        return;
    case Room::Respite:
        if (choice == 0) {
            addStat(Health, 10);
            ++player_.reputation;
            outcome("The spring restores your health. Reputation +1.\n\n" + readText(assets_ / "textfiles/respitedrink.txt"));
        } else {
            addAll(2);
            outcome("You uncover supplies among the ruins. All stats increase by 2, with your class bonuses.");
        }
        return;
    case Room::Shop:
        if (choice == 0) { addStat(Health, 5); outcome("Surtees offers a health potion. Your health increases, with your class bonus."); }
        else if (choice == 1) { addAll(1); outcome("The potion strengthens you. All stats increase by 1, with your class bonuses."); }
        else { ++player_.reputation; outcome(readText(assets_ / "textfiles/shopkeeperbook.txt") + "\n\nReputation +1."); }
        return;
    case Room::Battle:
    case Room::Boss:
        if (choice == 1) { outcome("You slip past unnoticed. You gain no battle reward, but the riddle keeper still awaits."); return; }
        ++player_.battlesFought;
        enemyHealth_ = enemyMaxHealth();
        strikesLeft_ = std::uniform_int_distribution<int>(3, 5)(rng_) + static_cast<int>(player_.stats[Agility] / 3);
        phase_ = Phase::Combat;
        message_ = "Time your strikes. A defeated enemy cannot strike back. Your class's primary stat determines damage.";
        return;
    }
}

void FloorOne::beginRiddle() {
    auto& r = riddles_.at(riddleOrder_.at(roomIndex_));
    const std::string correct = r.answers.at(r.correct);
    std::shuffle(r.answers.begin(), r.answers.end(), rng_);
    r.correct = static_cast<std::size_t>(std::find(r.answers.begin(), r.answers.end(), correct) - r.answers.begin());
    timeLimit_ = 20.f + std::floor(player_.stats[Wisdom] / 2);
    timeLeft_ = timeLimit_;
    phase_ = Phase::Riddle;
    message_.clear();
}

void FloorOne::resolveRiddle(bool correct, bool timeout) {
    if (phase_ != Phase::Riddle) return;
    ++player_.riddlesAnswered;
    if (correct) {
        ++player_.riddlesCorrect;
        addAll(.75f);
        chestUnlocked_ = room() == Room::Chest;
        message_ = "Correct. All stats increase by 0.75, with your class bonuses.";
        if (chestUnlocked_) message_ += "\nThe chest is unlocked. Claim it on the next screen.";
    } else {
        subtractAll(timeout ? .5f : 1.5f);
        message_ = timeout ? "Time has run out. All stats decrease by 0.5." : "Incorrect. All stats decrease by 1.5.";
        message_ += "\nThe answer was: " + riddle().answers.at(riddle().correct) + ".";
        if (room() == Room::Chest) message_ += "\nThe chest remains locked.";
    }
    timeLeft_ = 0;
    phase_ = player_.stats[Health] <= 0 ? Phase::Defeat : Phase::RiddleOutcome;
}

void FloorOne::answer(std::size_t choice) {
    if (phase_ == Phase::Riddle && choice < riddle().answers.size()) resolveRiddle(choice == riddle().correct, false);
}

void FloorOne::tick(float seconds) {
    if (phase_ != Phase::Riddle || !std::isfinite(seconds) || seconds <= 0) return;
    timeLeft_ = std::max(0.f, timeLeft_ - seconds);
    if (timeLeft_ <= 0) resolveRiddle(false, true);
}

void FloorOne::attack() {
    if (phase_ != Phase::Combat || strikesLeft_ <= 0) return;
    --strikesLeft_;
    Stat primary = Strength;
    if (player_.hero == Hero::Ranger) primary = Accuracy;
    else if (player_.hero == Hero::Mage) primary = Wisdom;
    else if (player_.hero == Hero::Rogue) primary = Agility;
    const bool critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
    const float damage = std::max(1.f, player_.stats[primary]) * (critical ? 2.f : 1.f);
    enemyHealth_ = std::max(0.f, enemyHealth_ - damage);
    message_ = std::string(critical ? "Critical hit! " : "You strike. ") + number(damage) + " damage.";
    if (enemyHealth_ <= 0) {
        ++player_.battlesWon;
        addAll(1);
        message_ += "\n" + enemyName() + " defeated. All stats increase by 1, with your class bonuses.";
        phase_ = room() == Room::Boss ? Phase::Complete : Phase::Outcome;
        return;
    }
    const float retaliation = room() == Room::Boss ? 5.f : (roomNumber() <= 3 ? 3.f : 4.f);
    player_.stats[Health] = std::max(0.f, player_.stats[Health] - retaliation);
    message_ += "\nThe enemy counters for " + number(retaliation) + " health.";
    if (player_.stats[Health] <= 0) {
        phase_ = Phase::Defeat;
        message_ += "\nYour strength gives out. Retry this room or begin a new descent.";
    } else if (strikesLeft_ == 0) {
        if (room() == Room::Boss) {
            phase_ = Phase::Defeat;
            message_ += "\nYour strikes are exhausted. The guardian still blocks the stairway.";
        } else {
            subtractAll(.75f);
            outcome(message_ + "\nYou retreat. All stats decrease by 0.75. The riddle keeper awaits.");
        }
    }
}

void FloorOne::advance() {
    switch (phase_) {
    case Phase::Outcome: beginRiddle(); return;
    case Phase::RiddleOutcome:
        if (chestUnlocked_) { phase_ = Phase::Loot; return; }
        break;
    case Phase::Loot:
        addAll(2);
        chestUnlocked_ = false;
        message_ = "You claim the chest's treasure. All stats increase by 2, with your class bonuses.";
        phase_ = Phase::RiddleOutcome;
        return;
    default: return;
    }
    if (roomIndex_ + 1 < rooms_.size()) { ++roomIndex_; enterRoom(); }
}

void FloorOne::retryRoom() {
    if (phase_ != Phase::Defeat) return;
    player_ = checkpoint_;
    enterRoom();
}

std::string FloorOne::heroName(Hero hero) {
    return std::array<std::string, 4>{"Warrior", "Ranger", "Mage", "Rogue"}.at(static_cast<std::size_t>(hero));
}
std::string FloorOne::portrait(Hero hero) {
    return "images/pfps/" + heroName(hero) + ((hero == Hero::Mage || hero == Hero::Ranger) ? ".jpeg" : ".jpg");
}
std::string FloorOne::ability(Hero hero) {
    switch (hero) {
    case Hero::Warrior: return "Runic might / +3 Strength\n1.5x Strength and Health rewards";
    case Hero::Ranger: return "Steady aim / +3 Accuracy\n1.5x Accuracy and Health rewards";
    case Hero::Mage: return "Arcane insight / +3 Wisdom\n1.5x stat and 3x Health rewards";
    case Hero::Rogue: return "Shadow step / +3 Agility\n1.5x Agility and Health rewards";
    }
    return {};
}
std::string FloorOne::statName(std::size_t stat) {
    return std::array<std::string, 6>{"Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy"}.at(stat);
}
std::string FloorOne::roomName(Room room) {
    return std::array<std::string, 7>{"Library", "Sealed chest", "Battle", "The prisoner", "Respite", "Surtees' shop", "The guardian"}.at(static_cast<std::size_t>(room));
}
std::string FloorOne::biomeName(Biome biome) { return biome == Biome::Ice ? "The Frozen Depths" : "The Overgrown Halls"; }
std::string FloorOne::biomeKey() const { return biome_ == Biome::Ice ? "ice" : "jungle"; }
std::string FloorOne::enemyName() const {
    if (room() == Room::Boss) return biome_ == Biome::Ice ? "The Yeti" : "The Jungle Warden";
    return biome_ == Biome::Ice ? "Frost creature" : "Jungle creature";
}
std::string FloorOne::title() const { return room() == Room::Boss ? enemyName() : roomName(room()); }
std::string FloorOne::image() const {
    const auto key = biomeKey();
    std::string suffix;
    switch (room()) {
    case Room::Library: suffix = "library/" + key + "library.png"; break;
    case Room::Chest: suffix = "chest/" + key + "chest.png"; break;
    case Room::Battle: suffix = "battle/" + key + (roomNumber() == 5 ? "battle2.jpg" : "battle.jpg"); break;
    case Room::Boss: suffix = "battle/" + key + "boss.jpg"; break;
    case Room::Encounter: return "images/" + std::string(biome_ == Biome::Ice ? "biomes/ice/encounter/iceencounter.png" : "prison.jpeg");
    case Room::Respite: suffix = "respite/" + key + "respite.png"; break;
    case Room::Shop: suffix = "shop/" + key + "shop.png"; break;
    }
    return "images/biomes/" + key + "/" + suffix;
}
std::string FloorOne::storyFile() const {
    const auto key = biomeKey();
    std::string suffix;
    switch (room()) {
    case Room::Library: suffix = "library/" + key + "library.txt"; break;
    case Room::Chest: suffix = "chest/" + key + "chest.txt"; break;
    case Room::Battle: suffix = "battle/" + key + (roomNumber() == 5 ? "battle2.txt" : "battle.txt"); break;
    case Room::Boss: suffix = "battle/" + key + "boss.txt"; break;
    case Room::Encounter: suffix = "encounter/" + key + "prison.txt"; break;
    case Room::Respite: suffix = "respite/" + key + "respite.txt"; break;
    case Room::Shop: return "textfiles/shopkeeper.txt";
    }
    return "textfiles/biomes/" + key + "/" + suffix;
}
} // namespace dungeon
