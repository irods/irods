#include "utility.hpp"
#include <irods/irods_client_api_table.hpp>
#include <irods/irods_pack_table.hpp>
#include <irods/rodsClient.h>
#include <irods/rodsError.h>
#include <irods/parseCommandLine.h>
#include <irods/rodsPath.h>
#include <irods/lsUtil.h>
#include <irods/irods_buffer_encryption.hpp>

#include <cstdio>
#include <string>
#include <iostream>

// =-=-=-=-=-=-=-
// NOTE:: these track the same structs in
//     :: the api example libhelloworld.cpp
typedef struct {
    int  _this;
    char _that [64];
} helloInp_t;

typedef struct {
    double _value;
} otherOut_t;

typedef struct {
    int  _this;
    char _that [64];
    otherOut_t _other;
} helloOut_t;

int
main( int argc, char** argv ) {

    signal( SIGPIPE, SIG_IGN );

    if (NULL != argv && NULL != argv[0]) {
        /* set SP_OPTION to argv[0] so it can be passed to server */
        char child[MAX_NAME_LEN], parent[MAX_NAME_LEN];
        *child = '\0';
        splitPathByKey(argv[0], parent, MAX_NAME_LEN, child, MAX_NAME_LEN, '/');
        if (*child != '\0') {
            mySetenvStr(SP_OPTION, child);
        }
    }

    rodsEnv myEnv;
    int status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        rodsLogError( LOG_ERROR, status, "main: getRodsEnv error. " );
        exit( 1 );
    }

    rErrMsg_t errMsg;
    rcComm_t *conn;
    conn = rcConnect(
               myEnv.rodsHost,
               myEnv.rodsPort,
               myEnv.rodsUserName,
               myEnv.rodsZone,
               0, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::pack_entry_table& pk_tbl = irods::get_pack_table();
    irods::api_entry_table& api_tbl = irods::get_client_api_table();
    init_api_table( api_tbl, pk_tbl );

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = utils::authenticate_client(conn, myEnv);
        if ( status != 0 ) {
            print_error_stack_to_file(conn->rError, stderr);
            rcDisconnect( conn );
            exit( 7 );
        }
    }

    helloInp_t inp;
    memset( &inp, 0, sizeof( inp ) );
    inp._this = 42;
    strncpy( inp._that, "hello, world.", 64 );

    void *tmp_out = NULL;
    status = procApiRequest( conn, 1300, &inp, NULL,
                             &tmp_out, NULL );
    if ( status < 0 ) {
        printf( "\n\nERROR - failed to call our api\n\n\n" );
        return 0;
    }
    else {
        helloOut_t* out = static_cast<helloOut_t*>( tmp_out );
        if ( out != NULL ) {
            printf( "\n\nthis [%d]  that [%s] other [%f]\n", out->_this, out->_that, out->_other._value );
        }
        else {
            printf( "ERROR: the 'out' variable is null\n" );
        }
    }

    rcDisconnect( conn );
}

