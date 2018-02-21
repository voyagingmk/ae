#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

extern "C" {
#include "wrapsock.h"
#include "error.h"
#include "ae.h"
#include "ikcp.h"
}

typedef IUINT32 ConvID;
