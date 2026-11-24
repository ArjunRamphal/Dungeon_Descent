#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace dungeon {

enum class Hero { Warrior, Ranger, Mage, Rogue };
enum class Difficulty { Easy, Medium, Hard, Impossible };
enum class Biome { Ice, Jungle, Desert, Ghost, Lava };
enum class Room { Library, Chest, Battle, Encounter, Respite, Shop, Boss };
enum class Phase { Room, Outcome, Riddle, RiddleOutcome, Loot, Combat, Complete, Defeat, Ending };
enum class Ending { None, Good, True, Bad, Perfect };
enum Stat { Strength, Wisdom, Observation, Agility, Health, Accuracy };

enum class CombatAction { StandardStrike, HeavyStrike, GuardStrike, ClassAbility };

struct Player {
    Hero hero = Hero::Warrior;
    std::array<float, 6> stats{};
    int reputation = 0;
    int riddlesCorrect = 0;
    int riddlesAnswered = 0;
    int riddleStreak = 0;
    int bestRiddleStreak = 0;
    int battlesWon = 0;
    int battlesFought = 0;
    int npcKills = 0;
    int classAbilitiesUsed = 0;
    int strikesLeftover = 0;
};

struct ScoreBreakdown {
    int statsPoints = 0;
    int healthPoints = 0;
    int strikesPoints = 0;
    int timePoints = 0;
    int total = 0;
};

struct Riddle {
    std::string question;
    std::array<std::string, 3> answers;
    std::size_t correct = 0;
    std::optional<Biome> requiredBiome;
};

struct BossIntent {
    bool active = false;
    bool telegraphed = false; // True when a special boss intent triggers on THIS turn
    std::string name;
    std::string detail;
    float counterMultiplier = 1.f;
    float damageTakenMultiplier = 1.f;
    bool drainsStrike = false;
    float lifestealRatio = 0.f;
    bool heavyPierces = false;
    bool guardBypasses = false;
};

struct CombatActionPreview {
    std::string title;
    std::string effect;
    int strikeCost = 1;
    float minDamage = 0.f;
    float critDamage = 0.f;
    float expectedCounter = 0.f;
    bool available = false;
};

struct CombatMathSummary {
    float baseDamage = 0.f;
    float heavyDamage = 0.f;
    float guardDamage = 0.f;
    float abilityDamage = 0.f;
    float critChance = 0.f;       // [0.0, 1.0] = Accuracy / 35 clamped
    float critMultiplier = 2.f;
    float critDamage = 0.f;
    float expectedDamage = 0.f;
    int minStrikes = 3;
    int maxStrikes = 5;
    float expectedStrikes = 4.f;
    float enemyHealth = 0.f;
    float enemyMaxHealth = 0.f;
    float enemyCounterDamage = 0.f;
    float guardCounterDamage = 0.f;
};

struct CombatExchangeResult {
    bool valid = false;
    CombatAction action = CombatAction::StandardStrike;
    bool critical = false;
    bool guarded = false;
    bool enemyDefeated = false;
    bool retreated = false;
    bool playerDefeated = false;
    int strikesSpent = 0;
    float damageDealt = 0.f;
    bool counterattacked = false;
    float counterDamage = 0.f;
    float enemyHealed = 0.f;
    int strikesDrained = 0;
};

// SFML-independent rules. The renderer sends actions here; it never mutates stats.
class DungeonRun {
public:
    explicit DungeonRun(const std::filesystem::path& assets);
    void start(Hero hero, Biome biome, std::uint32_t seed, Difficulty difficulty = Difficulty::Easy);
    bool descend(Biome biome);
    void choose(std::size_t choice);
    void advance();
    void answer(std::size_t choice);
    void tick(float seconds);
    void attack();
    void attack(CombatAction action);

