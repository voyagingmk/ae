#include "wynet.h"
using namespace wynet;

EventLoop *g_loop;

void AnotherLoop()
{
    printf("AnotherLoop(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("AnotherLoop(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    g_loop->loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    g_loop = &loop;

    Thread thread(AnotherLoop);
    thread.start();
    thread.join();
    return 0;
}