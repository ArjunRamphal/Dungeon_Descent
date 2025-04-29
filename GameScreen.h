#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <msclr\marshal_cppstd.h>
#include <msclr\marshal.h>
#include "Character.h"
#include "Warrior.h"
#include "Ranger.h"
#include "Mage.h"
#include "Rogue.h"
#include "RoomBase.h"
#include "Monster.h"
#include "Battle.h"
#include "Library.h"
#include "Respite.h"
#include "Shop.h"
#include "Chest.h"
#include "Encounter.h"
#include "Riddles.h"
#include "Lore.h"
#include "Music.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Audio.hpp>

namespace DungeonDescent {
	using namespace std;
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::IO;

	/// <summary>
	/// Summary for GameScreen
	/// </summary>
	public ref class GameScreen : public System::Windows::Forms::Form
	{
	public:

		Form^ obj;

		GameScreen(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

		// custom constructor to avoid circular reference
		GameScreen(Form^ obj1)
		{
			obj = obj1;
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

		Character* character; // Holds character
		Riddles* riddles; // Holds random riddles for each floor
		RoomBase* room; // Holds each room once loaded
		Battle* currentBattle = nullptr; // Holds current battle
		bool isBoss = false; // Set to true if boss battle, else false
		bool malvelDefeated = false; // Used to check whether player has defeated final boss
		bool isEnlarged = false; // Holds state of minimap
		bool roomState = false; // Holds whether room player is in is active
		bool battleState = false; // Holds whether a battle is active
		bool biomeSelect = true; // Holds whether player is in a biome select room
		bool riddleState = false; // Holds whether a riddle must appear or not
		bool chestOpen = false; // Holds whether chest is unlocked or not
		int riddleCounter = 0; // Holds current riddle number

		Music* music; // Global variable to hold SFML music

		// Random numbers to randomise positions of answers
		int randomAnswer1;
		int randomAnswer2;
		int randomAnswer3;

        ref class ManagedRoomBase {  
        public:  
           RoomBase* nativeRoomBase;  

           ManagedRoomBase(RoomBase* roomBase) {  
               nativeRoomBase = roomBase;  
           }  

		   RoomBase* getRoom() {
			   return nativeRoomBase;
		   }

           ~ManagedRoomBase() {  
               delete nativeRoomBase;
           }  
        };  
 
        System::Collections::Generic::Queue<ManagedRoomBase^>^ roomLoad;

    private: ref struct Feedback {  
       int score;  
       int reputation;  
       int maxReputation;  
       int battlesWon;  
       int maxBattles;  
       int riddlesCorrect;  
       int maxRiddles;  
       System::DateTime startTime;
       System::DateTime endTime;
       System::TimeSpan duration;

       Feedback() : score(0), reputation(0), battlesWon(0), riddlesCorrect(0), maxBattles(9), maxRiddles(0), maxReputation(9) {}
    };

		Feedback^ feedback;

	private: System::Windows::Forms::PictureBox^ pbBackground;
	public: System::Windows::Forms::Timer^ tmrRiddle;
	private: System::Windows::Forms::Button^ btnRight;
	private: System::Windows::Forms::Label^ lblProgress;

	public:
	private: System::Windows::Forms::Button^ btnLeft;
	private:

	public:
		
	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~GameScreen()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::PictureBox^ pbProfile;
	private: System::Windows::Forms::ProgressBar^ progRiddle;
	protected:

	private: System::Windows::Forms::PictureBox^ pbMap;

	private: System::Windows::Forms::RichTextBox^ redReader;

	private: System::Windows::Forms::GroupBox^ gbButtons;
	private: System::Windows::Forms::PictureBox^ pbAbility;

	private: System::Windows::Forms::ListBox^ lbStats;
	private: System::Windows::Forms::PictureBox^ pbBack;
	private: System::Windows::Forms::Button^ btnChoice1;
	private: System::Windows::Forms::Button^ btnChoice2;
	private: System::Windows::Forms::Button^ btnChoice3;
	private: System::Windows::Forms::Button^ btnChoice4;
	private: System::Windows::Forms::Button^ btnContinue;
	private: System::Windows::Forms::Button^ btnAnswer1;
	private: System::Windows::Forms::Button^ btnAnswer3;
	private: System::Windows::Forms::Button^ btnAnswer2;
	private: System::Windows::Forms::Button^ btnAttack;
	private: System::Windows::Forms::PictureBox^ pbSword;
	private: System::Windows::Forms::PictureBox^ pbThrowingKnife;
	private: System::Windows::Forms::PictureBox^ pbWand;
	private: System::Windows::Forms::PictureBox^ pbLongbow;
	private: System::ComponentModel::IContainer^ components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(GameScreen::typeid));
			this->pbProfile = (gcnew System::Windows::Forms::PictureBox());
			this->progRiddle = (gcnew System::Windows::Forms::ProgressBar());
			this->pbMap = (gcnew System::Windows::Forms::PictureBox());
			this->redReader = (gcnew System::Windows::Forms::RichTextBox());
			this->gbButtons = (gcnew System::Windows::Forms::GroupBox());
			this->btnAttack = (gcnew System::Windows::Forms::Button());
			this->btnAnswer3 = (gcnew System::Windows::Forms::Button());
			this->btnAnswer2 = (gcnew System::Windows::Forms::Button());
			this->btnAnswer1 = (gcnew System::Windows::Forms::Button());
			this->btnContinue = (gcnew System::Windows::Forms::Button());
			this->btnChoice4 = (gcnew System::Windows::Forms::Button());
			this->btnChoice3 = (gcnew System::Windows::Forms::Button());
			this->btnChoice2 = (gcnew System::Windows::Forms::Button());
			this->btnChoice1 = (gcnew System::Windows::Forms::Button());
			this->btnLeft = (gcnew System::Windows::Forms::Button());
			this->btnRight = (gcnew System::Windows::Forms::Button());
			this->pbAbility = (gcnew System::Windows::Forms::PictureBox());
			this->lbStats = (gcnew System::Windows::Forms::ListBox());
			this->pbBack = (gcnew System::Windows::Forms::PictureBox());
			this->pbSword = (gcnew System::Windows::Forms::PictureBox());
			this->pbThrowingKnife = (gcnew System::Windows::Forms::PictureBox());
			this->pbWand = (gcnew System::Windows::Forms::PictureBox());
			this->pbLongbow = (gcnew System::Windows::Forms::PictureBox());
			this->pbBackground = (gcnew System::Windows::Forms::PictureBox());
			this->tmrRiddle = (gcnew System::Windows::Forms::Timer(this->components));
			this->lblProgress = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbProfile))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbMap))->BeginInit();
			this->gbButtons->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbAbility))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbBack))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbSword))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbThrowingKnife))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbWand))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbLongbow))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbBackground))->BeginInit();
			this->SuspendLayout();
			// 
			// pbProfile
			// 
			this->pbProfile->Location = System::Drawing::Point(0, 0);
			this->pbProfile->Name = L"pbProfile";
			this->pbProfile->Size = System::Drawing::Size(182, 174);
			this->pbProfile->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbProfile->TabIndex = 0;
			this->pbProfile->TabStop = false;
			this->pbProfile->Visible = false;
			this->pbProfile->Click += gcnew System::EventHandler(this, &GameScreen::pbProfile_Click);
			// 
			// progRiddle
			// 
			this->progRiddle->Location = System::Drawing::Point(188, 0);
			this->progRiddle->Maximum = 20;
			this->progRiddle->Name = L"progRiddle";
			this->progRiddle->Size = System::Drawing::Size(903, 53);
			this->progRiddle->TabIndex = 1;
			this->progRiddle->Visible = false;
			// 
			// pbMap
			// 
			this->pbMap->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbMap.Image")));
			this->pbMap->Location = System::Drawing::Point(1097, 0);
			this->pbMap->Name = L"pbMap";
			this->pbMap->Size = System::Drawing::Size(182, 174);
			this->pbMap->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbMap->TabIndex = 2;
			this->pbMap->TabStop = false;
			this->pbMap->Visible = false;
			this->pbMap->Click += gcnew System::EventHandler(this, &GameScreen::pbMap_Click);
			// 
			// redReader
			// 
			this->redReader->BackColor = System::Drawing::Color::LightYellow;
			this->redReader->Font = (gcnew System::Drawing::Font(L"Book Antiqua", 14, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->redReader->Location = System::Drawing::Point(181, 663);
			this->redReader->Name = L"redReader";
			this->redReader->Size = System::Drawing::Size(1098, 282);
			this->redReader->TabIndex = 3;
			this->redReader->Text = L"";
			// 
			// gbButtons
			// 
			this->gbButtons->Controls->Add(this->btnAttack);
			this->gbButtons->Controls->Add(this->btnAnswer3);
			this->gbButtons->Controls->Add(this->btnAnswer2);
			this->gbButtons->Controls->Add(this->btnAnswer1);
			this->gbButtons->Controls->Add(this->btnContinue);
			this->gbButtons->Controls->Add(this->btnChoice4);
			this->gbButtons->Controls->Add(this->btnChoice3);
			this->gbButtons->Controls->Add(this->btnChoice2);
			this->gbButtons->Controls->Add(this->btnChoice1);
			this->gbButtons->Controls->Add(this->btnLeft);
			this->gbButtons->Controls->Add(this->btnRight);
			this->gbButtons->Location = System::Drawing::Point(181, 565);
			this->gbButtons->Name = L"gbButtons";
			this->gbButtons->Size = System::Drawing::Size(1098, 92);
			this->gbButtons->TabIndex = 4;
			this->gbButtons->TabStop = false;
			// 
			// btnAttack
			// 
			this->btnAttack->Location = System::Drawing::Point(7, 12);
			this->btnAttack->Name = L"btnAttack";
			this->btnAttack->Size = System::Drawing::Size(143, 74);
			this->btnAttack->TabIndex = 8;
			this->btnAttack->Text = L"Attack";
			this->btnAttack->UseVisualStyleBackColor = true;
			this->btnAttack->Visible = false;
			this->btnAttack->Click += gcnew System::EventHandler(this, &GameScreen::btnAttack_Click);
			// 
			// btnAnswer3
			// 
			this->btnAnswer3->Location = System::Drawing::Point(440, 12);
			this->btnAnswer3->Name = L"btnAnswer3";
			this->btnAnswer3->Size = System::Drawing::Size(143, 74);
			this->btnAnswer3->TabIndex = 7;
			this->btnAnswer3->Text = L"Answer 3";
			this->btnAnswer3->UseVisualStyleBackColor = true;
			this->btnAnswer3->Visible = false;
			this->btnAnswer3->Click += gcnew System::EventHandler(this, &GameScreen::btnAnswer3_Click);
			// 
			// btnAnswer2
			// 
			this->btnAnswer2->Location = System::Drawing::Point(219, 12);
			this->btnAnswer2->Name = L"btnAnswer2";
			this->btnAnswer2->Size = System::Drawing::Size(143, 74);
			this->btnAnswer2->TabIndex = 6;
			this->btnAnswer2->Text = L"Answer 2";
			this->btnAnswer2->UseVisualStyleBackColor = true;
			this->btnAnswer2->Visible = false;
			this->btnAnswer2->Click += gcnew System::EventHandler(this, &GameScreen::btnAnswer2_Click);
			// 
			// btnAnswer1
			// 
			this->btnAnswer1->Location = System::Drawing::Point(7, 12);
			this->btnAnswer1->Name = L"btnAnswer1";
			this->btnAnswer1->Size = System::Drawing::Size(143, 74);
			this->btnAnswer1->TabIndex = 5;
			this->btnAnswer1->Text = L"Answer 1";
			this->btnAnswer1->UseVisualStyleBackColor = true;
			this->btnAnswer1->Visible = false;
			this->btnAnswer1->Click += gcnew System::EventHandler(this, &GameScreen::btnAnswer1_Click);
			// 
			// btnContinue
			// 
			this->btnContinue->Location = System::Drawing::Point(938, 12);
			this->btnContinue->Name = L"btnContinue";
			this->btnContinue->Size = System::Drawing::Size(143, 74);
			this->btnContinue->TabIndex = 4;
			this->btnContinue->Text = L"Continue";
			this->btnContinue->UseVisualStyleBackColor = true;
			this->btnContinue->Visible = false;
			this->btnContinue->Click += gcnew System::EventHandler(this, &GameScreen::btnContinue_Click);
			// 
			// btnChoice4
			// 
			this->btnChoice4->Location = System::Drawing::Point(664, 12);
			this->btnChoice4->Name = L"btnChoice4";
			this->btnChoice4->Size = System::Drawing::Size(143, 74);
			this->btnChoice4->TabIndex = 3;
			this->btnChoice4->Text = L"Choice 4";
			this->btnChoice4->UseVisualStyleBackColor = true;
			this->btnChoice4->Visible = false;
			this->btnChoice4->Click += gcnew System::EventHandler(this, &GameScreen::btnChoice4_Click);
			// 
			// btnChoice3
			// 
			this->btnChoice3->Location = System::Drawing::Point(440, 12);
			this->btnChoice3->Name = L"btnChoice3";
			this->btnChoice3->Size = System::Drawing::Size(143, 74);
			this->btnChoice3->TabIndex = 2;
			this->btnChoice3->Text = L"Choice 3";
			this->btnChoice3->UseVisualStyleBackColor = true;
			this->btnChoice3->Visible = false;
			this->btnChoice3->Click += gcnew System::EventHandler(this, &GameScreen::btnChoice3_Click);
			// 
			// btnChoice2
			// 
			this->btnChoice2->Location = System::Drawing::Point(219, 12);
			this->btnChoice2->Name = L"btnChoice2";
			this->btnChoice2->Size = System::Drawing::Size(143, 74);
			this->btnChoice2->TabIndex = 1;
			this->btnChoice2->Text = L"Choice 2";
			this->btnChoice2->UseVisualStyleBackColor = true;
			this->btnChoice2->Visible = false;
			this->btnChoice2->Click += gcnew System::EventHandler(this, &GameScreen::btnChoice2_Click);
			// 
			// btnChoice1
			// 
			this->btnChoice1->Location = System::Drawing::Point(7, 12);
			this->btnChoice1->Name = L"btnChoice1";
			this->btnChoice1->Size = System::Drawing::Size(143, 74);
			this->btnChoice1->TabIndex = 0;
			this->btnChoice1->Text = L"Choice 1";
			this->btnChoice1->UseVisualStyleBackColor = true;
			this->btnChoice1->Visible = false;
			this->btnChoice1->Click += gcnew System::EventHandler(this, &GameScreen::btnChoice1_Click);
			// 
			// btnLeft
			// 
			this->btnLeft->Location = System::Drawing::Point(7, 12);
			this->btnLeft->Name = L"btnLeft";
			this->btnLeft->Size = System::Drawing::Size(143, 74);
			this->btnLeft->TabIndex = 9;
			this->btnLeft->Text = L"Go left";
			this->btnLeft->UseVisualStyleBackColor = true;
			this->btnLeft->Visible = false;
			this->btnLeft->Click += gcnew System::EventHandler(this, &GameScreen::btnLeft_Click);
			// 
			// btnRight
			// 
			this->btnRight->Location = System::Drawing::Point(219, 12);
			this->btnRight->Name = L"btnRight";
			this->btnRight->Size = System::Drawing::Size(143, 74);
			this->btnRight->TabIndex = 10;
			this->btnRight->Text = L"Go right";
			this->btnRight->UseVisualStyleBackColor = true;
			this->btnRight->Visible = false;
			this->btnRight->Click += gcnew System::EventHandler(this, &GameScreen::btnRight_Click);
			// 
			// pbAbility
			// 
			this->pbAbility->Location = System::Drawing::Point(0, 494);
			this->pbAbility->Name = L"pbAbility";
			this->pbAbility->Size = System::Drawing::Size(177, 163);
			this->pbAbility->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbAbility->TabIndex = 5;
			this->pbAbility->TabStop = false;
			this->pbAbility->Visible = false;
			// 
			// lbStats
			// 
			this->lbStats->Font = (gcnew System::Drawing::Font(L"Copperplate Gothic Light", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->lbStats->FormattingEnabled = true;
			this->lbStats->ItemHeight = 20;
			this->lbStats->Location = System::Drawing::Point(0, 663);
			this->lbStats->Name = L"lbStats";
			this->lbStats->Size = System::Drawing::Size(176, 124);
			this->lbStats->TabIndex = 6;
			this->lbStats->MouseEnter += gcnew System::EventHandler(this, &GameScreen::lbStats_MouseEnter);
			this->lbStats->MouseHover += gcnew System::EventHandler(this, &GameScreen::lbStats_MouseHover);
			// 
			// pbBack
			// 
			this->pbBack->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbBack.Image")));
			this->pbBack->Location = System::Drawing::Point(0, 860);
			this->pbBack->Name = L"pbBack";
			this->pbBack->Size = System::Drawing::Size(88, 85);
			this->pbBack->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbBack->TabIndex = 7;
			this->pbBack->TabStop = false;
			this->pbBack->Visible = false;
			this->pbBack->Click += gcnew System::EventHandler(this, &GameScreen::pbBack_Click);
			// 
			// pbSword
			// 
			this->pbSword->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbSword.Image")));
			this->pbSword->Location = System::Drawing::Point(30, 211);
			this->pbSword->Name = L"pbSword";
			this->pbSword->Size = System::Drawing::Size(284, 271);
			this->pbSword->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbSword->TabIndex = 8;
			this->pbSword->TabStop = false;
			this->pbSword->Click += gcnew System::EventHandler(this, &GameScreen::pbSword_Click);
			// 
			// pbThrowingKnife
			// 
			this->pbThrowingKnife->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbThrowingKnife.Image")));
			this->pbThrowingKnife->Location = System::Drawing::Point(348, 211);
			this->pbThrowingKnife->Name = L"pbThrowingKnife";
			this->pbThrowingKnife->Size = System::Drawing::Size(284, 271);
			this->pbThrowingKnife->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbThrowingKnife->TabIndex = 9;
			this->pbThrowingKnife->TabStop = false;
			this->pbThrowingKnife->Click += gcnew System::EventHandler(this, &GameScreen::pbThrowingKnife_Click);
			// 
			// pbWand
			// 
			this->pbWand->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbWand.Image")));
			this->pbWand->Location = System::Drawing::Point(669, 211);
			this->pbWand->Name = L"pbWand";
			this->pbWand->Size = System::Drawing::Size(284, 271);
			this->pbWand->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbWand->TabIndex = 10;
			this->pbWand->TabStop = false;
			this->pbWand->Click += gcnew System::EventHandler(this, &GameScreen::pbWand_Click);
			// 
			// pbLongbow
			// 
			this->pbLongbow->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbLongbow.Image")));
			this->pbLongbow->Location = System::Drawing::Point(982, 211);
			this->pbLongbow->Name = L"pbLongbow";
			this->pbLongbow->Size = System::Drawing::Size(284, 271);
			this->pbLongbow->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbLongbow->TabIndex = 11;
			this->pbLongbow->TabStop = false;
			this->pbLongbow->Click += gcnew System::EventHandler(this, &GameScreen::pbLongbow_Click);
			// 
			// pbBackground
			// 
			this->pbBackground->Location = System::Drawing::Point(0, 0);
			this->pbBackground->Name = L"pbBackground";
			this->pbBackground->Size = System::Drawing::Size(1279, 657);
			this->pbBackground->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbBackground->TabIndex = 12;
			this->pbBackground->TabStop = false;
			// 
			// tmrRiddle
			// 
			this->tmrRiddle->Interval = 1000;
			this->tmrRiddle->Tick += gcnew System::EventHandler(this, &GameScreen::tmrRiddle_Tick);
			// 
			// lblProgress
			// 
			this->lblProgress->AutoSize = true;
			this->lblProgress->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->lblProgress->ForeColor = System::Drawing::Color::White;
			this->lblProgress->Location = System::Drawing::Point(1093, 177);
			this->lblProgress->Name = L"lblProgress";
			this->lblProgress->Size = System::Drawing::Size(0, 22);
			this->lblProgress->TabIndex = 13;
			this->lblProgress->Visible = false;
			// 
			// GameScreen
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(9, 20);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::Color::Black;
			this->ClientSize = System::Drawing::Size(1278, 944);
			this->Controls->Add(this->lblProgress);
			this->Controls->Add(this->pbMap);
			this->Controls->Add(this->pbLongbow);
			this->Controls->Add(this->pbWand);
			this->Controls->Add(this->pbThrowingKnife);
			this->Controls->Add(this->pbSword);
			this->Controls->Add(this->pbBack);
			this->Controls->Add(this->lbStats);
			this->Controls->Add(this->pbAbility);
			this->Controls->Add(this->gbButtons);
			this->Controls->Add(this->redReader);
			this->Controls->Add(this->progRiddle);
			this->Controls->Add(this->pbProfile);
			this->Controls->Add(this->pbBackground);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->Name = L"GameScreen";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"GameScreen";
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &GameScreen::GameScreen_FormClosed);
			this->Shown += gcnew System::EventHandler(this, &GameScreen::GameScreen_Shown);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbProfile))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbMap))->EndInit();
			this->gbButtons->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbAbility))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbBack))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbSword))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbThrowingKnife))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbWand))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbLongbow))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbBackground))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
