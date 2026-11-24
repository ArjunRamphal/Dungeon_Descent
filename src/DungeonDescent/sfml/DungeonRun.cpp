#include "DungeonRun.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <limits>
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
std::uint32_t fnv1a32(const std::string& s) {
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h == 0 ? 1u : h;
}
std::string encodeHex(const std::string& input) {
    if (input.empty()) return "-";
    static constexpr char Hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char c : input) {
        out.push_back(Hex[(c >> 4) & 0x0f]);
        out.push_back(Hex[c & 0x0f]);
    }
    return out;
}
std::string decodeHex(const std::string& hex) {
    if (hex == "-") return {};
    if (hex.size() % 2 != 0) throw std::runtime_error("Corrupt hex string in saved run.");
    auto nibble = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<unsigned>(c - 'A' + 10);
        throw std::runtime_error("Invalid hex character in saved run.");
    };
    std::string out;
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        out.push_back(static_cast<char>((nibble(hex[i]) << 4) | nibble(hex[i + 1])));
    }
    return out;
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
    const auto parseTag = [](std::string& question) -> std::optional<Biome> {
        const std::array<std::pair<const char*, Biome>, 5> prefixes = {{
            {"[ice] ", Biome::Ice},
            {"[jungle] ", Biome::Jungle},
            {"[desert] ", Biome::Desert},
            {"[ghost] ", Biome::Ghost},
            {"[lava] ", Biome::Lava}
        }};
        for (const auto& [prefix, biome] : prefixes) {
            const std::size_t len = std::char_traits<char>::length(prefix);
            if (question.size() > len && question.compare(0, len, prefix) == 0) {
                question.erase(0, len);
                return biome;
            }
        }
        return std::nullopt;
    };
    for (std::size_t i = 0; i < banks.size(); ++i) {
        std::istringstream input(readText(assets / ("textfiles/riddles/" + banks[i])));
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty()) continue;
            Riddle r;
            r.question = line;
            r.requiredBiome = parseTag(r.question);
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
    seed_ = seed;
    rng_.seed(seed);
    player_ = makePlayer(hero);
    difficulty_ = difficulty;
    firstBiome_ = biome_ = biome;
    secondBiome_.reset();
    floorNumber_ = 1;
    ending_ = Ending::None;
    started_ = true;
    ++runSerial_;
    clearStatDeltas();
    lastCombatExchange_ = {};
    prepareFloor();
}

bool DungeonRun::descend(Biome biome) {
    if (!canDescend()) return false;
    if (floorNumber_ == 1 && biome != Biome::Desert && biome != Biome::Ghost) return false;
    if (floorNumber_ == 2 && biome != Biome::Lava) return false;
    if (floorNumber_ == 1) secondBiome_ = biome;
    ++floorNumber_;
    biome_ = biome;
    clearStatDeltas();
    lastCombatExchange_ = {};
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
    riddleOrder_.clear();
    for (std::size_t idx = 0; idx < riddles_.size(); ++idx) {
        const auto req = riddles_[idx].requiredBiome;
        const bool eligible = !req.has_value() || *req == firstBiome_ || *req == biome_ ||
                              (secondBiome_.has_value() && *req == *secondBiome_);
        if (eligible) riddleOrder_.push_back(idx);
    }
    std::shuffle(riddleOrder_.begin(), riddleOrder_.end(), rng_);
    enterRoom();
}

void DungeonRun::enterRoom() {
    phase_ = Phase::Room;
    chestUnlocked_ = false;
    classAbilityUsed_ = false;
    enemyHealth_ = 0;
    strikesLeft_ = 0;
    bossTurn_ = 1;
    timeLeft_ = timeLimit_ = 0;
    hiddenAnswer_.reset();
    message_.clear();
    clearStatDeltas();
    lastCombatExchange_ = {};
}

void DungeonRun::clearStatDeltas() {
    lastStatDeltas_.fill(0.f);
    lastReputationDelta_ = 0;
}

void DungeonRun::recordStatDeltas(const std::array<float, 6>& beforeStats, int beforeReputation) {
    for (std::size_t i = 0; i < player_.stats.size(); ++i) {
        lastStatDeltas_[i] = player_.stats[i] - beforeStats[i];
    }
    lastReputationDelta_ = player_.reputation - beforeReputation;
}

void DungeonRun::addStat(Stat stat, float amount) {
    const auto hero = player_.hero;
    const bool bonus =
        (hero == Hero::Warrior && stat == Strength) ||
        (hero == Hero::Ranger && stat == Accuracy) ||
        (hero == Hero::Mage && stat == Wisdom) ||
        (hero == Hero::Rogue && stat == Agility);
    player_.stats[stat] += amount * (bonus ? 1.5f : 1.f);
}

