#include <iostream>
#include <thread>
using namespace std;

volatile int* bound;
volatile bool done_flag;
int g_error;

void thread1()
{
	for (int i = 0; i < 50000000; ++i) {
		*bound = -(1 + *bound);
	}
	done_flag = true;
}

void thread2()
{
	while (!done_flag)
	{
		int v = *bound;
		if (v != 0 && v != -1) {
			printf("%x ", v);
			++g_error;
		}
	}
}

int main()
{
	//bound = new int{ 0 };
	// 128����Ʈ �Ҵ�
	int mem[32];
	// �� �� ������ �ּ�
	long long address = reinterpret_cast<long long>(&mem[31]);
	// 64�� ����� �ٲ�
	address = address / 64 * 64;
	address -= 2;
	// bound�� cache line�� 2����Ʈ�� ���� �ִ�.
	bound = reinterpret_cast<int*>(address);
	*bound = 0;

	thread t1(thread1);
	thread t2(thread2);

	t1.join();
	t2.join();

	cout << g_error << endl;
}