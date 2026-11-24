#include "GameApp.h"
#include <algorithm>
#include <cmath>
#include <fstream>
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
std::string formatDuration(int seconds) {
    if (seconds < 0) seconds = 0;
    std::ostringstream out;
    out << seconds / 3600 << ':' << std::setfill('0') << std::setw(2) << (seconds / 60 % 60)
        << ':' << std::setw(2) << (seconds % 60);
    return out.str();
}
std::string endingHint(Ending ending) {
    switch (ending) {
    case Ending::Good: return "Defeat Malvel on Floor 3 without uncovering the full truth of Falkreath.";
    case Ending::True: return "Defeat Malvel with at least 5 Reputation and strong battle or riddle mastery.";
    case Ending::Bad: return "Fall to Malvel in the final chamber of Floor 3.";
    case Ending::Perfect: return "Achieve a flawless descent: win all 9 battles, solve all 24 riddles, and earn 9/9 Reputation.";
    case Ending::None: return {};
    }
    return {};
}
} // namespace

sf::String GameApp::utf8(const std::string& text) { return sf::String::fromUtf8(text.begin(), text.end()); }
bool GameApp::isAnimationFramePath(const std::string& name) {
    return name.rfind("images/startscreen/startscreen", 0) == 0 || name.rfind("candle/candle", 0) == 0;
}

GameApp::GameApp(LaunchOptions options)
    : options_(std::move(options)),
      dataDir_(options_.dataDirectory.empty() ? Leaderboards::dataDirectory() : options_.dataDirectory),
      floor_(options_.assets),
      prequel_(options_.assets),
      leaderboards_(dataDir_ / "leaderboards.tsv"),
      suspendSave_(dataDir_ / "suspend.sav"),
      settingsStore_(dataDir_ / "settings.cfg"),
      codexStore_(dataDir_ / "codex.tsv"),
      sfx_(options_.audio) {
    if (!bodyFont_.openFromFile(options_.assets / "fonts/Arial.ttf") ||
        !titleFont_.openFromFile(options_.assets / "fonts/VIKING-N.TTF"))
        throw std::runtime_error("Cannot load fonts from " + options_.assets.string());
    if (!canvas_.resize({static_cast<unsigned>(Width), static_cast<unsigned>(Height)}))
        throw std::runtime_error("Cannot create the game canvas.");
    canvas_.setSmooth(true);
    if (std::filesystem::exists(options_.assets / "images/window_icon.png")) {
        hasWindowIcon_ = windowIcon_.loadFromFile(options_.assets / "images/window_icon.png");
    }
    try { settings_ = settingsStore_.load(); } catch (const std::exception&) { settings_ = {}; }
    try { codex_ = codexStore_.load(); } catch (const std::exception&) { codex_ = {}; }
    muted_ = settings_.muted;
    sfx_.setMasterVolume(settings_.sfxVolume);
    sfx_.setMuted(muted_);
    openWindow(options_.smoke ? true : settings_.fullscreen);
    seed_ = std::random_device{}();
    gauntletRng_.seed(seed_ ^ 0x9e3779b9u);
    refreshSuspendPreview();
    if (options_.smoke) {
        floor_.start(Hero::Warrior, Biome::Ice, 7);
        screen_ = Screen::Game;
        syncStory();
    } else if (options_.skipIntro) enter(Screen::Menu);
    else enter(Screen::Intro);
}

GameApp::~GameApp() {
    const bool anyPlaying =
        (narration_ && narration_->getStatus() == sf::Music::Status::Playing) ||
        (music_ && music_->getStatus() == sf::Music::Status::Playing) ||
        (fadingMusic_ && fadingMusic_->getStatus() == sf::Music::Status::Playing);
    if (narration_) narration_->pause();
    if (music_) music_->pause();
    if (fadingMusic_) fadingMusic_->pause();
    if (anyPlaying) sf::sleep(sf::milliseconds(30));
    if (narration_) narration_->stop();
    if (music_) music_->stop();
    if (fadingMusic_) fadingMusic_->stop();
    sfx_.stopAll();
}

void GameApp::applyAudioVolumes() {
    const float musicTarget = muted_ ? 0.f : static_cast<float>(settings_.musicVolume);
    const float narrationTarget = muted_ ? 0.f : static_cast<float>(settings_.narrationVolume);
    const float fadeFactor = std::clamp(musicFadeProgress_, 0.f, 1.f);
    if (music_) music_->setVolume(musicTarget * fadeFactor);
    if (fadingMusic_) fadingMusic_->setVolume(musicTarget * (1.f - fadeFactor));
    if (narration_) narration_->setVolume(narrationTarget);
    sfx_.setMasterVolume(settings_.sfxVolume);
    sfx_.setMuted(muted_);
}

void GameApp::saveSettings() {
    settings_.muted = muted_;
    settings_.fullscreen = fullscreen_;
    try { settingsStore_.save(settings_); } catch (const std::exception&) {}
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

void GameApp::recordCodexIfNeeded() {
    if (floor_.inProgress() || floor_.runSerial() == 0 || codexRecordedSerial_ == floor_.runSerial()) return;
    if (floor_.phase() != Phase::Ending && floor_.phase() != Phase::Defeat) return;
    codexRecordedSerial_ = floor_.runSerial();
    const bool wasUnlocked = codex_.prequelUnlocked();
    try {
        codex_ = codexStore_.recordCompletedRun(floor_, static_cast<int>(playTime_));
        if (!wasUnlocked && codex_.prequelUnlocked()) {
            statusBanner_ = "THE SEAL BREAKS! Ashes of Falkreath (Prequel) unlocked on the Main Menu!";
        }
    } catch (const std::exception& error) {
        std::cerr << "Codex: " << error.what() << '\n';
    }
}

void GameApp::recordPrequelIfNeeded() {
    if (prequel_.phase() != PrequelPhase::Epilogue || prequel_.serial() == 0 ||
        prequelRecordedSerial_ == prequel_.serial()) return;
    prequelRecordedSerial_ = prequel_.serial();
    const int codaIdx = prequel_.coda() == PrequelCoda::HollowCrown ? 0
                      : prequel_.coda() == PrequelCoda::LastDoubt ? 1 : -1;
    try {
        codex_ = codexStore_.recordPrequelCompletion(codaIdx, prequel_.ironChronicleEligible());
    } catch (const std::exception& error) {
        std::cerr << "Codex (Prequel): " << error.what() << '\n';
    }
}

void GameApp::startGauntlet() {
    gauntlet_ = {};
    gauntlet_.active = true;
    gauntlet_.lives = 3;
    const auto& banks = floor_.sourceRiddles();
    for (int tier = 0; tier < 3; ++tier) {
        std::vector<Riddle> tierCopy = banks[static_cast<std::size_t>(tier)];
        std::shuffle(tierCopy.begin(), tierCopy.end(), gauntletRng_);
        for (auto& r : tierCopy) {
            const std::string correct = r.answers[r.correct];
            std::shuffle(r.answers.begin(), r.answers.end(), gauntletRng_);
            r.correct = static_cast<std::size_t>(std::find(r.answers.begin(), r.answers.end(), correct) - r.answers.begin());
            gauntlet_.deck.emplace_back(tier, std::move(r));
        }
    }
    gauntlet_.index = 0;
    nextGauntletQuestion();
}

void GameApp::nextGauntletQuestion() {
    if (gauntlet_.index >= gauntlet_.deck.size() || gauntlet_.lives <= 0) {
        gauntlet_.finished = true;
        gauntlet_.showingFeedback = false;
        clearSavedGauntlet();
        try { codex_ = codexStore_.recordGauntletResult(gauntlet_.score, gauntlet_.bestStreak); }
        catch (const std::exception&) {}
        return;
    }
    gauntlet_.currentTier = gauntlet_.deck[gauntlet_.index].first;
    gauntlet_.timeLimit = gauntlet_.currentTier == 0 ? 20.f : (gauntlet_.currentTier == 1 ? 15.f : 10.f);
    gauntlet_.timeLeft = gauntlet_.timeLimit;
    gauntlet_.showingFeedback = false;
    gauntlet_.feedback.clear();
    lastTimerTickSecond_ = -1;
}

void GameApp::resolveGauntletAnswer(bool correct, bool timeout) {
    if (!gauntlet_.active || gauntlet_.finished || gauntlet_.showingFeedback) return;
    ++gauntlet_.answered;
    gauntlet_.lastCorrect = correct;
    const auto& r = gauntlet_.deck[gauntlet_.index].second;
    if (correct) {
        ++gauntlet_.score;
        ++gauntlet_.streak;
        gauntlet_.bestStreak = std::max(gauntlet_.bestStreak, gauntlet_.streak);
        gauntlet_.feedback = "Correct! Current streak: " + std::to_string(gauntlet_.streak) + ".";
        sfx_.play(SfxId::RiddleCorrect, muted_);
    } else {
        gauntlet_.streak = 0;
        --gauntlet_.lives;
        gauntlet_.feedback = std::string(timeout ? "Time expired! " : "Incorrect. ") +
                             "The answer was: " + r.answers[r.correct] + ".";
        sfx_.play(SfxId::RiddleWrong, muted_);
    }
    gauntlet_.timeLeft = 0.f;
    gauntlet_.showingFeedback = true;
    ++gauntlet_.index;
    if (gauntlet_.lives <= 0 || gauntlet_.index >= gauntlet_.deck.size()) {
        gauntlet_.finished = true;
        clearSavedGauntlet();
        try { codex_ = codexStore_.recordGauntletResult(gauntlet_.score, gauntlet_.bestStreak); }
        catch (const std::exception&) {}
    }
}

namespace {
std::string hexEncodeStr(const std::string& input) {
    static constexpr char Digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char c : input) {
        out.push_back(Digits[(c >> 4) & 0xF]);
        out.push_back(Digits[c & 0xF]);
    }
    return out.empty() ? "-" : out;
}

std::string hexDecodeStr(const std::string& hex) {
    if (hex == "-") return {};
    if (hex.size() % 2 != 0) throw std::runtime_error("Corrupt hex string in saved gauntlet.");
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };
    std::string out;
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = nibble(hex[i]), lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) throw std::runtime_error("Invalid hex character in saved gauntlet.");
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}
} // namespace

std::filesystem::path GameApp::gauntletSavePath() const {
    return options_.dataDirectory / "gauntlet_save.dat";
}

bool GameApp::hasSavedGauntlet() const {
    std::error_code ec;
    return std::filesystem::is_regular_file(gauntletSavePath(), ec);
}

void GameApp::clearSavedGauntlet() const {
    std::error_code ec;
    std::filesystem::remove(gauntletSavePath(), ec);
}

void GameApp::saveGauntletRun() const {
    if (!gauntlet_.active || gauntlet_.finished || gauntlet_.deck.empty()) return;
    const auto path = gauntletSavePath();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Could not write saved gauntlet file.");
    out << "GAUNTLET_SAVE 1\n";
    out << (gauntlet_.showingFeedback ? 1 : 0) << ' ' << (gauntlet_.lastCorrect ? 1 : 0) << ' '
        << gauntlet_.score << ' ' << gauntlet_.streak << ' ' << gauntlet_.bestStreak << ' '
        << gauntlet_.lives << ' ' << gauntlet_.answered << ' ' << gauntlet_.currentTier << ' '
        << gauntlet_.index << ' ' << gauntlet_.timeLimit << ' ' << gauntlet_.timeLeft << ' '
        << gauntlet_.deck.size() << '\n';
    out << hexEncodeStr(gauntlet_.feedback) << '\n';
    for (const auto& [tier, r] : gauntlet_.deck) {
        out << tier << ' ' << r.correct << ' ' << hexEncodeStr(r.question) << ' '
            << hexEncodeStr(r.answers[0]) << ' ' << hexEncodeStr(r.answers[1]) << ' '
            << hexEncodeStr(r.answers[2]) << '\n';
    }
}

