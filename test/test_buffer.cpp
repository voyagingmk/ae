#include "net.h"

using namespace wynet;
using namespace std;

MutexLock stdout_mutex;

void threadEntry(int i)
{
    BufferSet *bufferSet = BufferSet::getSingleton();
    for (int b = 0; b < 100; b++)
    {
        size_t s1 = bufferSet->getSize();
        UniqID uid = bufferSet->newBuffer();
        size_t s2 = bufferSet->getSize();
        MutexLockGuard<MutexLock> lock(stdout_mutex);
        cout << "thread:" << i << ", b:" << b << ", s1:" << s1 << ", s2:" << s2 << endl;
    }
    BufferRef a;
    BufferRef b{BufferRef()};
}

int main(int argc, char **argv)
{
    vector<shared_ptr<Thread>> threads;
    for (int i = 0; i < 10; i++)
    {
        threads.push_back(make_shared<Thread>(std::bind(threadEntry, i), "thread" + to_string(i)));
    }
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