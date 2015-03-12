/* For copyright information please refer to files in the COPYRIGHT directory
 */

#include "debug.hpp"
#include "stdio.hpp"
#include "time.hpp"
#include "parser.hpp"

#ifdef DEBUG
#include "re.hpp"
int GlobalAllRuleExecFlag = 0;

int GlobalREDebugFlag = 0;
int GlobalREAuditFlag = 0;

ruleStruct_t appRuleStrct, coreRuleStrct;
rulefmapdef_t appRuleFuncMapDef, coreRuleFuncMapDef;

int NumOfAction = 2;

microsdef_t MicrosTable[] = {
    {"print_hello_arg", 1, ( funcPtr )( void * )print},
    {"writeLineMS", 2, ( funcPtr )( void * )writeLine}
};

int print( msParam_t *str, ruleExecInfo_t *rei ) {
    printf( "%s", ( char * )str->inOutStruct );
    return 0;
}
int writeLine( msParam_t* out, msParam_t *str, ruleExecInfo_t *rei ) {

    printf( "%s\n", ( char * )str->inOutStruct );
    return 0;
}

char * getConfigDir() {
    return ".";
}

int msiMakeGenQuery( msParam_t* selectListStr, msParam_t* condStr, msParam_t* genQueryInpParam, ruleExecInfo_t *rei ) {
    genQueryInp_t *genQueryInp;

    /* parse selectListStr */
    if ( parseMspForStr( selectListStr ) == NULL ) {
        rodsLog( LOG_ERROR, "msiMakeGenQuery: input selectListStr is NULL." );
        return USER__NULL_INPUT_ERR;
    }


    /* parse condStr */
    if ( parseMspForStr( condStr ) == NULL ) {
        rodsLog( LOG_ERROR, "msiMakeGenQuery: input condStr is NULL." );
        return USER__NULL_INPUT_ERR;
    }


    /* The code below is partly taken from msiMakeQuery and msiExecStrCondQuery. There may be a better way to do this. */

    /* Generate raw SQL query string */
    rei->status = 0;

    /* allocate memory for genQueryInp */
    genQueryInp = ( genQueryInp_t* )malloc( sizeof( genQueryInp_t ) );
    memset( genQueryInp, 0, sizeof( genQueryInp_t ) );

    /* set up GenQueryInp */
    genQueryInp->maxRows = 0;
    genQueryInp->continueInx = 0;


    /* return genQueryInp through GenQueryInpParam */
    genQueryInpParam->type = strdup( GenQueryInp_MS_T );
    genQueryInpParam->inOutStruct = genQueryInp;



    return rei->status;

}


