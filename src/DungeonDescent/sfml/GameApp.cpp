#include "GameApp.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace dungeon {
namespace {
constexpr float Width = 1440, Height = 900;
constexpr std::size_t MaxPersistentTextures = 24;
const sf::Color Background{10, 15, 22}, Surface{17, 26, 36}, Border{48, 65, 78};
const sf::Color Gold{223, 187, 119}, Ink{226, 231, 233}, Muted{153, 171, 181};
const sf::Color Ice{123, 198, 224}, Leaf{119, 198, 147}, Sand{224, 181, 113}, Mist{191, 166, 226};
const sf::Color PositiveGreen{112, 214, 144}, NegativeRed{234, 108, 108}, GuardCyan{114, 202, 232};
sf::Color biomeAccent(Biome biome) {
    switch (biome) {
    case Biome::Ice: return Ice;
    case Biome::Jungle: return Leaf;
    case Biome::Desert: return Sand;
    case Biome::Ghost: return Mist;
    case Biome::Lava: return {238, 133, 89};
    }
    return Gold;
}
std::string num(float n, unsigned precision = 1) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << n;
    auto result = out.str();
    if (precision > 1) {
        while (!result.empty() && result.back() == '0') result.pop_back();
        if (!result.empty() && result.back() == '.') result.pop_back();
    }
    return result;
}
std::string signedDelta(float delta) {
    const std::string prefix = delta > 0 ? "+" : "";
    return prefix + num(delta, 2);
}
std::string roomTypeBadge(Room room) {
    switch (room) {
    case Room::Library: return "LIB";
    case Room::Chest: return "CHEST";
    case Room::Battle: return "FIGHT";
    case Room::Encounter: return "EVENT";
    case Room::Respite: return "REST";
    case Room::Shop: return "SHOP";
    case Room::Boss: return "BOSS";
    }
    return "ROOM";
}
std::string roomTypeDescription(Room room, int floorNumber) {
    switch (room) {
    case Room::Library: return "Ancient tomes and forbidden records followed by a timed riddle.";
    case Room::Chest: return "A runic lock guarded by the riddle keeper's test.";
    case Room::Battle: return "An optional clash in the dark, or a chance to slip past.";
    case Room::Encounter:
        return floorNumber == 1 ? "A captive warrior in chains awaiting mercy or ruthlessness."
             : floorNumber == 2 ? "A wounded butler clutching a mysterious vial."
                                : "Malvel's cornered minions in the volcanic halls.";
    case Room::Respite: return "A rare sanctuary to recover strength or search the ruins.";
    case Room::Shop: return "A mysterious merchant offering elixirs, draughts and lore.";
    case Room::Boss: return floorNumber == 3 ? "Malvel awaits in the final volcanic chamber."
                                             : "The floor guardian blocks the stairway below.";
    }
    return "";
}
}
sf::String GameApp::utf8(const std::string& text) { return sf::String::fromUtf8(text.begin(), text.end()); }
bool GameApp::isAnimationFramePath(const std::string& name) {
    return name.rfind("images/startscreen/startscreen", 0) == 0 || name.rfind("candle/candle", 0) == 0;
}
GameApp::GameApp(LaunchOptions options)
    : options_(std::move(options)),
      dataDir_(options_.dataDirectory.empty() ? Leaderboards::dataDirectory() : options_.dataDirectory),
      floor_(options_.assets),
      leaderboards_(dataDir_ / "leaderboards.tsv"),
      suspendSave_(dataDir_ / "suspend.sav"),
      sfx_(options_.audio) {
    if (!bodyFont_.openFromFile(options_.assets / "fonts/Arial.ttf") ||
        !titleFont_.openFromFile(options_.assets / "fonts/VIKING-N.TTF"))
        throw std::runtime_error("Cannot load fonts from " + options_.assets.string());
    if (!canvas_.resize({static_cast<unsigned>(Width), static_cast<unsigned>(Height)}))
        throw std::runtime_error("Cannot create the game canvas.");
    canvas_.setSmooth(true);
    openWindow(true);
    seed_ = std::random_device{}();
    refreshSuspendPreview();
    if (options_.smoke) {
        floor_.start(Hero::Warrior, Biome::Ice, 7);
        screen_ = Screen::Game;
        syncStory();
    } else if (options_.skipIntro) enter(Screen::Menu);
    else enter(Screen::Intro);
}
void GameApp::refreshSuspendPreview() {
    try {
        suspendPreview_ = suspendSave_.peek();
    } catch (const std::exception&) {
        suspendPreview_.reset();
    }
}
void GameApp::applyResolvedSeed() {
    if (dailyRunSelected_) {
        activeDailyRun_ = true;
        activeDailyDate_ = DungeonRun::dailyRunDateString();
        seed_ = DungeonRun::dailyRunSeedForToday();
    } else if (!seedInput_.empty()) {
        activeDailyRun_ = false;
        activeDailyDate_.clear();
        seed_ = DungeonRun::parseSeed(seedInput_);
    } else {
        activeDailyRun_ = false;
        activeDailyDate_.clear();
        ++seed_;
    }
}
void GameApp::openWindow(bool fullscreen) {
    fullscreen_ = fullscreen;
    if (fullscreen) window_.create(sf::VideoMode::getDesktopMode(), "Dungeon Descent", sf::State::Fullscreen);
    else {
        const auto desktop = sf::VideoMode::getDesktopMode().size;
        const unsigned w = std::min(1280u, desktop.x > 80 ? desktop.x - 80 : desktop.x);
        const unsigned h = std::min(800u, desktop.y > 100 ? desktop.y - 100 : desktop.y);
        window_.create(sf::VideoMode({w, h}), "Dungeon Descent", sf::Style::Default);
        window_.setMinimumSize(sf::Vector2u{800, 500});
    }
    window_.setVerticalSyncEnabled(true);
    window_.setFramerateLimit(60);
    window_.setKeyRepeatEnabled(false);
}
int GameApp::run() {
    sf::Clock clock;
    while (window_.isOpen()) {
        // Charge elapsed time before accepting answers, including the first
        // input after the window was unfocused or the event loop was delayed.
        update(clock.restart().asSeconds());
        while (const auto event = window_.pollEvent()) handle(*event);
        if (!window_.isOpen()) break;
        render();
        if (options_.smoke && ++smokeFrames_ >= 5) {
            if (!options_.screenshot.empty() && !canvas_.getTexture().copyToImage().saveToFile(options_.screenshot))
                throw std::runtime_error("Cannot save smoke-test screenshot.");
            window_.close();
        }
    }
    return 0;
}
std::optional<sf::Vector2f> GameApp::pointer(sf::Vector2i pixel) const {
    const auto size = window_.getSize();
    const float scale = std::min(size.x / Width, size.y / Height);
    const sf::Vector2f offset{(size.x - Width * scale) / 2, (size.y - Height * scale) / 2};
    const sf::Vector2f point{(pixel.x - offset.x) / scale, (pixel.y - offset.y) / scale};
    if (point.x < 0 || point.y < 0 || point.x >= Width || point.y >= Height) return std::nullopt;
    return point;
}
void GameApp::triggerCombatFeedback(const CombatExchangeResult& exchange) {
    if (!exchange.valid) return;
    combatBadges_.clear();
    if (exchange.critical) {
        sfx_.play(SfxId::CritHit, muted_);
        flashColor_ = {255, 215, 90};
        flashDuration_ = 0.34f;
        flash_ = flashDuration_;
        shakeTime_ = 0.28f;
        shakeAmplitude_ = 9.f;
        combatBadges_.push_back({"CRIT! -" + num(exchange.damageDealt, 2) + " HP", {255, 224, 112}, 1.5f, 1.5f, -90.f});
    } else if (exchange.guarded) {
        sfx_.play(SfxId::GuardHit, muted_);
        flashColor_ = GuardCyan;
        flashDuration_ = 0.26f;
        flash_ = flashDuration_;
        shakeTime_ = 0.16f;
        shakeAmplitude_ = 4.5f;
        combatBadges_.push_back({"GUARD -" + num(exchange.damageDealt, 2) + " HP", GuardCyan, 1.4f, 1.4f, -90.f});
    } else {
        sfx_.play(SfxId::Strike, muted_);
        flashColor_ = Gold;
        flashDuration_ = 0.22f;
        flash_ = flashDuration_;
        shakeTime_ = 0.15f;
        shakeAmplitude_ = 5.f;
        combatBadges_.push_back({"-" + num(exchange.damageDealt, 2) + " HP", Gold, 1.35f, 1.35f, -90.f});
    }
    if (exchange.counterattacked && exchange.counterDamage > 0.f) {
        sfx_.play(SfxId::TakeDamage, muted_);
        if (!exchange.critical) {
            flashColor_ = {225, 78, 78};
            flashDuration_ = 0.28f;
            flash_ = flashDuration_;
        }
        shakeTime_ = std::max(shakeTime_, 0.24f);
        shakeAmplitude_ = std::max(shakeAmplitude_, 7.5f);
        combatBadges_.push_back({"COUNTER -" + num(exchange.counterDamage, 2) + " HP", NegativeRed, 1.5f, 1.5f, 120.f});
    } else if (exchange.guarded && !exchange.enemyDefeated) {
        combatBadges_.push_back({"PARRIED", GuardCyan, 1.35f, 1.35f, 120.f});
    }
}
void GameApp::recordPhaseFeedback(Phase previousPhase, bool previousChestUnlocked) {
    const auto nowPhase = floor_.phase();
    if (previousPhase == Phase::Riddle && (nowPhase == Phase::RiddleOutcome || nowPhase == Phase::Defeat)) {
        const bool correct = floor_.message().rfind("Correct.", 0) == 0;
        sfx_.play(correct ? SfxId::RiddleCorrect : SfxId::RiddleWrong, muted_);
        if (correct && floor_.room() == Room::Chest) {
            sfx_.play(SfxId::ChestUnlock, muted_);
        }
    } else if (previousPhase == Phase::Loot && previousChestUnlocked) {
        sfx_.play(SfxId::ChestUnlock, muted_);
    }
}
void GameApp::handle(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) { overlay_ = Overlay::Quit; buttons_.clear(); selectedButton_ = 0; return; }
    if (event.is<sf::Event::FocusLost>()) {
        if (screen_ == Screen::Game && overlay_ == Overlay::None && !timedRiddle()) overlay_ = Overlay::Pause;
        return;
    }
    if (event.is<sf::Event::FocusGained>()) return;
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto pt = pointer(moved->position);
        hoveredStat_.reset();
        hoveredMapNode_.reset();
        if (pt) {
            if (screen_ == Screen::Game && overlay_ == Overlay::None) {
                for (const auto& target : statHoverTargets_) {
                    if (target.bounds.contains(*pt)) { hoveredStat_ = target.stat; break; }
                }
            } else if (overlay_ == Overlay::Map) {
                for (const auto& node : mapNodes_) {
                    if (node.bounds.contains(*pt)) { hoveredMapNode_ = node.index; selectedMapNode_ = node.index; break; }
                }
            }
        }
        return;
    }
    const bool enteringInitials = screen_ == Screen::Results && entryPending_ && overlay_ == Overlay::None;
    const bool enteringCustomSeed = screen_ == Screen::Biomes && !choosingSecondFloor_ && editingSeed_ && overlay_ == Overlay::None;
    if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
        if (enteringInitials && initials_.size() < 3) {
            auto letter = text->unicode;
            if (letter >= 'a' && letter <= 'z') letter -= 'a' - 'A';
            if (letter >= 'A' && letter <= 'Z') { initials_ += static_cast<char>(letter); selectedButton_ = 4; }
        } else if (enteringCustomSeed && seedInput_.size() < 16) {
            const auto ch = text->unicode;
            if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '-' || ch == '_') {
                seedInput_ += static_cast<char>(ch);
                dailyRunSelected_ = false;
            }
        }
        return;
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::F11) { openWindow(!fullscreen_); return; }
        if (enteringInitials) {
            if (key->code == Key::Backspace) { if (!initials_.empty()) initials_.pop_back(); selectedButton_ = 4; return; }
            if (key->code == Key::Escape) { action("skip-score"); return; }
            if (key->code == Key::Enter) {
                if (selectedButton_ < buttons_.size() && buttons_[selectedButton_].enabled && buttons_[selectedButton_].id != "save-score")
                    action(buttons_[selectedButton_].id);
                else action("save-score");
                return;
            }
            if (key->code != Key::Tab && key->code != Key::Up && key->code != Key::Down) return;
        }
        if (enteringCustomSeed) {
            if (key->code == Key::Backspace) { if (!seedInput_.empty()) seedInput_.pop_back(); return; }
            if (key->code == Key::Escape || key->code == Key::Enter) { editingSeed_ = false; return; }
            if (key->code != Key::Tab && key->code != Key::Up && key->code != Key::Down) return;
        }
        if (key->code == Key::V) { action("sound"); return; }
        if (key->code == Key::I && screen_ == Screen::Game && overlay_ == Overlay::None) {
            if (!focusedStat_.has_value()) focusedStat_ = Stat::Strength;
            else if (*focusedStat_ == Stat::Accuracy) focusedStat_.reset();
            else focusedStat_ = static_cast<Stat>(static_cast<int>(*focusedStat_) + 1);
            sfx_.play(SfxId::UiClick, muted_);
            return;
        }
        if (overlay_ == Overlay::Map && (key->code == Key::Left || key->code == Key::Right)) {
            if (key->code == Key::Left) selectedMapNode_ = (selectedMapNode_ + 8) % 9;
            else selectedMapNode_ = (selectedMapNode_ + 1) % 9;
            sfx_.play(SfxId::UiClick, muted_);
            return;
        }
        if (key->code == Key::Escape) {
            if (overlay_ != Overlay::None) {
                overlay_ = Overlay::None; buttons_.clear();
                if (screen_ == Screen::Results && entryPending_) selectedButton_ = 4;
            }
            else if (screen_ == Screen::Intro) enter(Screen::Menu);
            else if (screen_ == Screen::Lore) action("lore-back");
            else if (screen_ == Screen::Leaderboard) action("leaderboard-back");
            else if (screen_ == Screen::Biomes && choosingSecondFloor_) action("descent-back");
            else if (screen_ == Screen::Biomes) action("difficulty");
            else if (screen_ == Screen::Difficulty) action("heroes");
            else if (screen_ == Screen::Game) {
                if (!floor_.inProgress()) action("results");
                else if (!timedRiddle()) { overlay_ = Overlay::Pause; buttons_.clear(); }
            }
            else if (screen_ == Screen::Menu) { overlay_ = Overlay::Quit; buttons_.clear(); }
            else enter(Screen::Menu);
            return;
        }
        if (screen_ == Screen::Intro && overlay_ == Overlay::None) { enter(Screen::Menu); return; }
        if (key->code == Key::M && screen_ == Screen::Game) { action("map"); return; }
        if (key->code == Key::Right || key->code == Key::PageDown) { action("page-next"); return; }
        if (key->code == Key::Left || key->code == Key::PageUp) { action("page-prev"); return; }
        if (key->code == Key::Tab || key->code == Key::Down || key->code == Key::Up) {
            if (!buttons_.empty()) {
                const bool back = key->shift || key->code == Key::Up;
                for (std::size_t i = 0; i < buttons_.size(); ++i) {
                    selectedButton_ = (selectedButton_ + (back ? buttons_.size() - 1 : 1)) % buttons_.size();
                    if (buttons_[selectedButton_].enabled) break;
                }
            }
            return;
        }
        if ((key->code == Key::Enter || key->code == Key::Space) && selectedButton_ < buttons_.size()) {
            const auto button = buttons_[selectedButton_];
            if (button.enabled) action(button.id);
            return;
        }
        int digit = -1;
        if (key->code >= Key::Num1 && key->code <= Key::Num4) digit = static_cast<int>(key->code) - static_cast<int>(Key::Num1);
        if (digit >= 0) {
            const auto id = "choice-" + std::to_string(digit);
            for (const auto& button : buttons_) if (button.id == id && button.enabled) { action(id); break; }
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (click->button != sf::Mouse::Button::Left) return;
        if (screen_ == Screen::Intro && overlay_ == Overlay::None) { enter(Screen::Menu); return; }
        const auto point = pointer(click->position);
        if (!point) return;
        if (overlay_ == Overlay::Map) {
            for (const auto& node : mapNodes_) {
                if (node.bounds.contains(*point)) {
                    selectedMapNode_ = node.index;
                    hoveredMapNode_ = node.index;
                    sfx_.play(SfxId::UiClick, muted_);
                    return;
                }
            }
        }
        if (screen_ == Screen::Game && overlay_ == Overlay::None) {
            for (const auto& target : statHoverTargets_) {
                if (target.bounds.contains(*point)) {
                    if (focusedStat_ == target.stat) focusedStat_.reset();
                    else focusedStat_ = target.stat;
                    sfx_.play(SfxId::UiClick, muted_);
                    return;
                }
            }
        }
        for (const auto& button : buttons_) if (button.enabled && button.bounds.contains(*point)) {
            const auto id = button.id;
            if (editingSeed_ && id != "seed-edit") editingSeed_ = false;
            action(id);
            break;
        }
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (overlay_ == Overlay::None && wheel->wheel == sf::Mouse::Wheel::Vertical)
            action(wheel->delta < 0 ? "page-next" : "page-prev");
    }
}
void GameApp::enter(Screen screen) {
    if (narration_) narration_->stop();
    screen_ = screen;
    overlay_ = Overlay::None;
    editingSeed_ = false;
    page_ = 0;
    readingText_.clear();
    buttons_.clear();
    selectedButton_ = screen == Screen::Difficulty ? static_cast<std::size_t>(selectedDifficulty_) : 0;
    animationTime_ = 0;
    if (screen == Screen::Intro) { playMusic("DD.wav"); playIntroNarration(); }
    if (screen == Screen::Menu) {
        refreshSuspendPreview();
        playMusic("menu.mp3");
    }
    if (screen == Screen::Leaderboard) {
        try { leaderboardTables_ = leaderboards_.load(); leaderboardStatus_.clear(); }
        catch (const std::exception& error) {
            std::cerr << "Leaderboards: " << error.what() << '\n';
            leaderboardTables_ = {};
            leaderboardStatus_ = "Scores could not be loaded.";
        }
        selectedButton_ = static_cast<std::size_t>(leaderboardDifficulty_);
    }
    if (screen == Screen::Results) {
        prepareResults();
        selectedButton_ = hasEndingResult() && entryPending_ ? 4 : static_cast<std::size_t>(leaderboardDifficulty_);
    }
}
void GameApp::prepareResults() {
    if (!hasEndingResult()) {
        entryPending_ = false;
        leaderboardDifficulty_ = floor_.difficulty();
        leaderboardStatus_ = "Fallen descents do not submit a score. Browse the high scores below.";
    } else if (resultsRunSerial_ != floor_.runSerial()) {
        resultsRunSerial_ = floor_.runSerial();
        entryPending_ = true;
        initials_.clear(); leaderboardStatus_.clear(); leaderboardEntryId_.reset();
        leaderboardDifficulty_ = floor_.difficulty();
    }
    try { leaderboardTables_ = leaderboards_.load(); }
    catch (const std::exception& error) {
        std::cerr << "Leaderboards: " << error.what() << '\n';
        leaderboardTables_ = {};
        leaderboardStatus_ = entryPending_ ? "Scores could not be loaded. You can retry saving or skip this entry." : "Scores could not be loaded.";
    }
}
void GameApp::submitScore() {
    if (screen_ != Screen::Results || overlay_ != Overlay::None || !hasEndingResult() || !entryPending_ || !Leaderboards::validInitials(initials_)) return;
    try {
        const auto result = leaderboards_.submit(floor_.difficulty(), initials_, floor_.score(), floor_.player().hero, floor_.ending());
        leaderboardTables_ = result.tables;
        leaderboardEntryId_ = result.id;
        entryPending_ = false;
        leaderboardDifficulty_ = floor_.difficulty();
        leaderboardStatus_ = result.rank ? initials_ + " placed #" + std::to_string(*result.rank) + " on " + DungeonRun::difficultyName(floor_.difficulty()) + "."
            : "This score did not enter the top ten.";
        sfx_.play(SfxId::UiClick, muted_);
    } catch (const std::exception& error) {
        std::cerr << "Leaderboards: " << error.what() << '\n';
        leaderboardStatus_ = "Could not save your score. Try again or skip this entry.";
    }
    buttons_.clear(); selectedButton_ = entryPending_ ? 4 : static_cast<std::size_t>(leaderboardDifficulty_);
}
void GameApp::action(const std::string& id) {
    // Navigation may not replace a live descent, including floor transitions
    // and lore. A new hero can only be chosen before a run or after it ends.
    if (floor_.inProgress() && (id == "menu" || id == "start" || id == "heroes" || id == "restart" || id == "difficulty" || id == "resume-run")) return;
    if (timedRiddle() && (id == "pause" || id == "map" || id == "lore" || id == "suspend-run")) return;
    if (id == "save-score") { submitScore(); return; }
    if (id == "skip-score") {
        if (screen_ == Screen::Results && hasEndingResult() && entryPending_) {
            entryPending_ = false; leaderboardStatus_ = "Score entry skipped.";
            sfx_.play(SfxId::UiClick, muted_);
            buttons_.clear(); selectedButton_ = static_cast<std::size_t>(leaderboardDifficulty_);
        }
        return;
    }
    if (id.rfind("board-", 0) == 0) {
        const bool canBrowseBoards = (screen_ == Screen::Leaderboard) || (screen_ == Screen::Results && !entryPending_);
        if (canBrowseBoards && id.size() == 7 && id[6] >= '0' && id[6] <= '3') {
            leaderboardDifficulty_ = static_cast<Difficulty>(id[6] - '0');
            selectedButton_ = static_cast<std::size_t>(leaderboardDifficulty_);
            sfx_.play(SfxId::UiClick, muted_);
            buttons_.clear();
        }
        return;
    }
    if (screen_ == Screen::Results && entryPending_ && id == "menu") return;
    if (id == "sound") {
        muted_ = !muted_;
        if (music_) music_->setVolume(muted_ ? 0.f : 25.f);
        if (narration_) narration_->setVolume(muted_ ? 0.f : 60.f);
        sfx_.setMuted(muted_);
        if (!muted_) sfx_.play(SfxId::UiClick, muted_);
        return;
    }
    if (id == "page-next") { if (overlay_ == Overlay::None && page_ + 1 < pageCount_) { ++page_; sfx_.play(SfxId::UiClick, muted_); } return; }
    if (id == "page-prev") { if (overlay_ == Overlay::None && page_ > 0) { --page_; sfx_.play(SfxId::UiClick, muted_); } return; }
    if (id == "seed-edit") {
        if (screen_ == Screen::Biomes && !choosingSecondFloor_) {
            editingSeed_ = !editingSeed_;
            if (editingSeed_) dailyRunSelected_ = false;
            sfx_.play(SfxId::UiClick, muted_);
        }
        return;
    }
    if (id == "seed-daily") {
        if (screen_ == Screen::Biomes && !choosingSecondFloor_) {
            dailyRunSelected_ = !dailyRunSelected_;
            editingSeed_ = false;
            sfx_.play(SfxId::UiClick, muted_);
        }
        return;
    }
    if (id == "seed-clear") {
        if (screen_ == Screen::Biomes && !choosingSecondFloor_) {
            seedInput_.clear();
            dailyRunSelected_ = false;
            editingSeed_ = false;
            sfx_.play(SfxId::UiClick, muted_);
        }
        return;
    }
    if (id == "leaderboards") {
        sfx_.play(SfxId::UiClick, muted_);
        leaderboardReturn_ = screen_;
        leaderboardDifficulty_ = selectedDifficulty_;
        enter(Screen::Leaderboard);
        return;
    }
    if (id == "leaderboard-back") {
        sfx_.play(SfxId::UiClick, muted_);
        enter(leaderboardReturn_);
        return;
    }
    if (id == "resume-run") {
        if (!floor_.inProgress()) {
            try {
                if (const auto loaded = suspendSave_.loadAndConsume(floor_)) {
                    playTime_ = static_cast<float>(loaded->playTimeSeconds);
                    seed_ = loaded->seed;
                    activeDailyRun_ = loaded->dailyRun;
                    activeDailyDate_ = loaded->dailyDate;
                    selectedHero_ = floor_.player().hero;
                    selectedDifficulty_ = floor_.difficulty();
                    choosingSecondFloor_ = false;
                    statusBanner_.clear();
                    suspendPreview_.reset();
                    sfx_.play(SfxId::UiClick, muted_);
                    enter(Screen::Game);
                    storyRoom_ = -1;
                    syncStory();
                    playMusic(floor_.musicFile());
                    return;
                }
            } catch (const std::exception& error) {
                std::cerr << "SuspendSave: " << error.what() << '\n';
                statusBanner_ = "Suspended run could not be loaded.";
                refreshSuspendPreview();
            }
        }
        return;
    }
    if (id == "suspend-run") {
        if (screen_ == Screen::Game && floor_.canSuspend() && !timedRiddle()) {
            try {
                suspendSave_.save(floor_, static_cast<double>(playTime_), activeDailyRun_, activeDailyDate_);
                // Reset in-memory run so Main Menu knows no active in-memory descent blocks navigation.
                floor_ = DungeonRun(options_.assets);
                statusBanner_ = "Run suspended and saved.";
                sfx_.play(SfxId::UiClick, muted_);
                enter(Screen::Menu);
                return;
            } catch (const std::exception& error) {
                std::cerr << "SuspendSave: " << error.what() << '\n';
                statusBanner_ = "Could not save suspended run.";
            }
        }
        return;
    }
    if (id == "quit") { sfx_.play(SfxId::UiClick, muted_); overlay_ = Overlay::Quit; }
    else if (id == "quit-yes") {
        if (narration_) narration_->stop();
        if (music_) music_->stop();
        sfx_.stopAll();
        window_.close();
    }
    else if (id == "resume") { sfx_.play(SfxId::UiClick, muted_); overlay_ = Overlay::None; }
    else if (id == "pause") { sfx_.play(SfxId::UiClick, muted_); overlay_ = Overlay::Pause; }
    else if (id == "map") {
        sfx_.play(SfxId::UiClick, muted_);
        overlay_ = overlay_ == Overlay::Map ? Overlay::None : Overlay::Map;
        if (overlay_ == Overlay::Map) {
            selectedMapNode_ = static_cast<std::size_t>(std::clamp(floor_.roomNumber() - 1, 0, 8));
            hoveredMapNode_.reset();
        }
    }
    else if (id == "menu") { sfx_.play(SfxId::UiClick, muted_); enter(Screen::Menu); }
    else if (id == "start") { sfx_.play(SfxId::UiClick, muted_); statusBanner_.clear(); enter(Screen::Introduction); }
    else if (id == "heroes") { sfx_.play(SfxId::UiClick, muted_); choosingSecondFloor_ = false; enter(Screen::Heroes); }
    else if (id == "difficulty") { sfx_.play(SfxId::UiClick, muted_); enter(Screen::Difficulty); }
    else if (id == "descend" && floor_.canDescend()) {
        sfx_.play(SfxId::UiClick, muted_);
        if (floor_.floorNumber() == 1) { choosingSecondFloor_ = true; enter(Screen::Biomes); }
        else if (floor_.descend(Biome::Lava)) {
            enter(Screen::Game);
            storyRoom_ = -1;
            syncStory();
            playMusic(floor_.musicFile());
        }
    }
    else if (id == "descent-back") { sfx_.play(SfxId::UiClick, muted_); choosingSecondFloor_ = false; enter(Screen::Game); }
    else if (id == "lore") {
        sfx_.play(SfxId::UiClick, muted_);
        loreReturn_ = screen_;
        loreHero_ = screen_ == Screen::Menu ? Hero::Warrior : selectedHero_;
        enter(Screen::Lore);
    }
    else if (id == "lore-back") { sfx_.play(SfxId::UiClick, muted_); enter(loreReturn_); }
    else if (id.rfind("lore-", 0) == 0) {
        sfx_.play(SfxId::UiClick, muted_);
        loreHero_ = static_cast<Hero>(std::stoi(id.substr(5)));
        readingText_.clear();
        page_ = 0;
    }
    else if (id == "results" && !floor_.inProgress() && (floor_.phase() == Phase::Ending || floor_.phase() == Phase::Defeat)) {
        sfx_.play(SfxId::UiClick, muted_);
        try { suspendSave_.clear(); } catch (const std::exception&) {}
        refreshSuspendPreview();
        enter(Screen::Results);
    }
    else if (id == "advance") {
        const auto prevPhase = floor_.phase();
        const bool hadChest = floor_.room() == Room::Chest && prevPhase == Phase::Loot;
        floor_.advance();
        recordPhaseFeedback(prevPhase, hadChest);
        if (prevPhase != Phase::Loot) sfx_.play(SfxId::UiClick, muted_);
        page_ = 0;
        readingText_.clear();
        syncStory();
    }
    else if (id == "attack") {
        if (attackDelay_ > 0) return;
        floor_.attack(CombatAction::StandardStrike);
        triggerCombatFeedback(floor_.lastCombatExchange());
        attackDelay_ = .32f;
        page_ = 0;
        readingText_.clear();
        if (!floor_.inProgress()) {
            try { suspendSave_.clear(); } catch (const std::exception&) {}
            refreshSuspendPreview();
        }
    }
    else if (id.rfind("choice-", 0) == 0) {
        const auto choice = static_cast<std::size_t>(std::stoi(id.substr(7)));
        if (screen_ == Screen::Heroes) {
            sfx_.play(SfxId::UiClick, muted_);
            selectedHero_ = static_cast<Hero>(choice);
            choosingSecondFloor_ = false;
            enter(Screen::Difficulty);
        }
        else if (screen_ == Screen::Difficulty && choice < 4) {
            sfx_.play(SfxId::UiClick, muted_);
            selectedDifficulty_ = static_cast<Difficulty>(choice);
            enter(Screen::Biomes);
        }
        else if (screen_ == Screen::Biomes) {
            sfx_.play(SfxId::UiClick, muted_);
            const auto biome = static_cast<Biome>(choice + (choosingSecondFloor_ ? 2 : 0));
            if (choosingSecondFloor_) { if (!floor_.descend(biome)) return; }
            else {
                applyResolvedSeed();
                try { suspendSave_.clear(); } catch (const std::exception&) {}
                suspendPreview_.reset();
                floor_.start(selectedHero_, biome, seed_, selectedDifficulty_);
                playTime_ = 0;
                combatBadges_.clear();
            }
            choosingSecondFloor_ = false;
            enter(Screen::Game);
            storyRoom_ = -1;
            syncStory();
            playMusic(floor_.musicFile());
        }
        else if (screen_ == Screen::Game && overlay_ == Overlay::None) {
            const auto prevPhase = floor_.phase();
            if (prevPhase == Phase::Riddle) {
                floor_.answer(choice);
                recordPhaseFeedback(prevPhase, false);
            } else if (prevPhase == Phase::Combat) {
                if (attackDelay_ > 0 || !floor_.combatChoiceAvailable(choice)) return;
                floor_.choose(choice);
                triggerCombatFeedback(floor_.lastCombatExchange());
                attackDelay_ = .32f;
            } else {
                sfx_.play(SfxId::UiClick, muted_);
                floor_.choose(choice);
            }
            page_ = 0;
            readingText_.clear();
            if (!floor_.inProgress()) {
                try { suspendSave_.clear(); } catch (const std::exception&) {}
                refreshSuspendPreview();
            }
        }
    }
    buttons_.clear(); // No stale hit targets survive transitions or queued clicks.
    selectedButton_ = overlay_ != Overlay::None ? 0 : screen_ == Screen::Lore ? static_cast<std::size_t>(loreHero_) :
                      screen_ == Screen::Difficulty ? static_cast<std::size_t>(selectedDifficulty_) :
                      screen_ == Screen::Leaderboard ? static_cast<std::size_t>(leaderboardDifficulty_) :
                      screen_ == Screen::Results ? (hasEndingResult() && entryPending_ ? 4 : static_cast<std::size_t>(leaderboardDifficulty_)) : 0;
}
void GameApp::syncStory() {
    if (storyRoom_ != floor_.floorNumber() * 10 + floor_.roomNumber()) {
        storyRoom_ = floor_.floorNumber() * 10 + floor_.roomNumber();
        roomStory_ = DungeonRun::readText(options_.assets / floor_.storyFile());
        page_ = 0;
    }
}
void GameApp::update(float dt) {
    if (floor_.inProgress() && std::isfinite(dt) && dt > 0) playTime_ += dt;
    animationTime_ += std::min(dt, .1f);
    attackDelay_ = std::max(0.f, attackDelay_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shakeTime_ = std::max(0.f, shakeTime_ - dt);
    for (auto& badge : combatBadges_) badge.timer = std::max(0.f, badge.timer - dt);
    combatBadges_.erase(std::remove_if(combatBadges_.begin(), combatBadges_.end(),
                                       [](const FloatingBadge& b) { return b.timer <= 0.f; }),
                        combatBadges_.end());
    // Timed questions use wall-clock frame time, including focus loss and quit
    // confirmation. Frame stalls or window recreation must not buy extra time.
    if (screen_ == Screen::Game) {
        const auto prevPhase = floor_.phase();
        floor_.tick(dt);
        if (floor_.phase() == Phase::Riddle) {
            const int secsLeft = static_cast<int>(std::ceil(floor_.timeLeft()));
            if (secsLeft >= 1 && secsLeft <= 5 && secsLeft != lastTimerTickSecond_) {
                lastTimerTickSecond_ = secsLeft;
                sfx_.play(SfxId::TimerTick, muted_);
            }
        } else {
            lastTimerTickSecond_ = -1;
            if (prevPhase == Phase::Riddle && (floor_.phase() == Phase::RiddleOutcome || floor_.phase() == Phase::Defeat)) {
                sfx_.play(SfxId::RiddleWrong, muted_);
            }
        }
    } else {
        lastTimerTickSecond_ = -1;
    }
}
void GameApp::playMusic(const std::string& file) {
    if (!options_.audio || musicFile_ == file) return;
    if (!music_) music_ = std::make_unique<sf::Music>();
    music_->stop();
    musicFile_ = file;
    if (!music_->openFromFile(options_.assets / ("music/" + file))) {
        std::cerr << "Music unavailable: " << file << '\n';
        return;
    }
    music_->setLooping(true);
    music_->setVolume(muted_ ? 0.f : 25.f);
    music_->play();
}
void GameApp::playIntroNarration() {
    if (!options_.audio) return;
    if (!narration_) narration_ = std::make_unique<sf::Music>();
    if (!narration_->openFromFile(options_.assets / "music/intro.wav")) {
        std::cerr << "Intro narration unavailable: intro.wav\n";
        return;
    }
    // Keep the original voice-over louder than the looping intro soundtrack.
    narration_->setLooping(false);
    narration_->setVolume(muted_ ? 0.f : 60.f);
    narration_->play();
}
sf::Texture& GameApp::animationTexture(const std::string& name) {
    if (animationFrameName_ == name && animationFrameTexture_.getSize().x > 0) {
        ++animationTextureHits_;
        return animationFrameTexture_;
    }
    if (!animationFrameTexture_.loadFromFile(options_.assets / name))
        throw std::runtime_error("Missing animation frame: " + name);
    animationFrameTexture_.setSmooth(true);
    animationFrameName_ = name;
    ++animationTextureLoads_;
    return animationFrameTexture_;
}
sf::Texture& GameApp::texture(const std::string& name) {
    if (isAnimationFramePath(name)) return animationTexture(name);
    const auto found = textures_.find(name);
    if (found != textures_.end()) {
        ++persistentTextureHits_;
        // Promote on hit (LRU) so active multi-image screen textures never thrash.
        const auto it = std::find(textureOrder_.begin(), textureOrder_.end(), name);
        if (it != textureOrder_.end()) {
            textureOrder_.erase(it);
            textureOrder_.push_back(name);
        }
        return *found->second;
    }
    auto tex = std::make_unique<sf::Texture>();
    if (!tex->loadFromFile(options_.assets / name)) throw std::runtime_error("Missing image: " + name);
    tex->setSmooth(true);
    while (textures_.size() >= MaxPersistentTextures && !textureOrder_.empty()) {
        textures_.erase(textureOrder_.front());
        textureOrder_.pop_front();
        ++persistentTextureEvictions_;
    }
    textureOrder_.push_back(name);
    ++persistentTextureLoads_;
    auto& result = *tex;
    textures_.emplace(name, std::move(tex));
    return result;
}
void GameApp::image(const std::string& name, sf::FloatRect rect, bool cover, sf::Color tint) {
    auto& tex = texture(name);
    sf::Sprite sprite(tex);
    const auto size = tex.getSize();
    const float scale = cover ? std::max(rect.size.x / size.x, rect.size.y / size.y)
                              : std::min(rect.size.x / size.x, rect.size.y / size.y);
    if (cover) {
        const auto w = std::max(1, static_cast<int>(rect.size.x / scale));
        const auto h = std::max(1, static_cast<int>(rect.size.y / scale));
        sprite.setTextureRect(sf::IntRect({(static_cast<int>(size.x) - w) / 2, (static_cast<int>(size.y) - h) / 2}, {w, h}));
    }
    sprite.setScale({scale, scale});
    const auto bounds = sprite.getGlobalBounds();
    sprite.setPosition({rect.position.x + (rect.size.x - bounds.size.x) / 2,
                        rect.position.y + (rect.size.y - bounds.size.y) / 2});
    sprite.setColor(tint);
    canvas_.draw(sprite);
}
void GameApp::panel(sf::FloatRect rect, sf::Color fill, sf::Color outline) {
    sf::RectangleShape shape(rect.size);
    shape.setPosition(rect.position);
    shape.setFillColor(fill);
    shape.setOutlineColor(outline);
    shape.setOutlineThickness(-1);
    canvas_.draw(shape);
}
void GameApp::label(const std::string& text, sf::Vector2f position, unsigned size, sf::Color color, bool heading) {
    sf::Text rendered(heading ? titleFont_ : bodyFont_, utf8(text), size);
    rendered.setPosition(position);
    rendered.setFillColor(color);
    canvas_.draw(rendered);
}
std::vector<sf::String> GameApp::wrap(const std::string& text, unsigned size, float width) {
    sf::Text measure(bodyFont_, {}, size);
    std::vector<sf::String> lines;
    sf::String line, word;
    const auto flushWord = [&]() {
        if (word.isEmpty()) return;
        measure.setString(line + word);
        if (!line.isEmpty() && measure.getLocalBounds().size.x > width) { lines.push_back(line); line.clear(); }
        for (const auto ch : word) {
            measure.setString(line + sf::String(ch));
            if (!line.isEmpty() && measure.getLocalBounds().size.x > width) { lines.push_back(line); line.clear(); }
            line += ch;
        }
        word.clear();
    };
    for (const auto ch : utf8(text)) {
        if (ch == U' ' || ch == U'\t' || ch == U'\n') {
            flushWord();
            if (ch == U'\n') { lines.push_back(line); line.clear(); }
            else if (!line.isEmpty() && line[line.getSize() - 1] != U' ') line += U' ';
        } else word += ch;
    }
    flushWord();
    if (!line.isEmpty()) lines.push_back(line);
    if (lines.empty()) lines.emplace_back();
    return lines;
}
void GameApp::paragraph(const std::string& text, sf::FloatRect rect, unsigned size) {
    const auto lines = wrap(text, size, rect.size.x);
    float y = rect.position.y;
    for (const auto& line : lines) {
        if (y + size > rect.position.y + rect.size.y) break;
        sf::Text rendered(bodyFont_, line, size);
        rendered.setPosition({rect.position.x, y});
        rendered.setFillColor(Ink);
        canvas_.draw(rendered);
        y += size * 1.4f;
    }
}
void GameApp::reading(const std::string& text, sf::FloatRect rect) {
    if (readingText_ != text) { readingText_ = text; readingLines_ = wrap(text, 20, rect.size.x); page_ = 0; }
    const int perPage = std::max(1, static_cast<int>((rect.size.y - 56) / 29));
    pageCount_ = std::max(1, (static_cast<int>(readingLines_.size()) + perPage - 1) / perPage);
    page_ = std::clamp(page_, 0, pageCount_ - 1);
    for (int i = 0; i < perPage; ++i) {
        const auto index = static_cast<std::size_t>(page_ * perPage + i);
        if (index >= readingLines_.size()) break;
        sf::Text line(bodyFont_, readingLines_[index], 20);
        line.setPosition({rect.position.x, rect.position.y + i * 29.f});
        line.setFillColor(Ink);
        canvas_.draw(line);
    }
    if (pageCount_ > 1) {
        const float y = rect.position.y + rect.size.y - 43;
        button("page-prev", "<", {{rect.position.x, y}, {48, 38}}, page_ > 0);
        label(std::to_string(page_ + 1) + " / " + std::to_string(pageCount_), {rect.position.x + 69, y + 6}, 17, Muted);
        button("page-next", ">", {{rect.position.x + rect.size.x - 48, y}, {48, 38}}, page_ + 1 < pageCount_);
    }
}
void GameApp::button(const std::string& id, const std::string& text, sf::FloatRect rect, bool enabled, std::optional<bool> activeTab) {
    const auto mouse = pointer(sf::Mouse::getPosition(window_));
    const bool hover = enabled && mouse && rect.contains(*mouse);
    const bool selected = buttons_.size() == selectedButton_;
    const bool highlighted = activeTab.value_or(hover || selected);
    panel(rect, activeTab.value_or(false) ? sf::Color(53, 45, 32) : hover ? sf::Color(43, 55, 62) : Surface,
          enabled && highlighted ? Gold : Border);
    // Tab selection follows the displayed lore. Keyboard focus is a separate,
    // neutral underline, so focusing another name never marks its lore active.
    if (activeTab.has_value() && selected)
        panel({{rect.position.x + 15, rect.position.y + rect.size.y - 5}, {rect.size.x - 30, 2}}, Ink, sf::Color::Transparent);
    const auto lines = wrap(text, 18, rect.size.x - 30);
    const float total = std::min(2.f, static_cast<float>(lines.size())) * 23;
    for (std::size_t i = 0; i < lines.size() && i < 2; ++i) {
        sf::Text rendered(bodyFont_, lines[i], 18);
        rendered.setFillColor(enabled ? Ink : Muted);
        rendered.setPosition({rect.position.x + 15, rect.position.y + (rect.size.y - total) / 2 + i * 23.f - 2});
        canvas_.draw(rendered);
    }
    buttons_.push_back({rect, id, enabled});
}
void GameApp::bar(float ratio, sf::FloatRect rect, sf::Color color) {
    panel(rect, {32, 42, 53}, sf::Color::Transparent);
    rect.size.x *= std::clamp(ratio, 0.f, 1.f);
    panel(rect, color, sf::Color::Transparent);
}
void GameApp::drawMenu() {
    const int frame = static_cast<int>(animationTime_ * 12) % 91 + 1;
    image("images/startscreen/startscreen" + std::to_string(frame) + ".jpg", {{0, 0}, {Width, Height}}, true, {145, 155, 165});
    panel({{0, 0}, {Width, Height}}, {4, 9, 15, 130}, sf::Color::Transparent);
    label("DUNGEON", {78, 192}, 88, Ink, true);
    label("DESCENT", {78, 289}, 88, Gold, true);
    paragraph("Every choice leaves a mark.\nChoose your hero, challenge the riddle keeper\nand uncover the truth beneath Aethoria.", {{85, 422}, {680, 130}}, 23);
    if (!statusBanner_.empty()) {
        label(statusBanner_, {85, 550}, 18, Gold);
    }
    if (suspendPreview_.has_value()) {
        std::string resumeLabel = "Resume suspended run (Seed #" + std::to_string(suspendPreview_->seed);
        if (suspendPreview_->dailyRun && !suspendPreview_->dailyDate.empty()) resumeLabel += " / Daily " + suspendPreview_->dailyDate;
        resumeLabel += ")";
        button("resume-run", resumeLabel, {{85, 584}, {459, 54}});
    }
    button("start", "Begin descent", {{85, 650}, {310, 62}});
    button("leaderboards", "High scores", {{414, 650}, {210, 62}});
    button("lore", "Lore of the heroes", {{85, 724}, {310, 56}});
    button("quit", "Quit", {{414, 724}, {210, 56}});
    button("sound", muted_ ? "Sound off [V]" : "Sound on [V]", {{1180, 34}, {220, 45}});
}
void GameApp::drawHeroes() {
    label("CHOOSE YOUR WEAPON", {36, 28}, 40, Gold, true);
    label("Four paths into the dark. Compare all six starting stats and class passives.", {38, 86}, 21, Muted);
    const std::array<std::string, 4> weapons = {"Sword", "Longbow", "Wand", "Throwing knife"};
    const std::array<std::string, 6> shortStats = {"STR", "WIS", "OBS", "AGI", "HP", "ACC"};
    const std::array<float, 6> statMax = {10.f, 10.f, 10.f, 10.f, 45.f, 10.f};
    const std::array<sf::Color, 6> statColors = {
        sf::Color{228, 128, 98},  // STR
        sf::Color{146, 178, 238}, // WIS
        sf::Color{191, 166, 226}, // OBS
        sf::Color{119, 198, 147}, // AGI
        sf::Color{223, 187, 119}, // HP
        sf::Color{123, 198, 224}  // ACC
    };
    for (int i = 0; i < 4; ++i) {
        const auto hero = static_cast<Hero>(i);
        const float x = 36 + i * 348.f;
        panel({{x, 140}, {324, 652}}, Surface, Border);
        image(DungeonRun::portrait(hero), {{x + 10, 150}, {304, 236}}, true);
        label(DungeonRun::heroName(hero), {x + 18, 396}, 29, Gold, true);
        label(weapons[i], {x + 198, 404}, 18, Muted);
        paragraph(DungeonRun::ability(hero), {{x + 18, 434}, {288, 72}}, 16);
        const auto p = DungeonRun::makePlayer(hero);
        // Feature #3: 6-stat compact breakdown grid with visual bars.
        for (int s = 0; s < 6; ++s) {
            const float sy = 514.f + s * 32.f;
            label(shortStats[s], {x + 18, sy}, 15, Muted);
            bar(p.stats[s] / statMax[s], {{x + 68, sy + 5}, {178, 11}}, statColors[s]);
            label(num(p.stats[s]), {x + 258, sy - 1}, 16, Ink);
        }
        button("choice-" + std::to_string(i), std::to_string(i + 1) + "  Choose " + DungeonRun::heroName(hero), {{x + 16, 722}, {292, 54}});
    }
    button("menu", "Back to menu", {{36, 813}, {210, 48}});
}
void GameApp::drawBiomes() {
    const int targetFloor = choosingSecondFloor_ ? 2 : 1;
    label(choosingSecondFloor_ ? "DEEPER INTO THE DUNGEON" : "TWO DOORS. ONE DESCENT.", {38, 26}, 40, Gold, true);
    const std::string subtitle = choosingSecondFloor_
        ? "The stairway leads deeper. Choose your next path."
        : DungeonRun::heroName(selectedHero_) + " / " + DungeonRun::difficultyName(selectedDifficulty_) + ". Which door will you open?";
    label(subtitle, {40, 82}, 21, Muted);
    if (!choosingSecondFloor_) {
        // Feature #4: Custom Seed & Daily Run selector bar on first-floor biome selection.
        panel({{40, 114}, {1360, 52}}, {14, 22, 31}, editingSeed_ ? Gold : Border);
        std::string seedDisplay = dailyRunSelected_
            ? ("DAILY RUN (" + DungeonRun::dailyRunDateString() + " -> #" + std::to_string(DungeonRun::dailyRunSeedForToday()) + ")")
            : (!seedInput_.empty() ? ("CUSTOM SEED: " + seedInput_ + " (#" + std::to_string(DungeonRun::parseSeed(seedInput_)) + ")")
                                   : "RANDOM SEED (Auto)");
        if (editingSeed_) seedDisplay = "TYPE SEED: " + seedInput_ + "_  (0-9 / A-Z, Enter when done)";
        label("RUN SEED:", {58, 128}, 16, Gold);
        label(seedDisplay, {162, 127}, 18, Ink);
        button("seed-edit", editingSeed_ ? "Done seed" : "Custom seed", {{815, 120}, {175, 40}}, true, editingSeed_);
        button("seed-daily", "Daily run", {{1004, 120}, {175, 40}}, true, dailyRunSelected_);
        button("seed-clear", "Random seed", {{1193, 120}, {188, 40}}, !seedInput_.empty() || dailyRunSelected_ || editingSeed_);
    }
    const float cardY = choosingSecondFloor_ ? 150.f : 180.f;
    const float cardH = choosingSecondFloor_ ? 625.f : 605.f;
    const float imgH = choosingSecondFloor_ ? 390.f : 368.f;
    const std::array<std::string, 4> descriptions = {
        "Frozen chambers, a captive warrior and the Yeti guarding the stairway.",
        "Forgotten halls, living roots and the Jungle Warden waiting below.",
        "Sandstone crypts, an injured butler and Ahmose the Pharaoh.",
        "Haunted chambers, a hidden vision and Count Dracula."};
    for (int i = 0; i < 2; ++i) {
        const float x = 40 + i * 700.f;
        const auto biome = static_cast<Biome>((targetFloor - 1) * 2 + i);
        const auto index = static_cast<std::size_t>(biome);
        panel({{x, cardY}, {660, cardH}}, Surface, Border);
        image(DungeonRun::bossImage(biome), {{x + 1, cardY + 1}, {658, imgH}}, true, {65, 75, 85});
        image(DungeonRun::bossImage(biome), {{x + 1, cardY + 1}, {658, imgH}});
        label(DungeonRun::biomeName(biome), {x + 24, cardY + imgH + 16}, 29, biomeAccent(biome), true);
        paragraph(descriptions[index], {{x + 24, cardY + imgH + 62}, {610, 62}}, 20);
        button("choice-" + std::to_string(i), std::to_string(i + 1) + "  Enter the " + DungeonRun::biomeKey(biome) + " door",
               {{x + 24, cardY + cardH - 76}, {612, 56}});
    }
    button(choosingSecondFloor_ ? "descent-back" : "difficulty", choosingSecondFloor_ ? "Back to the stairway" : "Change difficulty", {{40, 812}, {290, 48}});
}
void GameApp::drawDifficulties() {
    label("CHOOSE YOUR CHALLENGE", {36, 28}, 40, Gold, true);
    label("Choose the strength of your enemies. Your choice lasts for the whole descent.", {38, 86}, 21, Muted);
    const std::array<Biome, 4> portraits = {Biome::Ice, Biome::Desert, Biome::Ghost, Biome::Lava};
    const std::array<std::string, 4> descriptions = {
        "Gentler foes. Room to learn, recover and explore.",
        "Stronger foes and longer battles. Prepare well and keep your health up.",
        "Relentless enemies. Strong growth and careful choices are essential.",
        "An extreme challenge. Even a well-prepared hero may not survive."};
    for (int i = 0; i < 4; ++i) {
        const float x = 36 + i * 348.f;
        const auto difficulty = static_cast<Difficulty>(i);
        panel({{x, 145}, {324, 638}}, Surface, Border);
        image(DungeonRun::bossImage(portraits[i]), {{x + 10, 155}, {304, 275}}, true);
        label(DungeonRun::difficultyName(difficulty), {x + 20, 453}, 30, biomeAccent(portraits[i]), true);
        paragraph(descriptions[i], {{x + 20, 514}, {284, 148}}, 21);
        button("choice-" + std::to_string(i), std::to_string(i + 1) + "  Choose " + DungeonRun::difficultyName(difficulty), {{x + 16, 700}, {292, 64}});
    }
    button("heroes", "Change hero", {{36, 813}, {230, 48}});
    label("Difficulty changes enemy health and damage. Riddles, hero stats and rewards stay the same.", {292, 827}, 17, Muted);
}
void GameApp::drawGame() {
    if (floor_.phase() == Phase::Ending) { drawEnding(); return; }
    statHoverTargets_.clear();
    const auto& p = floor_.player();
    const auto phase = floor_.phase();
    const auto accent = biomeAccent(floor_.biome());
    const float shakeX = shakeTime_ > 0 ? std::sin(animationTime_ * 95.f) * shakeAmplitude_ * (shakeTime_ / 0.28f) : 0.f;
    const float shakeY = shakeTime_ > 0 ? std::cos(animationTime_ * 110.f) * shakeAmplitude_ * 0.6f * (shakeTime_ / 0.28f) : 0.f;

    label("DUNGEON DESCENT", {28, 19}, 31, Gold, true);
    label("FLOOR " + std::to_string(floor_.floorNumber()) + "  /  " + DungeonRun::biomeName(floor_.biome()) + "  /  " + DungeonRun::difficultyName(floor_.difficulty()), {30, 68}, 18, accent);
    for (int i = 0; i < 9; ++i) {
        const float x = 527 + i * 36.f;
        panel({{x, 65}, {27, 27}}, i + 1 < floor_.roomNumber() ? sf::Color(64, 101, 99) : Surface,
              i + 1 == floor_.roomNumber() ? Gold : Border);
        label(std::to_string(i + 1), {x + 8, 68}, 15, i + 1 == floor_.roomNumber() ? Gold : Muted);
    }
    panel({{28 + shakeX, 126 + shakeY}, {864, 428}}, Surface, Border);
    image(floor_.image(), {{29 + shakeX, 127 + shakeY}, {862, 426}}, true, {55, 65, 75});
    image(floor_.image(), {{29 + shakeX, 127 + shakeY}, {862, 426}});
    panel({{29 + shakeX, 487 + shakeY}, {862, 66}}, {9, 16, 24, 225}, sf::Color::Transparent);
    label("ROOM " + std::to_string(floor_.roomNumber()) + " / 9", {48 + shakeX, 500 + shakeY}, 14, accent);
    label(floor_.title(), {222 + shakeX, 497 + shakeY}, 28, Ink, true);
    if (flash_ > 0) {
        const float alphaRatio = std::clamp(flash_ / std::max(0.05f, flashDuration_), 0.f, 1.f);
        sf::Color overlayColor = flashColor_;
        overlayColor.a = static_cast<std::uint8_t>(alphaRatio * 140.f);
        panel({{29 + shakeX, 127 + shakeY}, {862, 360}}, overlayColor, sf::Color::Transparent);
    }
    // Feature #10: Floating damage / crit / guard / counterattack badges over the scene.
    for (std::size_t bi = 0; bi < combatBadges_.size(); ++bi) {
        const auto& badge = combatBadges_[bi];
        const float progress = 1.f - (badge.timer / std::max(0.1f, badge.duration));
        const float bx = 430.f + badge.xOffset;
        const float by = 290.f - progress * 55.f + static_cast<float>(bi) * 34.f;
        panel({{bx - 12, by - 4}, {225, 34}}, {10, 16, 24, 225}, badge.color);
        label(badge.text, {bx, by}, 19, badge.color, false);
    }

    // Hero & stats panel (bottom-left) with Feature #2 stat hover/focus targets and Feature #10 green/red deltas.
    const bool showStatDeltas = (phase == Phase::Outcome || phase == Phase::RiddleOutcome || phase == Phase::Loot);
    const auto& deltas = floor_.lastStatDeltas();
    const int repDelta = floor_.lastReputationDelta();
    panel({{28, 574}, {280, 284}}, Surface, Border);
    image(DungeonRun::portrait(p.hero), {{43, 590}, {73, 84}}, true);
    label(DungeonRun::heroName(p.hero), {130, 586}, 23, Gold, true);

    const sf::FloatRect hpBounds{{126, 618}, {172, 26}};
    statHoverTargets_.push_back({hpBounds, Stat::Health});
    const float hpDelta = showStatDeltas ? deltas[Stat::Health] : 0.f;
    const sf::Color hpColor = std::abs(hpDelta) > 0.005f ? (hpDelta > 0 ? PositiveGreen : NegativeRed) : Ink;
    std::string hpText = "HP " + num(p.stats[Health]);
    if (std::abs(hpDelta) > 0.005f) hpText += " (" + signedDelta(hpDelta) + ")";
    label(hpText, {130, 620}, 19, hpColor);

    std::string repText = "Rep " + std::to_string(p.reputation) + "/9";
    if (showStatDeltas && repDelta != 0) repText += " (" + std::string(repDelta > 0 ? "+" : "") + std::to_string(repDelta) + ")";
    label(repText, {130, 648}, 16, (showStatDeltas && repDelta != 0) ? (repDelta > 0 ? PositiveGreen : NegativeRed) : Muted);

    const auto activeTooltipStat = hoveredStat_.has_value() ? hoveredStat_ : focusedStat_;
    for (int s = 0; s < 6; ++s) {
        if (s == Health) continue;
        const int row = s < Health ? s : 4;
        const float rowY = 684.f + row * 26.f;
        const auto st = static_cast<Stat>(s);
        const sf::FloatRect rowBounds{{38, rowY - 2}, {260, 24}};
        statHoverTargets_.push_back({rowBounds, st});
        const bool statHighlighted = activeTooltipStat == st;
        if (statHighlighted) {
            panel(rowBounds, {35, 48, 60}, Gold);
        }
        const float d = showStatDeltas ? deltas[s] : 0.f;
        const bool hasDelta = std::abs(d) > 0.005f;
        const sf::Color valColor = hasDelta ? (d > 0 ? PositiveGreen : NegativeRed) : (statHighlighted ? Gold : Ink);
        label(DungeonRun::statName(s), {46, rowY}, 16, statHighlighted ? Gold : Muted);
        std::string valStr = num(p.stats[s]);
        if (hasDelta) valStr += " (" + signedDelta(d) + ")";
        label(valStr, {hasDelta ? 174.f : 235.f, rowY}, 16, valColor);
    }

    // Center action / combat panel.
    panel({{328, 574}, {564, 284}}, Surface, Border);
    label(phase == Phase::Combat ? "TACTICAL COMBAT" : phase == Phase::Riddle ? "THE RIDDLE KEEPER" : "YOUR CHOICE", {350, 588}, 16, accent);
    if (phase == Phase::Room) {
        const auto choices = floor_.choices();
        const bool isBattlePreview = (floor_.room() == Room::Battle || floor_.room() == Room::Boss);
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const bool vertical = choices.size() == 3;
            const float x = vertical ? 350.f : 348 + (i % 2) * 270.f;
            const float y = vertical ? 622 + i * 69.f : (isBattlePreview ? 618.f : 632.f) + (i / 2) * 87.f;
            const float h = vertical ? 58.f : (isBattlePreview ? 62.f : 73.f);
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + choices[i], {{x, y}, {vertical ? 520.f : 254.f, h}});
        }
        // Feature #2: Pre-battle combat math summary alongside enemy stats.
        if (isBattlePreview) {
            const auto summary = floor_.combatSummary();
            panel({{348, 694}, {524, 152}}, {13, 20, 29}, Border);
            label("COMBAT FORECAST", {364, 704}, 15, Gold);
            label("Enemy: " + floor_.enemyName() + "  |  HP " + num(summary.enemyMaxHealth) +
                  "  |  Counter " + num(summary.enemyCounterDamage, 2) + " (Guard " + num(summary.guardCounterDamage, 2) + ")",
                  {364, 728}, 16, Ink);
            label("Your Strike: " + num(summary.baseDamage, 2) + " dmg  |  Crit: " + num(summary.critDamage, 2) +
                  " (" + num(summary.critChance * 100.f, 0) + "% chance)  |  Expected: " + num(summary.expectedDamage, 2),
                  {364, 754}, 16, PositiveGreen);
            label("Heavy (2 strikes): " + num(summary.heavyDamage, 2) + " dmg  |  Guard: " + num(summary.guardDamage, 2) +
                  " dmg  |  Strikes: " + std::to_string(summary.minStrikes) + "-" + std::to_string(summary.maxStrikes),
                  {364, 780}, 16, Muted);
            label("Class Ability: " + DungeonRun::combatAbilityName(p.hero) + " (" + num(summary.abilityDamage, 2) + " base dmg)",
                  {364, 806}, 15, accent);
        }
    } else if (phase == Phase::Riddle) {
        unsigned row = 0;
        for (std::size_t i = 0; i < 3; ++i) {
            if (!floor_.answerAvailable(i)) continue;
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + floor_.riddle().answers[i], {{350, 626 + row++ * 69.f}, {520, 58}});
        }
    } else if (phase == Phase::Combat) {
        // Feature #8: Active tactical choices in combat (4 tactical options + compat).
        label("Strikes left: " + std::to_string(floor_.strikesLeft()), {350, 614}, 21, Ink);
        label(floor_.enemyName() + "   " + num(floor_.enemyHealth(), 2) + " / " + num(floor_.enemyMaxHealth(), 2) +
              " HP   (Counter " + num(floor_.enemyDamage(), 2) + ")", {520, 617}, 16, Muted);
        bar(floor_.enemyHealth() / floor_.enemyMaxHealth(), {{350, 646}, {520, 9}}, accent);
        const auto combatChoices = floor_.choices();
        for (std::size_t i = 0; i < combatChoices.size() && i < 4; ++i) {
            const float x = 348.f + (i % 2) * 266.f;
            const float y = 668.f + (i / 2) * 70.f;
            const bool avail = attackDelay_ <= 0 && floor_.combatChoiceAvailable(i);
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + combatChoices[i], {{x, y}, {254, 60}}, avail);
        }
        button("attack", "Quick Standard Strike [Enter / Space]", {{348, 812}, {520, 34}},
               attackDelay_ <= 0 && floor_.combatChoiceAvailable(0));
    } else if (phase == Phase::Complete || phase == Phase::Defeat) {
        label(phase == Phase::Complete ? "FLOOR CLEARED" : "GAME OVER", {350, 638}, 26, Gold, true);
        if (phase == Phase::Defeat) {
            button("results", "View results", {{350, 736}, {520, 66}});
        } else if (floor_.canDescend()) button("descend", floor_.floorNumber() == 1 ? "Descend the stairway" : "Enter Malvel's domain", {{350, 736}, {520, 66}});
    } else {
        const std::string next = phase == Phase::Loot ? "Claim chest treasure" :
            phase == Phase::Outcome ? "Face the riddle keeper" : "Continue to the next room";
        paragraph(phase == Phase::Loot ? "The seal has broken. Your reward can be claimed once." : "Read the outcome, then continue your descent.", {{350, 637}, {520, 75}}, 21);
        button("advance", next, {{350, 748}, {520, 76}});
    }
    button("lore", "Hero lore", {{44, 824}, {246, 24}}, !timedRiddle());

    // Feature #2: Render stat tooltip popup when a stat is hovered or focused via [I].
    if (activeTooltipStat.has_value()) {
        panel({{28, 492}, {560, 74}}, {10, 17, 26, 245}, Gold);
        label("STAT TOOLTIP: " + DungeonRun::statName(static_cast<std::size_t>(*activeTooltipStat)), {44, 500}, 15, Gold);
        paragraph(DungeonRun::statTooltip(*activeTooltipStat), {{44, 522}, {530, 40}}, 15);
    }

    panel({{916, 126}, {496, 732}}, Surface, Border);
    std::string heading = "THE CHAMBER", text = roomStory_;
    if (phase == Phase::Riddle) {
        heading = "ANSWER BEFORE TIME RUNS OUT";
        text = floor_.riddle().question;
        const int secsLeft = static_cast<int>(std::ceil(floor_.timeLeft()));
        label(std::to_string(secsLeft) + " seconds", {938, 201}, 24, secsLeft <= 5 ? NegativeRed : Gold);
        bar(floor_.timeLeft() / floor_.timeLimit(), {{938, 245}, {452, 8}}, secsLeft <= 5 ? NegativeRed : accent);
        if (floor_.hasObservationHint()) label("Observation rules out one false answer.", {938, 270}, 18, accent);
    } else if (phase != Phase::Room) { heading = "THE OUTCOME"; text = floor_.message(); }
    if (phase == Phase::Complete) {
        heading = "THE STAIRWAY IS OPEN";
        text = floor_.message() + (floor_.floorNumber() == 1
            ? "\n\nThe guardian falls. Two doors wait below."
            : "\n\nThe path to Malvel's domain lies open. The final confrontation awaits.");
    }
    label(heading, {938, 147}, 16, Gold);
    const bool riddle = phase == Phase::Riddle;
    const float storyY = riddle ? (floor_.hasObservationHint() ? 315.f : 285.f) : 196.f;
    reading(text, {{938, storyY}, {452, 836.f - storyY}});
    button("map", "Map [M]", {{942, 28}, {133, 52}}, !timedRiddle());
    button("pause", "Pause [Esc]", {{1090, 28}, {158, 52}}, !timedRiddle() && floor_.inProgress());
    button("sound", muted_ ? "Sound off [V]" : "Sound on [V]", {{1263, 28}, {149, 52}});
    label(timedRiddle()
        ? "Timed riddle: the countdown continues when you leave the window. Number keys to answer. V sound. F11 fullscreen."
        : "1-4 choices/tactics   [I] or hover stats for tooltips   [M] map   [Esc] pause/save & quit   F11 fullscreen", {30, 876}, 15, Muted);
}
void GameApp::drawLore() {
    label("LORE OF THE HEROES", {40, 30}, 40, Gold, true);
    for (int i = 0; i < 4; ++i) button("lore-" + std::to_string(i), DungeonRun::heroName(static_cast<Hero>(i)), {{40 + i * 206.f, 102}, {188, 46}}, true, static_cast<Hero>(i) == loreHero_);
    image(DungeonRun::portrait(loreHero_), {{40, 185}, {450, 545}});
    panel({{530, 185}, {870, 610}}, Surface, Border);
    reading(DungeonRun::readText(options_.assets / ("textfiles/charIntro/" + DungeonRun::heroName(loreHero_) + "Intro.txt")), {{558, 210}, {814, 559}});
    button("lore-back", "Back", {{40, 816}, {210, 48}});
}
void GameApp::drawEnding() {
    const auto ending = floor_.ending();
    const std::string kind = ending == Ending::Perfect ? "PERFECT ENDING" :
        ending == Ending::True ? "TRUE ENDING" : ending == Ending::Good ? "GOOD ENDING" : "BAD ENDING";
    label(kind, {38, 28}, 18, Gold);
    label(floor_.endingName(), {36, 64}, 43, Ink, true);
    panel({{36, 150}, {740, 648}}, Surface, Border);
    image(floor_.image(), {{37, 151}, {738, 646}}, true, {55, 65, 75});
    image(floor_.image(), {{37, 151}, {738, 646}});
    panel({{800, 150}, {604, 648}}, Surface, Border);
    reading(floor_.message(), {{826, 176}, {552, 596}});
    button("results", "View your journey", {{36, 824}, {350, 52}});
    button("sound", muted_ ? "Sound off [V]" : "Sound on [V]", {{1180, 28}, {224, 48}});
}
void GameApp::drawLeaderboard(bool readOnlyBrowse, bool drawBackground) {
    if (drawBackground) {
        image("images/startscreen.jpg", {{0, 0}, {Width, Height}}, true, {65, 75, 88});
        panel({{0, 0}, {Width, Height}}, {6, 11, 18, 170}, sf::Color::Transparent);
    }
    const float panelX = drawBackground ? 262.f : 488.f;
    panel({{panelX, 70}, {916, 760}}, {10, 17, 25, 245}, Border);
    label("HIGH SCORES", {panelX + 32, 100}, 36, Gold, true);
    for (int mode = 0; mode < 4; ++mode)
        button("board-" + std::to_string(mode), DungeonRun::difficultyName(static_cast<Difficulty>(mode)),
               {{panelX + 32 + mode * 214.f, 158}, {198, 42}}, readOnlyBrowse || !entryPending_,
               static_cast<Difficulty>(mode) == leaderboardDifficulty_);
    if (!readOnlyBrowse && entryPending_) {
        label("ENTER THREE LETTERS", {panelX + 32, 212}, 16, Gold);
        for (std::size_t i = 0; i < 3; ++i) {
            const float x = panelX + 32 + static_cast<float>(i) * 82;
            panel({{x, 238}, {62, 54}}, Surface, i == initials_.size() ? Gold : Border);
            label(i < initials_.size() ? std::string(1, initials_[i]) : "_", {x + 18, 244}, 32, Ink, true);
        }
        button("save-score", "Save score [Enter]", {{panelX + 318, 238}, {250, 54}}, Leaderboards::validInitials(initials_));
        button("skip-score", "Skip entry [Esc]", {{panelX + 592, 238}, {288, 54}});
        if (leaderboardStatus_.empty()) label("A-Z only. Backspace to edit.", {panelX + 32, 304}, 16, Muted);
        else paragraph(leaderboardStatus_, {{panelX + 32, 298}, {848, 40}}, 17);
    } else {
        label(drawBackground ? "LOCAL DIFFICULTY LEADERBOARDS" : "THIS DESCENT", {panelX + 32, 218}, 16, Gold);
        const std::string statusText = leaderboardStatus_.empty()
            ? ("Viewing top ten descents for " + DungeonRun::difficultyName(leaderboardDifficulty_) + ".")
            : leaderboardStatus_;
        paragraph(statusText, {{panelX + 32, 246}, {848, 70}}, 22);
    }
    label("RANK", {panelX + 36, 340}, 16, Muted);
    label("NAME", {panelX + 132, 340}, 16, Muted);
    label("SCORE", {panelX + 306, 340}, 16, Muted);
    label("HERO", {panelX + 478, 340}, 16, Muted);
    label("ENDING", {panelX + 712, 340}, 16, Muted);
    const auto& entries = leaderboardTables_[static_cast<std::size_t>(leaderboardDifficulty_)];
    for (std::size_t i = 0; i < Leaderboards::Limit; ++i) {
        const float y = 371 + static_cast<float>(i) * 38;
        const bool filled = i < entries.size();
        const bool own = !drawBackground && filled && leaderboardEntryId_ == entries[i].id && leaderboardDifficulty_ == floor_.difficulty();
        panel({{panelX + 28, y - 2}, {856, 36}}, own ? sf::Color(61, 54, 37) : Surface, own ? Gold : sf::Color::Transparent);
        label(std::to_string(i + 1), {panelX + 46, y}, 22, own ? Gold : Muted);
        label(filled ? entries[i].initials : "---", {panelX + 132, y}, 22, own ? Gold : Ink, true);
        label(filled ? std::to_string(entries[i].score) : "--", {panelX + 306, y}, 22, own ? Gold : Ink);
        if (filled) {
            label(DungeonRun::heroName(entries[i].hero), {panelX + 478, y + 2}, 19, Ink);
            const auto ending = entries[i].ending;
            label(ending == Ending::Perfect ? "PERFECT" : ending == Ending::True ? "TRUE" : ending == Ending::Good ? "GOOD" : "BAD",
                  {panelX + 712, y + 2}, 18, ending == Ending::Perfect ? Gold : Muted);
        }
    }
    if (drawBackground) {
        button("leaderboard-back", "Back to menu [Esc]", {{panelX + 32, 766}, {260, 46}});
        label("Top 10 per difficulty. Equal scores keep the earlier entry.", {panelX + 318, 777}, 17, Muted);
    } else {
        label("Top 10 per difficulty. Equal scores keep the earlier entry.", {panelX + 32, 782}, 17, Muted);
    }
}
void GameApp::drawResults() {
    const auto& p = floor_.player();
    image(floor_.image(), {{0, 0}, {Width, Height}}, true, {70, 80, 90});
    const int seconds = static_cast<int>(playTime_);
    std::ostringstream duration;
    duration << seconds / 3600 << ':' << std::setfill('0') << std::setw(2) << seconds / 60 % 60 << ':' << std::setw(2) << seconds % 60;
    std::string seedText = "#" + std::to_string(floor_.seed());
    if (activeDailyRun_ && !activeDailyDate_.empty()) seedText += " (" + activeDailyDate_ + ")";
    // Feature #4: Include Seed in Journey Results alongside Feature #1 read-only leaderboard on Defeat.
    const std::array<std::pair<std::string, std::string>, 8> results = {{
        {"Score", std::to_string(floor_.score())},
        {"Run seed", seedText},
        {"Floor reached", std::to_string(floor_.floorNumber()) + " / 3"},
        {"Riddles solved", std::to_string(p.riddlesCorrect) + " / " + std::to_string(p.riddlesAnswered)},
        {"Battles won", std::to_string(p.battlesWon) + " / " + std::to_string(p.battlesFought)},
        {"Reputation", std::to_string(p.reputation) + " / 9"},
        {"Health remaining", num(p.stats[Health])},
        {"Playtime", duration.str()}}};
    drawLeaderboard(!hasEndingResult(), false);
    panel({{36, 70}, {424, 760}}, {10, 17, 25, 245}, Border);
    label("YOUR JOURNEY", {64, 108}, 32, Gold, true);
    paragraph(floor_.endingName(), {{64, 166}, {368, 56}}, 25);
    label(DungeonRun::heroName(p.hero) + " / " + DungeonRun::difficultyName(floor_.difficulty()), {64, 232}, 20, Muted);
    for (std::size_t i = 0; i < results.size(); ++i) {
        const float y = 282 + static_cast<float>(i) * 54;
        label(results[i].first, {64, y}, 19, Muted);
        label(results[i].second, {260, y}, 20, i == 0 ? Gold : Ink);
    }
    button("menu", "Main menu", {{64, 742}, {172, 52}}, !hasEndingResult() || !entryPending_);
    button("quit", "Quit game", {{252, 742}, {180, 52}});
}
void GameApp::drawOverlay() {
    panel({{0, 0}, {Width, Height}}, {0, 0, 0, 205}, sf::Color::Transparent);
    buttons_.clear();
    mapNodes_.clear();
    if (overlay_ == Overlay::Map) {
        panel({{210, 36}, {1020, 828}}, Background, Border);
        label("FLOOR " + std::to_string(floor_.floorNumber()) + " / INTERACTIVE ROUTE MAP", {248, 58}, 31, Gold, true);
        image(floor_.mapImage(), {{248, 114}, {944, 412}});
        panel({{248, 114}, {944, 412}}, {8, 14, 22, 115}, Border);

        // Feature #11: Interactive visual node markers and connecting paths over the map artwork.
        const std::array<sf::Vector2f, 9> nodeCenters = {{
            {345.f, 425.f}, {445.f, 230.f}, {545.f, 415.f},
            {645.f, 220.f}, {745.f, 410.f}, {845.f, 225.f},
            {945.f, 415.f}, {1045.f, 230.f}, {1125.f, 325.f}
        }};
        for (std::size_t i = 0; i + 1 < nodeCenters.size(); ++i) {
            const auto a = nodeCenters[i];
            const auto b = nodeCenters[i + 1];
            const sf::Vector2f diff = b - a;
            const float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            const float angle = std::atan2(diff.y, diff.x);
            const bool clearedEdge = static_cast<int>(i + 1) < floor_.roomNumber();
            sf::RectangleShape line({len, 4.f});
            line.setPosition(a);
            line.setRotation(sf::radians(angle));
            line.setFillColor(clearedEdge ? PositiveGreen : Border);
            canvas_.draw(line);
        }
        const auto inspectedIndex = hoveredMapNode_.value_or(std::min<std::size_t>(selectedMapNode_, 8));
        for (std::size_t i = 0; i < 9; ++i) {
            const auto center = nodeCenters[i];
            const sf::FloatRect nodeRect{{center.x - 44.f, center.y - 34.f}, {88.f, 68.f}};
            mapNodes_.push_back({nodeRect, i});
            const int roomNum = static_cast<int>(i) + 1;
            const bool cleared = roomNum < floor_.roomNumber();
            const bool current = roomNum == floor_.roomNumber();
            const bool inspected = i == inspectedIndex;
            const sf::Color fill = current ? sf::Color{62, 52, 30} : cleared ? sf::Color{24, 54, 48} : Surface;
            const sf::Color outline = inspected ? Gold : current ? Gold : cleared ? PositiveGreen : Border;
            panel(nodeRect, fill, outline);
            label("#" + std::to_string(roomNum) + " " + roomTypeBadge(floor_.route()[i]),
                  {nodeRect.position.x + 10.f, nodeRect.position.y + 8.f}, 14, current ? Gold : Ink);
            const std::string stateTag = cleared ? "CLEARED" : current ? "CURRENT" : "UPCOMING";
            label(stateTag, {nodeRect.position.x + 10.f, nodeRect.position.y + 36.f}, 13,
                  cleared ? PositiveGreen : current ? Gold : Muted);
        }

        // Detailed room card for the hovered/selected map node + full route legend.
        const auto inspectedRoom = floor_.route()[inspectedIndex];
        const int inspectedNum = static_cast<int>(inspectedIndex) + 1;
        const std::string inspectedState = inspectedNum < floor_.roomNumber() ? "CLEARED"
                                         : inspectedNum == floor_.roomNumber() ? "CURRENT CHAMBER" : "UPCOMING";
        panel({{248, 540}, {944, 102}}, {14, 22, 31}, Gold);
        label("ROOM " + std::to_string(inspectedNum) + " / 9  —  " + floor_.roomTitle(inspectedRoom) +
              "  [" + roomTypeBadge(inspectedRoom) + " / " + inspectedState + "]",
              {268, 552}, 19, Gold);
        paragraph(roomTypeDescription(inspectedRoom, floor_.floorNumber()) +
                  "  (Click any node or use Left/Right arrows to inspect; first three rooms shuffle each run.)",
                  {{268, 584}, {904, 48}}, 16);

        for (int i = 0; i < 9; ++i) {
            const float x = 256 + (i % 3) * 312.f, y = 656 + (i / 3) * 38.f;
            const std::string prefix = i + 1 < floor_.roomNumber() ? "DONE  " : i + 1 == floor_.roomNumber() ? "HERE  " : "NEXT  ";
            const bool isInspected = static_cast<std::size_t>(i) == inspectedIndex;
            label(prefix + std::to_string(i + 1) + "  [" + roomTypeBadge(floor_.route()[i]) + "] " + floor_.roomTitle(floor_.route()[i]),
                  {x, y}, 16, isInspected ? Gold : (i + 1 == floor_.roomNumber() ? PositiveGreen : Ink));
        }
        button("resume", "Return to the chamber [M / Esc]", {{248, 786}, {944, 52}});
    } else {
        panel({{430, 200}, {580, 495}}, Background, Border);
        label(overlay_ == Overlay::Quit ? "LEAVE THE DUNGEON?" : "PAUSED", {468, 232}, 33, Gold, true);
        // Feature #4: Display active run seed (and Daily Run date if active) on Pause menu.
        std::string seedInfo = "Run Seed: #" + std::to_string(floor_.seed());
        if (activeDailyRun_ && !activeDailyDate_.empty()) seedInfo += "  (Daily Run " + activeDailyDate_ + ")";
        label(seedInfo, {468, 284}, 18, Gold);
        paragraph(overlay_ == Overlay::Quit
            ? (screen_ == Screen::Results && entryPending_ ? "Your score has not been saved. Quit without entering your initials?" :
               timedRiddle() ? "The riddle timer is still running. Quit the game and end this descent?" : "Quit the game? Your unsaved progress will end unless you Save & Quit from Pause.")
            : "Pause is available between riddles. You can suspend your run and resume later from the main menu.", {{468, 322}, {504, 92}}, 20);
        button("resume", overlay_ == Overlay::Quit ? "Keep playing" : "Resume descent", {{468, 430}, {504, 54}});
        if (overlay_ == Overlay::Pause && floor_.canSuspend() && !timedRiddle()) {
            button("suspend-run", "Save & quit to menu (Suspend run)", {{468, 498}, {504, 54}});
            button("quit", "Abandon & quit game", {{468, 566}, {504, 54}});
        } else {
            button(overlay_ == Overlay::Quit ? "quit-yes" : "quit", "Quit game", {{468, 506}, {504, 56}});
        }
    }
}
void GameApp::render() {
    buttons_.clear();
    canvas_.clear(Background);
    switch (screen_) {
    case Screen::Intro: {
        const int frame = static_cast<int>(animationTime_ * 10) % 11 + 1;
        image("candle/candle" + std::to_string(frame) + ".jpg", {{0, 0}, {Width, Height}});
        panel({{0, 735}, {Width, 165}}, {4, 8, 14, 220}, sf::Color::Transparent);
        label("DUNGEON DESCENT", {478, 750}, 44, Gold, true);
        label("Press any key or click to begin", {555, 824}, 23, Ink);
        break;
    }
    case Screen::Menu: drawMenu(); break;
    case Screen::Introduction:
        image("images/startscreen.jpg", {{0, 0}, {Width, Height}}, true, {70, 85, 100});
        panel({{285, 65}, {870, 755}}, {12, 20, 29, 245}, Border);
        label("THE AWAKENING", {325, 95}, 38, Gold, true);
        reading(DungeonRun::readText(options_.assets / "textfiles/introduction/introduction.txt"), {{325, 166}, {790, 545}});
        button("heroes", "Choose your weapon", {{325, 742}, {790, 55}});
        break;
    case Screen::Heroes: drawHeroes(); break;
    case Screen::Difficulty: drawDifficulties(); break;
    case Screen::Biomes: drawBiomes(); break;
    case Screen::Game: drawGame(); break;
    case Screen::Lore: drawLore(); break;
    case Screen::Results: drawResults(); break;
    case Screen::Leaderboard: drawLeaderboard(true, true); break;
    }
    if (overlay_ != Overlay::None) drawOverlay();
    canvas_.display();
    window_.clear(sf::Color::Black);
    const auto size = window_.getSize();
    window_.setView(sf::View(sf::FloatRect({0, 0}, {static_cast<float>(size.x), static_cast<float>(size.y)})));
    const float scale = std::min(size.x / Width, size.y / Height);
    sf::Sprite screen(canvas_.getTexture());
    screen.setScale({scale, scale});
    screen.setPosition({(size.x - Width * scale) / 2, (size.y - Height * scale) / 2});
    window_.draw(screen);
    window_.display();
}
}
