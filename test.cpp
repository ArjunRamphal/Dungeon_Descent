#pragma once
#include "test.h"

// Constructor
test::test(int floor, bool isBoss) : floor(floor), isBoss(isBoss) {
    // Calculate base health based on floor level
    float baseHealth = 10 + (floor * 2);

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
float test::getHealth() const {
    return health;
}

// Reduce health by damage amount
void test::takeDamage(int damage) {
    health -= damage;
    if (health < 0) {
        health = 0; // Ensure health doesn't go below 0
    }
}

// Check if the enemy is defeated
bool test::isDefeated() const {
    return (health <= 0);
}