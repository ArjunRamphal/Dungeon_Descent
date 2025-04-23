#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <msclr\marshal_cppstd.h>
#include <msclr\marshal.h>
#include "Character.h"
#include "Warrior.h"
#include "Ranger.h"
#include "Mage.h"
#include "Rogue.h"
#include "test.h"
#include "RoomBase.h"
#include "Battle.h"
#include "Library.h"
#include "Respite.h"
#include "Shop.h"
#include "Chest.h"
#include "Boss.h"
#include "Riddles.h"
#include "Lore.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/VertexBuffer.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/BlendMode.hpp>




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

		GameScreen(Form^ obj1)
		{
			obj = obj1;
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

		Character* character;
		Riddles* riddles;
		Battle* currentBattle = nullptr;
		bool isBoss = false;
		bool isEnlarged = false;
		bool pathChoice = true;
		bool completed = false;
		bool LibState = false;
		bool roomState = false;
		bool biomeSelect = true;
		bool riddleState = false;
		int riddleCorrect = 0;
		bool chestOpen = false;
		bool battleState = false;
		RoomBase* temp = new RoomBase();
		RoomBase* room;
		int globalCorrect = 1;
		bool ContState;
		int biome;
		int floor = 0;
		int roomCounter = 0;
		int num = 3;
		int randomNum1;
		int randomNum2;
		int randomNum3;
		int riddleCounter = 0;
		int randomAnswer1;
		int randomAnswer2;
		int randomAnswer3;
	private: System::Windows::Forms::PictureBox^ pbBackground;
	public: System::Windows::Forms::Timer^ tmrRiddle;
	private: System::Windows::Forms::Button^ btnRight;
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
			// GameScreen
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(9, 20);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::Color::Black;
			this->ClientSize = System::Drawing::Size(1278, 944);
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
			this->Name = L"GameScreen";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"GameScreen";
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

		}
