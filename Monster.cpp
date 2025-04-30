#pragma once
#include "Monster.h"

// Constructor for the Monster class. Initializes a monster with health based on the floor and boss status.
Monster::Monster(int floor, bool isBoss) : floor(floor), isBoss(isBoss) {
    // Calculate the base health of the monster based on the current floor level.
    float baseHealth = 30 + (floor * 2);

    // Increase the base health for higher floor levels to make them more challenging.
    if (floor == 2) {
        baseHealth *= 2; // Monsters on floor 2 have triple the base health.
    }
    else if (floor == 3) {
        baseHealth *= 3.5; // Monsters on floor 3 have 4.5 times the base health.
    }

    // Further increase the health if the monster is a boss.
    if (isBoss) {
        baseHealth *= 4.5; // Boss monsters have 50% more health than regular monsters on the same floor.
    }
    // Set the monster's current health to the calculated base health.
    health = baseHealth;
}

// Get the current health of the enemy.
// @return The current health points of the monster.
float Monster::getHealth() const {
    return health;
}

// Reduce the monster's health by a given damage amount.
// @param damage The amount of damage taken by the monster.
// Reduce health by damage amount
template<typename T>
void Monster::takeDamage(T damage) {
    health -= static_cast<float>(damage); // Cast damage to float for health calculation
    if (health < 0) {
        health = 0; // Ensure health doesn't go below 0
    }
}
template void Monster::takeDamage<int>(int);
template void Monster::takeDamage<float>(float);

// Check if the enemy has been defeated.
// @return True if the monster's health is zero, indicating it is defeated; false otherwise.
bool Monster::isDefeated() const {
    return (health == 0);
}