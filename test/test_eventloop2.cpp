#include "wynet.h"
using namespace wynet;

EventLoop *g_loop;

void threadEntry()
{
    printf("threadEntry(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("threadEntry(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    g_loop->loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    g_loop = &loop;

    Thread thread(threadEntry);
    thread.start();
    thread.join();
    return 0;
}