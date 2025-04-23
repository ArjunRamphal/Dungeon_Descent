#include "Mage.h"
#include <iostream>

Mage::Mage() : Character("Mage")
{
	baseHealth = 15;
	statValue[0] = 1;
	statValue[1] = 4;
	statValue[2] = 3;
	statValue[3] = 1;
	statValue[4] = 15;
	statValue[5] = 1;

	Lastfpwrupused = 0;

	pfpImageName = "Mage.jpeg";
	abilityFileName = "mageAbility.jpg";
}

void Mage::Ability() {
	for (int i = 0; i < 6; i++) {
		statValue[i] = 2 * statValue[i];
	}
}