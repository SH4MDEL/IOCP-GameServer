#include "main.h"

#include "mainScene.h"

void InitInstance();

int main()
{
	wcout.imbue(locale("korean"));
    sf::Socket::Status status = g_socket.connect("127.0.0.1", SERVER_PORT);
	g_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"������ ������ �� �����ϴ�.\n";
		exit(-1);
	}

	if (!g_font.loadFromFile("..\\Resource\\cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}

	InitInstance();

	g_window = make_shared<sf::RenderWindow>(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Overlapped I/O 2");

    cs_packet_login packet;
    packet.size = sizeof(cs_packet_login);
    packet.type = CS_PACKET_LOGIN;
    Send(&packet);
#ifdef NETWORK_DEBUG
    cout << "CS_PACKET_LOGIN �۽�" << endl;
#endif
	
	while (g_window->isOpen()) {
        sf::Event event;
		while (g_window->pollEvent(event))
		{
			if (event.type == sf::Event::Closed) {
				g_window->close();
			}
			if (event.type == sf::Event::KeyPressed) {
				g_gameFramework.OnProcessingKeyboardMessage(event);
			}
		}
		g_window->clear();
		g_gameFramework.FrameAdvance();
		g_window->display();
	}
}

void InitInstance()
{
	g_gameFramework.OnCreate();
}

void Send(void* packetBuf)
{
    unsigned char* packet = reinterpret_cast<unsigned char*>(packetBuf);
    size_t sent = 0;
    g_socket.send(packetBuf, packet[0], sent);
}

void Recv()
{
    char buf[BUFSIZE];
    size_t received;
    auto recvResult = g_socket.receive(buf, BUFSIZE, received);

    if (recvResult == sf::Socket::Error) {
#ifdef NETWORK_DEBUG
        cout << "Recv Error!" << endl;
        exit(-1);
#endif // NETWORK_DEBUG
    }
    if (recvResult == sf::Socket::Disconnected) {
#ifdef NETWORK_DEBUG
        cout << "Disconnected!" << endl;
        exit(-1);
#endif // NETWORK_DEBUG
    }
    if (recvResult != sf::Socket::NotReady) {
        if (received > 0) {
            TranslatePacket(buf, received);
        }
    }
}

void TranslatePacket(char* buf, size_t io_byte)
{
    char* packet = buf;
    static size_t remainPacketSize = 0;
    static char remainPacketBuffer[BUFSIZE];

    memcpy(remainPacketBuffer + remainPacketSize, packet, io_byte);
    // ���� ���� ���ۿ� �̹��� ���� ��Ŷ�� �̾� ����
    remainPacketSize += io_byte;
    // �̹��� ���� ������ �����ŭ ����
    while (remainPacketSize > 0) {
        // ���� ������ ����� 0���� Ŭ ��
        int packetSize = remainPacketBuffer[0];

        if (remainPacketSize < packetSize) break;
        // �ϳ��� ������ ��Ŷ�� ����� ���� �������
        // ���ڶ� �� Ż��

        ProcessPacket(remainPacketBuffer);
        // ��Ŷ ó��

        packet += packetSize;
        remainPacketSize -= packetSize;
        if (remainPacketSize != 0) {
            memcpy(remainPacketBuffer, packet, remainPacketSize);
        }
    }
}

void ProcessPacket(char* buf)
{
    switch (buf[1])
    {
    case SC_PACKET_LOGIN_CONFIRM:
    {
        sc_packet_login_confirm* pk = reinterpret_cast<sc_packet_login_confirm*>(buf);
        g_clientID = pk->id;
#ifdef NETWORK_DEBUG
        cout << "SC_PACKET_LOGIN_CONFIRM ����" << endl;
#endif
        break;
    }
    case SC_PACKET_ADD_PLAYER:
    {
        sc_packet_add_player* pk = reinterpret_cast<sc_packet_add_player*>(buf);
        MainScene* scene = (MainScene*)g_gameFramework.GetScene();
        scene->AddPlayer(pk->id, pk->coord);
#ifdef NETWORK_DEBUG
        cout << "SC_PACKET_ADD_PLAYER ����" << endl;
#endif
        break;
    }
    case SC_PACKET_OBJECT_INFO:
    {
        sc_packet_object_info* pk = reinterpret_cast<sc_packet_object_info*>(buf);
        MainScene* scene = (MainScene*)g_gameFramework.GetScene();
        scene->Move(pk->id, pk->coord);
#ifdef NETWORK_DEBUG
        cout << "SC_PACKET_OBJECT_INFO ����" << endl;
#endif
        break;
    }
    case SC_PACKET_EXIT_PLAYER:
    {
        sc_packet_exit_player* pk = reinterpret_cast<sc_packet_exit_player*>(buf);
        MainScene* scene = (MainScene*)g_gameFramework.GetScene();
        scene->ExitPlayer(pk->id);
#ifdef NETWORK_DEBUG
        cout << "SC_PACKET_EXIT_PLAYER ����" << endl;
#endif
        break;
    }
    }
}
