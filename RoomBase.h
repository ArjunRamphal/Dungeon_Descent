#ifndef ROOMBASE_H
#define ROOMBASE_H
#include <iostream>
#include <string>
#include <vector>
#include "Character.h"

using namespace std;

class RoomBase {
protected:
	vector<string> choices;

public:
	bool Completed;
	bool KeyExists;
	int NPC;
	string type;
	string imageFileName;
	string textFileName;
	int Book;
	bool BookExists;
	int Floor;
	int EnemyType;
	RoomBase(const std::string& name);
	RoomBase();

	int getBook();
	bool getKey();
	int getFloor();
	int getNPC();
	string getType();

	virtual vector<string> getChoices(int roomCounter, int biome);

	virtual string getImageFileName(int biome);
	virtual string getTextFileName(int biome);

	void setBook(int book);
	void setKey(bool key);
	void setFloor(int floor);

	virtual string getbtnChoice1(Character& character);
	virtual string getbtnChoice2(Character& character);
	virtual string getbtnChoice3(Character& character);
	virtual string getbtnChoice4(Character& character);
};

#endif // ROOMBASE_H
