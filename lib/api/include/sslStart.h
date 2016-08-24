#ifndef SSL_START_H__
#define SSL_START_H__

#include "rcConnect.h"

typedef struct {
    char *arg0;
} sslStartInp_t;
#define sslStartInp_PI "str *arg0;"

#ifdef __cplusplus
extern "C"
#endif
int rcSslStart( rcComm_t *conn, sslStartInp_t *sslStartInp );

#endif
