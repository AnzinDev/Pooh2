#include <stdio.h>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <json.h>
#include <http_listener.h>
#pragma comment(lib, "cpprest_2_10")

using namespace sf;

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;


enum Commands
{
	FLIGHT = 1,
	LANDING,
	WAITING,
};

Commands cmd = Commands::WAITING;

class Pooh
{
	double mass;
	RectangleShape pooh;

public:

	Pooh(double m)
	{
		mass = m;
		pooh.setSize(Vector2f(25, 35));
		pooh.setPosition(Vector2f(630, 720));
		pooh.setFillColor(Color(73, 42, 18));
	}

	void Moving(double height)
	{
		pooh.move(Vector2f(0, -height * 10));
	}

	RectangleShape& ToDraw()
	{
		return pooh;
	}

	void Rot()
	{
		pooh.rotate(20);
	}

	double GetPoohMass()
	{
		return mass;
	}

	bool Eating(double& honeyMass)
	{
		if (honeyMass >= 0.2)
		{
			mass += 0.2;
			honeyMass -= 0.2;
			return 1;
		}
		else
		{
			mass += honeyMass;
			honeyMass = 0;
			return 0; //мёд кончился 
		}
	}
};

class World
{
	RectangleShape tree;
	RectangleShape land;
	RectangleShape hole;
	double honeyMass;

public:
	World()
	{
		tree.setSize(Vector2f(35, 720));
		tree.setPosition(675, 30);
		tree.setFillColor(Color(109, 77, 64));

		land.setSize(Vector2f(800, 50));
		land.setPosition(0, 750);
		land.setFillColor(Color(73, 159, 65));

		srand(time(NULL));
		hole.setSize(Vector2f(30, 30));
		hole.setPosition(670, (60 + rand() % 290));
		hole.setPosition(670, 250);
		hole.setFillColor(Color(50, 30, 21));

		honeyMass = (5 + rand() % 25) / 10;
	}

	std::vector<RectangleShape> ToDraw()
	{
		std::vector<RectangleShape> td;
		td.push_back(tree);
		td.push_back(land);
		td.push_back(hole);

		return td;
	}

	double GetHoleHight()
	{
		return (750 - hole.getPosition().y) * 0.1;
	}

	double GetHoneyMass()
	{
		return honeyMass;
	}

	bool Bees()
	{
		srand(time(NULL));
		return (rand() % 100 < 33) ? 1 : 0;
	}
};

class Engine
{
	double thrust;
	double maxThrust;
	double minThrust;

public:

	Engine(double min, double max, double StartThrust = 0)
	{
		thrust = StartThrust;
		minThrust = min;
		maxThrust = max;
	}

	void SetThrust(double percent)
	{
		thrust = percent * (maxThrust / 100.);
	}

	double GetThrust()
	{
		return thrust;
	}

	double GetMaxThrust()
	{
		return maxThrust;
	}

	double GetMinThrust()
	{
		return minThrust;
	}
};

class PID
{
	double pFactor;
	double iFactor;
	double dFactor;
	double dt;

	double err = 0, prevErr = 0;

	double p = 0, i = 0, d = 0;

	double output = 0;

public:

	PID(double p, double i, double d, double t)
	{
		pFactor = p;
		iFactor = i;
		dFactor = d;
		dt = t;
	}

	double Correction(double setValue, double currValue)
	{
		p = err = setValue - currValue;
		i += err * dt;
		d = (err - prevErr) / (1000 * dt);
		prevErr = err;

		output = pFactor * p + iFactor * i + dFactor * d;

		if (output > 100)
		{
			return 100;
		}
		else if (output < -100)
		{
			return -100;
		}
		else
		{
			return output;
		}
	}
};

class CS
{
	Engine* engine;
	PID* pid;
	Pooh* pooh;
	World* world;
	double height = 0, mass, vel = 0, acc = 0;

	double dh = 0;
	double dv = 0;
	double dt;

	const double g = 9.810665;
	double setValue;// todo
	double simTime = 0;
	double honeyMass;
	//test
	bool isEating = 0;
	//~test
public:

	CS(Engine* _e, PID* _pid, Pooh* _pooh, World* _world, double _dt)
	{
		engine = _e;
		pid = _pid;
		pooh = _pooh;
		world = _world;

		dt = _dt;
	}

	void SetValue(double value)
	{
		setValue = value;
	}

	void SetHoneyMass(double hm)
	{
		honeyMass = hm;
	}

	void Calculate()
	{
		mass = pooh->GetPoohMass();

		acc = engine->GetThrust() / mass;
		vel += acc * dt;
		height += vel * dt;

		engine->SetThrust(pid->Correction(setValue, height));

		simTime += dt;
		//test console logging
		std::system("cls");
		std::cout << "height = " << height << std::endl;
		std::cout << "vel = " << vel << std::endl;
		std::cout << "acc = " << acc << std::endl;
		std::cout << "Force% = " << engine->GetThrust() << std::endl;
		//~test console logging

	}

	double GetHeight()
	{
		return vel * dt;
	}

	double GetVelocity()
	{
		return vel;
	}

