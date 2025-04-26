#pragma once
#ifndef RIDDLES_H
#define RIDDLES_H
#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Riddles {
private:
	vector<string> riddles;
	vector<vector<string>> answers;
	vector<string> answerCorrect;
public:
	Riddles(int floor);
	vector<string> getRiddles() { return riddles; };
	vector<vector<string>> getAnswers() { return answers; };
	vector<string> getAnswerCorrect() { return answerCorrect; };
};

#endif // RIDDLES_H
