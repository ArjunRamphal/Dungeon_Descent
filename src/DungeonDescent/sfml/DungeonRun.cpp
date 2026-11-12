#include "DungeonRun.h"

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
struct EnemyStats { float health, damage; };
// Difficulty, floor, then opening battle / later battle / guardian.
// Easy is the original game. Each encounter has its own health/damage budget;
// opening damage stays low while guardians test both health and strike limits.
constexpr EnemyStats EnemyBalance[4][3][3] = {
    {{{32, 3}, {32, 4}, {144, 5}}, {{68, 6}, {68, 8}, {306, 10}}, {{126, 9}, {126, 12}, {567, 15}}},
    {{{36, 3}, {48, 4.25f}, {190, 5.25f}}, {{100, 6.25f}, {145, 8.25f}, {480, 10.5f}}, {{220, 9.25f}, {300, 12.25f}, {1000, 15.5f}}},
    {{{48, 3}, {60, 4.5f}, {230, 5.25f}}, {{125, 6.5f}, {175, 8.5f}, {550, 10.5f}}, {{280, 9.5f}, {350, 12.5f}, {1200, 15.75f}}},
    {{{56, 3}, {70, 4.75f}, {450, 5.5f}}, {{145, 6.75f}, {210, 8.75f}, {620, 10.75f}}, {{330, 9.75f}, {420, 12.75f}, {1450, 16}}}
};
std::string number(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << value;
    auto result = out.str();
    while (!result.empty() && result.back() == '0') result.pop_back();
    if (!result.empty() && result.back() == '.') result.pop_back();
    return result;
}
}

std::string DungeonRun::readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Missing story file: " + path.string());
    std::ostringstream out;
    out << file.rdbuf();
    std::string text = out.str();
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);
    return text;
}

DungeonRun::DungeonRun(const std::filesystem::path& assets) : assets_(assets) {
    const std::array<std::string, 3> banks = {"1/EasyRiddles.txt", "2/MediumRiddles.txt", "3/HardRiddles.txt"};
    for (std::size_t i = 0; i < banks.size(); ++i) {
        std::istringstream input(readText(assets / ("textfiles/riddles/" + banks[i])));
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) continue;
            Riddle r;
            r.question = line;
            for (auto& answer : r.answers) {
                if (!std::getline(input, answer) || answer.empty())
                    throw std::runtime_error("Incomplete four-line riddle in " + banks[i]);
            }
            sourceRiddles_[i].push_back(std::move(r));
        }
        if (sourceRiddles_[i].size() < 8) throw std::runtime_error("Each floor needs at least eight riddles.");
    }
    riddles_ = sourceRiddles_[0];
}

