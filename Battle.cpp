#pragma 
#include "Battle.h"
#include <random>
#include <iostream>

Battle::Battle(bool isBoss, Character& player) {
    // Create the player and enemy object
    this->isBoss = isBoss; // Set Boss to true or false
    character = &player;
    enemy = new Monster(player.getFloor(), isBoss);
    extra = Calculate_Extra_Strikes(player.getStats()[3]);
    // Set the number of strikes (including extra strikes)
    strikesRemaining = Total_Strikes();  // Set the number of strikes
    type = "Battle";
}

bool Battle::attack(Character& player) {
    if (strikesRemaining > 0 && !enemy->isDefeated()) {
        // Decrease the number of strikes remaining
        strikesRemaining--;

        // Perform the attack
        if (isCrit_Strike(player.getStats()[5])) {
            enemy->takeDamage(player.getStats()[0] * 2);  // Critical strike
            return true;
        }
        else {
            enemy->takeDamage(player.getStats()[0] * 1);  // Normal attack
            return false;
        }
    }
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

int Battle::getStrikesRemaining() const
{
    return strikesRemaining;
}

string Battle::getImageFileName(int biome)
{
    if (character->getRoomCounter() % 10 == 9) {
        if (biome == 0) {
            return "iceboss.jpg";
        }
        else if (biome == 1) {
            return "jungleboss.jpg";
        }
        else if (biome == 2) {
            return "desertboss.jpeg";
        }
        else if (biome == 3) {
            return "ghostboss.jpeg";
        }
        else if (biome == 4) {
            return "lavaboss.jpg";
        }
    }
    else {
        if (biome == 0) {
            if (character->getRoomCounter() == 5) {
                return "icebattle2.jpg";
            }
            else {
                return "icebattle.jpg";
            }
        }
        else if (biome == 1) {
            if (character->getRoomCounter() == 5) {
                return "junglebattle2.jpg";
            }
            else {
                return "junglebattle.jpg";
            }
        }
        else if (biome == 2) {
            if (character->getRoomCounter() == 18) {
                return "desertbattle2.jpg";
            }
            else {
                return "desertbattle.jpg";
            }
        }
        else if (biome == 3) {
            if (character->getRoomCounter() == 18) {
                return "ghostbattle2.jpeg";
            }
            else {
                return "ghostbattle1.jpeg";
            }
        }
        else if (biome == 4) {
            if (character->getRoomCounter() == 25) {
                return "lavabattle2.jpg";
            }
            else {
                return "lavabattle1.jpg";
            }
        }
    }
}

string Battle::getTextFileName(int biome)
{
    if (character->getRoomCounter() % 10 == 9) {
        if (biome == 0) {
            return "iceboss.txt";
        }
        else if (biome == 1) {
            return "jungleboss.txt";
        }
        else if (biome == 2) {
            return "desertboss.txt";
        }
        else if (biome == 3) {
            return "ghostboss.txt";
        }
        else if (biome == 4) {
            return "lavaboss.txt";
        }
    }
    else {
        if (biome == 0) {
            if (character->getRoomCounter() == 5) {
                return "icebattle2.txt";
            }
            else {
                return "icebattle.txt";
            }
        }
        else if (biome == 1) {
            if (character->getRoomCounter() == 5) {
                return "junglebattle2.txt";
            }
            else {
                return "junglebattle.txt";
            }
        }
        else if (biome == 2) {
            if (character->getRoomCounter() == 18) {
                return "desertbattle.txt";
            }
            else {
                return "desertbattle2.txt";
            }
        }
        else if (biome == 3) {
            if (character->getRoomCounter() == 18) {
                return "ghostbattle2.txt";
            }
            else {
                return "ghostbattle.txt";
            }
        }
        else if (biome == 4) {
            if (character->getRoomCounter() == 25) {
                return "lavabattle2.txt";
            }
            else {
                return "lavabattle.txt";
            }
        }
    }
}

Monster* Battle::getEnemy() const
{
    return enemy;
}

vector<string> Battle::getChoices(int roomCounter, int biome)
{
    if (isBoss) {
        return vector<string>{"Confront"};
    }
    else {
        return vector<string>{"Confront", "Sneak past"};
    }
    
}

string Battle::getbtnChoice2(Character& character)
{
    return "You quietly sneak past the monster. Your presence goes unnoticed. You continue through the dungeon.";
}