private: System::Void GameScreen_Shown(System::Object^ sender, System::EventArgs^ e) {
	setGameScreen();
}

private: void dungeonCreate() {
	srand(time(0));

	int randomNum1 = (rand() % 3) + 1;
	int randomNum2 = 0;
	int randomNum3 = 0;

	do {
		srand(time(0));
		randomNum2 = (rand() % 3) + 1;
	} while (randomNum1 == randomNum2);

	do {
		srand(time(0));
		randomNum3 = (rand() % 3) + 1;
	} while ((randomNum3 == randomNum1) || (randomNum3 == randomNum2));

	if (character->getFloor() == 1) { // generate floor 1 rooms
		roomLoad = gcnew System::Collections::Generic::Queue<ManagedRoomBase^>();

		randomRooms(randomNum1, randomNum2, randomNum3);
		
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Encounter()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(false, *character)));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Respite()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Library()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Shop()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(true, *character)));
	}
	else if (character->getFloor() == 2) { // generate floor 2 rooms
		roomLoad = gcnew System::Collections::Generic::Queue<ManagedRoomBase^>();

		randomRooms(randomNum1, randomNum2, randomNum3);

		roomLoad->Enqueue(gcnew ManagedRoomBase(new Shop()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Chest()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Encounter()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Respite()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(false, *character)));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(true, *character)));
	}
	else if (character->getFloor() == 3) { // generate floor 3 rooms
		roomLoad = gcnew System::Collections::Generic::Queue<ManagedRoomBase^>();

		randomRooms(randomNum1, randomNum2, randomNum3);

		roomLoad->Enqueue(gcnew ManagedRoomBase(new Chest()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(false, *character)));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Shop()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Encounter()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Respite()));
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(true, *character)));
	}
}

