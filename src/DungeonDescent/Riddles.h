#pragma once
#ifndef RIDDLES_H
#define RIDDLES_H
#include <iostream>
#include <string>
#include <vector> // Include for using std::vector
#include "Character.h"

using namespace std;

// Class definition for the Riddles encountered in the game.
class Riddles {
private:
    // A vector of strings to store the riddles themselves.
    vector<string> riddles;

    // A vector of vectors of strings. Each inner vector contains the possible answer choices for a corresponding riddle.
    vector<vector<string>> answers;

    // A vector of strings to store the correct answer for each corresponding riddle.
    vector<string> answerCorrect;

	Character* character; // Reference to the Character object, likely to access character stats or abilities.
public:
    // Constructor for the Riddles class. Takes the current floor as input, likely to determine the difficulty or type of riddles.
    // @param floor An integer representing the current floor level of the game.
    Riddles(Character& character);

    // Getter method to retrieve the vector of riddles.
    // @return A vector of strings, where each string is a riddle.
    vector<string> getRiddles();

    // Getter method to retrieve the vector of answer choices for each riddle.
    // @return A vector of vectors of strings, where each inner vector contains the answer choices for a riddle.
    vector<vector<string>> getAnswers();

    // Getter method to retrieve the vector of correct answers for each riddle.
    // @return A vector of strings, where each string is the correct answer to a riddle.
    vector<string> getAnswerCorrect() { return answerCorrect; };
};

#endif // RIDDLES_H