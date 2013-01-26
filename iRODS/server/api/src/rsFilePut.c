/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See filePut.h for a description of this API call.*/

#include "filePut.h"
#include "miscServerFunct.h"
#include "fileCreate.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"



/* rsFilePut - Put the content of a small file from a single buffer
 * in filePutInpBBuf->buf.
 * Return value - int - number of bytes read.
 */

int
rsFilePut (rsComm_t *rsComm, fileOpenInp_t *filePutInp, 
           bytesBuf_t *filePutInpBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&filePutInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFilePut (rsComm, filePutInp, filePutInpBBuf,
                             rodsServerHost); 
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFilePut (rsComm, filePutInp, filePutInpBBuf, 
                                rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFilePut: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    if (status < 0) {
        return (status);
    }


    return (status);
}

int
remoteFilePut (rsComm_t *rsComm, fileOpenInp_t *filePutInp, 
               bytesBuf_t *filePutInpBBuf, rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFilePut: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFilePut (rodsServerHost->conn, filePutInp, filePutInpBBuf);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFilePut: rcFilePut failed for %s",
                 filePutInp->fileName);
    }

    return status;
}

// =-=-=-=-=-=-=-
// local implementation of put
int _rsFilePut( rsComm_t *rsComm, fileOpenInp_t *filePutInp, bytesBuf_t *filePutInpBBuf, 
                rodsServerHost_t *rodsServerHost ) {
    int fd = 0;

    // =-=-=-=-=-=-=-
    // NOTE:: this test does not seem to work for i86 solaris 
    if( ( filePutInp->otherFlags & FORCE_FLAG ) != 0 ) {
        // =-=-=-=-=-=-=-
        // create one if it does not exist */
        filePutInp->flags |= O_CREAT;
        fd = _rsFileOpen( rsComm, filePutInp );

    } else {
        fd = _rsFileCreate( rsComm, filePutInp, rodsServerHost );

    } // else

    // =-=-=-=-=-=-=-
    // log, error if any
    if( fd < 0 ) {
        if (getErrno (fd) == EEXIST) {
            rodsLog (LOG_DEBUG1,
                     "_rsFilePut: filePut for %s, status = %d",
                     filePutInp->fileName, fd);
        } else {
            rodsLog (LOG_NOTICE, 
                     "_rsFilePut: filePut for %s, status = %d",
                     filePutInp->fileName, fd);
        }
        return (fd);
    }

    // =-=-=-=-=-=-=-
    // call write for resource plugin
    eirods::file_object file_obj( rsComm, filePutInp->fileName, filePutInp->resc_hier_, fd, 0, 0 );
    eirods::error write_err = fileWrite( rsComm,
                                         file_obj, 
                                         filePutInpBBuf->buf, 
                                         filePutInpBBuf->len );
    int write_code = write_err.code();
    // =-=-=-=-=-=-=-
    // log errors, if any
    //if ( !write_code || write_code != filePutInpBBuf->len ) {
    if ( write_code != filePutInpBBuf->len ) {
        if( write_code >= 0 ) {
            std::stringstream msg;
            msg << "_rsFilePut: fileWrite for ";
            msg << filePutInp->fileName;
            msg << ", towrite ";
            msg << filePutInpBBuf->len;
            msg << ", status = ";
            msg << write_code;
            eirods::error err = PASS( false, write_code, msg.str(), write_err );
            eirods::log ( err );
            write_code = SYS_COPY_LEN_ERR;
        } else {
            std::stringstream msg;
            msg << "_rsFilePut: fileWrite for ";
            msg << filePutInp->fileName;
            msg << ", status = ";
            msg << write_code;
            eirods::error err = PASS( false, write_code, msg.str(), write_err );
            eirods::log ( err );
        }
    }
   
    // =-=-=-=-=-=-=-
    // close up after ourselves 
    eirods::error close_err = fileClose( rsComm,
                                         file_obj );
    if( !close_err.ok() ) {
        eirods::error err = PASS( false, close_err.code(), "_rsFilePut - error on close", close_err );
        eirods::log( err );
    }
   
    // =-=-=-=-=-=-=-
    // return 'write_err code' as this includes this implementation
    // assumes we are returning the size of the file 'put' via fileWrite 
    return write_code;

} // _rsFilePut




