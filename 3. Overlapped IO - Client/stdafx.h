#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����
#include <windows.h>
#include <atlimage.h>
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
// ���� ���� ��� �����Դϴ�.
#include "../3. Overlapped IO - Server/protocol.h"
#include <winsock2.h> // ����2 ���� ���
#include <ws2tcpip.h> // ����2 Ȯ�� ���
#pragma comment(lib, "ws2_32.lib") // ws2_32.lib ��ũ

#define SHOW_CAPTIONFPS

#define MAX_TITLE 64
#define TITLESTRING TEXT("1. Game Client")

#define MAX_FPS 1.0 / 60.0

extern string g_serverIP;
extern SOCKET g_socket;
extern INT g_playerID;

namespace Move
{
	extern int dx[];
	extern int dy[];
}

