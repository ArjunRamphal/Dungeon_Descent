#pragma once
#ifndef WARRIOR_H
#define WARRIOR_H
#include <iostream>
#include <string>
#include "Character.h"
class Warrior :
    public Character
{

private:
public:
    Warrior();
    void Ability();
    void incStats(float amount) override;
    string incStatsDisplay(float amount) override;
    void incXStat(int index, float amount) override;
    string incXStatDisplay(int index, float amount) override;
    Character& operator += (float amount) override;
};

#endif // Warrior