bool GameApp::loadGauntletRun() {
    if (!hasSavedGauntlet()) return false;
    try {
        std::ifstream in(gauntletSavePath(), std::ios::binary);
        std::string header;
        int version = 0;
        if (!(in >> header >> version) || header != "GAUNTLET_SAVE" || version != 1) return false;
        GauntletState loaded;
        loaded.active = true;
        int feedbackFlag = 0, lastOkFlag = 0;
        std::size_t deckSize = 0;
        if (!(in >> feedbackFlag >> lastOkFlag >> loaded.score >> loaded.streak >> loaded.bestStreak >>
              loaded.lives >> loaded.answered >> loaded.currentTier >> loaded.index >>
              loaded.timeLimit >> loaded.timeLeft >> deckSize) ||
            deckSize == 0 || loaded.lives <= 0 || loaded.index > deckSize) return false;
        loaded.showingFeedback = feedbackFlag != 0;
        loaded.lastCorrect = lastOkFlag != 0;
        loaded.timeLeft = std::max(0.5f, loaded.timeLeft);
        std::string fbHex;
        if (!(in >> fbHex)) return false;
        loaded.feedback = hexDecodeStr(fbHex);
        loaded.deck.reserve(deckSize);
        for (std::size_t i = 0; i < deckSize; ++i) {
            int tier = 0;
            std::size_t correct = 0;
            std::string qHex, a0Hex, a1Hex, a2Hex;
            if (!(in >> tier >> correct >> qHex >> a0Hex >> a1Hex >> a2Hex) || correct >= 3) return false;
            Riddle r;
            r.question = hexDecodeStr(qHex);
            r.answers = {hexDecodeStr(a0Hex), hexDecodeStr(a1Hex), hexDecodeStr(a2Hex)};
            r.correct = correct;
            loaded.deck.emplace_back(tier, std::move(r));
        }
        gauntlet_ = std::move(loaded);
        lastTimerTickSecond_ = -1;
        return true;
    } catch (const std::exception&) {
        clearSavedGauntlet();
        return false;
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
    if (hasWindowIcon_) window_.setIcon(windowIcon_);
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
        std::string counterText = "COUNTER -" + num(exchange.counterDamage, 2) + " HP";
        if (exchange.strikesDrained > 0) counterText += " (-1 STRIKE)";
        combatBadges_.push_back({counterText, NegativeRed, 1.5f, 1.5f, 110.f});
        if (exchange.enemyHealed > 0.f) {
            combatBadges_.push_back({"BOSS +" + num(exchange.enemyHealed, 1) + " HP", Mist, 1.5f, 1.5f, 110.f});
        }
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
        if (pt && screen_ == Screen::Game && overlay_ == Overlay::None) {
            for (const auto& target : statHoverTargets_) {
                if (target.bounds.contains(*pt)) { hoveredStat_ = target.stat; break; }
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
            if (letter >= 'A' && letter <= 'Z') { initials_ += static_cast<char>(letter); selectedButton_ = 5; }
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
        if (key->code == Key::F11) {
            openWindow(!fullscreen_);
            saveSettings();
            return;
        }
        if (enteringInitials) {
            if (key->code == Key::Backspace) { if (!initials_.empty()) initials_.pop_back(); selectedButton_ = 5; return; }
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
        if (key->code == Key::Escape) {
            if (overlay_ == Overlay::Settings) {
                overlay_ = settingsReturnOverlay_;
                buttons_.clear();
                selectedButton_ = 0;
            } else if (overlay_ != Overlay::None) {
                overlay_ = Overlay::None; buttons_.clear();
                if (screen_ == Screen::Results && entryPending_) selectedButton_ = 5;
            }
            else if (screen_ == Screen::Intro) enter(Screen::Menu);
            else if (screen_ == Screen::Lore) action("lore-back");
            else if (screen_ == Screen::Leaderboard) action("leaderboard-back");
            else if (screen_ == Screen::Codex) enter(Screen::Menu);
            else if (screen_ == Screen::Gauntlet) {
                if (gauntlet_.finished) enter(Screen::Menu);
                else { overlay_ = Overlay::Pause; buttons_.clear(); selectedButton_ = 0; }
            }
            else if (screen_ == Screen::Prequel) {
                if (!timedRiddle()) enter(Screen::Menu);
            }
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
        if (key->code == Key::Right || key->code == Key::PageDown) {
            action((screen_ == Screen::Leaderboard || screen_ == Screen::Results) ? "board-next" : "page-next");
            return;
        }
        if (key->code == Key::Left || key->code == Key::PageUp) {
            action((screen_ == Screen::Leaderboard || screen_ == Screen::Results) ? "board-prev" : "page-prev");
            return;
        }
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
        if (overlay_ == Overlay::None && wheel->wheel == sf::Mouse::Wheel::Vertical) {
            const bool onBoard = (screen_ == Screen::Leaderboard || screen_ == Screen::Results);
            action(wheel->delta < 0 ? (onBoard ? "board-next" : "page-next") : (onBoard ? "board-prev" : "page-prev"));
        }
    }
}

void GameApp::enter(Screen screen) {
    if (narration_ && narration_->getStatus() == sf::Music::Status::Playing) {
        narration_->pause();
        sf::sleep(sf::milliseconds(20));
        narration_->stop();
    }
    screen_ = screen;
    overlay_ = Overlay::None;
    editingSeed_ = false;
    page_ = 0;
    readingText_.clear();
    buttons_.clear();
    selectedButton_ = screen == Screen::Difficulty ? static_cast<std::size_t>(selectedDifficulty_) : 0;
    animationTime_ = 0;
    screenFade_ = 1.f;
    if (screen == Screen::Intro) { playMusic("DD.ogg"); playIntroNarration(); }
    if (screen == Screen::Menu) {
        refreshSuspendPreview();
        try { codex_ = codexStore_.load(); } catch (const std::exception&) {}
        playMusic("menu.mp3");
    }
    if (screen == Screen::Codex) {
        try { codex_ = codexStore_.load(); } catch (const std::exception&) {}
        selectedButton_ = static_cast<std::size_t>(codexTab_);
        syncCodexMusic();
    }
    if (screen == Screen::Gauntlet) {
        if (!loadGauntletRun()) startGauntlet();
        playMusic("menu.mp3");
    }
    if (screen == Screen::Prequel) {
        prequel_.start();
        combatBadges_.clear();
        playMusic(prequel_.musicFile());
    }
    if (screen == Screen::Leaderboard) {
        leaderboardPage_ = 0;
        leaderboardHeroFilter_ = -1;
        leaderboardEndingFilter_ = 0;
        try { leaderboardTables_ = leaderboards_.load(); leaderboardStatus_.clear(); }
        catch (const std::exception& error) {
            std::cerr << "Leaderboards: " << error.what() << '\n';
            leaderboardTables_ = {};
            leaderboardStatus_ = "Scores could not be loaded.";
        }
        selectedButton_ = leaderboardAllDifficulties_ ? 0 : static_cast<std::size_t>(leaderboardDifficulty_) + 1;
    }
    if (screen == Screen::Results) {
        recordCodexIfNeeded();
        prepareResults();
        selectedButton_ = hasEndingResult() && entryPending_ ? 5 : (leaderboardAllDifficulties_ ? 0 : static_cast<std::size_t>(leaderboardDifficulty_) + 1);
    }
}

void GameApp::prepareResults() {
    if (!hasEndingResult()) {
        entryPending_ = false;
        leaderboardDifficulty_ = floor_.difficulty();
        leaderboardAllDifficulties_ = true;
        leaderboardHeroFilter_ = -1;
        leaderboardEndingFilter_ = 0;
        leaderboardPage_ = 0;
        leaderboardStatus_ = "Fallen descents do not submit a score. Browse the high scores below.";
    } else if (resultsRunSerial_ != floor_.runSerial()) {
        resultsRunSerial_ = floor_.runSerial();
        entryPending_ = true;
        initials_.clear(); leaderboardStatus_.clear(); leaderboardEntryId_.reset();
        leaderboardDifficulty_ = floor_.difficulty();
        leaderboardAllDifficulties_ = true;
        leaderboardHeroFilter_ = -1;
        leaderboardEndingFilter_ = 0;
        leaderboardPage_ = 0;
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
        const int playSeconds = static_cast<int>(playTime_);
        const auto result = leaderboards_.submit(floor_.difficulty(), initials_, floor_.score(playSeconds),
                                                 floor_.player().hero, floor_.ending(),
                                                 floor_.seed(), playSeconds);
        leaderboardTables_ = result.tables;
        leaderboardEntryId_ = result.id;
        entryPending_ = false;
        leaderboardDifficulty_ = floor_.difficulty();
        leaderboardAllDifficulties_ = false;
        leaderboardHeroFilter_ = -1;
        leaderboardEndingFilter_ = 0;
        leaderboardPage_ = result.rank ? static_cast<int>((*result.rank - 1) / Leaderboards::PageSize) : 0;
        leaderboardStatus_ = result.rank ? initials_ + " placed #" + std::to_string(*result.rank) + " on " + DungeonRun::difficultyName(floor_.difficulty()) + "."
            : "Score saved.";
        sfx_.play(SfxId::UiClick, muted_);
    } catch (const std::exception& error) {
        std::cerr << "Leaderboards: " << error.what() << '\n';
        leaderboardStatus_ = "Could not save your score. Try again or skip this entry.";
    }
    buttons_.clear(); selectedButton_ = entryPending_ ? 5 : (leaderboardAllDifficulties_ ? 0 : static_cast<std::size_t>(leaderboardDifficulty_) + 1);
}

void GameApp::action(const std::string& id) {
    // Navigation may not replace a live descent, including floor transitions
    // and lore. A new hero can only be chosen before a run or after it ends.
    if (floor_.inProgress() && (id == "menu" || id == "start" || id == "heroes" || id == "restart" ||
                                id == "difficulty" || id == "resume-run" || id == "codex" || id == "gauntlet" ||
                                id == "prequel" || id.rfind("replay-", 0) == 0)) return;
    if (timedRiddle() && (id == "pause" || id == "map" || id == "lore" || id == "suspend-run" || id == "abandon-to-menu" || id == "settings")) return;
    if (id == "save-score") { submitScore(); return; }
    if (id == "skip-score") {
        if (screen_ == Screen::Results && hasEndingResult() && entryPending_) {
            entryPending_ = false; leaderboardStatus_ = "Score entry skipped.";
            sfx_.play(SfxId::UiClick, muted_);
            buttons_.clear(); selectedButton_ = leaderboardAllDifficulties_ ? 0 : static_cast<std::size_t>(leaderboardDifficulty_) + 1;
        }
        return;
    }
    if (id == "board-prev") {
        if (overlay_ == Overlay::None && (screen_ == Screen::Leaderboard || screen_ == Screen::Results) && leaderboardPage_ > 0) {
            --leaderboardPage_;
            sfx_.play(SfxId::UiClick, muted_);
        }
        return;
    }
    if (id == "board-next") {
        if (overlay_ == Overlay::None && (screen_ == Screen::Leaderboard || screen_ == Screen::Results) && leaderboardPage_ + 1 < leaderboardPageCount_) {
            ++leaderboardPage_;
            sfx_.play(SfxId::UiClick, muted_);
        }
        return;
    }
    if (id.rfind("filter-hero-", 0) == 0) {
        const bool canBrowseBoards = (screen_ == Screen::Leaderboard) || (screen_ == Screen::Results && !entryPending_);
        if (canBrowseBoards) {
            const int val = std::stoi(id.substr(12));
            if (val >= -1 && val <= 3) {
                leaderboardHeroFilter_ = val;
                leaderboardPage_ = 0;
                sfx_.play(SfxId::UiClick, muted_);
                buttons_.clear();
            }
        }
        return;
    }
    if (id.rfind("filter-ending-", 0) == 0) {
        const bool canBrowseBoards = (screen_ == Screen::Leaderboard) || (screen_ == Screen::Results && !entryPending_);
        if (canBrowseBoards) {
            const int val = std::stoi(id.substr(14));
            if (val >= 0 && val <= 4) {
                leaderboardEndingFilter_ = val;
                leaderboardPage_ = 0;
                sfx_.play(SfxId::UiClick, muted_);
                buttons_.clear();
            }
        }
        return;
    }
    if (id.rfind("replay-", 0) == 0) {
        const bool canBrowseBoards = (screen_ == Screen::Leaderboard) || (screen_ == Screen::Results && !entryPending_);
        if (canBrowseBoards && !floor_.inProgress()) {
            const std::uint64_t targetId = std::stoull(id.substr(7));
            auto tryReplayTable = [&](Difficulty diff) -> bool {
                const auto& table = leaderboardTables_[static_cast<std::size_t>(diff)];
                const auto it = std::find_if(table.begin(), table.end(), [&](const auto& e) { return e.id == targetId; });
                if (it != table.end() && it->seed != 0) {
                    selectedHero_ = it->hero;
                    selectedDifficulty_ = diff;
                    seedInput_ = std::to_string(it->seed);
                    dailyRunSelected_ = false;
                    editingSeed_ = false;
                    choosingSecondFloor_ = false;
                    sfx_.play(SfxId::UiClick, muted_);
                    enter(Screen::Biomes);
                    return true;
                }
                return false;
            };
            if (leaderboardAllDifficulties_) {
                for (int mode = 0; mode < 4; ++mode) {
                    if (tryReplayTable(static_cast<Difficulty>(mode))) return;
                }
            } else {
                tryReplayTable(leaderboardDifficulty_);
            }
        }
        return;
    }
    if (id == "board-all") {
        const bool canBrowseBoards = (screen_ == Screen::Leaderboard) || (screen_ == Screen::Results && !entryPending_);
        if (canBrowseBoards) {
            leaderboardAllDifficulties_ = true;
            leaderboardPage_ = 0;
            selectedButton_ = 0;
            sfx_.play(SfxId::UiClick, muted_);
            buttons_.clear();
        }
        return;
    }
    if (id.rfind("board-", 0) == 0) {
        const bool canBrowseBoards = (screen_ == Screen::Leaderboard) || (screen_ == Screen::Results && !entryPending_);
        if (canBrowseBoards && id.size() == 7 && id[6] >= '0' && id[6] <= '3') {
            leaderboardAllDifficulties_ = false;
            leaderboardDifficulty_ = static_cast<Difficulty>(id[6] - '0');
            leaderboardPage_ = 0;
            selectedButton_ = static_cast<std::size_t>(leaderboardDifficulty_) + 1;
            sfx_.play(SfxId::UiClick, muted_);
            buttons_.clear();
        }
        return;
    }
    if (screen_ == Screen::Results && entryPending_ && id == "menu") return;
    if (id == "sound") {
        muted_ = !muted_;
        applyAudioVolumes();
        saveSettings();
        if (!muted_) sfx_.play(SfxId::UiClick, muted_);
        return;
    }
    if (id == "settings") {
        sfx_.play(SfxId::UiClick, muted_);
        settingsReturnOverlay_ = (overlay_ == Overlay::Pause) ? Overlay::Pause : Overlay::None;
        overlay_ = Overlay::Settings;
        buttons_.clear();
        selectedButton_ = 0;
        return;
    }
    if (id == "settings-close") {
        sfx_.play(SfxId::UiClick, muted_);
        overlay_ = settingsReturnOverlay_;
        buttons_.clear();
        selectedButton_ = 0;
        return;
    }
    if (id == "vol-music-down" || id == "vol-music-up") {
        settings_.musicVolume = std::clamp(settings_.musicVolume + (id == "vol-music-up" ? 10 : -10), 0, 100);
        applyAudioVolumes(); saveSettings();
        sfx_.play(SfxId::UiClick, muted_);
        return;
    }
    if (id == "vol-sfx-down" || id == "vol-sfx-up") {
        settings_.sfxVolume = std::clamp(settings_.sfxVolume + (id == "vol-sfx-up" ? 10 : -10), 0, 100);
        applyAudioVolumes(); saveSettings();
        sfx_.play(SfxId::UiClick, muted_);
        return;
    }
    if (id == "vol-narr-down" || id == "vol-narr-up") {
        settings_.narrationVolume = std::clamp(settings_.narrationVolume + (id == "vol-narr-up" ? 10 : -10), 0, 100);
        applyAudioVolumes(); saveSettings();
        sfx_.play(SfxId::UiClick, muted_);
        return;
    }
    if (id == "toggle-fullscreen") {
        sfx_.play(SfxId::UiClick, muted_);
        openWindow(!fullscreen_);
        saveSettings();
        return;
    }
    if (id == "codex") {
        sfx_.play(SfxId::UiClick, muted_);
        codexTab_ = 0;
        codexMilestonePage_ = 0;
        enter(Screen::Codex);
        return;
    }
    if (id == "codex-tab-0" || id == "codex-tab-1") {
        sfx_.play(SfxId::UiClick, muted_);
        codexTab_ = (id == "codex-tab-1") ? 1 : 0;
        codexMilestonePage_ = 0;
        page_ = 0;
        readingText_.clear();
        buttons_.clear();
        selectedButton_ = static_cast<std::size_t>(codexTab_);
        syncCodexMusic();
        return;
    }
    if (id == "codex-ms-prev" || id == "codex-ms-next") {
        if (screen_ == Screen::Codex && codexTab_ == 0) {
            const int totalMs = static_cast<int>(codex_.milestones().size());
            const int totalPages = std::max(1, (totalMs + 7) / 8);
            const int target = std::clamp(codexMilestonePage_ + (id == "codex-ms-next" ? 1 : -1), 0, totalPages - 1);
            if (target != codexMilestonePage_) {
                codexMilestonePage_ = target;
                sfx_.play(SfxId::UiClick, muted_);
                buttons_.clear();
            }
        }
        return;
    }
    if (id.rfind("codex-ending-", 0) == 0) {
        const int e = std::stoi(id.substr(13));
        if (e >= 1 && e <= 4) {
            sfx_.play(SfxId::UiClick, muted_);
            codexEnding_ = static_cast<Ending>(e);
            page_ = 0;
            readingText_.clear();
            syncCodexMusic();
        }
        return;
    }
    if (id == "gauntlet") {
        sfx_.play(SfxId::UiClick, muted_);
        enter(Screen::Gauntlet);
        return;
    }
    if (id == "gauntlet-next") {
        sfx_.play(SfxId::UiClick, muted_);
        nextGauntletQuestion();
        buttons_.clear();
        selectedButton_ = 0;
        return;
    }
    if (id == "gauntlet-retry") {
        sfx_.play(SfxId::UiClick, muted_);
        clearSavedGauntlet();
        startGauntlet();
        buttons_.clear();
        selectedButton_ = 0;
        return;
    }
    if (id == "gauntlet-save") {
        if (screen_ == Screen::Gauntlet && gauntlet_.active && !gauntlet_.finished) {
            try {
                saveGauntletRun();
                if (gauntlet_.score > 0 || gauntlet_.bestStreak > 0) {
                    codex_ = codexStore_.recordGauntletResult(gauntlet_.score, gauntlet_.bestStreak);
                }
                gauntlet_.active = false;
                overlay_ = Overlay::None;
                statusBanner_ = "Riddle Gauntlet saved — resume anytime from the Main Menu.";
                sfx_.play(SfxId::UiClick, muted_);
                enter(Screen::Menu);
            } catch (const std::exception& error) {
                std::cerr << "GauntletSave: " << error.what() << '\n';
                statusBanner_ = "Could not save Riddle Gauntlet.";
            }
        }
        return;
    }
    if (id == "gauntlet-discard") {
        if (screen_ == Screen::Gauntlet) {
            clearSavedGauntlet();
            gauntlet_ = {};
            overlay_ = Overlay::None;
            statusBanner_ = "Riddle Gauntlet run discarded.";
            sfx_.play(SfxId::UiClick, muted_);
            enter(Screen::Menu);
        }
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
        leaderboardAllDifficulties_ = true;
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
    if (id == "abandon-to-menu") {
        if (screen_ == Screen::Game && overlay_ == Overlay::Pause && !timedRiddle()) {
            floor_ = DungeonRun(options_.assets);
            overlay_ = Overlay::None;
            statusBanner_ = "Returned to main menu without saving the current run.";
            sfx_.play(SfxId::UiClick, muted_);
            enter(Screen::Menu);
        }
        return;
    }
    if (id == "quit") { sfx_.play(SfxId::UiClick, muted_); overlay_ = Overlay::Quit; }
    else if (id == "quit-yes") {
        if (narration_) narration_->pause();
        if (music_) music_->pause();
        if (fadingMusic_) fadingMusic_->pause();
        sf::sleep(sf::milliseconds(20));
        if (narration_) narration_->stop();
        if (music_) music_->stop();
        if (fadingMusic_) { fadingMusic_->stop(); fadingMusic_.reset(); }
        sfx_.stopAll();
        window_.close();
    }
    else if (id == "resume") { sfx_.play(SfxId::UiClick, muted_); overlay_ = Overlay::None; }
    else if (id == "pause") { sfx_.play(SfxId::UiClick, muted_); overlay_ = Overlay::Pause; }
    else if (id == "map") {
        sfx_.play(SfxId::UiClick, muted_);
        overlay_ = overlay_ == Overlay::Map ? Overlay::None : Overlay::Map;
    }
    else if (id == "menu") {
        if (screen_ == Screen::Gauntlet && gauntlet_.active && (gauntlet_.score > 0 || gauntlet_.bestStreak > 0)) {
            try { codex_ = codexStore_.recordGauntletResult(gauntlet_.score, gauntlet_.bestStreak); }
            catch (const std::exception&) {}
            gauntlet_.active = false;
        }
        sfx_.play(SfxId::UiClick, muted_);
        enter(Screen::Menu);
    }
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
        const int prevRoom = floor_.roomNumber();
        const bool hadChest = floor_.room() == Room::Chest && prevPhase == Phase::Loot;
        floor_.advance();
        recordPhaseFeedback(prevPhase, hadChest);
        if (prevPhase != Phase::Loot) sfx_.play(SfxId::UiClick, muted_);
        if (floor_.roomNumber() != prevRoom) screenFade_ = 0.7f;
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
            recordCodexIfNeeded();
            if (floor_.phase() == Phase::Ending) playMusic(floor_.musicFile());
            try { suspendSave_.clear(); } catch (const std::exception&) {}
            refreshSuspendPreview();
        }
    }
    else if (id == "prequel") {
        if (prequelUnlocked()) {
            sfx_.play(SfxId::UiClick, muted_);
            statusBanner_.clear();
            enter(Screen::Prequel);
        } else {
            sfx_.play(SfxId::RiddleWrong, muted_);
            statusBanner_ = "Sealed Chronicle: Earn the Perfect Ending on Impossible difficulty to break the seal.";
        }
    }
    else if (id == "prequel-advance" && screen_ == Screen::Prequel) {
        const auto prevAct = prequel_.act();
        prequel_.advance();
        combatBadges_.clear();
        flash_ = 0.f;
        shakeTime_ = 0.f;
        sfx_.play(SfxId::UiClick, muted_);
        if (prequel_.act() != prevAct || prequel_.phase() == PrequelPhase::Epilogue) screenFade_ = 0.85f;
        page_ = 0;
        readingText_.clear();
        playMusic(prequel_.musicFile());
        recordPrequelIfNeeded();
    }
    else if (id == "prequel-retry-act" && screen_ == Screen::Prequel) {
        sfx_.play(SfxId::UiClick, muted_);
        prequel_.restartAct();
        combatBadges_.clear();
        page_ = 0;
        readingText_.clear();
        screenFade_ = 0.7f;
        playMusic(prequel_.musicFile());
    }
    else if (id == "prequel-restart" && screen_ == Screen::Prequel) {
        sfx_.play(SfxId::UiClick, muted_);
        prequel_.start();
        combatBadges_.clear();
        page_ = 0;
        readingText_.clear();
        screenFade_ = 0.7f;
        playMusic(prequel_.musicFile());
    }
    else if (id == "prequel-to-main" && screen_ == Screen::Prequel) {
        sfx_.play(SfxId::UiClick, muted_);
        statusBanner_.clear();
        enter(Screen::Introduction);
    }
    else if (id.rfind("choice-", 0) == 0) {
        const auto choice = static_cast<std::size_t>(std::stoi(id.substr(7)));
        if (screen_ == Screen::Gauntlet && overlay_ == Overlay::None) {
            if (gauntlet_.active && !gauntlet_.finished && !gauntlet_.showingFeedback && choice < 3) {
                resolveGauntletAnswer(choice == gauntlet_.deck[gauntlet_.index].second.correct, false);
            }
        }
        else if (screen_ == Screen::Prequel && overlay_ == Overlay::None) {
            const auto prevPhase = prequel_.phase();
            if (prevPhase == PrequelPhase::Scene) {
                sfx_.play(SfxId::UiClick, muted_);
                prequel_.choose(choice);
            } else if (prevPhase == PrequelPhase::Parley) {
                const bool ok = (choice == prequel_.parley().bestIndex);
                prequel_.answerParley(choice);
                sfx_.play(ok ? SfxId::RiddleCorrect : SfxId::RiddleWrong, muted_);
            } else if (prevPhase == PrequelPhase::Combat) {
                const auto actChoice = static_cast<CombatAction>(choice);
                if (attackDelay_ > 0 || !prequel_.combatChoiceAvailable(actChoice)) return;
                prequel_.attack(actChoice);
                triggerCombatFeedback(prequel_.lastCombatExchange());
                attackDelay_ = .32f;
            }
            page_ = 0;
            readingText_.clear();
            playMusic(prequel_.musicFile());
            recordPrequelIfNeeded();
        }
        else if (screen_ == Screen::Heroes) {
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
                recordCodexIfNeeded();
                if (floor_.phase() == Phase::Ending) playMusic(floor_.musicFile());
                try { suspendSave_.clear(); } catch (const std::exception&) {}
                refreshSuspendPreview();
            }
        }
    }
    buttons_.clear(); // No stale hit targets survive transitions or queued clicks.
    const std::size_t lbTabBtn = leaderboardAllDifficulties_ ? 0 : static_cast<std::size_t>(leaderboardDifficulty_) + 1;
    selectedButton_ = overlay_ != Overlay::None ? 0 : screen_ == Screen::Lore ? static_cast<std::size_t>(loreHero_) :
                      screen_ == Screen::Difficulty ? static_cast<std::size_t>(selectedDifficulty_) :
                      screen_ == Screen::Codex ? static_cast<std::size_t>(codexTab_) :
                      screen_ == Screen::Leaderboard ? lbTabBtn :
                      screen_ == Screen::Results ? (hasEndingResult() && entryPending_ ? 5 : lbTabBtn) : 0;
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
    const float clampedDt = std::isfinite(dt) && dt > 0 ? std::min(dt, .1f) : 0.f;
    animationTime_ += clampedDt;
    attackDelay_ = std::max(0.f, attackDelay_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shakeTime_ = std::max(0.f, shakeTime_ - dt);
    screenFade_ = std::max(0.f, screenFade_ - clampedDt / 0.22f);

    // Smooth music crossfading between tracks.
    if (musicFadeProgress_ < 1.f && clampedDt > 0.f) {
        musicFadeProgress_ = std::min(1.f, musicFadeProgress_ + clampedDt / MusicFadeDuration);
        applyAudioVolumes();
        if (musicFadeProgress_ >= 1.f && fadingMusic_) {
            fadingMusic_->pause();
            sf::sleep(sf::milliseconds(25));
            fadingMusic_.reset();
        }
    }

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
                if (!floor_.inProgress()) recordCodexIfNeeded();
            }
        }
    } else if (screen_ == Screen::Prequel && prequel_.phase() == PrequelPhase::Parley) {
        const auto prevPhase = prequel_.phase();
        prequel_.tickParley(dt);
        if (prequel_.phase() == PrequelPhase::Parley) {
            const int secsLeft = static_cast<int>(std::ceil(prequel_.parley().timeLeft));
            if (secsLeft >= 1 && secsLeft <= 5 && secsLeft != lastTimerTickSecond_) {
                lastTimerTickSecond_ = secsLeft;
                sfx_.play(SfxId::TimerTick, muted_);
            }
        } else {
            lastTimerTickSecond_ = -1;
            if (prevPhase == PrequelPhase::Parley) {
                sfx_.play(SfxId::RiddleWrong, muted_);
                page_ = 0;
                readingText_.clear();
            }
        }
    } else if (screen_ == Screen::Gauntlet && overlay_ == Overlay::None && gauntlet_.active && !gauntlet_.finished && !gauntlet_.showingFeedback) {
        if (std::isfinite(dt) && dt > 0.f) {
            gauntlet_.timeLeft = std::max(0.f, gauntlet_.timeLeft - dt);
            const int secsLeft = static_cast<int>(std::ceil(gauntlet_.timeLeft));
            if (secsLeft >= 1 && secsLeft <= 5 && secsLeft != lastTimerTickSecond_) {
                lastTimerTickSecond_ = secsLeft;
                sfx_.play(SfxId::TimerTick, muted_);
            }
            if (gauntlet_.timeLeft <= 0.f) {
                resolveGauntletAnswer(false, true);
            }
        }
    } else {
        lastTimerTickSecond_ = -1;
    }
}

