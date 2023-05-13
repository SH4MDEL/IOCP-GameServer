#include "main.h"

int main()
{
	HANDLE hiocp;

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

	hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_serverSocket), hiocp, 9999, 0);

	g_clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	g_expOverlapped.m_compType = COMP_TYPE::OP_ACCEPT;
	AcceptEx(g_serverSocket, g_clientSocket, g_expOverlapped.m_sendMsg, 0, 
		addr_size + 16, addr_size + 16, 0, &g_expOverlapped.m_overlapped);

	vector<thread> workerThreads;
	auto numThreads = thread::hardware_concurrency();
	for (UINT i = 0; i < numThreads; ++i) {
		workerThreads.emplace_back(WorkerThread, hiocp);
	}
	for (auto& thread : workerThreads) {
		thread.join();
	}

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
				for (auto& pl : g_gameServer.GetClients()) {
					{
						unique_lock<mutex> lock(pl->m_mutex);
						if (pl->m_state != CLIENT::INGAME) continue;
					}
					if (pl->m_id == key) continue;
					sc_packet_exit_player packet;
					packet.size = sizeof(sc_packet_exit_player);
					packet.type = SC_PACKET_EXIT_PLAYER;
					packet.id = key;
					pl->DoSend(&packet);
				}
				g_gameServer.ExitClient(key);
				if (expOverlapped->m_compType == OP_SEND) delete expOverlapped;
				continue;
			}
		}

		if ((received == 0) && ((expOverlapped->m_compType == OP_RECV) || (expOverlapped->m_compType == OP_SEND))) {
			// ���� ���� ��Ŷ ����
			for (auto& pl : g_gameServer.GetClients()) {
				{
					unique_lock<mutex> lock(pl->m_mutex);
					if (pl->m_state != CLIENT::INGAME) continue;
				}
				if (pl->m_id == key) continue;
				sc_packet_exit_player packet;
				packet.size = sizeof(sc_packet_exit_player);
				packet.type = SC_PACKET_EXIT_PLAYER;
				packet.id = key;
				pl->DoSend(&packet);
			}
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
			{
				unique_lock<mutex> lock(g_gameServer.GetClient(cid)->m_mutex);
				g_gameServer.GetClient(cid)->m_state = CLIENT::INGAME;
			}
#ifdef NETWORK_DEBUG
			cout << "SC_PACKET_LOGIN_CONFIRM �۽� - ID : " << (int)sendpk.id << endl;
#endif
		}
		g_gameServer.GetClient(cid)->SendAddPlayer(cid);

		for (auto& client : g_gameServer.GetClients()) {
			{
				unique_lock<mutex> lock(client->m_mutex);
				if (client->m_state != CLIENT::INGAME) continue;
			}
			if (client->m_id == cid) continue;
			client->SendAddPlayer(cid);
			g_gameServer.GetClient(cid)->SendAddPlayer(client->m_id);
		}
		break;
	}
	case CS_PACKET_MOVE:
	{
		cs_packet_move* pk = reinterpret_cast<cs_packet_move*>(packetBuf);
#ifdef NETWORK_DEBUG
		cout << "CS_PACKET_MOVE ����" << endl;
#endif
		sc_packet_object_info sendpk;
		sendpk.size = sizeof(sc_packet_object_info);
		sendpk.type = SC_PACKET_OBJECT_INFO;
		sendpk.id = cid;
		sendpk.coord = g_gameServer.Move(cid, (*pk).direction);
		sendpk.moveTime = pk->moveTime;
		
		int asdf = 0;
		// ������Ʈ�� �÷��̾��� ������ ��� �÷��̾�� ����
		for (auto& client : g_gameServer.GetClients()) {
			if (client->m_state != CLIENT::INGAME) continue;

			client->DoSend(&sendpk);
#ifdef NETWORK_DEBUG
			cout << "SC_PACKET_OBJECT_INFO �۽� - ID : " << client->m_id << endl;
#endif
		}
		break;
	}
	}
}