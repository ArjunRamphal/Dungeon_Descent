#pragma once
#ifndef BOSS_H
#define BOSS_H
#include <iostream>
#include <string>
#include "RoomBase.h"
class Boss :
    public RoomBase
{

private:
public:
    Boss();
    void Cleared();
    int StatReq[6];
    int defeat(int Stats[6]);
    int baseHealth;
    string getImageFileName(int biome);
    string getTextFileName(int biome);
};

#endif // Warrior