void GameApp::playMusic(const std::string& file) {
    if (!options_.audio || musicFile_ == file) return;
    auto nextMusic = std::make_unique<sf::Music>();
    if (!nextMusic->openFromFile(options_.assets / ("music/" + file))) {
        std::cerr << "Music unavailable: " << file << '\n';
        return;
    }
    const bool hadPlaying = music_ && music_->getStatus() == sf::Music::Status::Playing;
    if (fadingMusic_) {
        fadingMusic_->pause();
        sf::sleep(sf::milliseconds(25));
        fadingMusic_.reset();
    }
    musicFile_ = file;
    nextMusic->setLooping(true);
    if (hadPlaying) {
        fadingMusic_ = std::move(music_);
        music_ = std::move(nextMusic);
        musicFadeProgress_ = 0.f;
        music_->setVolume(0.f);
        music_->play();
    } else {
        music_ = std::move(nextMusic);
        musicFadeProgress_ = 1.f;
        music_->setVolume(muted_ ? 0.f : static_cast<float>(settings_.musicVolume));
        music_->play();
    }
}

void GameApp::syncCodexMusic() {
    if (screen_ == Screen::Codex && codexTab_ == 1 && codex_.endingUnlocked(codexEnding_)) {
        playMusic(DungeonRun::endingMusicFile(codexEnding_));
    } else {
        playMusic("menu.mp3");
    }
}

