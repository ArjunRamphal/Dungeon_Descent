#pragma once
#ifndef ROGUE_H
#define ROGUE_H
#include <iostream>
#include <string>
#include "Character.h" // Include the base Character class definition

// Class definition for the Rogue character type.
// Inherits from the Character class.
class Rogue :
    public Character
{

private:
public:
    // Default constructor for the Rogue class.
    Rogue();

    // Overrides the Ability method from the base Character class to define the Rogue's specific ability (though the implementation is not shown here).
    void Ability();

    // Overrides the incStats method from the base Character class to modify how all stats are increased for a Rogue.
    void incStats(float amount) override;

    // Overrides the incStatsDisplay method from the base Character class to provide a Rogue-specific display message for stat increases.
    string incStatsDisplay(float amount) override;

    // Overrides the incXStat method from the base Character class to modify how a specific stat is increased for a Rogue.
    void incXStat(int index, float amount) override;

    // Overrides the incXStatDisplay method from the base Character class to provide a Rogue-specific display message for individual stat increases.
    string incXStatDisplay(int index, float amount) override;

    // Overrides the += operator from the base Character class to modify how stats are added for a Rogue.
    Character& operator += (float amount) override;
};

#endif // ROGUE_H