#pragma once
#ifndef TEST_H
#define TEST_H

#include <string>
#include <iostream>

class test {
private:
    int floor; // Floor level (1, 2, or 3)
    float health; // Current health of the enemy
    bool isBoss; // Whether the enemy is a boss

public:
   
    
    test(int floor, bool isBoss); // Constructor


    float getHealth() const; // Get the current health of the enemy
    
    bool isDefeated() const; // Check if the enemy is defeated

    template<typename T>
    void takeDamage(T damage); // Reduce health by damage amount

};

#endif
