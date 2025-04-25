#pragma once
#ifndef BATTLE_H
#define BATTLE_H

#include <iostream>
#include <string>
#include "RoomBase.h"
#include "Character.h"
#include "Monster.h"  

class Battle : public RoomBase {
private:
    Monster* enemy;  // Declare a pointer to the Monster class (the enemy)
    Character* character;
    int strikesRemaining;  // Number of strikes remaining for the player
    int extra;  // Extra strikes based on player agility
    bool isBoss;
public:
    //constructor
    Battle(bool isBoss, Character& player);  // Constructor for Battle
    //methods
    string getImageFileName(int biome);
    string getTextFileName(int biome);
    string getbtnChoice2(Character& character) override;

    int Calculate_Extra_Strikes(int agility);  // Calculate extra strikes based on agility
    int Total_Strikes();  // Total number of strikes (including extra strikes)
    int getStrikesRemaining() const;  // Getter for strikes remaining

    bool attack(Character& player);  // Method to handle player attack
    bool isCrit_Strike(int accuracy);  // Determine if it's a critical strike
    bool isBattleFinished();  // Check if the battle is finished (based on strikes or defeat)

    vector<string> getChoices(int roomCounter, int biome) override;

    Monster* getEnemy() const;// Getter for enemy (accessor method)

    //destructor
    ~Battle() {
        delete enemy;  // Clean up memory for enemy when Battle is done
    }
};

#endif // BATTLE_H