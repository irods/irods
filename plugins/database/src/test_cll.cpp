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


extern "C" int cllTest( char*, char* );
/*int ProcessType=CLIENT_PT; */

int
main( int argc, char **argv ) {
    return cllTest( argv[1], argv[2] );
}

/* This is a dummy version of icatApplyRule for this test program so
   the rule-engine is not needed in this ICAT test. */
int
icatApplyRule( rsComm_t *rsComm, char *ruleName, char *arg1 ) {
    return 0;
}

