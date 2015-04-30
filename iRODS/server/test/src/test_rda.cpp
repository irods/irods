/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  RDA test program.
*/

#include "rodsClient.h"
#include "rdaHighLevelRoutines.hpp"

/*
Example command lines:
./test_rda RDA sql "create table t1( c1 varchar(250))"
./test_rda RDA sql "insert into t1 values (?)" abc
./test_rda RDA sqlr "select * from t1"

./test_rda RDA sql "create table t2( c1 varchar(250), c2 varchar(250))"
./test_rda RDA sql "insert into t2 values (?, ?)" 123 456abc
 */

rodsEnv myEnv;


int
main( int argc, char **argv ) {
    int status;
    rsComm_t *Comm;
    rodsArguments_t myRodsArgs;
    char *mySubName;
    char *myName;
    int didOne;

    Comm = ( rsComm_t* )malloc( sizeof( rsComm_t ) );
    memset( Comm, 0, sizeof( rsComm_t ) );

    parseCmdLineOpt( argc, argv, "", 0, &myRodsArgs );

    rodsLogLevel( LOG_NOTICE );

    rodsLogSqlReq( 1 );

    if ( argc < 4 ) {
        printf( "Usage: test_rda username pw testName [args...]\n" );
        exit( 3 );
    }

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        exit( 1 );
    }

    if ( strstr( myEnv.rodsDebug, "RDA" ) != NULL ) {
        rdaDebug( myEnv.rodsDebug );
    }

    status = rdaOpen( argv[1] );
    if ( status != 0 ) {
        rodsLog( LOG_SYS_FATAL,
                 "test_rda: rdaopen Error. Status = %d",
                 status );
        free( Comm ); // JMC cppcheck - leak
        return status;
    }

    didOne = 0;

    if ( strcmp( argv[2], "sql" ) == 0 ) {
        char *parms[50];
        int i, nParms;
        for ( i = 4; i < argc; i++ ) {
            parms[i - 4] = argv[i];
        }
        nParms = i - 4;
        status = rdaSqlNoResults( argv[3], parms, nParms );
        didOne = 1;
    }

    if ( strcmp( argv[2], "sqlr" ) == 0 ) {
        char *parms[50];
        int i, nParms;
        char *outBuf;
        for ( i = 4; i < argc; i++ ) {
            parms[i - 4] = argv[i];
        }
        nParms = i - 4;
        status = rdaSqlWithResults( argv[3], parms, nParms, &outBuf );
        if ( status ) {
            printf( "error %d\n", status );
        }
        else {
            printf( "%s", outBuf );
        }
        didOne = 1;
    }

    if ( status != 0 ) {
        myName = rodsErrorName( status, &mySubName );
        rodsLog( LOG_ERROR, "%s failed with error %d %s %s", argv[1],
                 status, myName, mySubName );
    }
    else {
        status = rdaClose();
        if ( status ) {
            printf( "rdaClose error %d\n", status );
        }
        else {
            if ( didOne ) {
                printf( "Completed successfully\n" );
            }
        }
    }

    if ( didOne == 0 ) {
        printf( "Unknown test type: %s\n", argv[3] );
    }

    free( Comm );
    return status;
}
/* This is a dummy version of icatApplyRule for this test program so
   the rule-engine is not needed in this ICAT test. */
int
icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 ) {
    return 0;
}
