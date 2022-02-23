#include "irods/rmdirUtil.h"
#include "irods/rmUtil.h"
#include "irods/rodsPath.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsLog.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rodsPath.h"
#include "irods/rcMisc.h"

#include "irods/filesystem.hpp"

#include <boost/filesystem.hpp>

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

    fs::path abs_path = collPath.outPath;

    // Handle "-p".
    if (treatAsPathname) {
        try {
            const auto options = myRodsArgs->force
                ? fs::remove_options::no_trash
                : fs::remove_options::none;
            
            // Iterate over the elements in the path. This guarantees that no collections
            // above the ones provided are touched.
            for (auto&& _ : fs::path{collPath.inPath}) {
                const auto status = fs::client::status(*conn, abs_path);

                if (!fs::client::exists(status)) {
                    std::cerr << "Failed to remove [" << abs_path << "]: Collection does not exist\n";
                    return 0;
                }

                if (!fs::client::is_collection(status)) {
                    std::cerr << "Failed to remove [" << abs_path << "]: Path does not point to a collection\n";
                    return 0;
                }

                if (!fs::client::is_empty(*conn, abs_path)) {
                    std::cerr << "Failed to remove [" << abs_path << "]: Collection is not empty\n";
                    return 0;
                }

                fs::client::remove(*conn, abs_path, options);
                abs_path = abs_path.parent_path();

                static_cast<void>(_); // Unused
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << '\n';
        }

        return 0;
    }

    if (!fs::client::is_collection(*conn, abs_path)) {
        std::cout << "Failed to remove [" << abs_path << "]: Collection does not exist\n";
        return 0;
    }

    // check to make sure it's not /, /home, or /home/username?
    // XXXXX

    if (!fs::client::is_empty(*conn, abs_path)) {
        std::cout << "Failed to remove [" << abs_path << "]: Collection is not empty\n";
        return 0;
    }

    collInp_t collInp;
    dataObjInp_t dataObjInp;

    initCondForRm( myRodsArgs, &dataObjInp, &collInp );
    rstrcpy( collInp.collName, collPath.outPath, MAX_NAME_LEN );

    const int status = rcRmColl( conn, &collInp, myRodsArgs->verbose );

    if (status < 0) {
        std::cout << "rmdirColl: rcRmColl failed with error " << status << '\n';
    }

    return status;
}