void
getNowStr( char *timeStr ) {
    time_t myTime;

    myTime = time( NULL );
    snprintf( timeStr, 15, "%011d", ( uint ) myTime );
}
int rsGeneralRowInsert( rsComm_t *rsComm, generalRowInsertInp_t *generalRowInsertInp ) {
    if ( strcmp( generalRowInsertInp->tableName, "versionRuleBase" ) == 0 ) {
        printf( "inserting into %s row (%s, %s)\n", generalRowInsertInp->tableName, generalRowInsertInp->arg1, generalRowInsertInp->arg2 );
    }
    else {
        printf( "inserting into %s row (%s, %s, %s, %s, %s, %s, %s, %s, %s)\n", generalRowInsertInp->tableName, generalRowInsertInp->arg1, generalRowInsertInp->arg2
                , generalRowInsertInp->arg3, generalRowInsertInp->arg4
                , generalRowInsertInp->arg5, generalRowInsertInp->arg6
                , generalRowInsertInp->arg7, generalRowInsertInp->arg8
                , generalRowInsertInp->arg9 );

    }
    return 0;
}
int
insertRulesIntoDBNew( char * baseName, RuleSet *ruleSet,
                      ruleExecInfo_t *rei ) {
    generalRowInsertInp_t generalRowInsertInp;
    char *ruleIdStr;
    int rc1, i;
    int  mapPriorityInt = 1;
    char mapPriorityStr[50];
    char myTime[50];

    getNowStr( myTime );

    /* Before inserting rules and its base map, we need to first version out the base map */
    generalRowInsertInp.tableName = "versionRuleBase";
    generalRowInsertInp.arg1 = baseName;
    generalRowInsertInp.arg2 = myTime;

    rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );

    for ( i = 0; i < ruleSet->len; i++ ) {
        RuleDesc *rd = ruleSet->rules[i];
        char *ruleType;
        switch ( rd->ruleType ) {
        case RK_FUNC:
            ruleType = "@FUNC";
            break;
        case RK_REL:
            ruleType = "@REL";
            break;
        case RK_DATA:
            ruleType = "@DATA";
            break;
        case RK_CONSTRUCTOR:
            ruleType = "@CONSTR";
            break;
        case RK_EXTERN:
            ruleType = "@EXTERN";
            break;
        }
        Node *ruleNode = rd->node;
        char ruleNameStr[MAX_RULE_LEN];
        char ruleCondStr[MAX_RULE_LEN];
        char ruleActionRecoveryStr[MAX_RULE_LEN];
        int s;
        char *p;
        p = ruleNameStr;
        s = MAX_RULE_LEN;
        ruleNameToString( &p, &s, 0, ruleNode->subtrees[0] );
        p = ruleCondStr;
        s = MAX_RULE_LEN;
        termToString( &p, &s, 0, MIN_PREC, ruleNode->subtrees[1], 0 );
        p = ruleActionRecoveryStr;
        s = MAX_RULE_LEN;
        actionsToString( &p, &s, 0, ruleNode->subtrees[2], ruleNode->subtrees[3] );
        Node *avu = lookupAVUFromMetadata( ruleNode->subtrees[4], "id" );
        if ( avu != NULL ) {
            ruleIdStr = avu->subtrees[1]->text;
        }
        else {
            ruleIdStr = "";
        }
        generalRowInsertInp.tableName = "ruleTable";
        generalRowInsertInp.arg1 = baseName;
        sprintf( mapPriorityStr, "%i", mapPriorityInt );
        mapPriorityInt++;
        generalRowInsertInp.arg2 = mapPriorityStr;
        generalRowInsertInp.arg3 = ruleNode->subtrees[0]->text;
        generalRowInsertInp.arg4 = ruleNameStr;
        generalRowInsertInp.arg5 = ruleCondStr;
        generalRowInsertInp.arg6 = ruleActionRecoveryStr;
        generalRowInsertInp.arg7 = ruleType;
        generalRowInsertInp.arg8 = ruleIdStr;
        generalRowInsertInp.arg9 = myTime;

        rc1 = rsGeneralRowInsert( rei->rsComm, &generalRowInsertInp );
    }

    return rc1;
}
int
readRuleSetFromDB( char *ruleBaseName, char *versionStr, RuleSet *ruleSet, ruleExecInfo_t *rei, rError_t *errmsg, Region *r ) {

    Env *env = newEnv( newHashTable2( 100, r ), NULL, NULL, r );
    char ruleStr[MAX_RULE_LEN * 4];
    //char *ruleBase = "base";
    //char *action   = "action";
    char *ruleHead = "testrule(*A, *B)";
    char *ruleCondition = "(true())";
    char *ruleAction    = "{writeLine(\"stdout\", \"*A, *B\");}";
    char *ruleRecovery  = "";
    if ( strlen( ruleRecovery ) == 0 ) {
        /* rulegen */
        if ( ruleAction[0] == '{' ) {
            snprintf( ruleStr, MAX_RULE_LEN * 4, "%s { on(%s) %s }", ruleHead, ruleCondition, ruleAction );
        }
        else {
            snprintf( ruleStr, MAX_RULE_LEN * 4, "%s = %s", ruleHead, ruleAction );
        }
    }
    else {
        snprintf( ruleStr, MAX_RULE_LEN * 4, "%s|%s|%s|%s", ruleHead, ruleCondition, ruleAction, ruleRecovery );
    }
    Pointer *p = newPointer2( ruleStr );
    int errloc;
    int errcode = parseRuleSet( p, ruleSet, env, &errloc, errmsg, r );
    deletePointer( p );
    if ( errcode < 0 ) {
        /* deleteEnv(env, 3); */
        return errcode;
    }
    /* deleteEnv(env, 3); */
    return 0;
}

