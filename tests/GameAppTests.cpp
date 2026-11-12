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
    static void checkContinuousRun(GameApp& app) {
        check(app.floor_.inProgress(), "Continuous-run check requires a live descent");
        app.render();
        check(std::none_of(app.buttons_.begin(), app.buttons_.end(), [](const auto& b) {
            return b.id == "menu" || b.id == "restart" || b.id == "start" || b.id == "retry" || b.id == "retry-floor";
        }), "A live descent exposes no reset, retry or main-menu control");
        const auto before = app.floor_.player();
        const auto screen = app.screen_;
        const int floor = app.floor_.floorNumber(), room = app.floor_.roomNumber();
        for (const auto& id : {"menu", "restart", "start", "heroes", "retry", "retry-floor"}) app.action(id);
        check(app.screen_ == screen && app.floor_.floorNumber() == floor && app.floor_.roomNumber() == room &&
              app.floor_.player().stats == before.stats && app.floor_.player().riddlesAnswered == before.riddlesAnswered,
              "Queued reset/navigation actions cannot replace a live descent");
        app.render();
    }
    static void checkLoreHighlight(GameApp& app, int active) {
        const auto pixels = app.canvas_.getTexture().copyToImage();
        for (int tab = 0; tab < 4; ++tab) {
            const auto border = pixels.getPixel({static_cast<unsigned>(40 + tab * 206), 102});
            check((border == sf::Color(223, 187, 119)) == (tab == active), "Only the displayed character's lore tab has a gold border");
        }
    }
    static void checkOtherEndings(GameApp& app, const std::filesystem::path& captures) {
        for (bool lose : {false, true}) {
            app.floor_.start(Hero::Mage, Biome::Ice, lose ? 9 : 15);
            for (int step = 0; step < 900 && app.floor_.inProgress(); ++step) {
                auto& run = app.floor_;
                if (run.phase() == Phase::Complete) run.descend(run.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
                else if (run.phase() == Phase::Room) {
                    std::size_t choice = 0;
                    if (run.room() == Room::Library) choice = lose && run.floorNumber() == 3 ? 3 : 2;
                    if (run.room() == Room::Battle && lose) choice = 1;
                    if (!lose && run.room() == Room::Shop) choice = run.floorNumber() == 3 ? 2 : 1;
                    if (!lose && (run.room() == Room::Respite || run.room() == Room::Encounter)) choice = 1;
                    run.choose(choice);
                } else if (run.phase() == Phase::Riddle) {
                    if (lose && run.floorNumber() == 3) {
                        for (std::size_t i = 0; i < 3; ++i) if (i != run.riddle().correct && run.answerAvailable(i)) { run.answer(i); break; }
                    } else run.answer(run.riddle().correct);
                } else if (run.phase() == Phase::Combat) run.attack();
                else run.advance();
            }
            check(app.floor_.phase() == Phase::Ending && app.floor_.ending() == (lose ? Ending::Bad : Ending::Good), "Prepare each remaining ending through a real run");
            app.enter(GameApp::Screen::Game);
            capture(app, captures, lose ? "Bad_Ending" : "Good_Ending");
            check(app.readingText_ == app.floor_.message(), "Full original ending text reaches the renderer");
            for (int page = 1; page < app.pageCount_; ++page) {
                key(app, sf::Keyboard::Key::Right);
                check(app.page_ == page, "Every ending page is reachable");
            }
            key(app, sf::Keyboard::Key::Escape);
            check(app.screen_ == GameApp::Screen::Results, "Escape from an ending opens results");
            capture(app, captures, lose ? "Bad_Ending_Results" : "Good_Ending_Results");
            click(app, "menu");
        }
        // Early game over shares the results flow and has no retry controls.
        app.floor_.start(Hero::Warrior, Biome::Jungle, 3);
        for (int step = 0; step < 400 && app.floor_.inProgress(); ++step) {
            if (app.floor_.phase() == Phase::Room) app.floor_.choose(0);
            else if (app.floor_.phase() == Phase::Riddle) app.floor_.answer((app.floor_.riddle().correct + 1) % 3);
            else if (app.floor_.phase() == Phase::Combat) app.floor_.attack();
            else app.floor_.advance();
        }
        check(app.floor_.phase() == Phase::Defeat, "Prepare an early game over");
        app.enter(GameApp::Screen::Game);
        app.storyRoom_ = -1; app.syncStory();
        capture(app, captures, "Game_Over");
        check(std::none_of(app.buttons_.begin(), app.buttons_.end(), [](const auto& b) { return b.id == "retry" || b.id == "retry-floor" || b.id == "restart"; }), "Game over offers no retries");
        click(app, "results");
        click(app, "menu");
    }
    static void checkPerfectEnding(GameApp& app, const std::filesystem::path& captures) {
        bool ready = false;
        for (unsigned seed = 0; seed < 200 && !ready; ++seed) {
            app.floor_.start(Hero::Mage, Biome::Ice, seed);
            for (int step = 0; step < 900 && app.floor_.inProgress(); ++step) {
                auto& run = app.floor_;
                if (run.floorNumber() == 3 && run.room() == Room::Boss && run.phase() == Phase::Combat) {
                    ready = run.player().battlesFought == 9 && run.player().battlesWon == 8 && run.player().reputation == 9 &&
                            run.player().riddlesAnswered == 24 && run.player().riddlesCorrect == 24;
                    break;
                }
                if (run.phase() == Phase::Complete) run.descend(run.floorNumber() == 1 ? Biome::Ghost : Biome::Lava);
                else if (run.phase() == Phase::Room) {
                    std::size_t choice = 0;
                    if (run.room() == Room::Library && run.floorNumber() < 3) choice = 2;
                    if (run.room() == Room::Respite) choice = run.floorNumber() == 1 ? 0 : 1;
                    if (run.room() == Room::Shop) choice = run.floorNumber() == 2 ? 1 : 2;
                    run.choose(choice);
                } else if (run.phase() == Phase::Combat) run.attack();
                else if (run.phase() == Phase::Riddle) run.answer(run.riddle().correct);
                else run.advance();
            }
        }
        check(ready, "Earn a perfect run up to the final battle without injecting stats");
        app.selectedHero_ = Hero::Mage;
        app.enter(GameApp::Screen::Game);
        app.storyRoom_ = -1; app.syncStory();
        for (int strike = 0; strike < 80 && app.floor_.phase() == Phase::Combat; ++strike) {
            app.update(.4f);
            click(app, "attack");
        }
        check(app.floor_.phase() == Phase::Ending && app.floor_.ending() == Ending::Perfect && app.floor_.player().battlesWon == 9,
              "The real final attack unlocks the fourth ending and counts Malvel");
        capture(app, captures, "Perfect_Ending");
        check(app.readingText_ == DungeonRun::readText(app.options_.assets / "textfiles/ending/perfectending.txt") && app.pageCount_ > 1,
              "The complete new story reaches the paginated ending screen");
        auto& artwork = app.texture("images/ending/perfectending.jpg");
        check(artwork.getSize() == sf::Vector2u(2048, 2048), "Load the supplied ending image at its original dimensions");
        for (int page = 1; page < app.pageCount_; ++page) {
            key(app, sf::Keyboard::Key::Right);
            check(app.page_ == page, "Every page of the perfect ending can be read");
        }
        capture(app, captures, "Perfect_Ending_Last_Page");
        click(app, "results");
        check(app.screen_ == GameApp::Screen::Results && app.floor_.player().reputation == 9 && app.floor_.player().riddlesCorrect == 24,
              "The fourth ending retains the 100% journey results");
        capture(app, captures, "Perfect_Ending_Results");
        click(app, "menu");
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
        for (int step = 0; step < 220 && app.floor_.inProgress() && app.floor_.phase() != Phase::Complete; ++step) {
            app.update(.4f);
            app.render();
            switch (app.floor_.phase()) {
            case Phase::Room: {
                std::size_t choice = 0;
                if (app.floor_.room() == Room::Library && app.selectedHero_ == Hero::Mage) choice = 2;
                if (app.floor_.room() == Room::Respite) choice = 1;
                if (app.floor_.room() == Room::Shop) choice = app.floor_.floorNumber() == 3 ? 2 : 1;
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
                if (app.floor_.floorNumber() == 3 && (app.floor_.message().find("Reputation +1") != std::string::npos)) {
                    capture(app, captures, prefix + (app.floor_.room() == Room::Library ? "_Diary" : app.floor_.room() == Room::Chest ? "_Orb" : "_Minion_Story"));
                }
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
        check(app.floor_.phase() == (app.floor_.floorNumber() == 3 ? Phase::Ending : Phase::Complete), "UI can complete the floor: " + prefix);
        if (app.floor_.floorNumber() == 2) check(visionChecked, "Floor 2 presents the original chest vision");
        capture(app, captures, prefix + "_Cleared");
    }
public:
    static void run(const std::filesystem::path& assets, const std::filesystem::path& captures) {
        std::filesystem::create_directories(captures);
        LaunchOptions options;
        options.assets = assets;
        options.audio = false;
        for (const auto& name : {"DD.wav", "menu.mp3", "icemusic.wav", "junglemusic.wav", "desertmusic.wav", "ghostmusic.wav", "lavamusic.wav"}) {
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
        for (int i = 0; i < 4; ++i) {
            click(app, "lore-" + std::to_string(i));
            check(app.loreHero_ == static_cast<Hero>(i), "Click changes the displayed lore");
            checkLoreHighlight(app, i);
            key(app, sf::Keyboard::Key::Tab);
            checkLoreHighlight(app, i);
        }
        click(app, "lore-1");
        capture(app, captures, "Lore_Ranger_Selected");
        key(app, sf::Keyboard::Key::Up);
        checkLoreHighlight(app, 1);
        capture(app, captures, "Lore_Ranger_Warrior_Focused");
        key(app, sf::Keyboard::Key::Enter);
        check(app.loreHero_ == Hero::Warrior, "Keyboard focus activates the selected lore tab");
        checkLoreHighlight(app, 0);
        click(app, "lore-back");
        for (int path = 0; path < 2; ++path) {
            app.seed_ = 42;
            click(app, "start");
            click(app, "heroes");
            capture(app, captures, "Hero_Selection");
            key(app, path == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num3);
            check(app.screen_ == GameApp::Screen::Biomes, "Keyboard selects a hero");
            key(app, path == 0 ? sf::Keyboard::Key::Num1 : sf::Keyboard::Key::Num2);
            check(app.floor_.player().stats == DungeonRun::makePlayer(app.selectedHero_).stats && app.playTime_ == 0, "Only a new run after results resets the hero and playtime");
            checkContinuousRun(app);
            click(app, "lore");
            checkContinuousRun(app);
            click(app, "lore-back");
            app.window_.setSize({1000, 700});
            app.render();
            check(!app.pointer({0, 0}), "Letterbox margins are not clickable");
            click(app, "map");
            check(app.overlay_ == GameApp::Overlay::Map, "Scaled mouse coordinates open map");
            key(app, sf::Keyboard::Key::M);
            app.window_.setSize({1280, 800});
            playFloor(app, captures, path == 0 ? "Ice" : "Jungle");
            checkContinuousRun(app);
            const auto carried = app.floor_.player();
            click(app, "descend");
            check(app.screen_ == GameApp::Screen::Biomes && app.choosingSecondFloor_, "First guardian opens floor 2 biome choice");
            checkContinuousRun(app);
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
            const float elapsed = app.playTime_;
            app.update(1.25f);
            check(app.playTime_ == elapsed + 1.25f, "Journey playtime includes pauses");
            checkContinuousRun(app);
            click(app, "resume");
            playFloor(app, captures, path == 0 ? "Desert" : "Ghost");
            check(app.floor_.player().riddlesAnswered == 16 && app.floor_.canDescend(), "Second guardian opens Malvel's domain");
            checkContinuousRun(app);
            const auto finalCarry = app.floor_.player();
            click(app, "descend");
            check(app.screen_ == GameApp::Screen::Game && app.floor_.floorNumber() == 3 && app.floor_.biome() == Biome::Lava &&
                  app.floor_.player().stats == finalCarry.stats && app.floor_.player().reputation == finalCarry.reputation && app.floor_.player().riddlesAnswered == 16,
                  "Malvel's domain continues the same run directly");
            check(app.roomStory_ == DungeonRun::readText(assets / app.floor_.storyFile()), "Lava stories replace the previous biome");
            checkContinuousRun(app);
            click(app, "map");
            capture(app, captures, "Lava_Map");
            check(app.floor_.mapImage() == "images/map/3/floor3_map.jpg", "Third-floor map");
            key(app, sf::Keyboard::Key::M);
            playFloor(app, captures, path == 0 ? "Lava_Warrior" : "Lava_Mage");
            check(app.floor_.ending() == Ending::True && app.floor_.player().riddlesAnswered == 24 && !app.floor_.inProgress(), "Actual three-floor UI playthrough reaches the true ending");
            check(app.pageCount_ > 1, "The full ending story is paginated");
            key(app, sf::Keyboard::Key::Right);
            check(app.page_ == 1, "Ending story pages can be read");
            capture(app, captures, "True_Ending_Page_2");
            click(app, "results");
            check(app.screen_ == GameApp::Screen::Results, "Completed run shows its results");
            const float finishedTime = app.playTime_;
            app.update(20);
            check(app.playTime_ == finishedTime, "Journey playtime freezes after the ending");
            capture(app, captures, "Journey_Results");
            click(app, "menu");
            check(app.screen_ == GameApp::Screen::Menu, "Return to the menu only after the run ends");
        }
        checkOtherEndings(app, captures);
        checkPerfectEnding(app, captures);
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
        std::cout << "Fullscreen/F11, five biomes, three-floor playthroughs, continuous descents, lore highlights, endings, maps, story pages and unpausable real-time riddles passed.\n";
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
