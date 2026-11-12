#pragma once
#include "FloorOne.h"
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <deque>
#include <map>
#include <memory>
#include <optional>

namespace dungeon {
struct LaunchOptions {
    std::filesystem::path assets;
    bool audio = true;
    bool skipIntro = false;
    bool smoke = false;
    std::filesystem::path screenshot;
};
class GameApp {
    friend class GameAppTests;
public:
    explicit GameApp(LaunchOptions options);
    int run();
private:
    enum class Screen { Intro, Menu, Introduction, Heroes, Biomes, Game, Lore };
    enum class Overlay { None, Pause, Map, Quit };
    struct Button { sf::FloatRect bounds; std::string id; bool enabled; };
    void openWindow(bool fullscreen);
    void handle(const sf::Event& event);
    void action(const std::string& id);
    void update(float dt);
    void render();
    void drawMenu();
    void drawHeroes();
    void drawBiomes();
    void drawGame();
    void drawOverlay();
    void drawLore();
    void panel(sf::FloatRect rect, sf::Color fill, sf::Color outline);
    void label(const std::string& text, sf::Vector2f position, unsigned size = 20,
               sf::Color color = sf::Color::White, bool heading = false);
    void button(const std::string& id, const std::string& text, sf::FloatRect rect, bool enabled = true);
    void image(const std::string& name, sf::FloatRect rect, bool cover = false, sf::Color tint = sf::Color::White);
    void paragraph(const std::string& text, sf::FloatRect rect, unsigned size = 19);
    void reading(const std::string& text, sf::FloatRect rect);
    void bar(float ratio, sf::FloatRect rect, sf::Color color);
    void playMusic(const std::string& file);
    void enter(Screen screen);
    void syncStory();
    std::vector<sf::String> wrap(const std::string& text, unsigned size, float width);
    std::optional<sf::Vector2f> pointer(sf::Vector2i pixel) const;
    sf::Texture& texture(const std::string& name);
    static sf::String utf8(const std::string& text);
    LaunchOptions options_;
    FloorOne floor_;
    sf::RenderWindow window_;
    sf::RenderTexture canvas_;
    sf::Font bodyFont_, titleFont_;
    std::map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::deque<std::string> textureOrder_;
    std::unique_ptr<sf::Music> music_;
    std::string musicFile_;
    bool muted_ = false, fullscreen_ = false, focused_ = true;
    Screen screen_ = Screen::Intro, loreReturn_ = Screen::Menu;
    Overlay overlay_ = Overlay::None;
    Hero selectedHero_ = Hero::Warrior, loreHero_ = Hero::Warrior;
    std::vector<Button> buttons_;
    std::size_t selectedButton_ = 0;
    std::string readingText_;
    std::vector<sf::String> readingLines_;
    int page_ = 0, pageCount_ = 1;
    float animationTime_ = 0, attackDelay_ = 0, flash_ = 0;
    std::string roomStory_;
    int storyRoom_ = -1;
    std::uint32_t seed_ = 0;
    unsigned smokeFrames_ = 0;
};
}
