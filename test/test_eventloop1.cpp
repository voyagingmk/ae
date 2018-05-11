#include "net.h"
using namespace wynet;

void AnotherLoop()
{
    printf("AnotherLoop(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("AnotherLoop(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    loop.loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    Thread thread(AnotherLoop);
    thread.start();

    EventLoop loop;
    loop.loop();

    pthread_exit(NULL);
    return 0;
}