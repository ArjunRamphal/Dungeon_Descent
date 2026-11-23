#pragma once

#include "DungeonRun.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dungeon {

enum class PrequelAct : int {
    Act1_Eldred = 0,
    Act2_Malvel = 1,
    Act3_Sophie = 2
};

enum class PrequelPhase {
    ActIntro,
    Scene,
    Parley,
    Combat,
    Outcome,
    ActComplete,
    Defeat,
    Epilogue
};

enum class PrequelCoda {
    None = 0,
    HollowCrown = 1, // High Obsidian Fury / low Remorse -> cold resolve before stepping into the cage
    LastDoubt = 2    // High Sophie's Remorse / Falkreath's Legacy -> lingering hesitation in her diary
};

struct PrequelCharacter {
    PrequelAct act = PrequelAct::Act1_Eldred;
    std::string name;
    std::string title;
    std::string weapon;
    std::string portrait;
    std::string abilityName;
    std::string abilityDesc;
    float power = 8.f;    // Governs base attack damage
    float resolve = 6.f;  // Extends Parley timer & reduces retaliation
    float cunning = 4.f;  // Governs crit chance & eliminates weak parley bluffs
    float agility = 4.f;  // Adds bonus strikes per battle
    float hp = 42.f;
    float maxHp = 42.f;
};

struct PrequelParley {
    std::string speaker;
    std::string prompt;
    std::array<std::string, 3> options{};
    std::size_t bestIndex = 0;
    std::size_t hiddenIndex = 3; // 3 = none eliminated
    float timeLimit = 18.f;
    float timeLeft = 18.f;
    std::string successText;
    std::string failText;
};

struct PrequelEnemy {
    std::string name;
    std::string title;
    std::string image;
    float hp = 0.f;
    float maxHp = 0.f;
    float counterDamage = 0.f;
    int strikesLeft = 0;
    int maxStrikes = 0;
    int turn = 0;
    bool boss = false;
    bool abilityUsedThisBattle = false;
    bool rallyGuardActive = false;
    BossIntent intent;
};

struct PrequelChoice {
    std::string label;
};

class PrequelRun {
public:
    explicit PrequelRun(std::filesystem::path assetsRoot = {});

    void start();
    void restartAct();

    void advance();
    void choose(std::size_t index);
    void answerParley(std::size_t index);
    void tickParley(float dt);
    void attack(CombatAction action = CombatAction::StandardStrike);

    PrequelAct act() const { return act_; }
    int actNumber() const { return static_cast<int>(act_) + 1; }
    int chapterNumber() const { return chapterIndex_ + 1; } // 1..15 overall
    int actChapterNumber() const { return (chapterIndex_ % 5) + 1; } // 1..5 within current act
    PrequelPhase phase() const { return phase_; }
    PrequelCoda coda() const { return coda_; }

    const PrequelCharacter& character() const { return character_; }
    const PrequelEnemy& enemy() const { return enemy_; }
    const PrequelParley& parley() const { return parley_; }
    const std::vector<PrequelChoice>& choices() const { return choices_; }
    const CombatExchangeResult& lastCombatExchange() const { return lastExchange_; }

    const std::string& heading() const { return heading_; }
    const std::string& subheading() const { return subheading_; }
    const std::string& body() const { return body_; }
    const std::string& sceneImage() const { return sceneImage_; }
    std::string musicFile() const;

    bool combatChoiceAvailable(CombatAction action) const;
    CombatActionPreview previewCombatAction(CombatAction action) const;
    bool parleyOptionAvailable(std::size_t index) const;

    int legacyThread() const { return legacy_; }
    int furyThread() const { return fury_; }
    int remorseThread() const { return remorse_; }
    int abilitiesUsed() const { return abilitiesUsed_; }
    bool ironChronicleEligible() const { return abilitiesUsed_ == 0; }
    int battlesWon() const { return battlesWon_; }
    int parleysWon() const { return parleysWon_; }
    std::uint64_t serial() const { return serial_; }

    static std::string actTitle(PrequelAct act);
    static std::string codaTitle(PrequelCoda coda);

private:
    void setupAct(PrequelAct act);
    void loadChapter(int index);
    void startParley(const PrequelParley& p);
    void startCombat(const std::string& name, const std::string& title, const std::string& img,
                     float hp, float counterDmg, int baseStrikes, bool boss,
                     std::string victoryText, int nextAfterWin);
    void updateBossIntent();
    void finalizeCampaign();

    std::filesystem::path assets_;
    PrequelAct act_ = PrequelAct::Act1_Eldred;
    int chapterIndex_ = 0; // 0..14
    PrequelPhase phase_ = PrequelPhase::ActIntro;
    PrequelCoda coda_ = PrequelCoda::None;

    PrequelCharacter character_;
    PrequelEnemy enemy_;
    PrequelParley parley_;
    std::vector<PrequelChoice> choices_;
    CombatExchangeResult lastExchange_;

    std::string heading_;
    std::string subheading_;
    std::string body_;
    std::string sceneImage_;
    std::string pendingVictoryText_;
    int pendingNextChapter_ = 0;
    bool pendingAfterOutcomeCombat_ = false;
    bool pendingAfterOutcomeParley_ = false;
    PrequelParley queuedParley_;
    PrequelEnemy queuedEnemy_;
    std::string queuedVictoryText_;
    int queuedNextChapter_ = 0;

    // Cross-Act Chronicle Threads
    int legacy_ = 0;  // Falkreath's Legacy (Act I protection & truth)
    int fury_ = 0;    // Obsidian Fury (Malvel's wrath & ruthless vengeance)
    int remorse_ = 0; // Sophie's Remorse (mercy & lingering humanity)

    // Snapshot at Act start so "Retry Act" restores cleanly
    int actStartLegacy_ = 0;
    int actStartFury_ = 0;
    int actStartRemorse_ = 0;
    int actStartAbilities_ = 0;
    int actStartBattles_ = 0;
    int actStartParleys_ = 0;

    int abilitiesUsed_ = 0;
    int battlesWon_ = 0;
    int parleysWon_ = 0;
    std::uint64_t serial_ = 0;
    std::uint32_t rngState_ = 0x5EED1337u;

    float nextUnitFloat();
};

} // namespace dungeon