#pragma endregion
private: System::Void GameScreen_Shown(System::Object^ sender, System::EventArgs^ e) {
	redReader->Text = File::ReadAllText("introduction.txt");
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
	lbStats->Font = (gcnew System::Drawing::Font(L"Copperplate Gothic Light", 9, System::Drawing::FontStyle::Regular,
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

}

private: System::Void pbSword_Click(System::Object^ sender, System::EventArgs^ e) {
	//Warrior* warrior = ;
	character = new Warrior();
	weaponPick();
}

private: System::Void pbThrowingKnife_Click(System::Object^ sender, System::EventArgs^ e) {
	Rogue* rogue = new Rogue();
	character = rogue;
	weaponPick();
}

private: System::Void pbWand_Click(System::Object^ sender, System::EventArgs^ e) {
	Mage* mage = new Mage();
	character = mage;
	weaponPick();
}
private: System::Void pbLongbow_Click(System::Object^ sender, System::EventArgs^ e) {
	Ranger* ranger = new Ranger();
	character = ranger;
	weaponPick();
}

private: void weaponPick() {
	pbMap->Visible = true;
	pbProfile->Visible = true;
	pbProfile->Image = Image::FromFile(gcnew String(character->getpfpImageName().c_str()));
	pbAbility->Image = Image::FromFile(gcnew String(character->getAbilityFileName().c_str()));

	pictureBoxWeaponInvisible();

	roomCreate();

	pbBackground->Image = Image::FromFile("twodoor.jpg");
	redReader->Text = File::ReadAllText("entrance.txt");

	showStats();
}

private: void pictureBoxWeaponInvisible() {
	pbSword->Visible = false;
	pbThrowingKnife->Visible = false;
	pbWand->Visible = false;
	pbLongbow->Visible = false;
}

private: void showStats() {
	lbStats->Items->Clear();
	for (int i = 0; i < 6; i++) {
		lbStats->Items->Add(gcnew String((character->getStatName(i) + " : " + to_string(character->getStatValue(i))).c_str()));
	}
}
private: System::Void pbMap_Click(System::Object^ sender, System::EventArgs^ e) {
	//Expand and shrink minimap
	if (!isEnlarged) {
		pbMap->Size = System::Drawing::Size(600, 600);
		pbMap->Location = System::Drawing::Point(400, 50);
		isEnlarged = true;
	}
	else {
		pbMap->Size = System::Drawing::Size(182, 174);
		pbMap->Location = System::Drawing::Point(1097, 0);
		isEnlarged = false;
	}
}
private: System::Void pbBack_Click(System::Object^ sender, System::EventArgs^ e) {
	// Exit the game and return to the main menu
	System::Windows::Forms::DialogResult result = MessageBox::Show(
		"Are you sure you want to exit? All progress will be lost.",
		"Exit",
		MessageBoxButtons::YesNo,
		MessageBoxIcon::Warning,
		MessageBoxDefaultButton::Button2
	);

	if (result == System::Windows::Forms::DialogResult::Yes) {
		this->Visible = false;
		obj->Visible = true;
	}
}

private: void roomCreate()
{
	//this function will use the same variable room for all rooms in the game
	//only one room will be generated at a time

	//have a global counter varibale to store what room the player is in.
	//eg. int i = 0..30;

	//have a case statement for the counter i, to see which room the player is in. If the player is in a room that
	//needs to be randomly generated, then generate it.
	//if the player is in a definite room, then just generate that room

	//there will be a boolean completed to check whether the player has completed the room or not
	//the room will count as completed once the player has answered the riddle

	//each room class will have a questions bank and an answers bank to randomly generate questions
	// and a function to get those questions and answers.
	//once completed = true and player decides to go to next room, the room will be generated

	//to check whether the player has completed a floor, do a mod calculation, eg. 20 % 10 = 0, therefore 
	//you will know that the player is proceeding to the next floor. if i = 30, then the player has completed the game


	completed = false;
	roomState = true;
	battleState = false;
	btnContinue->Visible = false;
	pbAbility->Visible = false;

	if ((roomCounter % 10) == 1)
	{
		// Seed the random number generator
		srand(time(0));

		// Generate a random number between 1 and 3
		randomNum1 = (rand() % num) + 1;

		if (randomNum1 == 1) {
			riddleState = true;
			Library* library = new Library();
			room = library;
			readerAndBackground();
			btnChoiceVisible();
			if ((biome == 0) || (biome == 1)) {
				btnChoice1->Text = "The Art of Warfare";
				btnChoice2->Text = "The Whispering Winds";
				btnChoice3->Text = "The Ancient Lore";
				btnChoice4->Text = "The Heart of the Forest";
			}
			else if (biome == 2) {
				btnChoice1->Text = "The Sands of Strategy";
				btnChoice2->Text = "The Silent Zephyrs";
				btnChoice3->Text = "The Hidden Glyphs";
				btnChoice4->Text = "The Oasis Eye";
			}
			else if (biome == 3) {
				btnChoice1->Text = "The Cyclops Might";
				btnChoice2->Text = "The Immortal Alucard";
				btnChoice3->Text = "The Weeping Wolfman";
				btnChoice4->Text = "The Eyes of the Raven";
			}
			else if (biome == 4) {
				btnChoice1->Text = "The Princess Diary";
				btnChoice2->Text = "The Smoldering Whisper";
				btnChoice3->Text = "The Magma Script";
				btnChoice4->Text = "The Ashen Eye";
			}
			room->type = "Library";
		}
		else if (randomNum1 == 2) {
			Battle* battle = new Battle(false, *character);
			room = battle;
			readerAndBackground();
			room->type = "Battle";
			pbAbility->Visible = true;

			btnChoiceVisible();
			btnChoice3->Visible = false;
			btnChoice4->Visible = false;
			btnChoice1->Text = "Confront";
			btnChoice2->Text = "Sneak past";

		}
		else if (randomNum1 == 3) {
			riddleState = true;
			Chest* chest = new Chest();
			room = chest;
			readerAndBackground();
			room->type = "Chest";

			btnChoiceInvisible();
			btnContinue->Visible = true;
		}
	}
	else if ((roomCounter % 10) == 2)
	{
		// Seed the random number generator
		srand(time(0));

		do {
			randomNum2 = (rand() % num) + 1;
		} while (randomNum2 == randomNum1);

		if (randomNum2 == 1) {
			riddleState = true;
			Library* library = new Library();
			room = library;
			readerAndBackground();
			btnChoiceVisible();
			if ((biome == 0) || (biome == 1)) {
				btnChoice1->Text = "The Art of Warfare";
				btnChoice2->Text = "The Whispering Winds";
				btnChoice3->Text = "The Ancient Lore";
				btnChoice4->Text = "The Heart of the Forest";
			}
			else if (biome == 2) {
				btnChoice1->Text = "The Sands of Strategy";
				btnChoice2->Text = "The Silent Zephyrs";
				btnChoice3->Text = "The Hidden Glyphs";
				btnChoice4->Text = "The Oasis Eye";
			}
			else if (biome == 3) {
				btnChoice1->Text = "The Cyclops Might";
				btnChoice2->Text = "The Immortal Alucard";
				btnChoice3->Text = "The Weeping Wolfman";
				btnChoice4->Text = "The Eyes of the Raven";
			}
			else if (biome == 4) {
				btnChoice1->Text = "The Princess Diary";
				btnChoice2->Text = "The Smoldering Whisper";
				btnChoice3->Text = "The Magma Script";
				btnChoice4->Text = "The Ashen Eye";
			}
			room->type = "Library";
		}
		else if (randomNum2 == 2) {
			Battle* battle = new Battle(false, *character);
			room = battle;
			readerAndBackground();
			room->type = "Battle";
			pbAbility->Visible = true;

			btnChoiceVisible();
			btnChoice3->Visible = false;
			btnChoice4->Visible = false;
			btnChoice1->Text = "Confront";
			btnChoice2->Text = "Sneak past";
			
		}
		else if (randomNum2 == 3) {
			riddleState = true;
			Chest* chest = new Chest();
			room = chest;
			readerAndBackground();
			room->type = "Chest";

			btnContinue->Visible = true;
		}
	}
	else if ((roomCounter % 10) == 3)
	{
		// Seed the random number generator
		srand(time(0));

		// Generate a random number between 1 and 3
		do {
			randomNum3 = (rand() % num) + 1;
		} while ((randomNum3 == randomNum2) || (randomNum3 == randomNum1));

		if (randomNum3 == 1) {
			riddleState = true;
			Library* library = new Library();
			room = library;
			readerAndBackground();
			btnChoiceVisible();
			if ((biome == 0) || (biome == 1)) {
				btnChoice1->Text = "The Art of Warfare";
				btnChoice2->Text = "The Whispering Winds";
				btnChoice3->Text = "The Ancient Lore";
				btnChoice4->Text = "The Heart of the Forest";
			}
			else if (biome == 2) {
				btnChoice1->Text = "The Sands of Strategy";
				btnChoice2->Text = "The Silent Zephyrs";
				btnChoice3->Text = "The Hidden Glyphs";
				btnChoice4->Text = "The Oasis Eye";
			}
			else if (biome == 3) {
				btnChoice1->Text = "The Cyclops Might";
				btnChoice2->Text = "The Immortal Alucard";
				btnChoice3->Text = "The Weeping Wolfman";
				btnChoice4->Text = "The Eyes of the Raven";
			}
			else if (biome == 4) {
				btnChoice1->Text = "The Princess Diary";
				btnChoice2->Text = "The Smoldering Whisper";
				btnChoice3->Text = "The Magma Script";
				btnChoice4->Text = "The Ashen Eye";
			}
			room->type = "Library";
		}
		else if (randomNum3 == 2) {
			Battle* battle = new Battle(false, *character);
			room = battle;
			readerAndBackground();
			room->type = "Battle";
			pbAbility->Visible = true;

			btnChoiceVisible();
			btnChoice3->Visible = false;
			btnChoice4->Visible = false;
			btnChoice1->Text = "Confront";
			btnChoice2->Text = "Sneak past";
		}
		else if (randomNum3 == 3) {
			riddleState = true;
			Chest* chest = new Chest();
			room = chest;
			readerAndBackground();
			room->type = "Chest";

			btnContinue->Visible = true;
		}
	}
	else if (roomCounter == 4) {
		Respite* respite = new Respite();
		room = respite;
		riddleState = true;
		if (biome == 0) {
			redReader->Text = File::ReadAllText("iceprison.txt");
		}
		else if (biome == 1) {
			redReader->Text = File::ReadAllText("jungleprison.txt");
		}
		pbBackground->Image = Image::FromFile("prison.jpeg");
		
		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Spare warrior";
		btnChoice2->Text = "Kill warrior";
		room->type = "Encounter";
	}
	else if (roomCounter == 5) {
		Battle* battle = new Battle(false, *character);
		room = battle;
		readerAndBackground();
		room->type = "Battle";
		pbAbility->Visible = true;

		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Confront";
		btnChoice2->Text = "Sneak past";
	}
	else if (roomCounter == 6) {
		Respite* respite = new Respite();
		room = respite;
		riddleState = true;
		readerAndBackground();
		
		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Restore health";
		btnChoice2->Text = "Search room";
		room->type = "Respite";
	}
	else if (roomCounter == 7) {
		Library* library = new Library();
		room = library;
		riddleState = true;
		readerAndBackground();
		btnChoiceVisible();

		btnChoice1->Text = "The Art of Warfare";
		btnChoice2->Text = "The Whispering Winds";
		btnChoice3->Text = "The Ancient Lore";
		btnChoice4->Text = "The Heart of the Forest";
		room->type = "Library";
	}
	else if (roomCounter == 8) {
		//Shop* shop = new Shop();
		room = new Shop();
		riddleState = true;
		redReader->Text = File::ReadAllText("shopkeeper.txt");
		pbBackground->Image = Image::FromFile(gcnew String(room->getImageFileName(biome).c_str()));
		room->type = "Shop";
		btnChoice1->Visible = true;
		btnChoice2->Visible = true;
		btnChoice3->Visible = true;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Restore health";
		btnChoice2->Text = "Stat increase";
		btnChoice3->Text = "Book of Knowledge";
	}
	else if ((roomCounter % 10) == 9) {
		Boss* boss = new Boss();
		room = boss;
		readerAndBackground();
		room->type = "Boss";
		pbAbility->Visible = true;
		btnChoice1->Visible = true;
		btnChoice1->Text = "Confront";
	}
	else if (roomCounter == 14) {
		Shop* shop = new Shop();
		room = shop;
		riddleState = true;
		readerAndBackground();
		if (biome == 2) {
			btnChoice1->Text = "Draught of Revitalization";
			btnChoice2->Text = "Essence of Fortitude";
			btnChoice3->Text = "Scroll of Ancient Wisdom";
		}
		else if (biome == 3) {
			btnChoice1->Text = "Elixir of Restless Slumber";
			btnChoice2->Text = "Phantom’s Breath";
			btnChoice3->Text = "Scroll of Lost Echoes";
		}
		btnChoiceVisible();
		btnChoice4->Visible = false;
		room->type = "Shop";
	}
	else if (roomCounter == 15) {
		Chest* chest = new Chest();
		room = chest;
		riddleState = true;
		readerAndBackground();
		room->type = "Chest";
		btnContinue->Visible = true;
	}
	else if (roomCounter == 16) {
		Respite* respite = new Respite();
		room = respite;
		riddleState = true;
		if (biome == 2) {
			redReader->Text = File::ReadAllText("desertbutler.txt");
			pbBackground->Image = Image::FromFile("desertbutler.jpeg");
		}
		else if (biome == 3) {
			redReader->Text = File::ReadAllText("ghostbutler.txt");
			pbBackground->Image = Image::FromFile("ghostbutler.jpeg");
		}
		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Save butler";
		btnChoice2->Text = "Take vial";
		room->type = "Encounter";
	}
	else if (roomCounter == 17) {
		Respite* respite = new Respite();
		room = respite;
		riddleState = true;
		readerAndBackground();
		room->type = "Respite";
		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Restore health";
		btnChoice2->Text = "Search room";
	}
	else if (roomCounter == 18) {
		Battle* battle = new Battle(false, *character);
		room = battle;
		readerAndBackground();
		if (biome == 3) {
			redReader->Text = File::ReadAllText("ghostbattle2.txt");
			pbBackground->Image = Image::FromFile("ghostbattle2.jpeg");
		}
		pbAbility->Visible = true;
		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Confront";
		btnChoice2->Text = "Sneak past";
		room->type = "Battle";
	}
	else if (roomCounter == 24) {
		Chest* chest = new Chest();
		room = chest;
		riddleState = true;
		readerAndBackground();
		room->type = "Chest";
		btnContinue->Visible = true;
	}
	else if (roomCounter == 25) {
		Battle* battle = new Battle(false, *character);
		room = battle;
		readerAndBackground();
		pbAbility->Visible = true;
		redReader->Text = File::ReadAllText("lavabattle2.txt");
		pbBackground->Image = Image::FromFile("lavabattle2.jpg");
		
		btnChoiceVisible();
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		btnChoice1->Text = "Confront";
		btnChoice2->Text = "Sneak past";
		room->type = "Battle";
	}
	else if (roomCounter == 26) {
		Shop* shop = new Shop();
		room = shop;
		riddleState = true;
		readerAndBackground();
		room->type = "Shop";
		btnChoiceVisible();
		btnChoice4->Visible = false;
		btnChoice1->Text = "Emberheart Elixir";
		btnChoice2->Text = "Corebound Tincture";
		btnChoice3->Text = "Inferno Draught";
	}
	else if (roomCounter == 27) {
		Respite* respite = new Respite();
		room = respite;
		riddleState = true;
		redReader->Text = File::ReadAllText("lavaminion.txt");
		pbBackground->Image = Image::FromFile("lavaminions.jpg");
		
		btnChoiceVisible();
		btnChoice1->Text = "Interrogate minions";
		btnChoice2->Text = "Kill minions";
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		room->type = "Encounter";
	}
	else if (roomCounter == 28) {
		Respite* respite = new Respite();
		room = respite;
		riddleState = true;
		readerAndBackground();
		btnChoiceVisible();
		btnChoice1->Text = "Restore health";
		btnChoice2->Text = "Search room";
		btnChoice3->Visible = false;
		btnChoice4->Visible = false;
		room->type = "Respite";
	}

	if (roomCounter != 30) {
		if ((roomCounter % 10) == 0) {
			biomeSelect = true;
			roomState = false;

			if (roomCounter == 20) {
				btnLeft->Text = "Lava biome";
				btnLeft->Visible = true;
			}
			else {
				btnLeft->Visible = true;
				btnRight->Visible = true;
			}
			btnChoiceInvisible();
		}
	}
	else {
		//determine what ending the player gets depending on true ending counter
		int endingCounter = character->getReputation();

		if ((endingCounter > 5) && (riddleCorrect > 14)) {
			redReader->Text = "Congratulations! You have completed the game!";
			btnChoiceInvisible();
		}
		
	}
	roomCounter++;
}

private: System::Void btnContinue_Click(System::Object^ sender, System::EventArgs^ e) {  
	
	if (character->getStatValue(4) == 0){

		System::Windows::Forms::DialogResult result = MessageBox::Show(
			"You have no health left. You cannot continue.",
			"Game Over",
			MessageBoxButtons::OK,
			MessageBoxIcon::Stop,
			MessageBoxDefaultButton::Button1
		);

		if (result == System::Windows::Forms::DialogResult::OK) {
			this->Visible = false;
			obj->Visible = true;
		}
	}
	
	if (riddleState) {
		randomRiddle();
	}
	else if (chestOpen) {
		btnContinue->Visible = false;
		btnChoice1->Visible = true;
		btnChoice1->Text = "Open chest";
	}
	else {
		btnAnswersInvisible();
		roomCreate();
	}

	/*
	//sf::Image image = sf::Image();

	sf::Texture texture("icerespite.png");

	sf::Sprite sprite(texture);

	//this->Controls->Add(sprite);

	// Create a new render-window
	sf::RenderWindow window(sf::VideoMode({ 2048, 2048 }), "SFML window");

	// Create a new render-texture
	sf::RenderTexture rendertexture({ 2048, 2048 });

	// The main loop
	while (window.isOpen())
	{
		
		
		
		// Event processing
		// ...

		// Clear the whole texture with red color
		rendertexture.clear(sf::Color::Red);

		// Draw stuff to the texture
		rendertexture.draw(sprite);  // sprite is a sf::Sprite
		//rendertexture.draw(shape);   // shape is a sf::Shape
		//rendertexture.draw(text);    // text is a sf::Text

		// We're done drawing to the texture
		rendertexture.display();

		// Now we start rendering to the window, clear it first
		window.clear();

		sf::Font font("Arial.ttf");

		sf::Text text(font); // a font is required to make a text object

		// set the string to display
		//text.setString((File::ReadAllText("introduction.txt")));

		// Load the text from the file into a standard string  
		std::string introductionText = msclr::interop::marshal_as<std::string>(File::ReadAllText("introduction.txt"));

		std::u32string uText = std::u32string(introductionText.begin(), introductionText.end());

		// Create an SFML string from the standard string  
		sf::String sfmlText(introductionText);

		text.setString(sfmlText);

		// set the character size
		text.setCharacterSize(24); // in pixels, not points!

		// set the color
		text.setFillColor(sf::Color::White);

		// set the text style
		text.setStyle(sf::Text::Bold | sf::Text::Underlined);

		//text.setPosition(100, 100);
		// Update the problematic line to use sf::Vector2f for setting the position.  
		text.setPosition(sf::Vector2f(0.0f, 1100.0f));
		
		// Draw the texture
		sf::Sprite sprite(rendertexture.getTexture());

		window.draw(sprite);
		
		// inside the main loop, between window.clear() and window.display()
		window.draw(text);
		
		// End the current frame and display its contents on screen
		window.display();
	}*/
}

private: void readerAndBackground() {
	redReader->Text = File::ReadAllText(gcnew String(room->getTextFileName(biome).c_str()));
	pbBackground->Image = Image::FromFile(gcnew String(room->getImageFileName(biome).c_str()));
}

private: void randomRiddle() {
	riddleState = false;
	btnChoiceInvisible();
	btnAnswersVisible();
	btnContinue->Visible = false;

	redReader->Text = gcnew String(riddles->getRiddles().at(riddleCounter).c_str());

	randomAnswer1 = rand() % 3;
	randomAnswer2;
	randomAnswer3;

	do {
		randomAnswer2 = rand() % 3;
	} while (randomAnswer2 == randomAnswer1);

	do {
		randomAnswer3 = rand() % 3;
	} while ((randomAnswer3 == randomAnswer1) || (randomAnswer3 == randomAnswer2));

	btnAnswer1->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer1).c_str());
	btnAnswer2->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer2).c_str());
	btnAnswer3->Text = gcnew String(riddles->getAnswers().at(riddleCounter).at(randomAnswer3).c_str());

	if (floor == 1) {
		progRiddle->Value = 20;
	}
	else if (floor == 2) {
		progRiddle->Maximum = 15;
		progRiddle->Value = 15;
	}
	else if (floor == 3) {
		progRiddle->Maximum = 10;
		progRiddle->Value = 10;
	}

	tmrRiddle->Enabled = true;
	tmrRiddle->Start();
	progRiddle->Visible = true;
}

