#pragma once

// Windows ��� ����
#include <windows.h>

// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ ��Ÿ�� ��� �����Դϴ�.
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
using namespace std;
#define endl "\n";

// SFML ��� �����Դϴ�.
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

// ���� ���� ��� �����Դϴ�.
#include "..\11. Database - Server\protocol.h"

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

class GameFramework;
extern GameFramework				g_gameFramework;
extern shared_ptr<sf::RenderWindow>	g_window;
extern sf::TcpSocket				g_socket;
extern INT							g_clientID;
extern sf::Font						g_font;
extern INT							g_leftX;
extern INT							g_topY;