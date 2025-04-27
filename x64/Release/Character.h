#ifndef CHARACTER_H
#define CHARACTER_H
#include <iostream>
#include <string>
#include <cmath>

using namespace std;

// Base class for all character types in the game.
class Character {
protected:
	// Time limit for answering questions.
	int QuestionTime = 25;
	// Player's reputation within the game world.
	int reputation;
	// Number of battles the character has won.
	int battlesWon = 0;
	// Current biome the character is in.
	int biome = 0;
	// Current floor the character is on in the dungeon or game world.
	int floor = 0;
	// Counter for the number of rooms the character has entered.
	int roomCounter = 0;
	// Number of riddles the character has answered correctly.
	int riddleCorrect = 0;
	// Array to store the character's core stats
	float statValue[6];
	// Character's name.
	string name;
	// Array of strings holding the names of the stats.
	string statName[6] = { "Strength", "Wisdom", "Observation", "Agility", "Health", "Accuracy" };
	// Filename for the character's ability image.
	string abilityFileName;
	// Filename for the character's profile picture image.
	string pfpImageName;
public:
	// Default constructor. Initializes a basic character with default values.
	Character();
	// Constructor that takes the character's name as input.
	Character(const string& name);

	// Getter method to retrieve the character's name.
	string getName();

	// Getter method to retrieve the filename of the character's ability image.
	string getAbilityFileName();
	// Getter method to retrieve the filename of the character's profile picture image.
	string getPfpImageName() const;

	// Getter method to retrieve a pointer to the array of character stats.
	float* getStats();
	// Getter method to retrieve the character's current health.
	float getHealth();

	// Method to reduce the character's health by a given amount of damage. Returns the amount of damage taken.
	float takeDamage(float damage);

	// Virtual method to increase a specific stat by a given amount.
	virtual void incXStat(int index, float amount);
	// Virtual method to return a string describing the increase of a specific stat.
	virtual string incXStatDisplay(int index, float amount);

	// Virtual method to increase all stats by a given amount.
	virtual void incStats(float amount);
	// Virtual method to return a string describing the increase of all stats
	virtual string incStatsDisplay(float amount);
	// Method to return a string describing the decrease of all stats.
	string decStatsDisplay(float amount);
	// Method to decrease all stats by a given amount.
	void decStats(float amount);

	// Virtual method representing the character's special ability.
	virtual void Ability();

	// Getter method to retrieve the current biome.
	int getBiome();
	// Setter method to set the current biome.
	void setBiome(int biome);

	// Getter method to retrieve the current floor.
	int getFloor();
	// Method to increment the current floor.
	void incFloor();

	// Getter method to retrieve the current room counter.
	int getRoomCounter();
	// Method to increment the current room counter.
	void incRoomCounter();

	// Getter method to retrieve the number of correctly answered riddles.
	int getRiddleCorrect();
	// Method to increment the number of correctly answered riddles.
	void incRiddleCorrect();

	// Getter method to retrieve the name of a specific stat based on its index.
	string getStatName(int index);
	// Getter method to retrieve the value of a specific stat based on its index.
	float getStatValue(int index);

	// Utility method to round a float value to the nearest integer.
	float roundFloat(float var);

	// Getter method to retrieve the character's reputation.
	int getReputation() {
		return reputation;
	}
	// Method to increase the character's reputation.
	void incReputation();

	// Getter method to retrieve the number of battles won.
	int getBattlesWon() {
		return battlesWon;
	}
	// Method to increment the number of battles won.
	void incBattlesWon();

	// Getter method to retrieve the base question time limit.
	int getQuestionTime();
	// Method to get any extra question time.
	int getExtraQuestionTime();

	// Virtual operator overload for adding a float value to the character stats.
	virtual Character& operator += (float amount);
	// Virtual operator overload for subtracting a float value from the character stats.
	virtual Character& operator -= (float amount);
};

#endif // CHARACTER_H