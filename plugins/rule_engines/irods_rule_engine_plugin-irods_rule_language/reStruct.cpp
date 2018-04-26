/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "reGlobalsExtern.hpp"
#include "reFuncDefs.hpp"
#include "rcMisc.h"
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

    if ( from->doi != nullptr ) {
        to->doi = ( dataObjInfo_t * ) mallocAndZero( sizeof( dataObjInfo_t ) );
        copyDataObjInfo( from->doi, to->doi );
    }
    else {
        to->doi = nullptr;
    }

    if ( from->uoic != nullptr ) {
        to->uoic = ( userInfo_t* )mallocAndZero( sizeof( userInfo_t ) );
        copyUserInfo( from->uoic, to->uoic );
    }
    else {
        to->uoic = nullptr;
    }

    if ( from->uoip != nullptr ) {
        to->uoip = ( userInfo_t* )mallocAndZero( sizeof( userInfo_t ) );
        copyUserInfo( from->uoip, to->uoip );
    }
    else {
        to->uoip = nullptr;
    }

    if ( from->coi != nullptr ) {
        to->coi = ( collInfo_t* )mallocAndZero( sizeof( collInfo_t ) );
        copyCollInfo( from->coi, to->coi );
    }
    else {
        to->coi = nullptr;
    }

    if ( from->uoio != nullptr ) {
        to->uoio = ( userInfo_t* )mallocAndZero( sizeof( userInfo_t ) );
        copyUserInfo( from->uoio, to->uoio );
    }
    else {
        to->uoio = nullptr;
    }

    if ( from->condInputData != nullptr ) {
        to->condInputData = ( keyValPair_t* )mallocAndZero( sizeof( keyValPair_t ) );
        copyKeyValPairStruct( from->condInputData, to->condInputData );
    }
    else {
        to->condInputData = nullptr;
    }

    if ( from->next != nullptr ) {
        to->next = ( ruleExecInfo_t* )mallocAndZero( sizeof( ruleExecInfo_t ) );
        copyRuleExecInfo( from->next, to->next );
    }
    else {
        to->next = nullptr;
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
    if ( rs->msParamArray != nullptr && ( freeSpeialStructFlag & FREE_MS_PARAM ) > 0 ) {
        clearMsParamArray( rs->msParamArray, 1 );
        free( rs->msParamArray );
    }

    if ( rs->doinp != nullptr && ( freeSpeialStructFlag & FREE_DOINP ) > 0 ) {
        clearDataObjInp( rs->doinp );
        free( rs->doinp );
    }

    if ( rs->doi != nullptr ) {
        freeAllDataObjInfo( rs->doi );
    }
    if ( rs->uoic != nullptr ) {
        freeUserInfo( rs->uoic );
    }
    if ( rs->uoip != nullptr ) {
        freeUserInfo( rs->uoip );
    }
    if ( rs->coi != nullptr ) {
        freeCollInfo( rs->coi );
    }
    if ( rs->uoio != nullptr ) {
        freeUserInfo( rs->uoio );
    }
    if ( rs->condInputData != nullptr ) {
        freeKeyValPairStruct( rs->condInputData );
    }
    if ( rs->next != nullptr ) {
        freeRuleExecInfoStruct( rs->next, freeSpeialStructFlag );
    }
    return 0;
}

int
copyDataObjInfo( dataObjInfo_t *from, dataObjInfo_t *to ) {
    *to = *from;
    if ( from->next != nullptr ) {
        to->next = ( dataObjInfo_t* )mallocAndZero( sizeof( dataObjInfo_t ) );
        copyDataObjInfo( from->next, to->next );
    }
    else {
        to->next = nullptr;
    }
    return 0;
}


int
copyCollInfo( collInfo_t *from, collInfo_t *to ) {
    *to = *from;
    if ( from->next != nullptr ) {
        to->next = ( collInfo_t* )mallocAndZero( sizeof( collInfo_t ) );
        copyCollInfo( from->next, to->next );
    }
    else {
        to->next = nullptr;
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
    if ( rs->next != nullptr ) {
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
packRei( ruleExecInfo_t *rei,
         bytesBuf_t **packedReiBBuf ) {
    int status;

    if ( packedReiBBuf == nullptr ) {
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
    if ( rei == nullptr || rei->msParamArray == nullptr ) {
        return 0;
    }
    /* need to use 0 on delStruct flag or core dump in when called by
     * finalizeMsParamNew */
    clearMsParamArray( rei->msParamArray, 0 );
    free( rei->msParamArray );

    rei->msParamArray = nullptr;

    return 0;
}

