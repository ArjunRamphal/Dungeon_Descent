#ifndef CHARACTER_H  
#define CHARACTER_H  
#include <iostream>  
#include <string>  

using namespace std;

class Character {
protected: 
	int QuestionTime = 20;
	int baseHealth;
	int Reputation;
	string abilityFileName;
	float statValue[6];
	string statName[6] = { "Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy" };
public:
	int health;

	string inventoryItemName[3];
	int inventoryItemAmount[3];

	string pfpImageName;
	

	int floor = 0;

	// Constructor  
	Character(const string& name);
	Character();

	// Getters  
	string getAbilityFileName();
	string getpfpImageName() const;

	int getBaseHealth();

	void setHealth(int health);

	int* getStats();
	int* getInv();

	void takeDamage(int damage);

	bool isAlive();
	void addGold(int amount);
	void subGold(int amount);
	void addKey();
	void subKey();

	void incStats(int index, int amount);
	void decStats(int index, int amount);

	int getFloor();
	void incFloor();

	void negEvent(int amountrep);
	void posEvent(int amountrep);

	int getQuestionTime() {
		return QuestionTime;
	}

	string getStatName(int index);
	int getStatValue(int index);

	int getReputation() {
		return Reputation;
	}

	void incReputation();
};

#endif // CHARACTER_H