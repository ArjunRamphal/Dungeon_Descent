#include <queue>
#include <string>
#include <iostream>
#include "StartScreen.h"  
#include <SFML/Graphics.hpp>  
#include <windows.h>  
#include <mmsystem.h>  
#include <SFML/Audio.hpp>  
#pragma comment(lib, "winmm.lib")  

using namespace System;  
using namespace System::Windows::Forms;  
//using namespace WMPLib;  
using namespace System::Media;  
using namespace DungeonDescent;  
using namespace System::Data;  

[STAThread]  
int main(cli::array<String^>^ args)  
{  
   Application::EnableVisualStyles();  
   Application::SetCompatibleTextRenderingDefault(false);  
   StartScreen^ form = gcnew StartScreen();  
   //PlaySound(TEXT("DD.wav"), NULL, SND_ASYNC | SND_LOOP);  

   sf::Music music;  
   if (!music.openFromFile("DD.wav")) { // Make sure this file exists  
       std::cerr << "Error loading music file!" << std::endl;  
       return -1;  
   }  

   music.setLoopPoints(sf::Music::TimeSpan(sf::seconds(0), music.getDuration())); // Set loop points to loop the entire track  
   music.setVolume(30);     // Volume ranges from 0 to 100  
   music.play();

   sf::Music music2;
   if (!music2.openFromFile("intro.wav")) { // Make sure this file exists  
       std::cerr << "Error loading music file!" << std::endl;
       return -1;
   }

   //music2.setLoopPoints(sf::Music::TimeSpan(sf::seconds(0), music2.getDuration())); // Set loop points to loop the entire track  
   music2.setVolume(60);     // Volume ranges from 0 to 100  
   music2.play();

   sf::RenderWindow window(sf::VideoMode({1300, 1000}), "Dungeon Descent");

   std::queue<std::shared_ptr<sf::Texture>> frameQueue;

   // Load 11 frames into the queue
   for (int i = 1; i <= 11; ++i) {
       auto texture = std::make_shared<sf::Texture>();
       std::string filename = "candle" + std::to_string(i) + ".jpg";

       if (!texture->loadFromFile(filename)) {
           std::cerr << "Failed to load " << filename << std::endl;
           continue;
       }

       frameQueue.push(texture);
   }

   if (frameQueue.empty()) return -1; // Exit if no frames loaded

   std::shared_ptr<sf::Texture> firstTexture = frameQueue.front();
   sf::Sprite sprite(*firstTexture);  // Explicit initialization with first texture

   sf::Clock clock;
   float frameDelay = 0.1f;

   while (window.isOpen()) {
       while (const std::optional event = window.pollEvent())
       {
           if (event->is<sf::Event::Closed>()) {
               window.close();
			   music2.stop();
           }
       }

       if (!frameQueue.empty() && clock.getElapsedTime().asSeconds() > frameDelay) {
           std::shared_ptr<sf::Texture> tex = frameQueue.front();
           frameQueue.pop();
           sprite.setTexture(*tex);
           frameQueue.push(tex); // loop the frame
           clock.restart();
       }

       window.clear();
       window.draw(sprite);
       window.display();
   }

   form->Size = System::Drawing::Size(1300, 1000);
   Application::Run(form);  
}