private: System::Void btnLeft_Click(System::Object^ sender, System::EventArgs^ e) {
	btnLeft->Visible = false;
	btnRight->Visible = false;

	if (biomeSelect) {
		biomeSelect = false;
		floor++;
		character->incFloor();
		btnChoiceVisible();

		if (floor == 1) {
			biome = 0;
		}
		else if (floor == 2) {
			pbMap->Image = Image::FromFile("floor2_map.jpg");
			biome = 2;
		}
		else {
			pbMap->Image = Image::FromFile("floor3_map.jpg");
			biome = 4;
		}
		riddles = new Riddles(floor);
		riddleCounter = 0;
		roomCreate();
	}
}
private: System::Void btnRight_Click(System::Object^ sender, System::EventArgs^ e) {
	btnLeft->Visible = false;
	btnRight->Visible = false;

	if (biomeSelect) {
		biomeSelect = false;
		floor++;
		btnChoiceVisible();

		if (floor == 1) {
			biome = 1;
		}
		else if (floor == 2) {
			pbMap->Image = Image::FromFile("floor2_map.jpg");
			biome = 3;
		}
		else {
			pbMap->Image = Image::FromFile("floor3_map.jpg");
			biome = 4;
		}
		riddles = new Riddles(floor);
		riddleCounter = 0;
		roomCreate();
	}
}

