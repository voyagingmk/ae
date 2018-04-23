#include "wynet.h"
#include "logger/log.h"
#include "logger/logger.h"

using namespace wynet;
using namespace std;

void threadProducer(int i)
{
    for (int k = 0; k < 100; k++)
    {
        log_debug("threadProducer<%d>: k = %d", i, k);
    }
}

int main()
{
    Logger logger("test");
    setLogLevel(LOG_LEVEL::LOG_DEBUG);
    setLogger(&logger);
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