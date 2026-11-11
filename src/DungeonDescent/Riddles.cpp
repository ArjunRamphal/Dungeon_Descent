#include "Riddles.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>  // For file input/output
#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For time() to seed the random number generator

using namespace std;

// Constructor for the Riddles class. Loads riddles and answers from different files based on the current floor.
Riddles::Riddles(Character& character)
{
	this->character = &character; // Initialize the character reference.
    // Load easy riddles if the current floor is 1.
    if (character.getFloor() == 1) {
        ifstream file("textfiles/riddles/1/EasyRiddles.txt"); // Open the file containing easy riddles and answers.
        if (!file) {
            cerr << "Error opening file!\n"; // Output an error message if the file cannot be opened.
        }

        string line;         // Variable to store each line read from the file.
        int lineNumber = 0;  // Counter for the current line number (not directly used in the logic).

        int num = 30;        // Initial number of riddles to choose from.
        vector<string> tempAnswers; // Temporary storage for all answers read from the file.
        vector<string> tempRiddles; // Temporary storage for all riddles read from the file.

        // Read riddles and their corresponding three answer choices from the file.
        while (getline(file, line)) {
            // Generate three distinct random numbers between 0 and 2 (inclusive)
            // to determine the order of the answer choices later.
            int randomNum1 = rand() % 3;
            int randomNum2;
            do {
                randomNum2 = rand() % 3;
            } while (randomNum2 == randomNum1);
            int randomNum3;
            do {
                randomNum3 = rand() % 3;
            } while ((randomNum3 == randomNum2) || (randomNum3 == randomNum1));

            lineNumber++;
            tempRiddles.push_back(line); // Store the riddle.

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line); // Store the first answer choice  (which is the correct one in the file structure).

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line); // Store the second answer choice.

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line); // Store the third answer choice.
        }

        // Randomly select riddles and their answers from the temporary vectors.
        while (!tempRiddles.empty()) {
            srand(time(0)); // Seed the random number generator with the current time to ensure different sequences.
            int randomNumber = (rand() % num); // Generate a random index within the current range of available riddles.
            num--; // Decrease the number of available riddles after one is selected.

            riddles.push_back(tempRiddles.at(randomNumber)); // Add the randomly selected riddle to the final riddles vector.
            tempRiddles.erase(tempRiddles.begin() + randomNumber); // Remove the selected riddle from the temporary vector.

            vector<string> answerSet; // Create a vector to store the answer choices for the current riddle.

            answerSet.push_back(tempAnswers.at((randomNumber * 3) + 2)); // Add the third answer
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 2);

            answerSet.push_back(tempAnswers.at((randomNumber * 3) + 1)); // Add the second answer
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 1);

            answerSet.push_back(tempAnswers.at(randomNumber * 3)); // Add the first answer (correct answer)
            answerCorrect.push_back(tempAnswers.at((randomNumber * 3))); // Store the correct answer separately.
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)));

            answers.push_back(answerSet); // Add the set of answer choices to the final answers vector.
        }
    }
    // Load medium riddles if the current floor is 2. The logic is the same as for easy riddles, but with a different file and number of riddles.
    else if (character.getFloor() == 2) {
        ifstream file("textfiles/riddles/2/MediumRiddles.txt");
        if (!file) {
            cerr << "Error opening file!\n";
        }

        string line;
        int lineNumber = 0;

        int num = 37;
        vector<string> tempAnswers;
        vector<string> tempRiddles;

        while (getline(file, line)) {
            int randomNum1 = rand() % 3;
            int randomNum2;
            do {
                randomNum2 = rand() % 3;
            } while (randomNum2 == randomNum1);
            int randomNum3;
            do {
                randomNum3 = rand() % 3;
            } while ((randomNum3 == randomNum2) || (randomNum3 == randomNum1));

            lineNumber++;
            tempRiddles.push_back(line);

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line);

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line);

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line);
        }

        while (!tempRiddles.empty()) {
            srand(time(0)); // Seed random number generator
            int randomNumber = (rand() % num); // Range [0, 37]
            num--;

            riddles.push_back(tempRiddles.at(randomNumber));

            tempRiddles.erase(tempRiddles.begin() + randomNumber);

            vector<string> answerSet;

            answerSet.push_back(tempAnswers.at((randomNumber * 3) + 2)); // Add the third answer
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 2);

            answerSet.push_back(tempAnswers.at((randomNumber * 3) + 1));
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 1);

            answerSet.push_back(tempAnswers.at(randomNumber * 3));
            answerCorrect.push_back(tempAnswers.at((randomNumber * 3)));
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)));

            answers.push_back(answerSet);
        }
    }
    // Load hard riddles if the current floor is 3. The logic is the same as for easy and medium riddles, but with a different file and number of riddles.
    else {
        ifstream file("textfiles/riddles/3/HardRiddles.txt");
        if (!file) {
            cerr << "Error opening file!\n";
        }

        string line;
        int lineNumber = 0;

        int num = 35;
        vector<string> tempAnswers;
        vector<string> tempRiddles;

        while (getline(file, line)) {
            int randomNum1 = rand() % 3;
            int randomNum2;
            do {
                randomNum2 = rand() % 3;
            } while (randomNum2 == randomNum1);
            int randomNum3;
            do {
                randomNum3 = rand() % 3;
            } while ((randomNum3 == randomNum2) || (randomNum3 == randomNum1));

            lineNumber++;
            tempRiddles.push_back(line);

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line);

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line);

            lineNumber++;
            std::getline(file, line);
            tempAnswers.push_back(line);
        }

        while (!tempRiddles.empty()) {
            srand(time(0)); // Seed random number generator
            int randomNumber = (rand() % num); // Range [0, 35]
            num--;

            riddles.push_back(tempRiddles.at(randomNumber));

            tempRiddles.erase(tempRiddles.begin() + randomNumber);

            vector<string> answerSet;

            answerSet.push_back(tempAnswers.at((randomNumber * 3) + 2)); // Add the third answer
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 2);

            answerSet.push_back(tempAnswers.at((randomNumber * 3) + 1));
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 1);

            answerSet.push_back(tempAnswers.at(randomNumber * 3));
            answerCorrect.push_back(tempAnswers.at((randomNumber * 3)));
            tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)));

            answers.push_back(answerSet);
        }
    }
}

vector<string> Riddles::getRiddles()
{
    return riddles;
}

vector<vector<string>> Riddles::getAnswers()
{
    return answers;
}