    const Player& player() const { return player_; }
    Difficulty difficulty() const { return difficulty_; }
    Biome biome() const { return biome_; }
    Biome firstBiome() const { return firstBiome_; }
    int floorNumber() const { return floorNumber_; }
    bool canDescend() const { return floorNumber_ < 3 && phase_ == Phase::Complete; }
    bool inProgress() const { return started_ && phase_ != Phase::Defeat && phase_ != Phase::Ending; }
    bool canSuspend() const { return inProgress() && phase_ != Phase::Riddle; }
    Ending ending() const { return ending_; }
    ScoreBreakdown scoreBreakdown(int playTimeSeconds = -1) const;
    int score(int playTimeSeconds = -1) const;
    std::uint32_t seed() const { return seed_; }
    std::uint64_t runSerial() const { return runSerial_; }
    Phase phase() const { return phase_; }
    Room room() const { return rooms_.at(roomIndex_); }
    int roomNumber() const { return static_cast<int>(roomIndex_) + 1; }
    const std::array<Room, 9>& route() const { return rooms_; }
    const std::string& message() const { return message_; }
    const Riddle& riddle() const { return riddles_.at(riddleOrder_.at(roomIndex_)); }
    float timeLeft() const { return timeLeft_; }
    float timeLimit() const { return timeLimit_; }
    float enemyHealth() const { return enemyHealth_; }
    float enemyMaxHealth() const;
    float enemyDamage() const;
    bool answerAvailable(std::size_t choice) const { return choice < 3 && hiddenAnswer_ != choice; }
    bool hasObservationHint() const { return hiddenAnswer_.has_value(); }
    int strikesLeft() const { return strikesLeft_; }
    int bossTurn() const { return bossTurn_; }
    BossIntent bossIntent() const;
    CombatActionPreview previewCombatAction(CombatAction action) const;
    bool classAbilityAvailable() const { return phase_ == Phase::Combat && !classAbilityUsed_ && strikesLeft_ >= 1; }
    bool classAbilityUsedInBattle() const { return classAbilityUsed_; }
    bool combatChoiceAvailable(std::size_t choice) const;
    CombatMathSummary combatSummary() const;
    const CombatExchangeResult& lastCombatExchange() const { return lastCombatExchange_; }
    const std::array<float, 6>& lastStatDeltas() const { return lastStatDeltas_; }
    int lastReputationDelta() const { return lastReputationDelta_; }
    std::vector<std::string> choices() const;
    std::size_t displayChoiceFor(std::size_t canonicalChoice) const;
    std::string image() const;
    std::string storyFile() const;
    std::string title() const;
    std::string enemyName() const;
    std::string musicFile() const;
    std::string mapImage() const;
    std::string roomTitle(Room type) const;
    std::string endingName() const;
    std::string endingKey() const;

    std::string serializeState() const;
    void deserializeState(const std::string& serialized);

    static Player makePlayer(Hero hero);
    static Ending resolveEnding(bool malvelDefeated, const Player& player);
    static std::string endingName(Ending ending);
    static std::string endingKey(Ending ending);
    static std::string endingImage(Ending ending);
    static std::string endingStoryFile(Ending ending);
    static std::string endingMusicFile(Ending ending);
    static std::string heroName(Hero hero);
    static std::string difficultyName(Difficulty difficulty);
    static std::string portrait(Hero hero);
    static std::string ability(Hero hero);
    static std::string combatAbilityName(Hero hero);
    static std::string combatAbilityDescription(Hero hero);
    static std::string statName(std::size_t stat);
    static std::string statTooltip(Stat stat);
    static std::array<std::string, 6> statTooltips();
    static std::string roomName(Room room);
    static std::string biomeName(Biome biome);
    static std::string biomeKey(Biome biome);
    static std::string bossImage(Biome biome);
    static std::string readText(const std::filesystem::path& path);
    static std::uint32_t parseSeed(const std::string& text);
    static std::uint32_t dailyRunSeed(int year, int month, int day);
    static std::string dailyRunDateString();
    static std::uint32_t dailyRunSeedForToday();
    const std::vector<Riddle>& riddles() const { return riddles_; }
    const std::vector<std::size_t>& riddleOrder() const { return riddleOrder_; }
    const std::array<std::vector<Riddle>, 3>& sourceRiddles() const { return sourceRiddles_; }
    std::optional<Biome> secondBiome() const { return secondBiome_; }

private:
    void enterRoom();
    void prepareFloor();
    void beginRiddle();
    void resolveRiddle(bool correct, bool timeout);
    void addAll(float amount);
    void subtractAll(float amount);
    void addStat(Stat stat, float amount);
    void recordStatDeltas(const std::array<float, 6>& beforeStats, int beforeReputation);
    void clearStatDeltas();
    void outcome(std::string text);
    void finish(bool malvelDefeated);
    std::vector<std::size_t> roomChoiceOrder() const;
    Stat primaryCombatStat() const;
    std::string biomeKey() const { return biomeKey(biome_); }
    std::filesystem::path assets_;
    Player player_;
    Difficulty difficulty_ = Difficulty::Easy;
    Biome biome_ = Biome::Ice;
    Biome firstBiome_ = Biome::Ice;
    std::optional<Biome> secondBiome_;
    int floorNumber_ = 1;
    Phase phase_ = Phase::Room;
    Ending ending_ = Ending::None;
    bool started_ = false;
    std::uint32_t seed_ = 0;
    std::uint64_t runSerial_ = 0;
    std::array<Room, 9> rooms_{};
    std::size_t roomIndex_ = 0;
    std::vector<Riddle> riddles_;
    std::array<std::vector<Riddle>, 3> sourceRiddles_;
    std::optional<std::size_t> hiddenAnswer_;
    std::vector<std::size_t> riddleOrder_;
    std::mt19937 rng_;
    bool chestUnlocked_ = false;
    bool classAbilityUsed_ = false;
    float timeLeft_ = 0.f;
    float timeLimit_ = 0.f;
    float enemyHealth_ = 0.f;
    int strikesLeft_ = 0;
    int bossTurn_ = 1;
    std::string message_;
    CombatExchangeResult lastCombatExchange_{};
    std::array<float, 6> lastStatDeltas_{};
    int lastReputationDelta_ = 0;
};
} // namespace dungeon
