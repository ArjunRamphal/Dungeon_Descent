#pragma once
#include <iostream>
#include <fstream>
#include <string>

namespace DungeonDescent {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::IO;

	/// <summary>
	/// Summary for Lore
	/// </summary>
	public ref class Lore : public System::Windows::Forms::Form
	{
	public:

		Form^ obj;

		Lore(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

		Lore(Form^ obj1)
		{
			obj = obj1;
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Lore()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::GroupBox^ groupBox1;
	private: System::Windows::Forms::PictureBox^ pbWarrior;
	private: System::Windows::Forms::PictureBox^ pbRanger;
	private: System::Windows::Forms::PictureBox^ pbMage;
	private: System::Windows::Forms::PictureBox^ pictureBox1;
	private: System::Windows::Forms::RichTextBox^ redLore;

	private: System::Windows::Forms::PictureBox^ pbCharacterName;
	private: System::Windows::Forms::PictureBox^ pbBack;




	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(Lore::typeid));
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->pbMage = (gcnew System::Windows::Forms::PictureBox());
			this->pbRanger = (gcnew System::Windows::Forms::PictureBox());
			this->pbWarrior = (gcnew System::Windows::Forms::PictureBox());
			this->redLore = (gcnew System::Windows::Forms::RichTextBox());
			this->pbCharacterName = (gcnew System::Windows::Forms::PictureBox());
			this->pbBack = (gcnew System::Windows::Forms::PictureBox());
			this->groupBox1->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbMage))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbRanger))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbWarrior))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbCharacterName))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbBack))->BeginInit();
			this->SuspendLayout();
			// 
			// groupBox1
			// 
			this->groupBox1->BackColor = System::Drawing::Color::Gold;
			this->groupBox1->Controls->Add(this->pictureBox1);
			this->groupBox1->Controls->Add(this->pbMage);
			this->groupBox1->Controls->Add(this->pbRanger);
			this->groupBox1->Controls->Add(this->pbWarrior);
			this->groupBox1->Location = System::Drawing::Point(12, 12);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(166, 720);
			this->groupBox1->TabIndex = 0;
			this->groupBox1->TabStop = false;
			// 
			// pictureBox1
			// 
			this->pictureBox1->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pictureBox1.Image")));
			this->pictureBox1->Location = System::Drawing::Point(6, 566);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(154, 148);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pictureBox1->TabIndex = 3;
			this->pictureBox1->TabStop = false;
			this->pictureBox1->Click += gcnew System::EventHandler(this, &Lore::pictureBox1_Click);
			// 
			// pbMage
			// 
			this->pbMage->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbMage.Image")));
			this->pbMage->Location = System::Drawing::Point(6, 384);
			this->pbMage->Name = L"pbMage";
			this->pbMage->Size = System::Drawing::Size(154, 148);
			this->pbMage->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbMage->TabIndex = 2;
			this->pbMage->TabStop = false;
			this->pbMage->Click += gcnew System::EventHandler(this, &Lore::pbMage_Click);
			// 
			// pbRanger
			// 
			this->pbRanger->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbRanger.Image")));
			this->pbRanger->Location = System::Drawing::Point(6, 203);
			this->pbRanger->Name = L"pbRanger";
			this->pbRanger->Size = System::Drawing::Size(154, 148);
			this->pbRanger->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbRanger->TabIndex = 1;
			this->pbRanger->TabStop = false;
			this->pbRanger->Click += gcnew System::EventHandler(this, &Lore::pbRanger_Click);
			// 
			// pbWarrior
			// 
			this->pbWarrior->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbWarrior.Image")));
			this->pbWarrior->Location = System::Drawing::Point(6, 15);
			this->pbWarrior->Name = L"pbWarrior";
			this->pbWarrior->Size = System::Drawing::Size(154, 148);
			this->pbWarrior->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbWarrior->TabIndex = 0;
			this->pbWarrior->TabStop = false;
			this->pbWarrior->Click += gcnew System::EventHandler(this, &Lore::pbWarrior_Click);
			// 
			// redLore
			// 
			this->redLore->BackColor = System::Drawing::Color::LightYellow;
			this->redLore->Font = (gcnew System::Drawing::Font(L"Engravers MT", 11, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->redLore->Location = System::Drawing::Point(184, 174);
			this->redLore->Name = L"redLore";
			this->redLore->Size = System::Drawing::Size(792, 566);
			this->redLore->TabIndex = 1;
			this->redLore->Text = L"";
			this->redLore->ZoomFactor = 2;
			// 
			// pbCharacterName
			// 
			this->pbCharacterName->BackColor = System::Drawing::Color::Transparent;
			this->pbCharacterName->Location = System::Drawing::Point(184, 12);
			this->pbCharacterName->Name = L"pbCharacterName";
			this->pbCharacterName->Size = System::Drawing::Size(693, 163);
			this->pbCharacterName->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbCharacterName->TabIndex = 2;
			this->pbCharacterName->TabStop = false;
			// 
			// pbBack
			// 
			this->pbBack->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbBack.Image")));
			this->pbBack->Location = System::Drawing::Point(883, 12);
			this->pbBack->Name = L"pbBack";
			this->pbBack->Size = System::Drawing::Size(83, 78);
			this->pbBack->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbBack->TabIndex = 3;
			this->pbBack->TabStop = false;
			this->pbBack->Click += gcnew System::EventHandler(this, &Lore::pbBack_Click);
			// 
			// Lore
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(9, 20);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::Color::Black;
			this->ClientSize = System::Drawing::Size(978, 744);
			this->Controls->Add(this->pbBack);
			this->Controls->Add(this->pbCharacterName);
			this->Controls->Add(this->redLore);
			this->Controls->Add(this->groupBox1);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->Name = L"Lore";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Lore";
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &Lore::Lore_FormClosed);
			this->Shown += gcnew System::EventHandler(this, &Lore::Lore_Shown);
			this->groupBox1->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbMage))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbRanger))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbWarrior))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbCharacterName))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbBack))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
