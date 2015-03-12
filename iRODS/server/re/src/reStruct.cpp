/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.hpp"
#include "reFuncDefs.hpp"
#include "rcMisc.hpp"
#include "objMetaOpr.hpp"
#include "resource.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"


void *mallocAndZero( int s ) {
    void *t;
    t = malloc( s );
    memset( t, 0, s );
    return t;
}

int
copyRuleExecInfo( ruleExecInfo_t *from, ruleExecInfo_t *to ) {

    ( *to ) = ( *from );

    /****
         The following structures are not copied and just the pointer is copied.

         dataObjInp_t *doinp;
         dataOprInp_t *dinp;
         fileOpenInp_t *finp;
         dataObjInp_t *doinpo;
         dataOprInp_t *dinpo;
         fileOpenInp_t *finpo;


    ***/
    if ( strlen( from->pluginInstanceName ) != 0 ) {
        rstrcpy( to->pluginInstanceName, from->pluginInstanceName, MAX_NAME_LEN );
    }

    if ( from->doi != NULL ) {
        to->doi = ( dataObjInfo_t * ) mallocAndZero( sizeof( dataObjInfo_t ) );
        copyDataObjInfo( from->doi, to->doi );
    }
    else {
        to->doi = NULL;
    }

    if ( from->uoic != NULL ) {
        to->uoic = ( userInfo_t* )mallocAndZero( sizeof( userInfo_t ) );
        copyUserInfo( from->uoic, to->uoic );
    }
    else {
        to->uoic = NULL;
    }

    if ( from->uoip != NULL ) {
        to->uoip = ( userInfo_t* )mallocAndZero( sizeof( userInfo_t ) );
        copyUserInfo( from->uoip, to->uoip );
    }
    else {
        to->uoip = NULL;
    }

    if ( from->coi != NULL ) {
        to->coi = ( collInfo_t* )mallocAndZero( sizeof( collInfo_t ) );
        copyCollInfo( from->coi, to->coi );
    }
    else {
        to->coi = NULL;
    }

    if ( from->uoio != NULL ) {
        to->uoio = ( userInfo_t* )mallocAndZero( sizeof( userInfo_t ) );
        copyUserInfo( from->uoio, to->uoio );
    }
    else {
        to->uoio = NULL;
    }

    if ( from->condInputData != NULL ) {
        to->condInputData = ( keyValPair_t* )mallocAndZero( sizeof( keyValPair_t ) );
        copyKeyValPairStruct( from->condInputData, to->condInputData );
    }
    else {
        to->condInputData = NULL;
    }

    if ( from->next != NULL ) {
        to->next = ( ruleExecInfo_t* )mallocAndZero( sizeof( ruleExecInfo_t ) );
        copyRuleExecInfo( from->next, to->next );
    }
    else {
        to->next = NULL;
    }
    return 0;
}

int
freeRuleExecInfoStruct( ruleExecInfo_t *rs, int freeSpeialStructFlag ) {
    freeRuleExecInfoInternals( rs, freeSpeialStructFlag );
    free( rs );
    return 0;
}
int
freeRuleExecInfoInternals( ruleExecInfo_t *rs, int freeSpeialStructFlag ) {
    if ( rs->msParamArray != NULL && ( freeSpeialStructFlag & FREE_MS_PARAM ) > 0 ) {
        clearMsParamArray( rs->msParamArray, 1 );
        free( rs->msParamArray );
    }

    if ( rs->doinp != NULL && ( freeSpeialStructFlag & FREE_DOINP ) > 0 ) {
        clearDataObjInp( rs->doinp );
        free( rs->doinp );
    }

    if ( rs->doi != NULL ) {
        freeAllDataObjInfo( rs->doi );
    }
    if ( rs->uoic != NULL ) {
        freeUserInfo( rs->uoic );
    }
    if ( rs->uoip != NULL ) {
        freeUserInfo( rs->uoip );
    }
    if ( rs->coi != NULL ) {
        freeCollInfo( rs->coi );
    }
    if ( rs->uoio != NULL ) {
        freeUserInfo( rs->uoio );
    }
    if ( rs->condInputData != NULL ) {
        freeKeyValPairStruct( rs->condInputData );
    }
    if ( rs->next != NULL ) {
        freeRuleExecInfoStruct( rs->next, freeSpeialStructFlag );
    }
    return 0;
}

