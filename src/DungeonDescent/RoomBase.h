#ifndef ROOMBASE_H
#define ROOMBASE_H
#include <iostream>
#include <string>
#include <vector>
#include "Character.h" // Include the Character class definition

using namespace std;

// Base class for all room types in the game.
// Provides common functionality and virtual methods for derived room classes.
class RoomBase {
protected:
    // A vector of strings to store the choices available to the player in this room.
    vector<string> choices;

    // A string to identify the type of the room (e.g., "Battle", "Shop", "Chest").
    string type;

    // A string to store the filename of the image associated with this room.
    string imageFileName;

    // A string to store the filename of the text description associated with this room.
    string textFileName;
public:

    // Constructor for the RoomBase class that takes a room name.
    // @param name The name of the room type.
    RoomBase(const std::string& name);

    // Default constructor for the RoomBase class.
    RoomBase();

    // Getter method to retrieve the type of the room.
    // @return A string representing the room type.
    string getType();

    // Virtual method to get the choices available to the player in this room.
    // Derived classes will override this to provide room-specific choices.
    // @param roomCounter The current room number the player is in (can be used to vary choices).
    // @param biome An integer representing the current biome of the game (can be used to vary choices).
    // @return A vector of strings, where each string is a choice.
    virtual vector<string> getChoices(int roomCounter, int biome);

    // Virtual method to get the filename of the image for this room.
    // Derived classes will override this to provide room-specific images.
    // @param biome An integer representing the current biome of the game (can be used to select biome-specific images).
    // @return A string containing the filename of the image.
    virtual string getImageFileName(int biome);

    // Virtual method to get the filename of the text description for this room.
    // Derived classes will override this to provide room-specific text.
    // @param biome An integer representing the current biome of the game (can be used to select biome-specific text).
    // @return A string containing the filename of the text file.
    virtual string getTextFileName(int biome);

    // Virtual method to handle the action when the first choice button is pressed.
    // Derived classes will override this to implement the specific logic for the first choice.
    // @param character A reference to the Character object interacting with the room.
    // @return A string describing the outcome of the first choice.
    virtual string getbtnChoice1(Character& character);

    // Virtual method to handle the action when the second choice button is pressed.
    // Derived classes will override this to implement the specific logic for the second choice.
    // @param character A reference to the Character object interacting with the room.
    // @return A string describing the outcome of the second choice.
    virtual string getbtnChoice2(Character& character);

    // Virtual method to handle the action when the third choice button is pressed.
    // Derived classes will override this to implement the specific logic for the third choice.
    // @param character A reference to the Character object interacting with the room.
    // @return A string describing the outcome of the third choice.
    virtual string getbtnChoice3(Character& character);

    // Virtual method to handle the action when the fourth choice button is pressed.
    // Derived classes will override this to implement the specific logic for the fourth choice.
    // @param character A reference to the Character object interacting with the room.
    // @return A string describing the outcome of the fourth choice.
    virtual string getbtnChoice4(Character& character);
};

#endif // ROOMBASE_H