#include "Character.h"
#include <iostream>
#include <cmath> // For the roundFloat function
#include <string> // For string manipulation

// Constructor for the Character class that takes a name.
Character::Character(const std::string& name) 
{
    // Initialize the character's name.
    this->name = name;
    // Initialize the starting floor to 0.
    floor = 0;
    // Initialize the starting reputation to 0.
    reputation = 0;
}

// Default constructor for the Character class.
Character::Character()
{
}

// Getter method to retrieve the profile picture image filename.
string Character::getPfpImageName() const
{
    return pfpImageName;
}

// Getter method to retrieve the character's name.
string Character::getName()
{
    return name;
}

// Getter method to retrieve the ability image filename.
string Character::getAbilityFileName()
{
    return abilityFileName;
}

// Getter method to retrieve a pointer to the character's stats array.
float* Character::getStats() 
{
    // Dynamically allocate a new array to hold the stat values.
    float* arr = new float[6];
    // Copy the stat values from the member array to the newly allocated array.
    for (int i = 0; i < 6; i++) {
        arr[i] = Character::statValue[i];
    }
    // Return a pointer to the newly allocated array.
    // The caller of this function is responsible for deleting this allocated memory using delete[].
    return arr;
}

// Getter method to retrieve the character's current health. Health is stored at index 4 of the statValue array.
float Character::getHealth()
{
    return statValue[4];
}

// Method to reduce the character's health by a given amount of damage.
float Character::takeDamage(float damage) 
{
    // Decrease the health by the amount of damage.
    statValue[4] -= damage;
    // Ensure health does not go below zero.
    if (statValue[4] < 0) {
        statValue[4] = 0;  // Set health to 0 if it becomes negative.
    }
    // Return the amount of damage taken.
    return damage;
}

// Method to increase a specific stat by a given amount.
void Character::incXStat(int index, float amount)
{
    // Increase the stat at the given index by the specified amount.
    statValue[index] = statValue[index] + amount;
}

// Method to return a string describing the increase of a specific stat.
string Character::incXStatDisplay(int index, float amount)
{
    // Convert the amount to a string.
    string amountStr = to_string(amount);
    // Find the position of the decimal point.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a formatted string indicating which stat increased and by how much (formatted to two decimal places).
    return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

// Method to increase all stats by a given amount (uses the += operator overload).
void Character::incStats(float amount)
{
    *this += amount;
}

// Method to decrease all stats by a given amount (uses the -= operator overload).
void Character::decStats(float amount)
{
    *this -= amount;
}

// Method to return a string describing the increase of all stats, scaled by the current floor.
string Character::incStatsDisplay(float amount)
{
    // Calculate the total increase based on the amount and the current floor.
    string amountStr = to_string(amount * floor);
    // Find the position of the decimal point.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a formatted string indicating that all stats have increased.
    return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

// Method to return a string describing the decrease of all stats, scaled by the current floor.
string Character::decStatsDisplay(float amount)
{
    // Calculate the total decrease based on the amount and the current floor.
    string amountStr = to_string(amount * floor);
    // Find the position of the decimal point.
    size_t decimalPos = amountStr.find_first_of(".");
    // Return a formatted string indicating that all stats have decreased.
    return "All stats have been decreased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

// Virtual method representing the character's special ability.
void Character::Ability()
{
}

// Getter method to retrieve the current biome.
int Character::getBiome()
{
    return biome;
}

// Setter method to set the current biome.
void Character::setBiome(int biome)
{
    this->biome = biome;
}

// Method to increment the current floor and decrease the question time.
void Character::incFloor() {
    // Increase the floor by 1.
    floor += 1;
    // Decrease the question time by 5 (scaling difficulty with floor).
    QuestionTime -= 5;
}

// Getter method to retrieve the current room counter.
int Character::getRoomCounter()
{
    return roomCounter;
}

// Method to increment the current room counter.
void Character::incRoomCounter()
{
    roomCounter++;
}

// Getter method to retrieve the number of correctly answered riddles.
int Character::getRiddleCorrect()
{
    return riddleCorrect;
}

// Method to increment the number of correctly answered riddles.
void Character::incRiddleCorrect()
{
    riddleCorrect++;
}

// Getter method to retrieve the current floor.
int Character::getFloor()
{
    return floor;
}

// Getter method to retrieve the name of a specific stat based on its index.
string Character::getStatName(int index)
{
    return statName[index];
}

// Getter method to retrieve the value of a specific stat based on its index, rounded to two decimal places.
float Character::getStatValue(int index)
{
    return roundFloat(statValue[index]);
}

// Utility method to round a float value to two decimal places.
float Character::roundFloat(float var)
{
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
}

// Method to increment the character's reputation.
void Character::incReputation()
{
    reputation++;
}

// Method to increment the number of battles won.
void Character::incBattlesWon()
{
    battlesWon++;
}

// Getter method to retrieve the base question time limit.
int Character::getQuestionTime()
{
    return QuestionTime;
}

// Method to calculate and return extra question time based on the character's wisdom (stat at index 1).
int Character::getExtraQuestionTime()
{
    return statValue[1] / 2;
}

// Operator overload for the += operator to increase all stats by a scaled amount.
Character& Character::operator += (float amount) 
{
    // Iterate through all the character's stats.
    for (int i = 0; i < 6; i++) {
        // Increase each stat by the given amount multiplied by the current floor (scaling with progress).
        statValue[i] += amount * floor;
    }
    // Return a reference to the modified Character object.
    return *this;
}

// Operator overload for the -= operator to decrease all stats by a scaled amount.
Character& Character::operator-=(float amount)
{
    // Iterate through all the character's stats.
    for (int i = 0; i < 6; i++) {
        // Check if decreasing the stat would make it negative.
        if (statValue[i] - amount * floor < 0) {
            // If it would be negative, set the stat to 0 to prevent negative values.
            statValue[i] = 0;
        }
        else {
            // Otherwise, decrease the stat by the given amount multiplied by the current floor.
            statValue[i] -= amount * floor;
        }
    }
    // Return a reference to the modified Character object.
    return *this;
}