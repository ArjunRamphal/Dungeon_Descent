#include "Encounter.h"

Encounter::Encounter() : RoomBase("Encounter")
{

}

string Encounter::getImageFileName(int biome)
{
	if ((biome == 0) || ((biome == 1))) {
		return "prison.jpeg";
	}
	else if (biome == 2) {
		return "desertbutler.jpeg";
	}
	else if (biome == 3) {
		return "ghostbutler.jpeg";
	}
	else if (biome == 4) {
		return "lavaminions.jpg";
	}
}

string Encounter::getTextFileName(int biome)
{
	if (biome == 0) {
		return "iceprison.txt";
	}
	else if (biome == 1) {
		return "jungleprison.txt";
	}
	else if (biome == 2) {
		return "desertbutler.txt";
	}
	else if (biome == 3) {
		return "ghostbutler.txt";
	}
	else if (biome == 4) {
		return "lavaminion.txt";
	}
}

vector<string> Encounter::getChoices(int roomCounter, int biome)
{
	if (roomCounter == 4)
		return vector<string>{"Spare warrior", "Kill warrior"};
	else if (roomCounter == 16) {
		return vector<string>{"Save butler", "Take vial"};
	}
	else {
		return vector<string>{"Threaten minions", "Kill minions"};
	}
}

string Encounter::getbtnChoice1(Character& character)
{
	if (character.getFloor() == 1) {
		if (character.getBiome() == 0) {
			character.incReputation();
			return " iceprisonfree.txt";
		}
			
		else {
			character.incReputation();
			return " jungleprisonfree.txt";
		}
	}
	else if (character.getFloor() == 2){
		if (character.getBiome() == 2) {
			character.incReputation();
			return " desertbutlerfree.txt";
		}
		else {
			character.incReputation();
			return " ghostbutlerfree.txt";
		}
	}
	else {
		character.incReputation();
		return " lavaminionfree.txt";
	}
}

string Encounter::getbtnChoice2(Character& character)
{
	if (character.getFloor() == 1) {
		character.incStats(1);
		return "You kill the warrior. " + character.incStatsDisplay(1);
	}
	else if (character.getFloor() == 2) {
		character.incStats(1.5);
		return "You take the vial and leave the butler to die. " + character.incStatsDisplay(1);
	}
	else {
		character.incStats(2);
		return "You kill the minions. " + character.incStatsDisplay(1);
	};
}
