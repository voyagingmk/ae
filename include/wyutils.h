#ifndef WY_UTILS_H
#define WY_UTILS_H

#include "common.h"

#ifdef DEBUG_MODE

void LogSocketState(int fd);

#else

#define LogSocketState(fd)

#endif

int SetSockSendBufSize(int fd, int newSndbuf);

int SetSockRecvBufSize(int fd, int newRcvbuf);

#endif
