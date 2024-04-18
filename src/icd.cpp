/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include <fcntl.h>
#include <irods/rods.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/miscUtil.h>
#include <irods/rcMisc.h>
#include <irods/genQuery.h>
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>

#include <cstdio>

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
        print_error_stack_to_file(Conn->rError, stderr);
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
        printf( "No such collection: %s\n", rodsPath.outPath );
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
    printf( "Change the current working collection.\n\n" );

    printf( "Usage: %s [-vVh] [COLLECTION]\n\n", prog );

    printf( "If invoked without any arguments, the current working collection is set\n" );
    printf( "back to your home collection as defined in your irods_environment.json\n" );
    printf( "file.\n\n" );

    printf( "If COLLECTION matches \"..\", the current working collection is set to the\n" );
    printf( "parent collection.\n\n" );

    printf( "If COLLECTION begins with a forward slash, the path is treated as an absolute\n" );
    printf( "path.\n\n" );

    printf( "Upon success, the current working collection is stored in\n" );
    printf( "irods_environment.json.PID where PID matches the shell's PID number. This\n" );
    printf( "allows multiple terminal sessions to exist within the same environment.\n\n" );

    printf( "If the inclusion of the PID isn't sufficient, this behavior can be overridden\n" );
    printf( "by setting the environment variable, IRODS_ENVIRONMENT_FILE, to the absolute\n" );
    printf( "path of a file that will serve as the new session file. This may require\n" );
    printf( "re-running iinit. Setting the environment variable causes %s to replace the\n", prog );
    printf( "\".PID\" extension with \".cwd\".\n\n" );

    printf( "Options:\n" );
    printf( " -v  Verbose.\n" );
    printf( " -V  Very verbose.\n" );
    printf( " -h  Show this message.\n" );

    printReleaseInfo(prog);
}