private: System::Void pbWarrior_Click(System::Object^ sender, System::EventArgs^ e) {
	redLore->Text = File::ReadAllText("WarriorIntro.txt");
	pbCharacterName->Image = Image::FromFile("Warrior_Name_Label.png");
}
private: System::Void pbRanger_Click(System::Object^ sender, System::EventArgs^ e) {
	redLore->Text = File::ReadAllText("RangerIntro.txt");
	pbCharacterName->Image = Image::FromFile("Ranger_Name_Label.png");
}
private: System::Void pbMage_Click(System::Object^ sender, System::EventArgs^ e) {
	redLore->Text = File::ReadAllText("MageIntro.txt");
	pbCharacterName->Image = Image::FromFile("Mage_Name_Label.png");
}
private: System::Void pictureBox1_Click(System::Object^ sender, System::EventArgs^ e) {
	redLore->Text = File::ReadAllText("RogueIntro.txt");
	pbCharacterName->Image = Image::FromFile("Rogue_Name_Label.png");
}
private: System::Void pbBack_Click(System::Object^ sender, System::EventArgs^ e) {
	this->Visible = false;
	obj->Visible = true;
}
private: System::Void Lore_Shown(System::Object^ sender, System::EventArgs^ e) {
	this->Size = System::Drawing::Size(1000, 800);
	pbBack->Location = System::Drawing::Point(883, 12);
	pbBack->Size = System::Drawing::Size(83, 78);
	pbCharacterName->Location = System::Drawing::Point(184, 12);
	pbCharacterName->Size = System::Drawing::Size(693, 163);
	pbWarrior->Location = System::Drawing::Point(6, 15);
	pbWarrior->Size = System::Drawing::Size(154, 148);
	pbRanger->Location = System::Drawing::Point(6, 203);
	pbRanger->Size = System::Drawing::Size(154, 148);
	pbMage->Location = System::Drawing::Point(6, 384);
	pbMage->Size = System::Drawing::Size(154, 148);
	pictureBox1->Location = System::Drawing::Point(6, 566);
	pictureBox1->Size = System::Drawing::Size(154, 148);
	groupBox1->Location = System::Drawing::Point(12, 12);
	groupBox1->Size = System::Drawing::Size(166, 720);
	redLore->Location = System::Drawing::Point(184, 174);
	redLore->Size = System::Drawing::Size(792, 566);
	redLore->Font = (gcnew System::Drawing::Font(L"Engravers MT", 11, System::Drawing::FontStyle::Bold,
		System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
}
private: System::Void Lore_FormClosed(System::Object^ sender, System::Windows::Forms::FormClosedEventArgs^ e) {
	this->Visible = false;
	obj->Visible = true;
}
};
}
