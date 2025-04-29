#include "RoomBase.h"
#include <iostream>
#include <string>
#include <vector>

// Constructor for the RoomBase class that takes a room name.
// Currently empty, derived classes should handle their specific initialization.
RoomBase::RoomBase(const std::string& name) {
}

// Getter method to retrieve the type of the room.
// @return A string representing the room type (e.g., "Battle", "Shop").
string RoomBase::getType() {
    return type;
}

// Virtual method to get the choices available to the player in this room.
// The base implementation returns an empty vector, indicating no choices.
// Derived classes override this to provide room-specific choices.
// @param roomCounter The current room number.
// @param biome The current biome.
vector<string> RoomBase::getChoices(int roomCounter, int biome)
{
    return vector<string>();
}

// Virtual method to get the filename of the image for this room.
// The base implementation returns the default imageFileName, which is initially empty.
// Derived classes override this to provide room-specific image filenames.
// @param biome The current biome.
// @return A string containing the filename of the image.
string RoomBase::getImageFileName(int biome)
{
    return imageFileName;
}

// Virtual method to get the filename of the text description for this room.
// The base implementation returns the default textFileName, which is initially empty.
// Derived classes override this to provide room-specific text filenames.
// @param biome The current biome.
// @return A string containing the filename of the text file.
string RoomBase::getTextFileName(int biome)
{
    return textFileName;
}

// Default constructor for the RoomBase class.
// Currently empty, derived classes should handle their specific initialization.
RoomBase::RoomBase() {
}

// Virtual method to handle the action when the first choice button is pressed.
// The base implementation returns an empty string, indicating no action.
// Derived classes override this to implement the specific logic for the first choice.
// @param character A reference to the Character object interacting with the room.
string RoomBase::getbtnChoice1(Character& character)
{
    return string();
}

// Virtual method to handle the action when the second choice button is pressed.
// The base implementation returns an empty string, indicating no action.
// Derived classes override this to implement the specific logic for the second choice.
// @param character A reference to the Character object interacting with the room.
string RoomBase::getbtnChoice2(Character& character)
{
    return string();
}

// Virtual method to handle the action when the third choice button is pressed.
// The base implementation returns an empty string, indicating no action.
// Derived classes override this to implement the specific logic for the third choice.
// @param character A reference to the Character object interacting with the room.
string RoomBase::getbtnChoice3(Character& character)
{
    return string();
}

// Virtual method to handle the action when the fourth choice button is pressed.
// The base implementation returns an empty string, indicating no action.
// Derived classes override this to implement the specific logic for the fourth choice.
// @param character A reference to the Character object interacting with the room.
string RoomBase::getbtnChoice4(Character& character)
{
    return string();
}