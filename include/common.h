#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <memory>

#define DEBUG_MODE 1

#define LOG_USE_COLOR 1

extern "C" {
#include "log.h"
#include "wrapsock.h"
#include "error.h"
#include "ae.h"
#include "ikcp.h"
}

typedef IUINT32 ConvID;

#ifdef __APPLE__
#define fwrite_unlocked fwrite
#define fflush_unlocked fflush
#endif

#define WYNET_CACHELINE_SIZE 64

#ifdef _MSC_VER
#define WYNET_CACHELINE_ALIGNMENT __declspec(align(WYNET_CACHELINE_SIZE))
#endif

#ifdef __GNUC__
#define WYNET_CACHELINE_ALIGNMENT __attribute__((aligned(WYNET_CACHELINE_SIZE)))
#endif

#ifndef WYNET_CACHELINE_ALIGNMENT
#define WYNET_CACHELINE_ALIGNMENT
#endif

#define G_LIKELY(expr) (__builtin_expect(!!(expr), 1))
#define G_UNLIKELY(expr) (__builtin_expect(!!(expr), 0))
