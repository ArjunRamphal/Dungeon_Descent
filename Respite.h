#pragma once
#ifndef RESPITE_H
#define RESPITE_H
#include <iostream>
#include <string>
#include "RoomBase.h"
class Respite :
	public RoomBase
{

private:
public:
	Respite();
	int Regen;
	int Accept(bool acc, int arr[6]);
	string imageFileName;
	string textFileName;
	string getTextFileName(int biome);
	string getImageFileName(int biome);
	vector<string> getChoices(int roomCounter, int biome) override;
	string getbtnChoice1(Character& character) override;
	string getbtnChoice2(Character& character) override;
};

#endif // Warrior