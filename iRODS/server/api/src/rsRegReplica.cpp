/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "regReplica.h"
#include "objMetaOpr.hpp" // JMC - backport 4497
#include "icatHighLevelRoutines.hpp"

#include "irods_file_object.hpp"

int _call_file_modified_for_replica(
    rsComm_t*     rsComm,
    regReplica_t* regReplicaInp );

int
rsRegReplica( rsComm_t *rsComm, regReplica_t *regReplicaInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *srcDataObjInfo;

    srcDataObjInfo = regReplicaInp->srcDataObjInfo;

    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )srcDataObjInfo->objPath,
                 &rodsServerHost );
    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsRegReplica( rsComm, regReplicaInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcRegReplica( rodsServerHost->conn, regReplicaInp );
        if ( status >= 0 ) {
            regReplicaInp->destDataObjInfo->replNum = status;
        }

    }

    if ( status >= 0 ) {
        status = _call_file_modified_for_replica( rsComm, regReplicaInp );
    }

    return status;
}

int
_rsRegReplica( rsComm_t *rsComm, regReplica_t *regReplicaInp ) {
#ifdef RODS_CAT
    int status;
    dataObjInfo_t *srcDataObjInfo;
    dataObjInfo_t *destDataObjInfo;
    int savedClientAuthFlag;

    srcDataObjInfo = regReplicaInp->srcDataObjInfo;
    destDataObjInfo = regReplicaInp->destDataObjInfo;
    if ( getValByKey( &regReplicaInp->condInput, SU_CLIENT_USER_KW ) != NULL ) {
        savedClientAuthFlag = rsComm->clientUser.authInfo.authFlag;
        rsComm->clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
        status = chlRegReplica( rsComm, srcDataObjInfo, destDataObjInfo,
                                &regReplicaInp->condInput );
        /* restore it */
        rsComm->clientUser.authInfo.authFlag = savedClientAuthFlag;
    }
    else {
        status = chlRegReplica( rsComm, srcDataObjInfo, destDataObjInfo, &regReplicaInp->condInput );
        if ( status >= 0 ) {
            status = destDataObjInfo->replNum;
        }
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4608
    if ( status == CAT_SUCCESS_BUT_WITH_NO_INFO ||
            status == CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) { // JMC - backport 4668, 4670
        int status2;
        /* register a repl with a copy with the same resource and phyPaht.
          * could be caused by 2 staging at the same time */
        status2 = checkDupReplica( rsComm, srcDataObjInfo->dataId,
                                   destDataObjInfo->rescName,
                                   destDataObjInfo->filePath );
        if ( status2 >= 0 ) {
            destDataObjInfo->replNum = status2; // JMC - backport 4668
            destDataObjInfo->dataId = srcDataObjInfo->dataId;
            return status2;
        }
    }
    // =-=-=-=-=-=-=-
    return status;
#else
    return SYS_NO_RCAT_SERVER_ERR;
#endif
}


int _call_file_modified_for_replica(
    rsComm_t*     rsComm,
    regReplica_t* regReplicaInp ) {
    int status = 0;
    dataObjInfo_t* destDataObjInfo = regReplicaInp->destDataObjInfo;

    irods::file_object_ptr file_obj(
        new irods::file_object(
            rsComm,
            destDataObjInfo ) );

    char* pdmo_kw = getValByKey( &regReplicaInp->condInput, IN_PDMO_KW );
    if ( pdmo_kw != NULL ) {
        file_obj->in_pdmo( pdmo_kw );
    }

    irods::error ret = fileModified( rsComm, file_obj );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to signal resource that the data object \"";
        msg << destDataObjInfo->objPath;
        msg << "\" was registered";
        ret = PASSMSG( msg.str(), ret );
        irods::log( ret );
        status = ret.code();
    }

    return status;

} // _call_file_modified_for_replica

