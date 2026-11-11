#include "Mage.h"
#include <iostream>
#include <string> // For string manipulation

// Constructor for the Mage class. Initializes a new Mage character with base stats and image filenames.
Mage::Mage() : Character("Mage")
{
    // Initialize the Mage's starting stat values.
    statValue[0] = 1; // Strength
    statValue[1] = 4; // Wisdom
    statValue[2] = 3; // Observation
    statValue[3] = 1; // Agility
    statValue[4] = 40; // Health
    statValue[5] = 1; // Accuracy

    // Set the filename for the Mage's profile picture.
    pfpImageName = "images/pfps/Mage.jpeg";
    // Set the filename for the Mage's ability description image.
    abilityFileName = "images/charAbilities/mageAbility.jpg";
}

// Overrides the Ability method from the Character class. Implements the Mage's special ability.
void Mage::Ability() {
    // Increase the Mage's Wisdom stat by 3.
    statValue[1] += 3;
}

// Overrides the incStats method from the Character class. Increases all stats for the Mage with a special modifier.
void Mage::incStats(float amount)
{
    // Calls the overloaded += operator of the Mage class to handle stat increases.
    *this += amount;
}

// Overrides the incStatsDisplay method from the Character class. Provides a Mage-specific message for stat increases.
string Mage::incStatsDisplay(float amount)
{
    // Calculate the increased amount, with a higher multiplier.
    string amountStr = to_string(1.5 * amount * floor);
    // Find the decimal point to format the output.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a message indicating the stat increase, with a special note for health.
    return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". Health increased by double.";
}

// Overrides the incXStat method from the Character class. Increases a specific stat for the Mage with a modifier.
void Mage::incXStat(int index, float amount)
{
    // Increase the specified stat by 1.5 times the given amount.
    statValue[index] = statValue[index] + 1.5 * amount;
}

// Overrides the incXStatDisplay method from the Character class. Provides a Mage-specific message for individual stat increases.
string Mage::incXStatDisplay(int index, float amount)
{
    // Calculate the increased amount.
    string amountStr = to_string(1.5 * amount);
    // Find the decimal point to format the output.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a message indicating which stat increased and by how much.
    return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

// Overrides the += operator from the Character class. Defines how stats are added to a Mage.
Character& Mage::operator += (float amount)
{
    // Iterate through all the stats.
    for (int i = 0; i < 6; i++) {
        // Increase health (index 4) by a larger factor.
        if (i == 4) {
            statValue[i] += 3 * amount * floor;
        }
        // Increase other stats by a different factor.
        else {
            statValue[i] += 1.5 * amount * floor;
        }
    }
    // Return a reference to the modified Mage object to allow for chaining.
    return *this;
}