private: void btnChoiceVisible() {
	btnChoice1->Visible = true;
	btnChoice2->Visible = true;
	btnChoice3->Visible = true;
	btnChoice4->Visible = true;
}

private: void btnChoiceInvisible() {
	btnChoice1->Visible = false;
	btnChoice2->Visible = false;
	btnChoice3->Visible = false;
	btnChoice4->Visible = false;
}

private: void btnAnswersVisible() {
	btnAnswer1->Visible = true;
	btnAnswer2->Visible = true;
	btnAnswer3->Visible = true;
}

private: void btnAnswersInvisible() {
	btnAnswer1->Visible = false;
	btnAnswer2->Visible = false;
	btnAnswer3->Visible = false;
}
private: System::Void btnChoice1_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 1

	btnContinue->Visible = true;

	if ((roomCounter % 10) == 2 || (roomCounter % 10) == 3 || (roomCounter % 10) == 4) {
		//2 = Library
		//3 = Battle
		//4 = Chest
		if (room->type == "Library") {
			if (floor == 3) {
				character->incStats(0, 2);
				redReader->Text = gcnew String(character->getStatName(0).c_str()) + " has increased by 2.";
				showStats();

				redReader->Text = redReader->Text + "\n\n" + File::ReadAllText("lavalibrarydiary.txt");
				character->incReputation();
			}
			character->incStats(0, 2);
			redReader->Text = gcnew String(character->getStatName(0).c_str()) + " has increased by 2.";
			showStats();
		}
		else if (room->type == "Battle") {
			// Check if the battle has already started
			if (currentBattle == nullptr) {
				// Initialize the Battle object (start the battle)
				character->floor = floor;
				isBoss = false;
				currentBattle = new Battle(isBoss, *character);
				btnAttack->Visible = true;  // Make the attack button visible
				btnContinue->Visible = false;
				battleState = true;
			}
		}
		else if (room->type == "Chest") {
			chestOpen = false;

			srand(time(0));
			int randomNum = rand() % 6;

			character->incStats(randomNum, 2);
			redReader->Text = gcnew String(character->getStatName(randomNum).c_str()) + " has increased by 2.";
			showStats();
		}
	}
	else if (roomCounter == 5) {
		//5 = Encounter
		redReader->Text = File::ReadAllText("iceprisonfree.txt");
		character->incReputation();
	}
	else if (roomCounter == 6) {
		//6 = Battle
		// Check if the battle has already started
		if (currentBattle == nullptr) {
			// Initialize the Battle object (start the battle)
			character->floor = floor;
			isBoss = false;
			currentBattle = new Battle(isBoss, *character);
			btnAttack->Visible = true;  // Make the attack button visible
			btnContinue->Visible = false;
			battleState = true;
		}
	}
	else if (roomCounter == 7) {
		//7 = Respite
		redReader->Text = File::ReadAllText("respitedrink.txt");
		character->incReputation();
	}
	else if (roomCounter == 8) {
		//8 = Library
		character->incStats(0, 2);
		redReader->Text = gcnew String(character->getStatName(0).c_str()) + " has increased by 2.";
		showStats();
	}
	else if (roomCounter == 9) {
		//9 = Shop
		character->setHealth(character->getBaseHealth());
		showStats();
	}
	else if ((roomCounter % 10) == 0) {
		//10 = Boss
		// Check if the battle has already started
		if (currentBattle == nullptr) {
			// Initialize the Battle object (start the battle)
			character->floor = floor;
			isBoss = true;
			currentBattle = new Battle(isBoss, *character);
			isBoss = false;
			btnAttack->Visible = true;  // Make the attack button visible
			btnContinue->Visible = false;
			battleState = true;

			//character->incReputation() if player wins
		}
	}
	else if (roomCounter == 15) {
		//15 = Shop
	}
	else if (roomCounter == 16) {
		//16 = Chest
		chestOpen = false;

		int randomNum = rand() % 6;

		character->incStats(randomNum, 2);
		showStats();

		redReader->Text = redReader->Text + "\n\n" + File::ReadAllText("floor2chestopen.txt");

		character->incReputation();
	}
	else if (roomCounter == 17) {
		//17 = Encounter
		character->incReputation();
	}
	else if (roomCounter == 18) {
		//18 = Respite
	}
	else if (roomCounter == 19) {
		//19 = Battle
		// Check if the battle has already started
		if (currentBattle == nullptr) {
			// Initialize the Battle object (start the battle)
			character->floor = floor;
			isBoss = false;
			currentBattle = new Battle(isBoss, *character);
			btnAttack->Visible = true;  // Make the attack button visible
			btnContinue->Visible = false;
			battleState = true;
		}
	}
	else if (roomCounter == 25) {
		//25 = Chest
		chestOpen = false;

		int randomNum = rand() % 6;

		character->incStats(randomNum, 2);
		showStats();
	}
	else if (roomCounter == 26) {
		//26 = Battle
		// Check if the battle has already started
		if (currentBattle == nullptr) {
			// Initialize the Battle object (start the battle)
			character->floor = floor;
			isBoss = false;
			currentBattle = new Battle(isBoss, *character);
			btnAttack->Visible = true;  // Make the attack button visible
			btnContinue->Visible = false;
			battleState = true;

			//character->incReputation() if player wins
		}
	}
	else if (roomCounter == 27) {
		//27 = Shop
	}
	else if (roomCounter == 28) {
		//28 = Encounter
		redReader->Text = File::ReadAllText("lavaminionfree.txt");
		character->incReputation();
	}
	else if (roomCounter == 29) {
		//29 = Respite
	}

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

	btnContinue->Visible = true;

	if ((roomCounter % 10) == 2 || (roomCounter % 10) == 3 || (roomCounter % 10) == 4) {
		//2 = Library
		//3 = Battle
		//4 = Chest
		if (room->type == "Library") {
			character->incStats(3, 2);
			redReader->Text = gcnew String(character->getStatName(3).c_str()) + " has increased by 2.";
			showStats();
		}
		else if (room->type == "Battle") {
			redReader->Text = "You quietly sneak past the monster. Your presence goes unnoticed. You continue through the dungeon.";
		}
		else if (room->type == "Chest") {

		}
	}

	if (roomCounter == 5) {
		//5 = Encounter
		redReader->Text = "You kill the warrior. You have gained stat points.";
		int randomNum = rand() % 6;
		character->incStats(randomNum, 2);
	}
	else if (roomCounter == 6) {
		//6 = Battle
		redReader->Text = "You quietly sneak past the monster. Your presence goes unnoticed. You continue through the dungeon.";
	}
	else if (roomCounter == 7) {
		//increase stats temporarily by 1.5 multiplier
		character->incStats(0, 1);
	}
	else if (roomCounter == 8) {
		//8 = Library
		if (room->type == "Library") {
			character->incStats(3, 2);
			redReader->Text = gcnew String(character->getStatName(3).c_str()) + " has increased by 2.";
			showStats();
		}
	}
	else if (roomCounter == 9) {
		//9 = Shop
		srand(time(0));
		int randomNum = rand() % 6;
		character->incStats(randomNum, 2);
		redReader->Text = gcnew String(character->getStatName(randomNum).c_str()) + " has increased by 2.";
		showStats();
	}
	else if ((roomCounter % 10) == 0) {

	}
	else if (roomCounter == 15) {
		//15 = Shop
	}
	else if (roomCounter == 16) {
		//16 = Chest
	}
	else if (roomCounter == 17) {
		//17 = Encounter
	}
	else if (roomCounter == 18) {
		//18 = Respite
	}
	else if (roomCounter == 19) {
		//19 = Battle
		redReader->Text = "You quietly sneak past the monster. Your presence goes unnoticed. You continue through the dungeon.";
	}
	else if (roomCounter == 25) {
		//25 = Chest
	}
	else if (roomCounter == 26) {
		//26 = Battle
		redReader->Text = "You quietly sneak past the monster. Your presence goes unnoticed. You continue through the dungeon.";
	}
	else if (roomCounter == 27) {
		//27 = Shop
	}
	else if (roomCounter == 28) {
		//28 = Encounter
	}
	else if (roomCounter == 29) {
		//29 = Respite
	}

	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
}
private: System::Void btnChoice3_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 3

	btnContinue->Visible = true;

	if ((roomCounter % 10) == 2 || (roomCounter % 10) == 3 || (roomCounter % 10) == 4) {
		//2 = Library
		//3 = Battle
		//4 = Chest
		if (room->type == "Library") {
			character->incStats(1, 2);
			redReader->Text = gcnew String(character->getStatName(1).c_str()) + " has increased by 2.";
			showStats();
		}
		else if (room->type == "Battle") {

		}
		else if (room->type == "Chest") {

		}
	}
	else if (roomCounter == 5) {
		redReader->Text = File::ReadAllText("iceprisonfree.txt");
	}
	else if (roomCounter == 6) {

	}
	else if (roomCounter == 7) {

	}
	else if (roomCounter == 8) {
		//8 = Library
		character->incStats(1, 2);
		redReader->Text = gcnew String(character->getStatName(1).c_str()) + " has increased by 2.";
		showStats();
	}
	else if (roomCounter == 9) {
		redReader->Text = File::ReadAllText("shopkeeperbook.txt");
		character->incReputation();
	}
	else if ((roomCounter % 10) == 0) {

	}
	else if (roomCounter == 15) {
		//15 = Shop
	}
	else if (roomCounter == 16) {
		//16 = Chest
	}
	else if (roomCounter == 17) {
		//17 = Encounter
	}
	else if (roomCounter == 18) {
		//18 = Respite
	}
	else if (roomCounter == 19) {
		//19 = Battle
	}
	else if (roomCounter == 25) {
		//25 = Chest
	}
	else if (roomCounter == 26) {
		//26 = Battle
	}
	else if (roomCounter == 27) {
		//27 = Shop
	}
	else if (roomCounter == 28) {
		//28 = Encounter
	}
	else if (roomCounter == 29) {
		//29 = Respite
	}

	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
}
private: System::Void btnChoice4_Click(System::Object^ sender, System::EventArgs^ e) {
	//Choice 4

	btnContinue->Visible = true;

	if ((roomCounter % 10) == 2 || (roomCounter % 10) == 3 || (roomCounter % 10) == 4) {
		//2 = Library
		//3 = Battle
		//4 = Chest
		if (room->type == "Library") {
			character->incStats(2, 2);
			redReader->Text = gcnew String(character->getStatName(2).c_str()) + " has increased by 2.";
			showStats();
		}
		else if (room->type == "Battle") {

		}
		else if (room->type == "Chest") {

		}
	}
	else if (roomCounter == 5) {
		redReader->Text = File::ReadAllText("iceprisonfree.txt");
	}
	else if (roomCounter == 6) {

	}
	else if (roomCounter == 7) {

	}
	else if (roomCounter == 8) {
		// 8 = Library
		character->incStats(2, 2);
		redReader->Text = gcnew String(character->getStatName(2).c_str()) + " has increased by 2.";
		showStats();
	}
	else if (roomCounter == 9) {

	}
	else if ((roomCounter % 10) == 0) {

	}
	else if (roomCounter == 15) {
		//15 = Shop
	}
	else if (roomCounter == 16) {
		//16 = Chest
	}
	else if (roomCounter == 17) {
		//17 = Encounter
	}
	else if (roomCounter == 18) {
		//18 = Respite
	}
	else if (roomCounter == 19) {
		//19 = Battle
	}
	else if (roomCounter == 25) {
		//25 = Chest
	}
	else if (roomCounter == 26) {
		//26 = Battle
	}
	else if (roomCounter == 27) {
		//27 = Shop
	}
	else if (roomCounter == 28) {
		//28 = Encounter
	}
	else if (roomCounter == 29) {
		//29 = Respite
	}

	if (roomState) {
		roomState = false;
		btnChoiceInvisible();
		btnContinue->Visible = true;
	}
}
private: System::Void btnAnswer1_Click(System::Object^ sender, System::EventArgs^ e) {
	riddleAnswered();

	if ((gcnew String(riddles->getAnswerCorrect().at(riddleCounter).c_str()))->Equals(gcnew String(btnAnswer1->Text))) {
		correctRiddleAnswer();
		riddleCorrect++;
	}
	else {
		incorrectRiddleAnswer();
	}
	riddleCounter++;
}
private: System::Void btnAnswer2_Click(System::Object^ sender, System::EventArgs^ e) {
	riddleAnswered();

	if ((gcnew String(riddles->getAnswerCorrect().at(riddleCounter).c_str()))->Equals(gcnew String(btnAnswer2->Text))) {
		correctRiddleAnswer();
		riddleCorrect++;
	}
	else {
		incorrectRiddleAnswer();
	}
	riddleCounter++;
}
private: System::Void btnAnswer3_Click(System::Object^ sender, System::EventArgs^ e) {
	riddleAnswered();

	if ((gcnew String(riddles->getAnswerCorrect().at(riddleCounter).c_str()))->Equals(gcnew String(btnAnswer3->Text))) {
		correctRiddleAnswer();
		riddleCorrect++;
	}
	else {
		incorrectRiddleAnswer();
	}
	riddleCounter++;
}

