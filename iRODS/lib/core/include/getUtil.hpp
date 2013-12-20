/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getUtil.h - Header for for getUtil.c */

#ifndef GET_UTIL_HPP
#define GET_UTIL_HPP

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"

extern "C" {

    int
    getUtil( rcComm_t **myConn, rodsEnv *myEnv, rodsArguments_t *myRodsArgs,
             rodsPathInp_t *rodsPathInp );
    int
    getDataObjUtil( rcComm_t *conn, char *srcPath, char *targPath,
                    rodsLong_t srcSize, uint dataMode, rodsEnv *myRodsEnv,
                    rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp );

    int
    initCondForGet( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                    dataObjInp_t *dataObjOprInp, rodsRestart_t *rodsRestart );

    int
    getCollUtil( rcComm_t **myConn, char *srcColl, char *targDir,
                 rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, dataObjInp_t *dataObjOprInp,
                 rodsRestart_t *rodsRestart );

}

#endif	/* GET_UTIL_H */
