#ifndef _STARTSOCK_H
#define _STARTSOCK_H

#ifdef _WIN32



#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <errno.h>
#include <fcntl.h>
#include "Unix2Nt.h"

/* #define errno WSAGetLastError() */
#define COMM_BUF_SIZE 512

#ifdef  __cplusplus
extern "C" {
#endif

int startWinsock();
int cleanWinsock();
SOCKET duplicateWinsock(SOCKET oldSocket);

#ifdef  __cplusplus
}
#endif

#endif



#endif
