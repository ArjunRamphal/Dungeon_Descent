#pragma once
#ifndef MAGE_H
#define MAGE_H
#include <iostream>
#include <string>
#include "Character.h" // Include the base Character class definition

// Class definition for the Mage character type.
// Inherits from the Character class.
class Mage :
    public Character
{
private:
public:
    // Default constructor for the Mage class.
    Mage();

    // Overrides the Ability method from the base Character class to define the Mage's specific ability.
    void Ability() override;

    // Overrides the incStats method from the base Character class to modify how all stats are increased for a Mage.
    void incStats(float amount) override;

    // Overrides the incStatsDisplay method from the base Character class to provide a Mage-specific display message for stat increases.
    string incStatsDisplay(float amount) override;

    // Overrides the incXStat method from the base Character class to modify how a specific stat is increased for a Mage.
    void incXStat(int index, float amount) override;

    // Overrides the incXStatDisplay method from the base Character class to provide a Mage-specific display message for individual stat increases.
    string incXStatDisplay(int index, float amount) override;

    // Overrides the += operator from the base Character class to modify how stats are added for a Mage.
    Character& operator += (float amount) override;
};

#endif // Mage