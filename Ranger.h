#pragma once
#ifndef RANGER_H
#define RANGER_H
#include <iostream>
#include <string>
#include "Character.h"
class Ranger :
    public Character
{
private:
public:
    Ranger();
    void Ability() override;
    void incStats(float amount) override;
    string incStatsDisplay(float amount) override;
    void incXStat(int index, float amount) override;
    string incXStatDisplay(int index, float amount) override;
    Character& operator += (float amount) override;
};

#endif // Ranger