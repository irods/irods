/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsApiHandler.h - header file rsApiHandler.c
 */



#ifndef RS_API_HANDLER_H
#define RS_API_HANDLER_H

#include "rods.h"
#include "apiHandler.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rcMisc.h"

#define	DISCONN_STATUS	-1
#define SEND_RCV_RETRY_CNT	1
#define SEND_RCV_SLEEP_TIME     2	/* in sec */

/* definition for handlerFlag used in _rsDataObjPut, _rsDataObjGet, etc */
#define BRANCH_MSG	0x1	/* whether to call sendAndRecvBranchMsg */
#define INTERNAL_SVR_CALL 0x2   /* called internally. not from the handler.
				 * error msg will be handled differently */

/* definition for flags in readAndProcClientMsg */
#define RET_API_STATUS	0x1	/* return the status of the API instead of
				 * the status of the readAndProcClientMsg
				 * call */
#define READ_HEADER_TIMEOUT 0x2 /* timeout when reading the header */

#define READ_HEADER_TIMEOUT_IN_SEC	86400	/* timeout in sec */
/* deinition for return value of readTimeoutHandler */
#define L1DESC_INUSE	1
#define READ_HEADER_TIMED_OUT 2
#define MAX_READ_HEADER_RETRY 3
int
rsApiHandler (rsComm_t *rsComm, int apiNumber, bytesBuf_t *inputStructBBuf,
bytesBuf_t *bsBBuf);
int
chkApiVersion (rsComm_t *rsComm, int apiInx);
int
chkApiPermission (rsComm_t *rsComm, int apiInx);
int
handlePortalOpr (rsComm_t *rsComm);
int
sendApiReply (rsComm_t *rsComm, int apiInx, int retVal,
void *myOutStruct, bytesBuf_t *myOutBsBBuf);
int
sendAndProcApiReply ( rsComm_t *rsComm, int apiInx, int status,
void *myOutStruct, bytesBuf_t *myOutBsBBuf);
int readAndProcClientMsg (rsComm_t *rsComm, int retApiStatus);
int
sendAndRecvBranchMsg (rsComm_t *rsComm, int apiInx, int status,
void *myOutStruct, bytesBuf_t *myOutBsBBuf);
int
svrSendCollOprStat (rsComm_t *rsComm, collOprStat_t *collOprStat);
int
_svrSendCollOprStat (rsComm_t *rsComm, collOprStat_t *collOprStat);
int
svrSendZoneCollOprStat (rsComm_t *rsComm, rcComm_t *conn,
collOprStat_t *collOprStat, int retval);
void
readTimeoutHandler (int sig);

#endif	/* RS_API_HANDLER_H */