void DungeonRun::addAll(float amount) {
    for (std::size_t s = 0; s < player_.stats.size(); ++s) {
        float multiplier = 1.f;
        if (s == Health ||
            (player_.hero == Hero::Warrior && s == Strength) ||
            (player_.hero == Hero::Ranger && s == Accuracy) ||
            (player_.hero == Hero::Mage && s == Wisdom) ||
            (player_.hero == Hero::Rogue && s == Agility)) {
            multiplier = 1.5f;
        }
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

Stat DungeonRun::primaryCombatStat() const {
    if (player_.hero == Hero::Ranger) return Accuracy;
    if (player_.hero == Hero::Mage) return Wisdom;
    if (player_.hero == Hero::Rogue) return Agility;
    return Strength;
}

BossIntent DungeonRun::bossIntent() const {
    BossIntent intent;
    if (room() != Room::Boss || phase_ != Phase::Combat) return intent;
    intent.active = true;
    const int beat = bossTurn_ % 3;
    switch (biome_) {
    case Biome::Ice:
        if (beat == 0) {
            intent.telegraphed = true;
            intent.name = "Glacial Slam";
            intent.detail = "2x counterattack! Guard or negate to survive.";
            intent.counterMultiplier = 2.f;
        } else {
            intent.name = "Gathering Frost";
            intent.detail = (beat == 2) ? "Next turn: Glacial Slam (2x counter)." : "Standard retaliation.";
        }
        break;
    case Biome::Jungle:
        if (beat == 0) {
            intent.telegraphed = true;
            intent.name = "Thorned Carapace";
            intent.detail = "1.6x counter on open strikes; Guard or Ability bypasses thorns.";
            intent.counterMultiplier = 1.6f;
            intent.guardBypasses = true;
        } else {
            intent.name = "Coiling Vines";
            intent.detail = (beat == 2) ? "Next turn: Thorned Carapace (1.6x thorns)." : "Standard retaliation.";
        }
        break;
    case Biome::Desert:
        if (beat == 0) {
            intent.telegraphed = true;
            intent.name = "Curse of the Sands";
            intent.detail = "1.75x counter + drains 1 strike unless Guarded/negated.";
            intent.counterMultiplier = 1.75f;
            intent.drainsStrike = true;
        } else {
            intent.name = "Shifting Dunes";
            intent.detail = (beat == 2) ? "Next turn: Curse of the Sands (drains 1 strike)." : "Standard retaliation.";
        }
        break;
    case Biome::Ghost:
        if (beat == 0) {
            intent.telegraphed = true;
            intent.name = "Blood Siphon";
            intent.detail = "1.5x counter + heals 2x damage dealt! Guard to starve heal.";
            intent.counterMultiplier = 1.5f;
            intent.lifestealRatio = 2.f;
        } else {
            intent.name = "Crimson Gaze";
            intent.detail = (beat == 2) ? "Next turn: Blood Siphon (heals on counter)." : "Standard retaliation.";
        }
        break;
    case Biome::Lava:
        if (beat == 0) {
            intent.telegraphed = true;
            intent.name = "Obsidian Cataclysm";
            intent.detail = "2.25x counterattack! Guard or negate this turn.";
            intent.counterMultiplier = 2.25f;
        } else if (beat == 2) {
            intent.telegraphed = true;
            intent.name = "Arcane Ward";
            intent.detail = "50% damage reduction vs Standard/Guard; Heavy & Ability pierce!";
            intent.damageTakenMultiplier = 0.5f;
            intent.heavyPierces = true;
        } else {
            intent.name = "Molten Focus";
            intent.detail = "Next turn: Arcane Ward (Heavy/Ability pierces).";
        }
        break;
    }
    return intent;
}

CombatActionPreview DungeonRun::previewCombatAction(CombatAction action) const {
    CombatActionPreview preview;
    const float base = std::max(1.f, player_.stats[primaryCombatStat()]);
    const auto intent = bossIntent();
    float mult = 1.f;
    float counterMult = 1.f;
    bool guaranteedCrit = false;
    bool piercesWard = false;
    bool bypassesThorns = false;

    switch (action) {
    case CombatAction::StandardStrike:
        preview.title = "Standard Strike";
        preview.strikeCost = 1;
        preview.available = combatChoiceAvailable(0);
        mult = 1.f;
        counterMult = 1.f;
        preview.effect = "Balanced 1-strike attack.";
        break;
    case CombatAction::HeavyStrike:
        preview.title = "Heavy Strike";
        preview.strikeCost = 2;
        preview.available = combatChoiceAvailable(1);
        mult = 1.85f;
        counterMult = 1.25f;
        piercesWard = true;
        preview.effect = "Costs 2 strikes, 1.85x dmg, 1.25x counter" +
                         std::string(intent.heavyPierces ? " (Pierces Ward!)" : ".");
        break;
    case CombatAction::GuardStrike:
        preview.title = "Guard Strike";
        preview.strikeCost = 1;
        preview.available = combatChoiceAvailable(2);
        mult = 0.75f;
        counterMult = 0.25f;
        bypassesThorns = true;
        preview.effect = "0.75x dmg, cuts counter to 25%" +
                         std::string(intent.drainsStrike ? " & blocks strike drain." : ".");
        break;
    case CombatAction::ClassAbility:
        preview.title = combatAbilityName(player_.hero);
        preview.strikeCost = 1;
        preview.available = combatChoiceAvailable(3);
        piercesWard = true;
        bypassesThorns = true;
        switch (player_.hero) {
        case Hero::Warrior:
            mult = 1.35f; counterMult = 0.f;
            preview.effect = "1.35x dmg + blocks counterattack (once/battle).";
            break;
        case Hero::Ranger:
            mult = 2.5f; counterMult = 1.f; guaranteedCrit = true;
            preview.effect = "Guaranteed 2.5x crit (once/battle).";
            break;
        case Hero::Mage:
            mult = 1.5f; counterMult = 1.f;
            preview.effect = "1.5x dmg + heals " + std::to_string(3 + floorNumber_) + " HP (once/battle).";
            break;
        case Hero::Rogue:
            mult = 1.15f; counterMult = 0.f;
            preview.effect = "1.15x dmg, evades counter & refunds strike.";
            break;
        }
        break;
    }

    const float wardMult = (intent.damageTakenMultiplier < 1.f && !(intent.heavyPierces && piercesWard))
        ? intent.damageTakenMultiplier : 1.f;
    preview.minDamage = std::max(1.f, base * mult * wardMult);
    preview.critDamage = guaranteedCrit ? preview.minDamage : std::max(1.f, base * mult * 2.f * wardMult);

    const float bossCounterMult = (intent.guardBypasses && bypassesThorns) ? 1.f : intent.counterMultiplier;
    preview.expectedCounter = (preview.minDamage >= enemyHealth_ && phase_ == Phase::Combat)
        ? 0.f : (enemyDamage() * counterMult * bossCounterMult);
    return preview;
}

CombatMathSummary DungeonRun::combatSummary() const {
    CombatMathSummary summary;
    const Stat primary = primaryCombatStat();
    const float base = std::max(1.f, player_.stats[primary]);
    const float critChance = std::clamp(player_.stats[Accuracy] / 35.f, 0.f, 1.f);
    summary.baseDamage = base;
    summary.heavyDamage = base * 1.85f;
    summary.guardDamage = std::max(1.f, base * 0.75f);
    switch (player_.hero) {
    case Hero::Warrior: summary.abilityDamage = base * 1.35f; break;
    case Hero::Ranger:  summary.abilityDamage = base * 2.5f; break;
    case Hero::Mage:    summary.abilityDamage = base * 1.5f; break;
    case Hero::Rogue:   summary.abilityDamage = base * 1.15f; break;
    }
    summary.critChance = critChance;
    summary.critMultiplier = 2.f;
    summary.critDamage = base * 2.f;
    summary.expectedDamage = base * (1.f + critChance);
    const int agilityBonus = static_cast<int>(std::max(0.f, player_.stats[Agility]) / 3.f);
    summary.minStrikes = 3 + agilityBonus;
    summary.maxStrikes = 5 + agilityBonus;
    summary.expectedStrikes = 4.f + static_cast<float>(agilityBonus);
    summary.enemyMaxHealth = enemyMaxHealth();
    summary.enemyHealth = phase_ == Phase::Combat ? enemyHealth_ : summary.enemyMaxHealth;
    summary.enemyCounterDamage = enemyDamage();
    summary.guardCounterDamage = std::max(0.f, summary.enemyCounterDamage * 0.25f);
    return summary;
}

bool DungeonRun::combatChoiceAvailable(std::size_t choice) const {
    if (phase_ != Phase::Combat || strikesLeft_ <= 0) return false;
    switch (choice) {
    case 0: return strikesLeft_ >= 1;
    case 1: return strikesLeft_ >= 2;
    case 2: return strikesLeft_ >= 1;
    case 3: return !classAbilityUsed_ && strikesLeft_ >= 1;
    default: return false;
    }
}

std::vector<std::size_t> DungeonRun::roomChoiceOrder() const {
    if (phase_ != Phase::Room) return {};
    const auto r = room();
    if (r != Room::Library && r != Room::Shop) return {};
    const std::size_t count = (r == Room::Library) ? 4 : 3;
    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), 0);
    // Deterministic per-seed, per-floor, per-room shuffle (cross-platform Fisher-Yates;
    // does not perturb main combat/riddle rng_ or suspend save compatibility).
    std::uint64_t mix = static_cast<std::uint64_t>(seed_) ^
                        (static_cast<std::uint64_t>(floorNumber_) * 0x9E3779B97F4A7C15ULL) ^
                        (static_cast<std::uint64_t>(roomIndex_ + 1) * 0xBF58476D1CE4E5B9ULL) ^
                        (r == Room::Library ? 0x94D049BB133111EBULL : 0x2545F4914F6CDD1DULL);
    mix ^= (mix >> 30);
    mix *= 0xBF58476D1CE4E5B9ULL;
    mix ^= (mix >> 27);
    mix *= 0x94D049BB133111EBULL;
    mix ^= (mix >> 31);
    std::mt19937 choiceRng(static_cast<std::uint32_t>(mix ^ (mix >> 32)));
    for (std::size_t i = count - 1; i > 0; --i) {
        const std::size_t j = static_cast<std::size_t>(choiceRng() % (i + 1));
        std::swap(order[i], order[j]);
    }
    return order;
}

