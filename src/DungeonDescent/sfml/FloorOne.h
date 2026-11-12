#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace dungeon {

enum class Hero { Warrior, Ranger, Mage, Rogue };
enum class Biome { Ice, Jungle };
enum class Room { Library, Chest, Battle, Encounter, Respite, Shop, Boss };
enum class Phase { Room, Outcome, Riddle, RiddleOutcome, Loot, Combat, Complete, Defeat };
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
class FloorOne {
public:
    explicit FloorOne(const std::filesystem::path& assets);
    void start(Hero hero, Biome biome, std::uint32_t seed);
    void choose(std::size_t choice);
    void advance();
    void answer(std::size_t choice);
    void tick(float seconds);
    void attack();
    void retryRoom();

    const Player& player() const { return player_; }
    Biome biome() const { return biome_; }
    Phase phase() const { return phase_; }
    Room room() const { return rooms_.at(roomIndex_); }
    int roomNumber() const { return static_cast<int>(roomIndex_) + 1; }
    const std::array<Room, 9>& route() const { return rooms_; }
    const std::string& message() const { return message_; }
    const Riddle& riddle() const { return riddles_.at(riddleOrder_.at(roomIndex_)); }
    float timeLeft() const { return timeLeft_; }
    float timeLimit() const { return timeLimit_; }
    float enemyHealth() const { return enemyHealth_; }
    float enemyMaxHealth() const { return room() == Room::Boss ? 144.f : 32.f; }
    int strikesLeft() const { return strikesLeft_; }
    std::vector<std::string> choices() const;
    std::string image() const;
    std::string storyFile() const;
    std::string title() const;
    std::string enemyName() const;

    static Player makePlayer(Hero hero);
    static std::string heroName(Hero hero);
    static std::string portrait(Hero hero);
    static std::string ability(Hero hero);
    static std::string statName(std::size_t stat);
    static std::string roomName(Room room);
    static std::string biomeName(Biome biome);
    static std::string readText(const std::filesystem::path& path);
    const std::vector<Riddle>& riddles() const { return riddles_; }

private:
    void enterRoom();
    void beginRiddle();
    void resolveRiddle(bool correct, bool timeout);
    void addAll(float amount);
    void subtractAll(float amount);
    void addStat(Stat stat, float amount);
    void outcome(std::string text);
    std::string biomeKey() const;
    std::filesystem::path assets_;
    Player player_;
    Player checkpoint_;
    Biome biome_ = Biome::Ice;
    Phase phase_ = Phase::Room;
    std::array<Room, 9> rooms_{};
    std::size_t roomIndex_ = 0;
    std::vector<Riddle> riddles_;
    std::vector<Riddle> sourceRiddles_;
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
