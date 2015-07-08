/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*** This code is rewritten by Illyoung Choi (iychoi@email.arizona.edu)    ***
 *** funded by iPlantCollaborative (www.iplantcollaborative.org).          ***/
#ifndef IFUSE_LIB_RODSCLIENTAPI_HPP
#define	IFUSE_LIB_RODSCLIENTAPI_HPP

#include <pthread.h>
#include "rodsClient.h"

#define IFUSE_RODSCLIENTAPI_TIMEOUT_SEC     (30)

void iFuseRodsClientInit();
void iFuseRodsClientDestroy();

int iFuseRodsClientReadMsgError(int status);

rcComm_t *iFuseRodsClientConnect(const char *rodsHost, int rodsPort, const char *userName, const char *rodsZone, int reconnFlag, rErrMsg_t *errMsg);
int iFuseRodsClientLogin(rcComm_t *conn);
int iFuseRodsClientDisconnect(rcComm_t *conn);


int iFuseRodsClientDataObjOpen(rcComm_t *conn, dataObjInp_t *dataObjInp);
int iFuseRodsClientDataObjClose(rcComm_t *conn, openedDataObjInp_t *dataObjCloseInp);
int iFuseRodsClientOpenCollection( rcComm_t *conn, char *collection, int flag, collHandle_t *collHandle );
int iFuseRodsClientCloseCollection(collHandle_t *collHandle);
int iFuseRodsClientObjStat(rcComm_t *conn, dataObjInp_t *dataObjInp, rodsObjStat_t **rodsObjStatOut);
int iFuseRodsClientDataObjLseek(rcComm_t *conn, openedDataObjInp_t *dataObjLseekInp, fileLseekOut_t **dataObjLseekOut);
int iFuseRodsClientDataObjRead(rcComm_t *conn, openedDataObjInp_t *dataObjReadInp, bytesBuf_t *dataObjReadOutBBuf);
int iFuseRodsClientDataObjWrite(rcComm_t *conn, openedDataObjInp_t *dataObjWriteInp, bytesBuf_t *dataObjWriteInpBBuf);
int iFuseRodsClientDataObjCreate(rcComm_t *conn, dataObjInp_t *dataObjInp);
int iFuseRodsClientDataObjUnlink(rcComm_t *conn, dataObjInp_t *dataObjUnlinkInp);
int iFuseRodsClientReadCollection(rcComm_t *conn, collHandle_t *collHandle, collEnt_t *collEnt);
int iFuseRodsClientCollCreate(rcComm_t *conn, collInp_t *collCreateInp);
int iFuseRodsClientRmColl(rcComm_t *conn, collInp_t *rmCollInp, int vFlag);
int iFuseRodsClientDataObjRename(rcComm_t *conn, dataObjCopyInp_t *dataObjRenameInp);
int iFuseRodsClientDataObjTruncate(rcComm_t *conn, dataObjInp_t *dataObjInp);
int iFuseRodsClientModDataObjMeta(rcComm_t *conn, modDataObjMeta_t *modDataObjMetaInp);

#endif	/* IFUSE_LIB_RODSCLIENTAPI_HPP */

