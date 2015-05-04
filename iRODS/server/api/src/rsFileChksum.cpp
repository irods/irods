/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileChksum.h for a description of this API call.*/

#include "fileChksum.h"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"
#include "irods_hasher_factory.hpp"
#include "readServerConfig.hpp"
#include "irods_server_properties.hpp"
#include "MD5Strategy.hpp"

#define SVR_MD5_BUF_SZ (1024*1024)

int
rsFileChksum(
    rsComm_t *rsComm,
    fileChksumInp_t *fileChksumInp,
    char **chksumStr ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;
    irods::error ret = irods::get_host_for_hier_string( fileChksumInp->rescHier, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }


    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileChksum( rsComm, fileChksumInp, chksumStr );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileChksum( rsComm, fileChksumInp, chksumStr,
                                   rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileChksum: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteFileChksum( rsComm_t *rsComm, fileChksumInp_t *fileChksumInp,
                  char **chksumStr, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileChksum: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileChksum( rodsServerHost->conn, fileChksumInp, chksumStr );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileChksum: rcFileChksum failed for %s",
                 fileChksumInp->fileName );
    }

    return status;
}

int
_rsFileChksum(
    rsComm_t *rsComm,
    fileChksumInp_t *fileChksumInp,
    char **chksumStr ) {
    int status;
    if ( !*chksumStr ) {
        *chksumStr = ( char* )malloc( sizeof( char ) * NAME_LEN );
    }

    status = fileChksum(
                 rsComm,
                 fileChksumInp->objPath,
                 fileChksumInp->fileName,
                 fileChksumInp->rescHier,
                 fileChksumInp->orig_chksum,
                 *chksumStr );
    if ( status < 0 ) {
        rodsLog( LOG_DEBUG,
                 "_rsFileChksum: fileChksum for %s, status = %d",
                 fileChksumInp->fileName, status );
        free( *chksumStr );
        *chksumStr = NULL;
    }

    return status;
}

int fileChksum(
    rsComm_t* rsComm,
    char*     objPath,
    char*     fileName,
    char*     rescHier,
    char*     orig_chksum,
    char*     chksumStr ) {
    // =-=-=-=-=-=-=-
    // capture server hashing settings
    std::string svr_hash_scheme;
    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.get_property< std::string >(
                           DEFAULT_HASH_SCHEME_KW,
                           svr_hash_scheme );
    std::string hash_scheme( irods::MD5_NAME );
    if ( ret.ok() ) {
        hash_scheme = svr_hash_scheme;
    }

    // make sure the read parameter is lowercased
    std::transform(
        hash_scheme.begin(),
        hash_scheme.end(),
        hash_scheme.begin(),
        ::tolower );

    std::string svr_hash_policy;
    ret = props.get_property< std::string >(
              MATCH_HASH_POLICY_KW,
              svr_hash_policy );
    std::string hash_policy;
    if ( ret.ok() ) {
        hash_policy = svr_hash_policy;
    }

    // =-=-=-=-=-=-=-
    // extract scheme from checksum string
    std::string chkstr_scheme;
    if ( orig_chksum ) {
        ret = irods::get_hash_scheme_from_checksum(
                  orig_chksum,
                  chkstr_scheme );
        if ( !ret.ok() ) {
            //irods::log( PASS( ret ) );
        }
    }

    // =-=-=-=-=-=-=-
    // check the hash scheme against the policy
    // if necessary
    std::string final_scheme( hash_scheme );
    if ( !chkstr_scheme.empty() ) {
        if ( !hash_policy.empty() ) {
            if ( irods::STRICT_HASH_POLICY == hash_policy ) {
                if ( hash_scheme != chkstr_scheme ) {
                    return USER_HASH_TYPE_MISMATCH;
                }
            }
        }
        final_scheme = chkstr_scheme;
    }

    rodsLog(
        LOG_DEBUG,
        "fileChksum :: final_scheme [%s]  chkstr_scheme [%s]  svr_hash_policy [%s]  hash_policy [%s]",
        final_scheme.c_str(),
        chkstr_scheme.c_str(),
        svr_hash_policy.c_str(),
        hash_policy.c_str() );

    // =-=-=-=-=-=-=-
    // call resource plugin to open file
    irods::file_object_ptr file_obj(
        new irods::file_object(
            rsComm,
            objPath,
            fileName,
            rescHier,
            -1, 0, O_RDONLY ) ); // FIXME :: hack until this is better abstracted - JMC
    ret = fileOpen( rsComm, file_obj );
    if ( !ret.ok() ) {
        int status = UNIX_FILE_OPEN_ERR - errno;
        if ( ret.code() != DIRECT_ARCHIVE_ACCESS ) {
            std::stringstream msg;
            msg << "fileOpen failed for [";
            msg << fileName;
            msg << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
        }
        else {
            status = ret.code();
        }
        return status;
    }

    // =-=-=-=-=-=-=-
    // create a hasher object and init given a scheme
    // if it is unsupported then default to md5
    irods::Hasher hasher;
    ret = irods::getHasher( final_scheme, hasher );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        irods::getHasher( irods::MD5_NAME, hasher );
    }

    // =-=-=-=-=-=-=-
    // do an inital read of the file
    char buffer[SVR_MD5_BUF_SZ];
    irods::error read_err = fileRead(
                                rsComm,
                                file_obj,
                                buffer,
                                SVR_MD5_BUF_SZ );
    int bytes_read = read_err.code();

    // =-=-=-=-=-=-=-
    // loop and update while there are still bytes to be read
    while ( read_err.ok() && bytes_read > 0 ) {
        // =-=-=-=-=-=-=-
        // update hasher
        hasher.update( std::string( buffer, bytes_read ) );

        // =-=-=-=-=-=-=-
        // read some more
        read_err = fileRead( rsComm, file_obj, buffer, SVR_MD5_BUF_SZ );
        if ( read_err.ok() ) {
            bytes_read = read_err.code();
        }
        else {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to read buffer from file: \"";
            msg << fileName;
            msg << "\"";
            irods::error result = PASSMSG( msg.str(), read_err );
            irods::log( result );
            return result.code();
        }

    } // while

    // =-=-=-=-=-=-=-
    // close out the file
    ret = fileClose( rsComm, file_obj );
    if ( !ret.ok() ) {
        irods::error err = PASSMSG( "error on close", ret );
        irods::log( err );
    }

    // =-=-=-=-=-=-=-
    // extract the digest from the hasher object
    // and copy to outgoing string
    std::string digest;
    hasher.digest( digest );
    strncpy( chksumStr, digest.c_str(), NAME_LEN );

    return 0;

}
