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

	pfpImageName = "Warrior.jpg";
	abilityFileName = "warriorAbility.jpg";
}

void Warrior::Ability() {
	statValue[0] = (2 * statValue[0]) + 3;
	statValue[3] *= 2;
	statValue[4] *= 2;
}

void Warrior::incStats(float amount)
{
	*this += amount;
}

string Warrior::incStatsDisplay(float amount)
{
	string amountStr = to_string(amount * floor);
	size_t decimalPos = amountStr.find_first_of(".");
	return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". Health and Strength increased by double.";
}

void Warrior::incXStat(int index, float amount)
{
	if (index == 0) {
		statValue[index] = statValue[index] + 2 * amount;
	}
	else {
		statValue[index] = statValue[index] + amount;
	}
}

string Warrior::incXStatDisplay(int index, float amount)
{
	if (index == 0) {
		string amountStr = to_string(2 * amount);
		size_t decimalPos = amountStr.find_first_of(".");
		return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
	}
	else {
		string amountStr = to_string(amount);
		size_t decimalPos = amountStr.find_first_of(".");
		return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
	}
}

Character& Warrior::operator+=(float amount)
{
	for (int i = 0; i < 6; i++) {
		if ((i == 0) || (i == 4)) {
			statValue[i] += 2 * amount * floor;
		}
		else {
			statValue[i] += amount * floor;
		}
	}
	return *this; // Return a reference to the modified object
}