int
copyDataObjInfo( dataObjInfo_t *from, dataObjInfo_t *to ) {
    *to = *from;
    if ( from->next != NULL ) {
        to->next = ( dataObjInfo_t* )mallocAndZero( sizeof( dataObjInfo_t ) );
        copyDataObjInfo( from->next, to->next );
    }
    else {
        to->next = NULL;
    }
    return 0;
}


int
copyCollInfo( collInfo_t *from, collInfo_t *to ) {
    *to = *from;
    if ( from->next != NULL ) {
        to->next = ( collInfo_t* )mallocAndZero( sizeof( collInfo_t ) );
        copyCollInfo( from->next, to->next );
    }
    else {
        to->next = NULL;
    }
    return 0;
}


#if 0	// #1472
int
copyRescGrpInfo( rescGrpInfo_t *from, rescGrpInfo_t *to ) {
    *to = *from;
    if ( from->next != NULL ) {
        to->next = ( rescGrpInfo_t* )mallocAndZero( sizeof( rescGrpInfo_t ) );
        copyRescGrpInfo( from->next, to->next );
    }
    else {
        to->next = NULL;
    }
    return 0;
}

int
freeRescGrpInfo( rescGrpInfo_t *rs ) {
    if ( rs->next != NULL ) {
        freeRescGrpInfo( rs->next );
    }
    if ( rs->rescInfo ) {
        delete rs->rescInfo;
    }
    free( rs );
    return 0;
}
#endif


int
freeCollInfo( collInfo_t *rs ) {
    if ( rs->next != NULL ) {
        freeCollInfo( rs->next );
    }
    free( rs );
    return 0;
}


int
copyUserInfo( userInfo_t *from, userInfo_t *to ) {
    *to = *from;
    to->authInfo = from->authInfo;
    to->userOtherInfo = from->userOtherInfo;
    return 0;
}


int
freeUserInfo( userInfo_t *rs ) {
    free( rs );
    return 0;
}


#if 0 // #1472
int
copyRescInfo( rescInfo_t *from, rescInfo_t *to ) {
    *to = *from;
    return 0;
}


int
freeRescInfo( rescInfo_t *rs ) {
    free( rs );
    return 0;
}
#endif


int
copyKeyValPairStruct( keyValPair_t *from, keyValPair_t *to ) {
    *to = *from;
    return 0;
}

int
freeKeyValPairStruct( keyValPair_t *rs ) {
    free( rs );
    return 0;
}



int
zeroRuleExecInfoStruct( ruleExecInfo_t *rei ) {

    memset( rei, 0, sizeof( ruleExecInfo_t ) );
    return 0;
}

int
initReiWithDataObjInp( ruleExecInfo_t *rei, rsComm_t *rsComm,
                       dataObjInp_t *dataObjInp ) {
    memset( rei, 0, sizeof( ruleExecInfo_t ) );
    rei->doinp = dataObjInp;
    rei->rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei->uoic = &rsComm->clientUser;
        rei->uoip = &rsComm->proxyUser;
    }

    return 0;
}

int
initReiWithCollInp( ruleExecInfo_t *rei, rsComm_t *rsComm,
                    collInp_t *collCreateInp, collInfo_t *collInfo ) {
    int status;

    memset( rei, 0, sizeof( ruleExecInfo_t ) );
    memset( collInfo, 0, sizeof( collInfo_t ) );
    rei->coi = collInfo;
    /* try to fill out as much info in collInfo as possible */
    if ( ( status = splitPathByKey( collCreateInp->collName,
                                    collInfo->collParentName, MAX_NAME_LEN, collInfo->collName, MAX_NAME_LEN, '/' ) ) < 0 ) {
        rodsLog( LOG_ERROR,
                 "initReiWithCollInp: splitPathByKey for %s error, status = %d",
                 collCreateInp->collName, status );
        return status;
    }
    else {
        rstrcpy( collInfo->collName, collCreateInp->collName, MAX_NAME_LEN );
    }
    rei->rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei->uoic = &rsComm->clientUser;
        rei->uoip = &rsComm->proxyUser;
    }

    return 0;
}

int
packRei( ruleExecInfo_t *rei,
         bytesBuf_t **packedReiBBuf ) {
    int status;

    if ( packedReiBBuf == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* pack the rei */

    status = packStruct( ( void * ) rei, packedReiBBuf,
                         "Rei_PI", RodsPackTable, 0, NATIVE_PROT );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "packRei: packStruct error. status = %d", status );
        return status;
    }

    return status;
}

