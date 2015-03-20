/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rcPortalOpr.hpp"
#include "dataObjOpen.hpp"
#include "dataObjWrite.hpp"
#include "dataObjRead.hpp"
#include "dataObjLseek.hpp"
#include "fileLseek.hpp"
#include "dataObjOpr.hpp"
#include "rodsLog.hpp"
#include "rcGlobalExtern.hpp"
#include "md5.hpp"

// =-=-=-=-=-=-=-
#include "irods_stacktrace.hpp"
#include "irods_buffer_encryption.hpp"
#include "irods_client_server_negotiation.hpp"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <iomanip>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;



int
sendTranHeader( int sock, int oprType, int flags, rodsLong_t offset,
                rodsLong_t length ) {
    transferHeader_t myHeader;
    int retVal;

    myHeader.oprType = htonl( oprType );
    myHeader.flags = htonl( flags );
    myHtonll( offset, ( rodsLong_t * ) &myHeader.offset );
    myHtonll( length, ( rodsLong_t * ) &myHeader.length );

    retVal = myWrite( sock, ( void * ) &myHeader, sizeof( myHeader ), NULL );

    if ( retVal != sizeof( myHeader ) ) {
        rodsLog( LOG_ERROR,
                 "sendTranHeader: toWrite = %d, written = %d",
                 sizeof( myHeader ), retVal );
        if ( retVal < 0 ) {
            return retVal;
        }
        else {
            return SYS_COPY_LEN_ERR;
        }
    }
    else {
        return 0;
    }
}

int
rcvTranHeader( int sock, transferHeader_t *myHeader ) {
    int retVal;
    transferHeader_t tmpHeader;

    retVal = myRead( sock, ( void * ) &tmpHeader, sizeof( tmpHeader ),
                     NULL, NULL );

    if ( retVal != sizeof( tmpHeader ) ) {
        rodsLog( LOG_ERROR,
                 "rcvTranHeader: toread = %d, read = %d",
                 sizeof( tmpHeader ), retVal );
        if ( retVal < 0 ) {
            return retVal;
        }
        else {
            return SYS_COPY_LEN_ERR;
        }
    }

    myHeader->oprType = htonl( tmpHeader.oprType );
    myHeader->flags = htonl( tmpHeader.flags );
    myNtohll( tmpHeader.offset, &myHeader->offset );
    myNtohll( tmpHeader.length, &myHeader->length );

    return 0;
}

int
fillBBufWithFile( rcComm_t *conn, bytesBuf_t *myBBuf, char *locFilePath,
                  rodsLong_t dataSize ) {
    int in_fd, status;

    if ( dataSize > 10 * MAX_SZ_FOR_SINGLE_BUF ) {
        rodsLog( LOG_ERROR,
                 "fillBBufWithFile: dataSize %lld too large", dataSize );
        return USER_FILE_TOO_LARGE;
    }
    else if ( dataSize > MAX_SZ_FOR_SINGLE_BUF ) {
        rodsLog( LOG_NOTICE,
                 "fillBBufWithFile: dataSize %lld too large", dataSize );
    }

#ifdef windows_platform
    in_fd = iRODSNt_bopen( locFilePath, O_RDONLY, 0 );
#else
    in_fd = open( locFilePath, O_RDONLY, 0 );
#endif
    if ( in_fd < 0 ) { /* error */
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "cannot open file %s", locFilePath, status );
        return status;
    }


    myBBuf->buf = malloc( dataSize );
    myBBuf->len = dataSize;
    conn->transStat.bytesWritten = dataSize;

    status = myRead( in_fd, myBBuf->buf, ( int ) dataSize,
                     NULL, NULL );

    close( in_fd );

    return status;
}

int
putFileToPortal( rcComm_t *conn, portalOprOut_t *portalOprOut,
                 char *locFilePath, char *objPath, rodsLong_t dataSize ) {
    portList_t *myPortList;
    int i, sock, in_fd;
    int numThreads;
    rcPortalTransferInp_t myInput[MAX_NUM_CONFIG_TRAN_THR];
    boost::thread* tid[MAX_NUM_CONFIG_TRAN_THR];
    int retVal = 0;

    if ( portalOprOut == NULL || portalOprOut->numThreads <= 0 ) {
        rodsLog( LOG_ERROR,
                 "putFileToPortal: invalid portalOprOut" );
        return SYS_INVALID_PORTAL_OPR;
    }

    numThreads = portalOprOut->numThreads;

    myPortList = &portalOprOut->portList;

    if ( portalOprOut->numThreads > MAX_NUM_CONFIG_TRAN_THR ) {
        for ( i = 0; i < portalOprOut->numThreads; i++ ) {
            sock = connectToRhostPortal( myPortList->hostAddr,
                                         myPortList->portNum, myPortList->cookie, myPortList->windowSize );
            if ( sock > 0 ) {
                close( sock );
            }
        }
        rodsLog( LOG_ERROR,
                 "putFileToPortal: numThreads %d too large",
                 portalOprOut->numThreads );
        return SYS_INVALID_PORTAL_OPR;
    }

    initFileRestart( conn, locFilePath, objPath, dataSize,
                     portalOprOut->numThreads );
    memset( tid, 0, sizeof( tid ) );
    memset( myInput, 0, sizeof( myInput ) );

    if ( numThreads == 1 ) {
        sock = connectToRhostPortal( myPortList->hostAddr,
                                     myPortList->portNum, myPortList->cookie, myPortList->windowSize );
        if ( sock < 0 ) {
            return sock;
        }
#ifdef windows_platform
        in_fd = iRODSNt_bopen( locFilePath, O_RDONLY, 0 );
#else
        in_fd = open( locFilePath, O_RDONLY, 0 );
#endif
        if ( in_fd < 0 ) { /* error */
            retVal = UNIX_FILE_OPEN_ERR - errno;
            rodsLogError( LOG_ERROR, retVal,
                          "cannot open file %s", locFilePath, retVal );
            return retVal;
        }

        fillRcPortalTransferInp( conn, &myInput[0], sock, in_fd, 0 );

        rcPartialDataPut( &myInput[0] );
        if ( myInput[0].status < 0 ) {
            return myInput[0].status;
        }
        else {
            if ( dataSize <= 0 || myInput[0].bytesWritten == dataSize ) {
                return 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "putFileToPortal: bytesWritten %lld dataSize %lld mismatch",
                         myInput[0].bytesWritten, dataSize );
                return SYS_COPY_LEN_ERR;
            }
        }
    }
    else {
        rodsLong_t totalWritten = 0;

        for ( i = 0; i < numThreads; i++ ) {
            sock = connectToRhostPortal( myPortList->hostAddr,
                                         myPortList->portNum, myPortList->cookie, myPortList->windowSize );
            if ( sock < 0 ) {
                return sock;
            }
            in_fd = open( locFilePath, O_RDONLY, 0 );
            if ( in_fd < 0 ) { 	/* error */
                retVal = UNIX_FILE_OPEN_ERR - errno;
                rodsLogError( LOG_ERROR, retVal,
                              "cannot open file %s", locFilePath, retVal );
                continue;
            }
            fillRcPortalTransferInp( conn, &myInput[i], sock, in_fd, i );
            try {
                tid[i] = new boost::thread( rcPartialDataPut, &myInput[i] );
            }
            catch ( const boost::thread_resource_error& ) {
                rodsLog( LOG_ERROR, "boost encountered thread_resource_error on constructing thread." );
                return SYS_THREAD_RESOURCE_ERR;
            }
        }
        if ( retVal < 0 ) {
            return retVal;
        }

        for ( i = 0; i < numThreads; i++ ) {
            if ( tid[i] != 0 ) {
                try {
                    tid[i]->join();
                }
                catch ( const boost::thread_resource_error& ) {
                    rodsLog( LOG_ERROR, "boost encountered thread_resource_error on constructing thread." );
                    return SYS_THREAD_RESOURCE_ERR;
                }
            }
            totalWritten += myInput[i].bytesWritten;
            if ( myInput[i].status < 0 ) {
                retVal = myInput[i].status;
            }
        }
        if ( retVal < 0 ) {
            return retVal;
        }
        else {
            if ( dataSize <= 0 || totalWritten == dataSize ) {
                if ( gGuiProgressCB != NULL ) {
                    gGuiProgressCB( &conn->operProgress );
                }
                return 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "putFileToPortal: totalWritten %lld dataSize %lld mismatch",
                         totalWritten, dataSize );
                return SYS_COPY_LEN_ERR;
            }
        }
    }
}