private: System::Void pbSword_Click(System::Object^ sender, System::EventArgs^ e) {
	character = new Warrior();
	weaponPick();
}

private: System::Void pbThrowingKnife_Click(System::Object^ sender, System::EventArgs^ e) {
	character = new Rogue();
	weaponPick();
}

private: System::Void pbWand_Click(System::Object^ sender, System::EventArgs^ e) {
	character = new Mage();
	weaponPick();
}
private: System::Void pbLongbow_Click(System::Object^ sender, System::EventArgs^ e) {
	character = new Ranger();
	weaponPick();
}

private: void weaponPick() {
	pbMap->Visible = true;
	pbProfile->Visible = true;
	// load character pfp image and ability image
	pbProfile->Image = Image::FromFile(gcnew String(character->getPfpImageName().c_str()));
	pbAbility->Image = Image::FromFile(gcnew String(character->getAbilityFileName().c_str()));
	pbAbility->Visible = true;

	// make weapon picture boxes invisible
	pbSword->Visible = false;
	pbThrowingKnife->Visible = false;
	pbWand->Visible = false;
	pbLongbow->Visible = false;

	// biome select room components visible
	biomeSelect = true;
	roomState = false;
	btnLeft->Visible = true;
	btnRight->Visible = true;
	btnChoiceInvisible();
	
	// load entrance background 
	floorEntrance();
	
	character->Ability();
	showStats();

	feedback = gcnew Feedback();
	feedback->startTime = DateTime::Now;
}