int
unpackRei( rsComm_t *rsComm, ruleExecInfo_t **rei,
           bytesBuf_t *packedReiBBuf ) {
    int status;

    if ( packedReiBBuf == NULL || rei == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* unpack the rei */

    /* alway use NATIVE_PROT for rei */
    status = unpackStruct( packedReiBBuf->buf, ( void ** ) rei,
                           "Rei_PI", RodsPackTable, NATIVE_PROT );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unpackRei: unpackStruct error. status = %d", status );
        return status;
    }

    /* try to touch up the unpacked rei */

    status = touchupPackedRei( rsComm, *rei );

    return status;
}

int
packReiAndArg( ruleExecInfo_t *rei, char *myArgv[],
               int myArgc, bytesBuf_t **packedReiAndArgBBuf ) {
    int status;
    ruleExecInfoAndArg_t reiAndArg;

    if ( packedReiAndArgBBuf == NULL ) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: NULL packedReiAndArgBBuf input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( myArgc > 0 && ( myArgv == NULL || *myArgv == NULL ) ) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: NULL myArgv input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    memset( &reiAndArg, 0, sizeof( reiAndArg ) );

    reiAndArg.rei = rei;

    reiAndArg.reArg.myArgc = myArgc;
    reiAndArg.reArg.myArgv = myArgv;

    /* pack the reiAndArg */

    status = packStruct( ( void * ) &reiAndArg, packedReiAndArgBBuf,
                         "ReiAndArg_PI", RodsPackTable, 0, NATIVE_PROT );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "packReiAndArg: packStruct error. status = %d", status );
        return status;
    }

    return status;
}

int
unpackReiAndArg( rsComm_t *rsComm, ruleExecInfoAndArg_t **reiAndArg,
                 bytesBuf_t *packedReiAndArgBBuf ) {
    int status;
    /*ruleExecInfo_t *myRei;*/

    if ( packedReiAndArgBBuf == NULL || reiAndArg == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* unpack the reiAndArg */

    status = unpackStruct( packedReiAndArgBBuf->buf, ( void ** ) reiAndArg,
                           "ReiAndArg_PI", RodsPackTable, NATIVE_PROT );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unpackReiAndArg: unpackStruct error. status = %d", status );
        return status;
    }

    /* try to touch up the unpacked rei */

    status = touchupPackedRei( rsComm, ( *reiAndArg )->rei );

    return status;
}

