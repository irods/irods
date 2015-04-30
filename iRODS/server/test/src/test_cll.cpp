/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 Simple program to test the Rcat Low Level routines to Postgres or Oracle.
 */

#include "rods.h"
#include "readServerConfig.hpp"
#include "irods_plugin_name_generator.hpp"

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

// =-=-=-=-=-=-=-
// STL Includes
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>


/*int ProcessType=CLIENT_PT; */

int
main( int argc, char **argv ) {
    rodsServerConfig_t cfg;
    int status = readServerConfig( &cfg );
    if ( status < 0 ) {
        printf( "readServerConfig failed horribly\n" );
        exit( 0 );
    }

    irods::plugin_name_generator name_gen;

    std::string so_name;
    irods::error ret = name_gen(
                           cfg.catalog_database_type,
                           irods::IRODS_DATABASE_HOME,
                           so_name );
    if ( !ret.ok() ) {
        printf( "name_gen failed" );
        return -1;
    }

    void* handle = dlopen( so_name.c_str(), RTLD_LAZY );
    if ( !handle ) {
        std::stringstream msg;
        msg << "failed to open shared object file [" << so_name
            << "] :: dlerror: is [" << dlerror() << "]";
        std::cout << msg.str() << std::endl;
        return -1;
    }


    typedef int ( *test_type )( char*, char* );
    test_type cll_test = reinterpret_cast< test_type >( dlsym( handle, "cllTest" ) );
    char* err = 0;
    if ( ( err = dlerror() ) != 0 ) {
        std::stringstream msg;
        msg << "failed to load symbol from shared object handle - cllTest"
            << " :: dlerror is [" << err << "]";
        dlclose( handle );
        std::cout << msg.str() << std::endl;
        return -1;
    }

    //int i = cllTest( argv[1], argv[2] );
    return cll_test( argv[1], argv[2] );
}

/* This is a dummy version of icatApplyRule for this test program so
   the rule-engine is not needed in this ICAT test. */
int
icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 ) {
    return 0;
}

