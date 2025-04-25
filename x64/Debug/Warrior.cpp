#include "Warrior.h"
#include <iostream>

Warrior::Warrior() : Character("Warrior")
{
	baseHealth = 15;
	statValue[0] = 4;
	statValue[1] = 1;
	statValue[2] = 0;
	statValue[3] = 3;
	statValue[4] = 15;
	statValue[5] = 2;

	Lastfpwrupused = 0;

	pfpImageName = "Warrior.jpg";
	abilityFileName = "warriorAbility.jpg";
}

void Warrior::Ability() {
	statValue[0] = (2 * statValue[0]) + 3;
	statValue[3] *= 2;
	statValue[4] *= 2;
}