private: System::Void pbMap_Click(System::Object^ sender, System::EventArgs^ e) {
	if (!isEnlarged) { 	
		// expand minimap
		pbMap->Size = System::Drawing::Size(600, 600);
		pbMap->Location = System::Drawing::Point(400, 50);
		isEnlarged = true;
	}
	else { 
		// shrink minimap
		pbMap->Size = System::Drawing::Size(182, 174);
		pbMap->Location = System::Drawing::Point(1097, 0);
		isEnlarged = false;
	}
}
// function is called when player clicks back button
private: System::Void pbBack_Click(System::Object^ sender, System::EventArgs^ e) {
	// Message box to check whether player wants to exit
	System::Windows::Forms::DialogResult result = MessageBox::Show(
		"Are you sure you want to exit? All progress will be lost.",
		"Exit",
		MessageBoxButtons::YesNo,
		MessageBoxIcon::Warning,
		MessageBoxDefaultButton::Button2
	);

	if (result == System::Windows::Forms::DialogResult::Yes) { //player wants to exit
		// show main menu
		this->Visible = false;
		obj->Visible = true;
		music->StopSound();
	}
}

/*
this function will use the same variable room for all rooms in the game
only one room will be generated at a time

uses a queue to store the rooms of the specific floor
dequeue rooms each time continue button is clicked which calls roomCreate
*/
private: void roomCreate()
{
	roomState = true;
	battleState = false;
	btnContinue->Visible = false;

	if (character->getRoomCounter() != 29) { // check whether player has reached final boss room
		riddleState = true;
		character->incRoomCounter(); // keep track of what room player is in by incrementing roomCounter
		ManagedRoomBase^ managedRoom = roomLoad->Dequeue(); // dequeue room from queue
		room = managedRoom->getRoom();
		readerAndBackground(); // load background image and text file for room

		if (room->getChoices(character->getRoomCounter(), character->getBiome()).size() == 1) {
			if (room->getType() == "Chest") {
				btnChoiceInvisible();
				btnContinue->Visible = true;
			}
			else if (room->getType() == "Battle") {
				btnChoiceInvisible();
				riddleState = false; // no riddle in boss room
				btnChoice1->Visible = true;
				// load choice text
				btnChoice1->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(0).c_str());
			}
				
			if (chestOpen) {
				btnChoice1->Visible = true;
				// load choice text
				btnChoice1->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(0).c_str());
				btnContinue->Visible = true;
			}
		}
		else if (room->getChoices(character->getRoomCounter(), character->getBiome()).size() == 2) {
			btnChoiceInvisible();
			btnChoice1->Visible = true;
			btnChoice2->Visible = true;
			// load choice text
			btnChoice1->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(0).c_str());
			btnChoice2->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(1).c_str());
		}
		else if (room->getChoices(character->getRoomCounter(), character->getBiome()).size() == 3) {
			btnChoiceVisible();
			btnChoice4->Visible = false;
			// load choice text
			btnChoice1->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(0).c_str());
			btnChoice2->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(1).c_str());
			btnChoice3->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(2).c_str());
		}
		else {
			btnChoiceVisible();
			// load choice text
			btnChoice1->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(0).c_str());
			btnChoice2->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(1).c_str());
			btnChoice3->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(2).c_str());
			btnChoice4->Text = gcnew String(room->getChoices(character->getRoomCounter(), character->getBiome()).at(3).c_str());
		}
	}
}