char *
getValByKey( keyValPair_t *condInput, char *keyWord ) {
    int i;

    if ( condInput == NULL ) {
        return NULL;
    }

    for ( i = 0; i < condInput->len; i++ ) {
        if ( strcmp( condInput->keyWord[i], keyWord ) == 0 ) {
            return condInput->value[i];
        }
    }

    return NULL;
}

/* parseMspForDataObjInp - This is a rather convoluted subroutine because
 * it tries to parse DataObjInp that have different types of msParam
 * in inpParam and different modes of output.
 *
 * If outputToCache == 0 and inpParam is DataObjInp_MS_T, *outDataObjInp
 *    will be set to the pointer given by inpParam->inOutStruct.
 * If inpParam is STR_MS_T or KeyValPair_MS_T, regardles of the value of
 *    outputToCache, the dataObjInpCache will be used to contain the output
 *    if it is not NULL. Otherwise, one will be malloc'ed (be sure to free
 *    it after your are done).
 * If outputToCache == 1, the dataObjInpCache will be used to contain the
 *    output if it is not NULL. Otherwise, one will be malloc'ed (be sure to
 *    free it after your are done).
 */

int
parseMspForDataObjInp( msParam_t *inpParam, dataObjInp_t *dataObjInpCache,
                       dataObjInp_t **outDataObjInp, int outputToCache ) {
    *outDataObjInp = NULL;

    if ( inpParam == NULL ) {
        rodsLog( LOG_ERROR,
                 "parseMspForDataObjInp: input inpParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        /* str input */
        if ( dataObjInpCache == NULL ) {
            rodsLog( LOG_ERROR,
                     "parseMspForDataObjInp: input dataObjInpCache is NULL" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
        memset( dataObjInpCache, 0, sizeof( dataObjInp_t ) );
        *outDataObjInp = dataObjInpCache;
        if ( strcmp( ( char * ) inpParam->inOutStruct, "null" ) != 0 ) {
            rstrcpy( dataObjInpCache->objPath, ( char* )inpParam->inOutStruct,
                     MAX_NAME_LEN );
        }
        return 0;
    }
    else if ( strcmp( inpParam->type, DataObjInp_MS_T ) == 0 ) {
        if ( outputToCache == 1 ) {
            dataObjInp_t *tmpDataObjInp;
            tmpDataObjInp = ( dataObjInp_t * )inpParam->inOutStruct;
            if ( dataObjInpCache == NULL ) {
                rodsLog( LOG_ERROR,
                         "parseMspForDataObjInp: input dataObjInpCache is NULL" );
                return SYS_INTERNAL_NULL_INPUT_ERR;
            }
            *dataObjInpCache = *tmpDataObjInp;
            /* zero out the condition of the original because it has been
             * moved */
            memset( &tmpDataObjInp->condInput, 0, sizeof( keyValPair_t ) );
            *outDataObjInp = dataObjInpCache;
        }
        else {
            *outDataObjInp = ( dataObjInp_t * ) inpParam->inOutStruct;
        }
        return 0;
    }
    else if ( strcmp( inpParam->type, KeyValPair_MS_T ) == 0 ) {
        /* key-val pair input needs ketwords "DATA_NAME" and  "COLL_NAME" */
        char *dVal, *cVal;
        keyValPair_t *kW;
        kW = ( keyValPair_t * )inpParam->inOutStruct;
        if ( ( dVal = getValByKey( kW, "DATA_NAME" ) ) == NULL ) {
            return USER_PARAM_TYPE_ERR;
        }
        if ( ( cVal = getValByKey( kW, "COLL_NAME" ) ) == NULL ) {
            return USER_PARAM_TYPE_ERR;
        }

        if ( dataObjInpCache == NULL ) {
            rodsLog( LOG_ERROR,
                     "parseMspForDataObjInp: input dataObjInpCache is NULL" );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }

        memset( dataObjInpCache, 0, sizeof( dataObjInp_t ) );
        snprintf( dataObjInpCache->objPath, MAX_NAME_LEN, "%s/%s", cVal, dVal );
        *outDataObjInp = dataObjInpCache;
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "parseMspForDataObjInp: Unsupported input Param1 type %s",
                 inpParam->type );
        return USER_PARAM_TYPE_ERR;
    }
}

/* parseMspForCollInp - see the explanation given for parseMspForDataObjInp
 */

int
parseMspForCollInp( msParam_t *inpParam, collInp_t *collInpCache,
                    collInp_t **outCollInp, int outputToCache ) {
    *outCollInp = NULL;

    if ( inpParam == NULL ) {
        rodsLog( LOG_ERROR,
                 "parseMspForCollInp: input inpParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        /* str input */
        if ( collInpCache == NULL ) {
            collInpCache = ( collInp_t * )malloc( sizeof( collInp_t ) );
        }
        memset( collInpCache, 0, sizeof( collInp_t ) );
        *outCollInp = collInpCache;
        if ( strcmp( ( char * ) inpParam->inOutStruct, "null" ) != 0 ) {
            rstrcpy( collInpCache->collName, ( char* )inpParam->inOutStruct,
                     MAX_NAME_LEN );
        }
        return 0;
    }
    else if ( strcmp( inpParam->type, CollInp_MS_T ) == 0 ) {
        if ( outputToCache == 1 ) {
            collInp_t *tmpCollInp;
            tmpCollInp = ( collInp_t * )inpParam->inOutStruct;
            if ( collInpCache == NULL ) {
                collInpCache = ( collInp_t * )malloc( sizeof( collInp_t ) );
            }
            *collInpCache = *tmpCollInp;
            /* zero out the condition of the original because it has been
             * moved */
            memset( &tmpCollInp->condInput, 0, sizeof( keyValPair_t ) );
            *outCollInp = collInpCache;
        }
        else {
            *outCollInp = ( collInp_t * ) inpParam->inOutStruct;
        }
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "parseMspForCollInp: Unsupported input Param1 type %s",
                 inpParam->type );
        return USER_PARAM_TYPE_ERR;
    }
}

int
parseMspForDataObjCopyInp( msParam_t *inpParam,
                           dataObjCopyInp_t *dataObjCopyInpCache, dataObjCopyInp_t **outDataObjCopyInp ) {
    *outDataObjCopyInp = NULL;
    if ( inpParam == NULL ) {
        rodsLog( LOG_ERROR,
                 "parseMspForDataObjCopyInp: input inpParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        /* str input */
        memset( dataObjCopyInpCache, 0, sizeof( dataObjCopyInp_t ) );
        rstrcpy( dataObjCopyInpCache->srcDataObjInp.objPath,
                 ( char * ) inpParam->inOutStruct, MAX_NAME_LEN );
        *outDataObjCopyInp = dataObjCopyInpCache;
    }
    else if ( strcmp( inpParam->type, DataObjCopyInp_MS_T ) == 0 ) {
        *outDataObjCopyInp = ( dataObjCopyInp_t* )inpParam->inOutStruct;
    }
    else if ( strcmp( inpParam->type, DataObjInp_MS_T ) == 0 ) {
        memset( dataObjCopyInpCache, 0, sizeof( dataObjCopyInp_t ) );
        dataObjCopyInpCache->srcDataObjInp =
            *( dataObjInp_t * )inpParam->inOutStruct;
        *outDataObjCopyInp = dataObjCopyInpCache;
    }
    else if ( strcmp( inpParam->type, KeyValPair_MS_T ) == 0 ) {
        /* key-val pair input needs ketwords "DATA_NAME" and  "COLL_NAME" */
        char *dVal, *cVal;
        keyValPair_t *kW;
        kW = ( keyValPair_t * )inpParam->inOutStruct;
        if ( ( dVal = getValByKey( kW, "DATA_NAME" ) ) == NULL ) {
            return USER_PARAM_TYPE_ERR;
        }
        if ( ( cVal = getValByKey( kW, "COLL_NAME" ) ) == NULL ) {
            return USER_PARAM_TYPE_ERR;
        }
        memset( dataObjCopyInpCache, 0, sizeof( dataObjCopyInp_t ) );
        snprintf( dataObjCopyInpCache->srcDataObjInp.objPath, MAX_NAME_LEN, "%s/%s", cVal, dVal );
        *outDataObjCopyInp = dataObjCopyInpCache;
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "parseMspForDataObjCopyInp: Unsupported input Param1 type %s",
                 inpParam->type );
        return USER_PARAM_TYPE_ERR;
    }
    return 0;
}

/* parseMspForCondInp - Take the inpParam->inOutStruct and use that as the
 * value of the keyVal pair. The KW of the keyVal pair is given in condKw.
 */

int
parseMspForCondInp( msParam_t *inpParam, keyValPair_t *condInput,
                    char *condKw ) {
    if ( inpParam != NULL ) {
        if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
            /* str input */
            if ( strcmp( ( char * ) inpParam->inOutStruct, "null" ) != 0 ) {
                addKeyVal( condInput, condKw,
                           ( char * ) inpParam->inOutStruct );
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "parseMspForCondInp: Unsupported input Param type %s",
                     inpParam->type );
            return USER_PARAM_TYPE_ERR;
        }
    }
    return 0;
}

/* parseMspForCondKw - Take the KW from inpParam->inOutStruct and add that
 * to condInput. The value of the keyVal  pair is assumed to be "".
 */

int
parseMspForCondKw( msParam_t *inpParam, keyValPair_t *condInput ) {
    if ( inpParam != NULL ) {
        if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
            /* str input */
            if ( strcmp( ( char * ) inpParam->inOutStruct, "null" ) != 0 &&
                    strlen( ( const char* )inpParam->inOutStruct ) > 0 ) {
                addKeyVal( condInput, ( char * ) inpParam->inOutStruct, "" );
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "parseMspForCondKw: Unsupported input Param type %s",
                     inpParam->type );
            return USER_PARAM_TYPE_ERR;
        }
    }
    return 0;
}

