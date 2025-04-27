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

void Ranger::incStats(float amount)
{
	*this += amount;
}

string Ranger::incStatsDisplay(float amount)
{
	string amountStr = to_string(amount * floor);
	size_t decimalPos = amountStr.find_first_of(".");
	return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". Health and Accuracy increased by double.";
}

void Ranger::incXStat(int index, float amount)
{
	if (index == 5) {
		statValue[index] = statValue[index] + 2 * amount;
	}
	else {
		statValue[index] = statValue[index] + amount;
	}
}

string Ranger::incXStatDisplay(int index, float amount)
{
	if (index == 5) {
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

Character& Ranger::operator+=(float amount)
{
	for (int i = 0; i < 6; i++) {
		if ((i == 5) || (i == 4)) {
			statValue[i] += 2 * amount * floor;
		}
		else {
			statValue[i] += amount * floor;
		}
	}
	return *this; // Return a reference to the modified object
}
