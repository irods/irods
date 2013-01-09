/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "structFileExtract.h" 
#include "miscServerFunct.h"
#include "syncMountedColl.h"

#include "dataObjOpr.h"
#include "rsGlobalExtern.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "physPath.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"


int
rsStructFileExtract (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&structFileOprInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsStructFileExtract (rsComm, structFileOprInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = 
	  remoteStructFileExtract (rsComm, structFileOprInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
             "rsStructFileExtract: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteStructFileExtract (rsComm_t *rsComm, 
structFileOprInp_t *structFileOprInp, rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteStructFileExtract: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcStructFileExtract (rodsServerHost->conn, structFileOprInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteStructFileExtract: rcStructFileExtract failed for %s, status = %d",
          structFileOprInp->specColl->collection, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// implemenation for local rs call for extraction
int _rsStructFileExtract( rsComm_t*           _comm, 
                          structFileOprInp_t* _struct_inp ) {
    // =-=-=-=-=-=-=-
    // pointer check
    if( _comm                 == NULL || 
        _struct_inp           == NULL ||
        _struct_inp->specColl == NULL) {
        rodsLog( LOG_ERROR, "_rsStructFileExtract: NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // =-=-=-=-=-=-=-
    // create dir into which to extract
    int status = procCacheDir( _comm, 
                               _struct_inp->specColl->cacheDir, 
                               _struct_inp->specColl->resource, 
                               _struct_inp->oprType ); // JMC - backport 4657
    if( status < 0 ) {
        rodsLog( LOG_ERROR, "_rsStructFileExtract - failed in call to procCacheDir status = %d", status );
        return status;
    }

    // =-=-=-=-=-=-=-
    // create a structured fco and resolve a resource plugin
    // to handle the extract process
    eirods::structured_object struct_obj;
    struct_obj.spec_coll( _struct_inp->specColl );
    struct_obj.addr( _struct_inp->addr );
    struct_obj.flags( _struct_inp->flags );
    struct_obj.comm( _comm );
    struct_obj.opr_type( _struct_inp->oprType );

    // =-=-=-=-=-=-=-
    // extract the data type
    char* data_type = getValByKey( &_struct_inp->condInput, DATA_TYPE_KW );
    if( data_type ) {
        struct_obj.data_type( data_type );
    } 

    // =-=-=-=-=-=-=-
	// retrieve the resource name given the object
	eirods::resource_ptr resc;
    eirods::error ret_err = struct_obj.resolve( resc_mgr, resc ); 
	if( !ret_err.ok() ) {
		eirods::error err = PASS( false, -1, "_rsStructFileExtract - failed to resolve resource", ret_err );
        eirods::log( err );
        return ret_err.code();
	}
 
	// =-=-=-=-=-=-=-
	// make the call to the "extract" interface
	ret_err = resc->call< eirods::first_class_object* >( "extract", &struct_obj );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        eirods::error err = PASS( false, ret_err.code(), "_rsStructFileExtract - failed to call 'extract'", ret_err );
        eirods::log( err );
        return ret_err.code();
	} else {
        return ret_err.code();
	}

} // _rsStructFileExtract

int
procCacheDir (rsComm_t *rsComm, char *cacheDir, char *resource, int oprType) {
    if( ( oprType & PRESERVE_DIR_CONT) == 0 ) { 
        int status = chkEmptyDir( -1, rsComm, cacheDir );
        if( status == SYS_DIR_IN_VAULT_NOT_EMPTY ) {
            rodsLog( LOG_ERROR,"procCacheDir: chkEmptyDir error for %s in resc %s, status = %d",
                     cacheDir, resource, status );
            return (status);
        }

    }

    mkFileDirR( rsComm, "/", cacheDir, getDefDirMode() );

    return 0;
}

