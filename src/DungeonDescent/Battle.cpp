#include "Battle.h"
#include <random>
#include <iostream>

// Constructor for the Battle class. Initializes a battle encounter.
Battle::Battle(bool isBoss, Character& player) {
    // Determine if this battle is against a boss monster.
    this->isBoss = isBoss;

    // Set the player for this battle.
    character = &player;

    // Create a new monster for this battle, scaling its difficulty based on the player's current floor and whether it's a boss.
    enemy = new Monster(player.getFloor(), isBoss);

    // Calculate the number of extra strikes the player gets based on their agility.
    extra = Calculate_Extra_Strikes(player.getStats()[3]);

    // Set the total number of strikes available in this battle.
    strikesRemaining = Total_Strikes();

    // Identify the type of event this object represents.
    type = "Battle";

    srand(time(0));
}

// Simulates an attack by the player on the enemy.
bool Battle::attack(Character& player) {
    // Check if there are any strikes remaining and if the enemy is still alive.
    if (strikesRemaining > 0 && !enemy->isDefeated()) {
        // Decrement the number of strikes available.
        strikesRemaining--;

        // Determine if the attack is a critical strike based on the player's accuracy.
        if (isCrit_Strike(player.getStats()[5])) {
            // Deal double damage for a critical strike.
            enemy->takeDamage(player.getStats()[0] * 2);
            return true; // Indicate a critical strike occurred.
        }
        else {
            // Deal normal damage for a regular attack.
            enemy->takeDamage(player.getStats()[0] * 1);
            return false; // Indicate a normal attack occurred.
        }
    }
    return false; // No attack occurred if no strikes are left or the enemy is defeated.
}

// Calculates and applies the damage taken by the player from the enemy.
float Battle::damageTaken()
{
    float damage;
    // Adjust the monster's attack power based on whether it's a boss.
    if (isBoss) {
        damage = enemy->calculateMonsterAttack(*character, 2.5); // Bosses deal more damage.
    }
    else {
        damage = enemy->calculateMonsterAttack(*character, 2);   // Regular monsters deal normal damage.
    }
    // Make the player take the calculated damage.
    character->takeDamage(damage);
    return damage; // Return the amount of damage taken.
}

// Calculates the number of extra strikes a player gets based on their agility stat.
int Battle::Calculate_Extra_Strikes(int agility) {
    return agility / 3; // Higher agility grants more extra strikes.
}

// Determines if the player's attack is a critical strike based on their accuracy.
bool Battle::isCrit_Strike(int accuracy) {
    if (accuracy < 0) return false;
    if (accuracy >= 35) return true;

    int roll = rand() % 35; // random number between 0 and 35
    return roll < accuracy; // if roll is less than accuracy, it's a crit
}

// Calculates the total number of strikes the player has in this battle.
int Battle::Total_Strikes() {
    std::random_device rd;   // Obtain a random seed from the system.
    std::mt19937 gen(rd());  // Initialize a Mersenne Twister pseudo-random number generator.
    std::uniform_int_distribution<> dist(3, 5); // Define a uniform distribution between 3 and 5 (inclusive).
    int random_number = dist(gen); // Generate a random number of base strikes.

    return random_number + extra; // The total strikes are the base strikes plus any extra strikes from agility.
}

// Checks if the current battle has concluded.
bool Battle::isBattleFinished() {
    // The battle ends if no strikes are remaining, the enemy is defeated, or the player's health reaches zero.
    return (strikesRemaining == 0 || enemy->isDefeated() || (character->getHealth() == 0));
}

// Returns the number of strikes remaining in the current battle.
int Battle::getStrikesRemaining() const
{
    return strikesRemaining;
}

