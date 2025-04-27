#pragma once
#ifndef ENCOUNTER_H
#define ENCOUNTER_H
#include <iostream>
#include <string>
#include <vector> // Include for using std::vector
#include "RoomBase.h" // Include the base class definition

// Class definition for the Encounter room type.
// Inherits from the RoomBase class, indicating it's a type of room in the game where events or choices occur.
class Encounter : public RoomBase
{
private:
    // A vector of strings to store the choices available to the player during this encounter.
    vector<string> choices;
public:
    // Default constructor for the Encounter class.
    Encounter();

    // Returns the filename of the image to display for this Encounter room based on the biome.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the image.
    string getImageFileName(int biome);

    // Returns the filename of the text description to display for this Encounter room based on the biome.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the text file.
    string getTextFileName(int biome);

    // Overrides the getChoices method from the base class to provide specific choices for this Encounter room.
    // @param roomCounter The current room number the player is in.
    // @param biome An integer representing the current biome of the game.
    // @return A vector of strings, where each string represents a choice the player can make in this encounter.
    vector<string> getChoices(int roomCounter, int biome) override;

    // Overrides the getbtnChoice1 method from the base class to define the outcome of the first choice in this Encounter room.
    // @param character A reference to the Character object interacting with the encounter.
    // @return A string describing the result of the first choice.
    string getbtnChoice1(Character& character) override;

    // Overrides the getbtnChoice2 method from the base class to define the outcome of the second choice in this Encounter room.
    // @param character A reference to the Character object interacting with the encounter.
    // @return A string describing the result of the second choice.
    string getbtnChoice2(Character& character) override;
};
#endif