/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileSync.hpp" 
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.hpp"
#include "eirods_resource_backport.hpp"
#include "eirods_stacktrace.hpp"

int
rsStructFileSync (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;
   
    char* resc_hier =  getValByKey( &structFileOprInp->condInput, RESC_HIER_STR_KW );
    if( resc_hier != NULL ) {
        eirods::error ret = eirods::get_host_for_hier_string( resc_hier, remoteFlag, rodsServerHost );
        if( !ret.ok() ) {
            eirods::log( PASSMSG( "failed in call to eirods::get_host_for_hier_string", ret ) );
            return -1;
        }
    } else {
        return -1;
    }

    //remoteFlag = resolveHost (&structFileOprInp->addr, &rodsServerHost);

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
    eirods::structured_object_ptr struct_obj( 
                                      new eirods::structured_object( 
                                           ) );
    struct_obj->spec_coll( _struct_inp->specColl );
    struct_obj->addr( _struct_inp->addr );
    struct_obj->flags( _struct_inp->flags );
    struct_obj->comm( _comm );
    struct_obj->opr_type( _struct_inp->oprType );
    struct_obj->resc_hier( eirods::EIRODS_LOCAL_USE_ONLY_RESOURCE );

    // =-=-=-=-=-=-=-
    // cache data type for selection of tasty compression options
    char* data_type = getValByKey( &_struct_inp->condInput, DATA_TYPE_KW );
    if( data_type ) {
        struct_obj->data_type( data_type );
    }

    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    eirods::plugin_ptr ptr;
    eirods::error ret_err = struct_obj->resolve( eirods::RESOURCE_INTERFACE, ptr ); 
    if( !ret_err.ok() ) {
        eirods::error err = PASSMSG( "failed to resolve resource", ret_err );
        eirods::log( err );
        return ret_err.code();
    }
    
    eirods::resource_ptr resc = boost::dynamic_pointer_cast< eirods::resource >( ptr );
 
    // =-=-=-=-=-=-=-
    // make the call to the "extract" interface
    ret_err = resc->call( _comm, "sync", struct_obj );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if( !ret_err.ok() ) {
        eirods::error err = PASSMSG( "failed to call 'sync'", ret_err );
        eirods::log( err );
        return ret_err.code();
    } else {
        return ret_err.code();
    }
    
} // _rsStructFileSync

