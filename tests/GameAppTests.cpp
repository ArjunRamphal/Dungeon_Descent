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
public:
    static void run(const std::filesystem::path& assets, const std::filesystem::path& captures) {
        std::filesystem::create_directories(captures);
        LaunchOptions options;
        options.assets = assets;
        options.audio = false;
        for (const auto& name : {"DD.wav", "menu.mp3", "icemusic.wav", "junglemusic.wav"}) {
            sf::InputSoundFile music;
            check(music.openFromFile(assets / "music" / name) && music.getSampleCount() > 0,
                  std::string("Decode music: ") + name);
        }
        GameApp app(options);
        app.render();
        key(app, sf::Keyboard::Key::Space);
        check(app.screen_ == GameApp::Screen::Menu, "Intro can be skipped");
        capture(app, captures, "01_Menu");
        click(app, "lore");
        for (int i = 0; i < 4; ++i) click(app, "lore-" + std::to_string(i));
        click(app, "lore-back");
        click(app, "start");
        check(app.screen_ == GameApp::Screen::Introduction, "Menu enters the introduction");
        click(app, "heroes");
        capture(app, captures, "02_Hero_Selection");
        // The renderer and event handler traverse an entire floor in each biome.
        for (int biome = 0; biome < 2; ++biome) {
            app.seed_ = 42;
            if (biome == 1) { click(app, "menu"); click(app, "start"); click(app, "heroes"); }
            key(app, biome == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num3);
            check(app.screen_ == GameApp::Screen::Biomes, "Keyboard selects a hero");
            capture(app, captures, "03_Biome_Selection");
            key(app, biome == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num2);
            check(app.screen_ == GameApp::Screen::Game, "Biome choice starts the game");
            app.window_.setSize({1000, 700});
            app.render();
            check(!app.pointer({0, 0}), "Letterbox margins are not clickable");
            click(app, "map");
            check(app.overlay_ == GameApp::Overlay::Map, "Scaled mouse coordinates open map");
            capture(app, captures, "04_Map");
            key(app, sf::Keyboard::Key::M);
            app.window_.setSize({1280, 800});
            bool pauseChecked = false;
            for (int step = 0; step < 220 && app.floor_.phase() != Phase::Complete; ++step) {
                app.update(.4f);
                app.render();
                switch (app.floor_.phase()) {
                case Phase::Room: {
                    std::size_t choice = 0;
                    if (app.floor_.room() == Room::Library && app.selectedHero_ == Hero::Mage) choice = 2;
                    if (app.floor_.room() == Room::Respite || app.floor_.room() == Room::Shop) choice = 1;
                    click(app, "choice-" + std::to_string(choice));
                    break;
                }
                case Phase::Combat:
                    if (app.floor_.room() == Room::Boss) capture(app, captures, biome == 0 ? "05_Ice_Boss" : "06_Jungle_Boss");
                    key(app, sf::Keyboard::Key::Enter);
                    break;
                case Phase::Riddle:
                    if (!pauseChecked) {
                        const float before = app.floor_.timeLeft();
                        key(app, sf::Keyboard::Key::Escape);
                        app.update(8);
                        check(app.floor_.timeLeft() == before, "Pause stops the riddle clock");
                        capture(app, captures, "07_Paused");
                        key(app, sf::Keyboard::Key::Escape);
                        key(app, sf::Keyboard::Key::M);
                        app.update(8);
                        key(app, sf::Keyboard::Key::Num1);
                        check(app.floor_.timeLeft() == before && app.floor_.phase() == Phase::Riddle, "Map freezes timer and blocks underlying choices");
                        key(app, sf::Keyboard::Key::M);
                        click(app, "lore");
                        app.update(8);
                        check(app.floor_.timeLeft() == before, "Lore freezes the riddle timer");
                        click(app, "lore-back");
                        app.handle(sf::Event::FocusLost{});
                        app.update(8);
                        check(app.floor_.timeLeft() == before && app.overlay_ == GameApp::Overlay::Pause, "Focus loss pauses the game");
                        app.handle(sf::Event::FocusGained{});
                        key(app, sf::Keyboard::Key::Escape);
                        capture(app, captures, "08_Riddle");
                        pauseChecked = true;
                    }
                    click(app, "choice-" + std::to_string(app.floor_.riddle().correct));
                    break;
                case Phase::Outcome:
                case Phase::RiddleOutcome:
                case Phase::Loot: click(app, "advance"); break;
                case Phase::Defeat: throw std::runtime_error("UI route unexpectedly failed");
                default: break;
                }
            }
            check(app.floor_.phase() == Phase::Complete, "UI can reach the floor-cleared screen");
            capture(app, captures, biome == 0 ? "09_Ice_Floor_Cleared" : "10_Jungle_Floor_Cleared");
        }
        app.handle(sf::Event::Closed{});
        app.render();
        check(app.overlay_ == GameApp::Overlay::Quit && app.window_.isOpen(), "Closing the window requests confirmation");
        capture(app, captures, "11_Quit_Confirmation");
        click(app, "quit-yes");
        check(!app.window_.isOpen(), "Quit closes the window");
        std::cout << "SFML rendering, input, resize, overlays and both complete floor routes passed.\n";
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
