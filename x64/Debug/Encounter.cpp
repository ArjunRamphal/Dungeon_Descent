#include "Encounter.h"

// Constructor for the Encounter class. Calls the RoomBase constructor with the name "Encounter".
Encounter::Encounter() : RoomBase("Encounter")
{
}

// Returns the filename of the image to display for this Encounter room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the encounter image corresponding to the biome.
string Encounter::getImageFileName(int biome)
{
    if ((biome == 0) || ((biome == 1))) {
        return "prison.jpeg"; // Use the prison image for Ice and Jungle biomes.
    }
    else if (biome == 2) {
        return "desertbutler.jpeg"; // Use the desert butler image for the Desert biome.
    }
    else if (biome == 3) {
        return "ghostbutler.jpeg"; // Use the ghost butler image for the Ghost biome.
    }
    else if (biome == 4) {
        return "lavaminions.jpg"; // Use the lava minions image for the Lava biome.
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns the filename of the text description to display for this Encounter room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the text file describing the encounter in the given biome.
string Encounter::getTextFileName(int biome)
{
    if (biome == 0) {
        return "iceprison.txt"; // Text for the ice prison encounter.
    }
    else if (biome == 1) {
        return "jungleprison.txt"; // Text for the jungle prison encounter.
    }
    else if (biome == 2) {
        return "desertbutler.txt"; // Text for the desert butler encounter.
    }
    else if (biome == 3) {
        return "ghostbutler.txt"; // Text for the ghost butler encounter.
    }
    else if (biome == 4) {
        return "lavaminion.txt"; // Text for the lava minion encounter.
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns a vector of strings representing the choices available to the player during this encounter.
// The choices offered depend on the current room counter and biome.
// @param roomCounter The current room number the player is in.
// @param biome An integer representing the current biome.
// @return A vector of strings, where each string represents a choice the player can make.
vector<string> Encounter::getChoices(int roomCounter, int biome)
{
    if (roomCounter == 4)
        return vector<string>{"Spare warrior", "Kill warrior"}; // Choices for the warrior encounter.
    else if (roomCounter == 16) {
        return vector<string>{"Save butler", "Take vial"}; // Choices for the butler encounter.
    }
    else {
        return vector<string>{"Threaten minions", "Kill minions"}; // Choices for the lava minions encounter.
    }
}

// Defines the outcome when the player chooses the first option in the encounter.
// The effect depends on the current floor and biome, typically increasing reputation.
// @param character A reference to the Character object interacting with the encounter.
// @return A string describing the result of the first choice.
string Encounter::getbtnChoice1(Character& character)
{
    if (character.getFloor() == 1) {
        if (character.getBiome() == 0) {
            character.incReputation(); // Increase reputation for sparing the warrior in the ice biome.
            return " iceprisonfree.txt"; // Return specific text for this outcome.
        }

        else {
            character.incReputation(); // Increase reputation for sparing the warrior in the jungle biome.
            return " jungleprisonfree.txt"; // Return specific text for this outcome.
        }
    }
    else if (character.getFloor() == 2) {
        if (character.getBiome() == 2) {
            character.incReputation(); // Increase reputation for saving the butler in the desert biome.
            return " desertbutlerfree.txt"; // Return specific text for this outcome.
        }
        else {
            character.incReputation(); // Increase reputation for saving the butler in the ghost biome.
            return " ghostbutlerfree.txt"; // Return specific text for this outcome.
        }
    }
    else {
        character.incReputation(); // Increase reputation for threatening the minions in the lava biome.
        return " lavaminionfree.txt"; // Return specific text for this outcome.
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Defines the outcome when the player chooses the second option in the encounter.
// The effect depends on the current floor, typically increasing stats.
// @param character A reference to the Character object interacting with the encounter.
// @return A string describing the result of the second choice, including stat increases.
string Encounter::getbtnChoice2(Character& character)
{
    if (character.getFloor() == 1) {
        character.incStats(1); // Increase all stats for killing the warrior.
        return "You kill the warrior. " + character.incStatsDisplay(1); // Return a message describing the action and stat increase.
    }
    else if (character.getFloor() == 2) {
        character.incStats(1.5); // Increase all stats for taking the vial.
        return "You take the vial and leave the butler to die. " + character.incStatsDisplay(1); // Return a message describing the action and stat increase.
    }
    else {
        character.incStats(2); // Increase all stats for killing the minions.
        return "You kill the minions. " + character.incStatsDisplay(1); // Return a message describing the action and stat increase.
    };
    return ""; // Return an empty string as a default or for error handling.
}