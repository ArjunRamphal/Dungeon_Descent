#pragma once
#ifndef MONSTER_H
#define MONSTER_H

#include <string>
#include <iostream>
#include "Character.h"

// Class definition for the Monster enemy.
class Monster {
private:
    // The floor level where the monster is encountered. Influences its strength.
    int floor;
    // The current health points of the monster.
    float health;
    // A boolean flag indicating if this monster is a boss enemy.
    bool isBoss;
public:
    // Constructor for the Monster class.
    // @param floor The current floor level of the game.
    // @param isBoss True if this monster is a boss, false otherwise.
    Monster(int floor, bool isBoss);

    // Template function to calculate the monster's attack damage.
    // The damage is scaled based on the character's floor and a multiplier.
    // The damage also varies slightly based on the character's room counter.
    // @param character A reference to the Character object being attacked.
    // @param multiplier A multiplier to adjust the base attack damage.
    // @return The calculated attack damage.
    template <class T>
    float calculateMonsterAttack(Character& character, T multiplier);

    // Getter method to retrieve the current health of the monster.
    // @return The monster's current health.
    float getHealth() const;

    // Method to reduce the monster's health by a given damage amount.
    // @param damage The amount of damage to inflict on the monster.
    void takeDamage(int damage);

    // Method to check if the monster has been defeated.
    // @return True if the monster's health is zero or less, false otherwise.
    bool isDefeated() const;
};

#endif

// Implementation of the calculateMonsterAttack template function.
template<class T>
inline float Monster::calculateMonsterAttack(Character& character, T multiplier)
{
    // Get the character's current room counter modulo 10 to determine a pattern.
    int room = character.getRoomCounter() % 10;
    // Adjust the attack damage based on the room number.
    // Rooms 1, 2, and 3 have a slightly lower damage multiplier.
    if ((room == 1) || (room == 2) || (room == 3))
        return 1.5 * character.getFloor() * multiplier;
    // Other rooms have a higher damage multiplier.
    else {
        return 2 * character.getFloor() * multiplier;
    }
}