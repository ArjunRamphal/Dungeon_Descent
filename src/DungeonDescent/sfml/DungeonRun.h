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
enum class Biome { Ice, Jungle, Desert, Ghost, Lava };
enum class Room { Library, Chest, Battle, Encounter, Respite, Shop, Boss };
enum class Phase { Room, Outcome, Riddle, RiddleOutcome, Loot, Combat, Complete, Defeat, Ending };
enum class Ending { None, Good, True, Bad };
enum Stat { Strength, Wisdom, Observation, Agility, Health, Accuracy };

struct Player {
    Hero hero = Hero::Warrior;
    std::array<float, 6> stats{};
    int reputation = 0;
    int riddlesCorrect = 0;
    int riddlesAnswered = 0;
    int battlesWon = 0;
    int battlesFought = 0;
};

struct Riddle {
    std::string question;
    std::array<std::string, 3> answers;
    std::size_t correct = 0;
};

// SFML-independent rules. The renderer sends actions here; it never mutates stats.
class DungeonRun {
public:
    explicit DungeonRun(const std::filesystem::path& assets);
    void start(Hero hero, Biome biome, std::uint32_t seed);
    bool descend(Biome biome);
    void choose(std::size_t choice);
    void advance();
    void answer(std::size_t choice);
    void tick(float seconds);
    void attack();

    const Player& player() const { return player_; }
    Biome biome() const { return biome_; }
    Biome firstBiome() const { return firstBiome_; }
    int floorNumber() const { return floorNumber_; }
    bool canDescend() const { return floorNumber_ < 3 && phase_ == Phase::Complete; }
    bool inProgress() const { return started_ && phase_ != Phase::Defeat && phase_ != Phase::Ending; }
    Ending ending() const { return ending_; }
    int score() const;
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
    bool answerAvailable(std::size_t choice) const { return choice < 3 && hiddenAnswer_ != choice; }
    bool hasObservationHint() const { return hiddenAnswer_.has_value(); }
    int strikesLeft() const { return strikesLeft_; }
    std::vector<std::string> choices() const;
    std::string image() const;
    std::string storyFile() const;
    std::string title() const;
    std::string enemyName() const;
    std::string musicFile() const;
    std::string mapImage() const;
    std::string roomTitle(Room type) const;
    std::string endingName() const;
    std::string endingKey() const;

    static Player makePlayer(Hero hero);
    static Ending resolveEnding(bool malvelDefeated, const Player& player);
    static std::string heroName(Hero hero);
    static std::string portrait(Hero hero);
    static std::string ability(Hero hero);
    static std::string statName(std::size_t stat);
    static std::string roomName(Room room);
    static std::string biomeName(Biome biome);
    static std::string biomeKey(Biome biome);
    static std::string bossImage(Biome biome);
    static std::string readText(const std::filesystem::path& path);
    const std::vector<Riddle>& riddles() const { return riddles_; }

private:
    void enterRoom();
    void prepareFloor();
    void beginRiddle();
    void resolveRiddle(bool correct, bool timeout);
    void addAll(float amount);
    void subtractAll(float amount);
    void addStat(Stat stat, float amount);
    void outcome(std::string text);
    void finish(bool malvelDefeated);
    std::string biomeKey() const { return biomeKey(biome_); }
    std::filesystem::path assets_;
    Player player_;
    Biome biome_ = Biome::Ice;
    Biome firstBiome_ = Biome::Ice;
    int floorNumber_ = 1;
    Phase phase_ = Phase::Room;
    Ending ending_ = Ending::None;
    bool started_ = false;
    std::array<Room, 9> rooms_{};
    std::size_t roomIndex_ = 0;
    std::vector<Riddle> riddles_;
    std::array<std::vector<Riddle>, 3> sourceRiddles_;
    std::optional<std::size_t> hiddenAnswer_;
    std::vector<std::size_t> riddleOrder_;
    std::mt19937 rng_;
    bool chestUnlocked_ = false;
    float timeLeft_ = 0.f;
    float timeLimit_ = 0.f;
    float enemyHealth_ = 0.f;
    int strikesLeft_ = 0;
    std::string message_;
};
} // namespace dungeon
