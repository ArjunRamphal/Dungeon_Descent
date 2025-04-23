#pragma once
#ifndef WARRIOR_H
#define WARRIOR_H
#include <iostream>
#include <string>
#include "Character.h"
class Warrior :
    public Character
{

private:
public:
    Warrior();
    void Ability();
    int Lastfpwrupused;

};

#endif // Warrior