Player DungeonRun::makePlayer(Hero hero) {
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

void DungeonRun::start(Hero hero, Biome biome, std::uint32_t seed, Difficulty difficulty) {
    if (biome != Biome::Ice && biome != Biome::Jungle)
        throw std::invalid_argument("A new descent begins with the Ice or Jungle biome.");
    if (difficulty < Difficulty::Easy || difficulty > Difficulty::Impossible)
        throw std::invalid_argument("Unknown combat difficulty.");
    rng_.seed(seed);
    player_ = makePlayer(hero);
    difficulty_ = difficulty;
    firstBiome_ = biome_ = biome;
    floorNumber_ = 1;
    ending_ = Ending::None;
    started_ = true;
    prepareFloor();
}

bool DungeonRun::descend(Biome biome) {
    if (!canDescend()) return false;
    if (floorNumber_ == 1 && biome != Biome::Desert && biome != Biome::Ghost) return false;
    if (floorNumber_ == 2 && biome != Biome::Lava) return false;
    ++floorNumber_;
    biome_ = biome;
    prepareFloor(); // Carry the player forward; do not recreate the hero or ability.
    return true;
}

void DungeonRun::prepareFloor() {
    riddles_ = sourceRiddles_.at(static_cast<std::size_t>(floorNumber_ - 1));
    roomIndex_ = 0;
    if (floorNumber_ == 1)
        rooms_ = {Room::Library, Room::Chest, Room::Battle, Room::Encounter,
                  Room::Battle, Room::Respite, Room::Library, Room::Shop, Room::Boss};
    else if (floorNumber_ == 2)
        rooms_ = {Room::Library, Room::Chest, Room::Battle, Room::Shop,
                  Room::Chest, Room::Encounter, Room::Respite, Room::Battle, Room::Boss};
    else
        rooms_ = {Room::Library, Room::Chest, Room::Battle, Room::Chest,
                  Room::Battle, Room::Shop, Room::Encounter, Room::Respite, Room::Boss};
    std::shuffle(rooms_.begin(), rooms_.begin() + 3, rng_);
    riddleOrder_.resize(riddles_.size());
    std::iota(riddleOrder_.begin(), riddleOrder_.end(), 0);
    std::shuffle(riddleOrder_.begin(), riddleOrder_.end(), rng_);
    enterRoom();
}

void DungeonRun::enterRoom() {
    phase_ = Phase::Room;
    chestUnlocked_ = false;
    enemyHealth_ = 0;
    strikesLeft_ = 0;
    timeLeft_ = timeLimit_ = 0;
    hiddenAnswer_.reset();
    message_.clear();
}

void DungeonRun::addStat(Stat stat, float amount) {
    const auto hero = player_.hero;
    const bool bonus = hero == Hero::Mage ||
        (hero == Hero::Warrior && stat == Strength) ||
        (hero == Hero::Ranger && stat == Accuracy) ||
        (hero == Hero::Rogue && stat == Agility);
    player_.stats[stat] += amount * (bonus ? 1.5f : 1.f);
}

void DungeonRun::addAll(float amount) {
    for (std::size_t s = 0; s < player_.stats.size(); ++s) {
        float multiplier = 1;
        if (player_.hero == Hero::Mage) multiplier = s == Health ? 3.f : 1.5f;
        else if (s == Health || (player_.hero == Hero::Warrior && s == Strength) ||
                 (player_.hero == Hero::Ranger && s == Accuracy) ||
                 (player_.hero == Hero::Rogue && s == Agility)) multiplier = 1.5f;
        player_.stats[s] += amount * floorNumber_ * multiplier;
    }
}

void DungeonRun::subtractAll(float amount) {
    for (auto& stat : player_.stats) stat = std::max(0.f, stat - amount * floorNumber_);
}

void DungeonRun::outcome(std::string text) {
    message_ = std::move(text);
    phase_ = player_.stats[Health] <= 0 ? Phase::Defeat : Phase::Outcome;
}

std::vector<std::string> DungeonRun::choices() const {
    if (phase_ != Phase::Room) return {};
    switch (room()) {
    case Room::Library:
        if (biome_ == Biome::Lava)
            return {"The Princess Diary / Strength +2, Reputation +1", "The Smoldering Whisper / Agility +2",
                    "The Magma Script / Wisdom +2", "The Ashen Eye / Observation +2"};
        if (biome_ == Biome::Desert)
            return {"The Sands of Strategy / Strength +2", "The Silent Zephyrs / Agility +2",
                    "The Hidden Glyphs / Wisdom +2", "The Oasis Eye / Observation +2"};
        if (biome_ == Biome::Ghost)
            return {"The Cyclops Might / Strength +2", "The Weeping Wolfman / Agility +2",
                    "The Immortal Alucard / Wisdom +2", "The Eyes of the Raven / Observation +2"};
        return {"The Art of Warfare / Strength +2", "The Whispering Winds / Agility +2",
                "The Ancient Lore / Wisdom +2", "The Heart of the Forest / Observation +2"};
    case Room::Chest: return {"Read the seal's riddle"};
    case Room::Battle: return {"Confront the creature", "Sneak past"};
    case Room::Encounter:
        if (floorNumber_ == 3) return {"Threaten minions / Reputation +1", "Kill minions / All stats +6"};
        return floorNumber_ == 1 ? std::vector<std::string>{"Free the warrior / Reputation +1", "Take his vitality / All stats +1"}
                                : std::vector<std::string>{"Save the butler / Reputation +1", "Take the vial / All stats +3"};
    case Room::Respite: return {floorNumber_ == 3 ? "Rest in the sauna / Health +10" : "Drink from the spring / Health +10",
                               "Search the room / All stats +" + std::to_string(2 * floorNumber_)};
    case Room::Shop:
        if (biome_ == Biome::Lava)
            return {"Emberheart Elixir / Health +5", "Corebound Tincture / Strength +2", "Inferno Draught / All stats +6"};
        if (biome_ == Biome::Desert)
            return {"Draught of Revitalization / Health +5", "Essence of Fortitude / Strength +1.5", "Scroll of Ancient Wisdom / Wisdom +2"};
        if (biome_ == Biome::Ghost)
            return {"Elixir of Restless Slumber / Health +5", "Phantom's Breath / All stats +4", "Scroll of Lost Echoes / Wisdom +2"};
        return {"Health Potion / Health +5", "Stat Increase Potion / All stats +1", "Book of Knowledge / Reputation +1"};
    case Room::Boss: return {floorNumber_ == 3 ? "Face Malvel" : "Face the guardian"};
    }
    return {};
}

void DungeonRun::choose(std::size_t choice) {
    if (phase_ != Phase::Room || choice >= choices().size()) return;
    switch (room()) {
    case Room::Chest: beginRiddle(); return;
    case Room::Library: {
        const std::array<Stat, 4> stats = {Strength, Agility, Wisdom, Observation};
        const auto stat = stats[choice];
        const float before = player_.stats[stat];
        addStat(stat, 2);
        outcome("You study the book. " + statName(stat) + " increased by " + number(player_.stats[stat] - before) + ".");
        if (floorNumber_ == 3 && choice == 0) {
            ++player_.reputation;
            message_ += "\nReputation +1.\n\n" + readText(assets_ / "textfiles/biomes/lava/library/lavalibrarydiary.txt");
        }
        return;
    }
    case Room::Encounter:
        if (choice == 0) {
            ++player_.reputation;
            const auto suffix = floorNumber_ == 1 ? "prisonfree.txt" : floorNumber_ == 2 ? "butlerfree.txt" : "minionfree.txt";
            outcome(readText(assets_ / ("textfiles/biomes/" + biomeKey() + "/encounter/" + biomeKey() + suffix)) + "\n\nReputation +1.");
        } else {
            const float amount = floorNumber_ == 1 ? 1.f : floorNumber_ == 2 ? 1.5f : 2.f;
            addAll(amount);
            outcome(std::string(floorNumber_ == 1 ? "You take the warrior's vitality." :
                                floorNumber_ == 2 ? "You take the vial and leave the butler to die." : "You kill the minions.") +
                    " All stats increase by " + number(amount * floorNumber_) + ", with your class bonuses.");
        }
        return;
    case Room::Respite:
        if (choice == 0) {
            addStat(Health, 10);
            if (floorNumber_ == 1) {
                ++player_.reputation;
                outcome("The spring restores your health. Reputation +1.\n\n" + readText(assets_ / "textfiles/respitedrink.txt"));
            } else outcome(std::string(floorNumber_ == 3 ? "You rest in the sauna." : "You drink from the respite.") + " Health increases by 10, with your class bonus.");
        } else {
            addAll(2);
            outcome("You uncover supplies among the ruins. All stats increase by " + std::to_string(2 * floorNumber_) + ", with your class bonuses.");
        }
        return;
    case Room::Shop:
        if (choice == 0) { addStat(Health, 5); outcome("The potion increases Health by 5, with your class bonus."); }
        else if (choice == 1 && biome_ == Biome::Desert) { addStat(Strength, 1.5f); outcome("Essence of Fortitude increases Strength by 1.5, with your class bonus."); }
        else if (choice == 1 && biome_ == Biome::Lava) { addStat(Strength, 2); outcome("Corebound Tincture increases Strength by 2, with your class bonus."); }
        else if (choice == 1) {
            const float amount = floorNumber_ == 1 ? 1.f : 2.f;
            addAll(amount);
            outcome("The potion strengthens you. All stats increase by " + number(amount * floorNumber_) + ", with your class bonuses.");
        } else if (floorNumber_ == 3) { addAll(2); outcome("Inferno Draught strengthens you. All stats increase by 6, with your class bonuses."); }
        else if (floorNumber_ == 1) { ++player_.reputation; outcome(readText(assets_ / "textfiles/shopkeeperbook.txt") + "\n\nReputation +1."); }
        else { addStat(Wisdom, 2); outcome("The scroll increases Wisdom by 2, with your class bonus."); }
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

void DungeonRun::beginRiddle() {
    auto& r = riddles_.at(riddleOrder_.at(roomIndex_));
    const std::string correct = r.answers.at(r.correct);
    std::shuffle(r.answers.begin(), r.answers.end(), rng_);
    r.correct = static_cast<std::size_t>(std::find(r.answers.begin(), r.answers.end(), correct) - r.answers.begin());
    hiddenAnswer_.reset();
    if (player_.stats[Observation] > 30) {
        std::array<std::size_t, 2> wrong{};
        std::size_t index = 0;
        for (std::size_t i = 0; i < 3; ++i) if (i != r.correct) wrong[index++] = i;
        hiddenAnswer_ = wrong[std::uniform_int_distribution<int>(0, 1)(rng_)];
    }
    timeLimit_ = (25.f - 5.f * floorNumber_) + std::floor(player_.stats[Wisdom] / 2);
    timeLeft_ = timeLimit_;
    phase_ = Phase::Riddle;
    message_.clear();
}

void DungeonRun::resolveRiddle(bool correct, bool timeout) {
    if (phase_ != Phase::Riddle) return;
    ++player_.riddlesAnswered;
    if (correct) {
        ++player_.riddlesCorrect;
        addAll(.75f);
        chestUnlocked_ = room() == Room::Chest;
        message_ = "Correct. All stats increase by " + number(.75f * floorNumber_) + ", with your class bonuses.";
        if (chestUnlocked_) message_ += "\nThe chest is unlocked. Claim it on the next screen.";
    } else {
        subtractAll(timeout ? .5f : 1.5f);
        message_ = timeout ? "Time has run out. All stats decrease by " + number(.5f * floorNumber_) + "."
                           : "Incorrect. All stats decrease by " + number(1.5f * floorNumber_) + ".";
        message_ += "\nThe answer was: " + riddle().answers.at(riddle().correct) + ".";
        if (room() == Room::Chest) message_ += "\nThe chest remains locked.";
    }
    timeLeft_ = 0;
    phase_ = player_.stats[Health] <= 0 ? Phase::Defeat : Phase::RiddleOutcome;
}

void DungeonRun::answer(std::size_t choice) {
    if (phase_ == Phase::Riddle && answerAvailable(choice)) resolveRiddle(choice == riddle().correct, false);
}

void DungeonRun::tick(float seconds) {
    if (phase_ != Phase::Riddle || !std::isfinite(seconds) || seconds <= 0) return;
    timeLeft_ = std::max(0.f, timeLeft_ - seconds);
    if (timeLeft_ <= 0) resolveRiddle(false, true);
}

void DungeonRun::attack() {
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
        if (room() == Room::Boss && floorNumber_ == 3) { finish(true); return; }
        addAll(1);
        message_ += "\n" + enemyName() + " defeated. All stats increase by " + std::to_string(floorNumber_) + ", with your class bonuses.";
        if (room() == Room::Boss && floorNumber_ == 2) {
            ++player_.reputation;
            message_ += "\nReputation +1.\n\n" + readText(assets_ / ("textfiles/biomes/" + biomeKey() + "/battle/" + biomeKey() + "bosswin.txt"));
        }
        phase_ = room() == Room::Boss ? Phase::Complete : Phase::Outcome;
        return;
    }
    const float retaliation = enemyDamage();
    player_.stats[Health] = std::max(0.f, player_.stats[Health] - retaliation);
    message_ += "\nThe enemy counters for " + number(retaliation) + " health.";
    if (player_.stats[Health] <= 0) {
        if (room() == Room::Boss && floorNumber_ == 3) { finish(false); return; }
        phase_ = Phase::Defeat;
        message_ += "\nYour strength gives out. Your descent has ended.";
    } else if (strikesLeft_ == 0) {
        if (room() == Room::Boss) {
            if (floorNumber_ == 3) { finish(false); return; }
            phase_ = Phase::Defeat;
            message_ += "\nYour strikes are exhausted. The guardian still blocks the stairway.";
        } else {
            subtractAll(.75f);
            outcome(message_ + "\nYou retreat. All stats decrease by " + number(.75f * floorNumber_) + ". The riddle keeper awaits.");
        }
    }
}

void DungeonRun::advance() {
    switch (phase_) {
    case Phase::Outcome: beginRiddle(); return;
    case Phase::RiddleOutcome:
        if (chestUnlocked_) { phase_ = Phase::Loot; return; }
        break;
    case Phase::Loot:
        addAll(2);
        chestUnlocked_ = false;
        message_ = "You claim the chest's treasure. All stats increase by " + std::to_string(2 * floorNumber_) + ", with your class bonuses.";
        if (floorNumber_ == 2 && roomNumber() == 5) {
            ++player_.reputation;
            message_ += "\nReputation +1.\n\n" + readText(assets_ / "textfiles/floor2chestopen.txt");
        }
        if (floorNumber_ == 3 && roomNumber() <= 3) {
            ++player_.reputation;
            message_ += "\nReputation +1.\n\n" + readText(assets_ / "textfiles/biomes/lava/chest/lavachestopen.txt");
        }
        phase_ = Phase::RiddleOutcome;
        return;
    default: return;
    }
    if (roomIndex_ + 1 < rooms_.size()) { ++roomIndex_; enterRoom(); }
}

Ending DungeonRun::resolveEnding(bool malvelDefeated, const Player& player) {
    if (!malvelDefeated) return Ending::Bad;
    if (player.battlesFought == 9 && player.battlesWon == 9 &&
        player.riddlesAnswered == 24 && player.riddlesCorrect == 24 && player.reputation == 9)
        return Ending::Perfect;
    return player.reputation >= 5 && (player.riddlesCorrect >= 14 || player.battlesWon >= 5)
        ? Ending::True : Ending::Good;
}

int DungeonRun::score() const {
    int total = 0;
    for (float stat : player_.stats) total += static_cast<int>(stat);
    return total; // Preserve the original feedback's integer stat total.
}

void DungeonRun::finish(bool malvelDefeated) {
    ending_ = resolveEnding(malvelDefeated, player_);
    phase_ = Phase::Ending;
    message_ = readText(assets_ / storyFile());
}

std::string DungeonRun::endingKey() const {
    switch (ending_) {
    case Ending::Good: return "goodending";
    case Ending::True: return "trueending";
    case Ending::Bad: return "badending";
    case Ending::Perfect: return "perfectending";
    case Ending::None: return {};
    }
    return {};
}

std::string DungeonRun::endingName() const {
    switch (ending_) {
    case Ending::Good: return "Hero of Aethoria?";
    case Ending::True: return "Justice for Falkreath";
    case Ending::Bad: return "Dungeon Demise";
    case Ending::Perfect: return "A Crown in Ashes";
    case Ending::None: return "Game over";
    }
    return {};
}

float DungeonRun::enemyMaxHealth() const {
    const int encounter = room() == Room::Boss ? 2 : roomNumber() <= 3 ? 0 : 1;
    return EnemyBalance[static_cast<int>(difficulty_)][floorNumber_ - 1][encounter].health;
}
float DungeonRun::enemyDamage() const {
    const int encounter = room() == Room::Boss ? 2 : roomNumber() <= 3 ? 0 : 1;
    return EnemyBalance[static_cast<int>(difficulty_)][floorNumber_ - 1][encounter].damage;
}
std::string DungeonRun::difficultyName(Difficulty difficulty) {
    return std::array<std::string, 4>{"Easy", "Medium", "Hard", "Impossible"}.at(static_cast<std::size_t>(difficulty));
}

std::string DungeonRun::heroName(Hero hero) {
    return std::array<std::string, 4>{"Warrior", "Ranger", "Mage", "Rogue"}.at(static_cast<std::size_t>(hero));
}
std::string DungeonRun::portrait(Hero hero) {
    return "images/pfps/" + heroName(hero) + ((hero == Hero::Mage || hero == Hero::Ranger) ? ".jpeg" : ".jpg");
}
std::string DungeonRun::ability(Hero hero) {
    switch (hero) {
    case Hero::Warrior: return "Runic might / +3 Strength\n1.5x Strength and Health rewards";
    case Hero::Ranger: return "Steady aim / +3 Accuracy\n1.5x Accuracy and Health rewards";
    case Hero::Mage: return "Arcane insight / +3 Wisdom\n1.5x stat and 3x Health rewards";
    case Hero::Rogue: return "Shadow step / +3 Agility\n1.5x Agility and Health rewards";
    }
    return {};
}
std::string DungeonRun::statName(std::size_t stat) {
    return std::array<std::string, 6>{"Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy"}.at(stat);
}
std::string DungeonRun::roomName(Room room) {
    return std::array<std::string, 7>{"Library", "Sealed chest", "Battle", "The prisoner", "Respite", "Surtees' shop", "The guardian"}.at(static_cast<std::size_t>(room));
}
std::string DungeonRun::biomeName(Biome biome) {
    return std::array<std::string, 5>{"The Frozen Depths", "The Overgrown Halls", "The Sunken Sands", "The Haunted Halls", "Malvel's Domain"}.at(static_cast<std::size_t>(biome));
}
std::string DungeonRun::biomeKey(Biome biome) {
    return std::array<std::string, 5>{"ice", "jungle", "desert", "ghost", "lava"}.at(static_cast<std::size_t>(biome));
}
std::string DungeonRun::bossImage(Biome biome) {
    const auto key = biomeKey(biome);
    return "images/biomes/" + key + "/battle/" + key + "boss" + (biome == Biome::Desert || biome == Biome::Ghost ? ".jpeg" : ".jpg");
}
std::string DungeonRun::musicFile() const { return biomeKey() + "music.wav"; }
std::string DungeonRun::mapImage() const {
    const auto floor = std::to_string(floorNumber_);
    return "images/map/" + floor + "/floor" + floor + "_map.jpg";
}
std::string DungeonRun::enemyName() const {
    if (room() == Room::Boss)
        return std::array<std::string, 5>{"The Yeti", "The Jungle Warden", "Ahmose the Pharaoh", "Count Dracula", "Malvel"}.at(static_cast<std::size_t>(biome_));
    if (biome_ == Biome::Lava) return roomNumber() == 5 ? "The Lava Knight" : "Lava mini dragon";
    if (biome_ == Biome::Desert) return roomNumber() == 8 ? "Giant scorpion" : "The Sandwyrm";
    if (biome_ == Biome::Ghost) return roomNumber() == 8 ? "The Werewolf" : "Restless zombie";
    return biome_ == Biome::Ice ? "Frost creature" : "Jungle creature";
}
std::string DungeonRun::roomTitle(Room type) const {
    if (floorNumber_ == 3 && type == Room::Encounter) return "Malvel's minions";
    if (floorNumber_ == 3 && type == Room::Shop) return "The merchant";
    if (floorNumber_ == 2 && type == Room::Encounter) return "The wounded butler";
    if (floorNumber_ == 2 && type == Room::Shop) return "The merchant";
    return roomName(type);
}
std::string DungeonRun::title() const { return room() == Room::Boss ? enemyName() : roomTitle(room()); }
std::string DungeonRun::image() const {
    if (phase_ == Phase::Ending) return "images/ending/" + endingKey() + ".jpg";
    const auto key = biomeKey();
    if (room() == Room::Boss) return bossImage(biome_);
    if (floorNumber_ == 3) {
        std::string part;
        switch (room()) {
        case Room::Library: part = "library/lavalibrary.jpg"; break;
        case Room::Chest: part = "chest/lavachest.jpg"; break;
        case Room::Battle: part = roomNumber() == 5 ? "battle/lavabattle2.jpg" : "battle/lavabattle1.jpg"; break;
        case Room::Encounter: part = "encounter/lavaminions.jpg"; break;
        case Room::Respite: part = "respite/lavarespite.jpg"; break;
        case Room::Shop: part = "shop/lavashop.jpg"; break;
        default: break;
        }
        return "images/biomes/lava/" + part;
    }
    if (floorNumber_ == 2) {
        std::string part;
        switch (room()) {
        case Room::Library: part = "library/" + key + "library.jpeg"; break;
        case Room::Chest: part = "chest/" + key + "chest.jpeg"; break;
        case Room::Battle:
            part = "battle/" + key + (biome_ == Biome::Ghost ? (roomNumber() == 8 ? "battle2.jpeg" : "battle1.jpeg")
                                                                       : (roomNumber() == 8 ? "battle2.jpg" : "battle.jpg"));
            break;
        case Room::Encounter: part = "encounter/" + key + "butler.jpeg"; break;
        case Room::Respite: part = "respite/" + key + "respite.jpeg"; break;
        case Room::Shop: part = "shop/" + key + "shop.jpeg"; break;
        default: break;
        }
        return "images/biomes/" + key + "/" + part;
    }
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
std::string DungeonRun::storyFile() const {
    if (phase_ == Phase::Ending) return "textfiles/ending/" + endingKey() + ".txt";
    const auto key = biomeKey();
    std::string suffix;
    switch (room()) {
    case Room::Library: suffix = "library/" + key + "library.txt"; break;
    case Room::Chest: suffix = "chest/" + key + "chest.txt"; break;
    case Room::Battle:
        // Desert asset numbering differs: battle.jpg depicts the Sandwyrm,
        // whose story is battle2.txt; battle2.jpg depicts the scorpion.
        if (biome_ == Biome::Desert) suffix = "battle/" + key + (roomNumber() == 8 ? "battle.txt" : "battle2.txt");
        else suffix = "battle/" + key + (roomNumber() == (floorNumber_ == 2 ? 8 : 5) ? "battle2.txt" : "battle.txt");
        break;
    case Room::Boss: suffix = "battle/" + key + "boss.txt"; break;
    case Room::Encounter: suffix = "encounter/" + key + (floorNumber_ == 1 ? "prison.txt" : floorNumber_ == 2 ? "butler.txt" : "minion.txt"); break;
    case Room::Respite: suffix = "respite/" + key + "respite.txt"; break;
    case Room::Shop: if (floorNumber_ == 1) return "textfiles/shopkeeper.txt"; else suffix = "shop/" + key + "shop.txt"; break;
    }
    return "textfiles/biomes/" + key + "/" + suffix;
}
} // namespace dungeon