int
fillRcPortalTransferInp( rcComm_t *conn, rcPortalTransferInp_t *myInput,
                         int destFd, int srcFd, int threadNum ) {
    if ( myInput == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    myInput->conn = conn;
    myInput->destFd = destFd;
    myInput->srcFd = srcFd;
    myInput->threadNum = threadNum;
    memcpy( myInput->shared_secret, conn->shared_secret, NAME_LEN );

    return 0;
}


void
rcPartialDataPut( rcPortalTransferInp_t *myInput ) {
    transferHeader_t myHeader;
    int destFd = 0;
    int srcFd = 0;
    transferStat_t *myTransStat = 0;
    rodsLong_t curOffset = 0;
    rcComm_t *conn = 0;
    fileRestartInfo_t* info = 0;
    int threadNum = 0;

    if ( myInput == NULL ) {
        rodsLog( LOG_ERROR,
                 "rcPartialDataPut: NULL input" );
        return;
    }
#ifdef PARA_DEBUG
    printf( "rcPartialDataPut: thread %d at start\n", myInput->threadNum );
#endif
    conn = myInput->conn;
    info = &conn->fileRestart.info;
    threadNum = myInput->threadNum;

    myTransStat = &myInput->conn->transStat;

    destFd = myInput->destFd;
    srcFd = myInput->srcFd;

    myInput->bytesWritten = 0;

    if ( gGuiProgressCB != NULL ) {
        conn->operProgress.flag = 1;
    }

    // =-=-=-=-=-=-=-
    // flag to determine if we need to use encryption
    bool use_encryption_flg =
        ( myInput->conn->negotiation_results ==
          irods::CS_NEG_USE_SSL );

    // =-=-=-=-=-=-=-
    // get the client side Env to determine
    // encryption parameters
    rodsEnv rods_env;
    int status = getRodsEnv( &rods_env );
    if ( status < 0 ) {
        printf( "Failed to get irodsEnv" );
        return;
    }

    // =-=-=-=-=-=-=-
    // create an encryption context
    int iv_size = 0;
    irods::buffer_crypt::array_t iv;
    irods::buffer_crypt::array_t cipher;
    irods::buffer_crypt::array_t in_buf;
    irods::buffer_crypt::array_t shared_secret;
    irods::buffer_crypt crypt(
        rods_env.rodsEncryptionKeySize,
        rods_env.rodsEncryptionSaltSize,
        rods_env.rodsEncryptionNumHashRounds,
        rods_env.rodsEncryptionAlgorithm );

    // =-=-=-=-=-=-=-
    // set iv size
    if ( use_encryption_flg ) {
        iv_size = crypt.key_size();
        shared_secret.assign(
            &myInput->shared_secret[0],
            &myInput->shared_secret[iv_size] );
    }

    // =-=-=-=-=-=-=-
    // allocate a buffer for writing
    size_t buf_size = 2 * TRANS_BUF_SZ * sizeof( unsigned char );
    unsigned char* buf = ( unsigned char* )malloc( buf_size );

    while ( myInput->status >= 0 ) {
        rodsLong_t toPut;

        myInput->status = rcvTranHeader( destFd, &myHeader );

#ifdef PARA_DEBUG
        printf( "rcPartialDataPut: thread %d after rcvTranHeader\n",
                myInput->threadNum );
#endif

        if ( myInput->status < 0 ) {
            break;
        }

        if ( myHeader.oprType == DONE_OPR ) {
            break;
        }
        if ( myHeader.offset != curOffset ) {
            curOffset = myHeader.offset;
            if ( lseek( srcFd, curOffset, SEEK_SET ) < 0 ) {
                myInput->status = UNIX_FILE_LSEEK_ERR - errno;
                rodsLogError( LOG_ERROR, myInput->status,
                              "rcPartialDataPut: lseek to %lld error, status = %d",
                              curOffset, myInput->status );
                break;
            }
            if ( info->numSeg > 0 ) {   /* file restart */
                info->dataSeg[threadNum].offset = curOffset;
            }
        }

        toPut = myHeader.length;
        while ( toPut > 0 ) {
            int toRead, bytesRead, bytesWritten;

            if ( toPut > TRANS_BUF_SZ ) {
                toRead = TRANS_BUF_SZ;
            }
            else {
                toRead = toPut;
            }

            bytesRead = myRead(
                            srcFd,
                            buf,
                            toRead,
                            &bytesRead,
                            NULL );
            if ( bytesRead != toRead ) {
                myInput->status = SYS_COPY_LEN_ERR - errno;
                rodsLogError( LOG_ERROR, myInput->status,
                              "rcPartialDataPut: toPut %lld, bytesRead %d",
                              toPut, bytesRead );
                break;
            }

            // =-=-=-=-=-=-=-
            // compute an iv for this particular transmission and use
            // it to encrypt this buffer
            int new_size = bytesRead;
            if ( use_encryption_flg ) {
                irods::error ret = crypt.initialization_vector( iv );
                if ( !ret.ok() ) {
                    ret = PASS( ret );
                    printf( "%s", ret.result().c_str() );
                    break;
                }

                // =-=-=-=-=-=-=-
                // encrypt
                in_buf.assign(
                    &buf[0],
                    &buf[ bytesRead ] );
                ret = crypt.encrypt(
                          shared_secret,
                          iv,
                          in_buf,
                          cipher );
                if ( !ret.ok() ) {
                    ret = PASS( ret );
                    printf( "%s", ret.result().c_str() );
                    break;
                }

                // =-=-=-=-=-=-=-
                // capture the iv with the cipher text
                memset( buf, 0,  buf_size );
                std::copy(
                    iv.begin(),
                    iv.end(),
                    &buf[0] );
                std::copy(
                    cipher.begin(),
                    cipher.end(),
                    &buf[iv_size] );

                new_size = iv_size + cipher.size();

                // =-=-=-=-=-=-=-
                // need to send the incoming size as encryption might change
                // the size of the data from the writen values
                bytesWritten = myWrite(
                                   destFd,
                                   &new_size,
                                   sizeof( int ),
                                   &bytesWritten );

            }

            // =-=-=-=-=-=-=-
            // then write the actual buffer
            bytesWritten = myWrite(
                               destFd,
                               buf,
                               new_size,
                               &bytesWritten );

            if ( bytesWritten != new_size ) {
                myInput->status = SYS_COPY_LEN_ERR - errno;
                rodsLogError( LOG_ERROR, myInput->status,
                              "rcPartialDataPut: toWrite %d, bytesWritten %d, errno = %d",
                              bytesRead, bytesWritten, errno );
                break;
            }

            toPut -= bytesRead;
            if ( info->numSeg > 0 ) {   /* file restart */
                info->dataSeg[threadNum].len += bytesRead;
                conn->fileRestart.writtenSinceUpdated += bytesRead;
                if ( threadNum == 0 && conn->fileRestart.writtenSinceUpdated >=
                        RESTART_FILE_UPDATE_SIZE ) {
                    int status;
                    /* time to write to the restart file */
                    status = writeLfRestartFile( conn->fileRestart.infoFile,
                                                 &conn->fileRestart.info );
                    if ( status < 0 ) {
                        rodsLog( LOG_ERROR,
                                 "rcPartialDataPut: writeLfRestartFile for %s, status = %d",
                                 conn->fileRestart.info.fileName, status );
                    }
                    conn->fileRestart.writtenSinceUpdated = 0;
                }
            }

        } // while

        curOffset += myHeader.length;
        myInput->bytesWritten += myHeader.length;
        /* should lock this. But window browser is the only one using it */
        myTransStat->bytesWritten += myHeader.length;
        /* should lock this. but it is info only */
        if ( gGuiProgressCB != NULL ) {
            conn->operProgress.curFileSizeDone += myHeader.length;
            if ( myInput->threadNum == 0 ) {
                gGuiProgressCB( &conn->operProgress );
            }
        }
    }


    free( buf );
    close( srcFd );
    mySockClose( destFd );
}

int
putFile( rcComm_t *conn, int l1descInx, char *locFilePath, char *objPath,
         rodsLong_t dataSize ) {
    int in_fd, status;
    bytesBuf_t dataObjWriteInpBBuf;
    openedDataObjInp_t dataObjWriteInp;
    int bytesWritten;
    rodsLong_t totalWritten = 0;
    int bytesRead;
    int progressCnt = 0;
    fileRestartInfo_t *info = &conn->fileRestart.info;
    rodsLong_t lastUpdateSize = 0;


#ifdef windows_platform
    in_fd = iRODSNt_bopen( locFilePath, O_RDONLY, 0 );
#else
    in_fd = open( locFilePath, O_RDONLY, 0 );
#endif
    if ( in_fd < 0 ) { /* error */
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "cannot open file %s", locFilePath, status );
        return status;
    }

    bzero( &dataObjWriteInp, sizeof( dataObjWriteInp ) );
    dataObjWriteInpBBuf.buf = malloc( TRANS_BUF_SZ );
    dataObjWriteInpBBuf.len = 0;
    dataObjWriteInp.l1descInx = l1descInx;
    initFileRestart( conn, locFilePath, objPath, dataSize, 1 );

    if ( gGuiProgressCB != NULL ) {
        conn->operProgress.flag = 1;
    }

    while ( ( dataObjWriteInpBBuf.len =
                  myRead( in_fd, dataObjWriteInpBBuf.buf, TRANS_BUF_SZ,
                          &bytesRead, NULL ) ) > 0 ) {
        /* Write to the data object */

        dataObjWriteInp.len = dataObjWriteInpBBuf.len;
        bytesWritten = rcDataObjWrite( conn, &dataObjWriteInp,
                                       &dataObjWriteInpBBuf );
        if ( bytesWritten < dataObjWriteInp.len ) {
            rodsLog( LOG_ERROR,
                     "putFile: Read %d bytes, Wrote %d bytes.\n ",
                     dataObjWriteInp.len, bytesWritten );
            free( dataObjWriteInpBBuf.buf );
            close( in_fd );
            return SYS_COPY_LEN_ERR;
        }
        else {
            totalWritten += bytesWritten;
            conn->transStat.bytesWritten = totalWritten;
            if ( info->numSeg > 0 ) {	/* file restart */
                info->dataSeg[0].len += bytesWritten;
                if ( totalWritten - lastUpdateSize >= RESTART_FILE_UPDATE_SIZE ) {
                    /* time to write to the restart file */
                    status = writeLfRestartFile( conn->fileRestart.infoFile,
                                                 &conn->fileRestart.info );
                    if ( status < 0 ) {
                        rodsLog( LOG_ERROR,
                                 "putFile: writeLfRestartFile for %s, status = %d",
                                 locFilePath, status );
                        free( dataObjWriteInpBBuf.buf );
                        close( in_fd );
                        return status;
                    }
                    lastUpdateSize = totalWritten;
                }
            }
            if ( gGuiProgressCB != NULL ) {
                if ( progressCnt >= ( MAX_PROGRESS_CNT - 1 ) ) {
                    conn->operProgress.curFileSizeDone +=
                        ( ( MAX_PROGRESS_CNT - 1 ) * TRANS_BUF_SZ + bytesWritten );
                    gGuiProgressCB( &conn->operProgress );
                    progressCnt = 0;
                }
                else {
                    progressCnt ++;
                }
            }
        }
    }

    free( dataObjWriteInpBBuf.buf );
    close( in_fd );

    if ( dataSize <= 0 || totalWritten == dataSize ) {
        if ( gGuiProgressCB != NULL ) {
            conn->operProgress.curFileSizeDone = conn->operProgress.curFileSize;
            gGuiProgressCB( &conn->operProgress );
        }
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "putFile: totalWritten %lld dataSize %lld mismatch",
                 totalWritten, dataSize );
        return SYS_COPY_LEN_ERR;
    }
}

