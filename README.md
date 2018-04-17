

## Server

主线程只做listen，accept成功后创建Connection并放进工作线程。

多个listen的问题

## Client

只做connect，connect成功后创建Connection。单线程。

## Connection

处理连接通讯。

## EventLoop

每个thread有且只有一个EventLoop，EventLoop调用epoll，处理io事件。

## EventLoopThread

## EventLoopThreadPool




