#include "Warrior.h"
#include <iostream>
#include <string> // For string manipulation

// Constructor for the Warrior class. Initializes a new Warrior character with base stats and image filenames.
Warrior::Warrior() : Character("Warrior")
{
    // Initialize the Warrior's starting stat values.
    statValue[0] = 4; // Strength
    statValue[1] = 1; // Wisdom
    statValue[2] = 0; // Observation
    statValue[3] = 3; // Agility
    statValue[4] = 35; // Health
    statValue[5] = 2; // Accuracy

    // Set the filename for the Warrior's profile picture.
    pfpImageName = "images/pfps/Warrior.jpg";
    // Set the filename for the Warrior's ability description image.
    abilityFileName = "images/charAbilities/warriorAbility.jpg";
}

// Overrides the Ability method from the Character class. Implements the Warrior's special ability.
void Warrior::Ability() {
    // Increase the Warrior's Strength stat by 3 when the ability is used.
    statValue[0] += 3;
}

// Overrides the incStats method from the Character class. Increases all stats for the Warrior with a special multiplier for Health and Strength.
void Warrior::incStats(float amount)
{
    // Calls the overloaded += operator of the Warrior class to handle stat increases.
    *this += amount;
}

// Overrides the incStatsDisplay method from the Character class. Provides a Warrior-specific message for stat increases.
string Warrior::incStatsDisplay(float amount)
{
    // Calculate the base increase amount scaled by the current floor.
    string amountStr = to_string(amount * floor);
    // Find the decimal point to format the output.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a message indicating the stat increase, with a special note for Health and Strength.
    return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". 1.5x multiplier for Health and Strength.";
}

// Overrides the incXStat method from the Character class. Increases a specific stat for the Warrior, with a multiplier for Strength.
void Warrior::incXStat(int index, float amount)
{
    // Apply a 1.5x multiplier if the stat being increased is Strength (index 0).
    if (index == 0) {
        statValue[index] = statValue[index] + 1.5 * amount;
    }
    // Otherwise, increase the stat by the base amount.
    else {
        statValue[index] = statValue[index] + amount;
    }
}

// Overrides the incXStatDisplay method from the Character class. Provides a Warrior-specific message for individual stat increases, with a note for Strength.
string Warrior::incXStatDisplay(int index, float amount)
{
    // If the increased stat is Strength, display the modified amount.
    if (index == 0) {
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

// Overrides the += operator from the Character class. Defines how stats are added to a Warrior, with a 1.5x multiplier for Health and Strength.
Character& Warrior::operator+=(float amount)
{
    // Iterate through all the stats.
    for (int i = 0; i < 6; i++) {
        // Apply a 1.5x multiplier if the stat is Strength (index 0) or Health (index 4).
        if ((i == 0) || (i == 4)) {
            statValue[i] += 1.5 * amount * floor;
        }
        // Otherwise, increase the stat by the base amount scaled by the floor.
        else {
            statValue[i] += amount * floor;
        }
    }
    // Return a reference to the modified Warrior object to allow for chaining.
    return *this;
}