int
getIncludeFile( rcComm_t *conn, bytesBuf_t *dataObjOutBBuf, char *locFilePath ) {
    int status, out_fd, bytesWritten;

    if ( strcmp( locFilePath, STDOUT_FILE_NAME ) == 0 ) {
        if ( dataObjOutBBuf->len <= 0 ) {
            return 0;
        }
        bytesWritten = fwrite( dataObjOutBBuf->buf, dataObjOutBBuf->len,
                               1, stdout );
        if ( bytesWritten == 1 ) {
            bytesWritten = dataObjOutBBuf->len;
        }
    }
    else {
#ifdef windows_platform
        out_fd = iRODSNt_bopen( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
#else
        out_fd = open( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
#endif
        if ( out_fd < 0 ) { /* error */
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLogError( LOG_ERROR, status,
                          "cannot open file %s", locFilePath, status );
            return status;
        }

        if ( dataObjOutBBuf->len <= 0 ) {
            close( out_fd );
            return 0;
        }

        bytesWritten = myWrite( out_fd, dataObjOutBBuf->buf,
                                dataObjOutBBuf->len, NULL );

        close( out_fd );
    }
    if ( bytesWritten != dataObjOutBBuf->len ) {
        rodsLog( LOG_ERROR,
                 "getIncludeFile: Read %d bytes, Wrote %d bytes. errno = %d\n ",
                 dataObjOutBBuf->len, bytesWritten, errno );
        return SYS_COPY_LEN_ERR;
    }
    else {
        conn->transStat.bytesWritten = bytesWritten;
        return 0;
    }
}

int
getFile( rcComm_t *conn, int l1descInx, char *locFilePath, char *objPath,
         rodsLong_t dataSize ) {
    int out_fd, status;
    bytesBuf_t dataObjReadInpBBuf;
    openedDataObjInp_t dataObjReadInp;
    int bytesWritten, bytesRead;
    rodsLong_t totalWritten = 0;
    int progressCnt = 0;
    fileRestartInfo_t *info = &conn->fileRestart.info;
    rodsLong_t lastUpdateSize = 0;

    if ( strcmp( locFilePath, STDOUT_FILE_NAME ) == 0 ) {
        /* streaming to stdout */
        out_fd = 1;
    }
    else {
#ifdef windows_platform
        out_fd = iRODSNt_bopen( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
#else
        out_fd = open( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
#endif
    }

    if ( out_fd < 0 ) { /* error */
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "cannot open file %s", locFilePath, status );
        return status;
    }

    bzero( &dataObjReadInp, sizeof( dataObjReadInp ) );
    dataObjReadInpBBuf.buf = malloc( TRANS_BUF_SZ );
    dataObjReadInpBBuf.len = dataObjReadInp.len = TRANS_BUF_SZ;
    dataObjReadInp.l1descInx = l1descInx;
    initFileRestart( conn, locFilePath, objPath, dataSize, 1 );

    if ( gGuiProgressCB != NULL ) {
        conn->operProgress.flag = 1;
    }

    while ( ( bytesRead = rcDataObjRead( conn, &dataObjReadInp,
                                         &dataObjReadInpBBuf ) ) > 0 ) {

        if ( out_fd == 1 ) {
            bytesWritten = fwrite( dataObjReadInpBBuf.buf, bytesRead,
                                   1, stdout );
            if ( bytesWritten == 1 ) {
                bytesWritten = bytesRead;
            }
        }
        else {
            bytesWritten = myWrite( out_fd, dataObjReadInpBBuf.buf,
                                    bytesRead, NULL );
        }

        if ( bytesWritten != bytesRead ) {
            rodsLog( LOG_ERROR,
                     "getFile: Read %d bytes, Wrote %d bytes.\n ",
                     bytesRead, bytesWritten );
            free( dataObjReadInpBBuf.buf );
            if ( out_fd != 1 ) {
                close( out_fd );
            }
            return SYS_COPY_LEN_ERR;
        }
        else {
            totalWritten += bytesWritten;
            conn->transStat.bytesWritten = totalWritten;
            if ( info->numSeg > 0 ) {   /* file restart */
                info->dataSeg[0].len += bytesWritten;
                if ( totalWritten - lastUpdateSize >= RESTART_FILE_UPDATE_SIZE ) {
                    /* time to write to the restart file */
                    status = writeLfRestartFile( conn->fileRestart.infoFile,
                                                 &conn->fileRestart.info );
                    if ( status < 0 ) {
                        rodsLog( LOG_ERROR,
                                 "getFile: writeLfRestartFile for %s, status = %d",
                                 locFilePath, status );
                        free( dataObjReadInpBBuf.buf );
                        if ( out_fd != 1 ) {
                            close( out_fd );
                        }
                        return status;
                    }
                    lastUpdateSize = totalWritten;
                }
            }
            if ( gGuiProgressCB != NULL ) {
                if ( progressCnt >= ( MAX_PROGRESS_CNT - 1 ) ) {
                    conn->operProgress.curFileSizeDone +=
                        ( ( MAX_PROGRESS_CNT - 1 ) * TRANS_BUF_SZ + bytesWritten );
                    gGuiProgressCB( &conn->operProgress );
                    progressCnt = 0;
                }
                else {
                    progressCnt ++;
                }
            }
        }
    }

    free( dataObjReadInpBBuf.buf );
    if ( out_fd != 1 ) {
        close( out_fd );
    }

    if ( bytesRead >= 0 ) {
        if ( gGuiProgressCB != NULL ) {
            conn->operProgress.curFileSizeDone = conn->operProgress.curFileSize;
            gGuiProgressCB( &conn->operProgress );
        }
        return 0;
    }
    else {
        rodsLog( LOG_ERROR,
                 "getFile: totalWritten %lld dataSize %lld mismatch",
                 totalWritten, dataSize );
        return bytesRead;

    }
}

int
getFileFromPortal( rcComm_t *conn, portalOprOut_t *portalOprOut,
                   char *locFilePath, char *objPath, rodsLong_t dataSize ) {
    portList_t *myPortList;
    int i, sock, out_fd;
    int numThreads;
    rcPortalTransferInp_t myInput[MAX_NUM_CONFIG_TRAN_THR];
    boost::thread* tid[MAX_NUM_CONFIG_TRAN_THR];
    int retVal = 0;

    if ( portalOprOut == NULL || portalOprOut->numThreads <= 0 ) {
        rodsLog( LOG_ERROR,
                 "getFileFromPortal: invalid portalOprOut" );
        return SYS_INVALID_PORTAL_OPR;
    }

    numThreads = portalOprOut->numThreads;

    myPortList = &portalOprOut->portList;

    if ( portalOprOut->numThreads > MAX_NUM_CONFIG_TRAN_THR ) {
        /* drain the connection or it will be stuck */
        for ( i = 0; i < numThreads; i++ ) {
            sock = connectToRhostPortal( myPortList->hostAddr,
                                         myPortList->portNum, myPortList->cookie, myPortList->windowSize );
            if ( sock > 0 ) {
                close( sock );
            }
        }
        rodsLog( LOG_ERROR,
                 "getFileFromPortal: numThreads %d too large", numThreads );
        return SYS_INVALID_PORTAL_OPR;
    }

    memset( tid, 0, sizeof( tid ) );
    memset( myInput, 0, sizeof( myInput ) );

    initFileRestart( conn, locFilePath, objPath, dataSize,
                     portalOprOut->numThreads );

    if ( numThreads == 1 ) {
        sock = connectToRhostPortal( myPortList->hostAddr,
                                     myPortList->portNum, myPortList->cookie, myPortList->windowSize );
        if ( sock < 0 ) {
            return sock;
        }
#ifdef windows_platform
        out_fd = iRODSNt_bopen( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
#else
        out_fd = open( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
#endif
        if ( out_fd < 0 ) { /* error */
            retVal = UNIX_FILE_OPEN_ERR - errno;
            rodsLogError( LOG_ERROR, retVal,
                          "cannot open file %s", locFilePath, retVal );
            return retVal;
        }
        fillRcPortalTransferInp( conn, &myInput[0], out_fd, sock, 0640 );
        rcPartialDataGet( &myInput[0] );
        if ( myInput[0].status < 0 ) {
            return myInput[0].status;
        }
        else {
            if ( dataSize <= 0 || myInput[0].bytesWritten == dataSize ) {
                return 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "getFileFromPortal:bytesWritten %lld dataSize %lld mismatch",
                         myInput[0].bytesWritten, dataSize );
                return SYS_COPY_LEN_ERR;
            }
        }
    }
    else {
        rodsLong_t totalWritten = 0;

        for ( i = 0; i < numThreads; i++ ) {
            sock = connectToRhostPortal( myPortList->hostAddr,
                                         myPortList->portNum, myPortList->cookie, myPortList->windowSize );
            if ( sock < 0 ) {
                return sock;
            }
            if ( i == 0 ) {
                out_fd = open( locFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0640 );
            }
            else {
                out_fd = open( locFilePath, O_WRONLY, 0640 );
            }
            if ( out_fd < 0 ) {  /* error */
                retVal = UNIX_FILE_OPEN_ERR - errno;
                rodsLogError( LOG_ERROR, retVal,
                              "cannot open file %s", locFilePath, retVal );
                CLOSE_SOCK( sock );
                continue;
            }
            fillRcPortalTransferInp( conn, &myInput[i], out_fd, sock, i );
            try {
                tid[i] = new boost::thread( rcPartialDataGet, &myInput[i] );
            }
            catch ( const boost::thread_resource_error& ) {
                rodsLog( LOG_ERROR, "boost encountered thread_resource_error on constructing thread." );
                return SYS_THREAD_RESOURCE_ERR;
            }
        }

        if ( retVal < 0 ) {
            return retVal;
        }

        for ( i = 0; i < numThreads; i++ ) {
            if ( tid[i] != 0 ) {
                try {
                    tid[i]->join();
                }
                catch ( const boost::thread_resource_error& ) {
                    rodsLog( LOG_ERROR, "boost encountered thread_resource_error on join." );
                    return SYS_THREAD_RESOURCE_ERR;
                }
            }
            totalWritten += myInput[i].bytesWritten;
            if ( myInput[i].status < 0 ) {
                retVal = myInput[i].status;
            }
        }
        if ( retVal < 0 ) {
            return retVal;
        }
        else {
            if ( dataSize <= 0 || totalWritten == dataSize ) {
                if ( gGuiProgressCB != NULL ) {
                    gGuiProgressCB( &conn->operProgress );
                }
                return 0;
            }
            else {
                rodsLog( LOG_ERROR,
                         "getFileFromPortal: totalWritten %lld dataSize %lld mismatch",
                         totalWritten, dataSize );
                return SYS_COPY_LEN_ERR;
            }
        }
    }
}

void
rcPartialDataGet( rcPortalTransferInp_t *myInput ) {
    transferHeader_t myHeader;
    int destFd;
    int srcFd;
    unsigned char *buf;
    transferStat_t *myTransStat;
    rodsLong_t curOffset = 0;
    rcComm_t *conn;
    fileRestartInfo_t *info;
    int threadNum;
    if ( myInput == NULL ) {
        rodsLog( LOG_ERROR,
                 "rcPartialDataGet: NULL input" );
        return;
    }

#ifdef PARA_DEBUG
    printf( "rcPartialDataGet: thread %d at start\n", myInput->threadNum );
#endif
    conn = myInput->conn;
    info = &conn->fileRestart.info;
    threadNum = myInput->threadNum;

    myTransStat = &myInput->conn->transStat;

    destFd = myInput->destFd;
    srcFd = myInput->srcFd;


    myInput->bytesWritten = 0;

    if ( gGuiProgressCB != NULL ) {
        conn = myInput->conn;
        conn->operProgress.flag = 1;
    }

    // =-=-=-=-=-=-=-
    // flag to determine if we need to use encryption
    bool use_encryption_flg =
        ( myInput->conn->negotiation_results ==
          irods::CS_NEG_USE_SSL );

    // =-=-=-=-=-=-=-
    // get the client side Env to determine
    // encryption parameters
    rodsEnv rods_env;
    int status = getRodsEnv( &rods_env );
    if ( status < 0 ) {
        printf( "Failed to get irodsEnv" );
        return;
    }

    // =-=-=-=-=-=-=-
    // create an encryption context
    int iv_size = 0;
    irods::buffer_crypt::array_t iv;
    irods::buffer_crypt::array_t this_iv;
    irods::buffer_crypt::array_t cipher;
    irods::buffer_crypt::array_t plain;
    irods::buffer_crypt::array_t shared_secret;
    irods::buffer_crypt crypt(
        rods_env.rodsEncryptionKeySize,
        rods_env.rodsEncryptionSaltSize,
        rods_env.rodsEncryptionNumHashRounds,
        rods_env.rodsEncryptionAlgorithm );

    // =-=-=-=-=-=-=-
    // set iv size
    if ( use_encryption_flg ) {
        iv_size = crypt.key_size();
        shared_secret.assign(
            &myInput->shared_secret[0],
            &myInput->shared_secret[iv_size] );
    }

    size_t buf_size = ( 2 * TRANS_BUF_SZ ) * sizeof( unsigned char );
    buf = ( unsigned char* )malloc( buf_size );

    while ( myInput->status >= 0 ) {
        rodsLong_t toGet;

        myInput->status = rcvTranHeader( srcFd, &myHeader );

#ifdef PARA_DEBUG
        printf( "rcPartialDataGet: thread %d after rcvTranHeader\n",
                myInput->threadNum );
#endif

        if ( myInput->status < 0 ) {
            break;
        }

        if ( myHeader.oprType == DONE_OPR ) {
            break;
        }
        if ( myHeader.offset != curOffset ) {
            curOffset = myHeader.offset;
            if ( lseek( destFd, curOffset, SEEK_SET ) < 0 ) {
                myInput->status = UNIX_FILE_LSEEK_ERR - errno;
                rodsLogError( LOG_ERROR, myInput->status,
                              "rcPartialDataGet: lseek to %lld error, status = %d",
                              curOffset, myInput->status );
                break;
            }
            if ( info->numSeg > 0 ) {   /* file restart */
                info->dataSeg[threadNum].offset = curOffset;
            }
        }

        toGet = myHeader.length;
        while ( toGet > 0 ) {
            int toRead, bytesRead, bytesWritten;

            if ( toGet > TRANS_BUF_SZ ) {
                toRead = TRANS_BUF_SZ;
            }
            else {
                toRead = toGet;
            }

            // =-=-=-=-=-=-=-
            // read the incoming size as it might differ due to encryption
            int new_size = toRead;
            if ( use_encryption_flg ) {
                bytesRead = myRead(
                                srcFd,
                                &new_size,
                                sizeof( int ),
                                NULL, NULL );
                if ( bytesRead != sizeof( int ) ) {
                    rodsLog(
                        LOG_ERROR,
                        "_partialDataGet:Bytes Read != %d",
                        sizeof( int ) );
                    break;
                }
            }

            // =-=-=-=-=-=-=-
            // now read the provided number of bytes as suggested by
            // the incoming size
            bytesRead = myRead(
                            srcFd,
                            buf,
                            new_size,
                            &bytesRead,
                            NULL );
            if ( bytesRead != new_size ) {
                myInput->status = SYS_COPY_LEN_ERR - errno;
                rodsLogError( LOG_ERROR, myInput->status,
                              "rcPartialDataGet: toGet %lld, bytesRead %d",
                              toGet, bytesRead );
                break;
            }

            // =-=-=-=-=-=-=-
            // if using encryption, strip off the iv
            // and decrypt before writing
            int plain_size = bytesRead;
            if ( use_encryption_flg ) {
                this_iv.assign(
                    &buf[ 0 ],
                    &buf[ iv_size ] );
                cipher.assign(
                    &buf[ iv_size ],
                    &buf[ new_size ] );
                irods::error ret = crypt.decrypt(
                                       shared_secret,
                                       this_iv,
                                       cipher,
                                       plain );
                if ( !ret.ok() ) {
                    irods::log( PASS( ret ) );
                    myInput->status = SYS_COPY_LEN_ERR;
                    break;
                }

                memset( buf, 0, buf_size );
                std::copy(
                    plain.begin(),
                    plain.end(),
                    &buf[0] );
                plain_size = plain.size();

            }

            bytesWritten = myWrite(
                               destFd,
                               buf,
                               plain_size,
                               &bytesWritten );
            if ( bytesWritten != plain_size ) {
                myInput->status = SYS_COPY_LEN_ERR - errno;
                rodsLogError( LOG_ERROR, myInput->status,
                              "rcPartialDataGet: toWrite %d, bytesWritten %d",
                              plain_size, bytesWritten );
                break;
            }

            toGet -= bytesWritten;

            if ( info->numSeg > 0 ) {   /* file restart */
                info->dataSeg[threadNum].len += bytesWritten;
                conn->fileRestart.writtenSinceUpdated += bytesWritten;
                if ( threadNum == 0 && conn->fileRestart.writtenSinceUpdated >=
                        RESTART_FILE_UPDATE_SIZE ) {
                    int status;
                    /* time to write to the restart file */
                    status = writeLfRestartFile( conn->fileRestart.infoFile,
                                                 &conn->fileRestart.info );
                    if ( status < 0 ) {
                        rodsLog( LOG_ERROR,
                                 "rcPartialDataGet: writeLfRestartFile for %s, status = %d",
                                 conn->fileRestart.info.fileName, status );
                    }
                    conn->fileRestart.writtenSinceUpdated = 0;
                }
            }
        }
        curOffset += myHeader.length;
        myInput->bytesWritten += myHeader.length;
        /* should lock this. But window browser is the only one using it */
        myTransStat->bytesWritten += myHeader.length;
        /* should lock this. but it is info only */
        if ( gGuiProgressCB != NULL ) {
            conn->operProgress.curFileSizeDone += myHeader.length;
            if ( myInput->threadNum == 0 ) {
                gGuiProgressCB( &conn->operProgress );
            }
        }
    }

    free( buf );
    close( destFd );
    CLOSE_SOCK( srcFd );
}

/* putFileToPortalRbudp - The client side of putting a file using
 * Rbudp. If locFilePath is NULL, the local file has already been opned
 * and locFd should be used. If sendRate and packetSize are 0, it will
 * try to set it based on env and default.
 */
int putFileToPortalRbudp(
    portalOprOut_t*      portalOprOut,
    char*                locFilePath,
    int                  locFd,
    int                  veryVerbose,
    int                  sendRate,
    int                  packetSize ) {

    portList_t *myPortList;
    int status;
    rbudpSender_t rbudpSender;
    int mysendRate, mypacketSize;
    char *tmpStr;

    if ( portalOprOut == NULL || portalOprOut->numThreads != 1 ) {
        rodsLog( LOG_ERROR,
                 "putFileToPortal: invalid portalOprOut" );
        return SYS_INVALID_PORTAL_OPR;
    }

    myPortList = &portalOprOut->portList;

    bzero( &rbudpSender, sizeof( rbudpSender ) );
    status = initRbudpClient( &rbudpSender.rbudpBase, myPortList );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "putFileToPortalRbudp: initRbudpClient error for %s",
                 myPortList->hostAddr );
        return status;
    }
    rbudpSender.rbudpBase.verbose = veryVerbose;
    if ( ( mysendRate = sendRate ) < 1 &&
            ( ( tmpStr = getenv( RBUDP_SEND_RATE_KW ) ) == NULL ||
              ( mysendRate = atoi( tmpStr ) ) < 1 ) ) {
        mysendRate = DEF_UDP_SEND_RATE;
    }
    if ( ( mypacketSize = packetSize ) < 1 &&
            ( ( tmpStr = getenv( RBUDP_PACK_SIZE_KW ) ) == NULL ||
              ( mypacketSize = atoi( tmpStr ) ) < 1 ) ) {
        mypacketSize = DEF_UDP_PACKET_SIZE;
    }

    if ( locFilePath == NULL ) {
        status = sendfileByFd(
                     &rbudpSender,
                     mysendRate,
                     mypacketSize,
                     locFd );
    }
    else {
        status = rbSendfile(
                     &rbudpSender,
                     mysendRate,
                     mypacketSize,
                     locFilePath );
    }

    sendClose( &rbudpSender );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "putFileToPortalRbudp: sendfile error for %s:%d", // JMC - backport 4590
                 myPortList->hostAddr, myPortList->portNum & 0xffff0000 );
        return status;
    }
    return status;
}