std::size_t DungeonRun::displayChoiceFor(std::size_t canonicalChoice) const {
    const auto order = roomChoiceOrder();
    if (order.empty()) return canonicalChoice;
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (order[i] == canonicalChoice) return i;
    }
    return canonicalChoice;
}

std::vector<std::string> DungeonRun::choices() const {
    if (phase_ == Phase::Combat) {
        return {
            "Standard Strike",
            "Heavy Strike (2 strikes)",
            "Guard Strike",
            combatAbilityName(player_.hero) + (classAbilityUsed_ ? " (Used)" : "")
        };
    }
    if (phase_ != Phase::Room) return {};
    const auto reorder = [this](const std::vector<std::string>& canonical) {
        const auto order = roomChoiceOrder();
        std::vector<std::string> out;
        out.reserve(order.size());
        for (const auto idx : order) out.push_back(canonical.at(idx));
        return out;
    };
    switch (room()) {
    case Room::Library:
        if (biome_ == Biome::Lava)
            return reorder({"The Princess Diary", "The Smoldering Whisper",
                            "The Magma Script", "The Ashen Eye"});
        if (biome_ == Biome::Desert)
            return reorder({"The Sands of Strategy", "The Silent Zephyrs",
                            "The Hidden Glyphs", "The Oasis Eye"});
        if (biome_ == Biome::Ghost)
            return reorder({"The Cyclops Might", "The Weeping Wolfman",
                            "The Immortal Alucard", "The Eyes of the Raven"});
        return reorder({"The Art of Warfare", "The Whispering Winds",
                        "The Ancient Lore", "The Heart of the Forest"});
    case Room::Chest: return {"Read the seal's riddle"};
    case Room::Battle: return {"Confront the creature", "Sneak past"};
    case Room::Encounter:
        if (floorNumber_ == 3) return {"Threaten minions", "Kill minions"};
        return floorNumber_ == 1 ? std::vector<std::string>{"Free the warrior", "Take his vitality"}
                                : std::vector<std::string>{"Save the butler", "Take the vial"};
    case Room::Respite: return {floorNumber_ == 3 ? "Rest in the sauna" : "Drink from the spring",
                               "Search the room"};
    case Room::Shop:
        if (biome_ == Biome::Lava)
            return reorder({"Emberheart Elixir", "Corebound Tincture", "Inferno Draught"});
        if (biome_ == Biome::Desert)
            return reorder({"Draught of Revitalization", "Essence of Fortitude", "Scroll of Ancient Wisdom"});
        if (biome_ == Biome::Ghost)
            return reorder({"Elixir of Restless Slumber", "Phantom's Breath", "Scroll of Lost Echoes"});
        return reorder({"Health Potion", "Stat Increase Potion", "Book of Knowledge"});
    case Room::Boss: return {floorNumber_ == 3 ? "Face Malvel" : "Face the guardian"};
    }
    return {};
}

