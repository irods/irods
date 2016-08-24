/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef RODS_PATH_H__
#define RODS_PATH_H__

#include "rodsDef.h"
#include "rods.h"
#include "getRodsEnv.h"
#include "rodsType.h"
#include "objStat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STDOUT_FILE_NAME	"-"	/* pipe to stdout */

typedef struct RodsPath {
    objType_t objType;
    objStat_t objState;
    rodsLong_t size;
    uint objMode;
    char inPath[MAX_NAME_LEN];	 /* input from command line */
    char outPath[MAX_NAME_LEN];	 /* the path after parsing the inPath */
    char dataId[NAME_LEN];
    char chksum[NAME_LEN];
    rodsObjStat_t *rodsObjStat;
} rodsPath_t;

/* This is the struct for a command line path input. Normally it contains
 * one or more source input paths and 0 or 1 destination paths */

typedef struct RodsPathInp {
    int numSrc;
    rodsPath_t *srcPath;	/* pointr to an array of rodsPath_t */
    rodsPath_t *destPath;
    rodsPath_t *targPath;	/* This is a target path for a
                                  * source/destination type command */
    int resolved;
} rodsPathInp_t;

/* definition for flag in parseCmdLinePath */

#define	ALLOW_NO_SRC_FLAG	0x1

int
parseRodsPath( rodsPath_t *rodsPath, rodsEnv *myRodsEnv );
int
parseRodsPathStr( char *inPath, rodsEnv *myRodsEnv, char *outPath );
int
addSrcInPath( rodsPathInp_t *rodsPathInp, const char *inPath );
int
parseLocalPath( rodsPath_t *rodsPath );
int
parseCmdLinePath( int argc, char **argv, int optInd, rodsEnv *myRodsEnv,
                  int srcFileType, int destFileType, int flag, rodsPathInp_t *rodsPathInp );

int
getLastPathElement( char *inPath, char *lastElement );

int
getFileType( rodsPath_t *rodsPath );
void
clearRodsPath( rodsPath_t *rodsPath );
#ifdef __cplusplus
}
#endif

#endif	// RODS_PATH_H__
