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

    // Corrected loop
    while (loadingClock.getElapsedTime().asSeconds() < 1.0f) {
        // Modern SFML event handling with std::optional
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

    auto applyLetterboxVideo = [&](sf::Sprite& spr, std::shared_ptr<sf::Texture> tex) {
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

    applyLetterboxVideo(videoSprite, videoFrames.front());

    // --- Application State ---
    enum class AppState { Intro, Video, Form };
    AppState state = AppState::Intro;

    music.play();
    music2.play();

    // --- Main Loop ---
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (event->is<sf::Event::KeyPressed>() && state == AppState::Intro) {
                state = AppState::Video; // transition to video
				music.stop();
                music2.stop();
            }
        }


        window.clear();

        if (state == AppState::Intro) {
            // Update intro animation
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
        else if (state == AppState::Video) {
            if (videoClock.getElapsedTime().asSeconds() > videoFrameDelay) {
                currentVideoFrame = (currentVideoFrame + 1) % videoFrames.size();
                videoSprite.setTexture(*videoFrames[currentVideoFrame], true);
                applyLetterboxVideo(videoSprite, videoFrames[currentVideoFrame]);
                videoClock.restart();
            }
            window.draw(videoSprite);
        }

        window.display();
    }

    // --- Run Windows Form after window closes ---
    form->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
    form->WindowState = System::Windows::Forms::FormWindowState::Maximized;
    //Application::Run(form);
}
