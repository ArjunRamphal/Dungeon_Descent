#include "Ranger.h"
#include <iostream>

Ranger::Ranger() : Character("Ranger")
{
	Accuracy = 5;
	baseHealth = 10;
	statValue[0] = 2;
	statValue[1] = 2;
	statValue[2] = 4;
	statValue[3] = 2;
	statValue[4] = 10;
	statValue[5] = 5;

	Lastfpwrupused = 0;

	pfpImageName = "Ranger.jpeg";
	abilityFileName = "rangerAbility.jpg";
}
bool Ranger::Ability() {
	Accuracy += 3;

	std::srand(std::time(0));

	// Generate a random value: 0 for Heads, 1 for Tails
	int coinFlip = std::rand() % 2;

	// Output the result
	if (coinFlip == 0) {
		return true;
	}
	else {
		return false;
	}

}