private: System::Void btnContinue_Click(System::Object^ sender, System::EventArgs^ e) {  
	// check whether character has 0 health
	if (character->getRoomCounter() != 29) {
		if (character->getHealth() == 0) {

			// Message box telling player they have no health left
			System::Windows::Forms::DialogResult result = MessageBox::Show(
				"You have no health left. You cannot continue.",
				"Game Over",
				MessageBoxButtons::OK,
				MessageBoxIcon::Stop,
				MessageBoxDefaultButton::Button1
			);

			// if player clicks ok, show main menu
			if (result == System::Windows::Forms::DialogResult::OK) {
				this->Visible = false;
				obj->Visible = true;
				music->StopSound();
			}
		}
	}
	
	if (riddleState) { // check whether a riddle needs to be generated
		randomRiddle(); // generate random riddle
	}
	else if (chestOpen) { // check whether a chest is open
		btnContinue->Visible = false;
		btnChoice1->Visible = true;
		btnChoice1->Text = "Open chest";
	}
	else if (character->getRoomCounter() == 31) { // check whether player has completed the game
		redReader->Text = "Press continue to return to main menu";
		this->Visible = false;
		obj->Visible = true;
		music->StopSound();
	}
	else if (character->getRoomCounter() == 30) { // check whether player is at end of game
		outputFeedback();
	}
	else if (character->getRoomCounter() % 10 == 9){ // check whether player is at a boss battle
		lblProgress->Visible = false;

		if ((character->getRoomCounter() == 9) || (character->getRoomCounter() == 19)) {
			biomeSelect = true;
			roomState = false;
			btnContinue->Visible = false;

			if (character->getRoomCounter() == 19) { // check whether player is entering last floor
				btnLeft->Text = "Malvel's Domain";
				btnLeft->Visible = true;
				btnRight->Visible = false;
			}
			else {
				btnLeft->Visible = true;
				btnRight->Visible = true;
			}
			btnChoiceInvisible();
		}

		if (character->getRoomCounter() == 29) { // check whether player is at final boss
			btnAnswersInvisible();
			btnChoiceInvisible();

			feedback->endTime = DateTime::Now;
			feedback->duration = feedback->endTime - feedback->startTime;
			lblProgress->Visible = true;
			lblProgress->Text = "Game completed";

			/*
			determine what ending the player gets depending on if final boss defeated, reputation counter 5/9 (if player selected correct 
			decisions), if player got 14/24 random riddles correct or if player won 5/9 battles
			*/
			if (malvelDefeated) {
				if ((character->getReputation() >= 5) && ((character->getRiddleCorrect() >= 14) || (character->getBattlesWon() >= 5))) {
					character->incRoomCounter();
					redReader->Text = File::ReadAllText("textfiles/ending/trueending.txt");
					redReader->Text = redReader->Text + "\nYou have received the TRUE Ending: Justice for Falkreath (2/3)";
					pbBackground->Image = Image::FromFile("images/ending/trueending.jpg");
				}
				else {
					character->incRoomCounter();
					redReader->Text = File::ReadAllText("textfiles/ending/goodending.txt");
					redReader->Text = redReader->Text + "\nYou have received the GOOD Ending: Hero of Aethoria? (1/3)";
					pbBackground->Image = Image::FromFile("images/ending/goodending.jpg");
				}
			}
			else {
				character->incRoomCounter();
				redReader->Text = File::ReadAllText("textfiles/ending/badending.txt");
				redReader->Text = redReader->Text + "\nYou have received the BAD Ending: Dungeon Demise (3/3)";
				pbBackground->Image = Image::FromFile("images/ending/badending.jpg");
			}
		}
		else {
			character->incRoomCounter();
			// generate biome select room
			floorEntrance();
		}	
	}
	else {
		btnAnswersInvisible();
		roomCreate(); // generate next room

		lblProgress->Visible = true;
		lblProgress->Text = "Floor " + character->getFloor() + " :Room " + (character->getRoomCounter() % 10); // Mod by 10 to get correct number for room in floor
	}
}

// Generate random riddle, randomise answers in buttons, set timer
private: void randomRiddle() {
	riddleState = false;
	btnChoiceInvisible();
	btnAnswersVisible();
	btnContinue->Visible = false;

	redReader->Text = gcnew String(riddles->getRiddles().at(riddleCounter).c_str());
	
	if (character->getStatValue(2) > 30) {
		randomAnswer1 = rand() % 2 + 1;

		do {
			randomAnswer2 = rand() % 2 + 1;
		} while (randomAnswer2 == randomAnswer1);

		btnAnswer1->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer1).c_str());
		btnAnswer2->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer2).c_str());
		btnAnswer3->Visible = false;
	}
	else {
		randomAnswer1 = rand() % 3;

		do {
			randomAnswer2 = rand() % 3;
		} while (randomAnswer2 == randomAnswer1);

		do {
			randomAnswer3 = rand() % 3;
		} while ((randomAnswer3 == randomAnswer1) || (randomAnswer3 == randomAnswer2));

		btnAnswer1->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer1).c_str());
		btnAnswer2->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer2).c_str());
		btnAnswer3->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer3).c_str());
	}

	progRiddle->Maximum = character->getQuestionTime() + character->getExtraQuestionTime();
	progRiddle->Value = character->getQuestionTime() + character->getExtraQuestionTime();

	tmrRiddle->Enabled = true;
	tmrRiddle->Start();
	progRiddle->Visible = true;
}

// biome select, calls dungeonCreate to generate new queue of rooms for next floor
private: System::Void btnLeft_Click(System::Object^ sender, System::EventArgs^ e) {
	btnLeft->Visible = false;
	btnRight->Visible = false;
	pbBack->Visible = true;

	if (biomeSelect) {
		biomeSelect = false;
		character->incFloor();
		btnChoiceVisible();

		if (character->getFloor() == 1) {
			character->setBiome(0);
			music = new Music();
			music->iceBiome();
		}
		else if (character->getFloor() == 2) {
			pbMap->Image = Image::FromFile("images/map/2/floor2_map.jpg");
			character->setBiome(2);
			music->StopSound();
			music = new Music();
			music->desertBiome();
		}
		else {
			pbMap->Image = Image::FromFile("images/map/3/floor3_map.jpg");
			character->setBiome(4);
			music->StopSound();
			music = new Music();
			music->lavaBiome();
		}
		riddles = new Riddles(*character);

		riddleCounter = 0;
		dungeonCreate();
		roomCreate();
		Progress();
	}
}

// biome select, calls dungeonCreate to generate new queue of rooms for next floor
private: System::Void btnRight_Click(System::Object^ sender, System::EventArgs^ e) {
	btnLeft->Visible = false;
	btnRight->Visible = false;
	pbBack->Visible = true;

	if (biomeSelect) {
		biomeSelect = false;
		character->incFloor();
		btnChoiceVisible();

		if (character->getFloor() == 1) {
			character->setBiome(1);
			music = new Music();
			music->jungleBiome();
		}
		else if (character->getFloor() == 2) {
			pbMap->Image = Image::FromFile("images/map/2/floor2_map.jpg");
			character->setBiome(3);
			music->StopSound();
			music = new Music();
			music->ghostBiome();
		}
		else {
			pbMap->Image = Image::FromFile("images/map/3/floor3_map.jpg");
			character->setBiome(4);
			music->StopSound();
			music = new Music();
			music->lavaBiome();
		}
		riddles = new Riddles(*character);

		riddleCounter = 0;
		dungeonCreate();
		roomCreate();
		Progress();
	}
}

