#ifndef ROOMBASE_H
#define ROOMBASE_H
#include <iostream>
#include <string>

using namespace std;

class RoomBase {
private:




public:
	bool Completed;
	bool KeyExists;
	int NPC;
	string type;
	string imageFileName;
	string textFileName;
	int Book;
	bool BookExists;
	bool Enemy;
	int Biome;
	int Floor;
	int EnemyType;
	RoomBase(const std::string& name);
	RoomBase();

	int getEnemy();
	int getBook();
	bool getKey();
	int getBiome();
	int getFloor();
	int getNPC();
	string getType();
	virtual string getImageFileName(int biome);
	virtual string getTextFileName(int biome);

	void setEnemyType(int enemyType);
	void setBook(int book);
	void setKey(bool key);
	void setBiome(int biome);
	void setFloor(int floor);
	void setNPC(int npc);
};

#endif // ROOMBASE_H
