#pragma once
#include "DungeonRun.h"
#include "Leaderboards.h"
#include "SoundEffects.h"
#include "../portable/SuspendSave.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace dungeon {
struct LaunchOptions {
    std::filesystem::path assets;
    bool audio = true;
    bool skipIntro = false;
    bool smoke = false;
    std::filesystem::path screenshot;
    std::filesystem::path dataDirectory;
};
class GameApp {
    friend class GameAppTests;
public:
    explicit GameApp(LaunchOptions options);
    int run();
private:
    enum class Screen { Intro, Menu, Introduction, Heroes, Difficulty, Biomes, Game, Lore, Results, Leaderboard };
    enum class Overlay { None, Pause, Map, Quit };
    struct Button { sf::FloatRect bounds; std::string id; bool enabled; };
    struct StatHoverTarget { sf::FloatRect bounds; Stat stat; };
    struct FloatingBadge {
        std::string text;
        sf::Color color;
        float timer = 0.f;
        float duration = 1.6f;
        float xOffset = 0.f;
    };

    void openWindow(bool fullscreen);
    void handle(const sf::Event& event);
    void action(const std::string& id);
    void update(float dt);
    bool timedRiddle() const { return screen_ == Screen::Game && floor_.phase() == Phase::Riddle; }
    void render();
    void drawMenu();
    void drawHeroes();
    void drawDifficulties();
    void drawBiomes();
    void drawGame();
    void drawOverlay();
    void drawLore();
    void drawEnding();
    void drawResults();
    void drawLeaderboard(bool readOnlyBrowse, bool drawBackground = false);
    void prepareResults();
    void submitScore();
    void refreshSuspendPreview();
    void applyResolvedSeed();
    void triggerCombatFeedback(const CombatExchangeResult& exchange);
    void recordPhaseFeedback(Phase previousPhase, bool previousChestUnlocked);
    bool hasEndingResult() const { return floor_.phase() == Phase::Ending && floor_.ending() != Ending::None; }
    void panel(sf::FloatRect rect, sf::Color fill, sf::Color outline);
    void label(const std::string& text, sf::Vector2f position, unsigned size = 20,
               sf::Color color = sf::Color::White, bool heading = false);
    void button(const std::string& id, const std::string& text, sf::FloatRect rect, bool enabled = true,
                std::optional<bool> activeTab = std::nullopt);
    void image(const std::string& name, sf::FloatRect rect, bool cover = false, sf::Color tint = sf::Color::White);
    void paragraph(const std::string& text, sf::FloatRect rect, unsigned size = 19);
    void reading(const std::string& text, sf::FloatRect rect);
    void bar(float ratio, sf::FloatRect rect, sf::Color color);
    void playMusic(const std::string& file);
    void playIntroNarration();
    void enter(Screen screen);
    void syncStory();
    std::vector<sf::String> wrap(const std::string& text, unsigned size, float width);
    std::optional<sf::Vector2f> pointer(sf::Vector2i pixel) const;
    sf::Texture& texture(const std::string& name);
    sf::Texture& animationTexture(const std::string& name);
    static bool isAnimationFramePath(const std::string& name);
    static sf::String utf8(const std::string& text);

    LaunchOptions options_;
    std::filesystem::path dataDir_;
    DungeonRun floor_;
    Leaderboards leaderboards_;
    portable::SuspendSave suspendSave_;
    std::optional<portable::SuspendedRun> suspendPreview_;
    Leaderboards::Tables leaderboardTables_;
    Difficulty leaderboardDifficulty_ = Difficulty::Easy;
    std::uint64_t resultsRunSerial_ = 0;
    std::optional<std::uint64_t> leaderboardEntryId_;
    std::string initials_, leaderboardStatus_;
    bool entryPending_ = false;
    sf::RenderWindow window_;
    sf::RenderTexture canvas_;
    sf::Font bodyFont_, titleFont_;

    // Feature #12: Separated persistent UI/room/portrait cache vs reusable animation frame buffer.
    std::map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::deque<std::string> textureOrder_;
    sf::Texture animationFrameTexture_;
    std::string animationFrameName_;
    std::uint64_t persistentTextureLoads_ = 0;
    std::uint64_t persistentTextureHits_ = 0;
    std::uint64_t persistentTextureEvictions_ = 0;
    std::uint64_t animationTextureLoads_ = 0;
    std::uint64_t animationTextureHits_ = 0;

    std::unique_ptr<sf::Music> music_;
    std::unique_ptr<sf::Music> narration_;
    std::string musicFile_;
    SoundEffects sfx_;
    bool muted_ = false, fullscreen_ = false;
    bool choosingSecondFloor_ = false;
    Screen screen_ = Screen::Intro, loreReturn_ = Screen::Menu, leaderboardReturn_ = Screen::Menu;
    Overlay overlay_ = Overlay::None;
    Hero selectedHero_ = Hero::Warrior, loreHero_ = Hero::Warrior;
    Difficulty selectedDifficulty_ = Difficulty::Easy;
    std::vector<Button> buttons_;
    std::size_t selectedButton_ = 0;
    std::string readingText_;
    std::vector<sf::String> readingLines_;
    int page_ = 0, pageCount_ = 1;
    float animationTime_ = 0, attackDelay_ = 0;

    // Feature #4: Custom seed & Daily Run state.
    std::string seedInput_;
    bool editingSeed_ = false;
    bool dailyRunSelected_ = false;
    bool activeDailyRun_ = false;
    std::string activeDailyDate_;

    // Feature #2: Hovered/focused stat tooltip tracking.
    std::vector<StatHoverTarget> statHoverTargets_;
    std::optional<Stat> hoveredStat_;
    std::optional<Stat> focusedStat_;

    // Feature #10: Visual combat feedback & stat change indicators.
    float flash_ = 0.f;
    float flashDuration_ = 0.25f;
    sf::Color flashColor_{223, 187, 119};
    float shakeTime_ = 0.f;
    float shakeAmplitude_ = 0.f;
    std::vector<FloatingBadge> combatBadges_;

    // Feature #9: Low-time riddle countdown tick tracking.
    int lastTimerTickSecond_ = -1;

    float playTime_ = 0;
    std::string roomStory_;
    int storyRoom_ = -1;
    std::uint32_t seed_ = 0;
    std::string statusBanner_;
    unsigned smokeFrames_ = 0;
};
}
