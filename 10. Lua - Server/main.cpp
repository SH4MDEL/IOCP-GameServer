#include "main.h"

int main()
{
	g_gameServer.InitializeNPC();

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	g_serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(g_serverSocket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_serverSocket, SOMAXCONN);

	INT addr_size = sizeof(SOCKADDR_IN);

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_serverSocket), g_iocp, 9999, 0);

	g_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	g_expOverlapped.m_compType = COMP_TYPE::OP_ACCEPT;
	AcceptEx(g_serverSocket, g_clientSocket, g_expOverlapped.m_sendMsg, 0, 
		addr_size + 16, addr_size + 16, 0, &g_expOverlapped.m_overlapped);

	thread timerThread{ TimerThread, g_iocp };

	vector<thread> workerThreads;
	auto numThreads = thread::hardware_concurrency();
	for (UINT i = 0; i < numThreads; ++i) {
		workerThreads.emplace_back(WorkerThread, g_iocp);
	}
	for (auto& thread : workerThreads) {
		thread.join();
	}
	timerThread.join();

	closesocket(g_serverSocket);
	WSACleanup();
}

void WorkerThread(HANDLE hiocp)
{
	while (true) {
		DWORD received;
		ULONG_PTR key;
		WSAOVERLAPPED* overlapped = nullptr;

		BOOL ret = GetQueuedCompletionStatus(hiocp, &received, &key, &overlapped, INFINITE);
		EXP_OVER* expOverlapped = reinterpret_cast<EXP_OVER*>(overlapped);

		if (!ret) {
			if (expOverlapped->m_compType == COMP_TYPE::OP_ACCEPT) {
				cout << "OP_ACCEPT Error\n";
				exit(-1);
			}
			else {
				cout << "GetQueuedCompletionStatus Error on client[" << key << "]\n";
				// ���� ���� ��Ŷ ����

				g_gameServer.ExitClient(key);
				if (expOverlapped->m_compType == OP_SEND) delete expOverlapped;
				continue;
			}
		}

		if ((received == 0) && ((expOverlapped->m_compType == OP_RECV) || (expOverlapped->m_compType == OP_SEND))) {
			// ���� ���� ��Ŷ ����

			g_gameServer.ExitClient(key);
			if (expOverlapped->m_compType == OP_SEND) delete expOverlapped;
			continue;
		}

		switch (expOverlapped->m_compType)
		{
		case OP_ACCEPT:
		{
			UINT clientID = g_gameServer.RegistClient(g_clientSocket);
			if (clientID != -1) {
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_clientSocket),
					hiocp, clientID, 0);
				g_gameServer.GetClient(clientID)->DoRecv();
#ifdef NETWORK_DEBUG
				cout << "�÷��̾� ���� ����. ID : " << clientID << endl;
#endif
			}
			else {
				cout << "MAX USER EXCEEDED.\n";
				closesocket(g_clientSocket);
			}
			g_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
			ZeroMemory(&g_expOverlapped.m_overlapped, sizeof(g_expOverlapped.m_overlapped));
			INT addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_serverSocket, g_clientSocket, g_expOverlapped.m_sendMsg, 0,
				addr_size + 16, addr_size + 16, 0, &g_expOverlapped.m_overlapped);
			break;
		}
		case OP_RECV:
		{
			// ��Ŷ ������ �ʿ�
			size_t remainPacketSize = g_gameServer.GetClient(key)->m_prevRemain + received;
			char* remainPacketBuffer = expOverlapped->m_sendMsg;

			while (remainPacketSize > 0) {
				// ���� ������ ����� 0���� Ŭ ��
				int packetSize = remainPacketBuffer[0];

				if (remainPacketSize < packetSize) {
					g_gameServer.GetClient(key)->m_prevRemain = remainPacketSize;
					if (remainPacketSize != 0) {
						memcpy(expOverlapped->m_sendMsg, remainPacketBuffer, remainPacketSize);
					}
					break;
				}
				// �ϳ��� ������ ��Ŷ�� ����� ���� �������
				// ���ڶ� �� Ż��

				ProcessPacket(key, remainPacketBuffer);
				// ��Ŷ ó��

				remainPacketBuffer += packetSize;
				remainPacketSize -= packetSize;
				// ó���� ��ŭ ������ ��ġ�� �ڷ� ����
				// ������ ũ��� ����

				if (remainPacketSize != 0) {
					memcpy(expOverlapped->m_sendMsg, remainPacketBuffer, remainPacketSize);
				}
			}
			if (remainPacketSize == 0) g_gameServer.GetClient(key)->m_prevRemain = 0;
			g_gameServer.GetClient(key)->DoRecv();

			break;
		}
		case OP_SEND:
		{
			delete expOverlapped;
			break;
		}
		case OP_NPC_MOVE:
		{
			g_gameServer.MoveNPC(key);
			bool activate = false;

			auto npc = g_gameServer.GetNPC(key);

			short sectorX = npc->m_position.x / (VIEW_RANGE * 2);
			short sectorY = npc->m_position.y / (VIEW_RANGE * 2);
			const array<INT, 9> dx = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
			const array<INT, 9> dy = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
			// �ֺ� 9���� ���� ���� ����
			for (int i = 0; i < 9; ++i) {
				if (sectorX + dx[i] >= MAP_WIDTH / (VIEW_RANGE * 2) || sectorX + dx[i] < 0 ||
					sectorY + dy[i] >= MAP_HEIGHT / (VIEW_RANGE * 2) || sectorY + dy[i] < 0) {
					continue;
				}

				g_sectorLock[sectorY + dy[i]][sectorX + dx[i]].lock();
				for (auto& id : g_sector[sectorY + dy[i]][sectorX + dx[i]]) {
					if (id < MAX_USER) {
						if (g_gameServer.GetClient(id)->m_state != OBJECT::INGAME) continue;
						if (!g_gameServer.CanSee(key, id)) continue;
						activate = true;
						break;
					}
				}
				g_sectorLock[sectorY + dy[i]][sectorX + dx[i]].unlock();

			}
			if (activate) {
				int moveCount = --expOverlapped->m_eventMsg;
				if (moveCount > 0) {
					g_gameServer.AddTimer(key, Event::RANDOM_MOVE, chrono::system_clock::now() + 1s, moveCount);
				}
				else {
					EXP_OVER* expOverlapped = new EXP_OVER;
					expOverlapped->m_compType = OP_NPC_HELLO;
					//expOverlapped->m_eventMsg = waker;
					//PostQueuedCompletionStatus(g_iocp, 1, id, &expOverlapped->m_overlapped);
				}
			}
			else {
				npc->m_isActive = false;
			}
			delete expOverlapped;
			break;
		}
		case OP_NPC_HELLO:
		{
			auto npc = g_gameServer.GetNPC(key);
			npc->m_luaLock.lock();
			lua_getglobal(npc->m_luaState, "event_player_move");
			lua_pushnumber(npc->m_luaState, expOverlapped->m_eventMsg);
			lua_pcall(npc->m_luaState, 1, 0, 0);
			lua_pop(npc->m_luaState, 1);
			npc->m_luaLock.unlock();
			delete expOverlapped;
			break;
		}
		case OP_NPC_BYE:
		{
			auto npc = g_gameServer.GetNPC(key);
			npc->m_luaLock.lock();
			lua_getglobal(npc->m_luaState, "event_player_leave");
			lua_pushnumber(npc->m_luaState, expOverlapped->m_eventMsg);
			lua_pcall(npc->m_luaState, 1, 0, 0);
			lua_pop(npc->m_luaState, 1);
			npc->m_luaLock.unlock();
			delete expOverlapped;
			break;
		}
		}
	}
}