// Make all choice buttons visible
private: void btnChoiceVisible() {
	btnChoice1->Visible = true;
	btnChoice2->Visible = true;
	btnChoice3->Visible = true;
	btnChoice4->Visible = true;
}

// Make all choice buttons invisible
private: void btnChoiceInvisible() {
	btnChoice1->Visible = false;
	btnChoice2->Visible = false;
	btnChoice3->Visible = false;
	btnChoice4->Visible = false;
}

// Make all answer buttons visible
private: void btnAnswersVisible() {
	btnAnswer1->Visible = true;
	btnAnswer2->Visible = true;
	btnAnswer3->Visible = true;
}

// Make all answer buttons invisible
private: void btnAnswersInvisible() {
	btnAnswer1->Visible = false;
	btnAnswer2->Visible = false;
	btnAnswer3->Visible = false;
}

private: System::Void btnChoice1_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 1

	btnContinue->Visible = true;

	chestOpen = false;

	if (room->getType() == "Battle") {
		bool isBoss = false;
		redReader->Text = "";
		if (character->getRoomCounter() % 10 == 9) { // 9th room on floor is boss
			isBoss = true; 
		}
		if (currentBattle == nullptr) {
			// Initialize the Battle object (start the battle)
			currentBattle = new Battle(isBoss, *character);
			btnAttack->Visible = true;  // Make the attack button visible
			btnContinue->Visible = false;
			battleState = true;
		}
	}
	// get text files from specific rooms 
	else if ((character->getRoomCounter() == 4) || (character->getRoomCounter() == 6) || (character->getRoomCounter() == 15) || (character->getRoomCounter() == 16) || (character->getRoomCounter() == 21) || (character->getRoomCounter() == 22) || (character->getRoomCounter() == 23) || (character->getRoomCounter() == 27)) {
		string temp = room->getbtnChoice1(*character);

		int ptr = temp.find_last_of(" ");

		string txtName = temp.substr(ptr + 1, temp.length());
		temp.erase(ptr, temp.length());

		redReader->Text = gcnew String(temp.c_str());

		if (redReader->Text == "") {
			redReader->Text = File::ReadAllText(gcnew String(txtName.c_str()));
		}
		else {
			redReader->Text = redReader->Text + "\n\n" + File::ReadAllText(gcnew String(txtName.c_str()));
		}
	}
	else {
		redReader->Text = gcnew String(room->getbtnChoice1(*character).c_str()); // Output feedback for decision
	}

	showStats();

	// No longer in active room
	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
	
	if (battleState) {
		btnContinue->Visible = false;
	}

}

private: System::Void btnChoice2_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 2

	redReader->Text = gcnew String(room->getbtnChoice2(*character).c_str()); // Output feedback for decision

	showStats();

	btnContinue->Visible = true;

	// No longer in active room
	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
}

private: System::Void btnChoice3_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 3

	btnContinue->Visible = true;

	// get text file for specific room
	if (character->getRoomCounter() == 8) {
		string temp = room->getbtnChoice3(*character);

		int ptr = temp.find_last_of(" ");

		string txtName = temp.substr(ptr + 1, temp.length());
		temp.erase(ptr, temp.length());

		redReader->Text = gcnew String(temp.c_str());

		if (redReader->Text == "") {
			redReader->Text = File::ReadAllText(gcnew String(txtName.c_str()));
		}
		else {
			redReader->Text = redReader->Text + "\n\n" + File::ReadAllText(gcnew String(txtName.c_str()));
		}
	}
	else {
		redReader->Text = gcnew String(room->getbtnChoice3(*character).c_str()); // Output feedback for decision
		}

	showStats();

	// No longer in active room
	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
}

private: System::Void btnChoice4_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 4

	redReader->Text = gcnew String(room->getbtnChoice4(*character).c_str()); // Output feedback for decision

	btnContinue->Visible = true;
	
	showStats();

	// No longer in active room
	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
}

private: System::Void btnAnswer1_Click(System::Object^ sender, System::EventArgs^ e) {
	// check whether player's answer is correct
	if ((gcnew String(riddles->getAnswerCorrect().at(riddleCounter).c_str()))->Equals(gcnew String(btnAnswer1->Text))) {
		correctRiddleAnswer();
		character->incRiddleCorrect();
	}
	else {
		incorrectRiddleAnswer();
	}
	riddleCounter++;
	feedback->maxRiddles++;
	riddleAnswered();

	// Riddle feedback
	redReader->Text = redReader->Text + " You have gotten " + character->getRiddleCorrect() + " out of " + feedback->maxRiddles + " riddles correct.";
}

private: System::Void btnAnswer2_Click(System::Object^ sender, System::EventArgs^ e) {
	// check whether player's answer is correct
	if ((gcnew String(riddles->getAnswerCorrect().at(riddleCounter).c_str()))->Equals(gcnew String(btnAnswer2->Text))) {
		correctRiddleAnswer();
		character->incRiddleCorrect();
	}
	else {
		incorrectRiddleAnswer();
	}
	riddleCounter++;
	feedback->maxRiddles++;
	riddleAnswered();

	// Riddle feedback
	redReader->Text = redReader->Text + " You have gotten " + character->getRiddleCorrect() + " out of " + feedback->maxRiddles + " riddles correct.";
}

private: System::Void btnAnswer3_Click(System::Object^ sender, System::EventArgs^ e) {
	// check whether player's answer is correct
	if ((gcnew String(riddles->getAnswerCorrect().at(riddleCounter).c_str()))->Equals(gcnew String(btnAnswer3->Text))) {
		correctRiddleAnswer();
		character->incRiddleCorrect();
	}
	else {
		incorrectRiddleAnswer();
	}
	riddleCounter++;
	feedback->maxRiddles++;
	riddleAnswered();

	// Riddle feedback
	redReader->Text = redReader->Text + " You have gotten " + character->getRiddleCorrect() + " out of " + feedback->maxRiddles + " riddles correct.";
}

private: void incorrectRiddleAnswer() {
	// Decrease the character's stats
	decStats(0.5);

	// Display the incorrect answer message
	redReader->Text = "Incorrect! You have failed the riddle!\n\n" + redReader->Text;

	// Flash the screen red
	flashScreenRed();

	if (room->getType() == "Chest") { // check whether player is in chest room
		chestOpen = false; // chest is locked
		redReader->Text = redReader->Text + " The chest will remain locked.";
	}
}

private: void correctRiddleAnswer() {
	// Increase the character's stats
	incStats(0.75);

	// Display the correct answer message
	redReader->Text = "Correct! You have answered the riddle!\n\n" + redReader->Text;

	// Flash the screen green
	flashScreenGreen();

	if (room->getType() == "Chest") { // check if player is in chest room
		chestOpen = true; // chest is open
		redReader->Text = redReader->Text + " The chest is now unlocked.";
	}
}

