#ifndef RMDIR_UTIL_H__
#define RMDIR_UTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"

#ifdef __cplusplus
extern "C" {
#endif

int rmdirUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs, int treatAsPathname, int numColls, rodsPath_t collPaths[] );

int rmdirCollUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs, int treatAsPathname, rodsPath_t collPath );

int checkCollExists( rcComm_t *conn, rodsArguments_t *myRodsArgs, const char *collPath );

int checkCollIsEmpty( rcComm_t *conn, rodsArguments_t *myRodsArgs, const char *collPath );

rodsPath_t getParentColl( const char *collPath );

#ifdef __cplusplus
}
#endif

#endif  // RMDIR_UTIL_H__
