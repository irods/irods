/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sslSockComm.h - header file for sslSockComm.c
 */

#ifndef SSL_SOCK_COMM_H
#define SSL_SOCK_COMM_H

#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

#include "rodsDef.h"
#include "rcConnect.h"
#include "rodsPackInstruct.h"

#define SSL_CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

#ifdef  __cplusplus
extern "C" {
#endif

int
sslStart(rcComm_t *rcComm);
int
sslEnd(rcComm_t *rcComm);
int
sslAccept(rsComm_t *rsComm);
int
sslShutdown(rsComm_t *rsComm);
int 
sslReadMsgHeader(int sock, msgHeader_t *myHeader, struct timeval *tv, SSL *ssl);
int 
sslReadMsgBody(int sock, msgHeader_t *myHeader, bytesBuf_t *inputStructBBuf,
bytesBuf_t *bsBBuf, bytesBuf_t *errorBBuf, irodsProt_t irodsProt,
struct timeval *tv, SSL *ssl);
int 
sslWriteMsgHeader(int sock, msgHeader_t *myHeader, SSL *ssl);
int 
sslSendRodsMsg(int sock, char *msgType, bytesBuf_t *msgBBuf, 
bytesBuf_t *byteStreamBBuf, bytesBuf_t *errorBBuf, int intInfo,
irodsProt_t irodsProt, SSL *ssl);
int 
sslRead(int sock, void *buf, int len, irodsDescType_t irodsDescType,
int *bytesRead, struct timeval *tv, SSL *ssl);
int 
sslWrite(int sock, void *buf, int len, irodsDescType_t irodsDescType,
int *bytesWritten, SSL *ssl);

#ifdef  __cplusplus
}
#endif

#endif /* USE_SSL */
#endif	/* SSL_SOCK_COMM_H */
