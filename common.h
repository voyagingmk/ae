#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <vector>
#include <iostream>
#include <map>
#include <set>

extern "C" {
#include "wrapsock.h"
#include "error.h"
#include "ae.h"
#include "ikcp.h"
}

typedef IUINT32 ConvID;

