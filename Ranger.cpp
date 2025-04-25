#include "Ranger.h"
#include <iostream>

Ranger::Ranger() : Character("Ranger")
{
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
void Ranger::Ability() {
	statValue[5] += 3;
}