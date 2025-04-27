#include "Rogue.h"
#include <iostream>

Rogue::Rogue() : Character("Rogue")
{
	statValue[0] = 2;
	statValue[1] = 0;
	statValue[2] = 4;
	statValue[3] = 5;
	statValue[4] = 30;
	statValue[5] = 5;

	pfpImageName = "Rogue.jpg";
	abilityFileName = "rogueAbility.jpg";
}

void Rogue::Ability() {
	statValue[3] += 3;
}

void Rogue::incStats(float amount)
{
	*this += amount;
}

string Rogue::incStatsDisplay(float amount)
{
	string amountStr = to_string(amount * floor);
	size_t decimalPos = amountStr.find_first_of(".");
	return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". 1.5x multiplier for Health and Agility.";
}

void Rogue::incXStat(int index, float amount)
{
	if (index == 3) {
		statValue[index] = statValue[index] + 1.5 * amount;
	}
	else {
		statValue[index] = statValue[index] + amount;
	}
}

string Rogue::incXStatDisplay(int index, float amount)
{
	if (index == 3) {
		string amountStr = to_string(1.5 * amount);
		size_t decimalPos = amountStr.find_first_of(".");
		return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
	}
	else {
		string amountStr = to_string(amount);
		size_t decimalPos = amountStr.find_first_of(".");
		return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
	}
}

Character& Rogue::operator+=(float amount)
{
	for (int i = 0; i < 6; i++) {
		if ((i == 3) || (i == 4)) {
			statValue[i] += 1.5 * amount * floor;
		}
		else {
			statValue[i] += amount * floor;
		}
	}
	return *this; // Return a reference to the modified object
}
