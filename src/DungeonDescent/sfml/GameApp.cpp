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
const sf::Color Background{10, 15, 22}, Surface{17, 26, 36}, Border{48, 65, 78};
const sf::Color Gold{223, 187, 119}, Ink{226, 231, 233}, Muted{153, 171, 181};
const sf::Color Ice{123, 198, 224}, Leaf{119, 198, 147}, Sand{224, 181, 113}, Mist{191, 166, 226};
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
}
sf::String GameApp::utf8(const std::string& text) { return sf::String::fromUtf8(text.begin(), text.end()); }
GameApp::GameApp(LaunchOptions options) : options_(std::move(options)), floor_(options_.assets) {
    if (!bodyFont_.openFromFile(options_.assets / "fonts/Arial.ttf") ||
        !titleFont_.openFromFile(options_.assets / "fonts/VIKING-N.TTF"))
        throw std::runtime_error("Cannot load fonts from " + options_.assets.string());
    if (!canvas_.resize({static_cast<unsigned>(Width), static_cast<unsigned>(Height)}))
        throw std::runtime_error("Cannot create the game canvas.");
    canvas_.setSmooth(true);
    openWindow(true);
    seed_ = std::random_device{}();
    if (options_.smoke) {
        floor_.start(Hero::Warrior, Biome::Ice, 7);
        screen_ = Screen::Game;
        syncStory();
    } else if (options_.skipIntro) enter(Screen::Menu);
    else enter(Screen::Intro);
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
void GameApp::handle(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) { overlay_ = Overlay::Quit; buttons_.clear(); return; }
    if (event.is<sf::Event::FocusLost>()) {
        if (screen_ == Screen::Game && overlay_ == Overlay::None && !timedRiddle()) overlay_ = Overlay::Pause;
        return;
    }
    if (event.is<sf::Event::FocusGained>()) return;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::F11) { openWindow(!fullscreen_); return; }
        if (key->code == Key::V) { action("sound"); return; }
        if (key->code == Key::Escape) {
            if (overlay_ != Overlay::None) { overlay_ = Overlay::None; buttons_.clear(); }
            else if (screen_ == Screen::Intro) enter(Screen::Menu);
            else if (screen_ == Screen::Lore) action("lore-back");
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
        for (const auto& button : buttons_) if (button.enabled && button.bounds.contains(*point)) {
            const auto id = button.id;
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
    page_ = 0;
    readingText_.clear();
    buttons_.clear();
    selectedButton_ = screen == Screen::Difficulty ? static_cast<std::size_t>(selectedDifficulty_) : 0;
    animationTime_ = 0;
    if (screen == Screen::Intro) { playMusic("DD.wav"); playIntroNarration(); }
    if (screen == Screen::Menu) playMusic("menu.mp3");
}
void GameApp::action(const std::string& id) {
    // Navigation may not replace a live descent, including floor transitions
    // and lore. A new hero can only be chosen before a run or after it ends.
    if (floor_.inProgress() && (id == "menu" || id == "start" || id == "heroes" || id == "restart" || id == "difficulty")) return;
    if (timedRiddle() && (id == "pause" || id == "map" || id == "lore")) return;
    if (id == "sound") {
        muted_ = !muted_;
        if (music_) music_->setVolume(muted_ ? 0.f : 25.f);
        if (narration_) narration_->setVolume(muted_ ? 0.f : 60.f);
        return;
    }
    if (id == "page-next") { if (overlay_ == Overlay::None) page_ = std::min(page_ + 1, pageCount_ - 1); return; }
    if (id == "page-prev") { if (overlay_ == Overlay::None) page_ = std::max(0, page_ - 1); return; }
    if (id == "quit") overlay_ = Overlay::Quit;
    else if (id == "quit-yes") {
        if (narration_) narration_->stop();
        if (music_) music_->stop();
        window_.close();
    }
    else if (id == "resume") overlay_ = Overlay::None;
    else if (id == "pause") overlay_ = Overlay::Pause;
    else if (id == "map") overlay_ = overlay_ == Overlay::Map ? Overlay::None : Overlay::Map;
    else if (id == "menu") enter(Screen::Menu);
    else if (id == "start") enter(Screen::Introduction);
    else if (id == "heroes") { choosingSecondFloor_ = false; enter(Screen::Heroes); }
    else if (id == "difficulty") enter(Screen::Difficulty);
    else if (id == "descend" && floor_.canDescend()) {
        if (floor_.floorNumber() == 1) { choosingSecondFloor_ = true; enter(Screen::Biomes); }
        else if (floor_.descend(Biome::Lava)) {
            enter(Screen::Game);
            storyRoom_ = -1;
            syncStory();
            playMusic(floor_.musicFile());
        }
    }
    else if (id == "descent-back") { choosingSecondFloor_ = false; enter(Screen::Game); }
    else if (id == "lore") {
        loreReturn_ = screen_;
        loreHero_ = screen_ == Screen::Menu ? Hero::Warrior : selectedHero_;
        enter(Screen::Lore);
    }
    else if (id == "lore-back") enter(loreReturn_);
    else if (id.rfind("lore-", 0) == 0) { loreHero_ = static_cast<Hero>(std::stoi(id.substr(5))); readingText_.clear(); page_ = 0; }
    else if (id == "results" && !floor_.inProgress() && (floor_.phase() == Phase::Ending || floor_.phase() == Phase::Defeat)) enter(Screen::Results);
    else if (id == "advance") { floor_.advance(); page_ = 0; readingText_.clear(); syncStory(); }
    else if (id == "attack") {
        if (attackDelay_ > 0) return;
        floor_.attack();
        attackDelay_ = .32f;
        flash_ = .2f;
        page_ = 0;
        readingText_.clear();
    }
    else if (id.rfind("choice-", 0) == 0) {
        const auto choice = static_cast<std::size_t>(std::stoi(id.substr(7)));
        if (screen_ == Screen::Heroes) { selectedHero_ = static_cast<Hero>(choice); choosingSecondFloor_ = false; enter(Screen::Difficulty); }
        else if (screen_ == Screen::Difficulty && choice < 4) { selectedDifficulty_ = static_cast<Difficulty>(choice); enter(Screen::Biomes); }
        else if (screen_ == Screen::Biomes) {
            const auto biome = static_cast<Biome>(choice + (choosingSecondFloor_ ? 2 : 0));
            if (choosingSecondFloor_) { if (!floor_.descend(biome)) return; }
            else { floor_.start(selectedHero_, biome, ++seed_, selectedDifficulty_); playTime_ = 0; }
            choosingSecondFloor_ = false;
            enter(Screen::Game);
            storyRoom_ = -1;
            syncStory();
            playMusic(floor_.musicFile());
        }
        else if (screen_ == Screen::Game && overlay_ == Overlay::None) {
            if (floor_.phase() == Phase::Riddle) floor_.answer(choice);
            else floor_.choose(choice);
            page_ = 0;
            readingText_.clear();
        }
    }
    buttons_.clear(); // No stale hit targets survive transitions or queued clicks.
    selectedButton_ = screen_ == Screen::Lore ? static_cast<std::size_t>(loreHero_) :
                      screen_ == Screen::Difficulty ? static_cast<std::size_t>(selectedDifficulty_) : 0;
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
    // Timed questions use wall-clock frame time, including focus loss and quit
    // confirmation. Frame stalls or window recreation must not buy extra time.
    if (screen_ == Screen::Game) floor_.tick(dt);
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
sf::Texture& GameApp::texture(const std::string& name) {
    const auto found = textures_.find(name);
    if (found != textures_.end()) return *found->second;
    auto tex = std::make_unique<sf::Texture>();
    if (!tex->loadFromFile(options_.assets / name)) throw std::runtime_error("Missing image: " + name);
    tex->setSmooth(true);
    // Bound animation memory instead of preloading 102 full-resolution frames.
    while (textures_.size() >= 8) { textures_.erase(textureOrder_.front()); textureOrder_.pop_front(); }
    textureOrder_.push_back(name);
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
    label("DUNGEON", {78, 212}, 88, Ink, true);
    label("DESCENT", {78, 309}, 88, Gold, true);
    paragraph("Every choice leaves a mark.\nChoose your hero, challenge the riddle keeper\nand uncover the truth beneath Aethoria.", {{85, 450}, {680, 140}}, 23);
    button("start", "Begin descent", {{85, 634}, {310, 66}});
    button("lore", "Lore of the heroes", {{85, 718}, {310, 56}});
    button("quit", "Quit", {{414, 718}, {130, 56}});
    button("sound", muted_ ? "Sound off [V]" : "Sound on [V]", {{1180, 34}, {220, 45}});
}
void GameApp::drawHeroes() {
    label("CHOOSE YOUR WEAPON", {36, 28}, 40, Gold, true);
    label("Four paths into the dark. Choose the weapon that calls to you.", {38, 86}, 21, Muted);
    const std::array<std::string, 4> weapons = {"Sword", "Longbow", "Wand", "Throwing knife"};
    for (int i = 0; i < 4; ++i) {
        const auto hero = static_cast<Hero>(i);
        const float x = 36 + i * 348.f;
        panel({{x, 145}, {324, 638}}, Surface, Border);
        image(DungeonRun::portrait(hero), {{x + 10, 155}, {304, 310}}, true);
        label(DungeonRun::heroName(hero), {x + 20, 484}, 31, Gold, true);
        label(weapons[i], {x + 20, 528}, 19, Muted);
        paragraph(DungeonRun::ability(hero), {{x + 20, 569}, {284, 94}}, 18);
        const auto p = DungeonRun::makePlayer(hero);
        label("Health " + num(p.stats[Health]), {x + 20, 675}, 19, Ink);
        button("choice-" + std::to_string(i), std::to_string(i + 1) + "  Choose " + DungeonRun::heroName(hero), {{x + 16, 714}, {292, 52}});
    }
    button("menu", "Back to menu", {{36, 813}, {210, 48}});
}
void GameApp::drawBiomes() {
    const int targetFloor = choosingSecondFloor_ ? 2 : 1;
    label(choosingSecondFloor_ ? "DEEPER INTO THE DUNGEON" : "TWO DOORS. ONE DESCENT.", {38, 30}, 40, Gold, true);
    const std::string subtitle = choosingSecondFloor_
        ? "The stairway leads deeper. Choose your next path."
        : DungeonRun::heroName(selectedHero_) + " / " + DungeonRun::difficultyName(selectedDifficulty_) + ". Which door will you open?";
    label(subtitle, {40, 90}, 22, Muted);
    const std::array<std::string, 4> descriptions = {
        "Frozen chambers, a captive warrior and the Yeti guarding the stairway.",
        "Forgotten halls, living roots and the Jungle Warden waiting below.",
        "Sandstone crypts, an injured butler and Ahmose the Pharaoh.",
        "Haunted chambers, a hidden vision and Count Dracula."};
    for (int i = 0; i < 2; ++i) {
        const float x = 40 + i * 700.f;
        const auto biome = static_cast<Biome>((targetFloor - 1) * 2 + i);
        const auto index = static_cast<std::size_t>(biome);
        panel({{x, 150}, {660, 625}}, Surface, Border);
        image(DungeonRun::bossImage(biome), {{x + 1, 151}, {658, 390}}, true, {65, 75, 85});
        image(DungeonRun::bossImage(biome), {{x + 1, 151}, {658, 390}});
        label(DungeonRun::biomeName(biome), {x + 24, 558}, 29, biomeAccent(biome), true);
        paragraph(descriptions[index], {{x + 24, 607}, {610, 65}}, 20);
        button("choice-" + std::to_string(i), std::to_string(i + 1) + "  Enter the " + DungeonRun::biomeKey(biome) + " door", {{x + 24, 694}, {612, 56}});
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
    const auto& p = floor_.player();
    const auto phase = floor_.phase();
    const auto accent = biomeAccent(floor_.biome());
    label("DUNGEON DESCENT", {28, 19}, 31, Gold, true);
    label("FLOOR " + std::to_string(floor_.floorNumber()) + "  /  " + DungeonRun::biomeName(floor_.biome()) + "  /  " + DungeonRun::difficultyName(floor_.difficulty()), {30, 68}, 18, accent);
    for (int i = 0; i < 9; ++i) {
        const float x = 527 + i * 36.f;
        panel({{x, 65}, {27, 27}}, i + 1 < floor_.roomNumber() ? sf::Color(64, 101, 99) : Surface,
              i + 1 == floor_.roomNumber() ? Gold : Border);
        label(std::to_string(i + 1), {x + 8, 68}, 15, i + 1 == floor_.roomNumber() ? Gold : Muted);
    }
    panel({{28, 126}, {864, 428}}, Surface, Border);
    image(floor_.image(), {{29, 127}, {862, 426}}, true, {55, 65, 75});
    image(floor_.image(), {{29, 127}, {862, 426}});
    panel({{29, 487}, {862, 66}}, {9, 16, 24, 225}, sf::Color::Transparent);
    label("ROOM " + std::to_string(floor_.roomNumber()) + " / 9", {48, 500}, 14, accent);
    label(floor_.title(), {222, 497}, 28, Ink, true);
    if (flash_ > 0) panel({{29, 127}, {862, 360}}, {223, 187, 119, static_cast<std::uint8_t>(flash_ * 160)}, sf::Color::Transparent);
    panel({{28, 574}, {280, 284}}, Surface, Border);
    image(DungeonRun::portrait(p.hero), {{43, 590}, {73, 84}}, true);
    label(DungeonRun::heroName(p.hero), {130, 589}, 24, Gold, true);
    label("HP  " + num(p.stats[Health]), {130, 628}, 24, Ink);
    for (int s = 0; s < 6; ++s) {
        if (s == Health) continue;
        const int row = s < Health ? s : 4;
        label(DungeonRun::statName(s), {46, 687.f + row * 26}, 17, Muted);
        label(num(p.stats[s]), {240, 687.f + row * 26}, 17, Ink);
    }
    panel({{328, 574}, {564, 284}}, Surface, Border);
    label(phase == Phase::Combat ? "YOUR TURN" : phase == Phase::Riddle ? "THE RIDDLE KEEPER" : "YOUR CHOICE", {350, 590}, 16, accent);
    if (phase == Phase::Room) {
        const auto choices = floor_.choices();
        for (std::size_t i = 0; i < choices.size(); ++i) {
            const bool vertical = choices.size() == 3;
            const float x = vertical ? 350.f : 348 + (i % 2) * 270.f;
            const float y = vertical ? 626 + i * 69.f : 632 + (i / 2) * 87.f;
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + choices[i], {{x, y}, {vertical ? 520.f : 254.f, vertical ? 58.f : 73.f}});
        }
        if (floor_.room() == Room::Battle || floor_.room() == Room::Boss)
            label("Enemy HP " + num(floor_.enemyMaxHealth()) + "  /  Counterattack " + num(floor_.enemyDamage(), 2), {350, 785}, 19, Muted);
    } else if (phase == Phase::Riddle) {
        unsigned row = 0;
        for (std::size_t i = 0; i < 3; ++i) {
            if (!floor_.answerAvailable(i)) continue;
            button("choice-" + std::to_string(i), std::to_string(i + 1) + "  " + floor_.riddle().answers[i], {{350, 626 + row++ * 69.f}, {520, 58}});
        }
    } else if (phase == Phase::Combat) {
        label("Strikes left: " + std::to_string(floor_.strikesLeft()), {350, 632}, 25, Ink);
        label(floor_.enemyName(), {350, 674}, 20, Muted);
        label(num(floor_.enemyHealth()) + " HP  /  Counterattack " + num(floor_.enemyDamage(), 2), {350, 705}, 18, Muted);
        bar(floor_.enemyHealth() / floor_.enemyMaxHealth(), {{350, 735}, {520, 9}}, accent);
        button("attack", "Attack [Enter / Space]", {{350, 752}, {520, 69}}, attackDelay_ <= 0);
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
    panel({{916, 126}, {496, 732}}, Surface, Border);
    std::string heading = "THE CHAMBER", text = roomStory_;
    if (phase == Phase::Riddle) {
        heading = "ANSWER BEFORE TIME RUNS OUT";
        text = floor_.riddle().question;
        label(std::to_string(static_cast<int>(std::ceil(floor_.timeLeft()))) + " seconds", {938, 201}, 24, Gold);
        bar(floor_.timeLeft() / floor_.timeLimit(), {{938, 245}, {452, 8}}, accent);
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
        : "Tab / arrows to select   Enter to confirm   1-4 for choices   Mouse wheel to turn pages   F11 fullscreen", {30, 876}, 15, Muted);
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
void GameApp::drawResults() {
    const auto& p = floor_.player();
    image(floor_.image(), {{0, 0}, {Width, Height}}, true, {70, 80, 90});
    panel({{310, 70}, {820, 760}}, {10, 17, 25, 245}, Border);
    label("YOUR JOURNEY", {354, 106}, 40, Gold, true);
    label(floor_.endingName(), {354, 168}, 28, Ink, true);
    label(DungeonRun::heroName(p.hero) + " / " + DungeonRun::difficultyName(floor_.difficulty()), {354, 230}, 22, Muted);
    const int seconds = static_cast<int>(playTime_);
    std::ostringstream duration;
    duration << seconds / 3600 << ':' << std::setfill('0') << std::setw(2) << seconds / 60 % 60 << ':' << std::setw(2) << seconds % 60;
    const std::array<std::pair<std::string, std::string>, 7> results = {{
        {"Score", std::to_string(floor_.score())},
        {"Floor reached", std::to_string(floor_.floorNumber()) + " / 3"},
        {"Riddles solved", std::to_string(p.riddlesCorrect) + " / " + std::to_string(p.riddlesAnswered)},
        {"Battles won", std::to_string(p.battlesWon) + " / " + std::to_string(p.battlesFought)},
        {"Reputation", std::to_string(p.reputation) + " / 9"},
        {"Health remaining", num(p.stats[Health])},
        {"Playtime", duration.str()}}};
    for (std::size_t i = 0; i < results.size(); ++i) {
        const float y = 280 + i * 53.f;
        label(results[i].first, {354, y}, 23, Muted);
        label(results[i].second, {930, y}, 25, Ink);
    }
    button("menu", "Main menu", {{354, 718}, {340, 60}});
    button("quit", "Quit game", {{718, 718}, {368, 60}});
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
    } else {
        panel({{440, 240}, {560, 425}}, Background, Border);
        label(overlay_ == Overlay::Quit ? "LEAVE THE DUNGEON?" : "PAUSED", {477, 275}, 33, Gold, true);
        paragraph(overlay_ == Overlay::Quit
            ? (timedRiddle() ? "The riddle timer is still running. Quit the game and end this descent?" : "Quit the game? Your current descent will end.")
            : "Pause is available between riddles. Timed riddles continue even if you switch away from the game.", {{478, 340}, {484, 92}}, 21);
        button("resume", overlay_ == Overlay::Quit ? "Keep playing" : "Resume descent", {{478, 450}, {484, 56}});
        button(overlay_ == Overlay::Quit ? "quit-yes" : "quit", "Quit game", {{478, 526}, {484, 56}});
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
