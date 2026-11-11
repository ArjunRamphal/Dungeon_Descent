#pragma once
#ifndef LIBRARY_H
#define LIBRARY_H
#include <iostream>
#include <string>
#include <vector> // Include for using std::vector
#include "RoomBase.h" // Include the base class definition

// Class definition for the Library room type.
// Inherits from the RoomBase class.
class Library :
    public RoomBase
{
private:
    // A vector of strings to store the choices available to the player in the library.
    vector<string> choices;
public:
    // Default constructor for the Library class.
    Library();

    // Returns the filename of the image to display for the Library room based on the biome.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the image.
    string getImageFileName(int biome);

    // Returns the filename of the text description to display for the Library room based on the biome.
    // @param biome An integer representing the current biome of the game.
    // @return A string containing the filename of the text file.
    string getTextFileName(int biome);

    // Overrides the getChoices method from the base class to provide specific choices for the Library room.
    // @param roomCounter The current room number the player is in.
    // @param biome An integer representing the current biome of the game.
    // @return A vector of strings, where each string represents a choice the player can make in the library
    vector<string> getChoices(int roomCounter, int biome) override;

    // Overrides the getbtnChoice1 method from the base class to define the outcome of the first choice in the Library room.
    // @param character A reference to the Character object interacting with the library.
    // @return A string describing the result of the first choice.
    string getbtnChoice1(Character& character) override;

    // Overrides the getbtnChoice2 method from the base class to define the outcome of the second choice in the Library room.
    // @param character A reference to the Character object interacting with the library.
    // @return A string describing the result of the second choice.
    string getbtnChoice2(Character& character) override;

    // Overrides the getbtnChoice3 method from the base class to define the outcome of the third choice in the Library room.
    // @param character A reference to the Character object interacting with the library.
    // @return A string describing the result of the third choice.
    string getbtnChoice3(Character& character) override;

    // Overrides the getbtnChoice4 method from the base class to define the outcome of the fourth choice in the Library room.
    // @param character A reference to the Character object interacting with the library.
    // @return A string describing the result of the fourth choice.
    string getbtnChoice4(Character& character) override;
};
#endif