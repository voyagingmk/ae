#include "wynet.h"
using namespace wynet;

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("threadFunc(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    loop.loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    Thread thread(threadFunc);
    thread.start();

    EventLoop loop;
    loop.loop();

    pthread_exit(NULL);
    return 0;
}