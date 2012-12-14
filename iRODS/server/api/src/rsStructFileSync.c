/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileDriver.h"
#include "structFileSync.h" 
#include "miscServerFunct.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"

int
rsStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&structFileOprInp->addr, &rodsServerHost);

    if (remoteFlag == LOCAL_HOST) {
        status = _rsStructFileSync (rsComm, structFileOprInp);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteStructFileSync (rsComm, structFileOprInp, rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
              "rsStructFileSync: resolveHost returned unrecognized value %d",
               remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp,
rodsServerHost_t *rodsServerHost)
{
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
          "remoteStructFileSync: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }

    status = rcStructFileSync (rodsServerHost->conn, structFileOprInp);

    if (status < 0) {
        rodsLog (LOG_NOTICE,
         "remoteStructFileSync: rcStructFileSync failed for %s, status = %d",
          structFileOprInp->specColl->collection, status);
    }

    return status;
}

// =-=-=-=-=-=-=-
// implementation for local rs call for sync
int _rsStructFileSync( rsComm_t*           _comm, 
                       structFileOprInp_t* _struct_inp ) {
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
	// cache data type for selection of tasty compression options
    char* data_type = getValByKey( &_struct_inp->condInput, DATA_TYPE_KW );
    if( data_type ) {
        struct_obj.data_type( data_type );
    }

    // =-=-=-=-=-=-=-
	// retrieve the resource name given the object
	eirods::resource_ptr resc;
    eirods::error ret_err = struct_obj.resolve( resc_mgr, resc ); 
	if( !ret_err.ok() ) {
		eirods::error err = PASS( false, -1, "_rsStructFileSync - failed to resolve resource", ret_err );
        eirods::log( err );
        return ret_err.code();
	}
 
	// =-=-=-=-=-=-=-
	// make the call to the "extract" interface
	ret_err = resc->call< eirods::first_class_object* >( "sync", &struct_obj );

    // =-=-=-=-=-=-=-
	// pass along an error from the interface or return SUCCESS
	if( !ret_err.ok() ) {
        eirods::error err = PASS( false, ret_err.code(), "_rsStructFileSync - failed to call 'sync'", ret_err );
        eirods::log( err );
        return ret_err.code();
	} else {
        return ret_err.code();
	}
    
} // _rsStructFileSync

