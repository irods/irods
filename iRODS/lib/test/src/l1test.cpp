/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* l1test.c - test the high level api */

#include "rodsClient.h"

#define USER_NAME	"rods"
#define RODS_ZONE	"tempZone"

/* NOTE : You have to change FILE_NAME, PAR_DIR_NAME, DIR_NAME and ADDR
 * for your own env */

#define DEST_RESC_NAME  "demoResc"
#define RESC_NAME   	"demoResc"
#define REPL_RESC_NAME   	"demoResc8"
#define REPL_RESC_NAME1   	"demoResc7"

#define MY_MODE          0750
#define OUT_FILE_NAME       "foo15"

#define BUFSIZE	(1024*1024)

int
main( int argc, char **argv ) {
    rcComm_t *conn;
    rodsEnv myRodsEnv;
    int status;
    rErrMsg_t errMsg;
    dataObjInp_t dataObjCreateInp;
    dataObjInp_t dataObjOpenInp;
    openedDataObjInp_t dataObjCloseInp;
    openedDataObjInp_t dataObjWriteInp;
    bytesBuf_t dataObjWriteInpBBuf;
    openedDataObjInp_t dataObjReadInp;
    bytesBuf_t dataObjReadOutBBuf;
    dataObjInp_t dataObjOprInp;
    bytesBuf_t dataObjInpBBuf;
    struct stat statbuf;

    int bytesWritten, bytesRead, total;
    int in_fd, out_fd;
    int l1descInx1, l1descInx2, l1descInx3;
    openedDataObjInp_t dataObjLseekInp;
    fileLseekOut_t *dataObjLseekOut = NULL;
    dataObjCopyInp_t dataObjCopyInp;
    collInp_t collCreateInp;
    execCmd_t execCmd;
    execCmdOut_t *execCmdOut = NULL;
    char *chksumStr;
    char myPath[MAX_NAME_LEN], myCwd[MAX_NAME_LEN];

    rError_t myError;

    memset( &myError, 0, sizeof( rError_t ) );

    addRErrorMsg( &myError, 999, "this is a test" );

    if ( argc != 2 ) {
        fprintf( stderr, "Usage: %s rods_dataObj\n", argv[0] );
        exit( 1 );
    }

    status = getRodsEnv( &myRodsEnv );

    if ( status < 0 ) {
        fprintf( stderr, "getRodsEnv error, status = %d\n", status );
        exit( 1 );
    }

    status = stat( argv[1], &statbuf );
    if ( status < 0 ) {
        fprintf( stderr, "stat of %s failed\n", argv[1] );
        exit( 1 );
    }
    else {
        printf( "input file %s size = %lld\n",
                ( char * ) argv[1], statbuf.st_size );
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

    memset( &execCmd, 0, sizeof( execCmd ) );
    rstrcpy( execCmd.cmd, "hello", LONG_NAME_LEN );
    rstrcpy( execCmd.cmdArgv, "x 'y' z", MAX_NAME_LEN );
    status = rcExecCmd( conn, &execCmd, &execCmdOut );

    if ( status >= 0 ) {
        printf( "rcExecCmd of hello success, output=%s\n",
                ( char * ) execCmdOut->stdoutBuf.buf );
    }
    else {
        printf( "rcExecCmd failed, status = %d\n", status );
        exit( 1 );
    }

    /* test rcDataObjPut */
    memset( &dataObjOprInp, 0, sizeof( dataObjOprInp ) );
    memset( &dataObjInpBBuf, 0, sizeof( dataObjInpBBuf ) );

    snprintf( dataObjOprInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, argv[1] );

    addKeyVal( &dataObjOprInp.condInput, RESC_NAME_KW, RESC_NAME );
    addKeyVal( &dataObjOprInp.condInput, DATA_TYPE_KW, "generic" );
    dataObjOprInp.createMode = MY_MODE;
    dataObjOprInp.openFlags = O_WRONLY;
    dataObjOprInp.numThreads = 4;
    dataObjOprInp.dataSize = statbuf.st_size;

    status = rcChksumLocFile( argv[1], VERIFY_CHKSUM_KW,
                              &dataObjOprInp.condInput );

    if ( status < 0 ) {
        fprintf( stderr, "rcChksumLocFile error, status = %d\n",
                 status );
        exit( 1 );
    }

    l1descInx3 = rcDataObjPut( conn, &dataObjOprInp, argv[1] );
    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjPut error. status = %d\n", l1descInx3 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjPut: l1descInx3 = %d\n", l1descInx3 );
    }

    /* test rcDataObjChksum */
    status = rcDataObjChksum( conn, &dataObjOprInp, &chksumStr );

    if ( status < 0 ) {
        fprintf( stderr, "rcDataObjChksum error, status = %d\n",
                 status );
        exit( 1 );
    }
    else {
        printf( "rcDataObjChksum: chksumStr = %s\n", chksumStr );
    }

    /* test dataObjRepl */
    addKeyVal( &dataObjOprInp.condInput, DEST_RESC_NAME_KW, REPL_RESC_NAME );

    l1descInx3 = rcDataObjRepl( conn, &dataObjOprInp );

    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjRepl error. status = %d\n", l1descInx3 );
    }
    else {
        printf( "rcDataObjRepl: l1descInx3 = %d\n", l1descInx3 );
    }

    /* test dataObjTruncate */

    dataObjOprInp.dataSize = 1000;
    l1descInx3 = rcDataObjTruncate( conn, &dataObjOprInp );

    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjTruncate error. status = %d\n", l1descInx3 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjTruncate: l1descInx3 = %d\n", l1descInx3 );
    }

    /* test dataObjTrim */
    addKeyVal( &dataObjOprInp.condInput, RESC_NAME_KW, REPL_RESC_NAME );
    addKeyVal( &dataObjOprInp.condInput, COPIES_KW, "1" );

    l1descInx3 = rcDataObjTrim( conn, &dataObjOprInp );

    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjTrim error. status = %d\n", l1descInx3 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjTrim: l1descInx3 = %d\n", l1descInx3 );
    }

    /* test dataObjCopy */
    memset( &dataObjCopyInp, 0, sizeof( dataObjCopyInp ) );

    snprintf( dataObjCopyInp.srcDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, argv[1] );
    snprintf( dataObjCopyInp.destDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo10" );
    addKeyVal( &dataObjCopyInp.srcDataObjInp.condInput,
               RESC_NAME_KW, RESC_NAME );
    addKeyVal( &dataObjCopyInp.destDataObjInp.condInput,
               RESC_NAME_KW, RESC_NAME );
    rmKeyVal( &dataObjCopyInp.destDataObjInp.condInput, VERIFY_CHKSUM_KW );
    addKeyVal( &dataObjCopyInp.destDataObjInp.condInput, VERIFY_CHKSUM_KW, "" );

    dataObjCopyInp.destDataObjInp.createMode = MY_MODE;
    dataObjCopyInp.destDataObjInp.openFlags = O_WRONLY;
    dataObjCopyInp.destDataObjInp.numThreads = 4;
    dataObjCopyInp.srcDataObjInp.dataSize = statbuf.st_size;

    l1descInx3 = rcDataObjCopy( conn, &dataObjCopyInp );

    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjCopy error. status = %d\n", l1descInx3 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjCopy: l1descInx3 = %d\n", l1descInx3 );
    }

    memset( &dataObjOprInp, 0, sizeof( dataObjOprInp ) );

    snprintf( dataObjOprInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, argv[1] );

    addKeyVal( &dataObjOprInp.condInput, DEST_RESC_NAME_KW, REPL_RESC_NAME );
    dataObjOprInp.createMode = MY_MODE;
    dataObjOprInp.numThreads = 4;

    addKeyVal( &dataObjOprInp.condInput, VERIFY_CHKSUM_KW, "" );

    l1descInx3 = rcDataObjRepl( conn, &dataObjOprInp );

    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjRepl error. status = %d\n", l1descInx3 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjRepl: l1descInx3 = %d\n", l1descInx3 );
    }

    /* test rcDataObjRename */
    memset( &dataObjCopyInp, 0, sizeof( dataObjCopyInp ) );

    dataObjCopyInp.srcDataObjInp.oprType =
        dataObjCopyInp.destDataObjInp.oprType = RENAME_DATA_OBJ;

    snprintf( dataObjCopyInp.srcDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo10" );
    snprintf( dataObjCopyInp.destDataObjInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo11" );

    status = rcDataObjRename( conn, &dataObjCopyInp );

    if ( status < 0 ) {
        fprintf( stderr, "rcDataObjRename error. status = %d\n",
                 status );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjRename: status = %d\n", status );
    }

    snprintf( dataObjOprInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo11" );

    dataObjOprInp.openFlags = O_RDONLY;
    addKeyVal( &dataObjOprInp.condInput, FORCE_FLAG_KW, "" );
    l1descInx3 = rcDataObjGet( conn, &dataObjOprInp, "foo20" );

    if ( l1descInx3 < 0 ) {
        fprintf( stderr, "rcDataObjGet error. status = %d\n", l1descInx3 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjGet: l1descInx3 = %d\n", l1descInx3 );
    }

    /* make change to local file and rsync */
    l1descInx3 = open( "foo20", O_WRONLY, 0640 );
    write( l1descInx3, "iRods", 40 );
    close( l1descInx3 );

    /* test rcDataObjRsync call */
    status = rcChksumLocFile( "foo20", RSYNC_CHKSUM_KW,
                              &dataObjOprInp.condInput );

    addKeyVal( &dataObjOprInp.condInput, RSYNC_DEST_PATH_KW, "foo20" );
    addKeyVal( &dataObjOprInp.condInput, RSYNC_MODE_KW, TO_LOCAL );

    status = rcDataObjRsync( conn, &dataObjOprInp );

    if ( status ) {
        fprintf( stderr, "rcDataObjRsync error. status = %d\n", status );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjRsync: l1descInx1 = %d\n", status );
    }

    /* test rcPhyPathReg call */
    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );

    snprintf( dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo12" );
    snprintf( myPath, MAX_NAME_LEN, "%s/%s", getcwd( myCwd, MAX_NAME_LEN ),
              "foo20" );
    addKeyVal( &dataObjOpenInp.condInput, FILE_PATH_KW, myPath );
    addKeyVal( &dataObjOpenInp.condInput, RESC_NAME_KW, RESC_NAME );

    l1descInx1 = rcPhyPathReg( conn, &dataObjOpenInp );

    if ( l1descInx1 ) {
        fprintf( stderr, "rcPhyPathReg error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcPhyPathReg: l1descInx1 = %d\n", l1descInx1 );
    }


    /* test rcDataObjUnlink call */
    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );

    snprintf( dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, argv[1] );
    addKeyVal( &dataObjOpenInp.condInput, FORCE_FLAG_KW, "" );

    l1descInx1 = rcDataObjUnlink( conn, &dataObjOpenInp );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcDataObjUnlink error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjUnlink: l1descInx1 = %d\n", l1descInx1 );
    }

    snprintf( dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo11" );

    l1descInx1 = rcDataObjUnlink( conn, &dataObjOpenInp );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcDataObjUnlink error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjUnlink: l1descInx1 = %d\n", l1descInx1 );
    }

    snprintf( dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foo12" );

    l1descInx1 = rcDataObjUnlink( conn, &dataObjOpenInp );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcDataObjUnlink error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjUnlink: l1descInx1 = %d\n", l1descInx1 );
    }

    /* test rcDataObjCreate call */
    memset( &dataObjCreateInp, 0, sizeof( dataObjCreateInp ) );

    snprintf( dataObjCreateInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, argv[1] );
    addKeyVal( &dataObjCreateInp.condInput, RESC_NAME_KW, RESC_NAME );
    addKeyVal( &dataObjCreateInp.condInput, DATA_TYPE_KW, "generic" );
    dataObjCreateInp.createMode = MY_MODE;
    dataObjCreateInp.openFlags = O_WRONLY;
    dataObjCreateInp.dataSize = -1;

    l1descInx1 = rcDataObjCreate( conn, &dataObjCreateInp );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcDataObjCreate error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjCreate: l1descInx1 = %d\n", l1descInx1 );
    }

    in_fd = open( argv[1], O_RDONLY, 0 );
    if ( in_fd < 0 ) {
        fprintf( stderr, "can't open dataObj\"%s\"\n", argv[1] );
        rcDisconnect( conn );
        exit( 1 );
    }

    dataObjWriteInpBBuf.buf = malloc( BUFSIZE );
    dataObjWriteInpBBuf.len = BUFSIZE;
    dataObjWriteInp.l1descInx = l1descInx1;

    total = 0;
    while ( ( dataObjWriteInpBBuf.len =
                  read( in_fd, dataObjWriteInpBBuf.buf, BUFSIZE ) ) > 0 ) {
        /* Write to the data object */
        dataObjWriteInp.len = dataObjWriteInpBBuf.len;
        bytesWritten = rcDataObjWrite( conn, &dataObjWriteInp,
                                       &dataObjWriteInpBBuf );
        if ( bytesWritten < dataObjWriteInp.len ) {
            fprintf( stderr, "Error: Read %d bytes, Wrote %d bytes.\n ",
                     dataObjWriteInp.len, bytesWritten );
            rcDisconnect( conn );
            exit( 1 );
        }
        total += bytesWritten;
    }
    /* close the files */
    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = l1descInx1;

    status = rcDataObjClose( conn, &dataObjCloseInp );
    if ( status < 0 ) {
        fprintf( stderr, "rcDataObjClose of %d error, status = %d\n",
                 l1descInx1, status );
        exit( 1 );
    }

    close( in_fd );

    /* try to open the same data obj */

    /* test rcDataObjOpen call */

    printf( "--------------open file -----------------\n" );

    memset( &dataObjOpenInp, 0, sizeof( dataObjOpenInp ) );

    snprintf( dataObjOpenInp.objPath, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, argv[1] );
    /* XXXXX rescName not needed normally */
    dataObjOpenInp.openFlags = O_RDONLY;

    l1descInx2 = rcDataObjOpen( conn, &dataObjOpenInp );

    if ( l1descInx2 < 0 ) {
        fprintf( stderr, "rcDataObjOpen error. status = %d\n", l1descInx2 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjOpen: l1descInx2 = %d\n", l1descInx2 );
    }

    printf( "--------------end of open file -----------------\n" );
    exit( 0 );

    memset( &dataObjLseekInp, 0, sizeof( dataObjLseekInp ) );

    dataObjLseekInp.l1descInx = l1descInx2;
    dataObjLseekInp.offset = 1000;
    dataObjLseekInp.whence = SEEK_SET;

    status = rcDataObjLseek( conn, &dataObjLseekInp, &dataObjLseekOut );

    if ( status < 0 ) {
        fprintf( stderr, "rcDataObjLseek error. status = %d\n", status );
        rcDisconnect( conn );
        exit( 1 );
    }
    else if ( dataObjLseekOut->offset != 1000 ) {
        fprintf( stderr, "rcDataObjLseek error. toset %d, got %lld\n",
                 1000, dataObjLseekOut->offset );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjLseek: status = %d\n", status );
    }

    dataObjLseekInp.offset = 0;

    status = rcDataObjLseek( conn, &dataObjLseekInp, &dataObjLseekOut );


    /* test rcDataObjRead call */

    dataObjReadOutBBuf.buf = dataObjWriteInpBBuf.buf;
    dataObjReadOutBBuf.len = BUFSIZE;
    dataObjReadInp.l1descInx = l1descInx2;
    dataObjReadInp.len = BUFSIZE;

    out_fd = open( OUT_FILE_NAME, O_CREAT | O_WRONLY, 0640 );
    if ( out_fd < 0 ) { /* error */
        fprintf( stderr, "can't create local dataObj %s\n", OUT_FILE_NAME );
        exit( 1 );
    }

    total = 0;
    while ( ( bytesRead = rcDataObjRead( conn, &dataObjReadInp,
                                         &dataObjReadOutBBuf ) ) > 0 ) {
        /* Write to the data object */
        bytesWritten = write( out_fd, dataObjReadOutBBuf.buf, bytesRead );
        if ( bytesWritten < bytesRead ) {
            fprintf( stderr, "Error: Read %d bytes, Wrote %d bytes.\n ",
                     bytesRead, bytesWritten );
            exit( 1 );
        }
        total += bytesWritten;
    }

    printf( "%d bytes written to %s\n", total, OUT_FILE_NAME );

    /* close the files */
    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = l1descInx2;

    status = rcDataObjClose( conn, &dataObjCloseInp );
    if ( status < 0 ) {
        fprintf( stderr, "rcDataObjClose of %d error, status = %d\n",
                 l1descInx2, status );
        exit( 1 );
    }
    else {
        printf( "rcDataObjClose: status = %d\n", status );
    }

    close( out_fd );

    /* test rcDataObjPhymv */
    addKeyVal( &dataObjOpenInp.condInput, DEST_RESC_NAME_KW, REPL_RESC_NAME );

    status = rcDataObjPhymv( conn, &dataObjOpenInp );

    if ( status < 0 ) {
        fprintf( stderr, "rcDataObjPhymv error, status = %d\n", status );
        exit( 1 );
    }
    else {
        printf( "rcDataObjPhymv: status = %d\n", status );
    }

    /* test rcDataObjUnlink call */

    addKeyVal( &dataObjOpenInp.condInput, FORCE_FLAG_KW, "" );
    l1descInx1 = rcDataObjUnlink( conn, &dataObjOpenInp );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcDataObjUnlink error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcDataObjUnlink: l1descInx1 = %d\n", l1descInx1 );
    }

    /* testing rcCollCreate */

    memset( &collCreateInp, 0, sizeof( collCreateInp ) );

    snprintf( collCreateInp.collName, MAX_NAME_LEN, "%s/%s",
              myRodsEnv.rodsCwd, "foodir" );

    l1descInx1 = rcCollCreate( conn, &collCreateInp );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcCollCreate error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcCollCreate: l1descInx1 = %d\n", l1descInx1 );
    }

    addKeyVal( &collCreateInp.condInput, FORCE_FLAG_KW, "" );
    addKeyVal( &collCreateInp.condInput, RMTRASH_KW, "" );

    l1descInx1 = rcRmColl( conn, &collCreateInp, True );

    if ( l1descInx1 < 0 ) {
        fprintf( stderr, "rcRmColl error. status = %d\n", l1descInx1 );
        rcDisconnect( conn );
        exit( 1 );
    }
    else {
        printf( "rcRmColl: l1descInx1 = %d\n", l1descInx1 );
    }


    rcDisconnect( conn );

    free( dataObjWriteInpBBuf.buf ); // JMC cppcheck - leak
}

