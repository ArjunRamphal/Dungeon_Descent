
#pragma once
#include "SFML/Audio.hpp"
#include <iostream>

class Music {
public:
	// Music objects
	sf::Music sound;

	Music();
	~Music();

	void iceBiome();
	void jungleBiome();
	void desertBiome();
	void ghostBiome();
	void lavaBiome();

	void StopSound();

};