void ProcessPacket(UINT cid, CHAR* packetBuf)
{
	switch (packetBuf[1])
	{
	case CS_PACKET_LOGIN:
	{
		// ���� �÷��̾� ����
		cs_packet_login* pk = reinterpret_cast<cs_packet_login*>(packetBuf);
#ifdef NETWORK_DEBUG
		cout << "CS_PACKET_LOGIN ����" << endl;
#endif
		{
			sc_packet_login_confirm sendpk;
			sendpk.size = sizeof(sc_packet_login_confirm);
			sendpk.type = SC_PACKET_LOGIN_CONFIRM;
			sendpk.id = cid;
			g_gameServer.GetClient(cid)->DoSend(&sendpk);
			strcpy_s(g_gameServer.GetClient(cid)->m_name, pk->name);
#ifdef NETWORK_DEBUG
			cout << "SC_PACKET_LOGIN_CONFIRM �۽� - ID : " << (int)sendpk.id << endl;
#endif
			{
				unique_lock<mutex> lock(g_gameServer.GetClient(cid)->m_mutex);
				g_gameServer.GetClient(cid)->m_state = OBJECT::INGAME;
			}

		}

		// ���� �ȿ� �ִ� ������Ʈ�� ID�� ����.
		unordered_set<int> newViewList;
		short sectorX = g_gameServer.GetClient(cid)->m_position.x / (VIEW_RANGE * 2);
		short sectorY = g_gameServer.GetClient(cid)->m_position.y / (VIEW_RANGE * 2);
		const array<INT, 9> dx = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
		const array<INT, 9> dy = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
		// �ֺ� 9���� ���� ���� ����
		for (int i = 0; i < 9; ++i) {
			if (sectorX + dx[i] >= MAP_WIDTH / (VIEW_RANGE * 2) || sectorX + dx[i] < 0 ||
				sectorY + dy[i] >= MAP_HEIGHT / (VIEW_RANGE * 2) || sectorY + dy[i] < 0) {
				continue;
			}

			g_sectorLock[sectorY + dy[i]][sectorX + dx[i]].lock();
			for (auto& id : g_sector[sectorY + dy[i]][sectorX + dx[i]]) {
				{
					if (id < MAX_USER) {
						unique_lock<mutex> lock(g_gameServer.GetClient(id)->m_mutex);
						if (g_gameServer.GetClient(id)->m_state != OBJECT::INGAME) continue;
					}
					else {
						unique_lock<mutex> lock(g_gameServer.GetNPC(id)->m_mutex);
						if (g_gameServer.GetNPC(id)->m_state != OBJECT::INGAME) continue;
					}
				}
				if (!g_gameServer.CanSee(cid, id)) continue;

				// �ϴ� ������ ����
				g_gameServer.GetClient(cid)->SendAddPlayer(id);
				// ���� NPC�� �ƴ� ��� ����
				if (id < MAX_USER) g_gameServer.GetClient(id)->SendAddPlayer(cid);
				else g_gameServer.WakeupNPC(id, cid);
			}
			g_sectorLock[sectorY + dy[i]][sectorX + dx[i]].unlock();
		}
		break;
	}
	case CS_PACKET_MOVE:
	{
		cs_packet_move* pk = reinterpret_cast<cs_packet_move*>(packetBuf);
#ifdef NETWORK_DEBUG
		cout << "CS_PACKET_MOVE ����" << endl;
#endif

		g_gameServer.Move(cid, (*pk).direction);
		g_gameServer.GetClient(cid)->m_lastMoveTime = pk->moveTime;
		
		// ���� �ȿ� �ִ� ������Ʈ�� ID�� ����.
		unordered_set<int> newViewList;
		g_gameServer.GetClient(cid)->m_viewLock.lock();
		auto oldViewList = g_gameServer.GetClient(cid)->m_viewList;
		g_gameServer.GetClient(cid)->m_viewLock.unlock();

		short sectorX = g_gameServer.GetClient(cid)->m_position.x / (VIEW_RANGE * 2);
		short sectorY = g_gameServer.GetClient(cid)->m_position.y / (VIEW_RANGE * 2);
		const array<INT, 9> dx = { -1, 0, 1, -1, 0, 1, -1, 0, 1 };
		const array<INT, 9> dy = { -1, -1, -1, 0, 0, 0, 1, 1, 1 };
		// �ֺ� 9���� ���� ���� ����
		for (int i = 0; i < 9; ++i) {
			if (sectorX + dx[i] >= MAP_WIDTH / (VIEW_RANGE * 2) || sectorX + dx[i] < 0 ||
				sectorY + dy[i] >= MAP_HEIGHT / (VIEW_RANGE * 2) || sectorY + dy[i] < 0) {
				continue;
			}

			g_sectorLock[sectorY + dy[i]][sectorX + dx[i]].lock();
			for (auto& id : g_sector[sectorY + dy[i]][sectorX + dx[i]]) {
				if (id < MAX_USER) {
					if (g_gameServer.GetClient(id)->m_state != OBJECT::INGAME) continue;
				}
				else {
					if (g_gameServer.GetNPC(id)->m_state != OBJECT::INGAME) continue;
				}
				if (!g_gameServer.CanSee(cid, id)) continue;

				newViewList.insert(id);
			}
			g_sectorLock[sectorY + dy[i]][sectorX + dx[i]].unlock();
		}

		// �� View List�� �߰��Ǿ��µ� ������ ���� �÷��̾��� ���� �߰�
		for (auto& id : newViewList) {
			g_gameServer.GetClient(cid)->m_viewLock.lock();
			if (!g_gameServer.GetClient(cid)->m_viewList.count(id)) {
				// View List�� ���ٸ� �����ش�.
				g_gameServer.GetClient(cid)->m_viewLock.unlock();
				g_gameServer.GetClient(cid)->SendAddPlayer(id);
				if (id < MAX_USER) g_gameServer.GetClient(id)->SendAddPlayer(cid);
			}
			else {
				// �ִٸ� �̵� ��Ų��.
				g_gameServer.GetClient(cid)->m_viewLock.unlock();
				if (id < MAX_USER) g_gameServer.GetClient(id)->SendObjectInfo(cid);
			}

			if (!oldViewList.count(id)) {
				g_gameServer.GetClient(cid)->SendAddPlayer(id);
				if (id >= MAX_USER) g_gameServer.WakeupNPC(id, cid);
			}
		}
		
		// �� View List�� ���ŵǾ��µ� ������ �ִ� �÷��̾��� ���� ����

		for (auto& id : oldViewList) {
			if (!newViewList.count(id)) {
				g_gameServer.GetClient(cid)->SendExitPlayer(id);
				if (id < MAX_USER) g_gameServer.GetClient(id)->SendExitPlayer(cid);
			}
		}

		break;
	}
	}
}

void TimerThread(HANDLE hiocp)
{
	g_gameServer.TimerThread(hiocp);
}