void DungeonRun::choose(std::size_t choice) {
    if (phase_ == Phase::Combat) {
        if (choice < 4) attack(static_cast<CombatAction>(choice));
        return;
    }
    if (phase_ != Phase::Room || choice >= choices().size()) return;
    const auto beforeStats = player_.stats;
    const int beforeReputation = player_.reputation;
    switch (room()) {
    case Room::Chest: beginRiddle(); return;
    case Room::Library: {
        const std::size_t canonical = roomChoiceOrder().at(choice);
        const std::array<Stat, 4> stats = {Strength, Agility, Wisdom, Observation};
        const auto stat = stats[canonical];
        const float before = player_.stats[stat];
        addStat(stat, 2);
        outcome("You study the book. " + statName(stat) + " increased by " + number(player_.stats[stat] - before) + ".");
        if (floorNumber_ == 3 && canonical == 0) {
            ++player_.reputation;
            message_ += "\nReputation +1.\n\n" + readText(assets_ / "textfiles/biomes/lava/library/lavalibrarydiary.txt");
        }
        recordStatDeltas(beforeStats, beforeReputation);
        return;
    }
    case Room::Encounter:
        if (choice == 0) {
            ++player_.reputation;
            const auto suffix = floorNumber_ == 1 ? "prisonfree.txt" : floorNumber_ == 2 ? "butlerfree.txt" : "minionfree.txt";
            outcome(readText(assets_ / ("textfiles/biomes/" + biomeKey() + "/encounter/" + biomeKey() + suffix)) + "\n\nReputation +1.");
        } else {
            ++player_.npcKills;
            const float amount = floorNumber_ == 1 ? 1.f : floorNumber_ == 2 ? 1.5f : 2.f;
            addAll(amount);
            outcome(std::string(floorNumber_ == 1 ? "You take the warrior's vitality." :
                                floorNumber_ == 2 ? "You take the vial and leave the butler to die." : "You kill the minions.") +
                    " All stats increase by " + number(amount * floorNumber_) + ", with your class bonuses.");
        }
        recordStatDeltas(beforeStats, beforeReputation);
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
        recordStatDeltas(beforeStats, beforeReputation);
        return;
    case Room::Shop: {
        const std::size_t canonical = roomChoiceOrder().at(choice);
        if (canonical == 0) { addStat(Health, 5); outcome("The potion increases Health by 5, with your class bonus."); }
        else if (canonical == 1 && biome_ == Biome::Desert) { addStat(Strength, 1.5f); outcome("Essence of Fortitude increases Strength by 1.5, with your class bonus."); }
        else if (canonical == 1 && biome_ == Biome::Lava) { addStat(Strength, 2); outcome("Corebound Tincture increases Strength by 2, with your class bonus."); }
        else if (canonical == 1) {
            const float amount = floorNumber_ == 1 ? 1.f : 2.f;
            addAll(amount);
            outcome("The potion strengthens you. All stats increase by " + number(amount * floorNumber_) + ", with your class bonuses.");
        } else if (floorNumber_ == 3) { addAll(2); outcome("Inferno Draught strengthens you. All stats increase by 6, with your class bonuses."); }
        else if (floorNumber_ == 1) { ++player_.reputation; outcome(readText(assets_ / "textfiles/shopkeeperbook.txt") + "\n\nReputation +1."); }
        else { addStat(Wisdom, 2); outcome("The scroll increases Wisdom by 2, with your class bonus."); }
        recordStatDeltas(beforeStats, beforeReputation);
        return;
    }
    case Room::Battle:
    case Room::Boss:
        if (choice == 1) {
            outcome("You slip past unnoticed. You gain no battle reward, but the riddle keeper still awaits.");
            recordStatDeltas(beforeStats, beforeReputation);
            return;
        }
        ++player_.battlesFought;
        enemyHealth_ = enemyMaxHealth();
        strikesLeft_ = std::uniform_int_distribution<int>(3, 5)(rng_) + static_cast<int>(player_.stats[Agility] / 3);
        bossTurn_ = 1;
        classAbilityUsed_ = false;
        lastCombatExchange_ = {};
        clearStatDeltas();
        phase_ = Phase::Combat;
        message_ = "Choose your combat tactic. Heavy Strike trades two strikes for burst power; Guard Strike dampens retaliation; your class ability can turn a tough fight once per battle.";
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
    clearStatDeltas();
    phase_ = Phase::Riddle;
    message_.clear();
}

void DungeonRun::resolveRiddle(bool correct, bool timeout) {
    if (phase_ != Phase::Riddle) return;
    const auto beforeStats = player_.stats;
    const int beforeReputation = player_.reputation;
    ++player_.riddlesAnswered;
    if (correct) {
        ++player_.riddlesCorrect;
        ++player_.riddleStreak;
        player_.bestRiddleStreak = std::max(player_.bestRiddleStreak, player_.riddleStreak);
        const float streakMult = player_.riddleStreak >= 5 ? 1.30f : (player_.riddleStreak >= 3 ? 1.15f : 1.f);
        addAll(.75f * streakMult);
        chestUnlocked_ = room() == Room::Chest;
        message_ = "Correct. All stats increase by " + number(.75f * streakMult * floorNumber_) + ", with your class bonuses.";
        if (player_.riddleStreak >= 3) {
            const int pct = player_.riddleStreak >= 5 ? 30 : 15;
            message_ += "\nRiddle streak x" + std::to_string(player_.riddleStreak) + " (+" + std::to_string(pct) + "% bonus)!";
        }
        if (chestUnlocked_) message_ += "\nThe chest is unlocked. Claim it on the next screen.";
    } else {
        player_.riddleStreak = 0;
        subtractAll(timeout ? .5f : 1.5f);
        message_ = timeout ? "Time has run out. All stats decrease by " + number(.5f * floorNumber_) + "."
                           : "Incorrect. All stats decrease by " + number(1.5f * floorNumber_) + ".";
        message_ += "\nThe answer was: " + riddle().answers.at(riddle().correct) + ".";
        if (room() == Room::Chest) message_ += "\nThe chest remains locked.";
    }
    timeLeft_ = 0;
    recordStatDeltas(beforeStats, beforeReputation);
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
    attack(CombatAction::StandardStrike);
}

void DungeonRun::attack(CombatAction action) {
    if (phase_ != Phase::Combat || strikesLeft_ <= 0) return;
    if (action == CombatAction::HeavyStrike && strikesLeft_ < 2) return;
    if (action == CombatAction::ClassAbility && classAbilityUsed_) return;

    const auto beforeStats = player_.stats;
    const int beforeReputation = player_.reputation;
    const auto intent = bossIntent();

    const int strikeCost = action == CombatAction::HeavyStrike ? 2 : 1;
    strikesLeft_ -= strikeCost;
    if (action == CombatAction::ClassAbility) {
        classAbilityUsed_ = true;
        ++player_.classAbilitiesUsed;
    }

    const float base = std::max(1.f, player_.stats[primaryCombatStat()]);
    bool critical = false;
    float actionMultiplier = 1.f;
    float counterMultiplier = 1.f;
    bool piercesWard = false;
    bool bypassesThorns = false;
    std::string prefix = "You strike. ";

    switch (action) {
    case CombatAction::StandardStrike:
        critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
        actionMultiplier = critical ? 2.f : 1.f;
        prefix = critical ? "Critical hit! " : "You strike. ";
        break;
    case CombatAction::HeavyStrike:
        critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
        actionMultiplier = 1.85f * (critical ? 2.f : 1.f);
        counterMultiplier = 1.25f;
        piercesWard = true;
        prefix = critical ? "Devastating critical Heavy Strike! " : "Heavy Strike! ";
        break;
    case CombatAction::GuardStrike:
        critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
        actionMultiplier = 0.75f * (critical ? 2.f : 1.f);
        counterMultiplier = 0.25f;
        bypassesThorns = true;
        prefix = critical ? "Guarded critical strike! " : "Guard Strike! ";
        break;
    case CombatAction::ClassAbility:
        piercesWard = true;
        bypassesThorns = true;
        switch (player_.hero) {
        case Hero::Warrior:
            // Shield Bash: solid blow + blocks enemy counterattack this exchange.
            critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
            actionMultiplier = 1.35f * (critical ? 2.f : 1.f);
            counterMultiplier = 0.f;
            prefix = critical ? "Critical Shield Bash! Enemy staggered. " : "Shield Bash! Enemy staggered. ";
            break;
        case Hero::Ranger:
            // Precision Shot: guaranteed critical strike at 2.5x base damage.
            critical = true;
            actionMultiplier = 2.5f;
            counterMultiplier = 1.f;
            prefix = "Precision Shot! Guaranteed critical strike! ";
            break;
        case Hero::Mage:
            // Arcane Surge: burst arcane strike + restores 3 + floor HP.
            critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
            actionMultiplier = 1.5f * (critical ? 2.f : 1.f);
            counterMultiplier = 1.f;
            player_.stats[Health] += 3.f + static_cast<float>(floorNumber_);
            prefix = critical ? "Critical Arcane Surge! " : "Arcane Surge! ";
            break;
        case Hero::Rogue:
            // Shadow Step: swift strike that refunds its strike and evades retaliation.
            critical = std::uniform_int_distribution<int>(0, 34)(rng_) < player_.stats[Accuracy];
            actionMultiplier = 1.15f * (critical ? 2.f : 1.f);
            counterMultiplier = 0.f;
            ++strikesLeft_;
            prefix = critical ? "Critical Shadow Step! Evaded counter and kept strike momentum. "
                              : "Shadow Step! Evaded counter and kept strike momentum. ";
            break;
        }
        break;
    }

    const bool wardReduced = intent.damageTakenMultiplier < 1.f && !(intent.heavyPierces && piercesWard);
    const float wardMult = wardReduced ? intent.damageTakenMultiplier : 1.f;
    const float damage = std::max(1.f, base * actionMultiplier * wardMult);
    enemyHealth_ = std::max(0.f, enemyHealth_ - damage);
    message_ = prefix + number(damage) + " damage.";
    if (wardReduced) {
        message_ += " (" + intent.name + " dampens the blow!)";
    } else if (intent.damageTakenMultiplier < 1.f && intent.heavyPierces && piercesWard) {
        message_ += " (Pierced " + intent.name + "!)";
    }

    CombatExchangeResult result;
    result.valid = true;
    result.action = action;
    result.critical = critical;
    result.guarded = (action == CombatAction::GuardStrike) ||
                     (action == CombatAction::ClassAbility && (player_.hero == Hero::Warrior || player_.hero == Hero::Rogue));
    result.strikesSpent = strikeCost;
    result.damageDealt = damage;

    if (enemyHealth_ <= 0) {
        ++player_.battlesWon;
        player_.strikesLeftover += std::max(0, strikesLeft_);
        result.enemyDefeated = true;
        lastCombatExchange_ = result;
        if (room() == Room::Boss && floorNumber_ == 3) {
            recordStatDeltas(beforeStats, beforeReputation);
            finish(true);
            return;
        }
        addAll(1);
        message_ += "\n" + enemyName() + " defeated. All stats increase by " + std::to_string(floorNumber_) + ", with your class bonuses.";
        if (room() == Room::Boss && floorNumber_ == 2) {
            ++player_.reputation;
            message_ += "\nReputation +1.\n\n" + readText(assets_ / ("textfiles/biomes/" + biomeKey() + "/battle/" + biomeKey() + "bosswin.txt"));
        }
        recordStatDeltas(beforeStats, beforeReputation);
        phase_ = room() == Room::Boss ? Phase::Complete : Phase::Outcome;
        return;
    }

    const float bossCounterMult = (intent.guardBypasses && bypassesThorns) ? 1.f : intent.counterMultiplier;
    const float retaliation = enemyDamage() * counterMultiplier * bossCounterMult;
    if (retaliation > 0.f) {
        player_.stats[Health] = std::max(0.f, player_.stats[Health] - retaliation);
        result.counterattacked = true;
        result.counterDamage = retaliation;
        if (action == CombatAction::GuardStrike) {
            message_ += "\nGuard reduces the counterattack to " + number(retaliation) + " health.";
        } else if (intent.telegraphed && bossCounterMult > 1.f) {
            message_ += "\n" + intent.name + " strikes back for " + number(retaliation) + " health!";
        } else {
            message_ += "\nThe enemy counters for " + number(retaliation) + " health.";
        }
        if (intent.lifestealRatio > 0.f) {
            const float heal = std::min(enemyMaxHealth() - enemyHealth_, retaliation * intent.lifestealRatio);
            if (heal > 0.f) {
                enemyHealth_ += heal;
                result.enemyHealed = heal;
                message_ += "\n" + intent.name + " restores " + number(heal) + " enemy HP!";
            }
        }
        if (intent.drainsStrike && !result.guarded && strikesLeft_ > 0) {
            --strikesLeft_;
            result.strikesDrained = 1;
            message_ += "\n" + intent.name + " drains 1 strike!";
        }
    } else {
        message_ += "\nThe enemy's counterattack is negated.";
    }

    if (room() == Room::Boss) {
        ++bossTurn_;
    }

    if (player_.stats[Health] <= 0) {
        result.playerDefeated = true;
        lastCombatExchange_ = result;
        recordStatDeltas(beforeStats, beforeReputation);
        if (room() == Room::Boss && floorNumber_ == 3) { finish(false); return; }
        phase_ = Phase::Defeat;
        message_ += "\nYour strength gives out. Your descent has ended.";
    } else if (strikesLeft_ == 0) {
        if (room() == Room::Boss) {
            result.playerDefeated = true;
            lastCombatExchange_ = result;
            recordStatDeltas(beforeStats, beforeReputation);
            if (floorNumber_ == 3) { finish(false); return; }
            phase_ = Phase::Defeat;
            message_ += "\nYour strikes are exhausted. The guardian still blocks the stairway.";
        } else {
            result.retreated = true;
            lastCombatExchange_ = result;
            subtractAll(.75f);
            recordStatDeltas(beforeStats, beforeReputation);
            outcome(message_ + "\nYou retreat. All stats decrease by " + number(.75f * floorNumber_) + ". The riddle keeper awaits.");
        }
    } else {
        lastCombatExchange_ = result;
    }
}

void DungeonRun::advance() {
    switch (phase_) {
    case Phase::Outcome: beginRiddle(); return;
    case Phase::RiddleOutcome:
        if (chestUnlocked_) {
            clearStatDeltas();
            phase_ = Phase::Loot;
            return;
        }
        break;
    case Phase::Loot: {
        const auto beforeStats = player_.stats;
        const int beforeReputation = player_.reputation;
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
        recordStatDeltas(beforeStats, beforeReputation);
        phase_ = Phase::RiddleOutcome;
        return;
    }
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

ScoreBreakdown DungeonRun::scoreBreakdown(int playTimeSeconds) const {
    ScoreBreakdown bd;
    int nonHealthStats = 0;
    for (int i = 0; i < 6; ++i) {
        if (i == Health) continue;
        nonHealthStats += std::max(0, static_cast<int>(player_.stats[static_cast<std::size_t>(i)]));
    }
    bd.statsPoints = nonHealthStats * 5;
    bd.healthPoints = std::max(0, static_cast<int>(player_.stats[Health])) * 10;
    bd.strikesPoints = std::max(0, player_.strikesLeftover) * 10;
    const bool victorious = (ending_ != Ending::None && ending_ != Ending::Bad);
    if (victorious && playTimeSeconds >= 0) {
        bd.timePoints = std::max(0, 1800 - playTimeSeconds) * 2;
    }
    bd.total = bd.statsPoints + bd.healthPoints + bd.strikesPoints + bd.timePoints;
    return bd;
}

int DungeonRun::score(int playTimeSeconds) const {
    return scoreBreakdown(playTimeSeconds).total;
}

void DungeonRun::finish(bool malvelDefeated) {
    ending_ = resolveEnding(malvelDefeated, player_);
    phase_ = Phase::Ending;
    message_ = readText(assets_ / storyFile());
}

std::string DungeonRun::endingKey(Ending ending) {
    switch (ending) {
    case Ending::Good: return "goodending";
    case Ending::True: return "trueending";
    case Ending::Bad: return "badending";
    case Ending::Perfect: return "perfectending";
    case Ending::None: return {};
    }
    return {};
}

std::string DungeonRun::endingName(Ending ending) {
    switch (ending) {
    case Ending::Good: return "Hero of Aethoria?";
    case Ending::True: return "Justice for Falkreath";
    case Ending::Bad: return "Dungeon Demise";
    case Ending::Perfect: return "A Crown in Ashes";
    case Ending::None: return "Game over";
    }
    return {};
}

std::string DungeonRun::endingImage(Ending ending) {
    const auto key = endingKey(ending);
    return key.empty() ? std::string{} : ("images/ending/" + key + ".jpg");
}

std::string DungeonRun::endingStoryFile(Ending ending) {
    const auto key = endingKey(ending);
    return key.empty() ? std::string{} : ("textfiles/ending/" + key + ".txt");
}

std::string DungeonRun::endingMusicFile(Ending ending) {
    switch (ending) {
    case Ending::Good: return "ending_good.ogg";
    case Ending::True: return "ending_true.ogg";
    case Ending::Bad: return "ending_bad.ogg";
    case Ending::Perfect: return "ending_perfect.ogg";
    case Ending::None: return "menu.mp3";
    }
    return "menu.mp3";
}

std::string DungeonRun::endingKey() const { return endingKey(ending_); }
std::string DungeonRun::endingName() const { return endingName(ending_); }

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
    case Hero::Warrior: return "Runic might / +3 Strength\n1.5x Strength & Health | Shield Bash";
    case Hero::Ranger:  return "Steady aim / +3 Accuracy\n1.5x Accuracy & Health | Precision Shot";
    case Hero::Mage:    return "Arcane insight / +3 Wisdom\n1.5x Wisdom & Health | Arcane Surge";
    case Hero::Rogue:   return "Shadow step / +3 Agility\n1.5x Agility & Health | Shadow Step";
    }
    return {};
}
std::string DungeonRun::combatAbilityName(Hero hero) {
    switch (hero) {
    case Hero::Warrior: return "Shield Bash";
    case Hero::Ranger:  return "Precision Shot";
    case Hero::Mage:    return "Arcane Surge";
    case Hero::Rogue:   return "Shadow Step";
    }
    return {};
}
std::string DungeonRun::combatAbilityDescription(Hero hero) {
    switch (hero) {
    case Hero::Warrior: return "1.35x Strength strike and staggers the enemy to block its counterattack (once per battle).";
    case Hero::Ranger:  return "Guaranteed critical hit for 2.5x Accuracy damage (once per battle).";
    case Hero::Mage:    return "1.5x Wisdom strike and restores 3 + floor Health (once per battle).";
    case Hero::Rogue:   return "1.15x Agility strike, evades counterattack, and refunds its strike (once per battle).";
    }
    return {};
}
std::string DungeonRun::statName(std::size_t stat) {
    return std::array<std::string, 6>{"Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy"}.at(stat);
}
std::string DungeonRun::statTooltip(Stat stat) {
    switch (stat) {
    case Strength:    return "Strength: Primary combat damage for the Warrior (and scales Sword strikes).";
    case Wisdom:      return "Wisdom: Primary combat damage for the Mage and adds floor(Wisdom / 2)s to riddle timers.";
    case Observation: return "Observation: Above 30 eliminates one false answer on every riddle.";
    case Agility:     return "Agility: Primary combat damage for the Rogue and adds floor(Agility / 3) extra strikes per battle.";
    case Health:      return "Health: Your growing life pool. Reaching 0 in combat or riddles ends the descent.";
    case Accuracy:    return "Accuracy: Primary combat damage for the Ranger and grants Accuracy / 35 chance for 2x critical hits.";
    }
    return {};
}
std::array<std::string, 6> DungeonRun::statTooltips() {
    return {
        statTooltip(Strength),
        statTooltip(Wisdom),
        statTooltip(Observation),
        statTooltip(Agility),
        statTooltip(Health),
        statTooltip(Accuracy)
    };
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
std::string DungeonRun::musicFile() const {
    if (phase_ == Phase::Ending && ending_ != Ending::None) return endingMusicFile(ending_);
    return biomeKey() + "music.ogg";
}
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
    if (phase_ == Phase::Ending) return endingImage(ending_);
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
    case Room::Library: suffix = "library/" + key + "library.jpg"; break;
    case Room::Chest: suffix = "chest/" + key + "chest.jpg"; break;
    case Room::Battle: suffix = "battle/" + key + (roomNumber() == 5 ? "battle2.jpg" : "battle.jpg"); break;
    case Room::Boss: suffix = "battle/" + key + "boss.jpg"; break;
    case Room::Encounter: return "images/" + std::string(biome_ == Biome::Ice ? "biomes/ice/encounter/iceencounter.jpg" : "prison.jpeg");
    case Room::Respite: suffix = "respite/" + key + "respite.jpg"; break;
    case Room::Shop: suffix = "shop/" + key + "shop.jpg"; break;
    }
    return "images/biomes/" + key + "/" + suffix;
}
std::string DungeonRun::storyFile() const {
    if (phase_ == Phase::Ending) return endingStoryFile(ending_);
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

std::uint32_t DungeonRun::parseSeed(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) throw std::invalid_argument("Seed text cannot be empty.");
    const auto last = text.find_last_not_of(" \t\r\n");
    const std::string trimmed = text.substr(first, last - first + 1);
    const bool allDigits = std::all_of(trimmed.begin(), trimmed.end(), [](unsigned char c) { return c >= '0' && c <= '9'; });
    if (allDigits) {
        try {
            std::size_t idx = 0;
            const unsigned long long parsed = std::stoull(trimmed, &idx, 10);
            if (idx == trimmed.size() && parsed <= std::numeric_limits<std::uint32_t>::max()) {
                return static_cast<std::uint32_t>(parsed);
            }
        } catch (const std::exception&) {
        }
    }
    return fnv1a32(trimmed);
}

