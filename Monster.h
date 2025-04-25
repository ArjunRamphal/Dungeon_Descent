#pragma once
#ifndef MONSTER_H
#define MONSTER_H

#include <string>
#include <iostream>

class Monster{
private:
    int floor; // Floor level (1, 2, or 3)
    float health; // Current health of the enemy
    bool isBoss; // Whether the enemy is a boss
public:
    Monster(int floor, bool isBoss); // Constructor

    float getHealth() const; // Get the current health of the enemy
    void takeDamage(int damage); // Reduce health by damage amount
    bool isDefeated() const; // Check if the enemy is defeated
};

#endif