	double GetAcc()
	{
		return acc;
	}
};

class Drawing
{

	std::vector<RectangleShape> figures;
	std::vector<Text> text;
	RectangleShape pooh;

public:

	void AddPoohToDraw(const RectangleShape& obj)
	{
		pooh = obj;
	}

	void AddToDraw(const RectangleShape& obj)
	{
		figures.push_back(obj);
	}

	void AddToDraw(const std::vector<RectangleShape>& vec)
	{
		for (int i = 0; i < vec.size(); i++)
		{
			figures.push_back(vec[i]);
		}
	}

	void AddTextToDraw(const std::vector<Text>& t)
	{
		for (int i = 0; i < t.size(); i++)
		{
			text.push_back(t[i]);
		}
	}

	void DrawAll(RenderWindow& window)
	{
		window.draw(pooh);

		for (int i = 0; i < figures.size(); i++)
		{
			window.draw(figures[i]);
		}

		for (int i = 0; i < text.size(); i++)
		{
			window.draw(text[i]);
		}

		text.clear();
	}

};

class Messages
{
	Text AltText;
	Text VelocityText;
	Text MassText;

	RectangleShape TextBackground;

public:
	Messages(Font& font)
	{
		TextBackground.setPosition(Vector2f(50, 50));
		TextBackground.setFillColor(Color::White);
		TextBackground.setOutlineThickness(4);
		TextBackground.setOutlineColor(Color::Black);
		TextBackground.setSize(Vector2f(150, 200));

		AltText.setFont(font);
		AltText.setCharacterSize(32);
		AltText.setFillColor(Color::Black);
		AltText.setPosition(56, 56);
		AltText.setStyle(Text::Bold);

		VelocityText.setCharacterSize(32);
		VelocityText.setFont(font);
		VelocityText.setFillColor(Color::Black);
		VelocityText.setPosition(56, 90);
		VelocityText.setStyle(Text::Bold);

		MassText.setFont(font);
		MassText.setCharacterSize(32);
		MassText.setFillColor(Color::Black);
		MassText.setPosition(56, 124);
		MassText.setStyle(Text::Bold);

	}

	void SetAltText(double alt)
	{
		char str[10];
		sprintf_s(str, "H %3.2f", alt);
		AltText.setString(str);
	}

	void SetVelocityText(double vel)
	{
		char str[10];
		sprintf_s(str, "V %3.2f", vel);
		VelocityText.setString(str);
	}

	void SetMassText(double mass)
	{
		char str[10];
		sprintf_s(str, "M %3.2f", mass);
		MassText.setString(str);
	}

	void SetAccelerationText(double acc)
	{
		char str[10];
		sprintf_s(str, "A %3.2f", acc);
		MassText.setString(str);
	}

	RectangleShape& ToDraw()
	{
		return TextBackground;
	}

	std::vector<Text> ToDrawText()
	{
		std::vector<Text> td;
		td.push_back(AltText);
		td.push_back(VelocityText);
		td.push_back(MassText);

		return td;
	}
};

int main()
{
	RenderWindow window(VideoMode(800, 800), "Pooh", Style::Close);

	Drawing drawing;

	Font font;
	font.loadFromFile("lucon.ttf");

	Messages message(font);

	Pooh pooh(15);

	World world;

	Engine e(-500, 500);

	PID pid(0.05, 0.0002, 0.0012, 0.016);

	CS cs(&e, &pid, &pooh, &world, 0.016);

	http_listener server(L"https://localhost/");

	cmd = Commands::WAITING;

	drawing.AddToDraw(world.ToDraw());

	double overallTime = 0;


	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed) window.close();
		}

		//overallTime += 0.016;
		//std::cout << "OVERALLTIME = " << overallTime << std::endl;

		switch (cmd)
		{
		case FLIGHT:
		{
			cs.SetValue(world.GetHoleHight());
			cs.SetHoneyMass(world.GetHoneyMass());

			cs.Calculate();
			pooh.Moving(cs.GetHeight());

			message.SetAltText(cs.GetHeight());
			message.SetVelocityText(cs.GetVelocity());
			message.SetAccelerationText(cs.GetAcc());
			break;
		}
		case LANDING:
		{
			cs.SetValue(0);
			cs.Calculate();
			pooh.Moving(cs.GetHeight());

			message.SetAltText(cs.GetHeight());
			message.SetVelocityText(cs.GetVelocity());
			message.SetAccelerationText(cs.GetAcc());
			break;
		}
		case WAITING:
		{
			std::system("cls");
			std::cout << "WAITING" << std::endl;
			message.SetAltText(cs.GetHeight());
			message.SetVelocityText(cs.GetVelocity());
			message.SetAccelerationText(cs.GetAcc());
			break;
		}
		default:
			std::cout << "\nINVALID COMMAND" << std::endl;
			break;
		}

		window.clear(Color(120, 219, 226, 255));
		//drawing.AddToDraw(message.ToDraw());
		drawing.AddTextToDraw(message.ToDrawText());
		drawing.AddPoohToDraw(pooh.ToDraw());
		drawing.DrawAll(window);

		window.display();
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	return 0;
}