/* getFileToPortalRbudp - The client side of getting a file using
 * Rbudp. If locFilePath is NULL, the local file has already been opned
 * and locFd should be used. If sendRate and packetSize are 0, it will
 * try to set it based on env and default.
 */
int getFileToPortalRbudp(
    portalOprOut_t* portalOprOut,
    char*           locFilePath,
    int             locFd,
    int             veryVerbose,
    int             packetSize ) {


    portList_t *myPortList;
    int status;
    rbudpReceiver_t rbudpReceiver;
    int mypacketSize;
    char *tmpStr;

    if ( portalOprOut == NULL || portalOprOut->numThreads != 1 ) {
        rodsLog( LOG_ERROR,
                 "getFileToPortalRbudp: invalid portalOprOut" );
        return SYS_INVALID_PORTAL_OPR;
    }

    myPortList = &portalOprOut->portList;

    bzero( &rbudpReceiver, sizeof( rbudpReceiver ) );
    status = initRbudpClient( &rbudpReceiver.rbudpBase, myPortList );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getFileToPortalRbudp: initRbudpClient error for %s",
                 myPortList->hostAddr );
        return status;
    }

    rbudpReceiver.rbudpBase.verbose = veryVerbose;

    if ( ( mypacketSize = packetSize ) < 1 &&
            ( ( tmpStr = getenv( RBUDP_PACK_SIZE_KW ) ) == NULL ||
              ( mypacketSize = atoi( tmpStr ) ) < 1 ) ) {
        mypacketSize = DEF_UDP_PACKET_SIZE;
    }

    rodsEnv rods_env;
    status = getRodsEnv( &rods_env );
    if ( status < 0 ) {
        return status;
    }

    if ( locFilePath == NULL ) {
        status = getfileByFd(
                     &rbudpReceiver,
                     locFd,
                     mypacketSize );

    }
    else {
        status = getfile(
                     &rbudpReceiver,
                     NULL,
                     locFilePath,
                     mypacketSize );
    }

    recvClose( &rbudpReceiver );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getFileToPortalRbudp: getfile error for %s",
                 myPortList->hostAddr );
        return status;
    }
    return status;
}

