#include "wynet.h"

using namespace wynet;
using namespace std;

void threadEntry()
{
}

int main(int argc, char **argv)
{
    vector<shared_ptr<Thread>> threads;
    for (int i = 0; i < 10; i++)
    {
        threads.push_back(make_shared<Thread>(threadEntry, "thread" + to_string(i)));
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