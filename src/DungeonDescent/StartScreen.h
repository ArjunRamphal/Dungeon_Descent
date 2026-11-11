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

	private: System::Windows::Forms::PictureBox^ pbDev;

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
			this->pbDev = (gcnew System::Windows::Forms::PictureBox());
			this->panelMenu = (gcnew System::Windows::Forms::Panel());
			this->pbQuit = (gcnew System::Windows::Forms::PictureBox());
			this->pbLore = (gcnew System::Windows::Forms::PictureBox());
			this->pbStart = (gcnew System::Windows::Forms::PictureBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbTitle))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbDev))->BeginInit();
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
			this->pbDev->Click += gcnew System::EventHandler(this, &StartScreen::pbDev_Click);
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
			this->Controls->Add(this->pbDev);
			this->Controls->Add(this->pbTitle);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->Name = L"StartScreen";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Dungeon Descent";
			this->Shown += gcnew System::EventHandler(this, &StartScreen::StartScreen_Shown);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbTitle))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbDev))->EndInit();
			this->panelMenu->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbQuit))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbLore))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pbStart))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
private: System::Void pbStart_Click(System::Object^ sender, System::EventArgs^ e) {
	// Create a new instance of GameScreen and show it
	GameScreen^ gamescreen = gcnew GameScreen(this);
	gamescreen->Visible = true;
	gamescreen->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
	this->Visible = false;
}

private: System::Void pbLore_Click(System::Object^ sender, System::EventArgs^ e) {
	// Create a new instance of Lore and show it
	Lore^ lore = gcnew Lore(this);
	lore->Visible = true;
	this->Visible = false;
}

private: System::Void pbQuit_Click(System::Object^ sender, System::EventArgs^ e) {
	// Close the application
	this->Close();
}