// Returns the filename of the image to display for the current battle based on the biome and room counter.
string Battle::getImageFileName(int biome)
{
    // Check if it's a boss battle
    if (character->getRoomCounter() % 10 == 9) {
        // Return the appropriate boss image based on the current biome.
        if (biome == 0) {
            return "images/biomes/ice/battle/iceboss.jpg";
        }
        else if (biome == 1) {
            return "images/biomes/jungle/battle/jungleboss.jpg";
        }
        else if (biome == 2) {
            return "images/biomes/desert/battle/desertboss.jpeg";
        }
        else if (biome == 3) {
            return "images/biomes/ghost/battle/ghostboss.jpeg";
        }
        else if (biome == 4) {
            return "images/biomes/lava/battle/lavaboss.jpg";
        }
    }
    // If it's not a boss battle, return the regular battle image based on the biome and the room counter.
    else {
        if (biome == 0) {
            if (character->getRoomCounter() == 5) {
                return "images/biomes/ice/battle/icebattle2.jpg";
            }
            else {
                return "images/biomes/ice/battle/icebattle.jpg";
            }
        }
        else if (biome == 1) {
            if (character->getRoomCounter() == 5) {
                return "images/biomes/jungle/battle/junglebattle2.jpg";
            }
            else {
                return "images/biomes/jungle/battle/junglebattle.jpg";
            }
        }
        else if (biome == 2) {
            if (character->getRoomCounter() == 18) {
                return "images/biomes/desert/battle/desertbattle2.jpg";
            }
            else {
                return "images/biomes/desert/battle/desertbattle.jpg";
            }
        }
        else if (biome == 3) {
            if (character->getRoomCounter() == 18) {
                return "images/biomes/ghost/battle/ghostbattle2.jpeg";
            }
            else {
                return "images/biomes/ghost/battle/ghostbattle1.jpeg";
            }
        }
        else if (biome == 4) {
            if (character->getRoomCounter() == 25) {
                return "images/biomes/lava/battle/lavabattle2.jpg";
            }
            else {
                return "images/biomes/lava/battle/lavabattle1.jpg";
            }
        }
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns the filename of the text description to display for the current battle.
string Battle::getTextFileName(int biome)
{
    // Check if it's a boss battle.
    if (character->getRoomCounter() % 10 == 9) {
        // Return the appropriate boss text file based on the biome.
        if (biome == 0) {
            return "textfiles/biomes/ice/battle/iceboss.txt";
        }
        else if (biome == 1) {
            return "textfiles/biomes/jungle/battle/jungleboss.txt";
        }
        else if (biome == 2) {
            return "textfiles/biomes/desert/battle/desertboss.txt";
        }
        else if (biome == 3) {
            return "textfiles/biomes/ghost/battle/ghostboss.txt";
        }
        else if (biome == 4) {
            return "textfiles/biomes/lava/battle/lavaboss.txt";
        }
    }
    // If it's not a boss battle, return the regular battle text file based on the biome and potentially the room counter.
    else {
        if (biome == 0) {
            if (character->getRoomCounter() == 5) {
                return "textfiles/biomes/ice/battle/icebattle2.txt";
            }
            else {
                return "textfiles/biomes/ice/battle/icebattle.txt";
            }
        }
        else if (biome == 1) {
            if (character->getRoomCounter() == 5) {
                return "textfiles/biomes/jungle/battle/junglebattle2.txt";
            }
            else {
                return "textfiles/biomes/jungle/battle/junglebattle.txt";
            }
        }
        else if (biome == 2) {
            if (character->getRoomCounter() == 18) {
                return "textfiles/biomes/desert/battle/desertbattle.txt";
            }
            else {
                return "textfiles/biomes/desert/battle/desertbattle2.txt";
            }
        }
        else if (biome == 3) {
            if (character->getRoomCounter() == 18) {
                return "textfiles/biomes/ghost/battle/ghostbattle2.txt";
            }
            else {
                return "textfiles/biomes/ghost/battle/ghostbattle.txt";
            }
        }
        else if (biome == 4) {
            if (character->getRoomCounter() == 25) {
                return "textfiles/biomes/lava/battle/lavabattle2.txt";
            }
            else {
                return "textfiles/biomes/lava/battle/lavabattle.txt";
            }
        }
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns a pointer to the enemy monster in this battle.
Monster* Battle::getEnemy() const
{
    return enemy;
}

// Returns whether this battle is against a boss monster.
bool Battle::getIsBoss()
{
    return isBoss;
}

// Returns a vector of strings representing the choices available to the player in this battle.
vector<string> Battle::getChoices(int roomCounter, int biome)
{
    // If it's a boss battle, the only option is to confront.
    if (isBoss) {
        return vector<string>{"Confront"};
    }
    // In a regular battle, the player can choose to confront or try to sneak past.
    else {
        return vector<string>{"Confront", "Sneak past"};
    }

}

// Returns the text displayed when the player chooses to sneak past the monster.
string Battle::getbtnChoice2(Character& character)
{
    return "You quietly sneak past the monster. Your presence goes unnoticed. You continue through the dungeon.";
}