int
initRbudpClient( rbudpBase_t *rbudpBase, portList_t *myPortList ) {
    int  tcpSock;
    int tcpPort, udpPort;
    int status;
    struct sockaddr_in localUdpAddr;
    int udpLocalPort;

    if ( ( udpPort = getUdpPortFromPortList( myPortList ) ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "putFileToPortalRbudp: udpPort == 0" );
        return SYS_INVALID_PORTAL_OPR;
    }

    tcpPort = getTcpPortFromPortList( myPortList );

    tcpSock = connectToRhostPortal( myPortList->hostAddr,
                                    tcpPort, myPortList->cookie, myPortList->windowSize );
    if ( tcpSock < 0 ) {
        return tcpSock;
    }

    rbudpBase->udpSockBufSize = UDPSOCKBUF;
    rbudpBase->tcpPort = tcpPort;
    rbudpBase->tcpSockfd = tcpSock;
    rbudpBase->hasTcpSock = 0;	/* so it will close properly */
    rbudpBase->udpRemotePort = udpPort;

    /* connect to the server's UDP port */
    status = passiveUDP( rbudpBase, myPortList->hostAddr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initRbudpClient: passiveUDP connect to %s error. status = %d",
                 myPortList->hostAddr, status );
        return SYS_UDP_CONNECT_ERR + status;
    }

    /* inform the server of the UDP port */
    rbudpBase->udpLocalPort =
        setLocalAddr( rbudpBase->udpSockfd, &localUdpAddr );
    if ( rbudpBase->udpLocalPort < 0 ) {
        return rbudpBase->udpLocalPort;
    }
    udpLocalPort = htonl( rbudpBase->udpLocalPort );
    status = writen( rbudpBase->tcpSockfd, ( char * ) &udpLocalPort,
                     sizeof( udpLocalPort ) );
    if ( status != sizeof( udpLocalPort ) ) {
        rodsLog( LOG_ERROR,
                 "initRbudpClient: writen error. towrite %d, bytes written %d ",
                 sizeof( udpLocalPort ), status );
        return SYS_UDP_CONNECT_ERR;
    }

    return 0;
}