private: void incorrectRiddleAnswer() {

	int randomNum = rand() % 6;

	// Decrease the character's stats based on the random number  
	character->decStats(randomNum, 1);

	// Display the incorrect answer message
	redReader->Text = "Incorrect! You have failed the riddle! " + gcnew String(character->getStatName(randomNum).c_str())
		+ " has decreased by 1.";

	// Update the stats display  
	showStats();

	flashScreenRed();

	if (room->type == "Chest") {
		chestOpen = false;
		redReader->Text = redReader->Text + " The chest will remain locked.";
	}
}

private: void correctRiddleAnswer() {

	int randomNum = rand() % 6;

	// Increase the character's stats based on the random number
	character->incStats(randomNum, 2);

	// Display the correct answer message
	redReader->Text = "Correct! You have answered the riddle! " + gcnew String(character->getStatName(randomNum).c_str())
		+ " has increased by 2.";

	// Update the stats display
	showStats();

	flashScreenGreen();

	if (room->type == "Chest") {
		chestOpen = true;
		redReader->Text = redReader->Text + " The chest is now unlocked.";
	}
}

private: void riddleAnswered() {
	tmrRiddle->Stop();
	progRiddle->Visible = false;
	btnAnswersInvisible();
	btnContinue->Visible = true;
}

