#pragma once
#ifndef ENCOUNTER_H
#define ENCOUNTER_H
#include <iostream>
#include <string>
#include <vector>
#include "RoomBase.h"
class Encounter : public RoomBase
{
private:
    vector<string> choices;
public:
    Encounter();
    string getImageFileName(int biome);
    string getTextFileName(int biome);

    vector<string> getChoices(int roomCounter, int biome) override;

    string getbtnChoice1(Character& character) override;
    string getbtnChoice2(Character& character) override;
};
#endif