int
touchupPackedRei( rsComm_t *rsComm, ruleExecInfo_t *myRei ) {
    int status = 0;

    if ( myRei == NULL || rsComm == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( myRei->rsComm != NULL ) {
        free( myRei->rsComm );
    }

    myRei->rsComm = rsComm;
    /* copy the clientUser. proxyUser is assumed to be in rsComm already */
    rsComm->clientUser = *( myRei->uoic );

    if ( myRei->doi != NULL ) {
        if ( myRei->doi->next != NULL ) {
            free( myRei->doi->next );
            myRei->doi->next = NULL;
        }
    }

    return status;
}


int
copyTaggedValue( char *str, char *tag, char *buf, int bufLen ) {
    /*int i;*/
    char tVal[NAME_LEN];
    char *t, *s, *u;

    snprintf( tVal, NAME_LEN, "<%s>", tag );
    if ( ( t = strstr( str, tVal ) ) == NULL ) {
        if ( strcmp( tag, "KVALPR" ) ) {
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            while ( ( t = strstr( str, "<_____X>" ) ) != NULL ) {
                memcpy( t + 1, tag, 6 );
            }
        }
        return -1;
    }
    s = t + strlen( tVal );
    snprintf( tVal, NAME_LEN, "</%s>", tag );
    if ( ( u = strstr( str, tVal ) ) == NULL ) {
        return INPUT_ARG_NOT_WELL_FORMED_ERR;
    }
    *u = '\0';
    strncpy( buf, s, bufLen );
    *u = '<';
    if ( !strcmp( tag, "KVALPR" ) ) {
        memcpy( t + 1, "_____X", 6 );
    }
    return 0;

}
int
fillSubmitConditions( char *action, char *inDelayCondition,
                      bytesBuf_t *packedReiAndArgBBuf, ruleExecSubmitInp_t *ruleSubmitInfo,
                      ruleExecInfo_t *rei ) {
    int i;
    int j = 0;
    char kwp[NAME_LEN * 2];
    char *t, *s;
    char *delayCondition;

    delayCondition = strdup( inDelayCondition );
    snprintf( ruleSubmitInfo->ruleName, sizeof( ruleSubmitInfo->ruleName ), "%s", action );
    /*
      i= copyTaggedValue(delayCondition,"UN", ruleSubmitInfo->userName,NAME_LEN);
      if (i != 0 && i != UNMATCHED_KEY_OR_INDEX)  return(i);
    */
    i = copyTaggedValue( delayCondition, "EA", ruleSubmitInfo->exeAddress, NAME_LEN );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX )  {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    i = copyTaggedValue( delayCondition, "ET", ruleSubmitInfo->exeTime, TIME_LEN );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX ) {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    else if ( i == 0 ) {
        i  = checkDateFormat( ruleSubmitInfo->exeTime );
        if ( i != 0 ) {
            free( delayCondition ); // JMC cppcheck - leak
            return i;
        }

    }
    i = copyTaggedValue( delayCondition, "EF", ruleSubmitInfo->exeFrequency, NAME_LEN );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX ) {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    i = copyTaggedValue( delayCondition, "PRI", ruleSubmitInfo->priority, NAME_LEN );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX ) {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    i = copyTaggedValue( delayCondition, "EET", ruleSubmitInfo->estimateExeTime, NAME_LEN );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX ) {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    i = copyTaggedValue( delayCondition, "NA", ruleSubmitInfo->notificationAddr, NAME_LEN );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX ) {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    i = copyTaggedValue( delayCondition, "PLUSET", kwp, NAME_LEN * 2 );
    if ( i != 0 && i != UNMATCHED_KEY_OR_INDEX ) {
        free( delayCondition ); // JMC cppcheck - leak
        return i;
    }
    else if ( i == 0 ) {
        i  = checkDateFormat( kwp );
        if ( i != 0 ) {
            free( delayCondition );
            return i;
        }
        getOffsetTimeStr( ruleSubmitInfo->exeTime, kwp );
    }
    i = copyTaggedValue( delayCondition, "KVALPR", kwp, NAME_LEN * 2 );
    while ( i >= 0 ) {
        if ( ( t = strstr( kwp, "=" ) ) == NULL ) {
            free( delayCondition );
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }
        *t = '\0';
        s = t - 1;
        while ( *s == ' ' ) {
            s--;
        }
        *( s + 1 ) = '\0';
        ruleSubmitInfo->condInput.keyWord[j] = strdup( kwp );
        t++;
        while ( *t == ' ' ) {
            t++;
        }
        ruleSubmitInfo->condInput.value[j] = t;
        j++;
        i = copyTaggedValue( delayCondition, "KWVAL", kwp, NAME_LEN * 2 );
    }
    ruleSubmitInfo->condInput.len = j;
    ruleSubmitInfo->packedReiAndArgBBuf = packedReiAndArgBBuf;
    if ( strlen( ruleSubmitInfo->userName ) == 0 ) {
        if ( strlen( rei->uoic->userName ) != 0 ) {
            snprintf( ruleSubmitInfo->userName, sizeof( ruleSubmitInfo->userName ),
                      "%s", rei->uoic->userName );
        }
        else if ( strlen( rei->rsComm->clientUser.userName ) != 0 ) {
            snprintf( rei->rsComm->clientUser.userName, sizeof( rei->rsComm->clientUser.userName ),
                      "%s", rei->uoic->userName );
        }
    }
    free( delayCondition );
    return 0;
}


int
pushStack( strArray_t *strArray, char *value ) {

    return addStrArray( strArray, value );

}

int
popStack( strArray_t *strArray, char *value ) {

    if ( strArray->len <= 0 || strArray->size == 0 ) {
        rodsLog( LOG_ERROR,
                 "popStack: Stack is empty: invalid size %d, len %d",
                 strArray->size, strArray->len );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    rstrcpy( value, &strArray->value[( strArray->len - 1 ) * strArray->size], strArray->size );
    strArray->len--;
    return 0;

}

int
clearMsparamInRei( ruleExecInfo_t *rei ) {
    if ( rei == NULL || rei->msParamArray == NULL ) {
        return 0;
    }
    /* need to use 0 on delStruct flag or core dump in when called by
     * finalizeMsParamNew */
    clearMsParamArray( rei->msParamArray, 0 );
    free( rei->msParamArray );

    rei->msParamArray = NULL;

    return 0;
}