int
parseMspForPhyPathReg( msParam_t *inpParam, keyValPair_t *condInput ) {
    char *tmpStr;

    if ( inpParam != NULL ) {
        if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
            tmpStr = ( char * ) inpParam->inOutStruct;
            /* str input */
            if ( tmpStr != NULL && strlen( tmpStr ) > 0 &&
                    strcmp( tmpStr, "null" ) != 0 ) {
                if ( strcmp( tmpStr, COLLECTION_KW ) == 0 ) {
                    addKeyVal( condInput, COLLECTION_KW, "" );
                }
                else if ( strcmp( tmpStr, MOUNT_POINT_STR ) == 0 ) {
                    addKeyVal( condInput, COLLECTION_TYPE_KW, MOUNT_POINT_STR );
                }
                else if ( strcmp( tmpStr, LINK_POINT_STR ) == 0 ) {
                    addKeyVal( condInput, COLLECTION_TYPE_KW, LINK_POINT_STR );
                }
                else if ( strcmp( tmpStr, UNMOUNT_STR ) == 0 ) {
                    addKeyVal( condInput, COLLECTION_TYPE_KW, UNMOUNT_STR );
                }
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "parseMspForCondKw: Unsupported input Param type %s",
                     inpParam->type );
            return USER_PARAM_TYPE_ERR;
        }
    }
    return 0;
}

