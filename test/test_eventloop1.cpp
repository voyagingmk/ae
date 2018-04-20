#include "wynet.h"
using namespace wynet;

void threadEntry()
{
    printf("threadEntry(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("threadEntry(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    loop.loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    Thread thread(threadEntry);
    thread.start();

    EventLoop loop;
    loop.loop();

    pthread_exit(NULL);
    return 0;
}