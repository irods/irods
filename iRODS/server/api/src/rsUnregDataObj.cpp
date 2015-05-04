/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "unregDataObj.h"
#include "icatHighLevelRoutines.hpp"
#include "fileDriver.hpp"

#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"

int
rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *dataObjInfo;

    dataObjInfo = unregDataObjInp->dataObjInfo;

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )dataObjInfo->objPath,
                                 &rodsServerHost );
    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsUnregDataObj( rsComm, unregDataObjInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcUnregDataObj( rodsServerHost->conn, unregDataObjInp );
    }

    return status;
}

int
_rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp ) {
#ifdef RODS_CAT
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *condInput;
    int status;
    irods::error ret;

    condInput = unregDataObjInp->condInput;
    dataObjInfo = unregDataObjInp->dataObjInfo;

    status = chlUnregDataObj( rsComm, dataObjInfo, condInput );
    if ( status < 0 ) {
        char* sys_error;
        char* rods_error = rodsErrorName( status, &sys_error );
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Failed to unregister the data object \"";
        msg << dataObjInfo->objPath;
        msg << "\" - ";
        msg << rods_error << " " << sys_error;
        ret = ERROR( status, msg.str() );
        irods::log( ret );
    }
    else {
        irods::file_object_ptr file_obj(
            new irods::file_object(
                rsComm,
                dataObjInfo ) );
        ret = fileUnregistered( rsComm, file_obj );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to signal resource that the data object \"";
            msg << dataObjInfo->objPath;
            msg << "\" was unregistered";
            ret = PASSMSG( msg.str(), ret );
            irods::log( ret );
            status = ret.code();
        }
    }
    return status;
#else
    return SYS_NO_RCAT_SERVER_ERR;
#endif

}

