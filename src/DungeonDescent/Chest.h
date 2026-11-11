#pragma once
#ifndef CHEST_H
#define CHEST_H
#include <iostream>
#include <string>
#include <vector> // Include for using std::vector
#include "RoomBase.h" // Include the base class definition

// Class definition for the Chest room type.
// Inherits from the RoomBase class.
class Chest :
    public RoomBase
{

private:
public:
    // Default constructor for the Chest class.
    Chest();

    // Returns the filename of the image to display for a Chest room based on the biome.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the image.
    string getImageFileName(int biome);

    // Returns the filename of the text description to display for a Chest room based on the biome.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the text file.
    string getTextFileName(int biome);

    // Overrides the getChoices method from the base class to provide specific choices for a Chest room.
    // @param roomCounter The current room number the player is in.
    // @param biome An integer representing the current biome of the game.
    // @return A vector of strings, where each string represents a choice the player can make in this room.
    vector<string> getChoices(int roomCounter, int biome) override;

    // Overrides the getbtnChoice1 method from the base class to define the outcome of the first choice in a Chest room.
    // @param character A reference to the Character object interacting with the chest.
    // @return A string describing the result of the first choice.
    string getbtnChoice1(Character& character) override;
};

#endif // CHEST_H