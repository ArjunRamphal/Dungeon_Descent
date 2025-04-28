#pragma once
#include "GameScreen.h"
#include "Lore.h"
namespace DungeonDescent {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for StartScreen
	/// </summary>
	public ref class StartScreen : public System::Windows::Forms::Form
	{
	public:
		StartScreen(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~StartScreen()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::PictureBox^ pbTitle;
	private: System::Windows::Forms::PictureBox^ pbSettings;
	private: System::Windows::Forms::PictureBox^ pbDev;
	private: System::Windows::Forms::PictureBox^ pbFAQ;
	private: System::Windows::Forms::Panel^ panelMenu;
	private: System::Windows::Forms::PictureBox^ pbStart;
	private: System::Windows::Forms::PictureBox^ pbLore;
	private: System::Windows::Forms::PictureBox^ pbQuit;







	protected:

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
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(StartScreen::typeid));
			this->pbTitle = (gcnew System::Windows::Forms::PictureBox());
			this->pbSettings = (gcnew System::Windows::Forms::PictureBox());
			this->pbDev = (gcnew System::Windows::Forms::PictureBox());
			this->pbFAQ = (gcnew System::Windows::Forms::PictureBox());
			this->panelMenu = (gcnew System::Windows::Forms::Panel());
			this->pbQuit = (gcnew System::Windows::Forms::PictureBox());
			this->pbLore = (gcnew System::Windows::Forms::PictureBox());
			this->pbStart = (gcnew System::Windows::Forms::PictureBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbTitle))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbSettings))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbDev))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbFAQ))->BeginInit();
			this->panelMenu->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbQuit))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbLore))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbStart))->BeginInit();
			this->SuspendLayout();
			// 
			// pbTitle
			// 
			this->pbTitle->BackColor = System::Drawing::Color::Transparent;
			this->pbTitle->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbTitle.Image")));
			this->pbTitle->Location = System::Drawing::Point(67, 21);
			this->pbTitle->Name = L"pbTitle";
			this->pbTitle->Size = System::Drawing::Size(1155, 206);
			this->pbTitle->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbTitle->TabIndex = 0;
			this->pbTitle->TabStop = false;
			// 
			// pbSettings
			// 
			this->pbSettings->BackColor = System::Drawing::Color::Transparent;
			this->pbSettings->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbSettings.Image")));
			this->pbSettings->Location = System::Drawing::Point(1148, -2);
			this->pbSettings->Name = L"pbSettings";
			this->pbSettings->Size = System::Drawing::Size(130, 130);
			this->pbSettings->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbSettings->TabIndex = 1;
			this->pbSettings->TabStop = false;
			// 
			// pbDev
			// 
			this->pbDev->BackColor = System::Drawing::Color::Transparent;
			this->pbDev->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbDev.Image")));
			this->pbDev->Location = System::Drawing::Point(0, 813);
			this->pbDev->Name = L"pbDev";
			this->pbDev->Size = System::Drawing::Size(130, 130);
			this->pbDev->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbDev->TabIndex = 2;
			this->pbDev->TabStop = false;
			// 
			// pbFAQ
			// 
			this->pbFAQ->BackColor = System::Drawing::Color::Transparent;
			this->pbFAQ->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbFAQ.Image")));
			this->pbFAQ->Location = System::Drawing::Point(1148, 813);
			this->pbFAQ->Name = L"pbFAQ";
			this->pbFAQ->Size = System::Drawing::Size(130, 130);
			this->pbFAQ->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbFAQ->TabIndex = 3;
			this->pbFAQ->TabStop = false;
			// 
			// panelMenu
			// 
			this->panelMenu->BackColor = System::Drawing::Color::Transparent;
			this->panelMenu->Controls->Add(this->pbQuit);
			this->panelMenu->Controls->Add(this->pbLore);
			this->panelMenu->Controls->Add(this->pbStart);
			this->panelMenu->Location = System::Drawing::Point(473, 384);
			this->panelMenu->Name = L"panelMenu";
			this->panelMenu->Size = System::Drawing::Size(247, 407);
			this->panelMenu->TabIndex = 4;
			// 
			// pbQuit
			// 
			this->pbQuit->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbQuit.Image")));
			this->pbQuit->Location = System::Drawing::Point(5, 266);
			this->pbQuit->Name = L"pbQuit";
			this->pbQuit->Size = System::Drawing::Size(239, 93);
			this->pbQuit->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbQuit->TabIndex = 2;
			this->pbQuit->TabStop = false;
			this->pbQuit->Click += gcnew System::EventHandler(this, &StartScreen::pbQuit_Click);
			// 
			// pbLore
			// 
			this->pbLore->BackColor = System::Drawing::Color::Transparent;
			this->pbLore->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbLore.Image")));
			this->pbLore->Location = System::Drawing::Point(5, 143);
			this->pbLore->Name = L"pbLore";
			this->pbLore->Size = System::Drawing::Size(239, 93);
			this->pbLore->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbLore->TabIndex = 1;
			this->pbLore->TabStop = false;
			this->pbLore->Click += gcnew System::EventHandler(this, &StartScreen::pbLore_Click);
			// 
			// pbStart
			// 
			this->pbStart->BackColor = System::Drawing::Color::Transparent;
			this->pbStart->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pbStart.Image")));
			this->pbStart->Location = System::Drawing::Point(5, 19);
			this->pbStart->Name = L"pbStart";
			this->pbStart->Size = System::Drawing::Size(239, 93);
			this->pbStart->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->pbStart->TabIndex = 0;
			this->pbStart->TabStop = false;
			this->pbStart->Click += gcnew System::EventHandler(this, &StartScreen::pbStart_Click);
			// 
			// StartScreen
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(9, 20);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackgroundImage = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"$this.BackgroundImage")));
			this->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Stretch;
			this->ClientSize = System::Drawing::Size(1278, 944);
			this->Controls->Add(this->panelMenu);
			this->Controls->Add(this->pbFAQ);
			this->Controls->Add(this->pbDev);
			this->Controls->Add(this->pbSettings);
			this->Controls->Add(this->pbTitle);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->Name = L"StartScreen";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"StartScreen";
			this->Shown += gcnew System::EventHandler(this, &StartScreen::StartScreen_Shown);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbTitle))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbSettings))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbDev))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbFAQ))->EndInit();
			this->panelMenu->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbQuit))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbLore))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbStart))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
