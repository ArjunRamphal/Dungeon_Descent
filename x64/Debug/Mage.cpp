#include "Mage.h"
#include <iostream>

Mage::Mage() : Character("Mage")
{
	statValue[0] = 1;
	statValue[1] = 4;
	statValue[2] = 3;
	statValue[3] = 1;
	statValue[4] = 30;
	statValue[5] = 1;

	pfpImageName = "Mage.jpeg";
	abilityFileName = "mageAbility.jpg";
}

void Mage::Ability() {
	QuestionTime += 3;
}

void Mage::incStats(int index, int amount)  
{  
   statValue[index] += 2 * amount;  
}

string Mage::incStatsDisplay(int index, int amount)
{
   return getStatName(index) + " has increased by " + to_string(2 * amount) + ".";
}
