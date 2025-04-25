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

int RoomBase::getBook() {
	if (BookExists == true) {
		return Book;
	}
	else {
		return 0;
	}
}

bool RoomBase::getKey() {
	return KeyExists;
}

int RoomBase::getFloor() {
	return Floor;
}

int RoomBase::getNPC() {
	return NPC;
}

void RoomBase::setBook(int book)
{
	Book = book;
}

void RoomBase::setKey(bool key)
{
	KeyExists = key;
}

void RoomBase::setFloor(int floor)
{
	Floor = floor;
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