private: System::Void pbStart_Click(System::Object^ sender, System::EventArgs^ e) {
	GameScreen^ gamescreen = gcnew GameScreen(this);
	gamescreen->Visible = true;
	gamescreen->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
	this->Visible = false;
}

private: System::Void pbLore_Click(System::Object^ sender, System::EventArgs^ e) {
	Lore^ lore = gcnew Lore(this);
	lore->Visible = true;
	this->Visible = false;
}

private: System::Void pbQuit_Click(System::Object^ sender, System::EventArgs^ e) {
	this->Close();
}

// Set location and size of all components
private: System::Void StartScreen_Shown(System::Object^ sender, System::EventArgs^ e) {
	pbTitle->Location = System::Drawing::Point(67, 21);
	pbTitle->Size = System::Drawing::Size(1155, 206);
	pbSettings->Location = System::Drawing::Point(1148, -2);
	pbSettings->Size = System::Drawing::Size(130, 130);
	pbDev->Location = System::Drawing::Point(0, 813);
	pbDev->Size = System::Drawing::Size(130, 130);
	pbFAQ->Location = System::Drawing::Point(1148, 813);
	pbFAQ->Size = System::Drawing::Size(130, 130);
	panelMenu->Location = System::Drawing::Point(473, 384);
	panelMenu->Size = System::Drawing::Size(247, 407);
	pbStart->Location = System::Drawing::Point(5, 19);
	pbStart->Size = System::Drawing::Size(239, 93);
	pbLore->Location = System::Drawing::Point(5, 143);
	pbLore->Size = System::Drawing::Size(239, 93);
	pbQuit->Location = System::Drawing::Point(5, 266);
	pbQuit->Size = System::Drawing::Size(239, 93);
}
};
}
