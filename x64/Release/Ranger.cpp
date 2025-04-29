#include "Ranger.h"
#include <iostream>
#include <string> // For string manipulation

// Constructor for the Ranger class. Initializes a new Ranger character with base stats and image filenames.
Ranger::Ranger() : Character("Ranger")
{
    // Initialize the Ranger's starting stat values.
    statValue[0] = 2; // Strength
    statValue[1] = 2; // Wisdom
    statValue[2] = 4; // Observation
    statValue[3] = 2; // Agility
    statValue[4] = 30; // Health
    statValue[5] = 5; // Accuracy

    // Set the filename for the Ranger's profile picture.
    pfpImageName = "images/pfps/Ranger.jpeg";
    // Set the filename for the Ranger's ability description image.
    abilityFileName = "images/charAbilities/rangerAbility.jpg";
}

// Overrides the Ability method from the Character class. Implements the Ranger's special ability.
void Ranger::Ability() {
    // Increase the Ranger's Accuracy stat by 3 when the ability is used.
    statValue[5] += 3;
}

// Overrides the incStats method from the Character class. Increases all stats for the Ranger with a special modifier for Health and Accuracy.
void Ranger::incStats(float amount)
{
    // Calls the overloaded += operator of the Ranger class to handle stat increases.
    *this += amount;
}

// Overrides the incStatsDisplay method from the Character class. Provides a Ranger-specific message for stat increases.
string Ranger::incStatsDisplay(float amount)
{
    // Calculate the base increase amount.
    string amountStr = to_string(amount * floor);
    // Find the decimal point to format the output.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a message indicating the stat increase, with a special note for Health and Accuracy.
    return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ". 1.5x multiplier for Health and Accuracy.";
}

// Overrides the incXStat method from the Character class. Increases a specific stat for the Ranger, with a modifier for Accuracy.
void Ranger::incXStat(int index, float amount)
{
    // Apply a 1.5x multiplier if the stat being increased is Accuracy (index 5).
    if (index == 5) {
        statValue[index] = statValue[index] + 1.5 * amount;
    }
    // Otherwise, increase the stat by the base amount.
    else {
        statValue[index] = statValue[index] + amount;
    }
}

// Overrides the incXStatDisplay method from the Character class. Provides a Ranger-specific message for individual stat increases, with a note for Accuracy.
string Ranger::incXStatDisplay(int index, float amount)
{
    // If the increased stat is Accuracy, display the modified amount.
    if (index == 5) {
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

// Overrides the += operator from the Character class. Defines how stats are added to a Ranger, with a 1.5x multiplier for Health and Accuracy.
Character& Ranger::operator+=(float amount)
{
    // Iterate through all the stats.
    for (int i = 0; i < 6; i++) {
        // Apply a 1.5x multiplier if the stat is Accuracy (index 5) or Health (index 4).
        if ((i == 5) || (i == 4)) {
            statValue[i] += 1.5 * amount * floor;
        }
        // Otherwise, increase the stat by the base amount scaled by the floor.
        else {
            statValue[i] += amount * floor;
        }
    }
    // Return a reference to the modified Ranger object to allow for chaining.
    return *this;
}