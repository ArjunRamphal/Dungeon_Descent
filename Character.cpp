#include "Character.h"
#include <iostream>

// Constructor
Character::Character(const std::string& name) {
    this->name = name;
    std::string statName[6] = { "Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy" };
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

int* Character::getStats() {
    int* arr = new int[6]; // Allocate memory
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

void Character::incStats(int index, int amount)
{
    statValue[index] += amount;
}

void Character::decStats(int index, int amount)
{
    if (statValue[index] - amount < 0) {
        statValue[index] = 0; // Prevent negative stats
    }
    else {
        statValue[index] -= amount;
    }
}

string Character::incStatsDisplay(int index, int amount)
{
    return getStatName(index) + " has increased by " + to_string(amount) + ".";
}

string Character::decStatsDisplay(int index, int amount)
{
    return getStatName(index) + " has decreased by " + to_string(amount) + ".";
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

int Character::getStatValue(int index)
{
    return statValue[index];
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

void Character::decPotionLength()
{

}
/*
template<typename T>
void incStats(int index, T amount) {
    statValue[index] += amount;
    potionLength = 3;
}

template<typename T>
void decStats(int index, T amount) {
    statValue[index] -= amount;
}

void operator += (Character& c, float amount) {
    for (int i = 0; i < 6; i++) {
        c.statValue[i] *= amount;
    }
}*/