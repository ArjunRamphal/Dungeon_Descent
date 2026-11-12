#include "GameApp.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace dungeon {
class GameAppTests {
    static void check(bool condition, const std::string& message) {
        if (!condition) throw std::runtime_error(message);
    }
    static void key(GameApp& app, sf::Keyboard::Key key) {
        sf::Event::KeyPressed pressed{};
        pressed.code = key;
        app.handle(pressed);
        app.render();
    }
    static void click(GameApp& app, const std::string& id) {
        app.render();
        const auto found = std::find_if(app.buttons_.begin(), app.buttons_.end(), [&](const auto& b) { return b.id == id; });
        check(found != app.buttons_.end() && found->enabled, "Missing enabled control: " + id);
        const auto point = found->bounds.position + found->bounds.size / 2.f;
        const auto size = app.window_.getSize();
        const float scale = std::min(size.x / 1440.f, size.y / 900.f);
        const sf::Vector2i pixel{static_cast<int>(point.x * scale + (size.x - 1440 * scale) / 2),
                               static_cast<int>(point.y * scale + (size.y - 900 * scale) / 2)};
        app.handle(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, pixel});
        if (app.window_.isOpen()) app.render();
    }
    static void capture(GameApp& app, const std::filesystem::path& folder, const std::string& name) {
        app.render();
        check(app.canvas_.getTexture().copyToImage().saveToFile(folder / (name + ".png")), "Save screenshot");
    }
    static void checkTimedRiddle(GameApp& app, const std::filesystem::path& captures, const std::string& prefix) {
        const float before = app.floor_.timeLeft();
        key(app, sf::Keyboard::Key::Escape);
        check(app.overlay_ == GameApp::Overlay::None, "Escape cannot pause a timed riddle");
        key(app, sf::Keyboard::Key::M);
        check(app.overlay_ == GameApp::Overlay::None, "Map hotkey cannot suspend a timed riddle");
        for (const auto& id : {"pause", "map", "lore"}) {
            const auto button = std::find_if(app.buttons_.begin(), app.buttons_.end(), [&](const auto& b) { return b.id == id; });
            check(button != app.buttons_.end() && !button->enabled, std::string("Timed-riddle control disabled: ") + id);
            app.action(id);
            check(app.screen_ == GameApp::Screen::Game && app.overlay_ == GameApp::Overlay::None, "Disabled controls cannot change the screen");
        }
        app.handle(sf::Event::FocusLost{});
        app.update(1.25f);
        check(app.overlay_ == GameApp::Overlay::None && app.floor_.timeLeft() == before - 1.25f, "Focus loss keeps real countdown time");
        app.handle(sf::Event::FocusGained{});
        app.handle(sf::Event::Closed{});
        app.render();
        app.update(.5f);
        check(app.overlay_ == GameApp::Overlay::Quit && app.floor_.timeLeft() == before - 1.75f, "Quit confirmation does not pause the countdown");
        key(app, sf::Keyboard::Key::Escape);
        capture(app, captures, prefix + "_Riddle");
    }
    static void playFloor(GameApp& app, const std::filesystem::path& captures, const std::string& prefix) {
        bool timerChecked = false, visionChecked = false;
        for (int step = 0; step < 220 && app.floor_.phase() != Phase::Complete; ++step) {
            app.update(.4f);
            app.render();
            switch (app.floor_.phase()) {
            case Phase::Room: {
                std::size_t choice = 0;
                if (app.floor_.room() == Room::Library && app.selectedHero_ == Hero::Mage) choice = 2;
                if (app.floor_.room() == Room::Respite || app.floor_.room() == Room::Shop) choice = 1;
                if (app.floor_.room() == Room::Shop || app.floor_.room() == Room::Encounter)
                    capture(app, captures, prefix + (app.floor_.room() == Room::Shop ? "_Shop" : "_Encounter"));
                click(app, "choice-" + std::to_string(choice));
                break;
            }
            case Phase::Combat:
                if (app.floor_.room() == Room::Boss) capture(app, captures, prefix + "_Boss");
                key(app, sf::Keyboard::Key::Enter);
                break;
            case Phase::Riddle:
                if (!timerChecked) { checkTimedRiddle(app, captures, prefix); timerChecked = true; }
                if (app.floor_.hasObservationHint()) {
                    check(app.buttons_.end() == std::find_if(app.buttons_.begin(), app.buttons_.end(), [&](const auto& b) {
                        return b.id.rfind("choice-", 0) == 0 && !app.floor_.answerAvailable(static_cast<std::size_t>(std::stoi(b.id.substr(7))));
                    }), "Eliminated answers have no input target");
                    capture(app, captures, prefix + "_Observation");
                }
                click(app, "choice-" + std::to_string(app.floor_.riddle().correct));
                break;
            case Phase::Outcome:
            case Phase::RiddleOutcome:
            case Phase::Loot:
                if (app.floor_.floorNumber() == 2 && app.floor_.roomNumber() == 5 && app.floor_.message().find("Sophie") != std::string::npos) {
                    capture(app, captures, prefix + "_Chest_Vision");
                    if (app.pageCount_ > 1) {
                        key(app, sf::Keyboard::Key::Right);
                        check(app.page_ == 1, "Vision text can be paged");
                        capture(app, captures, prefix + "_Chest_Vision_Page_2");
                    }
                    visionChecked = true;
                }
                click(app, "advance");
                break;
            case Phase::Defeat: throw std::runtime_error("UI route unexpectedly failed in " + prefix);
            default: break;
            }
        }
        check(app.floor_.phase() == Phase::Complete, "UI can reach floor clear: " + prefix);
        if (app.floor_.floorNumber() == 2) check(visionChecked, "Floor 2 presents the original chest vision");
        capture(app, captures, prefix + "_Cleared");
    }
