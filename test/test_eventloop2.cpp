#include "wynet.h"
using namespace wynet;

EventLoop *g_loop;

void threadMain()
{
    printf("threadMain(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("threadMain(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    g_loop->loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    g_loop = &loop;

    Thread thread(threadMain);
    thread.start();
    thread.join();
    return 0;
}