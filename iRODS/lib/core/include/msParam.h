/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* msParam.h - header file for msParam.c
 */



#ifndef MS_PARAM_H
#define MS_PARAM_H

#include "rods.h"
#include "rodsError.h"
#include "objInfo.h"
#include "dataObjCopy.h"
#include "structFileExtAndReg.h"
#include "execCmd.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* some commonly used MS (micro service) type */
#define STR_MS_T                "STR_PI"
#define INT_MS_T                "INT_PI"
#define BUF_LEN_MS_T            "BUF_LEN_PI"    /* an integer indication the
                                                 * length of BBuf */
#define DOUBLE_MS_T             "DOUBLE_PI"
#define DataObjInp_MS_T         "DataObjInp_PI"
#define DataObjCloseInp_MS_T    "DataObjCloseInp_PI"
#define DataObjCopyInp_MS_T     "DataObjCopyInp_PI"
#define DataObjReadInp_MS_T     "dataObjReadInp_PI"
#define DataObjWriteInp_MS_T    "dataObjWriteInp_PI"
#define DataObjLseekInp_MS_T    "fileLseekInp_PI"
#define DataObjLseekOut_MS_T    "fileLseekOut_PI"
#define KeyValPair_MS_T         "KeyValPair_PI"
#define TagStruct_MS_T          "TagStruct_PI"
#define CollInp_MS_T            "CollInpNew_PI"
#define ExecCmd_MS_T            "ExecCmd_PI"
#define ExecCmdOut_MS_T         "ExecCmdOut_PI"
#define RodsObjStat_MS_T        "RodsObjStat_PI"
#define VaultPathPolicy_MS_T    "VaultPathPolicy_PI"
#define StrArray_MS_T		"StrArray_PI"
#define IntArray_MS_T		"IntArray_PI"
#define GenQueryInp_MS_T	"GenQueryInp_PI"
#define GenQueryOut_MS_T	"GenQueryOut_PI"
#define XmsgTicketInfo_MS_T     "XmsgTicketInfo_PI"
#define SendXmsgInfo_MS_T       "SendXmsgInfo_PI"
#define GetXmsgTicketInp_MS_T   "GetXmsgTicketInp_PI"
#define SendXmsgInp_MS_T        "SendXmsgInp_PI"
#define RcvXmsgInp_MS_T         "RcvXmsgInp_PI"
#define RcvXmsgOut_MS_T         "RcvXmsgOut_PI"
#define StructFileExtAndRegInp_MS_T         "StructFileExtAndRegInp_PI"
#define RuleSet_MS_T "RuleSet_PI"
#define RuleStruct_MS_T         "RuleStruct_PI"
#define DVMapStruct_MS_T        "DVMapStruct_PI"
#define FNMapStruct_MS_T        "FNMapStruct_PI"
#define MsrvcStruct_MS_T         "MsrvcStruct_PI"

/* micro service input/output parameter */
typedef struct MsParam {
    char *label;
    char *type;         /* this is the name of the packing instruction in
                         * rodsPackTable.h */
    void *inOutStruct;
    bytesBuf_t *inpOutBuf;
} msParam_t;

typedef struct MsParamArray {
    int len;
    int oprType;
    msParam_t **msParam;
} msParamArray_t;

#define MS_INP_SEP_STR	"++++"		/* the separator str for msInp */
#define MS_NULL_STR	"null"		/* no input */
typedef struct ParsedMsKeyValStr {
    char *inpStr;
    char *endPtr;	/* end pointer */
    char *curPtr;	/* current position */
    char *kwPtr;
    char *valPtr;
} parsedMsKeyValStr_t;

typedef struct ValidKeyWd {
    int flag;
    char *keyWd; 
} validKeyWd_t;

/* valid keyWd flags for dataObjInp_t */

#define RESC_NAME_FLAG		0x1
#define DEST_RESC_NAME_FLAG 	0x2
#define BACKUP_RESC_NAME_FLAG 	0x4
#define FORCE_FLAG_FLAG 	0x8
#define ALL_FLAG 		0x10
#define LOCAL_PATH_FLAG 	0x20
#define VERIFY_CHKSUM_FLAG	0x40
#define IRODS_ADMIN_FLAG 	0x80
#define UPDATE_REPL_FLAG 	0x100
#define REPL_NUM_FLAG 		0x200
#define DATA_TYPE_FLAG 		0x400
#define CHKSUM_ALL_FLAG 	0x800
#define FORCE_CHKSUM_FLAG 	0x1000
#define FILE_PATH_FLAG 		0x2000
#define CREATE_MODE_FLAG 	0x4000
#define OPEN_FLAGS_FLAG		0x8000
#define COLL_FLAGS_FLAG		0x8000
#define DATA_SIZE_FLAGS		0x10000
#define NUM_THREADS_FLAG	0x20000
#define OPR_TYPE_FLAG		0x40000
#define OBJ_PATH_FLAG		0x80000
#define COLL_NAME_FLAG		0x80000
#define IRODS_RMTRASH_FLAG	0x100000
#define IRODS_ADMIN_RMTRASH_FLAG 0x200000
#define DEF_RESC_NAME_FLAG 	0x400000
#define RBUDP_TRANSFER_FLAG 	0x800000
#define RBUDP_SEND_RATE_FLAG 	0x1000000
#define RBUDP_PACK_SIZE_FLAG 	0x2000000
#define BULK_OPR_FLAG 		0x4000000

