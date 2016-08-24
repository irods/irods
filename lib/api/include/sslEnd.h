#ifndef SSL_END_H__
#define SSL_END_H__

#include "rcConnect.h"

typedef struct {
    char *arg0;
} sslEndInp_t;
#define sslEndInp_PI "str *arg0;"

#ifdef __cplusplus
extern "C"
#endif
int rcSslEnd( rcComm_t *conn, sslEndInp_t *sslEndInp );

#endif