private: System::Void btnAttack_Click(System::Object^ sender, System::EventArgs^ e) {
	// Perform the attack action if strikes are remaining and battle isn't over
	if (!currentBattle->isBattleFinished()) {
		// Perform the attack (this is handled inside the Battle class)
		currentBattle->attack(*character);  // Player attacks the enemy
		redReader->Text = redReader->Text + "\nYou attacked! Strikes remaining: "
			+ currentBattle->getStrikesRemaining().ToString() + "\nEnemy health remaining: "
			+ currentBattle->getEnemy()->getHealth();
	}
	else {
		// Check if the battle is finished (either strikes are 0 or the enemy is defeated)
		btnAttack->Visible = false;  // Hide the attack button after the battle is over
		redReader->Text = "Battle finished!";

		// If the enemy is defeated, move to the next room
		if (currentBattle->getEnemy()->isDefeated()) {
			redReader->Text = "The enemy has been defeated! Moving to the next room.";
			btnContinue->Visible = true;
		}
		else {
			redReader->Text = "You could not defeat the enemy.\n\nAll stats have been decreased by 2.";;
			
			for (int i = 0; i < 5; i++) {
				character->decStats(i, 2);
			}

			showStats();
			btnContinue->Visible = true;

			flashScreenRed();
		}

		// Clean up the current battle after it's finished
		delete currentBattle;
		currentBattle = nullptr;  // Reset the battle reference for the next round
	}
}
private: System::Void tmrRiddle_Tick(System::Object^ sender, System::EventArgs^ e) {
	progRiddle->Value -= 1;
	if (progRiddle->Value == 0) {
		tmrRiddle->Stop();
		progRiddle->Visible = false;
		btnAnswersInvisible();
		btnContinue->Visible = true;
		riddleCounter++;

		int randomNum = rand() % 6;
		character->decStats(randomNum, 1);

		redReader->Text = "The riddle keeper shouts \"Time's up! You have failed the riddle!\" "
			+ gcnew String(character->getStatName(randomNum).c_str()) + " has been decreased by 1.";

		flashScreenRed();
	}
}
private: System::Void pbProfile_Click(System::Object^ sender, System::EventArgs^ e) {
	Lore^ lore = gcnew Lore(this);
	lore->Visible = true;
	this->Visible = false;
}

private: void flashScreenRed() {
	this->BackColor = System::Drawing::Color::Red;

	// Wait a short time (but still process UI events)
	System::Windows::Forms::Application::DoEvents();
	System::Threading::Thread::Sleep(1000); // 1000 ms flash

	// Change background back to black
	this->BackColor = System::Drawing::Color::Black;
	System::Threading::Thread::Sleep(200);
}

private: void flashScreenGreen() {
	this->BackColor = System::Drawing::Color::Green;

	// Wait a short time (but still process UI events)
	System::Windows::Forms::Application::DoEvents();
	System::Threading::Thread::Sleep(1000); // 1000 ms flash

	// Change background back to black
	this->BackColor = System::Drawing::Color::Black;
	System::Threading::Thread::Sleep(200);
}
};
}
