#pragma once
#include "stdafx.h"

// Ȯ�� OVERLAPPED ����ü. 
// WSAOVERLAPPED ����ü�� ���� �ʿ��� ������ �߰��� �����Ѵ�.
class EXP_OVER
{
public:
	EXP_OVER();
	EXP_OVER(char* packet);
	~EXP_OVER() = default;

public:
	WSAOVERLAPPED m_overlapped;
	WSABUF m_wsaBuf;
	char m_sendMsg[BUFSIZE];
	COMP_TYPE m_compType;
};

