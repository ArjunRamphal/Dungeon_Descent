#pragma once
#ifndef MAGE_H
#define MAGE_H
#include <iostream>
#include <string>
#include "Character.h"
class Mage :
    public Character
{
private:
public:
    Mage();
    void Ability();
    int Lastfpwrupused;
};

#endif // Mage