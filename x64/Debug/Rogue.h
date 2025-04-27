#pragma once
#ifndef ROGUE_H
#define ROGUE_H
#include <iostream>
#include <string>
#include "Character.h"
class Rogue :
    public Character
{

private:
public:
    Rogue();
    void Ability();
    void incStats(float amount) override;
    string incStatsDisplay(float amount) override;
    void incXStat(int index, float amount) override;
    string incXStatDisplay(int index, float amount) override;
    Character& operator += (float amount) override;

};

#endif // Rogue