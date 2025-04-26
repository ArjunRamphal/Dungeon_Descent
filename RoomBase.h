#ifndef ROOMBASE_H  
#define ROOMBASE_H  
#include <iostream>  
#include <string>  
#include <vector>  
#include "Character.h"  

using namespace std;  

class RoomBase {  
protected:  
   vector<string> choices;  
   string type;
   string imageFileName;
   string textFileName;
public: 
 
   RoomBase(const std::string& name);  
   RoomBase();  

   string getType();  

   virtual vector<string> getChoices(int roomCounter, int biome);  

   virtual string getImageFileName(int biome);  
   virtual string getTextFileName(int biome);  

   virtual string getbtnChoice1(Character& character);
   virtual string getbtnChoice2(Character& character);
   virtual string getbtnChoice3(Character& character);
   virtual string getbtnChoice4(Character& character);
};  

#endif // ROOMBASE_H