// Set location and size of all components
private: System::Void StartScreen_Shown(System::Object^ sender, System::EventArgs^ e) {
	pbTitle->Location = System::Drawing::Point(67, 21);
	pbTitle->Size = System::Drawing::Size(1155, 206);
	pbDev->Location = System::Drawing::Point(0, 813);
	pbDev->Size = System::Drawing::Size(130, 130);
	panelMenu->Location = System::Drawing::Point(473, 384);
	panelMenu->Size = System::Drawing::Size(247, 407);
	pbStart->Location = System::Drawing::Point(5, 19);
	pbStart->Size = System::Drawing::Size(239, 93);
	pbLore->Location = System::Drawing::Point(5, 143);
	pbLore->Size = System::Drawing::Size(239, 93);
	pbQuit->Location = System::Drawing::Point(5, 266);
	pbQuit->Size = System::Drawing::Size(239, 93);
}
private: System::Void pbDev_Click(System::Object^ sender, System::EventArgs^ e) {
	// Create the SFML window

	this->Visible = false; // Hide the current form

	sf::RenderWindow window(sf::VideoMode({ 1536, 1024 }), "Dungeon Descent");

	//std::shared_ptr<sf::Texture> texture = "";
	auto texture = std::make_shared<sf::Texture>();
	std::string filename = "images/devBackground.png";

	if (!texture->loadFromFile(filename)) {
		std::cerr << "Failed to load " << filename << std::endl;
	}
	sf::Sprite sprite(*texture);  // Explicit initialization with texture

	sf::Font font("fonts/Arial.ttf"); // Load a font from file
	sf::Font fontArjun("fonts/MINECRAFT PE.ttf");
	sf::Font fontDarian("fonts/Nasalization Rg.otf");
	sf::Font fontLokadi("fonts/The Last Shuriken.ttf");
	sf::Font fontKeolin("fonts/Transformers Movie.ttf");
	sf::Font fontZaid("fonts/Blazed.ttf");
	sf::Font fontZaheera("fonts/Arabic Ramadhan.ttf");
	sf::Font fontNafees("fonts/Secret Thief.otf");
	sf::Font fontAsh("fonts/VIKING-N.ttf");
	sf::Font fontSikhulile("fonts/Calinastiya demo.ttf");
	
	sf::Text text(font); // a font is required to make a text object
	sf::Text textArjun(fontArjun); // a font is required to make a text object
	sf::Text textDarian(fontDarian); // a font is required to make a text object
	sf::Text textLokadi(fontLokadi); // a font is required to make a text object
	sf::Text textKeolin(fontKeolin); // a font is required to make a text object
	sf::Text textZaid(fontZaid); // a font is required to make a text object
	sf::Text textZaheera(fontZaheera); // a font is required to make a text object
	sf::Text textNafees(fontNafees); // a font is required to make a text object
	sf::Text textAsh(fontAsh); // a font is required to make a text object
	sf::Text textSikhulile(fontSikhulile); // a font is required to make a text object

	// set the string to display
	textArjun.setString("Arjun Ramphal");
	// set the character size
	textArjun.setCharacterSize(24);
	// set the color
	textArjun.setFillColor(sf::Color::White);
	// set the text style
	textArjun.setStyle(sf::Text::Bold);
	// set the text position
	textArjun.setPosition(sf::Vector2f(25.0f, 600.0f));

	// set the string to display
	textDarian.setString("Darian Robert");
	// set the character size
	textDarian.setCharacterSize(24);
	// set the color
	textDarian.setFillColor(sf::Color::White);
	// set the text style
	textDarian.setStyle(sf::Text::Bold);
	// set the text position
	textDarian.setPosition(sf::Vector2f(10.0f, 200.0f));

	// set the string to display
	textLokadi.setString("Lokadi Naicker");
	// set the character size
	textLokadi.setCharacterSize(24);
	// set the color
	textLokadi.setFillColor(sf::Color::White);
	// set the text style
	textLokadi.setStyle(sf::Text::Bold);
	// set the text position
	textLokadi.setPosition(sf::Vector2f(300.0f, 325.0f));

	// set the string to display
	textKeolin.setString("Keolin Naicker");
	// set the character size
	textKeolin.setCharacterSize(24);
	// set the color
	textKeolin.setFillColor(sf::Color::White);
	// set the text style
	textKeolin.setStyle(sf::Text::Bold);
	// set the text position
	textKeolin.setPosition(sf::Vector2f(100.0f, 350.0f));

	// set the string to display
	textZaid.setString("Zaid Kajee");
	// set the character size
	textZaid.setCharacterSize(24);
	// set the color
	textZaid.setFillColor(sf::Color::White);
	// set the text style
	textZaid.setStyle(sf::Text::Bold);
	// set the text position
	textZaid.setPosition(sf::Vector2f(625.0f, 300.0f));

	// set the string to display
	textZaheera.setString("Zaheera Ganie");
	// set the character size
	textZaheera.setCharacterSize(50);
	// set the color
	textZaheera.setFillColor(sf::Color::White);
	// set the text style
	textZaheera.setStyle(sf::Text::Bold);
	// set the text position
	textZaheera.setPosition(sf::Vector2f(825.0f, 350.0f));

	// set the string to display
	textNafees.setString("Nafees Abdulla");
	// set the character size
	textNafees.setCharacterSize(24);
	// set the color
	textNafees.setFillColor(sf::Color::White);
	// set the text style
	textNafees.setStyle(sf::Text::Bold);
	// set the text position
	textNafees.setPosition(sf::Vector2f(1300.0f, 700.0f));

	// set the string to display
	textAsh.setString("Ashutosh Maraj");
	// set the character size
	textAsh.setCharacterSize(24);
	// set the color
	textAsh.setFillColor(sf::Color::White);
	// set the text style
	textAsh.setStyle(sf::Text::Bold);
	// set the text position
	textAsh.setPosition(sf::Vector2f(950.0f, 450.0f));

	// set the string to display
	textSikhulile.setString("Sikhulile Dlamini");
	// set the character size
	textSikhulile.setCharacterSize(50);
	// set the color
	textSikhulile.setFillColor(sf::Color::White);
	// set the text style
	textSikhulile.setStyle(sf::Text::Bold);
	// set the text position
	textSikhulile.setPosition(sf::Vector2f(1225.0f, 200.0f));

	// set the string to display
	text.setString("Press any key to return to main menu");

	// set the character size
	text.setCharacterSize(50);

	// set the color
	text.setFillColor(sf::Color::White);

	// set the text style
	text.setStyle(sf::Text::Bold | sf::Text::Underlined);

	// set the text position
	text.setPosition(sf::Vector2f(300.0f, 950.0f));

	while (window.isOpen()) {
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>()) { // Close the window
				window.close();
				this->Visible = true; // Show the previous form
			}

			if (event->is<sf::Event::KeyPressed>()) { // Key pressed
				window.close();
				this->Visible = true; // Show the previous form
			}
		}

		window.clear(); // clear the window
		window.draw(sprite); // draw the sprite
		window.draw(text); // draw the text
		window.draw(textArjun); // draw the text
		window.draw(textDarian); // draw the text
		window.draw(textLokadi); // draw the text
		window.draw(textKeolin); // draw the text
		window.draw(textZaid); // draw the text
		window.draw(textZaheera); // draw the text
		window.draw(textNafees); // draw the text
		window.draw(textAsh); // draw the text
		window.draw(textSikhulile); // draw the text
		window.display(); // display the window contents
	}
}
};
}
