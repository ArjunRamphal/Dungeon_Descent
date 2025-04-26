#include "Shop.h"
#include <iostream>

Shop::Shop() : RoomBase("Shop") {

}

string Shop::getImageFileName(int biome)
{
	if (biome == 0) {
		return "iceshop.png";
	}
	else if (biome == 1) {
		return "jungleshop.png";
	}
	else if (biome == 2) {
		return "desertshop.jpeg";
	}
	else if (biome == 3) {
		return "ghostshop.jpeg";
	}
	else if (biome == 4) {
		return "lavashop.jpg";
	}
}

string Shop::getTextFileName(int biome)
{
	if ((biome == 0) || ((biome == 1))) {
		return "shopkeeper.txt";
	}
	else if (biome == 2) {
		return "desertshop.txt";
	}
	else if (biome == 3) {
		return "ghostshop.txt";
	}
	else if (biome == 4) {
		return "lavashop.txt";
	}
}

vector<string> Shop::getChoices(int roomCounter, int biome)
{
	if (roomCounter == 8)
		return vector<string>{"Health Potion", "Stat Increase Potion", "Book of Knowledge"};
	else if (roomCounter == 14) {
		if (biome == 2) {
			return vector<string>{"Draught of Revitalization", "Essence of Fortitude", "Scroll of Ancient Wisdom"};
		}
		else if (biome == 3) {
			return vector<string>{"Elixir of Restless Slumber", "Phantom’s Breath", "Scroll of Lost Echoes"};
		}
	}
	else {
		return vector<string>{"Emberheart Elixir", "Corebound Tincture", "Inferno Draught"};
	}
}

string Shop::getbtnChoice1(Character& character)
{
	character.incStats(4, 5);
	return character.incStatsDisplay(4, 5);
}

string Shop::getbtnChoice2(Character& character)
{
	if (character.getFloor() == 1) {
		for (int i = 0; i < 5; i++) {
			character.incStats(i, 2);
		}
		return "All stats have been increased by 2";
	}
	else if (character.getBiome() == 2){
		character.incStats(0, 2);
		return character.incStatsDisplay(0, 2);
	}
	else if (character.getBiome() == 3) {
		for (int i = 0; i < 5; i++) {
			character.incStats(i, 2);
		}
		return "All stats have been increased by 2";
	}
	else if (character.getFloor() == 3) {
		character.incStats(0, 2);
		return character.incStatsDisplay(0, 2);
	}
	
}

string Shop::getbtnChoice3(Character& character)
{
	if (character.getFloor() == 1) {
		character.incReputation();
		return " shopkeeperbook.txt";
	}
	else if (character.getFloor() == 2) {
		character.incStats(1, 2);
		return character.incStatsDisplay(1, 2);
	}
	else if (character.getFloor() == 3) {
		for (int i = 0; i < 5; i++) {
			character.incStats(i, 2);
		}
		return "All stats have been increased by 2";
	}
}