int
initFileRestart( rcComm_t *conn, char *fileName, char *objPath,
                 rodsLong_t fileSize, int numThr ) {
    fileRestart_t *fileRestart = &conn->fileRestart;
    fileRestartInfo_t *info = &fileRestart->info;

    if ( fileRestart->flags != FILE_RESTART_ON ||
            fileSize < MIN_RESTART_SIZE || numThr <= 0 ) {
        info->numSeg = 0;	/* indicate no restart */
        return 0;
    }
    if ( numThr > MAX_NUM_CONFIG_TRAN_THR ) {
        rodsLog( LOG_NOTICE,
                 "initFileRestart: input numThr %d larger than max %d ",
                 numThr, MAX_NUM_CONFIG_TRAN_THR );
        info->numSeg = 0;	/* indicate no restart */
        return 0;
    }
    info->numSeg = numThr;
    info->fileSize = fileSize;
    rstrcpy( info->fileName, fileName, MAX_NAME_LEN );
    rstrcpy( info->objPath, objPath, MAX_NAME_LEN );
    bzero( info->dataSeg, sizeof( dataSeg_t ) * MAX_NUM_CONFIG_TRAN_THR );
    return 0;
}

int
writeLfRestartFile( char *infoFile, fileRestartInfo_t *info ) {
    bytesBuf_t *packedBBuf = NULL;
    int status, fd;

    status =  packStruct( ( void * ) info, &packedBBuf,
                          "FileRestartInfo_PI", RodsPackTable, 0, XML_PROT );
    if ( status < 0 || packedBBuf == NULL ) {
        rodsLog( LOG_ERROR,
                 "writeLfRestartFile: packStruct error for %s, status = %d",
                 info->fileName, status );
        freeBBuf( packedBBuf );
        return status;
    }

    /* write it to a file */
    fd = open( infoFile, O_CREAT | O_TRUNC | O_WRONLY, 0640 );
    if ( fd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_ERROR,
                 "writeLfRestartFile: open failed for %s, status = %d",
                 infoFile, status );
        freeBBuf( packedBBuf );
        return status;
    }

    status = write( fd, packedBBuf->buf, packedBBuf->len );
    close( fd );

    freeBBuf( packedBBuf );
    if ( status < 0 ) {
        status = UNIX_FILE_WRITE_ERR - errno;
        rodsLog( LOG_ERROR,
                 "writeLfRestartFile: write failed for %s, status = %d",
                 infoFile, status );
        return status;
    }
    return status;
}