int
resetMsParam (msParam_t *msParam);
int
clearMsParam (msParam_t *msParam, int freeStruct);
int
addMsParam (msParamArray_t *msParamArray, char *label,
char *packInstruct, void *inOutStruct, bytesBuf_t *inpOutBuf);
int
addIntParamToArray (msParamArray_t *msParamArray, char *label, int inpInt);
int
addMsParamToArray (msParamArray_t *msParamArray, char *label,
char *type, void *inOutStruct, bytesBuf_t *inpOutBuf, int replFlag);
int
replMsParamArray (msParamArray_t *msParamArray, 
msParamArray_t *outMsParamArray);
int
replMsParam (msParam_t *msParam, msParam_t *outMsParam);
int
replInOutStruct (void *inStruct, void **outStruct, char *type);
int
fillMsParam (msParam_t *msParam, char *label,
char *type, void *inOutStruct, bytesBuf_t *inpOutBuf);
msParam_t *
getMsParamByLabel (msParamArray_t *msParamArray, char *label);
msParam_t *
getMsParamByType (msParamArray_t *msParamArray, char *type);
int
rmMsParamByLabel (msParamArray_t *msParamArray, char *label, int freeStruct);
int
trimMsParamArray (msParamArray_t *msParamArray, char *outParamDesc);
int
printMsParam (msParamArray_t *msParamArray);
int
writeMsParam (char *buf, int len, msParam_t *msParam);
int
clearMsParamArray (msParamArray_t *msParamArray, int freeStruct);
int 
fillIntInMsParam (msParam_t *msParam, int myInt);
int
fillStrInMsParam (msParam_t *msParam, char *myStr);
int
fillBufLenInMsParam (msParam_t *msParam, int myInt, bytesBuf_t *bytesBuf);
int
parseMspForDataObjInp (msParam_t *inpParam, dataObjInp_t *dataObjInpCache, 
dataObjInp_t **outDataObjInp, int writeToCache);
int
parseMspForCollInp (msParam_t *inpParam, collInp_t *collInpCache,
collInp_t **outCollInp, int writeToCache);
int
parseMspForCondInp (msParam_t *inpParam, keyValPair_t *condInput,
char *condKw);
int
parseMspForCondKw (msParam_t *inpParam, keyValPair_t *condInput);
int
parseMspForPhyPathReg (msParam_t *inpParam, keyValPair_t *condInput);
int
parseMspForPosInt (msParam_t *inpParam);
char *
parseMspForStr (msParam_t *inpParam);
int
parseMspForDataObjCopyInp (msParam_t *inpParam,
dataObjCopyInp_t *dataObjCopyInpCache, dataObjCopyInp_t **outDataObjCopyInp);
int
parseMspForExecCmdInp (msParam_t *inpParam,
execCmd_t *execCmdInpCache, execCmd_t **ouExecCmdInp);
void 
*getMspInOutStructByLabel (msParamArray_t *msParamArray, char *label);
int
getStdoutInExecCmdOut (msParam_t *inpExecCmdOut, char **outStr);
int
getStderrInExecCmdOut (msParam_t *inpExecCmdOut, char **outStr);
int
initParsedMsKeyValStr (char *inpStr, parsedMsKeyValStr_t *parsedMsKeyValStr);
int
clearParsedMsKeyValStr (parsedMsKeyValStr_t *parsedMsKeyValStr);
int
getNextKeyValFromMsKeyValStr (parsedMsKeyValStr_t *parsedMsKeyValStr);
int
parseMsKeyValStrForDataObjInp (msParam_t *inpParam, dataObjInp_t *dataObjInp,
char *hintForMissingKw, int validKwFlags, char **outBadKeyWd);
int
chkDataObjInpKw (char *keyWd, int validKwFlags);
int
parseMsKeyValStrForCollInp (msParam_t *inpParam, collInp_t *collInp,
char *hintForMissingKw, int validKwFlags, char **outBadKeyWd);
int
chkCollInpKw (char *keyWd, int validKwFlags);
int
addKeyValToMspStr (msParam_t *keyStr, msParam_t *valStr, 
msParam_t *msKeyValStr);
int
chkStructFileExtAndRegInpKw (char *keyWd, int validKwFlags);
int
parseMsKeyValStrForStructFileExtAndRegInp (msParam_t *inpParam,
structFileExtAndRegInp_t *structFileExtAndRegInp,
char *hintForMissingKw, int validKwFlags, char **outBadKeyWd);
#ifdef  __cplusplus
}
#endif

#endif	/* MS_PARAM_H */
