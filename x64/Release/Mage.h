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
    int Lastfpwrupused;
    void incStats(int index, int amount) override;
    string incStatsDisplay(int index, int amount) override;
};

#endif // Mage