int
parseMspForPosInt( msParam_t *inpParam ) {
    int myInt;

    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        /* str input */
        if ( strcmp( ( char * ) inpParam->inOutStruct, "null" ) == 0 ) {
            return SYS_NULL_INPUT;
        }
        myInt = atoi( ( const char* )inpParam->inOutStruct );
    }
    else if ( strcmp( inpParam->type, INT_MS_T ) == 0 ||
              strcmp( inpParam->type, BUF_LEN_MS_T ) == 0 ) {
        myInt = *( int * )inpParam->inOutStruct;
    }
    else {
        rodsLog( LOG_ERROR,
                 "parseMspForPosInt: Unsupported input Param type %s",
                 inpParam->type );
        return USER_PARAM_TYPE_ERR;
    }
    if ( myInt < 0 ) {
        rodsLog( LOG_DEBUG,
                 "parseMspForPosInt: parsed int %d is negative", myInt );
    }
    return myInt;
}

char *
parseMspForStr( msParam_t *inpParam ) {
    if ( inpParam == NULL || inpParam->inOutStruct == NULL ) {
        return NULL;
    }

    if ( strcmp( inpParam->type, STR_MS_T ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "parseMspForStr: inpParam type %s is not STR_MS_T",
                 inpParam->type );
    }

    if ( strcmp( ( char * ) inpParam->inOutStruct, "null" ) == 0 ) {
        return NULL;
    }

    return ( char * )( inpParam->inOutStruct );
}

