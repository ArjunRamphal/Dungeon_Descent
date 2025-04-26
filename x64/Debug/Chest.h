#pragma once
#ifndef CHEST_H
#define CHEST_H
#include <iostream>
#include <string>
#include "RoomBase.h"
class Chest :
    public RoomBase
{

private:
public:
    Chest();
    string getImageFileName(int biome);
    string getTextFileName(int biome);
    vector<string> getChoices(int roomCounter, int biome) override;
    string getbtnChoice1(Character& character) override;
};

#endif // Warrior