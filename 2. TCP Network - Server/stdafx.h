#pragma once
// ���� ���� ��������Դϴ�.
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <WS2tcpip.h>
#include "protocol.h"

// C, C++ ���� ��������Դϴ�.
#include <iostream>
#include <vector>
#include <list>
#include <queue>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <mutex>
using namespace std;

extern SOCKET		g_listenSock;