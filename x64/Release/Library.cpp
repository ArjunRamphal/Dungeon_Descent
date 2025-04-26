#include "Library.h"

Library::Library() : RoomBase("Library") {
	type = "Library";
}

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
}

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
}

vector<string> Library::getChoices(int roomCounter, int biome)
{
   if (roomCounter == 1 || roomCounter == 2 || roomCounter == 3 || roomCounter == 7)
       return vector<string>{"The Art of Warfare", "The Whispering Winds", "The Ancient Lore", "The Heart of the Forest"};
   else if (roomCounter == 11 || roomCounter == 12 || roomCounter == 13) {
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

string Library::getbtnChoice1(Character& character)
{
	if (character.getFloor() == 3) {
		character.incStats(0, 2);
		character.incReputation();
		return character.incStatsDisplay(0, 2) + " lavalibrarydiary.txt";
	}
	else {
		character.incStats(0, 2);
		return character.incStatsDisplay(0, 2);
	}
}

string Library::getbtnChoice2(Character& character)
{
	character.incStats(3, 2);
	return character.incStatsDisplay(3, 2);
}

string Library::getbtnChoice3(Character& character)
{
	character.incStats(1, 2);
	return character.incStatsDisplay(1, 2);
}

string Library::getbtnChoice4(Character& character)
{
	character.incStats(2, 2);
	return character.incStatsDisplay(2, 2);
}
