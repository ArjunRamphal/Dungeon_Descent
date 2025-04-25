#pragma 
#include "Battle.h"
#include <random>
#include <iostream>
int extra;

Battle::Battle(bool isBoss, Character& player) {
    // Create the player and enemy object
    enemy = new test(player.floor, isBoss);
    extra = Calculate_Extra_Strikes(player.statValue[3]);
    // Set the number of strikes (including extra strikes)
    strikesRemaining = Total_Strikes();  // Set the number of strikes
}
void Battle::attack(Character& player) {
    if (strikesRemaining > 0 && !enemy->isDefeated()) {
        // Perform the attack
        if (isCrit_Strike(player.statValue[5])) {
            enemy->takeDamage(player.statValue[0] * 2);  // Critical strike
        }
        else {
            enemy->takeDamage(player.statValue[0] * 1);  // Normal attack
        }
    }

    // Decrease the number of strikes remaining
    strikesRemaining--;

}

int Battle::Calculate_Extra_Strikes(int agility) {
    return agility / 2;  // Calculate extra strikes based on agility
}

bool Battle::isCrit_Strike(int accuracy) {
    int max = 25;
    int AccuracyHold = 0;
    if (accuracy < 0) { return false; }
    else if (accuracy >= 25) { return true; }

    srand(time(0));
    int roll = rand() % (max - accuracy) + 1; // random number between 1 and 100
    return roll == 1;


}

int Battle::Total_Strikes() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(3, 5);
    int random_number = dist(gen);

    return random_number + extra;  // Example player agility value
}

bool Battle::isBattleFinished() {
    return (strikesRemaining == 0 || enemy->isDefeated());  // Battle is finished if no strikes are left or enemy is defeated
}

string Battle::getImageFileName(int biome)
{
    if (biome == 0) {
        return "icebattle.png";
    }
    else if (biome == 1) {
        return "junglebattle.png";
    }
    else if (biome == 2) {
        return "desertbattle.jpeg";
    }
    else if (biome == 3) {
        return "ghostbattle1.jpeg";
    }
    else if (biome == 4) {
        return "lavabattle1.jpg";
    }
}

string Battle::getTextFileName(int biome)
{
    if (biome == 0) {
        return "icebattle.txt";
    }
    else if (biome == 1) {
        return "junglebattle.txt";
    }
    else if (biome == 2) {
        return "desertbattle.txt";
    }
    else if (biome == 3) {
        return "ghostbattle.txt";
    }
    else if (biome == 4) {
        return "lavabattle.txt";
    }
}