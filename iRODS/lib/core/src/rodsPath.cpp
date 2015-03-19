/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rodsPath - a number of string operations designed for secure string
 * copying.
 */

#include "rodsPath.hpp"
#include "miscUtil.hpp"
#include "rcMisc.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#ifdef windows_platform
#include "irodsntutil.hpp"
#endif

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

/* parseRodsPathStr - This is similar to parseRodsPath except the
 * input and output are char string inPath and outPath
 */

int
parseRodsPathStr( char *inPath, rodsEnv *myRodsEnv, char *outPath ) {
    int status;
    rodsPath_t rodsPath;

    rstrcpy( rodsPath.inPath, inPath, MAX_NAME_LEN );

    status = parseRodsPath( &rodsPath, myRodsEnv );

    if ( status < 0 ) {
        return status;
    }

    rstrcpy( outPath, rodsPath.outPath, MAX_NAME_LEN );

    return status;
}

/* parseRodsPath - Parase a rods path into full rods path taking care of
 * all the "/../", "/./" and "." entries in the path.
 * The input is taken from rodsPath->inPath.
 * The output path is written in rodsPath->outPath.
 *
 */

int
parseRodsPath( rodsPath_t *rodsPath, rodsEnv *myRodsEnv ) {
    int len;
    char *tmpPtr1, *tmpPtr2;
    char tmpStr[MAX_NAME_LEN];


    if ( rodsPath == NULL ) {
        fprintf( stderr, "parseRodsPath: NULL rodsPath input\n" );
        return USER__NULL_INPUT_ERR;
    }

    rodsPath->objType = UNKNOWN_OBJ_T;
    rodsPath->objState = UNKNOWN_ST;

    if ( myRodsEnv == NULL && rodsPath->inPath[0] != '/' ) {
        fprintf( stderr, "parseRodsPath: NULL myRodsEnv input\n" );
        return USER__NULL_INPUT_ERR;
    }

    len = strlen( rodsPath->inPath );

    if ( len == 0 ) {
        /* just copy rodsCwd */
        rstrcpy( rodsPath->outPath, myRodsEnv->rodsCwd, MAX_NAME_LEN );
        rodsPath->objType = COLL_OBJ_T;
        return 0;
    }
    else if ( strcmp( rodsPath->inPath, "." ) == 0 ||
              strcmp( rodsPath->inPath, "./" ) == 0 ) {
        /* '.' or './' */
        rstrcpy( rodsPath->outPath, myRodsEnv->rodsCwd, MAX_NAME_LEN );
        rodsPath->objType = COLL_OBJ_T;
        return 0;
    }
    else if ( strcmp( rodsPath->inPath, "~" ) == 0 ||
              strcmp( rodsPath->inPath, "~/" ) == 0 ||
              strcmp( rodsPath->inPath, "^" ) == 0 ||
              strcmp( rodsPath->inPath, "^/" ) == 0 ) {
        /* ~ or ~/ */
        rstrcpy( rodsPath->outPath, myRodsEnv->rodsHome, MAX_NAME_LEN );
        rodsPath->objType = COLL_OBJ_T;
        return 0;
    }
    else if ( rodsPath->inPath[0] == '~' || rodsPath->inPath[0] == '^' ) {
        if ( rodsPath->inPath[1] == '/' ) {
            snprintf( rodsPath->outPath, MAX_NAME_LEN, "%s/%s",
                      myRodsEnv->rodsHome, rodsPath->inPath + 2 );
        }
        else {
            /* treat it like a relative path */
            snprintf( rodsPath->outPath, MAX_NAME_LEN, "%s/%s",
                      myRodsEnv->rodsCwd, rodsPath->inPath + 2 );
        }
    }
    else if ( rodsPath->inPath[0] == '/' ) {
        /* full path */
        rstrcpy( rodsPath->outPath, rodsPath->inPath, MAX_NAME_LEN );
    }
    else {
        /* a relative path */
        snprintf( rodsPath->outPath, MAX_NAME_LEN, "%s/%s",
                  myRodsEnv->rodsCwd, rodsPath->inPath );
    }

    /* take out any "//" */

    while ( ( tmpPtr1 = strstr( rodsPath->outPath, "//" ) ) != NULL ) {
        //    rstrcpy (tmpPtr1 + 1, tmpPtr1 + 2, MAX_NAME_LEN);
        rstrcpy( tmpStr, tmpPtr1 + 2, MAX_NAME_LEN );
        rstrcpy( tmpPtr1 + 1, tmpStr, MAX_NAME_LEN );
    }

    /* take out any "/./" */

    while ( ( tmpPtr1 = strstr( rodsPath->outPath, "/./" ) ) != NULL ) {
        //    rstrcpy (tmpPtr1 + 1, tmpPtr1 + 3, MAX_NAME_LEN);
        rstrcpy( tmpStr, tmpPtr1 + 3, MAX_NAME_LEN );
        rstrcpy( tmpPtr1 + 1, tmpStr, MAX_NAME_LEN );
    }

    /* take out any /../ */

    while ( ( tmpPtr1 = strstr( rodsPath->outPath, "/../" ) ) != NULL ) {
        /* go back */
        tmpPtr2 = tmpPtr1 - 1;
        while ( *tmpPtr2 != '/' ) {
            tmpPtr2 --;
            if ( tmpPtr2 < rodsPath->outPath ) {
                rodsLog( LOG_ERROR,
                         "parseRodsPath: parsing error for %s",
                         rodsPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
        }
        //    rstrcpy (tmpPtr2 + 1, tmpPtr1 + 4, MAX_NAME_LEN);
        rstrcpy( tmpStr, tmpPtr1 + 4, MAX_NAME_LEN );
        rstrcpy( tmpPtr2 + 1, tmpStr, MAX_NAME_LEN );
    }

    /* handle "/.", "/.." and "/" at the end */

    len = strlen( rodsPath->outPath );

    tmpPtr1 = rodsPath->outPath + len;

    if ( ( tmpPtr2 = strstr( tmpPtr1 - 3, "/.." ) ) != NULL ) {
        /* go back */
        tmpPtr2 -= 1;
        while ( *tmpPtr2 != '/' ) {
            tmpPtr2 --;
            if ( tmpPtr2 < rodsPath->outPath ) {
                rodsLog( LOG_ERROR,
                         "parseRodsPath: parsing error for %s",
                         rodsPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
        }
        *tmpPtr2 = '\0';
        if ( tmpPtr2 == rodsPath->outPath ) { /* nothing, special case */
            *tmpPtr2++ = '/'; /* root */
            *tmpPtr2 = '\0';
        }
        rodsPath->objType = COLL_OBJ_T;
        if ( strlen( rodsPath->outPath ) >= MAX_PATH_ALLOWED - 1 ) {
            return USER_PATH_EXCEEDS_MAX;
        }
        return 0;
    }

    /* take out "/." */

    if ( ( tmpPtr2 = strstr( tmpPtr1 - 2, "/." ) ) != NULL ) {
        *tmpPtr2 = '\0';
        rodsPath->objType = COLL_OBJ_T;
        if ( strlen( rodsPath->outPath ) >= MAX_PATH_ALLOWED - 1 ) {
            return USER_PATH_EXCEEDS_MAX;
        }
        return 0;
    }

    if ( *( tmpPtr1 - 1 ) == '/' && len > 1 ) {
        *( tmpPtr1 - 1 ) = '\0';
        rodsPath->objType = COLL_OBJ_T;
        if ( strlen( rodsPath->outPath ) >= MAX_PATH_ALLOWED - 1 ) {
            return USER_PATH_EXCEEDS_MAX;
        }
        return 0;
    }
    if ( strlen( rodsPath->outPath ) >= MAX_PATH_ALLOWED - 1 ) {
        return USER_PATH_EXCEEDS_MAX;
    }
    return 0;
}

/* parseLocalPath - Parse a local path into full path taking care of
 * all the "/../", "/./" and "." entries in the path.
 * The input is taken from rodsPath->inPath.
 * The output path is written in rodsPath->outPath.
 * In addition, a stat is done on the rodsPath->outPath to determine the
 * objType and size of the file.
 */

int
parseLocalPath( rodsPath_t *rodsPath ) {
    if ( rodsPath == NULL ) {
        fprintf( stderr, "parseLocalPath: NULL rodsPath input\n" );
        return USER__NULL_INPUT_ERR;
    }

    if ( strlen( rodsPath->inPath ) == 0 ) {
        rstrcpy( rodsPath->outPath, ".", MAX_NAME_LEN );
    }
    else {
        rstrcpy( rodsPath->outPath, rodsPath->inPath, MAX_NAME_LEN );
    }

    return getFileType( rodsPath );
}

int
getFileType( rodsPath_t *rodsPath ) {
    boost::filesystem::path p( rodsPath->outPath );

    try {
        if ( !exists( p ) ) {
            rodsPath->objType = UNKNOWN_FILE_T;
            rodsPath->objState = NOT_EXIST_ST;
            return NOT_EXIST_ST;
        }
        else if ( is_regular_file( p ) ) {
            rodsPath->objType = LOCAL_FILE_T;
            rodsPath->objState = EXIST_ST;
            rodsPath->size = file_size( p );
        }
        else if ( is_directory( p ) ) {
            rodsPath->objType = LOCAL_DIR_T;
            rodsPath->objState = EXIST_ST;
        }
    } catch ( const boost::filesystem::filesystem_error& e ) {
        fprintf( stderr, "%s\n", e.what() );
        return SYS_NO_PATH_PERMISSION;
    }


    return rodsPath->objType;
}

int
addSrcInPath( rodsPathInp_t *rodsPathInp, char *inPath ) {
    rodsPath_t *newSrcPath, *newTargPath;
    int newNumSrc;

    if ( rodsPathInp == NULL || inPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "addSrcInPath: NULL input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( ( rodsPathInp->numSrc % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newNumSrc = rodsPathInp->numSrc + PTR_ARRAY_MALLOC_LEN;
        newSrcPath = ( rodsPath_t * ) malloc( newNumSrc * sizeof( rodsPath_t ) );
        newTargPath = ( rodsPath_t * ) malloc( newNumSrc * sizeof( rodsPath_t ) );
        memset( ( void * ) newSrcPath, 0, newNumSrc * sizeof( rodsPath_t ) );
        memset( ( void * ) newTargPath, 0, newNumSrc * sizeof( rodsPath_t ) );
        if ( rodsPathInp->numSrc > 0 ) {
            memcpy( newSrcPath, rodsPathInp->srcPath,
                    rodsPathInp->numSrc * sizeof( rodsPath_t ) );
            memcpy( newTargPath, rodsPathInp->targPath,
                    rodsPathInp->numSrc * sizeof( rodsPath_t ) );
            free( rodsPathInp->srcPath );
            free( rodsPathInp->targPath );
        }
        rodsPathInp->srcPath = newSrcPath;
        rodsPathInp->targPath = newTargPath;
    }
    else {
        newSrcPath = rodsPathInp->srcPath;
    }
    rstrcpy( newSrcPath[rodsPathInp->numSrc].inPath, inPath, MAX_NAME_LEN );
    rodsPathInp->numSrc++;

    return 0;
}

/* parseCmdLinePath - parse the iCommand line path input. It can parse
 * path input of the form:
 * 1) srcPath1, srcPath2 ... destPath
 * 2) srcPath1, srcPath2 ...
 *
 * The source and destination paths can be irodsPath (data object) or
 * local path.
 * srcFileType/destFileType = UNKNOWN_OBJ_T means the path is a irodsPath.
 * srcFileType/destFileType = UNKNOWN_FILE_T means the path is a local path
 * destFileType = NULL_T means there is no destPath.
 *
 * argc and argv should be the usual values directly from the main ()
 * routine.
 * optind - diectly from the getopt call if the path inputs are directly
 * behind the input options or equal to (optind + n) where
 * where n is number of arguements before the path inputs.
 *
 * rodsPathInp - The output struct.
 * typedef struct RodsPathInp {
 *     int numSrc;
 *     rodsPath_t *srcPath;         // pointer to an array of rodsPath_t
 *     rodsPath_t *destPath;
 * } rodsPathInp_t;
 *
 * typedef struct RodsPath {
 *    objType_t objType;
 *    rodsLong_t size;
 *    char inPath[MAX_NAME_LEN];
 *    char outPath[MAX_NAME_LEN];
 * } rodsPath_t;
 *
 * inPath contains the input value.
 * outPath contains the parsed output path.
 *
 * Note that parseCmdLinePath only parses the path. It does not resolve the
 * dependency of the destination path to the source path.
 * e.g., "iput foo1 ."
 * The resolveRodsTarget() should be called to resolve the dependency.
 *
 */

int
parseCmdLinePath( int argc, char **argv, int optInd, rodsEnv *myRodsEnv,
                  int srcFileType, int destFileType, int flag, rodsPathInp_t *rodsPathInp ) {
    int nInput;
    int i, status;
    int numSrc;

    nInput = argc - optInd;

    if ( rodsPathInp == NULL ) {
        rodsLog( LOG_ERROR, "parseCmdLinePath: NULL rodsPathInp input" );
        return USER__NULL_INPUT_ERR;
    }

    memset( rodsPathInp, 0, sizeof( rodsPathInp_t ) );

    if ( nInput <= 0 ) {
        if ( ( flag & ALLOW_NO_SRC_FLAG ) == 0 ) {
            return USER__NULL_INPUT_ERR;
        }
        else {
            numSrc = 1;
        }
    }
    else if ( nInput == 1 ) {
        numSrc = 1;
    }
    else if ( destFileType == NO_INPUT_T ) {            /* no dest input */
        numSrc = nInput;
    }
    else {
        numSrc = nInput - 1;
    }

    for ( i = 0; i < numSrc; i++ ) {
        if ( nInput <= 0 ) {
            /* just add cwd */
            addSrcInPath( rodsPathInp, "." );
        }
        else {
            addSrcInPath( rodsPathInp, argv[optInd + i] );
        }
        if ( srcFileType <= COLL_OBJ_T ) {
            status = parseRodsPath( &rodsPathInp->srcPath[i], myRodsEnv );
        }
        else {
            status = parseLocalPath( &rodsPathInp->srcPath[i] );
        }
        if ( status < 0 ) {
            return status;
        }
    }

    if ( destFileType != NO_INPUT_T ) {
        rodsPathInp->destPath = ( rodsPath_t* )malloc( sizeof( rodsPath_t ) );
        memset( rodsPathInp->destPath, 0, sizeof( rodsPath_t ) );
        if ( nInput > 1 ) {
            rstrcpy( rodsPathInp->destPath->inPath, argv[argc - 1],
                     MAX_NAME_LEN );
        }
        else {
            rstrcpy( rodsPathInp->destPath->inPath, ".", MAX_NAME_LEN );
        }

        if ( destFileType <= COLL_OBJ_T ) {
            status = parseRodsPath( rodsPathInp->destPath, myRodsEnv );
        }
        else {
            status = parseLocalPath( rodsPathInp->destPath );
        }
    }

    return status;
}

/* resolveRodsTarget - based on srcPath and destPath, fill in targPath.
 * oprType -
 *      MOVE_OPR - do not create the target coll or dir because rename will
 *        take care of it.
 *      RSYNC_OPR - udes the destPath and the targPath if the src is a
 *        collection
 *      All other oprType will be treated as normal.
 */
int
resolveRodsTarget( rcComm_t *conn, rodsPathInp_t *rodsPathInp, int oprType ) {
    rodsPath_t *srcPath, *destPath;
    char srcElement[MAX_NAME_LEN], destElement[MAX_NAME_LEN];
    int status;
    int srcInx;
    rodsPath_t *targPath;

    if ( rodsPathInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "resolveRodsTarget: NULL rodsPathInp input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsPathInp->srcPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "resolveRodsTarget: NULL rodsPathInp->srcPath input" );
        return USER__NULL_INPUT_ERR;
    }

    if ( rodsPathInp->destPath == NULL ) {
        rodsLog( LOG_ERROR,
                 "resolveRodsTarget: NULL rodsPathInp->destPath input" );
        return USER__NULL_INPUT_ERR;
    }

    destPath = rodsPathInp->destPath;
    if ( destPath->objState == UNKNOWN_ST ) {
        getRodsObjType( conn, destPath );
    }

    for ( srcInx = 0; srcInx < rodsPathInp->numSrc; srcInx++ ) {
        srcPath = &rodsPathInp->srcPath[srcInx];
        targPath = &rodsPathInp->targPath[srcInx];

        /* we don't do wild card yet */

        if ( srcPath->objState == UNKNOWN_ST ) {
            getRodsObjType( conn, srcPath );
            if ( srcPath->objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: srcPath %s does not exist",
                         srcPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
        }

        if ( destPath->objType >= UNKNOWN_FILE_T &&
                strcmp( destPath->outPath, STDOUT_FILE_NAME ) == 0 ) {
            /* pipe to stdout */
            if ( srcPath->objType != DATA_OBJ_T ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: src %s is the wrong type for dest -",
                         srcPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
            *targPath = *destPath;
            targPath->objType = LOCAL_FILE_T;
        }
        else if ( srcPath->objType == DATA_OBJ_T ||
                  srcPath->objType == LOCAL_FILE_T ) {
            /* file type source */

            if ( ( destPath->objType == COLL_OBJ_T ||
                    destPath->objType == LOCAL_DIR_T ) &&
                    destPath->objState == EXIST_ST ) {
                if ( destPath->objType <= COLL_OBJ_T ) {
                    targPath->objType = DATA_OBJ_T;
                }
                else {
                    targPath->objType = LOCAL_FILE_T;
                }

                /* collection */
                getLastPathElement( srcPath->inPath, srcElement );
                if ( strlen( srcElement ) > 0 ) {
                    snprintf( targPath->outPath, MAX_NAME_LEN, "%s/%s",
                              destPath->outPath, srcElement );
                    if ( destPath->objType <= COLL_OBJ_T ) {
                        getRodsObjType( conn, destPath );
                    }
                }
                else {
                    rstrcpy( targPath->outPath, destPath->outPath,
                             MAX_NAME_LEN );
                }
            }
            else if ( destPath->objType == DATA_OBJ_T ||
                      destPath->objType == LOCAL_FILE_T || rodsPathInp->numSrc == 1 ) {
                *targPath = *destPath;
                if ( destPath->objType <= COLL_OBJ_T ) {
                    targPath->objType = DATA_OBJ_T;
                }
                else {
                    targPath->objType = LOCAL_FILE_T;
                }
            }
            else {
                rodsLogError( LOG_ERROR, USER_FILE_DOES_NOT_EXIST,
                              "resolveRodsTarget: target %s does not exist",
                              destPath->outPath );
                return USER_FILE_DOES_NOT_EXIST;
            }
        }
        else if ( srcPath->objType == COLL_OBJ_T ||
                  srcPath->objType == LOCAL_DIR_T ) {
            /* directory type source */

            if ( destPath->objType <= COLL_OBJ_T ) {
                targPath->objType = COLL_OBJ_T;
            }
            else {
                targPath->objType = LOCAL_DIR_T;
            }

            if ( destPath->objType == DATA_OBJ_T ||
                    destPath->objType == LOCAL_FILE_T ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: input destPath %s is a datapath",
                         destPath->outPath );
                return USER_INPUT_PATH_ERR;
            }
            else if ( ( destPath->objType == COLL_OBJ_T ||
                        destPath->objType == LOCAL_DIR_T ) &&
                      destPath->objState == EXIST_ST ) {
                /* the collection exist */
                getLastPathElement( srcPath->inPath, srcElement );
                if ( strlen( srcElement ) > 0 ) {
                    if ( rodsPathInp->numSrc == 1 && oprType == RSYNC_OPR ) {
                        getLastPathElement( destPath->inPath, destElement );
                        /* RSYNC_OPR. Just use the same path */
                        if ( strlen( destElement ) > 0 ) {
                            rstrcpy( targPath->outPath, destPath->outPath,
                                     MAX_NAME_LEN );
                        }
                    }
                    if ( targPath->outPath[0] == '\0' ) {
                        snprintf( targPath->outPath, MAX_NAME_LEN, "%s/%s",
                                  destPath->outPath, srcElement );
                        /* make the collection */
                        if ( destPath->objType == COLL_OBJ_T ) {
                            if ( oprType != MOVE_OPR ) {
                                /* rename does not need to mkColl */
                                if ( srcPath->objType <= COLL_OBJ_T ) {
                                    status = mkColl( conn, destPath->outPath );
                                }
                                else {
                                    status = mkColl( conn, targPath->outPath );
                                }
                            }
                            else {
                                status = 0;
                            }
                        }
                        else {
#ifdef windows_platform
                            status = iRODSNt_mkdir( targPath->outPath, 0750 );
#else
                            status = mkdir( targPath->outPath, 0750 );
#endif
                            if ( status < 0 && errno == EEXIST ) {
                                status = 0;
                            }
                        }
                        if ( status < 0 ) {
                            rodsLogError( LOG_ERROR, status,
                                          "resolveRodsTarget: mkColl/mkdir for %s,status=%d",
                                          targPath->outPath, status );
                            return status;
                        }
                    }
                }
                else {
                    rstrcpy( targPath->outPath, destPath->outPath,
                             MAX_NAME_LEN );
                }
            }
            else {
                /* dest coll does not exist */
                if ( destPath->objType <= COLL_OBJ_T ) {
                    if ( oprType != MOVE_OPR ) {
                        /* rename does not need to mkColl */
                        status = mkColl( conn, destPath->outPath );
                    }
                    else {
                        status = 0;
                    }
                }
                else {
                    /* use destPath. targPath->outPath not defined.
                     *  status = mkdir (targPath->outPath, 0750); */
#ifdef windows_platform
                    status = iRODSNt_mkdir( destPath->outPath, 0750 );
#else
                    status = mkdir( destPath->outPath, 0750 );
#endif
                }
                if ( status < 0 ) {
                    return status;
                }

                if ( rodsPathInp->numSrc == 1 ) {
                    rstrcpy( targPath->outPath, destPath->outPath,
                             MAX_NAME_LEN );
                }
                else {
                    rodsLogError( LOG_ERROR, USER_FILE_DOES_NOT_EXIST,
                                  "resolveRodsTarget: target %s does not exist",
                                  destPath->outPath );
                    return USER_FILE_DOES_NOT_EXIST;
                }
            }
            targPath->objState = EXIST_ST;
        }
        else {          /* should not be here */
            if ( srcPath->objState == NOT_EXIST_ST ) {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: source %s does not exist",
                         srcPath->outPath );
            }
            else {
                rodsLog( LOG_ERROR,
                         "resolveRodsTarget: cannot handle objType %d for srcPath %s",
                         srcPath->objType, srcPath->outPath );
            }
            return USER_INPUT_PATH_ERR;
        }
    }
    return 0;
}

int
getLastPathElement( char *inInPath, char *lastElement ) {
    char mydir[MAX_NAME_LEN];
    char inPath[MAX_NAME_LEN];
    int len;
    char *tmpPtr1, *tmpPtr2;

    if ( inInPath == NULL ) {
        *lastElement = '\0';
        return 0;
    }
    snprintf( inPath, sizeof( inPath ), "%s", inInPath );
#ifdef windows_platform
    iRODSNtPathForwardSlash( inPath );
#endif


    splitPathByKey( inPath, mydir, MAX_NAME_LEN, lastElement, MAX_NAME_LEN, '/' );

    len = strlen( lastElement );

    if ( len == 0 ) {
        len = strlen( inPath );
        if ( len == 0 ) {
            *lastElement = '\0';
            return 0;
        }
        else {
            rstrcpy( lastElement, inPath, MAX_NAME_LEN );
            len = strlen( lastElement );
        }
    }

    tmpPtr1 = lastElement + len;
    if ( len >= 2 ) {
        tmpPtr2 = tmpPtr1 - 2;
        if ( strcmp( tmpPtr2, "/." ) == 0 || strcmp( tmpPtr2, ".." ) == 0 ) {
            *tmpPtr2 = '\0';
            return 0;
        }
    }

    if ( len >= 1 ) {
        tmpPtr2 = tmpPtr1 - 1;
        if ( *tmpPtr2 == '.' || *tmpPtr2 == '~' ||
                *tmpPtr2 == '^' || *tmpPtr2 == '/' ) {
            *tmpPtr2 = '\0';
            return 0;
        }
    }

    return 0;
}

void
clearRodsPath( rodsPath_t *rodsPath ) {
    if ( rodsPath == NULL ) {
        return;
    }

    if ( rodsPath->rodsObjStat == NULL ) {
        return;
    }

    freeRodsObjStat( rodsPath->rodsObjStat );

    memset( rodsPath, 0, sizeof( rodsPath_t ) );

    return;
}

