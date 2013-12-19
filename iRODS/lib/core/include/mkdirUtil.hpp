/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getUtil.h - Header for for getUtil.c */

#ifndef MKDIR_UTIL_HPP
#define MKDIR_UTIL_HPP

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"

extern "C" {

    int
    mkdirUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
               rodsPathInp_t *rodsPathInp );

}

#endif	/* MKDIR_UTIL_H */
