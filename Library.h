#pragma once
#ifndef LIBRARY_H
#define LIBRARY_H
#include <iostream>
#include <string>
#include <vector>
#include "RoomBase.h"

class Library :
    public RoomBase
{
private:
    vector<string> choices;
public:
    Library();
    string imageFileName;
    string textFileName;
    string getImageFileName(int biome);
    string getTextFileName(int biome);
    vector<string> getChoices(int roomCounter, int biome) override;
    string getbtnChoice1(Character &character) override;
    string getbtnChoice2(Character& character) override;
    string getbtnChoice3(Character& character) override;
    string getbtnChoice4(Character& character) override;
};
#endif