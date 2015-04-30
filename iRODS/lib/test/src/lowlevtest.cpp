/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* lowlevtest.cpp - test the low level api */

#include "rodsClient.h"

/* NOTE : You have to change FILE_NAME, PAR_DIR_NAME, DIR_NAME and ADDR
 * for your own env */

#define FILE_NAME	"/data/mwan/test/RODS/vault/foo1"
#define PAR_DIR_NAME	"/data/mwan/test/RODS/vault"
#define DIR_NAME	"/data/mwan/test/RODS/vault/foodir1"
#define ADDR		"srbbrick8.sdsc.edu"

#define OUT_FILE_NAME       "foo1"
#define MY_MODE          0600

#define BUFSIZE	(1024*1024)

int
main( int argc, char **argv ) {
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    int status;
    rErrMsg_t errMsg;
    miscSvrInfo_t *outSvrInfo = NULL;
    fileCreateInp_t fileCreateInp;
    fileOpenInp_t fileOpenInp;
    fileWriteInp_t fileWriteInp;
    fileCloseInp_t fileCloseInp;
    fileLseekInp_t fileLseekInp;
    fileLseekOut_t *fileLseekOut = NULL;
    fileUnlinkInp_t fileUnlinkInp;
    fileMkdirInp_t fileMkdirInp;
    fileRmdirInp_t fileRmdirInp;
    fileChmodInp_t fileChmodInp;
    fileFstatInp_t fileFstatInp;
    fileFsyncInp_t fileFsyncInp;
    fileStageInp_t fileStageInp;
    fileGetFsFreeSpaceInp_t fileGetFsFreeSpaceInp;
    fileGetFsFreeSpaceOut_t *fileGetFsFreeSpaceOut = NULL;
    rodsStat_t *fileFstatOut;
    fileStatInp_t fileStatInp;
    rodsStat_t *fileStatOut;
    bytesBuf_t fileWriteInpBBuf;
    fileReadInp_t fileReadInp;
    bytesBuf_t fileReadOutBBuf;

    fileOpendirInp_t fileOpendirInp;
    fileReaddirInp_t fileReaddirInp;
    rodsDirent_t *fileReaddirOut;
    fileClosedirInp_t fileClosedirInp;
    char chksumStr[NAME_LEN];
    fileChksumInp_t fileChksumInp;
    char *chksumStrOut = NULL;

    int fileInx;
    int bytesWritten, bytesRead, total;
    int in_fd, out_fd;
    int dir_fd;

    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s local-file\n", argv[0] );
        exit( 1 );
    }

    status = chksumLocFile( argv[1], chksumStr );

    if ( status < 0 ) {
        fprintf( stderr, "chksumLocFile error, status = %d\n", status );
        exit( 1 );
    }
    else {
        printf( "chksumStr = %s\n", chksumStr );
    }

    status = getRodsEnv( &myRodsEnv );

    if ( status < 0 ) {
        fprintf( stderr, "getRodsEnv error, status = %d\n", status );
        exit( 1 );
    }

    conn = rcConnect( myRodsEnv.rodsHost, myRodsEnv.rodsPort,
                      myRodsEnv.rodsUserName, myRodsEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        fprintf( stderr, "rcConnect error\n" );
        exit( 1 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        fprintf( stderr, "clientLogin error\n" );
        rcDisconnect( conn );
        exit( 7 );
    }

    /* test rcGetMiscSvrInfo call */

    status = rcGetMiscSvrInfo( conn, &outSvrInfo );

    if ( status < 0 ) {
        fprintf( stderr, "rcGetMiscSvrInfo error. status = %d\n", status );
    }
    else {
        printf( "serverType = %d\n", outSvrInfo->serverType );
        printf( "server relVersion = %s\n", outSvrInfo->relVersion );
        printf( "server apiVersion = %s\n", outSvrInfo->apiVersion );
        printf( "server rodsZone = %s\n", outSvrInfo->rodsZone );
    }

    /* test rcFileCreate call */

    memset( &fileCreateInp, 0, sizeof( fileCreateInp ) );

    fileCreateInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileCreateInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileCreateInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileCreateInp.fileName, FILE_NAME, MAX_NAME_LEN );
    fileCreateInp.mode = 0640;
    fileCreateInp.dataSize = -1;

    fileInx = rcFileCreate( conn, &fileCreateInp );

    if ( fileInx < 0 ) {
        fprintf( stderr, "rcFileCreate error. status = %d\n", fileInx );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcFileCreate: fileInx = %d\n", fileInx );
    }

    in_fd = open( argv[1], O_RDONLY, 0 );
    if ( in_fd < 0 ) { /* error */
        fprintf( stderr, "can't open file\"%s\"\n", argv[1] );
        rcDisconnect( conn );
        exit( 1 );
    }

    fileWriteInpBBuf.buf = malloc( BUFSIZE );
    fileWriteInpBBuf.len = 0;

    total = 0;
    while ( ( fileWriteInpBBuf.len = read( in_fd, fileWriteInpBBuf.buf, BUFSIZE ) )
            > 0 ) {
        /* Write to the data object */

        fileWriteInp.fileInx = fileInx;
        fileWriteInp.len = fileWriteInpBBuf.len;
        bytesWritten = rcFileWrite( conn, &fileWriteInp, &fileWriteInpBBuf );
        if ( bytesWritten < fileWriteInp.len ) {
            fprintf( stderr, "Error: Read %d bytes, Wrote %d bytes.\n ",
                     fileWriteInp.len, bytesWritten );
            rcDisconnect( conn );
            exit( 1 );
        }
        total += bytesWritten;
    }

    printf( "rcFileWrite: total written = %d\n", total );

    /* test rcFileLseek call */

    fileLseekInp.fileInx = fileInx;
    fileLseekInp.offset = 100;
    fileLseekInp.whence = SEEK_SET;
    status = rcFileLseek( conn, &fileLseekInp, &fileLseekOut );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileLseek error. status = %d.\n ", status );
    }
    else {
        if ( fileLseekOut->offset != 100 ) {
            fprintf( stderr, "rcFileLseek error, seek to %d, got %lld\n",
                     100, fileLseekOut->offset );
        }
        printf( "rcFileLseek out offset = %lld\n", fileLseekOut->offset );
    }

    /* test rcFileClose call */

    fileCloseInp.fileInx = fileInx;

    status = rcFileClose( conn, &fileCloseInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileClose error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileClose: status = %d\n", status );
    }

    /* test rcFileChksum call */

    memset( &fileChksumInp, 0, sizeof( fileChksumInp ) );

    fileChksumInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileChksumInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileChksumInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileChksumInp.fileName, FILE_NAME, MAX_NAME_LEN );

    status = rcFileChksum( conn, &fileChksumInp, &chksumStrOut );

    if ( status < 0 ) {
        fprintf( stderr, "rcFileChksum error. status = %d\n", status );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcFileChksum: chksumStrOut = %s\n", chksumStrOut );
    }

    /* test rcFileGetFsFreeSpace call */

    fileGetFsFreeSpaceInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileGetFsFreeSpaceInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileGetFsFreeSpaceInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileGetFsFreeSpaceInp.fileName, FILE_NAME, MAX_NAME_LEN );
    fileGetFsFreeSpaceInp.flag = 0;

    status = rcFileGetFsFreeSpace( conn, &fileGetFsFreeSpaceInp,
                                   &fileGetFsFreeSpaceOut );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileGetFsFreeSpace error. status = %d.\n ",
                 status );
    }
    else {
        printf( "rcFileGetFsFreeSpace: freespace = %lld\n",
                fileGetFsFreeSpaceOut->size );
    }

    /* test rcFileOpen call */

    memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );

    fileOpenInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileOpenInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileOpenInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileOpenInp.fileName, FILE_NAME, MAX_NAME_LEN );
    fileOpenInp.mode = 0640;
    fileOpenInp.flags = O_RDONLY;
    fileOpenInp.otherFlags = 0;

    fileInx = rcFileOpen( conn, &fileOpenInp );

    if ( fileInx < 0 ) {
        fprintf( stderr, "rcFileOpen error. status = %d\n", fileInx );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcFileOpen: fileInx = %d\n", fileInx );
    }

    /* test rcFileRead call */

    fileReadOutBBuf.buf = fileWriteInpBBuf.buf;
    fileReadOutBBuf.len = 0;
    fileReadInp.fileInx = fileInx;
    fileReadInp.len = BUFSIZE;

    out_fd = open( OUT_FILE_NAME, O_CREAT | O_WRONLY, 0640 );
    if ( out_fd < 0 ) { /* error */
        fprintf( stderr, "can't create local file %s\n", OUT_FILE_NAME );
        exit( 1 );
    }

    total = 0;
    while ( ( bytesRead = rcFileRead( conn, &fileReadInp, &fileReadOutBBuf ) ) > 0 ) {
        /* Write to the data object */
        bytesWritten = write( out_fd, fileReadOutBBuf.buf, bytesRead );
        if ( bytesWritten < bytesRead ) {
            fprintf( stderr, "Error: Read %d bytes, Wrote %d bytes.\n ",
                     bytesRead, bytesWritten );
            exit( 1 );
        }
        total += bytesWritten;
    }

    printf( "%d bytes written to %s\n", total, OUT_FILE_NAME );

    fileFstatInp.fileInx = fileInx;

    /* test rcFileFstat call */

    status = rcFileFstat( conn, &fileFstatInp, &fileFstatOut );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileFstat error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileFstat: size = %lld, mode = %o\n",
                fileFstatOut->st_size, fileFstatOut->st_mode );
    }

    /* test rcFileClose call */

    fileCloseInp.fileInx = fileInx;

    status = rcFileClose( conn, &fileCloseInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileClose error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileClose: status = %d\n", status );
    }

    /* test rcFileChmod call */

    fileChmodInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileChmodInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileChmodInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileChmodInp.fileName, FILE_NAME, MAX_NAME_LEN );
    fileChmodInp.mode = MY_MODE;

    status = rcFileChmod( conn, &fileChmodInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileChmod error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileChmod: status = %d\n", status );
    }

    /* test rcFileStat call */

    fileStatInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileStatInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileStatInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileStatInp.fileName, FILE_NAME, MAX_NAME_LEN );

    status = rcFileStat( conn, &fileStatInp, &fileStatOut );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileStat error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileStat: size = %lld, mode = %o\n",
                fileStatOut->st_size, fileStatOut->st_mode );
    }

    /* test rcFileMkdir call */

    fileMkdirInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileMkdirInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileMkdirInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileMkdirInp.dirName, DIR_NAME, MAX_NAME_LEN );
    fileMkdirInp.mode = MY_MODE;

    status = rcFileMkdir( conn, &fileMkdirInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileMkdir error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileMkdir: status = %d\n", status );
    }

    /* test rcFileOpendir call */

    fileOpendirInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileOpendirInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileOpendirInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileOpendirInp.dirName, PAR_DIR_NAME, MAX_NAME_LEN );

    dir_fd = rcFileOpendir( conn, &fileOpendirInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: fileOpendirInp error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileOpendir: status = %d\n", status );
    }

    /* test rcFileReaddir call */

    fileReaddirInp.fileInx = dir_fd;

    while ( ( status = rcFileReaddir( conn, &fileReaddirInp, &fileReaddirOut ) )
            >= 0 ) {
        printf( "path %s in dir %s\n", fileReaddirOut->d_name, PAR_DIR_NAME );
    }

    if ( status < -1 ) {
        fprintf( stderr, "Error: rcFileReaddir error. status = %d.\n ", status );
    }

    /* test rcFileClosedir call */

    fileClosedirInp.fileInx = dir_fd;

    status = rcFileClosedir( conn, &fileClosedirInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileClosedir error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileClosedir: status = %d\n", status );
    }

    /* test rcFileUnlink call */

    fileUnlinkInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileUnlinkInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileUnlinkInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileUnlinkInp.fileName, FILE_NAME, MAX_NAME_LEN );

    status = rcFileUnlink( conn, &fileUnlinkInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileUnlink error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileUnlink: status = %d\n", status );
    }

    /* test rcFileRmdir call */

    fileRmdirInp.fileType = UNIX_FILE_TYPE;
    rstrcpy( fileRmdirInp.addr.hostAddr, ADDR, NAME_LEN );
    rstrcpy( fileRmdirInp.addr.zoneName, myRodsEnv.rodsZone, NAME_LEN );
    rstrcpy( fileRmdirInp.dirName, DIR_NAME, MAX_NAME_LEN );

    status = rcFileRmdir( conn, &fileRmdirInp );

    if ( status < 0 ) {
        fprintf( stderr, "Error: rcFileRmdir error. status = %d.\n ", status );
    }
    else {
        printf( "rcFileRmdir: status = %d\n", status );
    }


    rcDisconnect( conn );

    free( fileWriteInpBBuf.buf ); // JMC cppcheck - leak
}

