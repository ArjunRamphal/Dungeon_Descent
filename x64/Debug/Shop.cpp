#include "Shop.h"
#include <iostream>

Shop::Shop() : RoomBase("Shop") {
	int Stock[3];
	std::string StockNames[3] = { "Food","Keys","PlaceHolder" };
	std::srand(std::time(0));
	for (int i = 0; i < 3; i++) {
		Stock[i] = (std::rand() % 5) + 1;
	}

	//NPC = insert
	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip1 = std::rand() % 2;

	// Output the result
	if (coinFlip1 == 0) {
		QAvail = true;
	}
	else {
		QAvail = false;
	}

	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip = std::rand() % 2;

	// Output the result
	if (coinFlip == 0) {
		KeyExists = true;
	}
	else {
		KeyExists = false;
	}

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