private: System::Void btnAttack_Click(System::Object^ sender, System::EventArgs^ e) {
	// Perform the attack action if strikes are remaining and battle isn't over
	btnAttack->Text = "Attack";
	if (!currentBattle->isBattleFinished()) {
		// Perform the attack
		bool critStrike = currentBattle->attack(*character);  // Player attacks the enemy
		float damage = currentBattle->damageTaken();
		if (!currentBattle->getEnemy()->isDefeated()) {
			if (critStrike) {
				// Inform player of critical strike
				redReader->Text = redReader->Text + "You attacked! CRITICAL HIT!!\nStrikes remaining : "
					+ currentBattle->getStrikesRemaining().ToString() + "\nEnemy health remaining: "
					+ currentBattle->getEnemy()->getHealth() + "\n" + "Monster attacked you: -"
					+ gcnew String(to_string(damage).substr(0, to_string(damage).find_first_of(".") + 3).c_str()) + " health\n";
			}
			else {
				// Inform player of attack
				redReader->Text = redReader->Text + "You attacked!\nStrikes remaining : "
					+ currentBattle->getStrikesRemaining().ToString() + "\nEnemy health remaining: "
					+ currentBattle->getEnemy()->getHealth() + "\n" + "Monster attacked you: -" 
					+ gcnew String(to_string(damage).substr(0, to_string(damage).find_first_of(".") + 3).c_str()) + " health\n";
			}
		}
		else {
			btnAttack->Visible = false;  // Hide the attack button after the battle is over
			btnAttack->Text = "Continue";
			flashScreenGreen();

			if (character->getRoomCounter() == 29) { // check whether player is in final boss battle
				malvelDefeated = true; // player beat final boss
			}
			else {
				// increase stats if player wins battle
				redReader->Text = "The enemy has been defeated! Moving to the next room.\n\n" 
					+ gcnew String(character->incStatsDisplay(1).c_str()) + "\n\n";
				btnContinue->Visible = true;
				character->incBattlesWon();

				if (character->getRoomCounter() == 19) {
					if (character->getBiome() == 2) {
						character->incReputation();
						redReader->Text = redReader->Text + File::ReadAllText("textfiles/biomes/desert/battle/desertbosswin.txt");
					}
					else {
						character->incReputation();
						redReader->Text = redReader->Text + File::ReadAllText("textfiles/biomes/ghost/battle/ghostbosswin.txt");
					}
				}

				character->incStats(1);

				showStats();
			}

			btnContinue->Visible = true;
			
			// check whether it is a boss battle (no riddles in boss rooms)
			if (currentBattle->getIsBoss()) {
				riddleState = false;
			}
			else {
				riddleState = true;
			}
		}
	}

	if (currentBattle->isBattleFinished()) {
		// Check if the battle is finished (either strikes are 0 or the enemy is defeated)
		btnAttack->Visible = false;  // Hide the attack button after the battle is over

		// If the enemy is defeated, move to the next room
		if (!currentBattle->getEnemy()->isDefeated()) {
			if (character->getRoomCounter() == 29) { // check whether player is in final boss battle
				malvelDefeated = false; // player lost to final boss
			}
			else {
				// reduce stats if player loses battle
				btnAttack->Text = "Continue";
				redReader->Text = "You could not defeat the enemy.\n\n" 
					+ gcnew String(character->decStatsDisplay(0.75).c_str()) + "\n\n";

				character->decStats(0.75);

				showStats();
			}

			btnContinue->Visible = true;
			flashScreenRed();

			// check whether it is a boss battle (no riddles in boss rooms)
			if (currentBattle->getIsBoss()) {
				riddleState = false;
			}
			else {
				riddleState = true;
			}
		}
		
		delete currentBattle; // Clean up the current battle after it's finished
		currentBattle = nullptr;  // Reset the battle reference for the next round
	}
	showStats();
}

private: System::Void tmrRiddle_Tick(System::Object^ sender, System::EventArgs^ e) {
	progRiddle->Value -= 1;
	if (progRiddle->Value == 0) { //check whether player has run out of time
		tmrRiddle->Stop();
		progRiddle->Visible = false;
		btnAnswersInvisible();
		btnContinue->Visible = true;
		riddleCounter++;
		feedback->maxRiddles++;

		// decrease stats
		decStats(0.5);
		// Inform player that time has run out
		redReader->Text = "The riddle keeper shouts \"Time's up! You have failed the riddle!\" " + redReader->Text;
		redReader->Text = redReader->Text + " You have gotten " + character->getRiddleCorrect() + " out of " + feedback->maxRiddles + " riddles correct.";

		flashScreenRed();
	}
}

//show lore page when profile picture is clicked
private: System::Void pbProfile_Click(System::Object^ sender, System::EventArgs^ e) {
	Lore^ lore = gcnew Lore(this);
	lore->Visible = true;
	this->Visible = false;
}

// function to load the background image and text file for each room
private: void readerAndBackground() {
	redReader->Text = File::ReadAllText(gcnew String(room->getTextFileName(character->getBiome()).c_str()));
	pbBackground->Image = Image::FromFile(gcnew String(room->getImageFileName(character->getBiome()).c_str()));
}

// function called when player selects an answer to a riddle
private: void riddleAnswered() {
	tmrRiddle->Stop();
	progRiddle->Visible = false;
	btnAnswersInvisible();
	btnContinue->Visible = true;
	showStats();
}

// functiom called when player gets riddle wrong/loses battle
private: void flashScreenRed() {
	this->BackColor = System::Drawing::Color::Red;

	// Wait a short time but still process UI events
	System::Windows::Forms::Application::DoEvents();
	System::Threading::Thread::Sleep(1000); // 1000 ms flash

	// Change background back to black
	this->BackColor = System::Drawing::Color::Black;
	System::Threading::Thread::Sleep(200);
}

// functiom called when player gets riddle right/wins battle
private: void flashScreenGreen() {
	this->BackColor = System::Drawing::Color::Green;

	// Wait a short time but still process UI events
	System::Windows::Forms::Application::DoEvents();
	System::Threading::Thread::Sleep(1000); // 1000 ms flash

	// Change background back to black
	this->BackColor = System::Drawing::Color::Black;
	System::Threading::Thread::Sleep(200);
}

//function is called whenever stats have changed
private: void showStats() {
	lbStats->Items->Clear();
	for (int i = 0; i < 6; i++) {
		string temp = to_string(character->getStatValue(i)).substr(0, to_string(character->getStatValue(i)).find_first_of(".") + 3);
		lbStats->Items->Add(gcnew String((character->getStatName(i) + " : " + temp).c_str()));
	}
}

// function called when player gets riddle correct
private: void incStats(float increase){
	character->incStats(increase);
	showStats();
	redReader->Text = gcnew String(character->incStatsDisplay(increase).c_str());
}

// function called when player gets riddle wrong/runs out of time
private: void decStats(float decrease) {
	character->decStats(decrease);
	showStats();
	redReader->Text = gcnew String(character->decStatsDisplay(decrease).c_str());
}

// Generates entrance room to each level
private: void floorEntrance() {
	pbBackground->Image = Image::FromFile("images/twodoor.jpg");
	redReader->Text = File::ReadAllText("textfiles/entrance/entrance.txt");
}

