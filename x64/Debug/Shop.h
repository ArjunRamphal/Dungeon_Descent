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
    bool QAvail;
    bool QComp;
    string imageFileName;
    string textFileName;
    string getImageFileName(int biome);
    string getTextFileName(int biome);
};

#endif // Warrior