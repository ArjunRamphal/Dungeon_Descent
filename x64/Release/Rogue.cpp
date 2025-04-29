#include "Rogue.h"
#include <iostream>
#include <string> // For string manipulation

// Constructor for the Rogue class. Initializes a new Rogue character with base stats and image filenames.
Rogue::Rogue() : Character("Rogue")
{
    // Initialize the Rogue's starting stat values.
    statValue[0] = 2; // Strength
    statValue[1] = 0; // Wisdom
    statValue[2] = 4; // Observation
    statValue[3] = 5; // Agility
    statValue[4] = 30; // Health
    statValue[5] = 5; // Accuracy

    // Set the filename for the Rogue's profile picture.
    pfpImageName = "images/pfps/Rogue.jpg";
    // Set the filename for the Rogue's ability description image.
    abilityFileName = "images/charAbilities/rogueAbility.png";
}

// Overrides the Ability method from the Character class. Implements the Rogue's special ability.
void Rogue::Ability() {
    // Increase the Rogue's Agility stat by 3 when the ability is used.
    statValue[3] += 3;
}

// Overrides the incStats method from the Character class. Increases all stats for the Rogue with a special modifier for Health and Agility.
void Rogue::incStats(float amount)
{
    // Calls the overloaded += operator of the Rogue class to handle stat increases.
    *this += amount;
}

// Overrides the incStatsDisplay method from the Character class. Provides a Rogue-specific message for stat increases.
string Rogue::incStatsDisplay(float amount)
{
    // Calculate the base increase amount scaled by the current floor.
    string amountStr = to_string(amount * floor);
    // Find the decimal point to format the output.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a message indicating the stat increase, with a special note for Health and Agility.
    return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". 1.5x multiplier for Health and Agility.";
}

// Overrides the incXStat method from the Character class. Increases a specific stat for the Rogue, with a modifier for Agility.
void Rogue::incXStat(int index, float amount)
{
    // Apply a 1.5x multiplier if the stat being increased is Agility (index 3).
    if (index == 3) {
        statValue[index] = statValue[index] + 1.5 * amount;
    }
    // Otherwise, increase the stat by the base amount.
    else {
        statValue[index] = statValue[index] + amount;
    }
}

// Overrides the incXStatDisplay method from the Character class. Provides a Rogue-specific message for individual stat increases, with a note for Agility.
string Rogue::incXStatDisplay(int index, float amount)
{
    // If the increased stat is Agility, display the modified amount.
    if (index == 3) {
        string amountStr = to_string(1.5 * amount);
        size_t decimalPos = amountStr.find_first_of(".");
        return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
    }
    // Otherwise, display the base increase amount.
    else {
        string amountStr = to_string(amount);
        size_t decimalPos = amountStr.find_first_of(".");
        return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
    }
}

// Overrides the += operator from the Character class. Defines how stats are added to a Rogue, with a 1.5x multiplier for Health and Agility.
Character& Rogue::operator+=(float amount)
{
    // Iterate through all the stats.
    for (int i = 0; i < 6; i++) {
        // Apply a 1.5x multiplier if the stat is Agility (index 3) or Health (index 4).
        if ((i == 3) || (i == 4)) {
            statValue[i] += 1.5 * amount * floor;
        }
        // Otherwise, increase the stat by the base amount scaled by the floor.
        else {
            statValue[i] += amount * floor;
        }
    }
    // Return a reference to the modified Rogue object to allow for chaining.
    return *this;
}