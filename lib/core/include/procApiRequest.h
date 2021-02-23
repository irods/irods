#ifndef PROC_API_REQUEST_H__
#define PROC_API_REQUEST_H__

#include "rods.h"

#ifdef __cplusplus
extern "C" {
#endif

int procApiRequest(rcComm_t *conn,
                   int apiNumber,
                   const void *inputStruct,
                   const bytesBuf_t *inputBsBBuf,
                   void **outStruct,
                   bytesBuf_t *outBsBBuf);

int sendApiRequest(rcComm_t *conn,
                   int apiInx,
                   const void *inputStruct,
                   const bytesBuf_t *inputBsBBuf);

int procApiReply(rcComm_t *conn,
                 int apiInx,
                 void **outStruct,
                 bytesBuf_t *outBsBBuf,
                 msgHeader_t *myHeader,
                 bytesBuf_t *outStructBBuf,
                 bytesBuf_t *myOutBsBBuf,
                 bytesBuf_t *errorBBuf);

int readAndProcApiReply(rcComm_t *conn,
                        int apiInx,
                        void **outStruct,
                        bytesBuf_t *outBsBBuf);

int branchReadAndProcApiReply(rcComm_t *conn,
                              int apiNumber,
                              void **outStruct,
                              bytesBuf_t *outBsBBuf);

int cliGetCollOprStat(rcComm_t *conn,
                      collOprStat_t *collOprStat,
                      int vFlag,
                      int retval);

int _cliGetCollOprStat(rcComm_t *conn, collOprStat_t **collOprStat);

int apiTableLookup(int apiNumber);

#ifdef __cplusplus
}
#endif

#endif	// PROC_API_REQUEST_H__
