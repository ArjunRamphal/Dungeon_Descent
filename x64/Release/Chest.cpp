#include "Chest.h"
#include <iostream>

Chest::Chest() : RoomBase("Chest") {
	type = "Chest";
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
	character.incStats(2);
	if (character.getRoomCounter() == 15) {
		character.incReputation();
		return character.incStatsDisplay(2) + " floor2chestopen.txt";
	}
	else if ((character.getRoomCounter() == 21) || (character.getRoomCounter() == 22) || (character.getRoomCounter() == 23)) {
		character.incReputation();
		return character.incStatsDisplay(2) + " lavachestopen.txt";
	}
	else {
		return character.incStatsDisplay(2);
	}
}
