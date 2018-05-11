#ifndef WY_UTILS_H
#define WY_UTILS_H

#include "common.h"
#include "logger/log.h"

#ifdef DEBUG_MODE

void LogSocketState(int fd);

#else

#define LogSocketState(fd)

#endif

int SetSockSendBufSize(int fd, int newSndbuf, bool force = false);

int SetSockRecvBufSize(int fd, int newRcvbuf, bool force = false);

std::string hostname();

#endif