int
readLfRestartFile( char *infoFile, fileRestartInfo_t **info ) {
    int status, fd;
    rodsLong_t mySize;
    char *buf;

    *info = NULL;
    path p( infoFile );
    if ( !exists( p ) || !is_regular_file( p ) ) {
        status = UNIX_FILE_STAT_ERR - errno;
        return status;
    }
    else if ( ( mySize = file_size( p ) ) <= 0 ) {
        status = UNIX_FILE_STAT_ERR - errno;
        rodsLog( LOG_ERROR,
                 "readLfRestartFile restart infoFile size is 0 for %s",
                 infoFile );
        return status;
    }

    /* read the restart infoFile */
    fd = open( infoFile, O_RDONLY, 0640 );
    if ( fd < 0 ) {
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLog( LOG_ERROR,
                 "readLfRestartFile open failed for %s, status = %d",
                 infoFile, status );
        return status;
    }

    buf = ( char * ) calloc( 1, 2 * mySize );
    if ( buf == NULL ) {
        close( fd );
        return SYS_MALLOC_ERR;
    }
    status = read( fd, buf, mySize );
    if ( status != mySize ) {
        rodsLog( LOG_ERROR,
                 "readLfRestartFile error failed for %s, toread %d, read %d",
                 infoFile, mySize, status );
        status = UNIX_FILE_READ_ERR - errno;
        close( fd );
        free( buf );
        return status;
    }

    close( fd );

    status =  unpackStruct( buf, ( void ** ) info, "FileRestartInfo_PI", NULL, XML_PROT );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "readLfRestartFile: unpackStruct error for %s, status = %d",
                 infoFile, status );
    }
    free( buf );
    return status;
}


int
clearLfRestartFile( fileRestart_t *fileRestart ) {
    unlink( fileRestart->infoFile );
    bzero( &fileRestart->info, sizeof( fileRestartInfo_t ) );

    return 0;
}

int
lfRestartPutWithInfo( rcComm_t *conn, fileRestartInfo_t *info ) {
    rodsLong_t curOffset = 0;
    bytesBuf_t dataObjWriteInpBBuf;
    int status = 0, i = 0;
    int localFd = 0, irodsFd = 0;
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjWriteInp;
    openedDataObjInp_t dataObjLseekInp;
    openedDataObjInp_t dataObjCloseInp;
    fileLseekOut_t *dataObjLseekOut = NULL;
    int writtenSinceUpdated = 0;
    rodsLong_t gap;

#ifdef windows_platform
    localFd = iRODSNt_bopen( info->fileName, O_RDONLY, 0 );
#else
    localFd = open( info->fileName, O_RDONLY, 0 );
#endif
    if ( localFd < 0 ) { /* error */
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "cannot open file %s", info->fileName, status );
        return status;
    }

    bzero( &dataObjOpenInp, sizeof( dataObjOpenInp ) );
    rstrcpy( dataObjOpenInp.objPath, info->objPath, MAX_NAME_LEN );
    dataObjOpenInp.openFlags = O_WRONLY;
    addKeyVal( &dataObjOpenInp.condInput, FORCE_FLAG_KW, "" );

    irodsFd = rcDataObjOpen( conn, &dataObjOpenInp );
    if ( irodsFd < 0 ) { /* error */
        rodsLogError( LOG_ERROR, irodsFd,
                      "cannot open target file %s, status = %d", info->objPath, irodsFd );
        close( localFd );
        return irodsFd;
    }

    bzero( &dataObjWriteInp, sizeof( dataObjWriteInp ) );
    dataObjWriteInpBBuf.buf = malloc( TRANS_BUF_SZ );
    dataObjWriteInpBBuf.len = 0;
    dataObjWriteInp.l1descInx = irodsFd;

    memset( &dataObjLseekInp, 0, sizeof( dataObjLseekInp ) );
    dataObjLseekInp.whence = SEEK_SET;
    for ( i = 0; i < info->numSeg; i++ ) {
        gap = info->dataSeg[i].offset - curOffset;
        if ( gap < 0 ) {
            /* should not be here */
        }
        else if ( gap > 0 ) {
            rodsLong_t tmpLen, *lenToUpdate;
            if ( i == 0 ) {
                /* should not be here */
                tmpLen = 0;
                lenToUpdate = &tmpLen;
            }
            else {
                lenToUpdate = &info->dataSeg[i - 1].len;
            }

            status = putSeg( conn, gap, localFd, &dataObjWriteInp,
                             &dataObjWriteInpBBuf, TRANS_BUF_SZ, &writtenSinceUpdated,
                             info, lenToUpdate );
            if ( status < 0 ) {
                break;
            }
            curOffset += gap;
        }
        if ( info->dataSeg[i].len > 0 ) {
            curOffset += info->dataSeg[i].len;
            if ( lseek( localFd, curOffset, SEEK_SET ) < 0 ) {
                status = UNIX_FILE_LSEEK_ERR - errno;
                rodsLogError( LOG_ERROR, status,
                              "lfRestartWithInfo: lseek to %lld error for %s",
                              curOffset, info->fileName );
                break;
            }
            dataObjLseekInp.l1descInx = irodsFd;
            dataObjLseekInp.offset = curOffset;
            status = rcDataObjLseek( conn, &dataObjLseekInp, &dataObjLseekOut );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "lfRestartWithInfo: rcDataObjLseek to %lld error for %s",
                              curOffset, info->objPath );
                break;
            }
            else {
                if ( dataObjLseekOut != NULL ) {
                    free( dataObjLseekOut );
                }
            }
        }
    }
    if ( status >= 0 ) {
        gap = info->fileSize - curOffset;
        if ( gap > 0 ) {
            status = putSeg( conn, gap, localFd, &dataObjWriteInp,
                             &dataObjWriteInpBBuf, TRANS_BUF_SZ,  &writtenSinceUpdated,
                             info, &info->dataSeg[i - 1].len );
        }
    }
    free( dataObjWriteInpBBuf.buf );
    close( localFd );
    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = irodsFd;
    rcDataObjClose( conn, &dataObjCloseInp );
    return status;
}

int
putSeg( rcComm_t *conn, rodsLong_t segSize, int localFd,
        openedDataObjInp_t *dataObjWriteInp, bytesBuf_t *dataObjWriteInpBBuf,
        int bufLen, int *writtenSinceUpdated, fileRestartInfo_t *info,
        rodsLong_t *dataSegLen ) {
    rodsLong_t gap = segSize;
    int bytesWritten;
    int status;

    while ( gap > 0 ) {
        int toRead;
        if ( gap > bufLen ) {
            toRead = bufLen;
        }
        else {
            toRead = ( int ) gap;
        }

        dataObjWriteInpBBuf->len = myRead( localFd,
                                           dataObjWriteInpBBuf->buf, toRead, NULL, NULL );
        /* Write to the data object */
        dataObjWriteInp->len = dataObjWriteInpBBuf->len;
        bytesWritten = rcDataObjWrite( conn, dataObjWriteInp,
                                       dataObjWriteInpBBuf );
        if ( bytesWritten < dataObjWriteInp->len ) {
            rodsLog( LOG_ERROR,
                     "putFile: Read %d bytes, Wrote %d bytes.\n ",
                     dataObjWriteInp->len, bytesWritten );
            return SYS_COPY_LEN_ERR;
        }
        else {
            gap -= toRead;
            *writtenSinceUpdated += toRead;
            *dataSegLen += toRead;
            if ( *writtenSinceUpdated >= RESTART_FILE_UPDATE_SIZE ) {
                status = writeLfRestartFile( conn->fileRestart.infoFile, info );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                             "putSeg: writeLfRestartFile for %s, status = %d",
                             info->fileName, status );
                    return status;
                }
                *writtenSinceUpdated = 0;
            }
        }
    }
    return 0;
}

