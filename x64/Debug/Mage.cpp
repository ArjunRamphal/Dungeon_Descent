#include "Mage.h"
#include <iostream>

Mage::Mage() : Character("Mage")
{
	statValue[0] = 1;
	statValue[1] = 4;
	statValue[2] = 3;
	statValue[3] = 1;
	statValue[4] = 40;
	statValue[5] = 1;

	pfpImageName = "Mage.jpeg";
	abilityFileName = "mageAbility.jpg";
}

void Mage::Ability() {
	statValue[1] += 3;
}

void Mage::incStats(float amount)  
{  
	*this += amount;
}

string Mage::incStatsDisplay(float amount)
{
	string amountStr = to_string(1.5 * amount * floor);
	size_t decimalPos = amountStr.find_first_of(".");
	return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". Health increased by double.";
}

void Mage::incXStat(int index, float amount)
{
	statValue[index] = statValue[index] + 1.5 * amount;
}

string Mage::incXStatDisplay(int index, float amount)
{
	string amountStr = to_string(1.5 * amount);
	size_t decimalPos = amountStr.find_first_of(".");
	return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

Character& Mage::operator += (float amount)
{
	for (int i = 0; i < 6; i++) {
		if (i == 4) {
			statValue[i] += 3 * amount * floor;
		}
		else {
			statValue[i] += 1.5 * amount * floor;
		}
	}
	return *this; // Return a reference to the modified object
}
