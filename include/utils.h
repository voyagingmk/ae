#ifndef WY_UTILS_H
#define WY_UTILS_H

#include "common.h"
#include "logger/log.h"

#ifdef DEBUG_MODE

void LogSocketState(int fd);

#else

#define LogSocketState(fd)

#endif

std::string hostname();

#endif