void GameApp::playIntroNarration() {
    if (!options_.audio) return;
    if (!narration_) narration_ = std::make_unique<sf::Music>();
    if (!narration_->openFromFile(options_.assets / "music/intro.ogg")) {
        std::cerr << "Intro narration unavailable: intro.ogg\n";
        return;
    }
    // Keep the original voice-over louder than the looping intro soundtrack.
    narration_->setLooping(false);
    narration_->setVolume(muted_ ? 0.f : static_cast<float>(settings_.narrationVolume));
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
    if (activeTab.has_value() && selected)
        panel({{rect.position.x + 15, rect.position.y + rect.size.y - 5}, {rect.size.x - 30, 2}}, Ink, sf::Color::Transparent);
    const unsigned fontSize = rect.size.y <= 32.f ? 15u : 18u;
    const float lineStep = fontSize == 15u ? 18.f : 23.f;
    const float padX = rect.size.x <= 60.f ? 14.f : 15.f;
    const auto lines = wrap(text, fontSize, std::max(20.f, rect.size.x - padX * 2.f));
    const float total = std::min(2.f, static_cast<float>(lines.size())) * lineStep;
    for (std::size_t i = 0; i < lines.size() && i < 2; ++i) {
        sf::Text rendered(bodyFont_, lines[i], fontSize);
        rendered.setFillColor(enabled ? Ink : Muted);
        rendered.setPosition({rect.position.x + padX, rect.position.y + (rect.size.y - total) / 2 + i * lineStep - 2});
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
    label("DUNGEON", {78, 174}, 88, Ink, true);
    label("DESCENT", {78, 271}, 88, Gold, true);
    paragraph("Every choice leaves a mark.\nChoose your hero, challenge the riddle keeper\nand uncover the truth beneath Aethoria.", {{85, 396}, {680, 115}}, 22);
    if (!statusBanner_.empty()) {
        label(statusBanner_, {85, 514}, 17, Gold);
    }
    if (suspendPreview_.has_value()) {
        std::string resumeLabel = "Resume suspended run (Seed #" + std::to_string(suspendPreview_->seed);
        if (suspendPreview_->dailyRun && !suspendPreview_->dailyDate.empty()) resumeLabel += " / Daily " + suspendPreview_->dailyDate;
        resumeLabel += ")";
        button("resume-run", resumeLabel, {{85, 544}, {539, 48}});
    }
    button("start", "Begin descent", {{85, 606}, {310, 56}});
    button("leaderboards", "High scores", {{414, 606}, {210, 56}});
    button("codex", "Codex & Gallery (" + std::to_string(codex_.unlockedEndingCount()) + "/4)", {{85, 674}, {310, 52}});
    button("gauntlet", hasSavedGauntlet() ? "Riddle Gauntlet (Resume)" : "Riddle Gauntlet", {{414, 674}, {210, 52}});
    button("lore", "Lore of the heroes", {{85, 738}, {310, 52}});
    button("settings", "Settings", {{414, 738}, {108, 52}});
    button("quit", "Quit", {{534, 738}, {90, 52}});
    if (prequelUnlocked()) {
        button("prequel", "Ashes of Falkreath  (Prequel Campaign)", {{85, 802}, {539, 50}}, true, true);
    } else {
        button("prequel", "Sealed Chronicle  [Perfect Run on Impossible to Unlock]", {{85, 802}, {539, 50}});
    }
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
        image(DungeonRun::portrait(hero), {{x + 10, 150}, {304, 224}}, true);
        label(DungeonRun::heroName(hero), {x + 18, 382}, 28, Gold, true);
        label(weapons[i], {x + 18, 417}, 17, Muted);
        paragraph(DungeonRun::ability(hero), {{x + 18, 444}, {288, 68}}, 16);
        const auto p = DungeonRun::makePlayer(hero);
        for (int s = 0; s < 6; ++s) {
            const float sy = 518.f + s * 32.f;
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

    // Unique Telegraphed Boss Intent banner at top of the battle scene.
    const auto intent = floor_.bossIntent();
    if (intent.active) {
        const sf::Color intentBorder = intent.telegraphed ? NegativeRed : Gold;
        const sf::Color intentBg = intent.telegraphed ? sf::Color{44, 14, 18, 232} : sf::Color{12, 20, 30, 225};
        panel({{40 + shakeX, 136 + shakeY}, {840, 48}}, intentBg, intentBorder);
        label((intent.telegraphed ? "BOSS INTENT [ACTIVE]: " : "BOSS INTENT: ") + intent.name,
              {56 + shakeX, 141 + shakeY}, 15, intent.telegraphed ? NegativeRed : Gold);
        label(intent.detail, {56 + shakeX, 162 + shakeY}, 14, Ink);
    }

    if (flash_ > 0) {
        const float alphaRatio = std::clamp(flash_ / std::max(0.05f, flashDuration_), 0.f, 1.f);
        sf::Color overlayColor = flashColor_;
        overlayColor.a = static_cast<std::uint8_t>(alphaRatio * 140.f);
        panel({{29 + shakeX, 127 + shakeY}, {862, 360}}, overlayColor, sf::Color::Transparent);
    }
    for (std::size_t bi = 0; bi < combatBadges_.size(); ++bi) {
        const auto& badge = combatBadges_[bi];
        const float progress = 1.f - (badge.timer / std::max(0.1f, badge.duration));
        const float bx = 420.f + badge.xOffset;
        const float by = 290.f - progress * 55.f + static_cast<float>(bi) * 34.f;
        panel({{bx - 12, by - 4}, {260, 34}}, {10, 16, 24, 225}, badge.color);
        label(badge.text, {bx, by}, 17, badge.color, false);
    }

    // Hero & stats panel (bottom-left) with stat hover/focus targets and green/red deltas.
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
    label(phase == Phase::Combat ? "TACTICAL COMBAT" : phase == Phase::Riddle ? "THE RIDDLE KEEPER" : "YOUR CHOICE", {350, 584}, 15, accent);
    if (phase == Phase::Room) {
        const auto choices = floor_.choices();
        const bool isBattlePreview = (floor_.room() == Room::Battle || floor_.room() == Room::Boss);
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const bool vertical = choices.size() == 3;
            const float x = vertical ? 350.f : 348 + (i % 2) * 270.f;
            const float y = vertical ? 622 + i * 69.f : (isBattlePreview ? 616.f : 632.f) + (i / 2) * 87.f;
            const float h = vertical ? 58.f : (isBattlePreview ? 62.f : 73.f);
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + choices[i], {{x, y}, {vertical ? 520.f : 254.f, h}});
        }
        if (isBattlePreview) {
            const auto summary = floor_.combatSummary();
            panel({{348, 692}, {524, 154}}, {13, 20, 29}, Border);
            label("COMBAT FORECAST", {362, 701}, 14, Gold);
            label("Enemy: " + floor_.enemyName() + "  |  HP " + num(summary.enemyMaxHealth) +
                  "  |  Counter " + num(summary.enemyCounterDamage, 2) + " (Guard " + num(summary.guardCounterDamage, 2) + ")",
                  {362, 723}, 15, Ink);
            label("Strike: " + num(summary.baseDamage, 1) + "  |  Crit: " + num(summary.critDamage, 1) +
                  " (" + num(summary.critChance * 100.f, 0) + "%)  |  Avg: " + num(summary.expectedDamage, 1),
                  {362, 748}, 15, PositiveGreen);
            label("Heavy (2x): " + num(summary.heavyDamage, 1) + "  |  Guard: " + num(summary.guardDamage, 1) +
                  "  |  Strikes: " + std::to_string(summary.minStrikes) + "-" + std::to_string(summary.maxStrikes),
                  {362, 773}, 15, Muted);
            label("Ability: " + DungeonRun::combatAbilityName(p.hero) + " (" + num(summary.abilityDamage, 1) + " base dmg)",
                  {362, 798}, 15, accent);
            if (floor_.room() == Room::Boss) {
                label("Boss fights feature unique telegraphed intents every 3rd turn.", {362, 821}, 14, Gold);
            }
        }
    } else if (phase == Phase::Riddle) {
        unsigned row = 0;
        for (std::size_t i = 0; i < 3; ++i) {
            if (!floor_.answerAvailable(i)) continue;
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + floor_.riddle().answers[i], {{350, 626 + row++ * 69.f}, {520, 58}});
        }
    } else if (phase == Phase::Combat) {
        label("Strikes left: " + std::to_string(floor_.strikesLeft()), {350, 604}, 19, Ink);
        label(floor_.enemyName() + "  " + num(floor_.enemyHealth(), 1) + " / " + num(floor_.enemyMaxHealth(), 1) +
              " HP  (Counter " + num(floor_.enemyDamage(), 2) + ")", {500, 607}, 15, Muted);
        bar(floor_.enemyHealth() / floor_.enemyMaxHealth(), {{350, 632}, {520, 8}}, accent);
        const auto combatChoices = floor_.choices();
        const auto mousePos = pointer(sf::Mouse::getPosition(window_));
        CombatAction previewedAction = CombatAction::StandardStrike;
        bool hoverMatched = false;
        for (std::size_t i = 0; i < combatChoices.size() && i < 4; ++i) {
            const float x = 348.f + (i % 2) * 266.f;
            const float y = 648.f + (i / 2) * 56.f;
            const sf::FloatRect rect{{x, y}, {254, 48}};
            const bool avail = attackDelay_ <= 0 && floor_.combatChoiceAvailable(i);
            if (mousePos && rect.contains(*mousePos)) {
                previewedAction = static_cast<CombatAction>(i);
                hoverMatched = true;
            } else if (!hoverMatched && buttons_.size() == selectedButton_) {
                previewedAction = static_cast<CombatAction>(i);
            }
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + combatChoices[i], rect, avail);
        }
        // Live Tactical Action Preview for the hovered or focused combat move.
        const auto preview = floor_.previewCombatAction(previewedAction);
        panel({{348, 762}, {520, 48}}, {13, 20, 29}, preview.available ? Gold : Border);
        label(preview.title + "  |  Dmg " + num(preview.minDamage, 1) + " (Crit " + num(preview.critDamage, 1) +
              ")  |  Counter " + num(preview.expectedCounter, 2),
              {360, 767}, 14, preview.available ? PositiveGreen : Muted);
        label(preview.effect, {360, 787}, 13, Ink);
        button("attack", "Quick Standard Strike [Enter / Space]", {{348, 816}, {520, 32}},
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
        if (p.riddleStreak >= 2) {
            label("Streak x" + std::to_string(p.riddleStreak) + (p.riddleStreak >= 5 ? " (+30%)" : p.riddleStreak >= 3 ? " (+15%)" : ""),
                  {1185, 206}, 17, PositiveGreen);
        }
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
    for (int i = 0; i < 4; ++i)
        button("lore-" + std::to_string(i), DungeonRun::heroName(static_cast<Hero>(i)),
               {{40 + i * 206.f, 102}, {188, 46}}, true, static_cast<Hero>(i) == loreHero_);

    // Framed hero portrait + structured starting stats & class ability card on the left.
    panel({{40, 174}, {450, 386}}, Surface, Border);
    image(DungeonRun::portrait(loreHero_), {{46, 180}, {438, 374}});

    panel({{40, 574}, {450, 224}}, {12, 20, 29, 245}, Border);
    const auto basePlayer = DungeonRun::makePlayer(loreHero_);
    const Stat primaryStat = loreHero_ == Hero::Warrior ? Strength :
                             loreHero_ == Hero::Ranger ? Accuracy :
                             loreHero_ == Hero::Mage ? Wisdom : Agility;
    const std::array<const char*, 4> weapons = {"Sword", "Longbow", "Wand", "Throwing Knives"};
    label(DungeonRun::heroName(loreHero_) + "  |  " + weapons[static_cast<std::size_t>(loreHero_)], {58, 588}, 20, Gold, true);
    const std::array<std::pair<const char*, Stat>, 6> statGrid = {{
        {"Strength", Strength}, {"Health", Health},
        {"Wisdom", Wisdom}, {"Agility", Agility},
        {"Accuracy", Accuracy}, {"Observation", Observation}
    }};
    for (std::size_t s = 0; s < statGrid.size(); ++s) {
        const float sx = 58.f + static_cast<float>(s % 2) * 212.f;
        const float sy = 626.f + static_cast<float>(s / 2) * 28.f;
        const bool isPrimary = statGrid[s].second == primaryStat;
        label(statGrid[s].first, {sx, sy}, 16, isPrimary ? Gold : Muted);
        label(std::to_string(static_cast<int>(basePlayer.stats[statGrid[s].second])), {sx + 132.f, sy}, 16, isPrimary ? Gold : Ink);
    }
    label("Growth: 1.5x " + DungeonRun::statName(primaryStat) + " & Health", {58, 720}, 15, PositiveGreen);
    label("Ability: " + DungeonRun::combatAbilityName(loreHero_) + " (1/battle)", {58, 746}, 15, Gold);
    label("Primary Damage: " + DungeonRun::statName(primaryStat), {58, 770}, 15, Ice);

    panel({{530, 174}, {870, 624}}, Surface, Border);
    reading(DungeonRun::readText(options_.assets / ("textfiles/charIntro/" + DungeonRun::heroName(loreHero_) + "Intro.txt")),
            {{558, 198}, {814, 576}});
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
    label("HIGH SCORES", {panelX + 32, 94}, 36, Gold, true);
    const bool tabsEnabled = readOnlyBrowse || !entryPending_;
    button("board-all", "All Diffs", {{panelX + 32, 148}, {162, 40}}, tabsEnabled, leaderboardAllDifficulties_);
    for (int mode = 0; mode < 4; ++mode)
        button("board-" + std::to_string(mode), DungeonRun::difficultyName(static_cast<Difficulty>(mode)),
               {{panelX + 32 + (mode + 1) * 172.f, 148}, {162, 40}}, tabsEnabled,
               !leaderboardAllDifficulties_ && static_cast<Difficulty>(mode) == leaderboardDifficulty_);

    struct BoardEntry {
        Difficulty difficulty = Difficulty::Easy;
        LeaderboardEntry entry;
    };
    std::vector<BoardEntry> rawEntries;
    if (leaderboardAllDifficulties_) {
        for (int mode = 0; mode < 4; ++mode) {
            const auto diff = static_cast<Difficulty>(mode);
            for (const auto& e : leaderboardTables_[static_cast<std::size_t>(mode)]) {
                rawEntries.push_back({diff, e});
            }
        }
        std::stable_sort(rawEntries.begin(), rawEntries.end(), [](const BoardEntry& a, const BoardEntry& b) {
            if (a.entry.score != b.entry.score) return a.entry.score > b.entry.score;
            if (a.entry.playTimeSeconds != b.entry.playTimeSeconds) return a.entry.playTimeSeconds < b.entry.playTimeSeconds;
            return a.entry.id < b.entry.id;
        });
    } else {
        const auto& table = leaderboardTables_[static_cast<std::size_t>(leaderboardDifficulty_)];
        rawEntries.reserve(table.size());
        for (const auto& e : table) rawEntries.push_back({leaderboardDifficulty_, e});
    }

    if (!readOnlyBrowse && entryPending_) {
        label("ENTER THREE LETTERS", {panelX + 32, 206}, 16, Gold);
        for (std::size_t i = 0; i < 3; ++i) {
            const float x = panelX + 32 + static_cast<float>(i) * 82;
            panel({{x, 234}, {62, 54}}, Surface, i == initials_.size() ? Gold : Border);
            label(i < initials_.size() ? std::string(1, initials_[i]) : "_", {x + 18, 240}, 32, Ink, true);
        }
        button("save-score", "Save score [Enter]", {{panelX + 318, 234}, {250, 54}}, Leaderboards::validInitials(initials_));
        button("skip-score", "Skip entry [Esc]", {{panelX + 592, 234}, {288, 54}});
        if (leaderboardStatus_.empty()) label("A-Z only. Backspace to edit.", {panelX + 32, 300}, 16, Muted);
        else paragraph(leaderboardStatus_, {{panelX + 32, 294}, {848, 40}}, 17);
    } else {
        // Hero filter pills row.
        const std::array<std::string, 5> filterNames = {"All Heroes", "Warrior", "Ranger", "Mage", "Rogue"};
        for (int f = -1; f <= 3; ++f) {
            const int idx = f + 1;
            button("filter-hero-" + std::to_string(f), filterNames[static_cast<std::size_t>(idx)],
                   {{panelX + 32 + idx * 172.f, 198}, {162, 30}}, true, leaderboardHeroFilter_ == f);
        }
        // Ending filter pills row.
        const std::array<std::string, 5> endingNames = {"All Endings", "Good", "True", "Bad", "Perfect"};
        for (int e = 0; e <= 4; ++e) {
            button("filter-ending-" + std::to_string(e), endingNames[static_cast<std::size_t>(e)],
                   {{panelX + 32 + e * 172.f, 236}, {162, 30}}, true, leaderboardEndingFilter_ == e);
        }
        label(drawBackground ? "LOCAL DIFFICULTY LEADERBOARDS" : "THIS DESCENT", {panelX + 32, 276}, 14, Gold);
        const std::string boardLabel = leaderboardAllDifficulties_ ? "All Difficulties" : DungeonRun::difficultyName(leaderboardDifficulty_);
        const std::string statusText = leaderboardStatus_.empty()
            ? ("Viewing " + std::to_string(rawEntries.size()) + " recorded descent" +
               (rawEntries.size() == 1 ? "" : "s") + " for " + boardLabel + ".")
            : leaderboardStatus_;
        paragraph(statusText, {{panelX + 32, 296}, {848, 34}}, 17);
    }

    // Build filtered list while preserving each entry's true board rank (1-based).
    std::vector<std::pair<std::size_t, BoardEntry>> filtered;
    filtered.reserve(rawEntries.size());
    for (std::size_t i = 0; i < rawEntries.size(); ++i) {
        const bool heroMatch = leaderboardHeroFilter_ < 0 || static_cast<int>(rawEntries[i].entry.hero) == leaderboardHeroFilter_;
        const bool endingMatch = leaderboardEndingFilter_ == 0 || static_cast<int>(rawEntries[i].entry.ending) == leaderboardEndingFilter_;
        if (heroMatch && endingMatch) {
            filtered.emplace_back(i + 1, rawEntries[i]);
        }
    }

    if (leaderboardAllDifficulties_) {
        label("RANK", {panelX + 36, 340}, 15, Muted);
        label("NAME", {panelX + 96, 340}, 15, Muted);
        label("SCORE", {panelX + 194, 340}, 15, Muted);
        label("DIFF", {panelX + 286, 340}, 15, Muted);
        label("HERO", {panelX + 398, 340}, 15, Muted);
        label("ENDING", {panelX + 494, 340}, 15, Muted);
        label("SEED", {panelX + 592, 340}, 15, Muted);
    } else {
        label("RANK", {panelX + 36, 340}, 15, Muted);
        label("NAME", {panelX + 104, 340}, 15, Muted);
        label("SCORE", {panelX + 216, 340}, 15, Muted);
        label("HERO", {panelX + 324, 340}, 15, Muted);
        label("ENDING", {panelX + 452, 340}, 15, Muted);
        label("SEED", {panelX + 584, 340}, 15, Muted);
    }
    if (readOnlyBrowse || !entryPending_) label("REPLAY", {panelX + 754, 340}, 15, Muted);

    leaderboardPageCount_ = std::max(1, static_cast<int>((filtered.size() + Leaderboards::PageSize - 1) / Leaderboards::PageSize));
    leaderboardPage_ = std::clamp(leaderboardPage_, 0, leaderboardPageCount_ - 1);
    const std::size_t pageStart = static_cast<std::size_t>(leaderboardPage_) * Leaderboards::PageSize;
    for (std::size_t i = 0; i < Leaderboards::PageSize; ++i) {
        const std::size_t idx = pageStart + i;
        const float y = 371 + static_cast<float>(i) * 38;
        const bool filled = idx < filtered.size();
        const bool own = !drawBackground && filled && leaderboardEntryId_ == filtered[idx].second.entry.id &&
                         (leaderboardAllDifficulties_ || leaderboardDifficulty_ == floor_.difficulty());
        panel({{panelX + 28, y - 2}, {856, 36}}, own ? sf::Color(61, 54, 37) : Surface, own ? Gold : sf::Color::Transparent);
        const std::size_t shownRank = filled ? filtered[idx].first : (pageStart + i + 1);
        const float nameX = leaderboardAllDifficulties_ ? panelX + 96 : panelX + 104;
        const float scoreX = leaderboardAllDifficulties_ ? panelX + 194 : panelX + 216;
        label(std::to_string(shownRank), {panelX + 42, y}, 20, own ? Gold : Muted);
        label(filled ? filtered[idx].second.entry.initials : "---", {nameX, y}, 20, own ? Gold : Ink, true);
        label(filled ? std::to_string(filtered[idx].second.entry.score) : "--", {scoreX, y}, 20, own ? Gold : Ink);
        if (filled) {
            const auto& row = filtered[idx].second;
            const auto& entry = row.entry;
            const auto ending = entry.ending;
            const std::string endText = ending == Ending::Perfect ? "PERFECT" : ending == Ending::True ? "TRUE" : ending == Ending::Good ? "GOOD" : "BAD";
            if (leaderboardAllDifficulties_) {
                label(DungeonRun::difficultyName(row.difficulty), {panelX + 286, y + 3}, 16, Ink);
                label(DungeonRun::heroName(entry.hero), {panelX + 398, y + 3}, 16, Ink);
                label(endText, {panelX + 494, y + 3}, 16, ending == Ending::Perfect ? Gold : Muted);
                label(entry.seed != 0 ? ("#" + std::to_string(entry.seed)) : "-", {panelX + 592, y + 3}, 16, Muted);
            } else {
                label(DungeonRun::heroName(entry.hero), {panelX + 324, y + 2}, 18, Ink);
                label(endText, {panelX + 452, y + 2}, 17, ending == Ending::Perfect ? Gold : Muted);
                label(entry.seed != 0 ? ("#" + std::to_string(entry.seed)) : "-", {panelX + 584, y + 3}, 16, Muted);
            }
            if ((readOnlyBrowse || !entryPending_) && entry.seed != 0 && !floor_.inProgress()) {
                button("replay-" + std::to_string(entry.id), "Replay seed", {{panelX + 746, y + 1}, {130, 30}});
            }
        }
    }
    if (drawBackground) {
        button("leaderboard-back", "Back to menu [Esc]", {{panelX + 32, 766}, {240, 46}});
        label("Showing " + std::to_string(filtered.size()) + " record" + (filtered.size() == 1 ? "" : "s") +
              ". Ties keep earlier run.", {panelX + 286, 777}, 16, Muted);
    } else {
        label("Showing " + std::to_string(filtered.size()) + " record" + (filtered.size() == 1 ? "" : "s") +
              ". Equal scores keep earlier entry.", {panelX + 32, 777}, 16, Muted);
    }
    button("board-prev", "<", {{panelX + 676, 766}, {48, 46}}, leaderboardPage_ > 0);
    label("Page " + std::to_string(leaderboardPage_ + 1) + " / " + std::to_string(leaderboardPageCount_), {panelX + 734, 777}, 16, Ink);
    button("board-next", ">", {{panelX + 836, 766}, {48, 46}}, leaderboardPage_ + 1 < leaderboardPageCount_);
}

