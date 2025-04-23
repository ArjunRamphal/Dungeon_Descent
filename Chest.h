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
    int Reward;
    void Open();
    bool Locked;
    string imageFileName;
    string textFileName;
    string getImageFileName(int biome);
    string getTextFileName(int biome);
};

#endif // Warrior