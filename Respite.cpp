#include "Respite.h"

// Constructor for the Respite class. Calls the RoomBase constructor with the name "Respite".
Respite::Respite() : RoomBase("Respite") {
}

// Returns the filename of the text description to display for a Respite room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the text file describing the respite in the given biome.
string Respite::getTextFileName(int biome)
{
    if (biome == 0) {
        return "icerespite.txt";
    }
    else if (biome == 1) {
        return "junglerespite.txt";
    }
    else if (biome == 2) {
        return "desertrespite.txt";
    }
    else if (biome == 3) {
        return "ghostrespite.txt";
    }
    else if (biome == 4) {
        return "lavarespite.txt";
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns the filename of the image to display for a Respite room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the respite image corresponding to the biome.
string Respite::getImageFileName(int biome)
{
    if (biome == 0) {
        return "icerespite.png";
    }
    else if (biome == 1) {
        return "junglerespite.png";
    }
    else if (biome == 2) {
        return "desertrespite.jpeg";
    }
    else if (biome == 3) {
        return "ghostrespite.jpeg";
    }
    else if (biome == 4) {
        return "lavarespite.jpg";
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns a vector of strings representing the choices available to the player in a Respite room.
// @param roomCounter The current room number (not directly used in this implementation).
// @param biome The current biome (not directly used in this implementation).
// @return A vector containing two choices: "Increase health" and "Search room".
vector<string> Respite::getChoices(int roomCounter, int biome)
{
    return vector<string>{"Drink from respite", "Search room"};
}

// Defines the outcome when the player chooses to "Increase health".
// This implementation increases the character's health.
// @param character A reference to the Character object interacting with the respite room.
// @return A string describing the effect of increasing health, potentially with additional text based on the floor.
string Respite::getbtnChoice1(Character& character)
{
    // Increase the character's health (stat at index 4) by 10.
    character.incXStat(4, 10);

    // Check if the player is on the first floor.
    if (character.getFloor() == 1) {
        // Increase the character's reputation.
        character.incReputation();
        // Return a message describing the health increase and a specific text for the first floor.
        return character.incXStatDisplay(4, 10) + " respitedrink.txt";
    }
    // For any other floor, just return the message describing the health increase.
    else {
        return character.incXStatDisplay(4, 10);
    }
}

// Defines the outcome when the player chooses to "Search room".
// This implementation increases all of the character's stats.
// @param character A reference to the Character object interacting with the respite room.
// @return A string describing the effect of searching the room (stat increase).
string Respite::getbtnChoice2(Character& character)
{
    // Increase all of the character's stats by 2 (scaled by the current floor).
    character.incStats(2);
    return character.incStatsDisplay(2);
}