int
parseMspForExecCmdInp( msParam_t *inpParam,
                       execCmd_t *execCmdInpCache, execCmd_t **ouExecCmdInp ) {
    *ouExecCmdInp = NULL;
    if ( inpParam == NULL ) {
        rodsLog( LOG_ERROR,
                 "parseMspForExecCmdInp: input inpParam is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strcmp( inpParam->type, STR_MS_T ) == 0 ) {
        /* str input */
        memset( execCmdInpCache, 0, sizeof( execCmd_t ) );
        rstrcpy( execCmdInpCache->cmd,
                 ( char * ) inpParam->inOutStruct, LONG_NAME_LEN );
        *ouExecCmdInp = execCmdInpCache;
    }
    else if ( strcmp( inpParam->type, ExecCmd_MS_T ) == 0 ) {
        *ouExecCmdInp = ( execCmd_t* )inpParam->inOutStruct;
    }
    else {
        rodsLog( LOG_ERROR,
                 "parseMspForExecCmdInp: Unsupported input Param1 type %s",
                 inpParam->type );
        return USER_PARAM_TYPE_ERR;
    }
    return 0;
}

int addRErrorMsg( rError_t *myError, int status, char *msg ) {
    rErrMsg_t **newErrMsg;
    int newLen;
    int i;

    if ( myError == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ( myError->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newLen = myError->len + PTR_ARRAY_MALLOC_LEN;
        newErrMsg = ( rErrMsg_t ** ) malloc( newLen * sizeof( newErrMsg ) );
        memset( newErrMsg, 0, newLen * sizeof( newErrMsg ) );
        for ( i = 0; i < myError->len; i++ ) {
            newErrMsg[i] = myError->errMsg[i];
        }
        if ( myError->errMsg != NULL ) {
            free( myError->errMsg );
        }
        myError->errMsg = newErrMsg;
    }

    myError->errMsg[myError->len] = ( rErrMsg_t* )malloc( sizeof( rErrMsg_t ) );
    strncpy( myError->errMsg[myError->len]->msg, msg, ERR_MSG_LEN - 1 );
    myError->errMsg[myError->len]->status = status;
    myError->len++;

    return 0;
}

int
freeRErrorContent( rError_t *myError ) {
    int i;

    if ( myError == NULL ) {
        return 0;
    }

    if ( myError->len > 0 ) {
        for ( i = 0; i < myError->len; i++ ) {
            free( myError->errMsg[i] );
        }
        free( myError->errMsg );
    }

    memset( myError, 0, sizeof( rError_t ) );

    return 0;
}

int
clearMsParam( msParam_t *msParam, int freeStruct ) {
    if ( msParam == NULL ) {
        return 0;
    }

    if ( msParam->label != NULL ) {
        free( msParam->label );
    }
    /* XXXXXX : need to free their internal struct too */
    /* free STR_MS_T too 7/8/10 mw */
    if ( msParam->inOutStruct != NULL && ( freeStruct > 0 ||
                                           ( msParam->type != NULL && strcmp( msParam->type, STR_MS_T ) == 0 ) ) ) {
        free( msParam->inOutStruct );
    }
    if ( msParam->type != NULL ) {
        free( msParam->type );
    }

    memset( msParam, 0, sizeof( msParam_t ) );
    return 0;
}
int
clearMsParamArray( msParamArray_t *msParamArray, int freeStruct ) {
    int i;

    if ( msParamArray == NULL ) {
        return 0;
    }

    for ( i = 0; i < msParamArray->len; i++ ) {
        clearMsParam( msParamArray->msParam[i], freeStruct );
        free( msParamArray->msParam[i] );
    }

    if ( msParamArray->len > 0 && msParamArray->msParam != NULL ) {
        free( msParamArray->msParam );
        memset( msParamArray, 0, sizeof( msParamArray_t ) );
    }

    return 0;
}
char* rstrcpy( char *dest, char *src, int n ) {
    strncpy( dest, src, n );
    return dest;
}
int rSplitStr( char *all, char *head, int headLen, char *tail, int tailLen, char sep ) {
    char *i = strchr( all, sep );
    if ( i == NULL ) {
        tail[0] = '\0';
        strcpy( head, all );
    }
    else {
        strcpy( tail, i + 1 );
        strncpy( head, all, i - all );
        head[i - all] = '\0';
    }
    return 0;
}

/* mock functions */
int reDebug( RuleEngineEvent label, int flag, RuleEngineEventParam *param, Node *node, Env *env, ruleExecInfo_t *rei ) {
    return 0;
}

int copyRuleExecInfo( ruleExecInfo_t *a, ruleExecInfo_t *b ) {
    return 0;
}
void *mallocAndZero( int size ) {
    return NULL;
}
int freeRuleExecInfoStruct( ruleExecInfo_t *rei, int i ) {
    return 0;
}

int addKeyVal( keyValPair_t *k, char * key, char *val ) {
    return 0;
}
int clearKeyVal( keyValPair_t *k ) {
    return 0;
}
char * getAttrNameFromAttrId( int id ) {
    return NULL;
}
int msiExecGenQuery( msParam_t* genQueryInParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei ) {
    return 0;
}
int _delayExec( char *inActionCall, char *recoveryActionCall,
                char *delayCondition,  ruleExecInfo_t *rei ) {
    return 0;
}

int chlSetAVUMetadata( rsComm_t *rsComm, char *type, char *ame, char *attr, char *value, char *unit ) {
    return 0;
}

int msiGetMoreRows( msParam_t* genQueryInpParam, msParam_t* genQueryOutParam, msParam_t *contInxParam, ruleExecInfo_t *rei ) {
    return 0;
}
int msiCloseGenQuery( msParam_t* genQueryInpParam, msParam_t* genQueryOutParam, ruleExecInfo_t *rei ) {
    return 0;
}


int rsOpenCollection( rsComm_t *rsComm, collInp_t *openCollInp ) {
    return 0;
}
int rsCloseCollection( rsComm_t *rsComm, int *handleInxInp )  {
    return 0;
}
int rsReadCollection( rsComm_t *rsComm, int *handleInxInp, collEnt_t **collEnt )  {
    return 0;
}

int
rsExecMyRule( rsComm_t *rsComm, execMyRuleInp_t *execMyRuleInp,
              msParamArray_t **outParamArray ) {
    return 0;
}
int
parseHostAddrStr( char *hostAddr, rodsHostAddr_t *addr ) {
    return 0;
}

int
checkStringForSystem( char *inString ) {
    return 0;
}

int
addInxIval( inxIvalPair_t *inxIvalPair, int inx, int value ) {
    return 0;
}
int
addInxVal( inxValPair_t *inxValPair, int inx, char *value ) {
    return 0;
}

int
rsGenQuery( rsComm_t *rsComm, genQueryInp_t *genQueryInp,
            genQueryOut_t **genQueryOut ) {
    return 0;
}

sqlResult_t *
getSqlResultByInx( genQueryOut_t *genQueryOut, int attriInx ) {
    return NULL;
}

int
freeGenQueryOut( genQueryOut_t **genQueryOut ) {
    return 0;
}

int
clearGenQueryInp( genQueryInp_t *gqinp ) {
    return 0;
}

int getDataObjInfoIncSpecColl( rsComm_t *rsComm, dataObjInp_t *doinp, dataObjInfo_t **doi ) {
    return 0;
}

int trimWS( char *s ) {
    return 0;
}

rulevardef_t coreRuleVarDef;
rulevardef_t appRuleVarDef;

int
replErrorStack( rError_t *srcRError, rError_t *destRError ) {
    int i, len;
    rErrMsg_t *errMsg;

    if ( srcRError == NULL || destRError == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    len = srcRError->len;

    for ( i = 0; i < len; i++ ) {
        errMsg = srcRError->errMsg[i];
        addRErrorMsg( destRError, errMsg->status, errMsg->msg );
    }
    return 0;
}

int freeAllDataObjInfo( dataObjInfo_t *dataObjInfoHead ) {
    return 0;
}



#endif

