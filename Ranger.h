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
    int Lastfpwrupused;
    void incStats(int index, int amount) override;
    string incStatsDisplay(int index, int amount) override;
};

#endif // Ranger