std::uint32_t DungeonRun::dailyRunSeed(int year, int month, int day) {
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31)
        throw std::invalid_argument("Invalid calendar date for daily run seed.");
    std::ostringstream out;
    out << "DAILY-" << std::setw(4) << std::setfill('0') << year << '-'
        << std::setw(2) << std::setfill('0') << month << '-'
        << std::setw(2) << std::setfill('0') << day;
    return fnv1a32(out.str());
}

std::string DungeonRun::dailyRunDateString() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << (utc.tm_year + 1900) << '-'
        << std::setw(2) << std::setfill('0') << (utc.tm_mon + 1) << '-'
        << std::setw(2) << std::setfill('0') << utc.tm_mday;
    return out.str();
}

std::uint32_t DungeonRun::dailyRunSeedForToday() {
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    return dailyRunSeed(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday);
}

std::string DungeonRun::serializeState() const {
    if (!started_) throw std::runtime_error("Cannot serialize an unstarted DungeonRun.");
    std::ostringstream out;
    out << std::setprecision(9);
    out << "DUNGEON_RUN_STATE 5\n";
    out << "meta " << static_cast<int>(difficulty_) << ' ' << static_cast<int>(biome_) << ' '
        << static_cast<int>(firstBiome_) << ' ' << floorNumber_ << ' ' << static_cast<int>(phase_) << ' '
        << static_cast<int>(ending_) << ' ' << (started_ ? 1 : 0) << ' ' << seed_ << ' ' << runSerial_ << ' '
        << roomIndex_ << ' ' << (chestUnlocked_ ? 1 : 0) << ' ' << (classAbilityUsed_ ? 1 : 0) << ' '
        << timeLeft_ << ' ' << timeLimit_ << ' ' << enemyHealth_ << ' ' << strikesLeft_ << ' '
        << (hiddenAnswer_.has_value() ? static_cast<long long>(*hiddenAnswer_) : -1LL) << ' ' << bossTurn_ << ' '
        << (secondBiome_.has_value() ? static_cast<int>(*secondBiome_) : -1) << '\n';
    out << "player " << static_cast<int>(player_.hero) << ' ' << player_.reputation << ' '
        << player_.riddlesCorrect << ' ' << player_.riddlesAnswered << ' '
        << player_.battlesWon << ' ' << player_.battlesFought << ' '
        << player_.riddleStreak << ' ' << player_.bestRiddleStreak << ' '
        << player_.npcKills << ' ' << player_.classAbilitiesUsed << ' '
        << player_.strikesLeftover;
    for (float s : player_.stats) out << ' ' << s;
    out << '\n';
    out << "deltas " << lastReputationDelta_;
    for (float d : lastStatDeltas_) out << ' ' << d;
    out << '\n';
    out << "exchange " << (lastCombatExchange_.valid ? 1 : 0) << ' ' << static_cast<int>(lastCombatExchange_.action) << ' '
        << (lastCombatExchange_.critical ? 1 : 0) << ' ' << (lastCombatExchange_.guarded ? 1 : 0) << ' '
        << (lastCombatExchange_.enemyDefeated ? 1 : 0) << ' ' << (lastCombatExchange_.retreated ? 1 : 0) << ' '
        << (lastCombatExchange_.playerDefeated ? 1 : 0) << ' ' << lastCombatExchange_.strikesSpent << ' '
        << lastCombatExchange_.damageDealt << ' ' << (lastCombatExchange_.counterattacked ? 1 : 0) << ' '
        << lastCombatExchange_.counterDamage << ' ' << lastCombatExchange_.enemyHealed << ' '
        << lastCombatExchange_.strikesDrained << '\n';
    out << "rooms";
    for (Room r : rooms_) out << ' ' << static_cast<int>(r);
    out << '\n';
    out << "riddle_order " << riddleOrder_.size();
    for (std::size_t idx : riddleOrder_) out << ' ' << idx;
    out << '\n';
    const auto& activeRiddle = riddles_.at(riddleOrder_.at(roomIndex_));
    out << "active_riddle " << activeRiddle.correct;
    for (const auto& ans : activeRiddle.answers) out << ' ' << encodeHex(ans);
    out << '\n';
    out << "message " << encodeHex(message_) << '\n';
    out << "rng " << rng_ << '\n';
    return out.str();
}

