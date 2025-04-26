#pragma once
#include "Monster.h"

// Constructor
Monster::Monster(int floor, bool isBoss) : floor(floor), isBoss(isBoss) {
    // Calculate base health based on floor level
    float baseHealth = 15 + (floor * 2);

    // Adjust health for floor level
    if (floor == 2) {
        baseHealth *= 1.5;
    }
    else if (floor == 3) {
        baseHealth *= 2;
    }

    // Adjust health for bosses
    if (isBoss) {
        baseHealth *= 1.5; // Bosses have 50% more health
    }
    health = baseHealth;
}

// Get the current health of the enemy
float Monster::getHealth() const {
    return health;
}

// Reduce health by damage amount
void Monster::takeDamage(int damage) {
    health -= damage;
    if (health < 0) {
        health = 0; // Ensure health doesn't go below 0
    }
}

// Check if the enemy is defeated
bool Monster::isDefeated() const {
    return (health == 0);
}