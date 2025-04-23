#include "Library.h"
#include <iostream>

Library::Library() : RoomBase("Library") {

	BookExists = true;
	//Book type to be coded
	//NPC = insert
	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip = std::rand() % 2;

	// Output the result
	if (coinFlip == 0) {
		QAvail = true;
		QComp = false;
	}
	else {
		QAvail = false;
		QComp = true;
	}

	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip1 = std::rand() % 2;

	// Output the result
	if (coinFlip1 == 0) {
		KeyExists = true;
	}
	else {
		KeyExists = false;
	}

}

int Library::BookEff(int Type, int arr[6]) {

	//to code type effects

	return arr[6];
}

string Library::getImageFileName(int biome)
{
	if (biome == 0) {
		return "icelibrary.png";
	}
	else if (biome == 1) {
		return "junglelibrary.png";
	}
	else if (biome == 2) {
		return "desertlibrary.jpeg";
	}
	else if (biome == 3) {
		return "ghostlibrary.jpeg";
	}
	else if (biome == 4) {
		return "lavalibrary.jpg";
	}
}

string Library::getTextFileName(int biome)
{
	if (biome == 0) {
		return "icelibrary.txt";
	}
	else if (biome == 1) {
		return "junglelibrary.txt";
	}
	else if (biome == 2) {
		return "desertlibrary.txt";
	}
	else if (biome == 3) {
		return "ghostlibrary.txt";
	}
	else if (biome == 4) {
		return "lavalibrary.txt";
	}
}
