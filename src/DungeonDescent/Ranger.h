#pragma once
#ifndef RANGER_H
#define RANGER_H
#include <iostream>
#include <string>
#include "Character.h" // Include the base Character class definition

// Class definition for the Ranger character type.
// Inherits from the Character class.
class Ranger :
    public Character
{
private:
public:
    // Default constructor for the Ranger class.
    Ranger();

    // Overrides the Ability method from the base Character class to define the Ranger's specific ability.
    void Ability() override;

    // Overrides the incStats method from the base Character class to modify how all stats are increased for a Ranger.
    void incStats(float amount) override;

    // Overrides the incStatsDisplay method from the base Character class to provide a Ranger-specific display message for stat increases.
    string incStatsDisplay(float amount) override;

    // Overrides the incXStat method from the base Character class to modify how a specific stat is increased for a Ranger.
    void incXStat(int index, float amount) override;

    // Overrides the incXStatDisplay method from the base Character class to provide a Ranger-specific display message for individual stat increases.
    string incXStatDisplay(int index, float amount) override;

    // Overrides the += operator from the base Character class to modify how stats are added for a Ranger.
    Character& operator += (float amount) override;
};

#endif // Ranger