#include "net.h"

using namespace wynet;
using namespace std;

MutexLock stdout_mutex;
moodycamel::ConcurrentQueue<int> q;

void threadPush(int i)
{
    for (int k = 0; k < 1000; k++)
    {
        q.enqueue(i * 1000 + k);
    }
    MutexLockGuard<MutexLock> lock(stdout_mutex);
    cout << "threadPush finish " << i << endl;
}

void threadPop(int i)
{
    while (1)
    {
        int item;
        if (q.try_dequeue(item))
        {
            MutexLockGuard<MutexLock> lock(stdout_mutex);
            cout << "get " << item << endl;
        }
    }
    {
        MutexLockGuard<MutexLock> lock(stdout_mutex);
        cout << "thread finish" << endl;
    }
}

int main()
{

    vector<shared_ptr<Thread>> threads;
    for (int i = 0; i < 10; i++)
    {
        threads.push_back(make_shared<Thread>(std::bind(threadPush, i), "thread" + to_string(i)));
    }
    int i = 10;
    threads.push_back(make_shared<Thread>(std::bind(threadPop, i), "thread" + to_string(i)));
    for (auto pThread : threads)
    {
        pThread->start();
    }
    for (auto pThread : threads)
    {
        pThread->join();
    }
    return 0;
}