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
    int Hidden;
    void Ability();
    int Lastfpwrupused;
    void incStats(int index, int amount) override;
    string incStatsDisplay(int index, int amount) override;

};

#endif // Rogue