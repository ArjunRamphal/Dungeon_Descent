// --- SFML Includes ---
#include <queue>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
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
    if (!loadingTex.loadFromFile("images/loadscreen.png")) return -1;
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
        spr.setPosition({ 0.f, 0.f });
        };
    applyFullscreenVideo(videoSprite, videoFrames.front());

    // --- Menu Setup ---
    sf::Texture playTex, loreTex, quitTex, DDTex;
    if (!playTex.loadFromFile("images/menu/start.png")) return -1;
    if (!loreTex.loadFromFile("images/menu/lore.png")) return -1;
    if (!quitTex.loadFromFile("images/menu/quit.png")) return -1;
    if (!DDTex.loadFromFile("images/menu/title.png")) return -1;

    sf::Sprite playSprite(playTex), loreSprite(loreTex), quitSprite(quitTex), DDSprite(DDTex);

    playSprite.setPosition({ 1100.f, 600.f });
    loreSprite.setPosition({ 1100.f, 800.f });
    quitSprite.setPosition({ 1100.f, 1000.f });
    DDSprite.setPosition({ 750.f, 200.f });

    // --- Textbox Setup ---
    sf::Texture textboxTex;
    textboxTex.loadFromFile("images/scroll-Photoroombig-Photoroom.png");
    sf::Sprite textboxSprite(textboxTex);
    textboxSprite.setPosition(
        sf::Vector2f(
            static_cast<float>((window.getSize().x) / 2.f) - textboxTex.getSize().x / 2.f,
            static_cast<float>(window.getSize().y) - textboxTex.getSize().y + 25
        )
    );

    sf::Text textboxText(font);
    textboxText.setCharacterSize(20);
    textboxText.setFillColor(sf::Color::White);
    textboxText.setPosition({ textboxSprite.getPosition().x + 250.f, textboxSprite.getPosition().y + 150.f });

    auto loadTextFromFile = [&](const std::string& filename) {
        std::ifstream file(filename);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

    // --- Button Setup ---
    sf::Texture nextButtonTex;
    if (!nextButtonTex.loadFromFile("images/continue.png")) return -1;
    sf::Sprite nextButtonSprite(nextButtonTex);
    nextButtonSprite.setPosition(
        sf::Vector2f(
            static_cast<float>(window.getSize().x) / 2.f - static_cast<float>(nextButtonTex.getSize().x) / 2.f,
            static_cast<float>(window.getSize().y) - static_cast<float>(nextButtonTex.getSize().y) - 30.f
        )
    );
    bool showNextButton = false;

    // --- Split full text into chunks ---
    std::vector<std::string> textChunks;
    auto splitTextIntoChunks = [&](const std::string& fullText, size_t maxLineLength, size_t maxLines) {
        std::stringstream ss(fullText);
        std::string line;
        std::string currentChunk;
        size_t lineCount = 0;

        while (std::getline(ss, line)) {
            size_t start = 0;
            while (start < line.length()) {
                size_t len = std::min(maxLineLength, line.length() - start);
                size_t end = start + len;

                // Try to break at last space
                if (end < line.length()) {
                    size_t spacePos = line.rfind(' ', end);
                    if (spacePos != std::string::npos && spacePos > start) {
                        len = spacePos - start;
                    }
                }

                std::string chunkLine = line.substr(start, len);
                // Remove leading space if it exists
                if (!chunkLine.empty() && chunkLine[0] == ' ')
                    chunkLine.erase(0, 1);

                currentChunk += chunkLine + '\n';
                start += len;

                lineCount++;
                if (lineCount >= maxLines) {
                    textChunks.push_back(currentChunk);
                    currentChunk.clear();
                    lineCount = 0;
                }
            }

            if (line.empty()) {
                currentChunk += '\n';
                lineCount++;
                if (lineCount >= maxLines) {
                    textChunks.push_back(currentChunk);
                    currentChunk.clear();
                    lineCount = 0;
                }
            }
        }

        if (!currentChunk.empty()) textChunks.push_back(currentChunk);
    };


    std::string fullStoryText = loadTextFromFile("textfiles/introduction/introduction.txt");
    splitTextIntoChunks(fullStoryText, 82, 12);

    size_t currentChunkIndex = 0;
    textboxText.setString(textChunks[currentChunkIndex]);
    showNextButton = (textChunks.size() > 1);

    // --- Application State ---
    enum class AppState { Intro, Video, Menu, Lore, Game };
    AppState state = AppState::Intro;

    music.play();
    music2.play();

    // --- Main Loop ---
    while (window.isOpen()) {
        // --- Event Handling ---
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (event->is<sf::Event::KeyPressed>() && state == AppState::Intro) {
                state = AppState::Video;
                music.stop();
                music2.stop();
                musicmenu.play();
            }

            if (event->is<sf::Event::MouseButtonPressed>()) {
                auto mouse = sf::Mouse::getPosition(window);

                if (state == AppState::Menu) {
                    if (playSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) state = AppState::Game;
                    else if (loreSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) state = AppState::Lore;
                    else if (quitSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) {
                        if (MessageBox::Show("Are you sure you want to quit?", "Quit Confirmation", MessageBoxButtons::YesNo) == DialogResult::Yes)
                            window.close();
                    }
                }
                else if (state == AppState::Game && showNextButton) {
                    if (nextButtonSprite.getGlobalBounds().contains(sf::Vector2f(mouse.x, mouse.y))) {
                        currentChunkIndex++;
                        if (currentChunkIndex >= textChunks.size()) currentChunkIndex = textChunks.size() - 1;
                        textboxText.setString(textChunks[currentChunkIndex]);
                        showNextButton = (currentChunkIndex < textChunks.size() - 1);
                    }
                }
            }

            if (state == AppState::Lore && event->is<sf::Event::KeyPressed>()) state = AppState::Menu;
        }

        // --- Rendering ---
        window.clear();

        if (state == AppState::Intro) {
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
            if (videoClock.getElapsedTime().asSeconds() > videoFrameDelay) {
                currentVideoFrame = (currentVideoFrame + 1) % videoFrames.size();
                videoSprite.setTexture(*videoFrames[currentVideoFrame], true);
                videoClock.restart();
            }
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
            window.draw(textboxSprite);
            window.draw(textboxText);
            if (showNextButton) window.draw(nextButtonSprite);
        }

        window.display();

        if (state == AppState::Video && currentVideoFrame == videoFrames.size() - 1) state = AppState::Menu;
    }
}
