/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "structFileExtract.hpp"
#include "miscServerFunct.hpp"
#include "syncMountedColl.hpp"

#include "dataObjOpr.hpp"
#include "rsGlobalExtern.hpp"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "physPath.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"

int
rsStructFileExtract( rsComm_t *rsComm, structFileOprInp_t *structFileOprInp ) {
    //rodsServerHost_t *rodsServerHost;
    //int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&structFileOprInp->addr, &rodsServerHost);
    dataObjInp_t dataObjInp;
    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, structFileOprInp->specColl->objPath, MAX_NAME_LEN );

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    std::string       hier;
    int               local = LOCAL_HOST;
    rodsServerHost_t* host  =  0;
    if ( getValByKey( &structFileOprInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        irods::error ret = irods::resource_redirect( irods::OPEN_OPERATION, rsComm,
                           &dataObjInp, hier, host, local );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed in irods::resource_redirect for [";
            msg << dataObjInp.objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &structFileOprInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword


    if ( local == LOCAL_HOST ) {
        status = _rsStructFileExtract( rsComm, structFileOprInp );
    }
    else if ( local == REMOTE_HOST ) {
        //status = remoteStructFileExtract (rsComm, structFileOprInp, rodsServerHost);
        status = rcStructFileExtract( host->conn, structFileOprInp );
    }
    else {
        if ( local < 0 ) {
            return local;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsStructFileExtract: resolveHost returned unrecognized value %d",
                     local );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteStructFileExtract( rsComm_t *rsComm,
                         structFileOprInp_t *structFileOprInp, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteStructFileExtract: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcStructFileExtract( rodsServerHost->conn, structFileOprInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteStructFileExtract: rcStructFileExtract failed for %s, status = %d",
                 structFileOprInp->specColl->collection, status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// implemenation for local rs call for extraction
int _rsStructFileExtract( rsComm_t*           _comm,
                          structFileOprInp_t* _struct_inp ) {
    // =-=-=-=-=-=-=-
    // pointer check
    if ( _comm                 == NULL ||
            _struct_inp           == NULL ||
            _struct_inp->specColl == NULL ) {
        rodsLog( LOG_ERROR, "_rsStructFileExtract: NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // =-=-=-=-=-=-=-
    // create dir into which to extract
    int status = procCacheDir( _comm,
                               _struct_inp->specColl->cacheDir,
                               _struct_inp->specColl->resource,
                               _struct_inp->oprType,
                               _struct_inp->specColl->rescHier ); // JMC - backport 4657
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "_rsStructFileExtract - failed in call to procCacheDir status = %d", status );
        return status;
    }

    // =-=-=-=-=-=-=-
    // create a structured fco and resolve a resource plugin
    // to handle the extract process
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
        ) );
    struct_obj->spec_coll( _struct_inp->specColl );
    struct_obj->addr( _struct_inp->addr );
    struct_obj->flags( _struct_inp->flags );
    struct_obj->comm( _comm );
    struct_obj->opr_type( _struct_inp->oprType );
    struct_obj->spec_coll_type( _struct_inp->specColl->type );

    // =-=-=-=-=-=-=-
    // extract the data type
    char* data_type = getValByKey( &_struct_inp->condInput, DATA_TYPE_KW );
    if ( data_type ) {
        struct_obj->data_type( data_type );
    }

    // =-=-=-=-=-=-=-
    // retrieve the resource name given the object
    irods::plugin_ptr ptr;
    irods::error ret_err = struct_obj->resolve( irods::RESOURCE_INTERFACE, ptr );
    if ( !ret_err.ok() ) {
        irods::error err = PASSMSG( "failed to resolve resource", ret_err );
        irods::log( err );
        return ret_err.code();
    }

    irods::resource_ptr resc = boost::dynamic_pointer_cast< irods::resource >( ptr );

    // =-=-=-=-=-=-=-
    // make the call to the "extract" interface
    ret_err = resc->call( _comm, "extract", struct_obj );

    // =-=-=-=-=-=-=-
    // pass along an error from the interface or return SUCCESS
    if ( !ret_err.ok() ) {
        irods::error err = PASSMSG( "failed to call 'extract'", ret_err );
        irods::log( err );
        return ret_err.code();
    }
    else {
        return ret_err.code();
    }

} // _rsStructFileExtract

int
procCacheDir( rsComm_t *rsComm, char *cacheDir, char *resource, int oprType, char* hier ) {
    if ( ( oprType & PRESERVE_DIR_CONT ) == 0 ) {
        int status = chkEmptyDir( rsComm, cacheDir, hier );
        if ( status == SYS_DIR_IN_VAULT_NOT_EMPTY ) {
            rodsLog( LOG_ERROR, "procCacheDir: chkEmptyDir error for %s in resc %s, status = %d",
                     cacheDir, resource, status );
            return status;
        }

    }

    int status = mkFileDirR( rsComm, 0, cacheDir, hier, getDefDirMode() );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "mkFileDirR failed in procCacheDir with status %d", status );
    }
    return status;
}