void DungeonRun::deserializeState(const std::string& serialized) {
    std::istringstream in(serialized);
    std::string tag;
    int version = 0;
    if (!(in >> tag >> version) || tag != "DUNGEON_RUN_STATE" || version < 1 || version > 5)
        throw std::runtime_error("Invalid DungeonRun save state header.");

    int diff = 0, bio = 0, firstBio = 0, secondBio = -1, floor = 1, ph = 0, end = 0, started = 0, chest = 0, abilityUsed = 0;
    long long hidden = -1;
    std::uint32_t seedVal = 0;
    std::uint64_t serialVal = 0;
    std::size_t rIndex = 0;
    float tLeft = 0.f, tLimit = 0.f, eHealth = 0.f;
    int sLeft = 0, bTurn = 1;
    if (!(in >> tag) || tag != "meta" ||
        !(in >> diff >> bio >> firstBio >> floor >> ph >> end >> started >> seedVal >> serialVal >>
          rIndex >> chest >> abilityUsed >> tLeft >> tLimit >> eHealth >> sLeft >> hidden))
        throw std::runtime_error("Corrupt meta section in saved run.");
    if (version >= 2) {
        if (!(in >> bTurn) || bTurn < 1) throw std::runtime_error("Invalid bossTurn in saved run.");
    }
    if (version >= 5) {
        if (!(in >> secondBio) || secondBio < -1 || secondBio > 4)
            throw std::runtime_error("Invalid secondBiome in saved run.");
    } else if (floor == 2) {
        secondBio = bio;
    }
    if (diff < 0 || diff > 3 || bio < 0 || bio > 4 || firstBio < 0 || firstBio > 1 ||
        floor < 1 || floor > 3 || ph < 0 || ph > 8 || end < 0 || end > 4 ||
        rIndex >= 9 || !std::isfinite(tLeft) || !std::isfinite(tLimit) || !std::isfinite(eHealth) ||
        tLeft < 0.f || tLimit < 0.f || eHealth < 0.f || sLeft < 0 || hidden < -1 || hidden > 2)
        throw std::runtime_error("Out-of-range metadata in saved run.");

    Player loadedPlayer;
    int heroInt = 0;
    if (!(in >> tag) || tag != "player" ||
        !(in >> heroInt >> loadedPlayer.reputation >> loadedPlayer.riddlesCorrect >>
          loadedPlayer.riddlesAnswered >> loadedPlayer.battlesWon >> loadedPlayer.battlesFought))
        throw std::runtime_error("Corrupt player section in saved run.");
    if (version >= 2) {
        if (!(in >> loadedPlayer.riddleStreak >> loadedPlayer.bestRiddleStreak) ||
            loadedPlayer.riddleStreak < 0 || loadedPlayer.bestRiddleStreak < loadedPlayer.riddleStreak)
            throw std::runtime_error("Invalid riddle streak in saved run.");
    }
    if (version >= 3) {
        if (!(in >> loadedPlayer.npcKills >> loadedPlayer.classAbilitiesUsed) ||
            loadedPlayer.npcKills < 0 || loadedPlayer.npcKills > 3 || loadedPlayer.classAbilitiesUsed < 0)
            throw std::runtime_error("Invalid extended player counters in saved run.");
    }
    if (version >= 4) {
        if (!(in >> loadedPlayer.strikesLeftover) || loadedPlayer.strikesLeftover < 0)
            throw std::runtime_error("Invalid leftover strikes counter in saved run.");
    }
    if (heroInt < 0 || heroInt > 3 || loadedPlayer.reputation < 0 || loadedPlayer.reputation > 9 ||
        loadedPlayer.riddlesCorrect < 0 || loadedPlayer.riddlesAnswered < loadedPlayer.riddlesCorrect ||
        loadedPlayer.battlesWon < 0 || loadedPlayer.battlesFought < loadedPlayer.battlesWon)
        throw std::runtime_error("Invalid player counters in saved run.");
    loadedPlayer.hero = static_cast<Hero>(heroInt);
    for (float& s : loadedPlayer.stats) {
        if (!(in >> s) || !std::isfinite(s) || s < 0.f)
            throw std::runtime_error("Invalid stat value in saved run.");
    }

    int repDelta = 0;
    std::array<float, 6> statDeltas{};
    if (!(in >> tag) || tag != "deltas" || !(in >> repDelta))
        throw std::runtime_error("Corrupt deltas section in saved run.");
    for (float& d : statDeltas) {
        if (!(in >> d) || !std::isfinite(d))
            throw std::runtime_error("Invalid stat delta in saved run.");
    }

    CombatExchangeResult exch;
    int exValid = 0, exAct = 0, exCrit = 0, exGuard = 0, exEDef = 0, exRet = 0, exPDef = 0, exCnt = 0;
    if (!(in >> tag) || tag != "exchange" ||
        !(in >> exValid >> exAct >> exCrit >> exGuard >> exEDef >> exRet >> exPDef >>
          exch.strikesSpent >> exch.damageDealt >> exCnt >> exch.counterDamage) ||
        exAct < 0 || exAct > 3 || !std::isfinite(exch.damageDealt) || !std::isfinite(exch.counterDamage))
        throw std::runtime_error("Corrupt combat exchange section in saved run.");
    if (version >= 2) {
        if (!(in >> exch.enemyHealed >> exch.strikesDrained) || !std::isfinite(exch.enemyHealed) || exch.strikesDrained < 0)
            throw std::runtime_error("Corrupt extended combat exchange section in saved run.");
    }
    exch.valid = exValid != 0;
    exch.action = static_cast<CombatAction>(exAct);
    exch.critical = exCrit != 0;
    exch.guarded = exGuard != 0;
    exch.enemyDefeated = exEDef != 0;
    exch.retreated = exRet != 0;
    exch.playerDefeated = exPDef != 0;
    exch.counterattacked = exCnt != 0;

    std::array<Room, 9> loadedRooms{};
    if (!(in >> tag) || tag != "rooms")
        throw std::runtime_error("Corrupt rooms section in saved run.");
    for (Room& r : loadedRooms) {
        int rInt = 0;
        if (!(in >> rInt) || rInt < 0 || rInt > 6)
            throw std::runtime_error("Invalid room type in saved run.");
        r = static_cast<Room>(rInt);
    }

    std::vector<Riddle> loadedRiddles = sourceRiddles_.at(static_cast<std::size_t>(floor - 1));
    std::size_t orderSize = 0;
    if (!(in >> tag) || tag != "riddle_order" || !(in >> orderSize) || orderSize < 8 || orderSize > loadedRiddles.size())
        throw std::runtime_error("Corrupt riddle_order section in saved run.");
    std::vector<std::size_t> loadedOrder(orderSize);
    std::vector<bool> seen(loadedRiddles.size(), false);
    for (std::size_t& idx : loadedOrder) {
        if (!(in >> idx) || idx >= loadedRiddles.size() || seen[idx])
            throw std::runtime_error("Invalid riddle_order index in saved run.");
        seen[idx] = true;
    }

    std::size_t activeCorrect = 0;
    std::array<std::string, 3> activeAnswers;
    if (!(in >> tag) || tag != "active_riddle" || !(in >> activeCorrect) || activeCorrect >= 3)
        throw std::runtime_error("Corrupt active_riddle section in saved run.");
    for (auto& ans : activeAnswers) {
        std::string hex;
        if (!(in >> hex)) throw std::runtime_error("Missing active riddle answer in saved run.");
        ans = decodeHex(hex);
    }
    loadedRiddles.at(loadedOrder.at(rIndex)).correct = activeCorrect;
    loadedRiddles.at(loadedOrder.at(rIndex)).answers = std::move(activeAnswers);

    std::string msgHex;
    if (!(in >> tag) || tag != "message" || !(in >> msgHex))
        throw std::runtime_error("Corrupt message section in saved run.");
    std::string loadedMessage = decodeHex(msgHex);

    std::mt19937 loadedRng;
    if (!(in >> tag) || tag != "rng")
        throw std::runtime_error("Missing rng section in saved run.");
    in >> std::ws;
    if (!(in >> loadedRng))
        throw std::runtime_error("Corrupt rng state in saved run.");

    std::string extra;
    if (in >> extra)
        throw std::runtime_error("Unexpected trailing data in saved run.");

    difficulty_ = static_cast<Difficulty>(diff);
    biome_ = static_cast<Biome>(bio);
    firstBiome_ = static_cast<Biome>(firstBio);
    secondBiome_ = secondBio >= 0 ? std::optional<Biome>(static_cast<Biome>(secondBio)) : std::nullopt;
    floorNumber_ = floor;
    phase_ = static_cast<Phase>(ph);
    ending_ = static_cast<Ending>(end);
    started_ = started != 0;
    seed_ = seedVal;
    runSerial_ = serialVal;
    roomIndex_ = rIndex;
    chestUnlocked_ = chest != 0;
    classAbilityUsed_ = abilityUsed != 0;
    timeLeft_ = tLeft;
    timeLimit_ = tLimit;
    enemyHealth_ = eHealth;
    strikesLeft_ = sLeft;
    bossTurn_ = bTurn;
    hiddenAnswer_ = hidden >= 0 ? std::optional<std::size_t>(static_cast<std::size_t>(hidden)) : std::nullopt;
    player_ = loadedPlayer;
    lastReputationDelta_ = repDelta;
    lastStatDeltas_ = statDeltas;
    lastCombatExchange_ = exch;
    rooms_ = loadedRooms;
    riddles_ = std::move(loadedRiddles);
    riddleOrder_ = std::move(loadedOrder);
    message_ = std::move(loadedMessage);
    rng_ = loadedRng;
}
} // namespace dungeon
