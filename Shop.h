#pragma once
#pragma once
#ifndef SHOP_H
#define SHOP_H
#include <iostream>
#include <string>
#include "RoomBase.h"

using namespace std;
class Shop :
    public RoomBase
{

private:
public:
    Shop();
    string getImageFileName(int biome);
    string getTextFileName(int biome);
    vector<string> getChoices(int roomCounter, int biome) override;
    string getbtnChoice1(Character& character) override;
    string getbtnChoice2(Character& character) override;
    string getbtnChoice3(Character& character) override;
};

#endif // Warrior