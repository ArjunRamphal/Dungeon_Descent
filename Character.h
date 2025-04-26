#ifndef CHARACTER_H  
#define CHARACTER_H  
#include <iostream>  
#include <string>  

using namespace std;

class Character {
protected: 
	int QuestionTime = 25;
	int reputation;
	int battlesWon = 0;
	int biome = 0;
	int floor = 0;
	int roomCounter = 0;
	int riddleCorrect = 0;
	int potionLength = 0;
	float statValue[6];
	string name;
	string statName[6] = { "Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy" };
	string abilityFileName;
	string pfpImageName;
public:
	// Constructor  
	Character(const string& name);
	Character();

	// Getters  
	string getName();

	string getAbilityFileName();
	string getPfpImageName() const;

	int* getStats();
	float getHealth();

	float takeDamage(float damage);

	void virtual incStats(int index, int amount);
	string virtual incStatsDisplay(int index, int amount);
	string decStatsDisplay(int index, int amount);
	void decStats(int index, int amount);

	void virtual Ability();

	int getBiome();
	void setBiome(int biome);

	int getFloor();
	void incFloor();

	int getRoomCounter();
	void incRoomCounter();

	int getRiddleCorrect();
	void incRiddleCorrect();

	string getStatName(int index);
	int getStatValue(int index);

	int getReputation() {
		return reputation;
	}
	void incReputation();

	int getBattlesWon() { 
		return battlesWon; 
	}
	void incBattlesWon();

	int getQuestionTime();
	int getExtraQuestionTime();

	void decPotionLength();
};

#endif // CHARACTER_H