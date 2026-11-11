#pragma once
#ifndef WARRIOR_H
#define WARRIOR_H
#include <iostream>
#include <string>
#include "Character.h" // Include the base Character class definition

// Class definition for the Warrior character type.
// Inherits from the Character class.
class Warrior :
    public Character
{

private:
public:
    // Default constructor for the Warrior class.
    Warrior();

    // Method to perform the Warrior's special ability.
    void Ability();

    // Overrides the incStats method from the base Character class to define how all stats are increased for a Warrior.
    void incStats(float amount) override;

    // Overrides the incStatsDisplay method from the base Character class to provide a Warrior-specific display message for stat increases.
    string incStatsDisplay(float amount) override;

    // Overrides the incXStat method from the base Character class to define how a specific stat is increased for a Warrior.
    void incXStat(int index, float amount) override;

    // Overrides the incXStatDisplay method from the base Character class to provide a Warrior-specific display message for individual stat increases.
    string incXStatDisplay(int index, float amount) override;

    // Overrides the += operator from the base Character class to define how stats are added for a Warrior.
    Character& operator += (float amount) override;
};

#endif // Warrior