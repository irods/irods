/*** Copyright (c), The Regents of the University of California            ***
 *** For more infobunation please refer to files in the COPYRIGHT directory ***/
/* bunUtil.h - Header for for bunUtil.c */

#ifndef BUN_UTIL_HPP
#define BUN_UTIL_HPP

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "rodsPath.hpp"

#ifdef  __cplusplus
extern "C" {
#endif

int
bunUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp);
int
initCondForBunOpr (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
structFileExtAndRegInp_t *structFileExtAndRegInp, 
rodsPathInp_t *rodsPathInp);
#ifdef  __cplusplus
}
#endif

#endif	/* BUN_UTIL_H */
