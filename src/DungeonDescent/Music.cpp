#include "SFML/Audio.hpp" // Include the SFML audio library.
#include <iostream>
#include "Music.h"

// Constructor for the Music class.
Music::Music() {
    sound.setVolume(30); // Sets the initial volume of the music to 30 (on a scale of 0 to 100).
}

// Destructor for the Music class.
Music::~Music() {

}

// Background music methods for different biomes.

// Plays the background music for the ice biome.
void Music::iceBiome() {
    // Attempts to open the audio file for the ice biome.
    if (!sound.openFromFile("music/icemusic.wav")) {
        // Outputs an error message to the console if the music file fails to load.
        std::cout << "sound failed to load into file" << std::endl;
    }
    // Starts playing the loaded music.
    sound.play();
    // Sets the music to loop indefinitely.
    sound.setLooping(true);
}

// Plays the background music for the jungle biome.
void Music::jungleBiome() {
    // Attempts to open the audio file for the jungle biome.
    if (!sound.openFromFile("music/junglemusic.wav")) {
        // Outputs an error message to the console if the music file fails to load.
        std::cout << "sound failed to load into file" << std::endl;
    }
    // Starts playing the loaded music.
    sound.play();
    // Sets the music to loop indefinitely.
    sound.setLooping(true);
}

// Plays the background music for the desert biome.
void Music::desertBiome() {
    // Attempts to open the audio file for the desert biome.
    if (!sound.openFromFile("music/desertmusic.wav")) {
        // Outputs an error message to the console if the music file fails to load.
        std::cout << "sound failed to load into file" << std::endl;
    }
    // Starts playing the loaded music.
    sound.play();
    // Sets the music to loop indefinitely.
    sound.setLooping(true);
}

// Plays the background music for the ghost biome.
void Music::ghostBiome() {
    // Attempts to open the audio file for the ghost biome.
    if (!sound.openFromFile("music/ghostmusic.wav")) {
        // Outputs an error message to the console if the music file fails to load.
        std::cout << "could not open music file" << std::endl;
    }
    // Starts playing the loaded music.
    sound.play();
    // Sets the music to loop indefinitely.
    sound.setLooping(true);
}

// Plays the background music for the lava biome.
void Music::lavaBiome() {
    // Attempts to open the audio file for the lava biome.
    if (!sound.openFromFile("music/lavamusic.wav")) {
        // Outputs an error message to the console if the music file fails to load.
        std::cout << "could not open music file" << std::endl;
    }
    // Starts playing the loaded music.
    sound.play();
    // Sets the music to loop indefinitely.
    sound.setLooping(true);
}

// Stops the currently playing music.
void Music::StopSound() {
    // Checks if the music is currently playing.
    if (sound.getStatus() == sf::Music::Status::Playing) {
        // Stops the music playback.
        sound.stop();
    }
}