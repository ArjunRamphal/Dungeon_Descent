#pragma once
#ifndef MAGE_H
#define MAGE_H
#include <iostream>
#include <string>
#include "Character.h"
class Mage :
    public Character
{
private:
public:
    Mage();
    void Ability() override;
    void incStats(float amount) override;
    string incStatsDisplay(float amount) override;
    void incXStat(int index, float amount) override;
    string incXStatDisplay(int index, float amount) override;
    Character& operator += (float amount) override;
};

#endif // Mage