void GameApp::drawCodex() {
    image("images/startscreen.jpg", {{0, 0}, {Width, Height}}, true, {55, 65, 78});
    panel({{0, 0}, {Width, Height}}, {6, 11, 18, 195}, sf::Color::Transparent);
    label("CHRONICLES OF AETHORIA", {40, 26}, 38, Gold, true);
    button("codex-tab-0", "Career & Milestones", {{40, 92}, {260, 44}}, true, codexTab_ == 0);
    button("codex-tab-1", "Ending Gallery (" + std::to_string(codex_.unlockedEndingCount()) + " / 4)", {{318, 92}, {280, 44}}, true, codexTab_ == 1);

    if (codexTab_ == 0) {
        panel({{40, 156}, {520, 636}}, {12, 20, 29, 245}, Border);
        label("CAREER SUMMARY", {68, 180}, 20, Gold);
        const int accuracy = codex_.totalRiddlesAnswered > 0
            ? static_cast<int>(std::round(100.0 * codex_.totalRiddlesCorrect / codex_.totalRiddlesAnswered)) : 0;
        const std::array<std::pair<std::string, std::string>, 8> careerRows = {{
            {"Completed runs", std::to_string(codex_.totalRuns)},
            {"Victories vs Malvel", std::to_string(codex_.totalVictories)},
            {"Fallen descents", std::to_string(codex_.totalDefeats)},
            {"Impossible clears", std::to_string(codex_.impossibleVictories)},
            {"Riddle accuracy", std::to_string(codex_.totalRiddlesCorrect) + " / " + std::to_string(codex_.totalRiddlesAnswered) + " (" + std::to_string(accuracy) + "%)"},
            {"Best riddle streak", std::to_string(codex_.bestRiddleStreak)},
            {"Fastest victory", codex_.fastestVictorySeconds > 0 ? formatDuration(codex_.fastestVictorySeconds) : "--:--:--"},
            {"Best Gauntlet run", std::to_string(codex_.bestGauntletScore) + " pts (Streak " + std::to_string(codex_.bestGauntletStreak) + ")"}
        }};
        for (std::size_t i = 0; i < careerRows.size(); ++i) {
            const float y = 216.f + static_cast<float>(i) * 38.f;
            label(careerRows[i].first, {68, y}, 17, Muted);
            label(careerRows[i].second, {310, y}, 18, i == 1 ? PositiveGreen : Ink);
        }
        label("HERO RECORDS", {68, 540}, 18, Gold);
        for (int h = 0; h < 4; ++h) {
            const float y = 576.f + h * 48.f;
            panel({{64, y - 4}, {472, 40}}, Surface, Border);
            label(DungeonRun::heroName(static_cast<Hero>(h)), {80, y + 4}, 18, Gold);
            label("Wins: " + std::to_string(codex_.heroVictories[static_cast<std::size_t>(h)]), {230, y + 5}, 17, Ink);
            label("Best Score: " + std::to_string(codex_.heroBestScore[static_cast<std::size_t>(h)]), {355, y + 5}, 17, PositiveGreen);
        }

        panel({{584, 156}, {816, 636}}, {12, 20, 29, 245}, Border);
        const auto milestones = codex_.milestones();
        int unlockedCount = 0;
        for (const auto& m : milestones) if (m.unlocked) ++unlockedCount;
        label("MILESTONES (" + std::to_string(unlockedCount) + " / " + std::to_string(milestones.size()) + ")", {612, 178}, 20, Gold);

        constexpr std::size_t PerPage = 8;
        const int totalPages = std::max(1, static_cast<int>((milestones.size() + PerPage - 1) / PerPage));
        codexMilestonePage_ = std::clamp(codexMilestonePage_, 0, totalPages - 1);
        button("codex-ms-prev", "<", {{1168, 170}, {40, 34}}, codexMilestonePage_ > 0);
        label("Page " + std::to_string(codexMilestonePage_ + 1) + " / " + std::to_string(totalPages), {1218, 178}, 16, Ink);
        button("codex-ms-next", ">", {{1332, 170}, {40, 34}}, codexMilestonePage_ + 1 < totalPages);

        const std::size_t startIdx = static_cast<std::size_t>(codexMilestonePage_) * PerPage;
        for (std::size_t slot = 0; slot < PerPage && startIdx + slot < milestones.size(); ++slot) {
            const float mx = 612.f + static_cast<float>(slot % 2) * 388.f;
            const float my = 220.f + static_cast<float>(slot / 2) * 138.f;
            const auto& m = milestones[startIdx + slot];
            panel({{mx, my}, {372, 124}}, m.unlocked ? sf::Color{30, 38, 30} : Surface, m.unlocked ? Gold : Border);
            label(m.unlocked ? "UNLOCKED" : "LOCKED", {mx + 16, my + 12}, 13, m.unlocked ? PositiveGreen : Muted);
            label(m.title, {mx + 16, my + 32}, 18, m.unlocked ? Gold : Ink);
            paragraph(m.description, {{mx + 16, my + 62}, {340, 52}}, 15);
        }
    } else {
        const std::array<Ending, 4> allEndings = {Ending::Good, Ending::True, Ending::Bad, Ending::Perfect};
        for (int i = 0; i < 4; ++i) {
            const auto e = allEndings[static_cast<std::size_t>(i)];
            const bool unlocked = codex_.endingUnlocked(e);
            const std::string tag = unlocked ? DungeonRun::endingName(e) : ("Locked Ending #" + std::to_string(i + 1));
            button("codex-ending-" + std::to_string(static_cast<int>(e)), tag,
                   {{40 + i * 344.f, 154}, {328, 44}}, true, codexEnding_ == e);
        }
        if (codex_.endingUnlocked(codexEnding_)) {
            panel({{40, 214}, {660, 580}}, Surface, Border);
            image(DungeonRun::endingImage(codexEnding_), {{41, 215}, {658, 578}}, true, {55, 65, 75});
            image(DungeonRun::endingImage(codexEnding_), {{41, 215}, {658, 578}});
            panel({{724, 214}, {676, 580}}, Surface, Border);
            label(DungeonRun::endingName(codexEnding_), {752, 236}, 28, Gold, true);
            reading(DungeonRun::readText(options_.assets / DungeonRun::endingStoryFile(codexEnding_)), {{752, 290}, {620, 486}});
        } else {
            panel({{260, 270}, {920, 380}}, {12, 20, 29, 245}, Border);
            label("ENDING SEALED", {310, 326}, 34, Gold, true);
            paragraph("You have not yet unlocked this ending in your descents beneath Aethoria.", {{310, 396}, {820, 60}}, 22);
            label("HINT", {310, 476}, 16, Gold);
            paragraph(endingHint(codexEnding_), {{310, 504}, {820, 90}}, 20);
        }
    }
    button("menu", "Back to menu [Esc]", {{40, 814}, {240, 48}});
}

