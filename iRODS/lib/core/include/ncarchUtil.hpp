/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncarchUtil.h - Header for for ncarchUtil.c */

#ifndef NCARCHUTIL_HPP
#define NCARCHUTIL_HPP

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"

extern "C" {


    int
    ncarchUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
                rodsPathInp_t *rodsPathInp );
    int
    initCondForNcarch( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                       ncArchTimeSeriesInp_t *ncArchTimeSeriesInp );
}

#endif	/* NCARCHUTIL_H */
