/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* eraUtil.h - header file for eraUtil.c
 */

#ifndef ERAUTIL_H
#define ERAUTIL_H

#include "apiHeaderAll.h"
#include "objMetaOpr.h"
#include "miscUtil.h"



int appendStrToBBuf(bytesBuf_t *dest, char *str);
int appendFormattedStrToBBuf(bytesBuf_t *dest, size_t size, const char *format, ...);
char *unescape(char *myStr);
int parseMetadataModLine(char *inpLine, rsComm_t *rsComm);
int copyAVUMetadata(char *destPath, char *srcPath, rsComm_t *rsComm);
int recursiveCollCopy(collInp_t *destCollInp, collInp_t *srcCollInp, rsComm_t *rsComm);
int getDataObjPSmeta(char *objPath, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getCollectionPSmeta(char *objPath, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getDataObjACL(dataObjInp_t *myDataObjInp, bytesBuf_t *mybuf, rsComm_t *rsComm);
int getCollectionACL(collInp_t *myCollInp, char *label, bytesBuf_t *mybuf, rsComm_t *rsComm);
int loadMetadataFromDataObj(dataObjInp_t *dataObjInp, rsComm_t *rsComm);
int genQueryOutToXML(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, char **tags);
int extractPSQueryResults(int status, genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, char *fullName);
int extractGenQueryResults(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, char *header, char **descriptions);
int getUserInfo(char *userName, bytesBuf_t *mybuf, rsComm_t *rsComm);
int genAdminOpFromDataObj(dataObjInp_t *dataObjInp, generalAdminInp_t *generalAdminInp, rsComm_t *rsComm);
int parseGenAdminLine(char *inpLine, generalAdminInp_t *generalAdminInp, rsComm_t *rsComm);
int loadACLFromDataObj(dataObjInp_t *dataObjInp, rsComm_t *rsComm);
int extractACLQueryResults(genQueryOut_t *genQueryOut, bytesBuf_t *mybuf, int coll_flag);
int getUserACL(char *userName, bytesBuf_t *mybuf, rsComm_t *rsComm);
int parseACLModLine(char *inpLine, rsComm_t *rsComm);
int getSqlRowsByInx(genQueryOut_t *genQueryOut, intArray_t *indexes, bytesBuf_t *mybuf);
int getObjectByFilePath(char *filePath, char *rescName, char *objPath, rsComm_t *rsComm);

#endif	/* ERAUTIL_H */


