/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileChksum.h for a description of this API call.*/

#include "fileChksum.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"

#define SVR_MD5_BUF_SZ (1024*1024)

int
rsFileChksum (rsComm_t *rsComm, fileChksumInp_t *fileChksumInp, 
              char **chksumStr)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost (&fileChksumInp->addr, &rodsServerHost);
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileChksum (rsComm, fileChksumInp, chksumStr);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileChksum (rsComm, fileChksumInp, chksumStr,
                                   rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileChksum: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteFileChksum (rsComm_t *rsComm, fileChksumInp_t *fileChksumInp,
                  char **chksumStr, rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileChksum: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }


    status = rcFileChksum (rodsServerHost->conn, fileChksumInp, chksumStr);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileChksum: rcFileChksum failed for %s",
                 fileChksumInp->fileName);
    }

    return status;
}

int
_rsFileChksum (rsComm_t *rsComm, fileChksumInp_t *fileChksumInp,
               char **chksumStr)
{
    int status;

    *chksumStr = (char*)malloc (NAME_LEN);

    status = fileChksum (fileChksumInp->fileType, rsComm, 
                         fileChksumInp->fileName, *chksumStr);

    if (status < 0) {
        rodsLog (LOG_NOTICE, 
                 "_rsFileChksum: fileChksum for %s, status = %d",
                 fileChksumInp->fileName, status);
        free (*chksumStr);
        *chksumStr = NULL;
        return (status);
    }

    return (status);
} 

int
fileChksum (int fileType, rsComm_t *rsComm, char *fileName, char *chksumStr)
{
    MD5_CTX context;
    int bytes_read;
    unsigned char buffer[SVR_MD5_BUF_SZ], digest[16];
    int status;

#ifdef MD5_DEBUG
    rodsLong_t total_bytes_read = 0;    /* XXXX debug */
#endif
  
    // =-=-=-=-=-=-=-
    // call resource plugin to open file
    eirods::file_object file_obj( rsComm, fileName, "", -1, 0, O_RDONLY ); // FIXME :: hack until this is better abstracted - JMC
    eirods::error ret = fileOpen( file_obj );
    if( !ret.ok() ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_NOTICE,"fileChksum; fileOpen failed for %s. status = %d", fileName, status );
        return (status);
    }

    MD5Init (&context);

    eirods::error read_err = fileRead( file_obj, buffer, SVR_MD5_BUF_SZ );      
    bytes_read = read_err.code();
    while( read_err.ok() && bytes_read > 0 ) {
#ifdef MD5_DEBUG
        total_bytes_read += bytes_read;     /* XXXX debug */
#endif
        MD5Update (&context, buffer, bytes_read);
    
        read_err = fileRead( file_obj, buffer, SVR_MD5_BUF_SZ );
        bytes_read = read_err.code();

    } // while

    MD5Final (digest, &context);

    ret = fileClose( file_obj );
    if( !ret.ok() ) {
        eirods::error err = PASS( false, ret.code(), "fileChksum - error on close", ret );
        eirods::log( err );
    }

    md5ToStr (digest, chksumStr);

#ifdef MD5_DEBUG
    rodsLog (LOG_NOTICE,        /* XXXX debug */
             "fileChksum: chksum = %s, total_bytes_read = %lld", chksumStr, total_bytes_read);
#endif

    return (0);

}