int
lfRestartGetWithInfo( rcComm_t *conn, fileRestartInfo_t *info ) {
    rodsLong_t curOffset = 0;
    bytesBuf_t dataObjReadInpBBuf;
    int status = 0, i = 0;
    int localFd = 0, irodsFd = 0;
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjReadInp;
    openedDataObjInp_t dataObjLseekInp;
    openedDataObjInp_t dataObjCloseInp;
    fileLseekOut_t *dataObjLseekOut = NULL;
    int writtenSinceUpdated = 0;
    rodsLong_t gap;

#ifdef windows_platform
    localFd = iRODSNt_bopen( info->fileName, O_RDONLY, 0 );
#else
    localFd = open( info->fileName, O_RDWR, 0 );
#endif
    if ( localFd < 0 ) { /* error */
        status = UNIX_FILE_OPEN_ERR - errno;
        rodsLogError( LOG_ERROR, status,
                      "cannot open local file %s, status = %d", info->fileName, status );
        return status;
    }

    bzero( &dataObjOpenInp, sizeof( dataObjOpenInp ) );
    rstrcpy( dataObjOpenInp.objPath, info->objPath, MAX_NAME_LEN );
    dataObjOpenInp.openFlags = O_RDONLY;
    irodsFd = rcDataObjOpen( conn, &dataObjOpenInp );
    if ( irodsFd < 0 ) { /* error */
        rodsLogError( LOG_ERROR, irodsFd,
                      "cannot open iRODS src file %s, status = %d", info->objPath, irodsFd );
        close( localFd );
        return irodsFd;
    }

    bzero( &dataObjReadInp, sizeof( dataObjReadInp ) );
    dataObjReadInpBBuf.buf = malloc( TRANS_BUF_SZ );
    dataObjReadInpBBuf.len = 0;
    dataObjReadInp.l1descInx = irodsFd;

    memset( &dataObjLseekInp, 0, sizeof( dataObjLseekInp ) );
    dataObjLseekInp.whence = SEEK_SET;
    dataObjLseekInp.l1descInx = irodsFd;

    for ( i = 0; i < info->numSeg; i++ ) {
        gap = info->dataSeg[i].offset - curOffset;
        if ( gap < 0 ) {
            /* should not be here */
        }
        else if ( gap > 0 ) {
            rodsLong_t tmpLen, *lenToUpdate;
            if ( i == 0 ) {
                /* should not be here */
                tmpLen = 0;
                lenToUpdate = &tmpLen;
            }
            else {
                lenToUpdate = &info->dataSeg[i - 1].len;
            }
            status = getSeg( conn, gap, localFd, &dataObjReadInp,
                             &dataObjReadInpBBuf, TRANS_BUF_SZ, &writtenSinceUpdated,
                             info, lenToUpdate );
            if ( status < 0 ) {
                break;
            }
            curOffset += gap;
        }
        if ( info->dataSeg[i].len > 0 ) {
            curOffset += info->dataSeg[i].len;
            if ( lseek( localFd, curOffset, SEEK_SET ) < 0 ) {
                status = UNIX_FILE_LSEEK_ERR - errno;
                rodsLogError( LOG_ERROR, status,
                              "lfRestartWithInfo: lseek to %lld error for %s",
                              curOffset, info->fileName );
                break;
            }
            dataObjLseekInp.offset = curOffset;
            status = rcDataObjLseek( conn, &dataObjLseekInp, &dataObjLseekOut );
            if ( status < 0 ) {
                rodsLogError( LOG_ERROR, status,
                              "lfRestartWithInfo: rcDataObjLseek to %lld error for %s",
                              curOffset, info->objPath );
                break;
            }
            else {
                if ( dataObjLseekOut != NULL ) {
                    free( dataObjLseekOut );
                }
            }
        }
    }
    if ( status >= 0 ) {
        gap = info->fileSize - curOffset;
        if ( gap > 0 ) {
            status = getSeg( conn, gap, localFd, &dataObjReadInp,
                             &dataObjReadInpBBuf, TRANS_BUF_SZ,  &writtenSinceUpdated,
                             info, &info->dataSeg[i - 1].len );
        }
    }
    free( dataObjReadInpBBuf.buf );
    close( localFd );
    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = irodsFd;
    rcDataObjClose( conn, &dataObjCloseInp );
    return status;
}

int
getSeg( rcComm_t *conn, rodsLong_t segSize, int localFd,
        openedDataObjInp_t *dataObjReadInp, bytesBuf_t *dataObjReadInpBBuf,
        int bufLen, int *writtenSinceUpdated, fileRestartInfo_t *info,
        rodsLong_t *dataSegLen ) {
    rodsLong_t gap = segSize;
    int bytesWritten,  bytesRead;
    int status;

    while ( gap > 0 ) {
        int toRead;
        if ( gap > bufLen ) {
            toRead = bufLen;
        }
        else {
            toRead = ( int ) gap;
        }
        dataObjReadInp->len = dataObjReadInpBBuf->len = toRead;
        bytesRead = rcDataObjRead( conn, dataObjReadInp,
                                   dataObjReadInpBBuf );

        if ( bytesRead < 0 ) {
            rodsLog( LOG_ERROR,
                     "getSeg: rcDataObjRead error. status = %d", bytesRead );
            return bytesRead;
        }
        else if ( bytesRead == 0 ) {
            /* EOF */
            rodsLog( LOG_ERROR,
                     "getSeg: rcDataObjRead error. EOF reached. toRead = %d", toRead );
            return SYS_COPY_LEN_ERR;
        }
        bytesWritten = myWrite( localFd, dataObjReadInpBBuf->buf,
                                bytesRead, NULL );

        if ( bytesWritten != bytesRead ) {
            rodsLog( LOG_ERROR,
                     "getSeg: Read %d bytes, Wrote %d bytes.\n ",
                     bytesRead, bytesWritten );
            return SYS_COPY_LEN_ERR;
        }
        else {
            gap -= bytesWritten;
            *writtenSinceUpdated += bytesWritten;
            *dataSegLen += bytesWritten;
            if ( *writtenSinceUpdated >= RESTART_FILE_UPDATE_SIZE ) {
                status = writeLfRestartFile( conn->fileRestart.infoFile, info );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                             "getSeg: writeLfRestartFile for %s, status = %d",
                             info->fileName, status );
                    return status;
                }
                *writtenSinceUpdated = 0;
            }
        }
    }
    return 0;
}

int
catDataObj( rcComm_t *conn, char *objPath ) {
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjCloseInp;
    openedDataObjInp_t dataObjReadInp;
    bytesBuf_t dataObjReadOutBBuf;
    int l1descInx;
    int bytesRead;


    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );
    rstrcpy( dataObjOpenInp.objPath, objPath, MAX_NAME_LEN );
    dataObjOpenInp.openFlags = O_RDONLY;

    l1descInx = rcDataObjOpen( conn, &dataObjOpenInp );
    if ( l1descInx < 0 ) {
        rodsLogError( LOG_ERROR, l1descInx,
                      "catDataObj: rcDataObjOpen error for %s", objPath );
        return l1descInx;
    }
    bzero( &dataObjReadInp, sizeof( dataObjReadInp ) );
    dataObjReadOutBBuf.buf = malloc( TRANS_BUF_SZ + 1 );
    dataObjReadOutBBuf.len = TRANS_BUF_SZ + 1;
    dataObjReadInp.l1descInx = l1descInx;
    dataObjReadInp.len = TRANS_BUF_SZ;

    while ( ( bytesRead = rcDataObjRead( conn, &dataObjReadInp,
                                         &dataObjReadOutBBuf ) ) > 0 ) {
        char *buf = ( char * ) dataObjReadOutBBuf.buf;
        buf[bytesRead] = '\0';
        printf( "%s", buf );
    }
    free( dataObjReadOutBBuf.buf );
    printf( "\n" );
    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = l1descInx;

    rcDataObjClose( conn, &dataObjCloseInp );

    return 0;
}
