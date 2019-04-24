#ifndef IRODS_RE_STRUCTS_HPP
#define IRODS_RE_STRUCTS_HPP

#include "rodsUser.h"
#include "rods.h"
#include "msParam.h"
#include "reDefines.h"
#include "reconstants.hpp"
#include "boost/any.hpp"

#include <list>

#define MAX_NUM_OF_ARGS_IN_ACTION 20

#define FREE_MS_PARAM	0x1
#define FREE_DOINP	0x2

typedef struct RuleExecInfo {
    int status;
    char statusStr[MAX_NAME_LEN];
    char ruleName[NAME_LEN];	/* name of rule */
    rsComm_t *rsComm;
    char pluginInstanceName[MAX_NAME_LEN];
    msParamArray_t *msParamArray;
    msParamArray_t inOutMsParamArray;
    int l1descInx;
    dataObjInp_t *doinp;	/* data object type input */
    dataObjInfo_t *doi;
    char rescName[NAME_LEN]; // replaces rgi above
    userInfo_t *uoic;  /* client XXXX should get this from rsComm->clientUser */
    userInfo_t *uoip;  /* proxy XXXX should get this from rsComm->proxyUser */
    collInfo_t *coi;
    userInfo_t *uoio;     /* other user info */
    keyValPair_t *condInputData;
    /****        IF YOU ARE MAKING CHANGES CHECK BELOW
                 OR ABOVE FOR IMPORTANT INFORMATION  ****/
    char ruleSet[RULE_SET_DEF_LENGTH];
    struct RuleExecInfo *next;
} ruleExecInfo_t;

typedef struct ReArg {
    int myArgc;
    char **myArgv;
} reArg_t;

typedef struct RuleExecInfoAndArg {
    ruleExecInfo_t *rei;
    reArg_t reArg;
} ruleExecInfoAndArg_t;

int applyRule(
    char *inAction,
    msParamArray_t *inMsParamArray,
    ruleExecInfo_t *rei,
    int reiSaveFlag );

int applyRuleWithInOutVars(
    const char*            _action,
    std::list<boost::any>& _params,
    ruleExecInfo_t*        _rei );

void freeCmdExecOut( execCmdOut_t *ruleExecOut );

int applyRule( char *inAction, msParamArray_t *inMsParamArray,
               ruleExecInfo_t *rei, int reiSaveFlag );
int applyRuleForPostProcForRead( rsComm_t *rsComm, bytesBuf_t *dataObjReadOutBBuf, char *objPath );
int applyRuleForPostProcForWrite( rsComm_t *rsComm, bytesBuf_t *dataObjWriteOutBBuf, char *objPath );
int applyRuleArg( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc,
                  ruleExecInfo_t *rei, int reiSaveFlag );

int initReiWithDataObjInp( ruleExecInfo_t *rei, rsComm_t *rsComm,
                           dataObjInp_t *dataObjIn );
int initReiWithCollInp( ruleExecInfo_t *rei, rsComm_t *rsComm,
                        collInp_t *collCreateInp, collInfo_t *collInfo );


int _writeString( char *writeId, char *writeStr, ruleExecInfo_t *rei );
int writeString( msParam_t* where, msParam_t* inString, ruleExecInfo_t *rei );

int
unpackRei( rsComm_t *rsComm, ruleExecInfo_t **rei,
           bytesBuf_t *packedReiBBuf );
int
packReiAndArg( ruleExecInfo_t *rei, char *myArgv[],
               int myArgc, bytesBuf_t **packedReiAndArgBBuf );
int
unpackReiAndArg( rsComm_t *rsComm, ruleExecInfoAndArg_t **reiAndArg,
                 bytesBuf_t *packedReiAndArgBBuf );

int copyRuleExecInfo( ruleExecInfo_t *from, ruleExecInfo_t *to );

int freeRuleExecInfoStruct( ruleExecInfo_t *rs, int freeSpecialStructFlag );

int freeRuleExecInfoInternals( ruleExecInfo_t *rs, int freeSpecialStructFlag );

int copyDataObjInfo( dataObjInfo_t *from, dataObjInfo_t *to );

int copyCollInfo( collInfo_t *from, collInfo_t *to );

int freeCollInfo( collInfo_t *rs );

int copyUserInfo( userInfo_t *from, userInfo_t *to );

int freeUserInfo( userInfo_t *rs );

int copyKeyValPairStruct( keyValPair_t *from, keyValPair_t *to );

int freeKeyValPairStruct( keyValPair_t *rs );

void *mallocAndZero( int s );

#endif // IRODS_RE_STRUCTS_HPP
