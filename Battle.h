#pragma once
#ifndef BATTLE_H
#define BATTLE_H

#include <iostream>
#include <string>
#include "RoomBase.h"
#include "Character.h"
#include "test.h"  
#include "CriticalStrike.h"

class Battle : public RoomBase {
private:
    test* enemy;  // Declare a pointer to the test class (the enemy)
    Character* character;
    int strikesRemaining;  // Number of strikes remaining for the player
    int extra;  // Extra strikes based on player agility
    bool isBoss;


    CriticalStrike<int> intCrit{ 2 };       // 2x multiplier for integers
    CriticalStrike<float> floatCrit{ 1.5f }; // 1.5x multiplier for floats
public:
    //constructor
    Battle(bool isBoss, Character& player);  // Constructor for Battle
    //methods
    string getImageFileName(int biome);
    string getTextFileName(int biome);
    bool attack(Character& player);  // Method to handle player attack
    int Calculate_Extra_Strikes(int agility);  // Calculate extra strikes based on agility
    bool isCrit_Strike(int accuracy);  // Determine if it's a critical strike
    int Total_Strikes();  // Total number of strikes (including extra strikes)
    bool isBattleFinished();  // Check if the battle is finished (based on strikes or defeat)
    int getStrikesRemaining() const { 
        return strikesRemaining; 
    }  // Getter for strikes remaining
    // Getter for enemy (accessor method)
    test* getEnemy() const {
        return enemy;
    }
    vector<string> getChoices(int roomCounter, int biome) override;
    string getbtnChoice2(Character& character) override;

    //destructor
    ~Battle() {
        delete enemy;  // Clean up memory for enemy when Battle is done
    }
};

#endif // BATTLE_H
