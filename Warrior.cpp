#include "Warrior.h"
#include <iostream>

Warrior::Warrior() : Character("Warrior")
{
	statValue[0] = 4;
	statValue[1] = 1;
	statValue[2] = 0;
	statValue[3] = 3;
	statValue[4] = 25;
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

void Warrior::incStats(int index, int amount)
{
	if (index == 0) {
		statValue[index] += 2 * amount;
	}
	else {
		statValue[index] += amount;
	}
}

string Warrior::incStatsDisplay(int index, int amount)
{
	if (index == 0) {
		return getStatName(index) + " has increased by " + to_string(2 * amount) + ".";;
	}
	else {
		return getStatName(index) + " has increased by " + to_string(amount) + ".";
	}
}
