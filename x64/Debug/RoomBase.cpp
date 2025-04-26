#include "RoomBase.h"
#include <iostream>

RoomBase::RoomBase(const std::string& name) {

}

string RoomBase::getType() {
	return type;
}

vector<string> RoomBase::getChoices(int roomCounter, int biome)
{
	return vector<string>();
}

string RoomBase::getImageFileName(int biome)
{
	return imageFileName;
}

string RoomBase::getTextFileName(int biome)
{
	return textFileName;
}

RoomBase::RoomBase() {

}

string RoomBase::getbtnChoice1(Character& character)
{
	return string();
}

string RoomBase::getbtnChoice2(Character& character)
{
	return string();
}

string RoomBase::getbtnChoice3(Character& character)
{
	return string();
}

string RoomBase::getbtnChoice4(Character& character)
{
	return string();
}
