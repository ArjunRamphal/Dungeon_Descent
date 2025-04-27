#include "Library.h"

// Constructor for the Library class. Calls the RoomBase constructor with the name "Library".
Library::Library() : RoomBase("Library") {
    // Set the type of this room to "Library".
    type = "Library";
}

// Returns the filename of the image to display for a Library room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the library image corresponding to the biome.
string Library::getImageFileName(int biome)
{
    if (biome == 0) {
        return "icelibrary.png";
    }
    else if (biome == 1) {
        return "junglelibrary.png";
    }
    else if (biome == 2) {
        return "desertlibrary.jpeg";
    }
    else if (biome == 3) {
        return "ghostlibrary.jpeg";
    }
    else if (biome == 4) {
        return "lavalibrary.jpg";
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns the filename of the text description to display for a Library room based on the current biome.
// @param biome An integer representing the current biome (0: Ice, 1: Jungle, 2: Desert, 3: Ghost, 4: Lava).
// @return A string containing the filename of the text file describing the library encounter in the given biome.
string Library::getTextFileName(int biome)
{
    if (biome == 0) {
        return "icelibrary.txt";
    }
    else if (biome == 1) {
        return "junglelibrary.txt";
    }
    else if (biome == 2) {
        return "desertlibrary.txt";
    }
    else if (biome == 3) {
        return "ghostlibrary.txt";
    }
    else if (biome == 4) {
        return "lavalibrary.txt";
    }
    return ""; // Return an empty string as a default or for error handling.
}

// Returns a vector of strings representing the choices available to the player in the Library room.
// The choices offered depend on the current room counter and biome.
// @param roomCounter The current room number the player is in.
// @param biome An integer representing the current biome.
// @return A vector of strings, where each string represents a book or topic the player can choose.
vector<string> Library::getChoices(int roomCounter, int biome)
{
    // Offer different sets of books based on the room counter.
    if (roomCounter == 1 || roomCounter == 2 || roomCounter == 3 || roomCounter == 7)
        return vector<string>{"The Art of Warfare", "The Whispering Winds", "The Ancient Lore", "The Heart of the Forest"};
    else if (roomCounter == 11 || roomCounter == 12 || roomCounter == 13) {
        // Offer biome-specific books on room counters 11, 12, and 13.
        if (biome == 2) {
            return vector<string>{"The Sands of Strategy", "The Silent Zephyrs", "The Hidden Glyphs", "The Oasis Eye"};
        }
        else if (biome == 3) {
            return vector<string>{"The Cyclops Might", "The Weeping Wolfman", "The Immortal Alucard", "The Eyes of the Raven"};
        }
    }
    else {
        return vector<string>{"The Princess Diary", "The Smoldering Whisper", "The Magma Script", "The Ashen Eye"};
    }
}

// Defines the outcome when the player chooses the first book.
// This implementation increases the character's strength.
// @param character A reference to the Character object interacting with the library.
// @return A string describing the effect of reading the first book.
string Library::getbtnChoice1(Character& character)
{
    // Special case for floor 3: also increase reputation and return a specific text.
    if (character.getFloor() == 3) {
        character.incXStat(0, 2); // Increase Strength by 2.
        character.incReputation(); // Increase Reputation.
        return character.incXStatDisplay(0, 2) + " lavalibrarydiary.txt"; // Return the stat increase message and a specific text file.
    }
    else {
        character.incXStat(0, 2); // Increase Strength by 2.
        return character.incXStatDisplay(0, 2); // Return the stat increase message.
    }
}

// Defines the outcome when the player chooses the second book.
// This implementation increases the character's agility.
// @param character A reference to the Character object interacting with the library.
// @return A string describing the effect of reading the second book.
string Library::getbtnChoice2(Character& character)
{
    character.incXStat(3, 2); // Increase Agility by 2.
    return character.incXStatDisplay(3, 2); // Return the stat increase message.
}

// Defines the outcome when the player chooses the third book.
// This implementation increases the character's wisdom.
// @param character A reference to the Character object interacting with the library.
// @return A string describing the effect of reading the third book.
string Library::getbtnChoice3(Character& character)
{
    character.incXStat(1, 2); // Increase Wisdom by 2.
    return character.incXStatDisplay(1, 2); // Return the stat increase message.
}

// Defines the outcome when the player chooses the fourth book.
// This implementation increases the character's observation.
// @param character A reference to the Character object interacting with the library.
// @return A string describing the effect of reading the fourth book.
string Library::getbtnChoice4(Character& character)
{
    character.incXStat(2, 2); // Increase Observation by 2.
    return character.incXStatDisplay(2, 2); // Return the stat increase message.
}