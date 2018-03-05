#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>

#define LOG_USE_COLOR 1

extern "C" {
#include "log.h"
#include "wrapsock.h"
#include "error.h"
#include "ae.h"
#include "ikcp.h"
}

typedef IUINT32 ConvID;

