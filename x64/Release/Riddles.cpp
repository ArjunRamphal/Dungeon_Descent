#include "Riddles.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace std;

Riddles::Riddles(int floor)
{
	if (floor == 1) {
		ifstream file("EasyRiddles.txt");
		if (!file) {
			cerr << "Error opening file!\n";
		}

		string line;
		int lineNumber = 0;

		int num = 18;
		vector<string> tempAnswers;
		vector<string> tempRiddles;

		while (getline(file, line)) {
			//Random index generator for answer position
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
			int randomNumber = (rand() % num); // Range [0, 50]
			num--;

			riddles.push_back(tempRiddles.at(randomNumber));

			tempRiddles.erase(tempRiddles.begin() + randomNumber);

			vector<string> answerSet;
			answerSet.push_back(tempAnswers.at((randomNumber * 3) + 2));
			tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 2);

			answerSet.push_back(tempAnswers.at((randomNumber * 3) + 1));
			tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 1);

			answerSet.push_back(tempAnswers.at(randomNumber * 3));
			answerCorrect.push_back(tempAnswers.at((randomNumber * 3)));
			tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)));

			answers.push_back(answerSet);
		}
	}
	else if (floor == 2) {
		ifstream file("MediumRiddles.txt");
		if (!file) {
			cerr << "Error opening file!\n";
		}

		string line;
		int lineNumber = 0;

		int num = 29;
		vector<string> tempAnswers;
		vector<string> tempRiddles;

		while (getline(file, line)) {
			//Random index generator for answer position
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
			int randomNumber = (rand() % num); // Range [0, 39]
			num--;

			riddles.push_back(tempRiddles.at(randomNumber));

			tempRiddles.erase(tempRiddles.begin() + randomNumber);

			vector<string> answerSet;
			answerSet.push_back(tempAnswers.at((randomNumber * 3) + 2));
			tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 2);

			answerSet.push_back(tempAnswers.at((randomNumber * 3) + 1));
			tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)) + 1);

			answerSet.push_back(tempAnswers.at(randomNumber * 3));
			answerCorrect.push_back(tempAnswers.at((randomNumber * 3)));
			tempAnswers.erase((tempAnswers.begin() + (randomNumber * 3)));

			answers.push_back(answerSet);
		}
	}
	else {
		ifstream file("HardRiddles.txt");
		if (!file) {
			cerr << "Error opening file!\n";
		}

		string line;
		int lineNumber = 0;

		int num = 30;
		vector<string> tempAnswers;
		vector<string> tempRiddles;

		while (getline(file, line)) {
			//Random index generator for answer position
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
			int randomNumber = (rand() % num); // Range [0, 50]
			num--;

			riddles.push_back(tempRiddles.at(randomNumber));

			tempRiddles.erase(tempRiddles.begin() + randomNumber);

			vector<string> answerSet;
			answerSet.push_back(tempAnswers.at((randomNumber * 3) + 2));
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