void GameApp::drawGauntlet() {
    image("images/startscreen.jpg", {{0, 0}, {Width, Height}}, true, {55, 65, 80});
    panel({{0, 0}, {Width, Height}}, {6, 11, 18, 185}, sf::Color::Transparent);
    label("RIDDLE GAUNTLET", {48, 28}, 40, Gold, true);
    const std::string tierLabel = gauntlet_.currentTier == 0 ? "EASY" : (gauntlet_.currentTier == 1 ? "MEDIUM" : "HARD");
    label("Score: " + std::to_string(gauntlet_.score) + "   |   Streak: " + std::to_string(gauntlet_.streak) +
          "   |   Best: " + std::to_string(std::max(codex_.bestGauntletScore, gauntlet_.score)) +
          "   |   Lives: " + std::to_string(gauntlet_.lives) + " / 3   |   Tier: " + tierLabel,
          {50, 88}, 20, Ink);

    panel({{180, 142}, {1080, 646}}, {12, 20, 29, 245}, Border);
    if (gauntlet_.finished) {
        label(gauntlet_.lives > 0 ? "GAUNTLET CONQUERED!" : "GAUNTLET ENDED", {230, 194}, 38, Gold, true);
        if (!gauntlet_.feedback.empty()) {
            paragraph(gauntlet_.feedback, {{230, 260}, {980, 74}}, 20);
        }
        const std::array<std::pair<std::string, std::string>, 4> rows = {{
            {"Riddles solved", std::to_string(gauntlet_.score) + " / " + std::to_string(gauntlet_.answered)},
            {"Best streak this session", std::to_string(gauntlet_.bestStreak)},
            {"All-time best score", std::to_string(codex_.bestGauntletScore)},
            {"All-time best streak", std::to_string(codex_.bestGauntletStreak)}
        }};
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const float y = 360.f + static_cast<float>(i) * 52.f;
            label(rows[i].first, {230, y}, 22, Muted);
            label(rows[i].second, {680, y}, 24, i == 0 ? Gold : Ink);
        }
        button("gauntlet-retry", "Play again", {{230, 668}, {460, 62}});
        button("menu", "Main menu [Esc]", {{720, 668}, {460, 62}});
    } else if (gauntlet_.showingFeedback) {
        label(gauntlet_.lastCorrect ? "CORRECT!" : "WRONG ANSWER", {230, 198}, 34,
              gauntlet_.lastCorrect ? PositiveGreen : NegativeRed, true);
        paragraph(gauntlet_.feedback, {{230, 274}, {980, 180}}, 23);
        label("Remaining lives: " + std::to_string(gauntlet_.lives) + " / 3", {230, 490}, 20, Muted);
        button("gauntlet-next", "Next riddle [Enter]", {{230, 668}, {520, 62}});
        button("menu", "Leave Gauntlet [Esc]", {{780, 668}, {400, 62}});
    } else if (gauntlet_.index < gauntlet_.deck.size()) {
        const auto& r = gauntlet_.deck[gauntlet_.index].second;
        const int secsLeft = static_cast<int>(std::ceil(gauntlet_.timeLeft));
        label("RIDDLE #" + std::to_string(gauntlet_.index + 1) + " / " + std::to_string(gauntlet_.deck.size()) + "  (" + tierLabel + ")",
              {230, 174}, 17, Gold);
        label(std::to_string(secsLeft) + " seconds", {1060, 170}, 24, secsLeft <= 5 ? NegativeRed : Gold);
        bar(gauntlet_.timeLeft / std::max(1.f, gauntlet_.timeLimit), {{230, 214}, {980, 10}}, secsLeft <= 5 ? NegativeRed : Ice);
        paragraph(r.question, {{230, 254}, {980, 190}}, 24);
        for (std::size_t i = 0; i < 3; ++i) {
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + r.answers[i],
                   {{230, 474.f + static_cast<float>(i) * 76.f}, {980, 60}});
        }
    }
    if (!gauntlet_.finished) {
        button("pause", "Pause [Esc]", {{996, 28}, {168, 46}});
    }
    button("sound", muted_ ? "Sound off [V]" : "Sound on [V]", {{1180, 28}, {220, 46}});
}

