#include "RoomBase.h"
#include <iostream>

RoomBase::RoomBase(const std::string& name) {

}

string RoomBase::getType() {
	return type;
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

int RoomBase::getBiome() {
	return Biome;
}

int RoomBase::getEnemy() {
	if (Enemy == true) {
		return EnemyType;
	}
	else {
		return 0;
	}
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

void RoomBase::setEnemyType(int enemyType)
{
	EnemyType = enemyType;
}

void RoomBase::setBook(int book)
{
	Book = book;
}

void RoomBase::setKey(bool key)
{
	KeyExists = key;
}

void RoomBase::setBiome(int biome)
{
	Biome = biome;
}

void RoomBase::setFloor(int floor)
{
	Floor = floor;
}

void RoomBase::setNPC(int npc)
{
	NPC = npc;
}
