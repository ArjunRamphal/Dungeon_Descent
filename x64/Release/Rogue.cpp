#include "Rogue.h"
#include <iostream>

Rogue::Rogue() : Character("Rogue")
{
	statValue[0] = 2;
	statValue[1] = 0;
	statValue[2] = 4;
	statValue[3] = 5;
	statValue[4] = 20;
	statValue[5] = 5;

	Lastfpwrupused = 0;

	pfpImageName = "Rogue.jpg";
	abilityFileName = "rogueAbility.jpg";
}

void Rogue::Ability() {
	Hidden += 3;

	//toCode LockPick and GoldGain
}

void Rogue::incStats(int index, int amount)
{
	if (index == 3) {
		statValue[index] += 2 * amount;
	}
	else {
		statValue[index] += amount;
	}
}

string Rogue::incStatsDisplay(int index, int amount)
{
	if (index == 3) {
		return getStatName(index) + " has increased by " + to_string(2 * amount) + ".";;
	}
	else {
		return getStatName(index) + " has increased by " + to_string(amount) + ".";
	}
}
