// --- SFML Includes ---
#include <queue>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include "StartScreen.h"
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace System;
using namespace System::Windows::Forms;
using namespace DungeonDescent;

[STAThreadAttribute]
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    StartScreen^ form = gcnew StartScreen();

    // --- Intro Music ---
    sf::Music music;
    if (!music.openFromFile("music/DD.wav")) return -1;
    music.setLooping(true);
    music.setVolume(30);

    sf::Music music2;
    if (!music2.openFromFile("music/intro.wav")) return -1;
    music2.setVolume(60);

    sf::Music musicmenu;
    if (!musicmenu.openFromFile("music/menu.mp3")) return -1;
    musicmenu.setLooping(true);
    musicmenu.setVolume(30);

    // --- SFML Window ---
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Dungeon Descent", sf::State::Fullscreen);

    // --- Loading Screen Loop ---
    sf::Texture loadingTex;
    if (!loadingTex.loadFromFile("images/loadscreen.png")) {
        return -1;
    }
    sf::Sprite loadingSprite(loadingTex);
    sf::Clock loadingClock;

    while (loadingClock.getElapsedTime().asSeconds() < 1.0f) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                return 0;
            }
        }

        window.clear();
        window.draw(loadingSprite);
        window.display();
    }

    // --- Intro Animation Frames ---
    std::queue<std::shared_ptr<sf::Texture>> introFrames;
    for (int i = 1; i <= 11; ++i) {
        auto tex = std::make_shared<sf::Texture>();
        if (tex->loadFromFile("candle/candle" + std::to_string(i) + ".jpg"))
            introFrames.push(tex);
    }
    if (introFrames.empty()) return -1;

    sf::Sprite sprite(*introFrames.front());

    auto applyLetterbox = [&](sf::Sprite& spr, std::shared_ptr<sf::Texture> tex) {
        float scale = std::fmin(
            float(window.getSize().x) / tex->getSize().x,
            float(window.getSize().y) / tex->getSize().y
        );
        spr.setScale({ scale, scale });
        sf::FloatRect bounds = spr.getLocalBounds();
        spr.setPosition({
            (window.getSize().x - bounds.size.x * scale) / 2.f,
            (window.getSize().y - bounds.size.y * scale) / 2.f
            });
        };
    applyLetterbox(sprite, introFrames.front());

    sf::Font font;
    if (!font.openFromFile("fonts/Arial.ttf")) return -1;

    sf::Text text(font);
    text.setString("Press any key to continue");
    text.setCharacterSize(50);
    text.setFillColor(sf::Color::White);
    text.setStyle(sf::Text::Bold | sf::Text::Underlined);
    text.setPosition({ 950.f, 1350.f });

    sf::Clock clock;
    float introFrameDelay = 0.1f;

    // --- Start Screen Video Frames ---
    std::vector<std::shared_ptr<sf::Texture>> videoFrames;
    for (int i = 1; i <= 91; ++i) {
        auto tex = std::make_shared<sf::Texture>();
        std::string filename = "images/startscreen/startscreen" + std::to_string(i) + ".jpg";
        if (tex->loadFromFile(filename)) videoFrames.push_back(tex);
    }
    if (videoFrames.empty()) return -1;

    sf::Sprite videoSprite(*videoFrames.front());
    sf::Clock videoClock;
    float videoFrameDelay = 1.f / 30.f; // 30 FPS
    size_t currentVideoFrame = 0;

    auto applyFullscreenVideo = [&](sf::Sprite& spr, std::shared_ptr<sf::Texture> tex) {
        float scaleX = float(window.getSize().x) / tex->getSize().x;
        float scaleY = float(window.getSize().y) / tex->getSize().y;
        spr.setScale({ scaleX, scaleY });
        spr.setPosition({ 0.f, 0.f }); // top-left corner
        };


    applyFullscreenVideo(videoSprite, videoFrames.front());


    // --- Menu Setup (NEW) ---
        // --- Menu Buttons as Images ---
    sf::Texture playTex, loreTex, quitTex, DDTex;
    if (!playTex.loadFromFile("images/menu/start.png")) return -1;
    if (!loreTex.loadFromFile("images/menu/lore.png")) return -1;
    if (!quitTex.loadFromFile("images/menu/quit.png")) return -1;
    if (!DDTex.loadFromFile("images/menu/title.png")) return -1;

    sf::Sprite playSprite(playTex), loreSprite(loreTex), quitSprite(quitTex), DDSprite(DDTex);

    // Position the buttons (center them nicely)
    playSprite.setPosition({ 1100.f, 600.f });
    loreSprite.setPosition({ 1100.f, 800.f });
    quitSprite.setPosition({ 1100.f, 1000.f });
    DDSprite.setPosition({ 750.f, 200.f });

    // --- Application State ---
    enum class AppState { Intro, Video, Menu, Lore, Game };
    AppState state = AppState::Intro;

    music.play();
    music2.play();

    // --- Main Loop ---
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Transition from Intro → Video
            if (event->is<sf::Event::KeyPressed>() && state == AppState::Intro) {
                state = AppState::Video;
                music.stop();
                music2.stop();
				musicmenu.play();
            }

            // Mouse clicks in Menu
            if (event->is<sf::Event::MouseButtonPressed>() && state == AppState::Menu) {
                auto mouse = sf::Mouse::getPosition(window);

                if (playSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) {
                    // --- Start the game ---
                    window.close(); // Replace later with actual game start
                }
                else if (loreSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) {
                    MessageBox::Show("Lore of Dungeon Descent:\nThe heroes ...", "Lore");
                }
                else if (quitSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) {
                    auto result = MessageBox::Show("Are you sure you want to quit?",
                        "Quit Confirmation", MessageBoxButtons::YesNo);
                    if (result == DialogResult::Yes) window.close();
                }
            }


            // Escape from Lore back to Menu
            if (state == AppState::Lore && event->is<sf::Event::KeyPressed>()) {
                state = AppState::Menu;
            }
        }

        window.clear();

        if (state == AppState::Intro) {
            // Intro animation
            if (!introFrames.empty() && clock.getElapsedTime().asSeconds() > introFrameDelay) {
                auto tex = introFrames.front();
                introFrames.pop();
                sprite.setTexture(*tex, true);
                applyLetterbox(sprite, tex);
                introFrames.push(tex);
                clock.restart();
            }
            window.draw(sprite);
            window.draw(text);
        }
        else if (state == AppState::Video || state == AppState::Menu) {
            // Video always plays in background
            if (videoClock.getElapsedTime().asSeconds() > videoFrameDelay) {
                currentVideoFrame = (currentVideoFrame + 1) % videoFrames.size();
                videoSprite.setTexture(*videoFrames[currentVideoFrame], true);

                // --- Stretch video to fill entire screen ---
                sf::Vector2u texSize = videoFrames[currentVideoFrame]->getSize();
                sf::Vector2u winSize = window.getSize();
                sf::IntRect rect;
				rect.size = sf::Vector2<int>(texSize.x, texSize.y);
                videoSprite.setTextureRect(rect);
                //videoSprite.setScale(sf::Vector2<float>((winSize.x) / texSize.x, (winSize.y) / texSize.y));
                videoSprite.setPosition({ 0.f, 0.f });
                videoClock.restart();
            }
            //videoSprite.scale({ 1.100f, 1.100f }); // Slight zoom effect
            window.draw(videoSprite);

            if (state == AppState::Menu) {
                window.draw(playSprite);
                window.draw(loreSprite);
                window.draw(quitSprite);
				window.draw(DDSprite);
            }
        }
        else if (state == AppState::Lore) {
            sf::Text loreScreen(font);
            loreScreen.setString("Lore of the Heroes\n(Press any key to return)");
            loreScreen.setCharacterSize(60);
            loreScreen.setPosition({ 300.f, 300.f });
            window.draw(loreScreen);
        }
        else if (state == AppState::Game) {
            sf::Text gameScreen(font);
            gameScreen.setString("Game Starts Here!");
            gameScreen.setCharacterSize(60);
            gameScreen.setPosition({ 300.f, 300.f });
            window.draw(gameScreen);
        }

        window.display();

        // Auto transition from Video to Menu (after first loop)
        if (state == AppState::Video && currentVideoFrame == videoFrames.size() - 1) {
            state = AppState::Menu;
        }
    }
}
