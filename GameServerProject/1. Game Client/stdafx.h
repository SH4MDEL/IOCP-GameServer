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
using namespace std;

#define SHOW_CAPTIONFPS

#define MAX_TITLE 64
#define TITLESTRING TEXT("1. Game Client")

#define MAX_FPS 1.0 / 60.0

namespace Move
{
	extern int dx[];
	extern int dy[];
}