void GameApp::drawResults() {
    const auto& p = floor_.player();
    const int playSeconds = static_cast<int>(playTime_);
    const auto bd = floor_.scoreBreakdown(playSeconds);
    image(floor_.image(), {{0, 0}, {Width, Height}}, true, {70, 80, 90});
    std::string seedText = "#" + std::to_string(floor_.seed());
    if (activeDailyRun_ && !activeDailyDate_.empty()) seedText += " (" + activeDailyDate_ + ")";
    const std::array<std::pair<std::string, std::string>, 8> results = {{
        {"Run seed", seedText},
        {"Floor reached", std::to_string(floor_.floorNumber()) + " / 3"},
        {"Riddles solved", std::to_string(p.riddlesCorrect) + " / " + std::to_string(p.riddlesAnswered)},
        {"Battles won", std::to_string(p.battlesWon) + " / " + std::to_string(p.battlesFought)},
        {"Strikes leftover", std::to_string(p.strikesLeftover)},
        {"Reputation", std::to_string(p.reputation) + " / 9"},
        {"Health remaining", num(p.stats[Health])},
        {"Playtime", formatDuration(playSeconds)}}};
    drawLeaderboard(!hasEndingResult(), false);
    panel({{36, 70}, {424, 760}}, {10, 17, 25, 245}, Border);
    label("YOUR JOURNEY", {64, 102}, 32, Gold, true);
    paragraph(floor_.endingName(), {{64, 152}, {368, 52}}, 25);
    label(DungeonRun::heroName(p.hero) + " / " + DungeonRun::difficultyName(floor_.difficulty()), {64, 212}, 19, Muted);
    label("Final score", {64, 250}, 20, Muted);
    label(std::to_string(bd.total), {260, 247}, 24, Gold, true);
    panel({{64, 288}, {368, 66}}, {16, 26, 38, 225}, Border);
    label("Stats +" + std::to_string(bd.statsPoints), {78, 297}, 15, Ink);
    label("Health +" + std::to_string(bd.healthPoints), {254, 297}, 15, Ink);
    label("Strikes +" + std::to_string(bd.strikesPoints), {78, 324}, 15, Ink);
    label("Time +" + std::to_string(bd.timePoints), {254, 324}, 15, bd.timePoints > 0 ? Ink : Muted);
    for (std::size_t i = 0; i < results.size(); ++i) {
        const float y = 374.f + static_cast<float>(i) * 43.f;
        label(results[i].first, {64, y}, 18, Muted);
        label(results[i].second, {260, y}, 19, Ink);
    }
    button("menu", "Main menu", {{64, 742}, {172, 52}}, !hasEndingResult() || !entryPending_);
    button("quit", "Quit game", {{252, 742}, {180, 52}});
}

void GameApp::drawOverlay() {
    panel({{0, 0}, {Width, Height}}, {0, 0, 0, 205}, sf::Color::Transparent);
    buttons_.clear();
    if (overlay_ == Overlay::Map) {
        panel({{235, 45}, {970, 809}}, Background, Border);
        label("FLOOR " + std::to_string(floor_.floorNumber()) + " / YOUR ROUTE", {270, 70}, 32, Gold, true);
        image(floor_.mapImage(), {{270, 130}, {900, 420}});
        label("The first three rooms change order each descent.", {270, 570}, 20, Muted);
        for (int i = 0; i < 9; ++i) {
            const float x = 270 + (i % 3) * 300.f, y = 614 + (i / 3) * 47.f;
            const std::string prefix = i + 1 < floor_.roomNumber() ? "DONE  " : i + 1 == floor_.roomNumber() ? "HERE  " : "      ";
            label(prefix + std::to_string(i + 1) + "  " + floor_.roomTitle(floor_.route()[i]), {x, y}, 18, i + 1 == floor_.roomNumber() ? Gold : Ink);
        }
        button("resume", "Return to the chamber [M / Esc]", {{270, 778}, {900, 50}});
    } else if (overlay_ == Overlay::Settings) {
        panel({{360, 135}, {720, 620}}, Background, Border);
        label("SETTINGS", {402, 168}, 34, Gold, true);
        label("Audio and display preferences are saved automatically.", {404, 218}, 17, Muted);

        struct VolRow { const char* title; const char* downId; const char* upId; int value; sf::Color color; };
        const std::array<VolRow, 3> rows = {{
            {"Music volume", "vol-music-down", "vol-music-up", settings_.musicVolume, Ice},
            {"Sound effects volume", "vol-sfx-down", "vol-sfx-up", settings_.sfxVolume, PositiveGreen},
            {"Intro narration volume", "vol-narr-down", "vol-narr-up", settings_.narrationVolume, Gold}
        }};
        for (std::size_t i = 0; i < rows.size(); ++i) {
            const float y = 272.f + static_cast<float>(i) * 84.f;
            label(rows[i].title, {404, y}, 19, Ink);
            label(std::to_string(rows[i].value) + "%", {980, y}, 19, Gold);
            button(rows[i].downId, "-", {{404, y + 30}, {52, 36}}, rows[i].value > 0);
            bar(static_cast<float>(rows[i].value) / 100.f, {{472, y + 42}, {480, 12}}, rows[i].color);
            button(rows[i].upId, "+", {{968, y + 30}, {52, 36}}, rows[i].value < 100);
        }
        button("sound", muted_ ? "Audio: Muted [V]" : "Audio: Enabled [V]", {{404, 540}, {296, 52}});
        button("toggle-fullscreen", fullscreen_ ? "Display: Fullscreen [F11]" : "Display: Windowed [F11]", {{724, 540}, {296, 52}});
        button("settings-close", "Done [Esc]", {{404, 624}, {616, 54}});
    } else if (overlay_ == Overlay::Pause && screen_ == Screen::Gauntlet && !gauntlet_.finished) {
        panel({{430, 155}, {580, 560}}, Background, Border);
        label("GAUNTLET PAUSED", {468, 184}, 33, Gold, true);
        label("Riddle #" + std::to_string(std::min(gauntlet_.index + 1, gauntlet_.deck.size())) + " / " +
              std::to_string(gauntlet_.deck.size()) + "   |   Score: " + std::to_string(gauntlet_.score) +
              "   |   Lives: " + std::to_string(gauntlet_.lives) + " / 3",
              {468, 236}, 17, Gold);
        paragraph("Your riddle timer is paused. Save your Gauntlet run to come back to later, or discard it and return to the main menu.",
                  {{468, 272}, {504, 72}}, 18);
        button("resume", "Resume Gauntlet [Esc]", {{468, 360}, {504, 52}});
        button("settings", "Audio & Display Settings", {{468, 424}, {504, 52}});
        button("gauntlet-save", "Save & exit to menu (Resume later)", {{468, 488}, {504, 52}});
        button("gauntlet-discard", "Discard run & exit to menu", {{468, 552}, {504, 52}});
    } else {
        const bool pauseWithOptions = overlay_ == Overlay::Pause && floor_.canSuspend() && !timedRiddle();
        panel({{430, pauseWithOptions ? 135.f : 175.f}, {580, pauseWithOptions ? 615.f : 550.f}}, Background, Border);
        const float topY = pauseWithOptions ? 162.f : 206.f;
        label(overlay_ == Overlay::Quit ? "LEAVE THE DUNGEON?" : "PAUSED", {468, topY}, 33, Gold, true);
        std::string seedInfo = "Run Seed: #" + std::to_string(floor_.seed());
        if (activeDailyRun_ && !activeDailyDate_.empty()) seedInfo += "  (Daily Run " + activeDailyDate_ + ")";
        label(seedInfo, {468, topY + 50.f}, 18, Gold);
        paragraph(overlay_ == Overlay::Quit
            ? (screen_ == Screen::Results && entryPending_ ? "Your score has not been saved. Quit without entering your initials?" :
               timedRiddle() ? "The riddle timer is still running. Quit the game and end this descent?" : "Quit the game? Your unsaved progress will end unless you Save & Quit from Pause.")
            : "Pause is available between riddles. You can suspend your run to resume later, or return to the main menu without saving.",
            {{468, topY + 86.f}, {504, 78}}, 18);
        if (pauseWithOptions) {
            button("resume", "Resume descent", {{468, 344}, {504, 50}});
            button("settings", "Audio & Display Settings", {{468, 404}, {504, 50}});
            button("suspend-run", "Save & quit to menu (Suspend run)", {{468, 464}, {504, 50}});
            button("abandon-to-menu", "Return to main menu (Discard run)", {{468, 524}, {504, 50}});
            button("quit", "Abandon & quit game", {{468, 584}, {504, 50}});
        } else {
            button("resume", overlay_ == Overlay::Quit ? "Keep playing" : "Resume descent", {{468, 392}, {504, 52}});
            button(overlay_ == Overlay::Quit ? "quit-yes" : "quit", "Quit game", {{468, 468}, {504, 56}});
        }
    }
}

