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

[STAThreadAttribute]
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
   //ShowWindow(GetConsoleWindow(), SW_HIDE);
   Application::EnableVisualStyles();  
   Application::SetCompatibleTextRenderingDefault(false);  
   StartScreen^ form = gcnew StartScreen();  

   sf::Music music;  
   if (!music.openFromFile("music/DD.wav")) { // Make sure this file exists  
       std::cerr << "Error loading music file!" << std::endl;  
       return -1;  
   }  

   music.setLooping(true);
   music.setVolume(30); // Volume ranges from 0 to 100  
   music.play();

   sf::Music music2;
   if (!music2.openFromFile("music/intro.wav")) { // Make sure this file exists  
       std::cerr << "Error loading music file!" << std::endl;
       return -1;
   }
 
   music2.setVolume(60); // Volume ranges from 0 to 100  
   music2.play();

   sf::RenderWindow window(sf::VideoMode({1300, 1000}), "Dungeon Descent");

   std::queue<std::shared_ptr<sf::Texture>> frameQueue;

   // Load 11 frames into the queue
   for (int i = 1; i <= 11; ++i) {
       auto texture = std::make_shared<sf::Texture>();
       std::string filename = "candle/candle" + std::to_string(i) + ".jpg";

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

   sf::Font font("Arial.ttf");

   sf::Text text(font); // a font is required to make a text object

   // set the string to display
   text.setString("Press any key to continue");

   // Load the text from the file into a standard string
   //std::string introductionText = msclr::interop::marshal_as<std::string>(File::ReadAllText("introduction.txt"));


   // Create an SFML string from the standard string
   //sf::String sfmlText(introductionText);

   //text.setString(sfmlText);

   // set the character size
   text.setCharacterSize(50); 

   // set the color
   text.setFillColor(sf::Color::White);

   // set the text style
   text.setStyle(sf::Text::Bold | sf::Text::Underlined);

   // set the text position
   text.setPosition(sf::Vector2f(350.0f, 900.0f));

   while (window.isOpen()) {
       while (const std::optional event = window.pollEvent())
       {
           if (event->is<sf::Event::Closed>()) {
               window.close();
               music.stop();
			   music2.stop();
           }

           if (event->is<sf::Event::KeyPressed>()) {
               window.close();
               music.stop();
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
	   // inside the main loop, between window.clear() and window.display()
	   window.draw(text);
       window.display();
   }
   form->Size = System::Drawing::Size(1300, 1000);
   Application::Run(form);
}
