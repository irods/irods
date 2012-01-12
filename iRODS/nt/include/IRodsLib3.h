#ifndef IRODS3_WIN32_LIB
#define IRODS3_WIN32_LIB

#include "unix2nt.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define MAXEXFPATH 1024
#define CONNECT_TIMEOUT 200
#define MAX_CONN_SVR_CNT 100

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

typedef int socklen_t;

#ifdef  __cplusplus
}
#endif

#endif









