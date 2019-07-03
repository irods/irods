#include "rmdirUtil.h"
#include "rmUtil.h"
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "rcGlobalExtern.h"
#include "rodsPath.h"
#include "rcMisc.h"

#include "filesystem.hpp"

#include "boost/filesystem.hpp"

#include <iostream>

int
rmdirUtil( rcComm_t        *conn,
           rodsArguments_t *myRodsArgs,
           int             treatAsPathname,
           int             numColls,
           rodsPath_t      collPaths[] ) {
    if ( numColls <= 0 ) {
        return USER__NULL_INPUT_ERR;
    }

    int status = 0;

    for ( int i = 0; i < numColls; ++i ) {
        status = rmdirCollUtil( conn, myRodsArgs, treatAsPathname, collPaths[i] );
    }

    return status;
}

int rmdirCollUtil(rcComm_t        *conn, 
                  rodsArguments_t *myRodsArgs, 
                  int             treatAsPathname, 
                  rodsPath_t      collPath)
{
    namespace fs = irods::experimental::filesystem;

    // Handle "-p".
    if (treatAsPathname) {
        try {
            const auto options = myRodsArgs->force
                ? fs::remove_options::no_trash
                : fs::remove_options::none;
            
            fs::path abs_path = collPath.outPath;

            // Iterate over the elements in the path. This guarantees that no collections
            // above the ones provided are touched.
            for (auto&& _ : fs::path{collPath.inPath}) {
                if (!fs::client::exists(*conn, abs_path)) {
                    std::cerr << "Failed to remove [" << abs_path << "]: Collection does not exist\n";
                    return 0;
                }

                if (!fs::client::is_collection(*conn, abs_path)) {
                    std::cerr << "Failed to remove [" << abs_path << "]: Path does not point to a collection\n";
                    return 0;
                }

                if (!fs::client::is_empty(*conn, abs_path)) {
                    std::cerr << "Failed to remove [" << abs_path << "]: Collection is not empty\n";
                    return 0;
                }

                fs::client::remove(*conn, abs_path, options);
                abs_path = abs_path.parent_path();

                (void) _; // Unused
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << '\n';
        }

        return 0;
    }

    int status;
    collInp_t collInp;
    dataObjInp_t dataObjInp;

    if ( !checkCollExists( conn, myRodsArgs, collPath.outPath ) ) {
        std::cout << "Failed to remove ["
                  << collPath.outPath
                  << "]: Collection does not exist"
                  << std::endl;
        return 0;
    }

    // check to make sure it's not /, /home, or /home/username?
    // XXXXX

    if ( !checkCollIsEmpty( conn, myRodsArgs, collPath.outPath ) ) {
        std::cout << "Failed to remove ["
                  << collPath.outPath
                  << "]: Collection is not empty"
                  << std::endl;
        return 0;
    }

    initCondForRm( myRodsArgs, &dataObjInp, &collInp );
    rstrcpy( collInp.collName, collPath.outPath, MAX_NAME_LEN );

    status = rcRmColl( conn, &collInp, myRodsArgs->verbose );

    if ( status < 0 ) {
        std::cout << "rmdirColl: rcRmColl failed with error "
                  << status
                  << std::endl;
        return status;
    }

    return status;
}

int checkCollExists( rcComm_t *conn, rodsArguments_t *myRodsArgs, const char *collPath ) {
    int status;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    char condStr[MAX_NAME_LEN];

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_COLL_ID, 1 );
    genQueryInp.maxRows = 0;
    genQueryInp.options = AUTO_CLOSE;
    
    snprintf( condStr, MAX_NAME_LEN, "='%s'", collPath );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );

    status = rcGenQuery( conn, &genQueryInp, &genQueryOut );

    clearGenQueryInp( &genQueryInp );
    freeGenQueryOut( &genQueryOut );

    if ( status == CAT_NO_ROWS_FOUND ) {
        return 0;
    } else {
        return 1;
    }
}

int checkCollIsEmpty( rcComm_t *conn, rodsArguments_t *myRodsArgs, const char *collPath ) {
    int status;
    genQueryInp_t genQueryInp1, genQueryInp2;
    genQueryOut_t *genQueryOut1 = NULL, *genQueryOut2 = NULL;
    int noDataFound = 0;
    int noCollFound = 0;
    char condStr[MAX_NAME_LEN];

    memset( &genQueryInp1, 0, sizeof( genQueryInp1 ) );
    memset( &genQueryInp2, 0, sizeof( genQueryInp2 ) );

    snprintf( condStr, MAX_NAME_LEN, "select COLL_ID where COLL_NAME like '%s/%%'", collPath );
    fillGenQueryInpFromStrCond( condStr, &genQueryInp1 );

    status = rcGenQuery( conn, &genQueryInp1, &genQueryOut1 );

    clearGenQueryInp( &genQueryInp1 );
    freeGenQueryOut( &genQueryOut1 );

    if ( status == CAT_NO_ROWS_FOUND ) {
        noCollFound = 1;
    }

    snprintf( condStr, MAX_NAME_LEN, "select DATA_ID where COLL_NAME like '%s%%'", collPath );
    fillGenQueryInpFromStrCond( condStr, &genQueryInp2 );

    status = rcGenQuery( conn, &genQueryInp2, &genQueryOut2 );

    clearGenQueryInp( &genQueryInp2 );
    freeGenQueryOut( &genQueryOut2 );

    if ( status == CAT_NO_ROWS_FOUND ) {
        noDataFound = 1;
    }

    return ( noDataFound && noCollFound );
}

rodsPath_t getParentColl( const char *collPath ){
    rodsPath_t rodsPath;
    boost::filesystem::path temp(collPath);

    rstrcpy( rodsPath.inPath, temp.parent_path().string().c_str(), MAX_NAME_LEN );
    rstrcpy( rodsPath.outPath, temp.parent_path().string().c_str(), MAX_NAME_LEN );

    return rodsPath;
}