// Updates the progress label
private: void Progress() {
	lblProgress->Visible = true;
	lblProgress->Text = "Floor " + character->getFloor() + " :Room " + (character->getRoomCounter() % 10);
}

// Give player feedback once they have completed the game
private: void outputFeedback() {
	int score = 0;
	for (int i = 0; i < 6; i++) {
		score += character->getStatValue(i); // score is accumulation of stat values
	}

	feedback->score = score;
	feedback->reputation = character->getReputation();
	feedback->battlesWon = character->getBattlesWon();
	feedback->riddlesCorrect = character->getRiddleCorrect();

	// Give feedback to player
	redReader->Text = "Score: " + feedback->score + "\nKey events: " + feedback->reputation + "/" + feedback->maxReputation +
		"\nBattles won: " + feedback->battlesWon + "/" + feedback->maxBattles + "\nRiddles correct: " + feedback->riddlesCorrect + 
		"/" + feedback->maxRiddles + "\nPlaytime: " + feedback->duration;
	character->incRoomCounter();
	delete feedback;
}

// Randomise first three rooms
private: void randomRooms(int randomNum1, int randomNum2, int randomNum3) {
	if (randomNum1 == 1) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Library()));
	}
	else if (randomNum1 == 2) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Chest()));
	}
	else if (randomNum1 == 3) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(false, *character)));
	}

	if (randomNum2 == 1) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Library()));
	}
	else if (randomNum2 == 2) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Chest()));
	}
	else if (randomNum2 == 3) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(false, *character)));
	}

	if (randomNum3 == 1) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Library()));
	}
	else if (randomNum3 == 2) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Chest()));
	}
	else if (randomNum3 == 3) {
		roomLoad->Enqueue(gcnew ManagedRoomBase(new Battle(false, *character)));
	}
}

// Set location and size of all components
private: void setGameScreen() {
	redReader->Text = File::ReadAllText("textfiles/introduction/introduction.txt");
	this->Size = System::Drawing::Size(1300, 1000);
	this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
	redReader->Location = System::Drawing::Point(181, 663);
	redReader->Size = System::Drawing::Size(1098, 282);
	redReader->Font = (gcnew System::Drawing::Font(L"Book Antiqua", 14, System::Drawing::FontStyle::Regular,
		System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
	pbProfile->Location = System::Drawing::Point(0, 0);
	pbProfile->Size = System::Drawing::Size(182, 174);
	pbMap->Location = System::Drawing::Point(1097, 0);
	pbMap->Size = System::Drawing::Size(182, 174);
	progRiddle->Location = System::Drawing::Point(188, 0);
	progRiddle->Size = System::Drawing::Size(903, 53);
	pbAbility->Location = System::Drawing::Point(0, 494);
	pbAbility->Size = System::Drawing::Size(177, 163);
	gbButtons->Location = System::Drawing::Point(181, 565);
	gbButtons->Size = System::Drawing::Size(1098, 92);
	pbBackground->Location = System::Drawing::Point(0, 0);
	pbBackground->Size = System::Drawing::Size(1279, 657);
	pbSword->Location = System::Drawing::Point(30, 211);
	pbSword->Size = System::Drawing::Size(284, 271);
	pbThrowingKnife->Location = System::Drawing::Point(348, 211);
	pbThrowingKnife->Size = System::Drawing::Size(284, 271);
	pbWand->Location = System::Drawing::Point(669, 211);
	pbWand->Size = System::Drawing::Size(284, 271);
	pbLongbow->Location = System::Drawing::Point(982, 211);
	pbLongbow->Size = System::Drawing::Size(284, 271);
	pbBack->Location = System::Drawing::Point(0, 860);
	pbBack->Size = System::Drawing::Size(88, 85);
	lbStats->Location = System::Drawing::Point(0, 663);
	lbStats->Size = System::Drawing::Size(176, 124);
	lbStats->Font = (gcnew System::Drawing::Font(L"Copperplate Gothic Light", 11, System::Drawing::FontStyle::Regular,
		System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
	lbStats->ItemHeight = 20;
	btnAnswer1->Location = System::Drawing::Point(7, 12);
	btnAnswer1->Size = System::Drawing::Size(143, 74);
	btnAnswer2->Location = System::Drawing::Point(219, 12);
	btnAnswer2->Size = System::Drawing::Size(143, 74);
	btnAnswer3->Location = System::Drawing::Point(440, 12);
	btnAnswer3->Size = System::Drawing::Size(143, 74);
	btnChoice1->Location = System::Drawing::Point(7, 12);
	btnChoice1->Size = System::Drawing::Size(143, 74);
	btnChoice2->Location = System::Drawing::Point(219, 12);
	btnChoice2->Size = System::Drawing::Size(143, 74);
	btnChoice3->Location = System::Drawing::Point(440, 12);
	btnChoice3->Size = System::Drawing::Size(143, 74);
	btnChoice4->Location = System::Drawing::Point(664, 12);
	btnChoice4->Size = System::Drawing::Size(143, 74);
	btnLeft->Location = System::Drawing::Point(7, 12);
	btnLeft->Size = System::Drawing::Size(143, 74);
	btnRight->Location = System::Drawing::Point(219, 12);
	btnRight->Size = System::Drawing::Size(143, 74);
	btnContinue->Location = System::Drawing::Point(938, 12);
	btnContinue->Size = System::Drawing::Size(143, 74);
	btnAttack->Location = System::Drawing::Point(7, 12);
	btnAttack->Size = System::Drawing::Size(143, 74);
	lblProgress->Location = System::Drawing::Point(1093, 177);
	lblProgress->Size = System::Drawing::Size(0, 22);
}

// Exit application when form is closed
private: System::Void GameScreen_FormClosed(System::Object^ sender, System::Windows::Forms::FormClosedEventArgs^ e) {
	Application::Exit();
}
private: System::Void lbStats_MouseHover(System::Object^ sender, System::EventArgs^ e) {

}
private: System::Void lbStats_MouseEnter(System::Object^ sender, System::EventArgs^ e) {
	/*// Use a ToolTip to display the information instead of calling Show on the ListBox
	System::Windows::Forms::ToolTip^ toolTip = gcnew System::Windows::Forms::ToolTip();
	toolTip->SetToolTip(lbStats, "Health. \nYour lifeline in battle. If your health reaches zero, you'll be sent back to the main screen and must " +
		"restart your journey. \nStrength\nDetermines how much damage you deal to enemies. The higher your strength, the more powerful your " +
		"attacks. \nAgility\nDictates how many strikes you get per battle. Use them wisely—if you fail to defeat your opponent within your " +
		"allotted strikes, you lose the fight.\nAccuracy\nIncreases your chance of landing a critical hit, which deals extra damage beyond " +
		"your normal strength.\nWisdom\nGrants you more time to solve riddles. The wiser you are, the more time you’ll have to think." +
		"\nObservation\nIf your observation is above 15, the game helps you out by narrowing riddle choices down to just two: one correct " +
		"and one incorrect. A sharp eye makes all the difference.");
	*/
}
};
}