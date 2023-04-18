#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <chrono>
using namespace std;
using namespace std::chrono;

constexpr int MAX_THREAD = 64;

mutex m;
atomic_int ai = 0;
int sum = 0;

void lock()
{
	int lock_value = 0;
	while (!atomic_compare_exchange_strong(&ai, &lock_value, 1)) lock_value = 0;
}

void unlock()
{
	ai = 0;
}

// ���
// �����尡 �þ���� ���ϱ޼������� ��������.
// 
// ��?
// �޸� ������ locking�ϰ� �����ϴµ�, �� ������ ������尡 ��û���� ���ϴ�.
// ���� �����尡 locking�ϸ� ������ ��������.
// �����尡 �������� convoying ������ �Ͼ��.
// -> ���ؽ��� ���ٴ� ���?

void out_th(int threadid, int num_thread)
{
	for (int i = 0; i < 50000000 / num_thread; ++i) {
		lock();
		sum += 2;
		unlock();
	}
}

int main()
{
	//register int sum = 0;
	for (int num_thread = 1; num_thread <= MAX_THREAD; num_thread *= 2) {
		sum = 0;
		vector<thread> workers;

		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_thread; ++i)
			workers.emplace_back(out_th, i, num_thread);
		for (auto& t : workers) t.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();

		cout << "thread num : " << num_thread << ", sum : " << sum << ", " << exec_ms << "ms" << endl;
	}
	//int num_core = thread::hardware_concurrency();
	//// �� ���μ����� ������ �޾ƿ´�.


}