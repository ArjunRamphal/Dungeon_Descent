#include "Character.h"
#include <iostream>

// Constructor
Character::Character(const std::string& name) {
    this->name = name;
    string statName[6] = { "Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy" };
    float statValue[6] = { 0,0,0,0,0,0 };
    floor = 0;
    reputation = 0;
}

Character::Character()
{
}

// Getters
string Character::getPfpImageName() const
{
    return pfpImageName;
}

string Character::getName()
{
    return name;
}

string Character::getAbilityFileName()
{
    return abilityFileName;
}

float* Character::getStats() {
    float* arr = new float[6]; // Allocate memory
    for (int i = 0; i < 6; i++) {
        arr[i] = Character::statValue[i]; // Fill array
    }
    return arr; // Caller must delete[] it
}

float Character::getHealth()
{
    return statValue[4];
}

float Character::takeDamage(float damage) {
    statValue[4] -= damage;
    if (statValue[4] < 0) {
        statValue[4] = 0;  // Prevent negative health
    }
    return damage;
}

void Character::incXStat(int index, float amount)
{
    statValue[index] = statValue[index] + amount;
}

string Character::incXStatDisplay(int index, float amount)
{
    string amountStr = to_string(amount);
    size_t decimalPos = amountStr.find_first_of(".");
    return getStatName(index) + " has increased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

void Character::incStats(float amount)
{
    *this += amount;
}

void Character::decStats(float amount)
{
    *this -= amount;
}

string Character::incStatsDisplay(float amount)  
{   
   string amountStr = to_string(amount * floor);  
   size_t decimalPos = amountStr.find_first_of(".");  
   return "All stats have been increased by " + amountStr.substr(0, decimalPos + 3) + ".";  
}

string Character::decStatsDisplay(float amount)
{
    string amountStr = to_string(amount * floor);
    size_t decimalPos = amountStr.find_first_of(".");
    return "All stats have been decreased by " + amountStr.substr(0, decimalPos + 3) + ".";
}

void Character::Ability()
{
}

int Character::getBiome()
{
    return biome;
}

void Character::setBiome(int biome)
{
    this->biome = biome;
}

void Character::incFloor() {
    floor += 1;
    QuestionTime -= 5;
}

int Character::getRoomCounter()
{
    return roomCounter;
}

void Character::incRoomCounter()
{
    roomCounter++;
}

int Character::getRiddleCorrect()
{
    return riddleCorrect;
}

void Character::incRiddleCorrect()
{
    riddleCorrect++;
}

int Character::getFloor()
{
    return floor;
}

string Character::getStatName(int index)
{
    return statName[index];
}

float Character::getStatValue(int index)
{
    return statValue[index];
}

float Character::roundFloat(float var)
{
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
}

void Character::incReputation()
{
    reputation++;
}

void Character::incBattlesWon()
{
    battlesWon++;
}

int Character::getQuestionTime()
{
    return QuestionTime;
}

int Character::getExtraQuestionTime()
{
    return statValue[1] / 2;
}

Character& Character::operator += (float amount) {
    for (int i = 0; i < 6; i++) {
        statValue[i] += amount * floor;
    }
    return *this; // Return a reference to the modified object
}

Character& Character::operator-=(float amount)
{
    for (int i = 0; i < 6; i++) {
        if (statValue[i] - amount * floor < 0) {
            statValue[i] = 0; // Prevent negative stats
        }
        else {
            statValue[i] -= amount * floor;
        }
    }
    return *this; // Return a reference to the modified object
}
