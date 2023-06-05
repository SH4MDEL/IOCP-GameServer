#pragma once
#include "stdafx.h"
#include "singleton.h"
#include "server.h"

class GameServer;
class Database : public Singleton<Database>
{
public:
	Database();
	~Database();

	void DatabaseThread();

	bool Login(UINT uid, const char* id, const char* password);

private:
	void ShowError(SQLHANDLE handle, SQLSMALLINT type, RETCODE retcode);

private:
	const wstring	DatabaseName = TEXT("GameServerExample");

	// DB �ڵ鿡 ���� ���� ���� (Locking �ʼ�)
	SQLHENV		henv;
	SQLHDBC		hdbc;
	SQLHSTMT	hstmt;
	mutex		handleLock;
	SQLRETURN	retcode;
	SQLWCHAR	szName[NAMESIZE];
};

