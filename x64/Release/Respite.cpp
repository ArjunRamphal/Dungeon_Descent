#include "Respite.h"

Respite::Respite() : RoomBase("Shop") {

}

string Respite::getTextFileName(int biome)
{
	if (biome == 0) {
		return "icerespite.txt";
	}
	else if (biome == 1) {
		return "junglerespite.txt";
	}
	else if (biome == 2) {
		return "desertrespite.txt";
	}
	else if (biome == 3) {
		return "ghostrespite.txt";
	}
	else if (biome == 4) {
		return "lavarespite.txt";
	}
}

string Respite::getImageFileName(int biome)
{
	if (biome == 0) {
		return "icerespite.png";
	}
	else if (biome == 1) {
		return "junglerespite.png";
	}
	else if (biome == 2) {
		return "desertrespite.jpeg";
	}
	else if (biome == 3) {
		return "ghostrespite.jpeg";
	}
	else if (biome == 4) {
		return "lavarespite.jpg";
	}
}

vector<string> Respite::getChoices(int roomCounter, int biome)
{
	return vector<string>{"Increase health", "Search room"};
}

string Respite::getbtnChoice1(Character& character)
{
	character.incStats(4, 10);

	if (character.getFloor() == 1) {
		character.incReputation();
		return character.incStatsDisplay(4, 10) + " respitedrink.txt";
	}
	else {
		return character.incStatsDisplay(4, 10);
	}
}

string Respite::getbtnChoice2(Character& character)
{
	for (int i = 0; i < 5; i++) {
		character.incStats(i, 2);
	}
	return "You have found items which increase all stats by 2.";
}
