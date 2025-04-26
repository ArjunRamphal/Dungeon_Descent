#include "SFML/Audio.hpp"
#include <iostream>
#include "Music.h"

Music::Music() {
	sound.setVolume(30);	// Sets volume
}

Music::~Music() {

}

// Background music

void Music::iceBiome() {
	if (!sound.openFromFile("icemusic.wav")) {  //opens music file
		std::cout << "sound failed to load into file" << std::endl;
	}
	sound.play();
	sound.setLooping(true);
}

void Music::jungleBiome() {
	if (!sound.openFromFile("junglemusic.wav")) {  //opens music file
		std::cout << "sound failed to load into file" << std::endl;
	}
	sound.play();
	sound.setLooping(true);
}

void Music::desertBiome() {
	if (!sound.openFromFile("desertmusic.wav")) {  //opens music file
		std::cout << "sound failed to load into file" << std::endl;
	}
	sound.play();
	sound.setLooping(true);
}

void Music::ghostBiome() {
	if (!sound.openFromFile("ghostmusic.wav")) {  //opens music file
		std::cout << "could not open music file" << std::endl;
	}
	sound.play();
	sound.setLooping(true);
}

void Music::lavaBiome() {
	if (!sound.openFromFile("lavamusic.wav")) {  //opens music file
		std::cout << "could not open music file" << std::endl;
	}
	sound.play();
	sound.setLooping(true);
}

// Stops music
void Music::StopSound() {
	if (sound.getStatus() == sf::Music::Status::Playing) {
		sound.stop();
	}
}