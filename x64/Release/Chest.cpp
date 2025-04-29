#include "Chest.h"
#include <iostream>
#include <string>
#include <vector>

// Constructor for the Chest class. Calls the RoomBase constructor with the name "Chest".
Chest::Chest() : RoomBase("Chest") {
	// Set the type of this room to "Chest".
	type = "Chest";
}

// Returns the filename of the image to display for a Chest room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the chest image corresponding to the biome.
string Chest::getImageFileName(int biome)
{
	if (biome == 0) {
		return "images/biomes/ice/chest/icechest.png";
	}
	else if (biome == 1) {
		return "images/biomes/jungle/chest/junglechest.png";
	}
	else if (biome == 2) {
		return "images/biomes/desert/chest/desertchest.jpeg";
	}
	else if (biome == 3) {
		return "images/biomes/ghost/chest/ghostchest.jpeg";
	}
	else if (biome == 4) {
		return "images/biomes/lava/chest/lavachest.jpg";
	}
	return ""; // Return an empty string as a default or for error handling.
}

// Returns the filename of the text description to display for a Chest room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the text file describing the chest encounter in the given biome.
string Chest::getTextFileName(int biome)
{
	if (biome == 0) {
		return "textfiles/biomes/ice/chest/icechest.txt";
	}
	else if (biome == 1) {
		return "textfiles/biomes/jungle/chest/junglechest.txt";
	}
	else if (biome == 2) {
		return "textfiles/biomes/desert/chest/desertchest.txt";
	}
	else if (biome == 3) {
		return "textfiles/biomes/ghost/chest/ghostchest.txt";
	}
	else if (biome == 4) {
		return "textfiles/biomes/lava/chest/lavachest.txt";
	}
	return ""; // Return an empty string as a default or for error handling.
}

// Returns a vector of strings representing the choices available to the player in a Chest room.
// @param roomCounter The current room number.
// @param biome The current biome.
// @return A vector containing a single choice: "Open chest".
vector<string> Chest::getChoices(int roomCounter, int biome)
{
	return vector<string>{"Open chest"};
}

// Defines the outcome when the player chooses to "Open chest".
// @param character A reference to the Character object interacting with the chest.
// @return A string describing the result of opening the chest, including stat increases and additional text based on the room counter.
string Chest::getbtnChoice1(Character& character)
{
	// Increase all of the character's stats by 2.
	character.incStats(2);
	// Check if the player is in a specific room (room 15).
	if (character.getRoomCounter() == 15) {
		// Increase the character's reputation.
		character.incReputation();
		// Return a string describing the stat increase and a specific message for this room.
		return character.incStatsDisplay(2) + " textfiles/floor2chestopen.txt";
	}
	// Check if the player is in one of the lava biome specific rooms (21, 22, or 23).
	else if ((character.getRoomCounter() == 21) || (character.getRoomCounter() == 22) || (character.getRoomCounter() == 23)) {
		// Increase the character's reputation.
		character.incReputation();
		// Return a string describing the stat increase and a specific message for the lava biome.
		return character.incStatsDisplay(2) + " textfiles/biomes/lava/chest/lavachestopen.txt";
	}
	// For any other room, just return the message describing the stat increase.
	else {
		return character.incStatsDisplay(2);
	}
}