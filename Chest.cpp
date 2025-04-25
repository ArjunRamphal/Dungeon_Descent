#include "Chest.h"
#include <iostream>

Chest::Chest() : RoomBase("Chest") {

	//NPC = insert
	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip1 = std::rand() % 2;

	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip = std::rand() % 2;

	// Output the result
	if (coinFlip == 0) {
		Locked = true;
	}
	else {
		Locked = false;
	}
	type = "Chest";
}

void Chest::Open() {
	//to code effects and gain
}

string Chest::getImageFileName(int biome)
{
	if (biome == 0) {
		return "icechest.png";
	}
	else if (biome == 1) {
		return "junglechest.png";
	}
	else if (biome == 2) {
		return "desertchest.jpeg";
	}
	else if (biome == 3) {
		return "ghostchest.jpeg";
	}
	else if (biome == 4) {
		return "lavachest.jpg";
	}
}

string Chest::getTextFileName(int biome)
{
	if (biome == 0) {
		return "icechest.txt";
	}
	else if (biome == 1) {
		return "junglechest.txt";
	}
	else if (biome == 2) {
		return "desertchest.txt";
	}
	else if (biome == 3) {
		return "ghostchest.txt";
	}
	else if (biome == 4) {
		return "lavachest.txt";
	}
}

vector<string> Chest::getChoices(int roomCounter, int biome)
{
	return vector<string>{"Open chest"};
}

string Chest::getbtnChoice1(Character& character)
{
	srand(time(0));
	int randomNum = rand() % 6;
	character.incStats(randomNum, 2);
	if ((character.getRoomCounter() == 21) || (character.getRoomCounter() == 22) || (character.getRoomCounter() == 23)) {
		character.incReputation();
		return character.incStatsDisplay(randomNum, 2) + " lavachestopen.txt";
	}
	else {
		return character.incStatsDisplay(randomNum, 2);
	}
	
}
