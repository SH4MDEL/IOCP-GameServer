#include "main.h"

#include "mainScene.h"

void InitInstance();

int main()
{
	InitInstance();

	g_window = make_shared<sf::RenderWindow>(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "DataBase");
	
	while (g_window->isOpen()) {
        sf::Event event;
		while (g_window->pollEvent(event))
		{
			if (event.type == sf::Event::Closed) {
				g_window->close();
			}
			if (event.type == sf::Event::TextEntered ||
                event.type == sf::Event::KeyPressed ||
                event.type == sf::Event::KeyReleased) {
				g_gameFramework.OnProcessingKeyboardMessage(event);
			}
            if (event.type == sf::Event::MouseButtonPressed ||
                event.type == sf::Event::MouseButtonReleased ||
                event.type == sf::Event::MouseMoved) {
                g_gameFramework.OnProcessingMouseMessage(event, g_window);
            }
		}
		g_window->clear();
		g_gameFramework.FrameAdvance();
		g_window->display();
	}
}

void InitInstance()
{
    if (!g_font.loadFromFile("..\\Resource\\cour.ttf")) {
        cout << "Font Loading Error!\n";
        exit(-1);
    }
	g_gameFramework.OnCreate();
}

void Send(void* packetBuf)
{
    unsigned char* packet = reinterpret_cast<unsigned char*>(packetBuf);
    size_t sent = 0;
    g_socket.send(packetBuf, packet[0], sent);
}