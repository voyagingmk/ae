#include "wynet.h"
#include "logger/logger.h"

using namespace wynet;
using namespace std;

Logger *g_Logger;

void threadProducer(int i)
{
    for (int k = 0; k < 1000000000; k++)
    {
        char buff[64];
        snprintf(buff, sizeof(buff), "threadProducer < %d > %d\n", i, k);
        g_Logger->append(buff, strlen(buff));
    }
}

int main()
{
    Logger logger("test");
    g_Logger = &logger;
    logger.start();

    vector<shared_ptr<Thread>> threads;

    for (int i = 0; i < 10; i++)
    {
        threads.push_back(make_shared<Thread>(std::bind(threadProducer, i), "thread" + to_string(i)));
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