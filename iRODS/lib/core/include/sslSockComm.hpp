/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* sslSockComm.h - header file for sslSockComm.c
 */

#ifndef SSL_SOCK_COMM_HPP
#define SSL_SOCK_COMM_HPP

#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>

#include "rodsDef.h"
#include "rcConnect.hpp"
#include "rodsPackInstruct.hpp"

#define SSL_CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

#ifdef __cplusplus
extern "C" {
#endif

int
sslStart( rcComm_t *rcComm );
int
sslEnd( rcComm_t *rcComm );
int
sslAccept( rsComm_t *rsComm );
int
sslShutdown( rsComm_t *rsComm );
int
sslReadMsgHeader( int sock, msgHeader_t *myHeader, struct timeval *tv, SSL *ssl );
int
sslReadMsgBody( int sock, msgHeader_t *myHeader, bytesBuf_t *inputStructBBuf,
                bytesBuf_t *bsBBuf, bytesBuf_t *errorBBuf, irodsProt_t irodsProt,
                struct timeval *tv, SSL *ssl );
int
sslWriteMsgHeader( msgHeader_t *myHeader, SSL *ssl );
int
sslSendRodsMsg( char *msgType, bytesBuf_t *msgBBuf,
                bytesBuf_t *byteStreamBBuf, bytesBuf_t *errorBBuf, int intInfo,
                irodsProt_t irodsProt, SSL *ssl );
int
sslRead( int sock, void *buf, int len,
         int *bytesRead, struct timeval *tv, SSL *ssl );
int
sslWrite( void *buf, int len,
          int *bytesWritten, SSL *ssl );

#ifdef __cplusplus
}
#endif

#endif	/* SSL_SOCK_COMM_H */