void GameApp::drawPrequel() {
    const auto& ch = prequel_.character();
    const auto phase = prequel_.phase();
    const bool inCombat = (phase == PrequelPhase::Combat);
    const bool inParley = (phase == PrequelPhase::Parley);

    // Subtle atmospheric tinted background
    image(prequel_.sceneImage(), {{0, 0}, {Width, Height}}, true, {36, 42, 52});
    panel({{0, 0}, {Width, Height}}, {6, 10, 16, 224}, sf::Color::Transparent);

    // Top Header Bar
    label("ASHES OF FALKREATH", {36, 18}, 30, Gold, true);
    label(PrequelRun::actTitle(prequel_.act()) + "  /  Chapter " + std::to_string(prequel_.actChapterNumber()) + " of 5",
          {38, 56}, 16, sf::Color{175, 198, 214});

    // 5-Chapter Progress Pips for Current Act
    for (int i = 1; i <= 5; ++i) {
        const float px = 620.f + (i - 1) * 40.f;
        const bool current = (i == prequel_.actChapterNumber() && phase != PrequelPhase::Epilogue);
        const bool done = (i < prequel_.actChapterNumber() || phase == PrequelPhase::Epilogue || phase == PrequelPhase::ActComplete);
        panel({{px, 28}, {32, 32}},
              current ? sf::Color{58, 46, 26} : (done ? sf::Color{38, 72, 68} : Surface),
              current ? Gold : (done ? sf::Color{92, 156, 146} : Border));
        label(std::to_string(i), {px + 11.f, 34.f}, 16, current ? Gold : Ink);
    }

    // Act Indicator Pills (Act I / II / III)
    for (int a = 0; a < 3; ++a) {
        const float ax = 840.f + a * 86.f;
        const bool curAct = (a == static_cast<int>(prequel_.act()));
        const bool doneAct = (a < static_cast<int>(prequel_.act()) || phase == PrequelPhase::Epilogue);
        panel({{ax, 28}, {78, 32}},
              curAct ? sf::Color{58, 46, 26} : (doneAct ? sf::Color{34, 58, 54} : Surface),
              curAct ? Gold : Border);
        label("ACT " + std::string(a == 0 ? "I" : a == 1 ? "II" : "III"),
              {ax + 16.f, 35.f}, 15, curAct ? Gold : Muted);
    }

    button("sound", muted_ ? "Sound off [V]" : "Sound on [V]", {{1124, 24}, {136, 40}});
    button("menu", "Menu [Esc]", {{1272, 24}, {132, 40}}, !inParley);

    // =========================================================================
    // LEFT COLUMN: Native 1:1 Square Artwork Card + Enemy Status / Threads
    // =========================================================================
    panel({{36, 92}, {524, 774}}, Surface, Border);
    label("CHRONICLE SCENE", {52, 102}, 13, Gold);
    label("CHAPTER " + std::to_string(prequel_.chapterNumber()) + " / 15", {442, 102}, 13, Muted);

    float shakeX = 0.f, shakeY = 0.f;
    if (shakeTime_ > 0.f && shakeAmplitude_ > 0.f) {
        shakeX = std::sin(animationTime_ * 72.f) * shakeAmplitude_;
        shakeY = std::cos(animationTime_ * 64.f) * (shakeAmplitude_ * 0.6f);
    }
    const sf::FloatRect artRect{{48.f + shakeX, 126.f + shakeY}, {500.f, 500.f}};
    panel({{48, 126}, {500, 500}}, {8, 12, 18}, Border);
    image(prequel_.sceneImage(), artRect, false);

    if ((inCombat || phase == PrequelPhase::Outcome) && flash_ > 0.f && flashDuration_ > 0.f) {
        const auto alpha = static_cast<std::uint8_t>(std::clamp(flash_ / flashDuration_, 0.f, 1.f) * 110.f);
        panel({{48, 126}, {500, 500}}, {flashColor_.r, flashColor_.g, flashColor_.b, alpha}, sf::Color::Transparent);
    }
    if (inCombat || phase == PrequelPhase::Outcome) {
        for (const auto& badge : combatBadges_) {
            const float progress = 1.f - (badge.timer / std::max(0.01f, badge.duration));
            const float bx = 220.f + badge.xOffset;
            const float by = 370.f - progress * 64.f;
            const auto alpha = static_cast<std::uint8_t>(std::clamp((1.f - progress * progress) * 255.f, 0.f, 255.f));
            panel({{bx - 12.f, by - 4.f}, {214.f, 34.f}}, {10, 15, 22, static_cast<std::uint8_t>(alpha * 0.85f)}, sf::Color::Transparent);
            label(badge.text, {bx, by}, 19, {badge.color.r, badge.color.g, badge.color.b, alpha}, true);
        }
    }

    // Below-artwork status box (ZERO overlap with enemy/character faces!)
    if (inCombat) {
        const auto& en = prequel_.enemy();
        panel({{48, 638}, {500, 88}}, {11, 17, 25}, Border);
        label(en.name + "  (" + num(en.hp) + " / " + num(en.maxHp) + " HP)", {62, 648}, 18, Ink);
        label("Strikes left: " + std::to_string(en.strikesLeft), {416, 650}, 16, Gold);
        bar(en.maxHp > 0.f ? en.hp / en.maxHp : 0.f, {{62, 682}, {472, 16}}, sf::Color{224, 96, 92});
        label(en.title, {62, 704}, 13, Muted);

        const bool tele = en.intent.telegraphed;
        panel({{48, 738}, {500, 114}},
              tele ? sf::Color{48, 22, 18} : sf::Color{14, 21, 31},
              tele ? sf::Color{234, 108, 108} : Gold);
        label(en.intent.name, {62, 748}, 17, tele ? sf::Color{255, 184, 172} : Gold);
        paragraph(en.intent.detail, {{62, 778}, {472, 64}}, 15);
    } else {
        panel({{48, 638}, {500, 214}}, {11, 17, 25}, Border);
        label("CHRONICLE THREADS", {62, 650}, 15, Gold);
        label("Choices echo across all three Acts and shape the Prelude Coda.", {62, 674}, 14, Muted);

        label("Falkreath's Legacy", {62, 708}, 15, Ink);
        bar(std::min(1.f, prequel_.legacyThread() / 6.f), {{226, 713}, {250, 12}}, sf::Color{119, 198, 147});
        label(std::to_string(prequel_.legacyThread()), {490, 707}, 15, Ink);

        label("Obsidian Fury", {62, 740}, 15, Ink);
        bar(std::min(1.f, prequel_.furyThread() / 6.f), {{226, 745}, {250, 12}}, sf::Color{234, 114, 92});
        label(std::to_string(prequel_.furyThread()), {490, 739}, 15, Ink);

        label("Sophie's Remorse", {62, 772}, 15, Ink);
        bar(std::min(1.f, prequel_.remorseThread() / 6.f), {{226, 777}, {250, 12}}, sf::Color{146, 178, 238});
        label(std::to_string(prequel_.remorseThread()), {490, 771}, 15, Ink);

        const bool ironOk = prequel_.ironChronicleEligible();
        label(ironOk ? "Iron Chronicle (0 Abilities): ACTIVE" : "Signature Abilities Used: " + std::to_string(prequel_.abilitiesUsed()),
              {62, 814}, 15, ironOk ? Gold : Muted);
    }

    // =========================================================================
    // RIGHT COLUMN: Compact POV Strip + Narrative/Parley Card + Action Deck
    // =========================================================================
    panel({{580, 92}, {824, 114}}, Surface, Border);
    panel({{592, 102}, {94, 94}}, {9, 14, 21}, Gold);
    image(ch.portrait, {{594, 104}, {90, 90}}, false);

    label(ch.name + "  —  " + ch.title, {702, 102}, 21, Gold, true);
    label("Weapon: " + ch.weapon + "   |   Ability: " + ch.abilityName, {702, 130}, 15, Muted);

    label("HP " + num(ch.hp) + " / " + num(ch.maxHp), {702, 158}, 16, Ink);
    bar(ch.maxHp > 0.f ? ch.hp / ch.maxHp : 0.f, {{842, 163}, {180, 13}}, sf::Color{112, 214, 144});

    label("PWR " + num(ch.power) + "   RSV " + num(ch.resolve) + "   CUN " + num(ch.cunning) + "   AGI " + num(ch.agility),
          {1044, 158}, 15, sf::Color{216, 224, 230});
    label(ch.abilityDesc, {702, 182}, 13, Muted);

    // Narrative / Parley / Combat Log Card
    panel({{580, 220}, {824, 344}}, Surface, Border);
    label(prequel_.heading(), {600, 234}, 24, Gold, true);
    label(prequel_.subheading(), {600, 266}, 16, Muted);

    if (inParley) {
        const auto& p = prequel_.parley();
        const float ratio = p.timeLimit > 0.f ? p.timeLeft / p.timeLimit : 0.f;
        const bool low = p.timeLeft <= 5.f;
        label("Parley timer: " + std::to_string(static_cast<int>(std::ceil(p.timeLeft))) + "s remaining",
              {600, 294}, 16, low ? sf::Color{234, 108, 108} : Gold);
        if (p.hiddenIndex < 3) {
            label("High Cunning revealed 1 weak bluff!", {1132, 294}, 15, sf::Color{119, 198, 147});
        }
        bar(ratio, {{600, 320}, {784, 12}}, low ? sf::Color{234, 108, 108} : Gold);
        reading(prequel_.body(), {{600, 342}, {784, 206}});
    } else {
        reading(prequel_.body(), {{600, 298}, {784, 250}});
    }

    // Action / Choice / Combat Tactics Deck
    panel({{580, 578}, {824, 288}}, Surface, Border);
    if (phase == PrequelPhase::ActIntro) {
        label("BEGIN ACT", {600, 594}, 14, Gold);
        paragraph("Your decisions across Act I, II, and III weave together the events leading directly to the first chamber of Dungeon Descent.",
                  {{600, 622}, {784, 70}}, 17);
        button("prequel-advance", "Begin " + PrequelRun::actTitle(prequel_.act()) + "  [Enter]", {{600, 774}, {784, 62}}, true, true);
    } else if (phase == PrequelPhase::Scene) {
        label("YOUR CHOICE  [1-" + std::to_string(prequel_.choices().size()) + "]", {600, 592}, 14, Gold);
        const auto& choices = prequel_.choices();
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const float cy = 622.f + static_cast<float>(i) * 74.f;
            button("choice-" + std::to_string(i),
                   std::to_string(i + 1) + "   " + choices[i].label,
                   {{600, cy}, {784, 62}});
        }
    } else if (phase == PrequelPhase::Parley) {
        label("PARLEY RESPONSE  [1-3]", {600, 592}, 14, Gold);
        const auto& p = prequel_.parley();
        for (std::size_t i = 0; i < 3; ++i) {
            const float cy = 622.f + static_cast<float>(i) * 74.f;
            const bool avail = prequel_.parleyOptionAvailable(i);
            const std::string text = avail
                ? (std::to_string(i + 1) + "   " + p.options[i])
                : (std::to_string(i + 1) + "   [Eliminated by Cunning]");
            button("choice-" + std::to_string(i), text, {{600, cy}, {784, 62}}, avail);
        }
    } else if (phase == PrequelPhase::Combat) {
        label("COMBAT TACTICS  [1-4]", {600, 592}, 14, Gold);
        const std::array<std::string, 4> labels = {
            "1  Standard Strike",
            "2  Heavy Strike (2 strikes)",
            "3  Guard Strike",
            "4  " + ch.abilityName + (prequel_.combatChoiceAvailable(CombatAction::ClassAbility) ? "" : " (Used)")
        };
        CombatAction previewAction = CombatAction::StandardStrike;
        for (int i = 0; i < 4; ++i) {
            const auto actChoice = static_cast<CombatAction>(i);
            const float col = (i % 2) * 398.f;
            const float row = (i / 2) * 66.f;
            const sf::FloatRect r{{600.f + col, 620.f + row}, {386.f, 56.f}};
            const bool avail = prequel_.combatChoiceAvailable(actChoice);
            button("choice-" + std::to_string(i), labels[i], r, avail);
            const auto mousePos = pointer(sf::Mouse::getPosition(window_));
            if ((mousePos && r.contains(*mousePos)) ||
                (selectedButton_ < buttons_.size() && buttons_.back().id == buttons_[selectedButton_].id)) {
                previewAction = actChoice;
            }
        }
        const auto prev = prequel_.previewCombatAction(previewAction);
        panel({{600, 760}, {784, 88}}, {11, 17, 25}, Border);
        label("TACTICAL PREVIEW:  Dmg " + num(prev.minDamage) + " (Crit " + num(prev.critDamage) +
              ")   |   Counter: " + num(prev.expectedCounter) + " HP   |   Cost: " + std::to_string(prev.strikeCost) + " strike(s)",
              {616, 772}, 16, Gold);
        label(prev.effect, {616, 804}, 15, Ink);
    } else if (phase == PrequelPhase::Outcome) {
        label("CHRONICLE UPDATED", {600, 594}, 14, Gold);
        button("prequel-advance", "Continue  [Enter]", {{600, 774}, {784, 62}}, true, true);
    } else if (phase == PrequelPhase::ActComplete) {
        label("ACT COMPLETE", {600, 594}, 14, Gold);
        const std::string nextLabel = prequel_.act() == PrequelAct::Act1_Eldred
            ? "Continue to Act II: The Hidden Heir (Malvel)  [Enter]"
            : "Continue to Act III: The Silk Smile (Princess Sophie)  [Enter]";
        button("prequel-advance", nextLabel, {{600, 774}, {784, 62}}, true, true);
    } else if (phase == PrequelPhase::Defeat) {
        label("FALLEN IN THE CHRONICLE", {600, 594}, 14, sf::Color{234, 108, 108});
        button("prequel-retry-act", "Retry " + PrequelRun::actTitle(prequel_.act()), {{600, 636}, {784, 58}}, true, true);
        button("prequel-restart", "Restart Ashes of Falkreath from Act I", {{600, 708}, {784, 54}});
        button("menu", "Return to Main Menu", {{600, 776}, {784, 54}});
    } else if (phase == PrequelPhase::Epilogue) {
        label("PRELUDE COMPLETE  —  " + PrequelRun::codaTitle(prequel_.coda()), {600, 594}, 15, Gold);
        label("Battles won: " + std::to_string(prequel_.battlesWon()) +
              "   |   Parleys won: " + std::to_string(prequel_.parleysWon()) +
              (prequel_.ironChronicleEligible() ? "   |   IRON CHRONICLE!" : ""),
              {600, 622}, 16, Ink);
        button("prequel-to-main", "Awaken in the Pedestal Chamber (Begin Dungeon Descent)", {{600, 666}, {784, 58}}, true, true);
        button("prequel-restart", "Replay Ashes of Falkreath (Explore the Other Coda)", {{600, 736}, {784, 52}});
        button("menu", "Return to Main Menu", {{600, 798}, {784, 50}});
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
    case Screen::Prequel: drawPrequel(); break;
    case Screen::Lore: drawLore(); break;
    case Screen::Results: drawResults(); break;
    case Screen::Leaderboard: drawLeaderboard(true, true); break;
    case Screen::Codex: drawCodex(); break;
    case Screen::Gauntlet: drawGauntlet(); break;
    }
    if (overlay_ != Overlay::None) drawOverlay();
    if (screenFade_ > 0.01f && !options_.smoke) {
        panel({{0, 0}, {Width, Height}}, {4, 8, 14, static_cast<std::uint8_t>(std::clamp(screenFade_, 0.f, 1.f) * 180.f)}, sf::Color::Transparent);
    }
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
