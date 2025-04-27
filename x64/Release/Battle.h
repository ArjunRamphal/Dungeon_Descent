#pragma once
#ifndef BATTLE_H
#define BATTLE_H

#include <iostream>
#include <string>
#include <vector> // Include for using std::vector
#include "RoomBase.h" // Include the base class definition
#include "Character.h" // Include the Character class definition
#include "Monster.h"   // Include the Monster class definition

// Class definition for the Battle room type.
// Inherits from the RoomBase class, representing an encounter where the player fights a monster.
class Battle : public RoomBase {
private:
    // Pointer to the Monster object that the player is fighting.
    Monster* enemy;
    // Pointer to the Character object representing the player.
    Character* character;
    // The number of attack actions the player has remaining in the current battle.
    int strikesRemaining;
    // The number of additional attack actions granted based on the player's agility stat.
    int extra;
    // A boolean flag indicating whether this battle is against a boss monster.
    bool isBoss;
public:
    // Constructor for the Battle class. Initializes a new battle encounter.
    // @param isBoss A boolean indicating if the enemy is a boss monster.
    // @param player A reference to the Character object representing the player.
    Battle(bool isBoss, Character& player);

    // Returns the filename of the image to display for this Battle room based on the biome and battle type.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the battle image.
    string getImageFileName(int biome);

    // Returns the filename of the text description to display for this Battle room based on the biome and battle type.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the text file describing the battle encounter.
    string getTextFileName(int biome);

    // Overrides the getbtnChoice2 method from the base class to define the outcome of the second choice in a Battle room.
    // @param character A reference to the Character object interacting with the battle.
    // @return A string describing the result of the second choice.
    string getbtnChoice2(Character& character) override;

    // Calculates the number of extra attack actions the player gets based on their agility stat.
    // @param agility The player's agility stat.
    // @return The number of extra strikes.
    int Calculate_Extra_Strikes(int agility);

    // Calculates the total number of attack actions the player has at the start of the battle.
    // This includes a random base number of strikes plus any extra strikes from agility.
    // @return The total number of strikes.
    int Total_Strikes();

    // Getter method to retrieve the number of attack actions remaining in the current battle.
    // @return The number of strikes remaining.
    int getStrikesRemaining() const;

    // Calculates and applies the damage taken by the player from the enemy monster.
    // The damage is scaled based on whether the enemy is a boss.
    // @return The amount of damage taken by the player.
    float damageTaken();

    // Simulates an attack by the player on the enemy monster.
    // Reduces the number of remaining strikes and potentially deals damage based on a critical hit chance.
    // @param player A reference to the Character object representing the player.
    // @return True if the attack was a critical strike, false otherwise.
    bool attack(Character& player);

    // Determines if the player's current attack is a critical strike based on their accuracy stat.
    // @param accuracy The player's accuracy stat.
    // @return True if the attack is a critical strike, false otherwise.
    bool isCrit_Strike(int accuracy);

    // Checks if the current battle has concluded.
    // The battle ends if the player has no strikes remaining, the enemy is defeated, or the player's health reaches zero.
    // @return True if the battle is finished, false otherwise.
    bool isBattleFinished();

    // Overrides the getChoices method from the base class to provide specific choices for a Battle room.
    // @param roomCounter The current room number.
    // @param biome An integer representing the current biome.
    // @return A vector of strings, where each string represents a choice the player can make in this battle.
    vector<string> getChoices(int roomCounter, int biome) override;

    // Getter method to retrieve a pointer to the enemy Monster object.
    // @return A pointer to the enemy Monster.
    Monster* getEnemy() const;

    // Returns whether the current enemy is a boss monster.
    // @return True if the enemy is a boss, false otherwise.
    bool getIsBoss();

    // Destructor for the Battle class.
    // Responsible for releasing dynamically allocated memory, such as the enemy Monster object.
    ~Battle() {
        delete enemy; // Deallocate the memory occupied by the enemy Monster object.
    }
};

#endif // BATTLE_H