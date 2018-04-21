#include "wynet.h"

using namespace wynet;
using namespace std;


void threadProducer(int i)
{
  
}

void threadComsumer(int i)
{

}

int main()
{

    Logger logger("test");
    logger.start();

    vector<shared_ptr<Thread>> threads;

    for (int i = 0; i < 10; i++)
    {
        threads.push_back(make_shared<Thread>(std::bind(threadProducer, i), "thread" + to_string(i)));
    }

    int i = 10;
    threads.push_back(make_shared<Thread>(std::bind(threadComsumer, i), "thread" + to_string(i)));

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