#pragma once
#ifndef LIBRARY_H
#define LIBRARY_H
#include <iostream>
#include <string>
#include "RoomBase.h"
class Library :
    public RoomBase
{
private:
public:

    Library();
    bool QAvail;
    bool QComp;
    int BookEff(int Type, int arr[6]);
    string imageFileName;
    string textFileName;
    string getImageFileName(int biome);
    string getTextFileName(int biome);
};
#endif // Warrior