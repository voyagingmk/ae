#include "wynet.h"
using namespace wynet;

void threadMain()
{
    printf("threadMain(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("threadMain(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    EventLoop loop;
    loop.loop();
}

int main(int argc, char **argv)
{
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("main(): isMainThread: %s\n", CurrentThread::isMainThread() ? "true" : "false");

    Thread thread(threadMain);
    thread.start();

    EventLoop loop;
    loop.loop();

    pthread_exit(NULL);
    return 0;
}