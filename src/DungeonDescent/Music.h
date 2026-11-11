#pragma once
#include "SFML/Audio.hpp" // Include the SFML audio library for music functionality.
#include <iostream>

class Music {
public:
    // sf::Music object to handle background music playback.
    sf::Music sound;

    // Default constructor for the Music class.
    Music();

    // Virtual destructor for the Music class. Ensures proper cleanup of resources, especially if the class is inherited from.
    virtual ~Music();

    // Method to play the music associated with the ice biome.
    void iceBiome();

    // Method to play the music associated with the jungle biome.
    void jungleBiome();

    // Method to play the music associated with the desert biome.
    void desertBiome();

    // Method to play the music associated with the ghost biome.
    void ghostBiome();

    // Method to play the music associated with the lava biome.
    void lavaBiome();

    // Method to stop any currently playing music.
    void StopSound();
};