public:
    static void run(const std::filesystem::path& assets, const std::filesystem::path& captures) {
        std::filesystem::create_directories(captures);
        LaunchOptions options;
        options.assets = assets;
        options.audio = false;
        for (const auto& name : {"DD.wav", "menu.mp3", "icemusic.wav", "junglemusic.wav", "desertmusic.wav", "ghostmusic.wav"}) {
            sf::InputSoundFile music;
            check(music.openFromFile(assets / "music" / name) && music.getSampleCount() > 0,
                  std::string("Decode music: ") + name);
        }
        GameApp app(options);
        check(app.fullscreen_ && app.window_.getSize() == sf::VideoMode::getDesktopMode().size, "Game starts in fullscreen");
        app.render();
        key(app, sf::Keyboard::Key::F11);
        check(!app.fullscreen_, "F11 switches to a window");
        key(app, sf::Keyboard::Key::F11);
        check(app.fullscreen_, "F11 restores fullscreen");
        key(app, sf::Keyboard::Key::F11);
        key(app, sf::Keyboard::Key::Space);
        check(app.screen_ == GameApp::Screen::Menu, "Intro can be skipped");
        capture(app, captures, "Menu");
        click(app, "lore");
        for (int i = 0; i < 4; ++i) click(app, "lore-" + std::to_string(i));
        click(app, "lore-back");
        for (int path = 0; path < 2; ++path) {
            app.seed_ = 42;
            click(app, "start");
            click(app, "heroes");
            capture(app, captures, "Hero_Selection");
            key(app, path == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num3);
            check(app.screen_ == GameApp::Screen::Biomes, "Keyboard selects a hero");
            key(app, path == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num2);
            app.window_.setSize({1000, 700});
            app.render();
            check(!app.pointer({0, 0}), "Letterbox margins are not clickable");
            click(app, "map");
            check(app.overlay_ == GameApp::Overlay::Map, "Scaled mouse coordinates open map");
            key(app, sf::Keyboard::Key::M);
            app.window_.setSize({1280, 800});
            playFloor(app, captures, path == 0 ? "Ice" : "Jungle");
            const auto carried = app.floor_.player();
            click(app, "descend");
            check(app.screen_ == GameApp::Screen::Biomes && app.choosingSecondFloor_, "First guardian opens floor 2 biome choice");
            key(app, sf::Keyboard::Key::Escape);
            check(app.screen_ == GameApp::Screen::Game && app.floor_.phase() == Phase::Complete && app.floor_.player().stats == carried.stats,
                  "Backing out of the second-floor door preserves the victory and stats");
            click(app, "descend");
            capture(app, captures, "Floor_2_Doors");
            key(app, path == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num2);
            check(app.floor_.floorNumber() == 2 && app.floor_.player().stats == carried.stats && app.floor_.player().riddlesAnswered == 8,
                  "Door selection carries the hero into floor 2");
            check(app.roomStory_ == DungeonRun::readText(assets / app.floor_.storyFile()), "Second-floor story replaces the first-floor scene");
            check(app.floor_.mapImage() == "images/map/2/floor2_map.jpg", "Floor 2 uses its own map");
            click(app, "map");
            capture(app, captures, path == 0 ? "Desert_Map" : "Ghost_Map");
            key(app, sf::Keyboard::Key::M);
            click(app, "pause");
            check(app.overlay_ == GameApp::Overlay::Pause, "Pause remains available between riddles");
            click(app, "resume");
            playFloor(app, captures, path == 0 ? "Desert" : "Ghost");
            check(app.floor_.player().riddlesAnswered == 16 && !app.floor_.canDescend(), "Two-floor result contains cumulative progress");
            check(std::none_of(app.buttons_.begin(), app.buttons_.end(), [](const auto& b) { return b.id == "descend"; }), "No floor 3 action is exposed");
            click(app, "restart");
            check(app.floor_.floorNumber() == 1 && app.floor_.player().riddlesAnswered == 0 && app.floor_.player().stats == DungeonRun::makePlayer(app.selectedHero_).stats,
                  "A new descent from floor 2 resets the complete run");
            click(app, "pause");
            click(app, "menu");
        }
        // Check an actual timeout while the quit dialog is open, including a
        // long frame delay that used to be clamped and could buy extra time.
        app.floor_.start(Hero::Warrior, Biome::Ice, 7);
        app.enter(GameApp::Screen::Game);
        app.storyRoom_ = -1;
        app.syncStory();
        app.floor_.choose(app.floor_.room() == Room::Battle ? 1 : 0);
        if (app.floor_.phase() == Phase::Outcome) app.floor_.advance();
        check(app.floor_.phase() == Phase::Riddle, "Prepare a timed question for expiry check");
        const int answered = app.floor_.player().riddlesAnswered;
        app.handle(sf::Event::Closed{});
        app.update(1000);
        check(app.floor_.phase() == Phase::RiddleOutcome && app.floor_.player().riddlesAnswered == answered + 1,
              "A riddle expires even behind quit confirmation");
        app.update(1000);
        app.action("choice-" + std::to_string(app.floor_.riddle().correct));
        check(app.floor_.player().riddlesAnswered == answered + 1, "Background timeout resolves once");
        app.render();
        check(app.overlay_ == GameApp::Overlay::Quit && app.window_.isOpen(), "Closing the window requests confirmation");
        click(app, "quit-yes");
        check(!app.window_.isOpen(), "Quit closes the window");
        std::cout << "Fullscreen/F11, all four biome routes, floor transitions, carryover, input, maps, story pages and unpausable real-time riddles passed.\n";
    }
};
}
int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("Pass assets and screenshot directories");
        dungeon::GameAppTests::run(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
