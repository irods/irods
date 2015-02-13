/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include <fcntl.h>
#include "rods.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"
#include "miscUtil.hpp"
#include "rcMisc.hpp"
#include "genQuery.hpp"
#include "apiHandler.hpp"
#include "rodsClient.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

void usage( char *prog );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status, ix, i, fd, len;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;
    char *envFile;
    char buffer[MAX_NAME_LEN];
    rodsPath_t rodsPath;
    rcComm_t *Conn;
    rErrMsg_t errMsg;

    status = parseCmdLineOpt( argc, argv, "vVh", 0, &myRodsArgs );
    if ( status ) {
        printf( "Use -h for help.\n" );
        exit( 1 );
    }
    if ( myRodsArgs.help == True ) {
        usage( argv[0] );
        exit( 0 );
    }
    ix = myRodsArgs.optind;

    status = getRodsEnv( &myEnv );
    envFile = getRodsEnvFileName();

    /* Just "icd", so cd to home, so just remove the session file */
    /* (can do this for now, since session has only the cwd). */
    if ( ix >= argc ) {
        status = unlink( envFile );
        if ( myRodsArgs.verbose == True ) {
            printf( "Deleting (if it exists) session envFile:%s\n", envFile );
            printf( "unlink status = %d\n", status );
        }
        exit( 0 );
    }

    memset( ( char* )&rodsPath, 0, sizeof( rodsPath ) );
    rstrcpy( rodsPath.inPath, argv[ix], MAX_NAME_LEN );
    parseRodsPath( &rodsPath, &myEnv );

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    /* Connect and check that the path exists */
    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        rcDisconnect( Conn );
        exit( 7 );
    }

    status = getRodsObjType( Conn, &rodsPath );
    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    if ( status < 0 ) {
        fprintf( stderr, "error %d getting type", status );
        exit( 4 );
    }

    if ( rodsPath.objType != COLL_OBJ_T || rodsPath.objState != EXIST_ST ) {
        printf( "No such directory (collection): %s\n", rodsPath.outPath );
        exit( 3 );
    }


    /* open the sessionfile and write or update it */
    if ( ( fd = open( envFile, O_CREAT | O_RDWR | O_TRUNC, 0644 ) ) < 0 ) {
        fprintf( stderr, "Unable to open envFile %s\n", envFile );
        exit( 5 );
    }

    snprintf( buffer, sizeof( buffer ), "{\n\"irods_cwd\": \"%s\"\n}\n", rodsPath.outPath );
    len = strlen( buffer );
    i = write( fd, buffer, len );
    close( fd );
    if ( i != len ) {
        fprintf( stderr, "Unable to write envFile %s\n", envFile );
        exit( 6 );
    }

    return 0;
}

/* Check to see if a collection exists */
int
checkColl( rcComm_t *Conn, char *path ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[2];
    char v1[MAX_NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    i1a[0] = COL_COLL_ID;
    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_COLL_NAME;
    genQueryInp.sqlCondInp.inx = i2a;
    sprintf( v1, "='%s'", path );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    return status;
}

void usage( char *prog ) {
    printf( "Changes iRODS the current working directory (collection)\n" );
    printf( "Usage: %s [-vh] [directory]\n", prog );
    printf( "If no directory is entered, the cwd is set back to your home\n" );
    printf( "directory as defined in your .rodsEnv file.\n" );
    printf( "Like the unix 'cd', '..' will move up one level and \n" );
    printf( "'/name' starts at the root.\n" );
    printf( " -v  verbose\n" );
    printf( " -V  very verbose\n" );
    printf( " -h  this help\n" );
    printReleaseInfo( "icd" );
}
