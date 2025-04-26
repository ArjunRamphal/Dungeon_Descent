#include "Ranger.h"
#include <iostream>

Ranger::Ranger() : Character("Ranger")
{
	statValue[0] = 2;
	statValue[1] = 2;
	statValue[2] = 4;
	statValue[3] = 2;
	statValue[4] = 20;
	statValue[5] = 5;

	pfpImageName = "Ranger.jpeg";
	abilityFileName = "rangerAbility.jpg";
}
void Ranger::Ability() {
	statValue[5] += 3;
}

void Ranger::incStats(int index, int amount)
{
	if (index == 5) {
		statValue[index] += 2 * amount;
	}
	else {
		statValue[index] += amount;
	}
}

string Ranger::incStatsDisplay(int index, int amount)
{
	if (index == 5) {
		return getStatName(index) + " has increased by " + to_string(2 * amount) + ".";;
	}
	else {
		return getStatName(index) + " has increased by " + to_string(amount) + ".";
	}
}
