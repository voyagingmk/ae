#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> /* inet(3) functions */
#include <sys/ioctl.h>
#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <atomic>
#include <functional>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <cstdarg>
#include <chrono>

#if TIME_WITH_SYS_TIME
#include <sys/time.h> /* timeval{} for select() */
#include <time.h>     /* timespec{} for pselect() */
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h> /* includes <time.h> unsafely */
#else
#include <time.h> /* old system? */
#endif
#endif

#include <netdb.h>
#include <sys/stat.h> /* for S_xxx file mode constants */
#include <sys/uio.h>  /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h> /* for Unix domain sockets */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h> /* for convenience */
#endif

#ifdef HAVE_SYS_SYSCTL_H
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h> /* OpenBSD prereq for sysctl.h */
#endif
#include <sys/sysctl.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h> /* for convenience */
#endif

#ifdef HAVE_SYS_EVENT_H
#include <sys/event.h> /* for kqueue */
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h> /* for convenience */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif

// #define DEBUG_MODE 1

#define LOG_USE_COLOR 1

// #include "concurrentqueue.h"

extern "C"
{
#include "ae.h"
#include "ikcp.h"
}

using ConvID = IUINT32;

using SockFd = int;

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
