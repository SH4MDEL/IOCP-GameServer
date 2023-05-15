#include "client.h"

CLIENT::CLIENT() : OBJECT(), m_socket{0}
{}

CLIENT::~CLIENT() { closesocket(m_socket); }

void CLIENT::DoRecv()
{
	DWORD recv_flag = 0;
	memset(&m_recvOver.m_overlapped, 0, sizeof(m_recvOver.m_overlapped));
	m_recvOver.m_wsaBuf.len = BUFSIZE - m_prevRemain;

	m_recvOver.m_wsaBuf.buf = m_recvOver.m_sendMsg + m_prevRemain;
	int retval = WSARecv(m_socket, &m_recvOver.m_wsaBuf, 1, 0, &recv_flag, 
		&m_recvOver.m_overlapped, 0);
}

void CLIENT::DoSend(void* packet)
{
	EXP_OVER* send_over = new EXP_OVER(reinterpret_cast<char*>(packet));
	WSASend(m_socket, &send_over->m_wsaBuf, 1, 0, 0, &send_over->m_overlapped, 0);
}

void CLIENT::SendLoginConfirm()
{
}

void CLIENT::SendAddPlayer(INT id)
{
	// id�� ���� �÷��̾ ���� �����Ѵ�.
	m_viewLock.lock();
	if (m_viewList.count(id)) {
		// �ش� id�� ���� �÷��̾ �þ߿� �̹� �����Ѵ�.
		m_viewLock.unlock();
		// �̵��� ��Ű�� �����Ѵ�.
		SendObjectInfo(id);
		return;
	}
	m_viewList.insert(id);
	m_viewLock.unlock();

	sc_packet_add_player sendpk;
	sendpk.size = sizeof(sc_packet_add_player);
	sendpk.type = SC_PACKET_ADD_PLAYER;
	sendpk.id = id;
	if (id < MAX_USER) {
		sendpk.coord = g_gameServer.GetClient((UINT)id)->m_position;
	}
	else {
		sendpk.coord = g_gameServer.GetNPC((UINT)id)->m_position;
	}
	DoSend(&sendpk);
#ifdef NETWORK_DEBUG
	cout << "SC_PACKET_ADD_PLAYER �۽� - ID : " << id << ", x : " << sendpk.coord.x << ", y : " << sendpk.coord.y << endl;
#endif
}

void CLIENT::SendObjectInfo(INT id)
{
	m_viewLock.lock();
	if (!m_viewList.count(id)) {
		// �þ߿� �ش� ������Ʈ�� �������� �ʴ´ٸ�
		m_viewLock.unlock();
		// �� ������Ʈ�� ���� �߰����ش�.
		SendAddPlayer(id);
		return;
	}
	m_viewLock.unlock();

	sc_packet_object_info sendpk;
	sendpk.size = sizeof(sc_packet_object_info);
	sendpk.type = SC_PACKET_OBJECT_INFO;
	sendpk.id = id;
	if (id < MAX_USER) {
		sendpk.coord = g_gameServer.GetClient((UINT)id)->m_position;
		sendpk.moveTime = g_gameServer.GetClient(id)->m_lastMoveTime;
	}
	else {
		sendpk.coord = g_gameServer.GetNPC((UINT)id)->m_position;
		sendpk.moveTime = 0;
	}

	DoSend(&sendpk);
#ifdef NETWORK_DEBUG
	cout << "SC_PACKET_OBJECT_INFO �۽� - ID : " << id << ", x : " << sendpk.coord.x << ", y : " << sendpk.coord.y << endl;
#endif
}

void CLIENT::SendExitPlayer(INT id)
{
	m_viewLock.lock();
	if (!m_viewList.count(id)) {
		// �̹� �þ߿� �ش� ������Ʈ�� �������� �ʴ´ٸ�
		m_viewLock.unlock();
		// �ƹ� �ϵ� ���� �ʴ´�.
		return;
	}
	m_viewList.erase(id);
	m_viewLock.unlock();

	sc_packet_exit_player packet;
	packet.size = sizeof(sc_packet_exit_player);
	packet.type = SC_PACKET_EXIT_PLAYER;
	packet.id = id;
	DoSend(&packet);

#ifdef NETWORK_DEBUG
	cout << "SC_PACKET_EXIT_PLAYER �۽� - ID : " << id << endl;
#endif
}
