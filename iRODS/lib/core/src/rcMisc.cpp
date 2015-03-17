/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rcMisc.c - misc client routines
 */
#ifndef windows_platform
#include <sys/time.h>
#include <sys/wait.h>
#else
#include "Unix2Nt.hpp"
#endif
#include "rcMisc.hpp"
#include "apiHeaderAll.hpp"
#include "modDataObjMeta.hpp"
#include "rcGlobalExtern.hpp"
#include "rodsGenQueryNames.hpp"
#include "rodsType.hpp"
#include "rsApiHandler.hpp"
#include "dataObjPut.hpp"
#include "reFuncDefs.hpp"

#include "bulkDataObjPut.hpp"
#include "putUtil.hpp"
#include "sockComm.hpp"

#include <cstdlib>
#include <iostream>
#include <algorithm>

// =-=-=-=-=-=-=-
#include "irods_virtual_path.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"
#include "irods_client_api_table.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
using namespace boost::filesystem;

/* check with the input path is a valid path -
 * 1 - valid
 * 0 - not valid
 */

int
isPath( char *myPath ) {
    path p( myPath );

    if ( exists( p ) ) {
        return 1;
    }
    else {
        return 0;
    }
}

rodsLong_t
getFileSize( char *myPath ) {
    path p( myPath );

    if ( exists( p ) && is_regular_file( p ) ) {
        return file_size( p );
    }
    else {
        return -1;
    }
}


int freeBBuf( bytesBuf_t *myBBuf ) {
    if ( myBBuf == NULL ) {
        return 0;
    }

    if ( myBBuf->buf != NULL ) {
        free( myBBuf->buf );
    }
    free( myBBuf );
    return 0;
}

int
clearBBuf( bytesBuf_t *myBBuf ) {
    if ( myBBuf == NULL ) {
        return 0;
    }

    if ( myBBuf->buf != NULL ) {
        free( myBBuf->buf );
    }

    memset( myBBuf, 0, sizeof( bytesBuf_t ) );
    return 0;
}

/* addRErrorMsg - Add an error msg to the rError_t struct.
 *    rError_t *myError - the rError_t struct  for the error msg.
 *    int status - the input error status.
 *    char *msg - the error msg string. This string will be copied to myError.
 */

int
addRErrorMsg( rError_t *myError, int status, const char *msg ) {
    rErrMsg_t **newErrMsg;
    int newLen;
    int i;

    if ( myError == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ( myError->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newLen = myError->len + PTR_ARRAY_MALLOC_LEN;
        newErrMsg = ( rErrMsg_t ** ) malloc( newLen * sizeof( *newErrMsg ) );
        memset( newErrMsg, 0, newLen * sizeof( *newErrMsg ) );
        for ( i = 0; i < myError->len; i++ ) {
            newErrMsg[i] = myError->errMsg[i];
        }
        if ( myError->errMsg != NULL ) {
            free( myError->errMsg );
        }
        myError->errMsg = newErrMsg;
    }

    myError->errMsg[myError->len] = ( rErrMsg_t* )malloc( sizeof( rErrMsg_t ) );
    strncpy( myError->errMsg[myError->len]->msg, msg, ERR_MSG_LEN - 1 );
    myError->errMsg[myError->len]->status = status;
    myError->len++;

    return 0;
}

int
replErrorStack( rError_t *srcRError, rError_t *destRError ) {
    int i, len;
    rErrMsg_t *errMsg;

    if ( srcRError == NULL || destRError == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    len = srcRError->len;

    for ( i = 0; i < len; i++ ) {
        errMsg = srcRError->errMsg[i];
        addRErrorMsg( destRError, errMsg->status, errMsg->msg );
    }
    return 0;
}

int
freeRError( rError_t *myError ) {

    if ( myError == NULL ) {
        return 0;
    }

    freeRErrorContent( myError );
    free( myError );
    return 0;
}

int
freeRErrorContent( rError_t *myError ) {
    int i;

    if ( myError == NULL ) {
        return 0;
    }

    if ( myError->len > 0 ) {
        for ( i = 0; i < myError->len; i++ ) {
            free( myError->errMsg[i] );
        }
        free( myError->errMsg );
    }

    memset( myError, 0, sizeof( rError_t ) );

    return 0;
}

/*
   Parse the input fullUserNameIn into an output userName and userZone
   and check that the username is a valid format, meaning at most one
   '@' and at most one '#'.
   Full userNames are of the form user@department[#zone].
   It is assumed the output strings are at least NAME_LEN characters long
   and the input string is at most that long.
 */
int
parseUserName( const char *fullUserNameIn, char *userName, char *userZone ) {
    const std::string input( fullUserNameIn );
    boost::smatch matches;
    // This regex matches usernames with no hashes and optionally one at symbol,
    // and then optionally a hash followed by a zone name containing no hashes.
    //
    // Username must be between 1 and NAME_LEN-1 characters.
    // Username may contain any combination of word characters, @ symbols, dashes, and dots.
    // Username may not be . or .., as we create home directories for users
    const boost::regex expression( "((\\w|[-.@])+)(#([^#]*))?" );
    try {
        const bool matched = boost::regex_match( input, matches, expression );
        if ( !matched || matches.str( 1 ).size() >= NAME_LEN ||
                matches.str( 1 ).size() < 1 ||
                matches.str( 4 ).size() >= NAME_LEN ||
                matches.str( 1 ) == "." ||
                matches.str( 1 ) == ".." ) {
            if ( userName != NULL ) {
                userName[0] = '\0';
            }
            if ( userZone != NULL ) {
                userZone[0] = '\0';
            }
            return USER_INVALID_USERNAME_FORMAT;
        }
        if ( userName != NULL ) {
            snprintf( userName, NAME_LEN, "%s", matches.str( 1 ).c_str() );
        }
        if ( userZone != NULL ) {
            snprintf( userZone, NAME_LEN, "%s", matches.str( 4 ).c_str() );
        }
    }
    catch ( const boost::exception& ) {
        return SYS_INTERNAL_ERR;
    }
    return 0;
}

int
apiTableLookup( int apiNumber ) {
#ifndef RODS_SERVER
    return apiNumber;
#else
    irods::api_entry_table& RcApiTable = irods::get_client_api_table();
    if ( RcApiTable.find( apiNumber ) != RcApiTable.end() ) {
        return apiNumber;
    }
    return SYS_UNMATCHED_API_NUM;
#endif
}

int
myHtonll( rodsLong_t inlonglong, rodsLong_t *outlonglong ) {
    char *inPtr, *outPtr;

    if ( outlonglong == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ntohl( 1 ) == 1 ) {
        *outlonglong = inlonglong;
        return 0;
    }

    inPtr = ( char * )( static_cast< void * >( &inlonglong ) );
    outPtr = ( char * )( static_cast<void *>( outlonglong ) );

    int i;
    int byte_length = sizeof( rodsLong_t );
    for ( i = 0; i < byte_length; i++ ) {
        outPtr[i] = inPtr[byte_length - 1 - i];
    }
    return 0;
}

int
myNtohll( rodsLong_t inlonglong,  rodsLong_t *outlonglong ) {
    char *inPtr, *outPtr;

    if ( outlonglong == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ntohl( 1 ) == 1 ) {
        *outlonglong = inlonglong;
        return 0;
    }

    inPtr = ( char * )( static_cast< void * >( &inlonglong ) );
    outPtr = ( char * )( static_cast<void *>( outlonglong ) );

    int i;
    int byte_length = sizeof( rodsLong_t );
    for ( i = 0; i < byte_length; i++ ) {
        outPtr[i] = inPtr[byte_length - 1 - i];
    }
    return 0;
}

int
statToRodsStat( rodsStat_t *rodsStat, struct stat *myFileStat ) {
    if ( rodsStat == NULL || myFileStat == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    rodsStat->st_size = myFileStat->st_size;
    rodsStat->st_dev = myFileStat->st_dev;
    rodsStat->st_ino = myFileStat->st_ino;
    rodsStat->st_mode = myFileStat->st_mode;
    rodsStat->st_nlink = myFileStat->st_nlink;
    rodsStat->st_uid = myFileStat->st_uid;
    rodsStat->st_gid = myFileStat->st_gid;
    rodsStat->st_rdev = myFileStat->st_rdev;
    rodsStat->st_atim = myFileStat->st_atime;
    rodsStat->st_mtim = myFileStat->st_mtime;
    rodsStat->st_ctim = myFileStat->st_ctime;
#ifndef _WIN32
    rodsStat->st_blksize = myFileStat->st_blksize;
    rodsStat->st_blocks = myFileStat->st_blocks;
#endif

    return 0;
}

int
rodsStatToStat( struct stat *myFileStat, rodsStat_t *rodsStat ) {
    if ( myFileStat == NULL || rodsStat == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    myFileStat->st_size = rodsStat->st_size;
    myFileStat->st_dev = rodsStat->st_dev;
    myFileStat->st_ino = rodsStat->st_ino;
    myFileStat->st_mode = rodsStat->st_mode;
    myFileStat->st_nlink = rodsStat->st_nlink;
    myFileStat->st_uid = rodsStat->st_uid;
    myFileStat->st_gid = rodsStat->st_gid;
    myFileStat->st_rdev = rodsStat->st_rdev;
    myFileStat->st_atime = rodsStat->st_atim;
    myFileStat->st_mtime = rodsStat->st_mtim;
    myFileStat->st_ctime = rodsStat->st_ctim;
#ifndef _WIN32
    myFileStat->st_blksize = rodsStat->st_blksize;
    myFileStat->st_blocks = rodsStat->st_blocks;
#endif

    return 0;
}

int
direntToRodsDirent( struct rodsDirent *dirent, struct dirent *fileDirent ) {
    if ( dirent == NULL || fileDirent == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    strcpy( dirent->d_name, fileDirent->d_name );

#if defined(linux_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = 0;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = _D_EXACT_NAMLEN( fileDirent );
#elif defined(solaris_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = fileDirent->d_off;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = 0;
#elif defined(aix_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = fileDirent->d_offset;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = fileDirent->d_namlen;
#elif defined(sgi_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = fileDirent->d_off;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = 0;
#elif defined(sunos_platform)
    dirent->d_ino = fileDirent->d_fileno;
    dirent->d_offset = fileDirent->d_off;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = fileDirent->d_namlen;
#elif defined(alpha_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = 0;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = fileDirent->d_namlen;
#elif defined(osx_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = 0;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = fileDirent->d_namlen;
#elif defined(windows_platform)
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = 0;
    dirent->d_reclen = 0;
    dirent->d_namlen = 0;
#else   /* unknown. use linux */
    dirent->d_ino = fileDirent->d_ino;
    dirent->d_offset = 0;
    dirent->d_reclen = fileDirent->d_reclen;
    dirent->d_namlen = _D_EXACT_NAMLEN( fileDirent );
#endif
    return 0;
}

/* getStrInBuf get the next str in buf.
 * treat # as comment. ignore any leading white space.
 * Return the length of str copied, not including NULL.
 * bufSize is the remaining len in inbuf to be processed
 */

int
getStrInBuf( char **inbuf, char *outbuf, int *inbufLen, int outbufLen ) {
    char *inPtr, *outPtr;
    int bytesCopied  = 0;
    int c;

    inPtr = *inbuf;
    outPtr = outbuf;

    while ( ( c = *inPtr ) != '\0' && *inbufLen > 0 ) {
        ( *inbufLen )--;
        inPtr ++;
        if ( isspace( c ) ) {
            if ( bytesCopied > 0 ) {
                break;
            }
            else {
                continue;
            }
        }
        else if ( c == '#' ) {
            break;
        }
        else {
            if ( bytesCopied >= outbufLen - 1 ) {
                rodsLog( LOG_ERROR,
                         "getStrInBuf: outbuf overflow buf len %d", bytesCopied );
                break;
            }
            *outPtr = c;
            bytesCopied ++;
            outPtr++;
        }
    }
    *outPtr = '\0';
    *inbuf = inPtr;
    return bytesCopied;
}

/* getNextEleInStr - same as getStrInBuf except it did not check for # */
int
getNextEleInStr( char **inbuf, char *outbuf, int *inbufLen, int maxOutLen ) {
    char *inPtr, *outPtr;
    int bytesCopied  = 0;
    int c;
    int hasQuote;

    inPtr = *inbuf;
    outPtr = outbuf;

    hasQuote = 0;
    while ( ( c = *inPtr ) != '\0' && *inbufLen > 0 ) {
        ( *inbufLen )--;
        inPtr ++;
        if ( isspace( c ) && hasQuote == False ) {
            if ( bytesCopied > 0 ) {
                break;
            }
            else {
                continue;
            }
        }
        else if ( c == '\'' ) {
            if ( hasQuote == True ) {
                inPtr ++;
                break;
            }
            else {
                hasQuote = True;
            }
        }
        else {
            if ( bytesCopied >= maxOutLen - 1 ) {
                rodsLog( LOG_ERROR,
                         "getNextEleInStr: outbuf overflow buf len %d", bytesCopied );
                break;
            }
            *outPtr = c;
            bytesCopied ++;
            outPtr++;
        }
    }
    *outPtr = '\0';
    *inbuf = inPtr;
    return bytesCopied;
}

/* getLine - Read the next line. Add a NULL char at the end.
 * return the length of the line including the terminating NULL.
 */

int
getLine( FILE *fp, char *buf, int bufSize ) {
    int c;
    int len = 0;
    char *cptr = buf;

    while ( ( c = getc( fp ) ) != EOF ) {
        if ( c == '\n' ) {
            break;
        }
        *cptr ++ = c;
        len ++;

        /* overflow buf ? */
        if ( len >= bufSize - 1 ) {
            rodsLog( LOG_ERROR, "getLine: buffer overflow bufSize %d",
                     bufSize );
            break;
        }

    }
    if ( c == EOF && len == 0 ) {
        return EOF;
    }
    else {
        *cptr ++ = '\0';
        len ++;
        return len;
    }
}

int getZoneNameFromHint(
    const char* _hint,
    char*       _zone_name,
    int         _len ) {
    if ( !_hint ) {
        _zone_name[0] = '\0';
        return 0;
    }

    const std::string sep = irods::get_virtual_path_separator();

    std::string hint( _hint );
    if ( sep[0] == hint[0] ) {
        std::string::size_type pos = hint.find( sep, 1 );
        if ( std::string::npos != pos ) {
            hint = hint.substr( 1, pos - 1 );

        }
        else {
            hint = hint.substr( 1 );

        }
    }

    snprintf( _zone_name, _len, "%s", hint.c_str() );

    return 0;
}

int
freeDataObjInfo( dataObjInfo_t *dataObjInfo ) {
    if ( dataObjInfo == NULL ) {
        return 0;
    }

    clearKeyVal( &dataObjInfo->condInput );
    /* separate specColl */
    if ( dataObjInfo->specColl != NULL ) {
        free( dataObjInfo->specColl );
    }

    free( dataObjInfo );
    dataObjInfo = 0;
    return 0;
}

int
freeAllDataObjInfo( dataObjInfo_t *dataObjInfoHead ) {
    dataObjInfo_t *tmpDataObjInfo, *nextDataObjInfo;

    tmpDataObjInfo = dataObjInfoHead;
    while ( tmpDataObjInfo != NULL ) {
        nextDataObjInfo = tmpDataObjInfo->next;
        freeDataObjInfo( tmpDataObjInfo );
        tmpDataObjInfo = nextDataObjInfo;
    }
    return 0;
}

/* queDataObjInfo - queue the input dataObjInfo in dataObjInfoHead queue.
 * Input
 * int singleInfoFlag - 1 - the input dataObjInfo is a single dataObjInfo.
 *                      0 - the input dataObjInfo is a link list
 * int topFlag - 1 - queue the input dataObjInfo at the head of the queue
 *               0 - queue the input dataObjInfo at the bottom of the
 *                   queue.
 */
int queDataObjInfo(
    dataObjInfo_t** dataObjInfoHead,
    dataObjInfo_t*  dataObjInfo,
    int             singleInfoFlag,
    int             topFlag ) {

    dataObjInfo_t *tmpDataObjInfo;

    if ( dataObjInfo == NULL ) {
        return -1;
    }

    if ( *dataObjInfoHead == NULL ) {
        *dataObjInfoHead = dataObjInfo;
        if ( singleInfoFlag > 0 ) {
            dataObjInfo->next = NULL;
        }
    }
    else {
        if ( topFlag > 0 ) {
            dataObjInfo_t *savedDataObjInfo;

            savedDataObjInfo = *dataObjInfoHead;
            *dataObjInfoHead = dataObjInfo;
            if ( singleInfoFlag > 0 ) {
                ( *dataObjInfoHead )->next = savedDataObjInfo;
            }
            else {
                /* have to drill down to find the last DataObjInfo */
                tmpDataObjInfo = *dataObjInfoHead;
                while ( tmpDataObjInfo->next != NULL ) {
                    tmpDataObjInfo = tmpDataObjInfo->next;
                }
                tmpDataObjInfo->next = savedDataObjInfo;
            }
        }
        else {
            tmpDataObjInfo = *dataObjInfoHead;
            while ( tmpDataObjInfo->next != NULL ) {
                tmpDataObjInfo = tmpDataObjInfo->next;
            }
            tmpDataObjInfo->next = dataObjInfo;

            if ( singleInfoFlag > 0 ) {
                dataObjInfo->next = NULL;
            }
        }
    }

    return 0;
}
// =-=-=-=-=-=-=-
// JMC - backport 4590
int
dequeDataObjInfo( dataObjInfo_t **dataObjInfoHead, dataObjInfo_t *dataObjInfo ) {
    dataObjInfo_t *tmpDataObjInfo;
    dataObjInfo_t *prevDataObjInfo = NULL;

    if ( dataObjInfo == NULL || dataObjInfoHead == NULL ) {
        return -1;
    }

    tmpDataObjInfo = *dataObjInfoHead;
    while ( tmpDataObjInfo != NULL ) {
        if ( tmpDataObjInfo == dataObjInfo ) {
            if ( prevDataObjInfo == NULL ) {
                *dataObjInfoHead = tmpDataObjInfo->next;
            }
            else {
                prevDataObjInfo->next = tmpDataObjInfo->next;
            }
            return 0;
        }
        prevDataObjInfo = tmpDataObjInfo;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }
    return -1;
}
// =-=-=-=-=-=-=-
int
getDataObjInfoCnt( dataObjInfo_t *dataObjInfoHead ) {
    dataObjInfo_t *tmpDataObjInfo;
    int cnt = 0;
    tmpDataObjInfo = dataObjInfoHead;

    while ( tmpDataObjInfo != NULL ) {
        cnt++;
        tmpDataObjInfo = tmpDataObjInfo->next;
    }

    return cnt;
}

char *
getValByKey( const keyValPair_t *condInput, const char *keyWord ) {
    int i;

    if ( condInput == NULL ) {
        return NULL;
    }

    for ( i = 0; i < condInput->len; i++ ) {
        if ( strcmp( condInput->keyWord[i], keyWord ) == 0 ) {
            return condInput->value[i];
        }
    }

    return NULL;
}

int
getIvalByInx( inxIvalPair_t *inxIvalPair, int inx, int *outValue ) {
    int i;

    if ( inxIvalPair == NULL ) {
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < inxIvalPair->len; i++ ) {
        if ( inxIvalPair->inx[i] == inx ) {
            *outValue = inxIvalPair->value[i];
            return 0;
        }
    }

    return UNMATCHED_KEY_OR_INDEX;
}

int
rmKeyVal( keyValPair_t *condInput, char *keyWord ) {
    int i, j;

    if ( condInput == NULL ) {
        return 0;
    }

    for ( i = 0; i < condInput->len; i++ ) {
        if ( condInput->keyWord[i] != NULL &&
                strcmp( condInput->keyWord[i], keyWord ) == 0 ) {
            free( condInput->keyWord[i] );
            free( condInput->value[i] );
            condInput->len--;
            for ( j = i; j < condInput->len; j++ ) {
                condInput->keyWord[j] = condInput->keyWord[j + 1];
                condInput->value[j] = condInput->value[j + 1];
            }
            if ( condInput->len <= 0 ) {
                free( condInput->keyWord );
                free( condInput->value );
                condInput->value = condInput->keyWord = NULL;
            }
            break;
        }
    }
    return 0;
}

int
replKeyVal( const keyValPair_t *srcCondInput, keyValPair_t *destCondInput ) {
    int i;

    memset( destCondInput, 0, sizeof( keyValPair_t ) );

    for ( i = 0; i < srcCondInput->len; i++ ) {
        addKeyVal( destCondInput, srcCondInput->keyWord[i],
                   srcCondInput->value[i] );
    }
    return 0;
}

int copyKeyVal(
    const keyValPair_t* _src,
    keyValPair_t*       _dst ) {
    // =-=-=-=-=-=-=-
    // blatantly copy the src kvp to the dst
    for ( int i = 0; i < _src->len; ++i ) {
        addKeyVal( _dst, _src->keyWord[i], _src->value[i] );

    }

    return 0;
}

int
replDataObjInp( dataObjInp_t *srcDataObjInp, dataObjInp_t *destDataObjInp ) {
    *destDataObjInp = *srcDataObjInp;

    destDataObjInp->condInput.len = 0;
    destDataObjInp->condInput.keyWord = NULL;
    destDataObjInp->condInput.value = NULL;

    replKeyVal( &srcDataObjInp->condInput, &destDataObjInp->condInput );
    replSpecColl( srcDataObjInp->specColl, &destDataObjInp->specColl );
    return 0;
}

int
replSpecColl( specColl_t *inSpecColl, specColl_t **outSpecColl ) {
    if ( inSpecColl == NULL || outSpecColl == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    *outSpecColl = ( specColl_t * )malloc( sizeof( specColl_t ) );
    *( *outSpecColl ) = *inSpecColl;

    return 0;
}

int
addKeyVal( keyValPair_t *condInput, const char *keyWord, const char *value ) {
    if ( condInput == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if ( condInput->keyWord == NULL || condInput->value == NULL ) {
        condInput->len = 0;
    }

    /* check if the keyword exists */
    for ( int i = 0; i < condInput->len; i++ ) {
        if ( condInput->keyWord[i] == NULL || strlen( condInput->keyWord[i] ) == 0 ) {
            free( condInput->keyWord[i] );
            free( condInput->value[i] );
            condInput->keyWord[i] = strdup( keyWord );
            condInput->value[i] = value ? strdup( value ) : NULL;
            return 0;
        }
        else if ( strcmp( keyWord, condInput->keyWord[i] ) == 0 ) {
            free( condInput->value[i] );
            condInput->value[i] = value ? strdup( value ) : NULL;
            return 0;
        }
    }

    if ( ( condInput->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        condInput->keyWord = ( char ** )realloc( condInput->keyWord,
                             ( condInput->len + PTR_ARRAY_MALLOC_LEN ) * sizeof( *condInput->keyWord ) );
        condInput->value = ( char ** )realloc( condInput->value,
                                               ( condInput->len + PTR_ARRAY_MALLOC_LEN ) * sizeof( *condInput->value ) );
        memset( condInput->keyWord + condInput->len, 0, PTR_ARRAY_MALLOC_LEN * sizeof( *condInput->keyWord ) );
        memset( condInput->value + condInput->len, 0, PTR_ARRAY_MALLOC_LEN * sizeof( *condInput->value ) );
    }

    condInput->keyWord[condInput->len] = strdup( keyWord );
    condInput->value[condInput->len] = value ? strdup( value ) : NULL;
    condInput->len++;

    return 0;
}


int
addTagStruct( tagStruct_t *condInput, char *preTag, char *postTag, char *keyWord ) {
    char **newKeyWord;
    char **newPreTag;
    char **newPostTag;
    int newLen;
    int i;

    if ( condInput == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }



    if ( ( condInput->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newLen = condInput->len + PTR_ARRAY_MALLOC_LEN;
        newKeyWord = ( char ** ) malloc( newLen * sizeof( *newKeyWord ) );
        newPreTag = ( char ** ) malloc( newLen * sizeof( *newPreTag ) );
        newPostTag = ( char ** ) malloc( newLen * sizeof( *newPostTag ) );
        memset( newKeyWord, 0, newLen * sizeof( *newKeyWord ) );
        memset( newPreTag, 0, newLen * sizeof( *newPreTag ) );
        memset( newPostTag, 0, newLen * sizeof( *newPostTag ) );
        for ( i = 0; i < condInput->len; i++ ) {
            newKeyWord[i] = condInput->keyWord[i];
            newPreTag[i] = condInput->preTag[i];
            newPostTag[i] = condInput->postTag[i];
        }
        if ( condInput->keyWord != NULL ) {
            free( condInput->keyWord );
        }
        if ( condInput->preTag != NULL ) {
            free( condInput->preTag );
        }
        if ( condInput->postTag != NULL ) {
            free( condInput->postTag );
        }
        condInput->keyWord = newKeyWord;
        condInput->preTag = newPreTag;
        condInput->postTag = newPostTag;
    }

    condInput->keyWord[condInput->len] = strdup( keyWord );
    condInput->preTag[condInput->len] = strdup( preTag );
    condInput->postTag[condInput->len] = strdup( postTag );
    condInput->len++;

    return 0;
}




int
addInxIval( inxIvalPair_t *inxIvalPair, int inx, int value ) {
    int *newInx;
    int *newValue;
    int newLen;
    int i;

    if ( inxIvalPair == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ( inxIvalPair->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newLen = inxIvalPair->len + PTR_ARRAY_MALLOC_LEN;
        newInx = ( int * ) malloc( newLen * sizeof( int ) );
        newValue = ( int * ) malloc( newLen * sizeof( int ) );
        memset( newInx, 0, newLen * sizeof( int ) );
        memset( newValue, 0, newLen * sizeof( int ) );
        for ( i = 0; i < inxIvalPair->len; i++ ) {
            newInx[i] = inxIvalPair->inx[i];
            newValue[i] = inxIvalPair->value[i];
        }
        if ( inxIvalPair->inx != NULL ) {
            free( inxIvalPair->inx );
        }
        if ( inxIvalPair->value != NULL ) {
            free( inxIvalPair->value );
        }
        inxIvalPair->inx = newInx;
        inxIvalPair->value = newValue;
    }

    inxIvalPair->inx[inxIvalPair->len] = inx;
    inxIvalPair->value[inxIvalPair->len] = value;
    inxIvalPair->len++;

    return 0;
}

int
addInxVal( inxValPair_t *inxValPair, int inx, const char *value ) {
    int *newInx;
    char **newValue;
    int newLen;
    int i;

    if ( inxValPair == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( ( inxValPair->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newLen = inxValPair->len + PTR_ARRAY_MALLOC_LEN;
        newInx = ( int * ) malloc( newLen * sizeof( *newInx ) );
        newValue = ( char ** ) malloc( newLen * sizeof( *newValue ) );
        memset( newInx, 0, newLen * sizeof( *newInx ) );
        memset( newValue, 0, newLen * sizeof( *newValue ) );
        for ( i = 0; i < inxValPair->len; i++ ) {
            newInx[i] = inxValPair->inx[i];
            newValue[i] = inxValPair->value[i];
        }
        if ( inxValPair->inx != NULL ) {
            free( inxValPair->inx );
        }
        if ( inxValPair->value != NULL ) {
            free( inxValPair->value );
        }
        inxValPair->inx = newInx;
        inxValPair->value = newValue;
    }

    inxValPair->inx[inxValPair->len] = inx;
    inxValPair->value[inxValPair->len] = strdup( value );
    inxValPair->len++;

    return 0;
}

int
addStrArray( strArray_t *strArray, char *value ) {
    char *newValue;
    int newLen;
    int i;
    int size;
    int myLen;

    if ( strArray == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strArray->size <= 0 ) {
        if ( strArray->len == 0 ) {
            /* default to NAME_LEN */

            strArray->size = NAME_LEN;
        }
        else {
            rodsLog( LOG_ERROR,
                     "addStrArray: invalid size %d, len %d",
                     strArray->size, strArray->len );
            return SYS_INTERNAL_NULL_INPUT_ERR;
        }
    }

    myLen = strlen( value );

    size = strArray->size;
    while ( size < myLen + 1 ) {
        size = size * 2;
    }


    /* XXXXXXX should be replaced by resizeStrArray after 2.3 release */
    if ( size != strArray->size ||
            ( strArray->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        int oldSize = strArray->size;
        /* have to redo it */
        strArray->size = size;
        newLen = strArray->len + PTR_ARRAY_MALLOC_LEN;
        newValue = ( char * ) malloc( newLen * size );
        memset( newValue, 0, newLen * size );
        for ( i = 0; i < strArray->len; i++ ) {
            rstrcpy( &newValue[i * size], &strArray->value[i * oldSize], size );
        }
        if ( strArray->value != NULL ) {
            free( strArray->value );
        }
        strArray->value = newValue;
    }

    rstrcpy( &strArray->value[strArray->len * size], value, size );
    strArray->len++;

    return 0;
}

int
resizeStrArray( strArray_t *strArray, int newSize ) {
    int i, newLen;
    char *newValue;

    if ( newSize > strArray->size ||
            ( strArray->len % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        int oldSize = strArray->size;
        /* have to redo it */
        if ( strArray->size > newSize ) {
            newSize = strArray->size;
        }
        else {
            strArray->size = newSize;
        }
        newLen = strArray->len + PTR_ARRAY_MALLOC_LEN;
        newValue = ( char * ) malloc( newLen * newSize );
        memset( newValue, 0, newLen * newSize );
        for ( i = 0; i < strArray->len; i++ ) {
            rstrcpy( &newValue[i * newSize], &strArray->value[i * oldSize],
                     newSize );
        }
        if ( strArray->value != NULL ) {
            free( strArray->value );
        }
        strArray->value = newValue;
    }
    return 0;
}

int
clearKeyVal( keyValPair_t *condInput ) {

    if ( condInput == NULL || condInput->len < 1 ) {
        return 0;
    }

    for ( int i = 0; i < condInput->len; i++ ) {
        if ( condInput->keyWord != NULL ) {
            free( condInput->keyWord[i] );
        }
        if ( condInput->value != NULL ) {
            free( condInput->value[i] );
        }
    }

    free( condInput->keyWord );
    free( condInput->value );
    memset( condInput, 0, sizeof( keyValPair_t ) );
    return 0;
}

int
clearInxIval( inxIvalPair_t *inxIvalPair ) {
    if ( inxIvalPair == NULL || inxIvalPair->len <= 0 ) {
        return 0;
    }

    free( inxIvalPair->inx );
    free( inxIvalPair->value );
    memset( inxIvalPair, 0, sizeof( inxIvalPair_t ) );

    return 0;
}

int
clearInxVal( inxValPair_t *inxValPair ) {
    int i;

    if ( inxValPair == NULL || inxValPair->len <= 0 ) {
        return 0;
    }

    for ( i = 0; i < inxValPair->len; i++ ) {
        free( inxValPair->value[i] );
    }

    free( inxValPair->inx );
    free( inxValPair->value );
    memset( inxValPair, 0, sizeof( inxValPair_t ) );

    return 0;
}

int
freeGenQueryInp( genQueryInp_t **genQueryInp ) {
    if ( genQueryInp == NULL ) {
        return 0;
    }

    if ( *genQueryInp == NULL ) {
        return 0;
    }

    clearGenQueryInp( *genQueryInp );
    free( *genQueryInp );
    *genQueryInp = NULL;

    return 0;
}

void
clearGenQueryInp( void* voidInp ) {

    if ( voidInp == NULL ) {
        return;
    }

    genQueryInp_t *genQueryInp = ( genQueryInp_t* ) voidInp;
    clearInxIval( &genQueryInp->selectInp );
    clearInxVal( &genQueryInp->sqlCondInp );
    clearKeyVal( &genQueryInp->condInput );

    return;
}

int
freeGenQueryOut( genQueryOut_t **genQueryOut ) {
    if ( genQueryOut == NULL ) {
        return 0;
    }

    if ( *genQueryOut == NULL ) {
        return 0;
    }

    clearGenQueryOut( *genQueryOut );
    free( *genQueryOut );
    *genQueryOut = NULL;

    return 0;
}

void
clearGenQueryOut( void* voidInp ) {
    genQueryOut_t *genQueryOut = ( genQueryOut_t* ) voidInp;
    int i;

    if ( genQueryOut == NULL ) {
        return;
    }

    for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
        if ( genQueryOut->sqlResult[i].value != NULL ) {
            free( genQueryOut->sqlResult[i].value );
        }
    }
    return;
}

/* catGenQueryOut - Concatenate genQueryOut to targGenQueryOut up to maxRowCnt.
 * It is assumed that the two genQueryOut have the same attriInx and
 * len for each attri.
 *
 */
int
catGenQueryOut( genQueryOut_t *targGenQueryOut, genQueryOut_t *genQueryOut,
                int maxRowCnt ) {
    int i;
    int totalRowCnt;

    /* do some sanity checks */


    if ( targGenQueryOut == NULL || genQueryOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( genQueryOut->rowCnt == 0 ) {
        return 0;
    }

    if ( ( totalRowCnt = targGenQueryOut->rowCnt + genQueryOut->rowCnt ) >
            maxRowCnt ) {
        rodsLog( LOG_ERROR,
                 "catGenQueryOut: total rowCnt %d > %d",
                 targGenQueryOut->rowCnt + genQueryOut->rowCnt, maxRowCnt );
        return SYS_STRUCT_ELEMENT_MISMATCH;
    }


    if ( targGenQueryOut->attriCnt != genQueryOut->attriCnt ) {
        rodsLog( LOG_ERROR,
                 "catGenQueryOut: attriCnt mismatch %d != %d",
                 targGenQueryOut->attriCnt, genQueryOut->attriCnt );
        return SYS_STRUCT_ELEMENT_MISMATCH;
    }

    for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
        if ( targGenQueryOut->sqlResult[i].attriInx !=
                genQueryOut->sqlResult[i].attriInx ) {
            rodsLog( LOG_ERROR,
                     "catGenQueryOut: attriInx mismatch %d != %d",
                     targGenQueryOut->sqlResult[i].attriInx,
                     genQueryOut->sqlResult[i].attriInx );
            return SYS_STRUCT_ELEMENT_MISMATCH;
        }
        if ( targGenQueryOut->sqlResult[i].len !=
                genQueryOut->sqlResult[i].len ) {
            rodsLog( LOG_ERROR,
                     "catGenQueryOut: len mismatch %d != %d",
                     targGenQueryOut->sqlResult[i].len, genQueryOut->sqlResult[i].len );
            return SYS_STRUCT_ELEMENT_MISMATCH;
        }
    }

    for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
        char *tmpValue;
        int len;

        if ( ( len = genQueryOut->sqlResult[i].len ) <= 0 ) {
            continue;
        }
        if ( ( tmpValue = ( char * )malloc( totalRowCnt * len ) ) == 0 ) {
            return SYS_MALLOC_ERR - errno;
        }
        if ( targGenQueryOut->sqlResult[i].value != NULL ) {
            memcpy( tmpValue, targGenQueryOut->sqlResult[i].value,
                    len * targGenQueryOut->rowCnt );
            free( targGenQueryOut->sqlResult[i].value );
        }
        targGenQueryOut->sqlResult[i].value = tmpValue;
        tmpValue += len * targGenQueryOut->rowCnt;
        memcpy( tmpValue, genQueryOut->sqlResult[i].value,
                len * genQueryOut->rowCnt );
    }
    targGenQueryOut->rowCnt = totalRowCnt;

    return 0;
}

void
clearBulkOprInp( void* voidInp ) {
    bulkOprInp_t *bulkOprInp = ( bulkOprInp_t* ) voidInp;
    if ( bulkOprInp == NULL ) {
        return;
    }
    clearGenQueryOut( &bulkOprInp->attriArray );
    clearKeyVal( &bulkOprInp->condInput );
    return;
}

int
moveKeyVal( keyValPair_t *destKeyVal, keyValPair_t *srcKeyVal ) {
    if ( destKeyVal == NULL || srcKeyVal == NULL ) {
        return 0;
    }

    *destKeyVal = *srcKeyVal;
    memset( srcKeyVal, 0, sizeof( keyValPair_t ) );
    return 0;
}

int
getUnixUid( char *userName ) {
#ifndef _WIN32
    struct passwd *pw = 0;
    int myuid = 0;
    char* splitPos = 0;

    if ( ( splitPos = strchr( userName, '@' ) ) != NULL ) {
        *splitPos = '\0';       /* skip @ */
    }

    if ( !( pw = getpwnam( userName ) ) ) {
        myuid = -1;
    }
    else {
        myuid = ( int ) pw->pw_uid;
    }
    if ( splitPos != NULL ) {
        *splitPos = '@';
    }
    return myuid;
#else
    return -1;
#endif
}

int
getUnixUsername( int uid, char *username, int username_len ) {
#ifndef _WIN32
    struct passwd *pwent;

    if ( uid < 0 || username == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    /* if getpwuid returns NULL and errno is zero, it
       means the user doesn't exist in the user db */
    errno = 0;
    pwent = getpwuid( uid );
    if ( pwent == NULL ) {
        if ( errno ) {
            rodsLog( LOG_ERROR,
                     "getUnixUsername: error calling getpwuid for uid %d. errno = %d",
                     uid, errno );
        }
        else {
            rodsLog( LOG_ERROR, "getUnixUsername: no user with uid %d", uid );
        }
        return SYS_USER_RETRIEVE_ERR - errno;
    }
    if ( ( unsigned int )username_len <= strlen( pwent->pw_name ) ) {
        rodsLog( LOG_ERROR, "getUnixUsername: username input buffer too small (%d <= %d)",
                 username_len, strlen( pwent->pw_name ) );
        return USER_STRLEN_TOOLONG;
    }
    strcpy( username, pwent->pw_name );

    return 0;
#else
    return -1;
#endif
}

int
getUnixGroupname( int gid, char *groupname, int groupname_len ) {
#ifndef _WIN32
    struct group *grent;

    if ( gid < 0 || groupname == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    errno = 0;
    grent = getgrgid( gid );
    if ( grent == NULL ) {
        if ( errno ) {
            rodsLog( LOG_ERROR,
                     "getUnixGroupname: error calling getgrgid for gid %d. errno = %d",
                     gid, errno );
        }
        else {
            rodsLog( LOG_ERROR, "getUnixGroupname: no group with gid %d", gid );
        }
        return SYS_GROUP_RETRIEVE_ERR - errno;
    }
    if ( ( unsigned int )groupname_len <= strlen( grent->gr_name ) ) {
        rodsLog( LOG_ERROR, "getUnixGroupname: groupname input buffer too small (%d <= %d)",
                 groupname_len, strlen( grent->gr_name ) );
        return USER_STRLEN_TOOLONG;
    }
    strcpy( groupname, grent->gr_name );

    return 0;
#else
    return -1;
#endif
}

/*
   Return 64 semi-random bytes terminated by a null.

   This is designed to be very quick and sufficiently pseudo-random for
   the context in which it is used (a challenge in the challenge -
   response protocol).

   If /dev/urandom is available (as is usually the case on Linux/Unix
   hosts) we now use that, as it will be normally be sufficiently fast
   and has much entropy (very pseudo-random).

   Otherwise, this algorithm creates sufficient pseudo-randomness as
   described below.  There are about 20 bits of entropy from the
   microsecond clock, plus a few more via the pid, and more from the
   seconds value.

   By using the PID and a static counter as part of this, we're
   guaranteed that one or the other of those will vary each time, even
   if called repeatedly (the microsecond time value will vary too).

   The use of the epoch seconds value (a large number that increments
   each second), also helps prevent returning the same set of
   pseudo-random bytes over time.

   MD5 is a quick and handy way to fill out the 64 bytes using the input
   values, in a manner that is very unlikely to repeat.

   The entropy of the seeds, in combination with these other features,
   makes the probability of repeating a particular pattern on the order
   of one out of many billions.  If /dev/urandom is available, the odds
   are even smaller.

 */
int get64RandomBytes( char *buf ) {
    MD5_CTX context;
    char buffer[65]; /* each digest is 16 bytes, 4 of them */
    int ints[30];
    int pid;
#ifdef windows_platform
    SYSTEMTIME tv;
#else
    struct timeval tv;
#endif
    static int count = 12348;
    int i;

#ifndef windows_platform
    /*
       Use /dev/urandom instead, if available
     */
    int uran_fd, rval;
    uran_fd = open( "/dev/urandom", O_RDONLY );
    if ( uran_fd > 0 ) {
        rval = read( uran_fd, buffer, 64 );
        close( uran_fd );
        if ( rval == 64 ) {
            for ( i = 0; i < 64; i++ ) {
                if ( buffer[i] == '\0' ) {
                    buffer[i]++;  /* make sure no nulls before end of 'string'*/
                }
            }
            buffer[64] = '\0';
            strncpy( buf, buffer, 65 );
            return 0;
        }
    }
#endif

#ifdef windows_platform
    GetSystemTime( &tv );
#else
    gettimeofday( &tv, 0 );
#endif
    pid = getpid();
    count++;

    ints[0] = 12349994;
    ints[1] = count;
#ifdef windows_platform
    ints[2] = ( int )tv.wSecond;
    ints[5] = ( int )tv.wSecond;
#else
    ints[2] = tv.tv_usec;
    ints[5] = tv.tv_sec;
#endif
    MD5Init( &context );
    MD5Update( &context, ( unsigned char* )&ints[0], 100 );
    MD5Final( ( unsigned char* )buffer, &context );

    ints[0] = pid;
    ints[4] = ( int )buffer[10];
    MD5Init( &context );
    MD5Update( &context, ( unsigned char * )&ints[0], 100 );
    MD5Final( ( unsigned char* )( buffer + 16 ), &context );

    MD5Init( &context );
    MD5Update( &context, ( unsigned char* )&ints[0], 100 );
    MD5Final( ( unsigned char* )( buffer + 32 ), &context );

    MD5Init( &context );
    MD5Update( &context, ( unsigned char* )buffer, 40 );
    MD5Final( ( unsigned char* )( buffer + 48 ), &context );

    for ( i = 0; i < 64; i++ ) {
        if ( buffer[i] == '\0' ) {
            buffer[i]++;  /* make sure no nulls before end of 'string'*/
        }
    }
    buffer[64] = '\0';
    strncpy( buf, buffer, 65 );
    return 0;
}

sqlResult_t *
getSqlResultByInx( genQueryOut_t *genQueryOut, int attriInx ) {
    int i;

    if ( genQueryOut == NULL ) {
        return NULL;
    }

    for ( i = 0; i < genQueryOut->attriCnt; i++ ) {
        if ( genQueryOut->sqlResult[i].attriInx == attriInx ) {
            return &genQueryOut->sqlResult[i];
        }
    }
    return NULL;
}

void
clearModDataObjMetaInp( void* voidInp ) {
    modDataObjMeta_t *modDataObjMetaInp = ( modDataObjMeta_t* ) voidInp;
    if ( modDataObjMetaInp == NULL ) {
        return;
    }

    if ( modDataObjMetaInp->regParam != NULL ) {
        clearKeyVal( modDataObjMetaInp->regParam );
        free( modDataObjMetaInp->regParam );
    }

    if ( modDataObjMetaInp->dataObjInfo != NULL ) {
        freeDataObjInfo( modDataObjMetaInp->dataObjInfo );
    }

    return;
}

void
clearUnregDataObj( void* voidInp ) {
    unregDataObj_t *unregDataObjInp = ( unregDataObj_t* ) voidInp;
    if ( unregDataObjInp == NULL ) {
        return;
    }

    if ( unregDataObjInp->condInput != NULL ) {
        clearKeyVal( unregDataObjInp->condInput );
        free( unregDataObjInp->condInput );
    }

    if ( unregDataObjInp->dataObjInfo != NULL ) {
        freeDataObjInfo( unregDataObjInp->dataObjInfo );
    }

    return ;
}

void
clearRegReplicaInp( void* voidInp ) {
    regReplica_t *regReplicaInp = ( regReplica_t* ) voidInp;
    if ( regReplicaInp == NULL ) {
        return;
    }

    clearKeyVal( &regReplicaInp->condInput );

    if ( regReplicaInp->srcDataObjInfo != NULL ) {
        freeDataObjInfo( regReplicaInp->srcDataObjInfo );
    }

    if ( regReplicaInp->destDataObjInfo != NULL ) {
        freeDataObjInfo( regReplicaInp->destDataObjInfo );
    }

    memset( regReplicaInp, 0, sizeof( regReplica_t ) );

    return;
}

void
clearFileOpenInp( void* voidInp ) {
    fileOpenInp_t *fileOpenInp = ( fileOpenInp_t* ) voidInp;
    if ( fileOpenInp == NULL ) {
        return;
    }
    clearKeyVal( &fileOpenInp->condInput );
    memset( fileOpenInp, 0, sizeof( fileOpenInp_t ) );

    return;
}

void
clearDataObjInp( void* voidInp ) {
    dataObjInp_t *dataObjInp = ( dataObjInp_t* ) voidInp;
    if ( dataObjInp == NULL ) {
        return;
    }

    clearKeyVal( &dataObjInp->condInput );
    if ( dataObjInp->specColl != NULL ) {
        free( dataObjInp->specColl );
    }

    memset( dataObjInp, 0, sizeof( dataObjInp_t ) );

    return;
}

void
clearCollInp( void* voidInp ) {

    collInp_t *collInp = ( collInp_t* ) voidInp;
    if ( collInp == NULL ) {
        return;
    }

    clearKeyVal( &collInp->condInput );

    memset( collInp, 0, sizeof( collInp_t ) );

    return;
}

void
clearDataObjCopyInp( void* voidInp ) {
    dataObjCopyInp_t *dataObjCopyInp = ( dataObjCopyInp_t* ) voidInp;
    if ( dataObjCopyInp == NULL ) {
        return;
    }

    clearKeyVal( &dataObjCopyInp->destDataObjInp.condInput );
    clearKeyVal( &dataObjCopyInp->srcDataObjInp.condInput );

    if ( dataObjCopyInp->srcDataObjInp.specColl != NULL ) {
        free( dataObjCopyInp->srcDataObjInp.specColl );
    }

    memset( dataObjCopyInp, 0, sizeof( dataObjCopyInp_t ) );

    return;
}

int
freeAllRescQuota( rescQuota_t *rescQuotaHead ) {
    rescQuota_t *tmpRescQuota, *nextRescQuota;

    tmpRescQuota = rescQuotaHead;
    while ( tmpRescQuota != NULL ) {
        nextRescQuota = tmpRescQuota->next;
        free( tmpRescQuota );
        tmpRescQuota = nextRescQuota;
    }
    return 0;
}

int
parseMultiStr( char *strInput, strArray_t *strArray ) {
    char *startPtr, *endPtr;
    int endReached = 0;

    if ( strInput == NULL || strArray == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    startPtr = endPtr = strInput;

    while ( 1 ) {
        // two %% will be taken as an input instead of as a delimiter
        while ( *endPtr != '%' && *endPtr != '\0' ) {
            endPtr ++;
        }
        if ( *endPtr == '%' ) {
            if ( *( endPtr + 1 ) == '%' ) {
                endPtr ++;
                endPtr ++;
                continue;
            }
            *endPtr = '\0';
        }
        else {
            endReached = 1;
        }

        addStrArray( strArray, startPtr );

        if ( endReached == 1 ) {
            break;
        }

        endPtr++;
        startPtr = endPtr;
    }

    return strArray->len;
}

/* Get the current time, in the  form: 2006-10-25-10.52.43 */
/*                                     0123456789012345678 */
extern char *tzname[2];

/*
   Return an integer string of the current time in the Unix Time
   format (integer seconds since 1970).  This is the same
   in all timezones (sort of CUT) and is converted to local time
   for display.
 */
void
getNowStr( char *timeStr ) {
    time_t myTime;

    myTime = time( NULL );
    snprintf( timeStr, 15, "%011d", ( uint ) myTime );
}

/*
   Convert a Unix time value (as from getNowStr) to a local
   time format.
 */
int
getLocalTimeFromRodsTime( const char *timeStrIn, char *timeStr ) {
    time_t myTime;
    struct tm *mytm;

    // This is 1 because they actually capture a leading space
    if ( strlen( timeStrIn ) == 1 ) {
        strcpy( timeStr, "Never" );
    }
    else {
        if ( sizeof( time_t ) == 4 ) {
            myTime = atol( timeStrIn );
        }
        else {
#ifdef _WIN32
            myTime = _atoi64( timeStrIn );
#else
            myTime = atoll( timeStrIn );
#endif
        }

        mytm = localtime( &myTime );

        getLocalTimeStr( mytm, timeStr );
    }
    return 0;
}

int
getLocalTimeStr( struct tm *mytm, char *timeStr ) {
    snprintf( timeStr, TIME_LEN, "%4d-%2d-%2d.%2d:%2d:%2d",
              mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday,
              mytm->tm_hour, mytm->tm_min, mytm->tm_sec );

    if ( timeStr[5] == ' ' ) {
        timeStr[5] = '0';
    }
    if ( timeStr[8] == ' ' ) {
        timeStr[8] = '0';
    }
    if ( timeStr[11] == ' ' ) {
        timeStr[11] = '0';
    }
    if ( timeStr[14] == ' ' ) {
        timeStr[14] = '0';
    }
    if ( timeStr[17] == ' ' ) {
        timeStr[17] = '0';
    }

    return 0;
}


/* Get the current time + offset , in the  form: 2006-10-25-10.52.43 */
/*  offset is a string of the same form  */
/*                                               0123456789012345678 */
void
getOffsetTimeStr( char *timeStr, char *offSet ) {
    time_t myTime;

    myTime = time( NULL );
    myTime += atoi( offSet );

    snprintf( timeStr, TIME_LEN, "%d", ( uint ) myTime );
}

/* Update the input time string to be offset minutes ahead of the
   input value.  timeStr is input and ouput, in the form:
   2006-10-25-10.52.43
   0123456789012345678
   Offset the number of minutes to add.
   This is based on getOffsetTimeStr.
 */
void
updateOffsetTimeStr( char *timeStr, int offset ) {
    time_t myTime;
    struct tm *mytm;
    time_t newTime;
    char s[50];

    myTime = time( NULL );
    mytm = localtime( &myTime );

    rstrcpy( s, timeStr, 49 );

    s[19] = '\0';
    mytm->tm_sec = atoi( &s[17] );
    s[16] = '\0';
    mytm->tm_min = atoi( &s[14] );
    s[13] = '\0';
    mytm->tm_hour = atoi( &s[11] );
    s[10] = '\0';
    mytm->tm_mday = atoi( &s[8] );
    s[7] = '\0';
    mytm->tm_mon = atoi( &s[5] ) - 1;
    s[4] = '\0';
    mytm->tm_year = atoi( &s[0] ) - 1900;

    mytm->tm_min += offset; /* offset by the input minutes */

    newTime = mktime( mytm );
    mytm = localtime( &newTime );

    snprintf( timeStr, TIME_LEN, "%4d-%2d-%2d-%2d.%2d.%2d",
              mytm->tm_year + 1900, mytm->tm_mon + 1, mytm->tm_mday,
              mytm->tm_hour, mytm->tm_min, mytm->tm_sec );

    if ( timeStr[5] == ' ' ) {
        timeStr[5] = '0';
    }
    if ( timeStr[8] == ' ' ) {
        timeStr[8] = '0';
    }
    if ( timeStr[11] == ' ' ) {
        timeStr[11] = '0';
    }
    if ( timeStr[14] == ' ' ) {
        timeStr[14] = '0';
    }
    if ( timeStr[17] == ' ' ) {
        timeStr[17] = '0';
    }
}

/* getNextRepeatTime - analyze the string given in delayStr to output the time
 * in sec of unix time in string nextTime for the next repetition based on
 * the current time given in string currTime.
 * Strings currTime and nextTime will be in sec of unix time.
 * The delayStr is of the format:
 *     nnnnU <directive>  where
 *     nnnn is a number, and
 *     U is the unit of the number (s-sec,m-min,h-hour,d-day,y-year),
 * The <directive> can be for the form:
 *     <empty-directive>    - equal to REPEAT FOR EVER
 *     REPEAT FOR EVER
 *     REPEAT UNTIL SUCCESS
 *     REPEAT nnnn TIMES    - where nnnn is an integer
 *     REPEAT UNTIL <time>  - where <time> is of the time format
 *                            supported by checkDateFormat function below.
 *     REPEAT UNTIL SUCCESS OR UNTIL <time>
 *     REPEAT UNTIL SUCCESS OR nnnn TIMES
 *
 *     DOUBLE FOR EVER
 *     DOUBLE UNTIL SUCCESS   - delay is doubled every time.
 *     DOUBLE nnnn TIMES
 *     DOUBLE UNTIL <time>
 *     DOUBLE UNTIL SUCCESS OR UNTIL <time>
 *     DOUBLE UNTIL SUCCESS OR nnnn TIMES
 *     DOUBLE UNTIL SUCCESS UPTO <time>
 *
 * Return Values contain some semantics for followup actions:
 *    0 = continue with given delay
 *        that is, update the delayExec entry with nextExec time.
 *    1 = if the first operation was successful no more repeats.
 *        that is, remove the delayExec entry if execOp was successful.
 *             otherwise update the delayExec entry with nextExec time.
 *    2 = no more repeats whether the operation was successful or not.
 *        that is, remove the delayExec entry.
 *    3 = change the delayExec entry with new delayStr.
 *        that is, change both delayStr as well as nextExec time.
 *    4 = if the first operation was successful no more repeats.
 *        that is, remove the delayExec entry if execOp was successful.
 *             otherwise change both delayStr as well as nextExec time.
 *
 */
int
getNextRepeatTime( char *currTime, char *delayStr, char *nextTime ) {

    rodsLong_t  it, dt;
    char *t, *s;
    char u;
    char tstr[200];
    int n;

    t = delayStr;
    while ( isdigit( *t ) ) {
        t++;
    }
    u = *t;
    *t = '\0';
    dt = atol( delayStr );
    it = dt;
    *t = u;
    t++;
    switch ( u ) {
    case 's':
        break;
    case 'm':
        dt = dt * 60;
        break;
    case 'h':
        dt = dt * 3600;
        break;
    case 'd':
        dt = dt * 3600 * 24;
        break;
    case 'y':
        dt = dt * 3600 * 24 * 365;
        break;
    default:
        break;
    }

    while ( isspace( *t ) ) {
        t++;
    }
    if ( strlen( t ) == 0 || !strcmp( t, "REPEAT FOR EVER" ) ) {
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        return 0;
    }
    if ( !strcmp( t, "DOUBLE FOR EVER" ) ) {
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        sprintf( delayStr, "%lld%c DOUBLE FOR EVER", it * 2, u );
        return 3;
    }
    if ( ( s = strstr( t, "REPEAT UNTIL SUCCESS OR UNTIL " ) ) != NULL ) {
        s = s + strlen( "REPEAT UNTIL SUCCESS OR UNTIL " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        convertDateFormat( tstr, currTime );
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        if ( atol( tstr ) < dt ) {
            return 2;
        }
        else {
            return 1;
        }
    }
    if ( ( s = strstr( t, "DOUBLE UNTIL SUCCESS OR UNTIL " ) ) != NULL ) {
        s = s + strlen( "DOUBLE UNTIL SUCCESS OR UNTIL " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        convertDateFormat( tstr, currTime );
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS OR UNTIL %s", it * 2, u, s );
        if ( atol( tstr ) < dt ) {
            return 2;
        }
        else {
            return 4;
        }
    }
    if ( ( s = strstr( t, "REPEAT UNTIL SUCCESS OR " ) ) != NULL ) {
        s = s + strlen( "REPEAT UNTIL SUCCESS OR " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        s = tstr;
        while ( isdigit( *s ) ) {
            s++;
        }
        *s = '\0';
        n = atoi( tstr );
        n--;
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        if ( strstr( s + 1, "ORIGINAL TIMES" ) != NULL ) {
            sprintf( delayStr, "%lld%c REPEAT UNTIL SUCCESS OR %i %s", it, u, n, s + 1 );
        }
        else {
            sprintf( delayStr, "%lld%c REPEAT UNTIL SUCCESS OR %i TIMES. ORIGINAL TIMES=%i", it, u, n, n + 1 );
        }
        if ( n <= 0 ) {
            return 2;
        }
        else {
            return 4;
        }
    }
    if ( ( s = strstr( t, "DOUBLE UNTIL SUCCESS OR " ) ) != NULL ) {
        s = s + strlen( "DOUBLE UNTIL SUCCESS OR " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        s = tstr;
        while ( isdigit( *s ) ) {
            s++;
        }
        *s = '\0';
        n = atoi( tstr );
        n--;
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        if ( strstr( s + 1, "ORIGINAL TIMES" ) != NULL ) {
            sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS OR %i %s", it * 2, u, n, s + 1 );
        }
        else {
            sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS OR %i TIMES. ORIGINAL TIMES=%i", it * 2, u, n, n + 1 );
        }
        if ( n <= 0 ) {
            return 2;
        }
        else {
            return 4;
        }
    }
    if ( ( s = strstr( t, "DOUBLE UNTIL SUCCESS UPTO " ) ) != NULL ) {
        s = s + strlen( "DOUBLE UNTIL SUCCESS UPTO " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        convertDateFormat( tstr, currTime );
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS UPTO %s", it * 2, u, s );
        if ( atol( tstr ) > dt ) {
            sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS UPTO %s", it * 2 , u, s );
        }
        else {
            sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS UPTO %s", it , u, s );
        }
        return 4;
    }
    if ( strstr( t, "REPEAT UNTIL SUCCESS" ) != NULL ) {
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        return 1;
    }
    if ( strstr( t, "DOUBLE UNTIL SUCCESS" ) != NULL ) {
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        sprintf( delayStr, "%lld%c DOUBLE UNTIL SUCCESS", it * 2, u );
        return 4;
    }
    if ( ( s = strstr( t, "REPEAT UNTIL " ) ) != NULL ) {
        s = s + strlen( "REPEAT UNTIL " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        convertDateFormat( tstr, currTime );
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        if ( atol( tstr ) < dt ) {
            return 2;
        }
        else {
            return 0;
        }
    }

    if ( ( s = strstr( t, "DOUBLE UNTIL " ) ) != NULL ) {
        s = s + strlen( "DOUBLE UNTIL " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        convertDateFormat( tstr, currTime );
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        /* sprintf(delayStr,"%lld%c DOUBLE UNTIL %s", it * 2, u,s); */
        sprintf( delayStr, "%lld%c DOUBLE UNTIL %s", it * 2, u, tstr );
        if ( atol( tstr ) < dt ) {
            return 2;
        }
        else {
            return 3;
        }

    }
    if ( ( s = strstr( t, "REPEAT " ) ) != NULL ) {
        s = s + strlen( "REPEAT " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        s = tstr;
        while ( isdigit( *s ) ) {
            s++;
        }
        *s = '\0';
        n = atoi( tstr );
        n--;

        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        if ( strstr( s + 1, "ORIGINAL TIMES" ) != NULL ) {
            sprintf( delayStr, "%lld%c REPEAT %i %s", it, u, n, s + 1 );
        }
        else {
            sprintf( delayStr, "%lld%c REPEAT %i TIMES. ORIGINAL TIMES=%i", it, u, n, n + 1 );
        }
        if ( n <= 0 ) {
            return 2;
        }
        else {
            return 3;
        }
    }
    if ( ( s = strstr( t, "DOUBLE " ) ) != NULL ) {
        s = s + strlen( "DOUBLE " );
        while ( isspace( *s ) ) {
            s++;
        }
        snprintf( tstr, sizeof( tstr ), "%s", s );
        s = tstr;
        while ( isdigit( *s ) ) {
            s++;
        }
        *s = '\0';
        n = atoi( tstr );
        n--;
        dt = dt   + atol( currTime );
        sprintf( nextTime, "%lld", dt );
        if ( strstr( s + 1, "ORIGINAL TIMES" ) != NULL ) {
            sprintf( delayStr, "%lld%c DOUBLE %i %s", it * 2, u, n, s + 1 );
        }
        else {
            sprintf( delayStr, "%lld%c DOUBLE %i TIMES. ORIGINAL TIMES=%i", it * 2, u, n, n + 1 );
        }
        if ( n <= 0 ) {
            return 2;
        }
        else {
            return 3;
        }
    }

    return 0;
}

int
localToUnixTime( char * localTime, char * unixTime ) {
    time_t myTime;
    struct tm *mytm;
    time_t newTime;
    char s[TIME_LEN];

    myTime = time( NULL );
    mytm = localtime( &myTime );

    rstrcpy( s, localTime, TIME_LEN );

    s[19] = '\0';
    mytm->tm_sec = atoi( &s[17] );
    s[16] = '\0';
    mytm->tm_min = atoi( &s[14] );
    s[13] = '\0';
    mytm->tm_hour = atoi( &s[11] );
    s[10] = '\0';
    mytm->tm_mday = atoi( &s[8] );
    s[7] = '\0';
    mytm->tm_mon = atoi( &s[5] ) - 1;
    s[4] = '\0';
    mytm->tm_year = atoi( &s[0] ) - 1900;

    newTime = mktime( mytm );
    if ( sizeof( newTime ) == 64 ) {
        snprintf( unixTime, TIME_LEN, "%lld", ( rodsLong_t ) newTime );
    }
    else {
        snprintf( unixTime, TIME_LEN, "%d", ( uint ) newTime );
    }
    return 0;
}

int
isInteger( char * inStr ) {
    int i;
    int len;

    len = strlen( inStr );
    /* see if it is all digit */
    for ( i = 0; i < len; i++ ) {
        if ( !isdigit( inStr[i] ) ) {
            return 0;
        }
    }
    return 1;
}


/* convertDateFormat  - uses the checkDateFormat to convert string 's'
 * into sec of unix time. But if the time is in the YYYY-*** format
 * adds the current date (in unix seconds format) to forma  full date
 */

int
convertDateFormat( char * s, char * currTime ) {
    rodsLong_t  it;
    char tstr[200];
    int i;
    rstrcpy( tstr, s, 199 );
    i = checkDateFormat( tstr );
    if ( i != 0 ) {
        return i;
    }
    if ( !isInteger( s ) && strchr( s, '-' ) == NULL && strchr( s, ':' ) == NULL ) {
        it = atol( tstr ) + atol( currTime );
        sprintf( s, "%lld", it );
    }
    else {
        strcpy( s, tstr );
    }
    return 0;
}

/* checkDateFormat - convert the string given in s and output the time
 * in sec of unix time in the same string s
 * The input can be incremental time given in :
 *     nnnn - an integer. assumed to be in sec
 *     nnnns - an integer followed by 's' ==> in sec
 *     nnnnm - an integer followed by 'm' ==> in min
 *     nnnnh - an integer followed by 'h' ==> in hours
 *     nnnnd - an integer followed by 'd' ==> in days
 *     nnnny - an integer followed by 'y' ==> in years
 *     dd.hh:mm:ss - where dd, hh, mm and ss are 2 digits integers representing
 *       days, hours minutes and seconds, repectively. Truncation from the
 *       end is allowed. e.g. 20:40 means mm:ss
 * The input can also be full calendar time in the form:
 *    YYYY-MM-DD.hh:mm:ss  - Truncation from the beginning is allowed.
 *       e.g., 2007-07-29.12 means noon of July 29, 2007.
 *
 */

int
checkDateFormat( char * s ) {
    /* Note. The input *s is assumed to be TIME_LEN long */
    int len;
    char t[] = "0000-00-00.00:00:00";
    char outUnixTime[TIME_LEN];
    int status;
    int offset = 0;

    if ( isInteger( s ) ) {
        return 0;
    }

    len = strlen( s );

    if ( s[len - 1] == 's' ) {
        /* in sec */
        s[len - 1] = '\0';
        offset = atoi( s );
        snprintf( s, 19, "%d", offset );
        return 0;
    }
    else if ( s[len - 1] == 'm' ) {
        /* in min */
        s[len - 1] = '\0';
        offset = atoi( s ) * 60;
        snprintf( s, 19, "%d", offset );
        return 0;
    }
    else if ( s[len - 1] == 'h' ) {
        /* in hours */
        s[len - 1] = '\0';
        offset = atoi( s ) * 3600;
        snprintf( s, 19, "%d", offset );
        return 0;
    }
    else if ( s[len - 1] == 'd' ) {
        /* in days */
        s[len - 1] = '\0';
        offset = atoi( s ) * 3600 * 24;
        snprintf( s, 19, "%d", offset );
        return 0;
    }
    else if ( s[len - 1] == 'y' ) {
        /* in days */
        s[len - 1] = '\0';
        offset = atoi( s ) * 3600 * 24 * 365;
        snprintf( s, 19, "%d", offset );
        return 0;
    }
    else if ( len < 19 ) {
        /* not a full date. */
        if ( isdigit( s[0] ) && isdigit( s[1] ) && isdigit( s[2] ) && isdigit( s[3] ) ) {
            /* start with year, fill in the rest */
            strcat( s, ( char * )&t[len] );
        }
        else {
            /* must be offset */
            int mypos;

            /* sec */
            mypos = len - 1;
            while ( mypos >= 0 ) {
                if ( isdigit( s[mypos] ) ) {
                    offset += s[mypos] - 48;
                }
                else {
                    return DATE_FORMAT_ERR;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 10 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( s[mypos] != ':' ) {
                        return DATE_FORMAT_ERR;
                    }

                /* min */
                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 60 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 10 * 60 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( s[mypos] != ':' ) {
                        return DATE_FORMAT_ERR;
                    }

                /* hour */
                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 3600 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 10 * 3600 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( s[mypos] != '.' ) {
                        return DATE_FORMAT_ERR;
                    }

                /* day */

                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 24 * 3600 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }

                mypos--;
                if ( mypos >= 0 )
                    if ( isdigit( s[mypos] ) ) {
                        offset += 10 * 24 * 3600 * ( s[mypos] - 48 );
                    }
                    else {
                        return DATE_FORMAT_ERR;
                    }
                else {
                    break;
                }
            }
            snprintf( s, 19, "%d", offset );
            return 0;
        }
    }

    if ( isdigit( s[0] ) && isdigit( s[1] ) && isdigit( s[2] ) && isdigit( s[3] ) &&
            isdigit( s[5] ) && isdigit( s[6] ) && isdigit( s[8] ) && isdigit( s[9] ) &&
            isdigit( s[11] ) && isdigit( s[12] ) && isdigit( s[14] ) && isdigit( s[15] ) &&
            isdigit( s[17] ) && isdigit( s[18] ) &&
            s[4] == '-' && s[7] == '-' && s[10] == '.' &&
            s[13] == ':' && s[16] == ':' ) {
        status = localToUnixTime( s, outUnixTime );
        if ( status >= 0 ) {
            rstrcpy( s, outUnixTime, TIME_LEN );
        }
        return status;
    }
    else {
        return DATE_FORMAT_ERR;
    }
}

int
printErrorStack( rError_t * rError ) {
    int i, len;
    rErrMsg_t *errMsg;

    if ( rError == NULL ) {
        return 0;
    }

    len = rError->len;

    for ( i = 0; i < len; i++ ) {
        errMsg = rError->errMsg[i];
        if ( errMsg->status != STDOUT_STATUS ) {
            printf( "Level %d: ", i );
        }
        printf( "%s\n", errMsg->msg );
    }
    return 0;
}

int
appendRandomToPath( char * trashPath ) {
    int len;
    char *tmpPtr;

    len = strlen( trashPath );

    /* assume there is enough space for MAX_NAME_LEN char */

    if ( len >= MAX_NAME_LEN + 12 ) {
        return SYS_INVALID_FILE_PATH;
    }
    tmpPtr = trashPath + len;
    sprintf( tmpPtr, ".%d", ( uint ) random() );

    return 0;
}

// =-=-=-=-=-=-=-
// JMC - backport 4552
int
isBundlePath( char * myPath ) {
    char *tmpPtr, *tmpPtr1;

    tmpPtr = myPath;

    /* start with a '/' */
    if ( *tmpPtr != '/' ) {
        return False;
    }

    tmpPtr++;
    if ( ( tmpPtr1 = strchr( tmpPtr, '/' ) ) == NULL ) {
        return False;
    }

    tmpPtr = tmpPtr1 + 1;

    if ( strncmp( tmpPtr, "bundle/", 7 ) == 0 ) {
        return True;
    }
    else {
        return False;
    }
}
//  =-=-=-=-=-=-=-

int
isTrashPath( char * myPath ) {
    char *tmpPtr, *tmpPtr1;

    tmpPtr = myPath;

    /* start with a '/' */
    if ( *tmpPtr != '/' ) {
        return False;
    }

    tmpPtr++;
    if ( ( tmpPtr1 = strchr( tmpPtr, '/' ) ) == NULL ) {
        return False;
    }

    tmpPtr = tmpPtr1 + 1;

    if ( strncmp( tmpPtr, "trash/", 6 ) == 0 ) {
        return True;
    }
    else {
        return False;
    }
}

/* isTrashHome - see if the path is /myZone/trash/home or
 * /myZone/trash/home/myName.
 * return 0 if it is not
 * return 1 if it is.
 */

int
isTrashHome( char * myPath ) {
    char *tmpPtr, *tmpPtr1;

    tmpPtr = myPath;

    /* start with a '/' */
    if ( *tmpPtr != '/' ) {
        return 0;
    }

    tmpPtr++;
    if ( ( tmpPtr1 = strchr( tmpPtr, '/' ) ) == NULL ) {
        return 0;
    }

    tmpPtr = tmpPtr1 + 1;

    if ( strncmp( tmpPtr, "trash/home", 10 ) != 0 ) {
        return 0;
    }

    tmpPtr += 10;

    if ( *tmpPtr == '\0' ) {
        /* /myZone/trash/home */
        return 1;
    }
    else if ( *tmpPtr != '/' ) {
        return 0;
    }

    tmpPtr++;

    if ( strchr( tmpPtr, '/' ) == NULL ) {
        /* /myZone/trash/home/myName */
        return 1;
    }
    else {
        /* /myZone/trash/home/myName/... Not a trash home */
        return 0;
    }
}

/* isOrphanPath - check if path is a orphan path - /myZone/trash/orphan
 */
orphanPathType_t
isOrphanPath( char * myPath ) {
    char *tmpPtr, *tmpPtr1;

    tmpPtr = myPath;

    /* start with a '/' */
    if ( *tmpPtr != '/' ) {
        return NOT_ORPHAN_PATH;
    }

    tmpPtr++;
    if ( ( tmpPtr1 = strchr( tmpPtr, '/' ) ) == NULL ) {
        return NOT_ORPHAN_PATH;
    }

    tmpPtr = tmpPtr1 + 1;

    if ( strncmp( tmpPtr, "trash/orphan", 12 ) != 0 ) {
        return NOT_ORPHAN_PATH;
    }
    else if ( strcmp( tmpPtr, "trash/orphan" ) == 0 ) {
        return is_ORPHAN_HOME;
    }
    else {
        return IS_ORPHAN_PATH;
    }
}

/* isHomeColl - see if the path is /myZone/home or
 * /myZone/home/myName.
 * return 0 if it is not
 * return 1 if it is.
 */

int
isHomeColl( char * myPath ) {
    char *tmpPtr, *tmpPtr1;

    tmpPtr = myPath;

    /* start with a '/' */
    if ( *tmpPtr != '/' ) {
        return 0;
    }

    tmpPtr++;
    if ( ( tmpPtr1 = strchr( tmpPtr, '/' ) ) == NULL ) {
        return 0;
    }

    tmpPtr = tmpPtr1 + 1;

    if ( strncmp( tmpPtr, "home", 4 ) != 0 ) {
        return 0;
    }

    tmpPtr += 4;

    if ( *tmpPtr == '\0' ) {
        /* /myZone/home */
        return 1;
    }
    else if ( *tmpPtr != '/' ) {
        return 0;
    }

    tmpPtr++;

    if ( strchr( tmpPtr, '/' ) == NULL ) {
        /* /myZone/home/myName */
        return 1;
    }
    else {
        /* /myZone/home/myName/... Not a trash home */
        return 0;
    }
}

int
openRestartFile( char * restartFile, rodsRestart_t * rodsRestart ) {
    path p( restartFile );
    char buf[MAX_NAME_LEN * 3];
    char *inptr;
    char tmpStr[MAX_NAME_LEN];
    int status;

    if ( !exists( p ) || file_size( p ) == 0 ) {
#ifndef windows_platform
        rodsRestart->fd = open( restartFile, O_RDWR | O_CREAT, 0644 );
#else
        rodsRestart->fd = iRODSNt_bopen( restartFile, O_RDWR | O_CREAT, 0644 );
#endif
        if ( rodsRestart->fd < 0 ) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLogError( LOG_ERROR, status,
                          "openRestartFile: open error for %s", restartFile );
            return status;
        }
        rodsRestart->restartState = 0;
        printf( "New restartFile %s opened\n", restartFile );
    }
    else if ( !is_regular_file( p ) ) {
        close( rodsRestart->fd );
        rodsRestart->fd = -1;
        status = UNIX_FILE_OPEN_ERR;
        rodsLogError( LOG_ERROR, status,
                      "openRestartFile: %s is not a file", restartFile );
        return UNIX_FILE_OPEN_ERR;
    }
    else {
#ifndef windows_platform
        rodsRestart->fd = open( restartFile, O_RDWR, 0644 );
#else
        rodsRestart->fd = iRODSNt_bopen( restartFile, O_RDWR, 0644 );
#endif
        if ( rodsRestart->fd < 0 ) {
            status = UNIX_FILE_OPEN_ERR - errno;
            rodsLogError( LOG_ERROR, status,
                          "openRestartFile: open error for %s", restartFile );
            return status;
        }
        status = read( rodsRestart->fd, ( void * ) buf, MAX_NAME_LEN * 3 );
        if ( status <= 0 ) {
            close( rodsRestart->fd );
            status = UNIX_FILE_READ_ERR - errno;
            rodsLogError( LOG_ERROR, status,
                          "openRestartFile: read error for %s", restartFile );
            return status;
        }

        inptr = buf;
        if ( getLineInBuf( &inptr, rodsRestart->collection, MAX_NAME_LEN ) < 0 ) {
            rodsLog( LOG_ERROR,
                     "openRestartFile: restartFile %s is empty", restartFile );
            return USER_RESTART_FILE_INPUT_ERR;
        }
        if ( getLineInBuf( &inptr, tmpStr, MAX_NAME_LEN ) < 0 ) {
            rodsLog( LOG_ERROR,
                     "openRestartFile: restartFile %s has 1 only line", restartFile );
            return USER_RESTART_FILE_INPUT_ERR;
        }
        rodsRestart->doneCnt = atoi( tmpStr );

        if ( getLineInBuf( &inptr, rodsRestart->lastDonePath,
                           MAX_NAME_LEN ) < 0 ) {
            rodsLog( LOG_ERROR,
                     "openRestartFile: restartFile %s has only 2 lines", restartFile );
            return USER_RESTART_FILE_INPUT_ERR;
        }

        if ( getLineInBuf( &inptr, rodsRestart->oprType,
                           NAME_LEN ) < 0 ) {
            rodsLog( LOG_ERROR,
                     "openRestartFile: restartFile %s has only 3 lines", restartFile );
            return USER_RESTART_FILE_INPUT_ERR;
        }

        rodsRestart->restartState = PATH_MATCHING;
        printf( "RestartFile %s opened\n", restartFile );
        printf( "Restarting collection/directory = %s     File count %d\n",
                rodsRestart->collection, rodsRestart->doneCnt );
        printf( "File last completed = %s\n", rodsRestart->lastDonePath );
    }
    return 0;
}

int
getLineInBuf( char **inbuf, char * outbuf, int bufLen ) {
    char *inPtr, *outPtr;
    int bytesCopied  = 0;
    int c;

    inPtr = *inbuf;
    outPtr = outbuf;

    while ( ( c = *inPtr ) != '\n' && c != EOF && bytesCopied < bufLen ) {
        c = *inPtr;
        if ( c == '\n' || c == EOF ) {
            break;
        }
        *outPtr = c;
        inPtr++;
        outPtr++;
        bytesCopied++;
    }
    *outPtr = '\0';
    *inbuf = inPtr + 1;
    return bytesCopied;
}


int
setStateForResume( rcComm_t * conn, rodsRestart_t * rodsRestart,
                   char * restartPath, objType_t objType, keyValPair_t * condInput,
                   int deleteFlag ) {
    if ( restartPath != NULL && deleteFlag > 0 ) {
        if ( objType == DATA_OBJ_T ) {
            if ( ( condInput == NULL ||
                    getValByKey( condInput, FORCE_FLAG_KW ) == NULL ) &&
                    ( conn->fileRestart.info.status != FILE_RESTARTED ||
                      strcmp( conn->fileRestart.info.objPath, restartPath ) != 0 ) ) {
                dataObjInp_t dataObjInp;
                /* need to remove any partially completed file */
                /* XXXXX may not be enough for bulk put */
                memset( &dataObjInp, 0, sizeof( dataObjInp ) );
                addKeyVal( &dataObjInp.condInput, FORCE_FLAG_KW, "" );
                rstrcpy( dataObjInp.objPath, restartPath, MAX_NAME_LEN );
                int status = rcDataObjUnlink( conn, & dataObjInp );
                if ( status < 0 ) {
                    std::string notice = std::string( "rcDataObjUnlink returned with code: " );
                    notice.append( boost::lexical_cast<std::string>( status ) );
                    irods::log( LOG_NOTICE, notice );
                }
                clearKeyVal( &dataObjInp.condInput );
            }
        }
        else if ( objType == LOCAL_FILE_T ) {
            if ( conn->fileRestart.info.status != FILE_RESTARTED ||
                    strcmp( conn->fileRestart.info.fileName, restartPath ) != 0 ) {
                boost::filesystem::path path( restartPath );
                if ( boost::filesystem::exists( path ) ) {
                    int status = boost::filesystem::remove( path );
                    if ( status < 0 ) {
                        irods::log( ERROR( status, "boost:filesystem::remove() failed." ) );
                    }
                }
            }
        }
        else {
            rodsLog( LOG_ERROR,
                     "setStateForResume: illegal objType %d for %s",
                     objType, restartPath );
        }
    }
    rodsRestart->restartState = OPR_RESUMED;    /* resumed opr */

    return 0;
}


/* writeRestartFile - the restart file contain 4 lines:
 *   line 1 - collection.
 *   line 2 - doneCnt.
 *   line 3 - lastDonePath
 *   line 4 - oprType (BULK_OPR_KW or NON_BULK_OPR_KW);
 */

int
writeRestartFile( rodsRestart_t * rodsRestart, char * lastDonePath ) {
    char buf[MAX_NAME_LEN * 3];
    int status;

    rodsRestart->doneCnt = rodsRestart->curCnt;
    rstrcpy( rodsRestart->lastDonePath, lastDonePath, MAX_NAME_LEN );
    memset( buf, 0, MAX_NAME_LEN * 3 );
    snprintf( buf, MAX_NAME_LEN * 3, "%s\n%d\n%s\n%s\n",
              rodsRestart->collection, rodsRestart->doneCnt,
              rodsRestart->lastDonePath, rodsRestart->oprType );

    lseek( rodsRestart->fd, 0, SEEK_SET );
    status = write( rodsRestart->fd, buf, MAX_NAME_LEN * 3 );
    if ( status != MAX_NAME_LEN * 3 ) {
        rodsLog( LOG_ERROR,
                 "writeRestartFile: write error, errno = %d",
                 errno );
        return SYS_COPY_LEN_ERR - errno;
    }
    return 0;
}

int
procAndWrriteRestartFile( rodsRestart_t * rodsRestart, char * donePath ) {
    int status;

    if ( rodsRestart->fd <= 0 ) {
        return 0;
    }

    rodsRestart->curCnt ++;
    status = writeRestartFile( rodsRestart, donePath );

    return status;
}

int
setStateForRestart( rodsRestart_t * rodsRestart, rodsPath_t * targPath,
                    rodsArguments_t * rodsArgs ) {
    if ( rodsRestart->restartState & PATH_MATCHING ) {
        /* check the restart collection */
        if ( strstr( targPath->outPath, rodsRestart->collection ) != NULL ) {
            /* just use the rodsRestart->collection because the
             * targPath may be resolved into a different path */
            rstrcpy( targPath->outPath, rodsRestart->collection, MAX_NAME_LEN );
            rodsRestart->restartState |= MATCHED_RESTART_COLL;
            rodsRestart->curCnt = 0;
            if ( rodsArgs->verbose == True ) {
                printf( "**** Scanning to Restart Operation in %s ****\n",
                        targPath->outPath );
            }
        }
        else {
            /* take out MATCHED_RESTART_COLL */
            if ( rodsArgs->verbose == True ) {
                printf( "**** Skip Coll/dir %s ****\n",
                        targPath->outPath );
            }
            rodsRestart->restartState = rodsRestart->restartState &
                                        ( ~MATCHED_RESTART_COLL );
        }
    }
    else if ( rodsRestart->fd > 0 ) {
        /* just writing restart file */
        rstrcpy( rodsRestart->collection, targPath->outPath,
                 MAX_NAME_LEN );
        rodsRestart->doneCnt = rodsRestart->curCnt = 0;
    }
    return 0;
}

/* checkStateForResume - check the state for resume operation
 * return 0 - skip
 * return 1 - resume
 */

int
chkStateForResume( rcComm_t * conn, rodsRestart_t * rodsRestart,
                   char * targPath, rodsArguments_t * rodsArgs, objType_t objType,
                   keyValPair_t * condInput, int deleteFlag ) {
    int status;

    if ( rodsRestart->restartState & MATCHED_RESTART_COLL ) {
        if ( rodsRestart->curCnt > rodsRestart->doneCnt ) {
            rodsLog( LOG_ERROR,
                     "chkStateForResume:Restart failed.curCnt %d>doneCnt %d,path %s",
                     rodsRestart->curCnt, rodsRestart->doneCnt, targPath );
            return RESTART_OPR_FAILED;
        }

        if ( rodsRestart->restartState & LAST_PATH_MATCHED ) {
            if ( objType == DATA_OBJ_T || objType == LOCAL_FILE_T ) {
                /* a file */
                if ( rodsArgs->verbose == True ) {
                    printf( "***** RESUMING OPERATION ****\n" );
                }
                setStateForResume( conn, rodsRestart, targPath,
                                   objType, condInput, deleteFlag );
            }
            status = 1;
        }
        else if ( strcmp( targPath, rodsRestart->lastDonePath ) == 0 ) {
            /* will handle this with the next file */
            rodsRestart->curCnt ++;
            if ( rodsRestart->curCnt != rodsRestart->doneCnt ) {
                rodsLog( LOG_ERROR,
                         "chkStateForResume:Restart failed.curCnt %d!=doneCnt %d,path %s",
                         rodsRestart->curCnt, rodsRestart->doneCnt, targPath );
                return RESTART_OPR_FAILED;
            }
            rodsRestart->restartState |= LAST_PATH_MATCHED;
            status = 0;
        }
        else if ( objType == DATA_OBJ_T || objType == LOCAL_FILE_T ) {
            /* A file. no match - skip this */
            if ( rodsArgs->verbose == True ) {
                printf( "    ---- Skip file %s ----\n", targPath );
            }
            rodsRestart->curCnt ++;
            status = 0;
        }
        else {
            /* collection - drill down and see */
            status = 1;
        }
    }
    else if ( rodsRestart->restartState & PATH_MATCHING ) {
        /* the path does not match. skip */
        status = 0;
    }
    else {
        status = 1;
    }

    return status;
}

int
getAttrIdFromAttrName( char * cname ) {

    int i;
    for ( i = 0; i < NumOfColumnNames ; i++ ) {
        if ( !strcmp( columnNames[i].columnName, cname ) ) {
            return columnNames[i].columnId;
        }
    }
    return NO_COLUMN_NAME_FOUND;
}

int
showAttrNames() {
    int i;
    for ( i = 0; i < NumOfColumnNames ; i++ ) {
        printf( "%s\n", columnNames[i].columnName );
    }
    return 0;
}

int
separateSelFuncFromAttr( char * t, char **aggOp, char **colNm ) {
    char *s;

    if ( ( s = strchr( t, '(' ) ) == NULL ) {
        *colNm = t;
        *aggOp = NULL;
        return 0;
    }
    *aggOp = t;
    *s = '\0';
    s++;
    *colNm = s;
    if ( ( s = strchr( *colNm, ')' ) ) == NULL ) {
        return NO_COLUMN_NAME_FOUND;
    }
    *s = '\0';
    return 0;
}

int
getSelVal( char * c ) {
    if ( c == NULL ) {
        return 1;
    }
    if ( !strcmp( c, "sum" ) || !strcmp( c, "SUM" ) ) {
        return SELECT_SUM;
    }
    if ( !strcmp( c, "min" ) || !strcmp( c, "MIN" ) ) {
        return SELECT_MIN;
    }
    if ( !strcmp( c, "max" ) || !strcmp( c, "MAX" ) ) {
        return SELECT_MAX;
    }
    if ( !strcmp( c, "avg" ) || !strcmp( c, "AVG" ) ) {
        return SELECT_AVG;
    }
    if ( !strcmp( c, "count" ) || !strcmp( c, "COUNT" ) ) {
        return SELECT_COUNT;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4795
    if ( !strcmp( c, "order" ) || !strcmp( c, "ORDER" ) ) {
        return ORDER_BY;
    }
    if ( !strcmp( c, "order_desc" ) || !strcmp( c, "ORDER_DESC" ) ) {
        return ORDER_BY_DESC;
    }
    // =-=-=-=-=-=-=-

    return 1;
}


char *
getAttrNameFromAttrId( int cid ) {

    int i;
    for ( i = 0; i < NumOfColumnNames ; i++ ) {
        if ( columnNames[i].columnId == cid ) {
            return columnNames[i].columnName;
        }
    }
    return NULL;
}

int
goodStrExpr( char * expr ) {
    int qcnt = 0;
    int qqcnt = 0;
    int bcnt = 0;
    int i = 0;
    int inq =  0;
    int inqq =  0;
    while ( expr[i] != '\0' ) {
        if ( inq ) {
            if ( expr[i] == '\'' ) {
                inq--;
                qcnt++;
            }
        }
        else if ( inqq ) {
            if ( expr[i] == '"' ) {
                inqq--;
                qqcnt++;
            }
        }
        else if ( expr[i] == '\'' ) {
            inq++;
            qcnt++;
        }
        else if ( expr[i] == '"' ) {
            inqq++;
            qqcnt++;
        }
        else if ( expr[i] == '(' ) {
            bcnt++;
        }
        else if ( expr[i] == ')' )
            if ( bcnt > 0 ) {
                bcnt--;
            }
        i++;
    }
    if ( bcnt != 0 || qcnt % 2 != 0 || qqcnt % 2 != 0 ) {
        return -1;
    }
    return 0;

}


char *getCondFromString( char * t ) {
    char *u;
    char *u1, *u2;
    char *s;

    s = t;
    for ( ;; ) {
        /* Search for an 'and' string, either case, and use the one
           that appears first. */
        u1 = strstr( s, " and " );
        u2 = strstr( s, " AND " );
        u = u1;
        if ( u1 == NULL ) {
            u = u2;
        }
        if ( u1 != NULL && u2 != NULL ) {
            if ( strlen( u2 ) > strlen( u1 ) ) {
                u = u2;    /* both are present, use the first */
            }
        }

        if ( u != NULL ) {
            *u = '\0';
            if ( goodStrExpr( t ) == 0 ) {
                *u = ' ';
                return u;
            }
            *u = ' ';
            s = u + 1;
        }
        else {
            break;
        }
    }
    return NULL;
}

int
fillGenQueryInpFromStrCond( char * str, genQueryInp_t * genQueryInp ) {

    int  n, m;
    char *p, *t, *f, *u, *a, *c;
    char *s;
    s = strdup( str );
    if ( ( t = strstr( s, "select" ) ) != NULL ||
            ( t = strstr( s, "SELECT" ) ) != NULL ) {

        if ( ( f = strstr( t, "where" ) ) != NULL ||
                ( f = strstr( t, "WHERE" ) ) != NULL ) {
            /* Where Condition Found*/
            *f = '\0';
        }
        t = t +  7;
        while ( ( u = strchr( t, ',' ) ) != NULL ) {
            *u = '\0';
            trimWS( t );
            separateSelFuncFromAttr( t, &a, &c );
            m = getSelVal( a );
            n = getAttrIdFromAttrName( c );
            if ( n < 0 ) {
                free( s );
                return n;
            }
            addInxIval( &genQueryInp->selectInp, n, m );
            t  = u + 1;
        }
        trimWS( t );
        separateSelFuncFromAttr( t, &a, &c );
        m = getSelVal( a );
        n = getAttrIdFromAttrName( c );
        if ( n < 0 ) {
            free( s );
            return n;
        }
        addInxIval( &genQueryInp->selectInp, n, m );
        if ( f == NULL ) {
            free( s );
            return 0;
        }
    }
    else {
        free( s );
        return INPUT_ARG_NOT_WELL_FORMED_ERR;
    }
    t = f + 6;
    while ( ( u = getCondFromString( t ) ) != NULL ) {
        *u = '\0';
        trimWS( t );
        if ( ( p = strchr( t, ' ' ) ) == NULL ) {
            free( s );
            return INPUT_ARG_NOT_WELL_FORMED_ERR;
        }
        *p = '\0';
        n = getAttrIdFromAttrName( t );
        if ( n < 0 ) {
            free( s );
            return n;
        }
        addInxVal( &genQueryInp->sqlCondInp, n, p + 1 );
        t = u + 5;
    }
    trimWS( t );
    if ( ( p = strchr( t, ' ' ) ) == NULL ) {
        free( s );
        return INPUT_ARG_NOT_WELL_FORMED_ERR;
    }
    *p = '\0';
    n = getAttrIdFromAttrName( t );
    if ( n < 0 ) {
        free( s );
        return n;
    }
    addInxVal( &genQueryInp->sqlCondInp, n, p + 1 );
    free( s );
    return 0;
}

int
printGenQueryOut( FILE * fd, char * format, char * hint, genQueryOut_t * genQueryOut ) {
    int i = 0, n = 0, j = 0;
    sqlResult_t *v[MAX_SQL_ATTR];
    char * cname[MAX_SQL_ATTR];

    if ( hint != NULL &&  strlen( hint ) > 0 ) {
        //i = printHintedGenQueryOut(fd,format,hint, genQueryOut);
        return i;
    }

    n = genQueryOut->attriCnt;

    for ( i = 0; i < n; i++ ) {
        v[i] = &genQueryOut->sqlResult[i];
        cname[i] = getAttrNameFromAttrId( v[i]->attriInx );
        if ( cname[i] == NULL ) {
            return NO_COLUMN_NAME_FOUND;
        }
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        if ( format == NULL || strlen( format ) == 0 ) {
            for ( j = 0; j < n; j++ ) {
                fprintf( fd, "%s = %s\n", cname[j], &v[j]->value[v[j]->len * i] );
            }
            fprintf( fd, "------------------------------------------------------------\n" );
        }
        else {
            if ( n == 1 ) {
                fprintf( fd, format, &v[0]->value[v[0]->len * i] );
            }
            else if ( n == 2 ) {
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i] );
            }
            else if ( n == 3 ) {
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i] );
            }
            else if ( n == 4 ) {
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i] );
            }
            else if ( n == 5 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i] );
            else if ( n == 6 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i] );
            else if ( n == 7 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i], &v[6]->value[v[6]->len * i] );
            else if ( n == 8 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i], &v[6]->value[v[6]->len * i], &v[7]->value[v[7]->len * i] );
            else if ( n == 9 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i], &v[6]->value[v[6]->len * i], &v[7]->value[v[7]->len * i],
                         &v[8]->value[v[8]->len * i] );
            else if ( n == 10 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i], &v[6]->value[v[6]->len * i], &v[7]->value[v[7]->len * i],
                         &v[8]->value[v[8]->len * i], &v[9]->value[v[9]->len * i] );
            else if ( n == 11 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i], &v[6]->value[v[6]->len * i], &v[7]->value[v[7]->len * i],
                         &v[8]->value[v[8]->len * i], &v[9]->value[v[9]->len * i], &v[10]->value[v[10]->len * i] );
            else if ( n == 12 )
                fprintf( fd, format, &v[0]->value[v[0]->len * i], &v[1]->value[v[1]->len * i], &v[2]->value[v[2]->len * i], &v[3]->value[v[3]->len * i],
                         &v[4]->value[v[4]->len * i], &v[5]->value[v[5]->len * i], &v[6]->value[v[6]->len * i], &v[7]->value[v[7]->len * i],
                         &v[8]->value[v[8]->len * i], &v[9]->value[v[9]->len * i], &v[10]->value[v[10]->len * i], &v[11]->value[v[11]->len * i] );
            fprintf( fd, "\n" );
        }

    }
    return 0;
}


int
appendToByteBuf( bytesBuf_t * bytesBuf, char * str ) {
    int i, j;
    char *tBuf;

    i = strlen( str );
    j = 0;
    if ( bytesBuf->buf == NULL ) {
        bytesBuf->buf = malloc( i + 1 + MAX_NAME_LEN * 5 );
        strcpy( ( char * )bytesBuf->buf, str );
        bytesBuf->len = i + 1 + MAX_NAME_LEN * 5; /* allocated length */
    }
    else {
        j = strlen( ( char * )bytesBuf->buf );
        if ( ( i + j ) < bytesBuf->len ) {
            strcat( ( char * )bytesBuf->buf, str );
        }
        else { /* needs more space */
            tBuf = ( char * ) malloc( j + i + 1 + ( MAX_NAME_LEN * 5 ) );
            strcpy( tBuf, ( char * )bytesBuf->buf );
            strcat( tBuf, str );
            free( bytesBuf->buf );
            bytesBuf->len = j + i + 1 + ( MAX_NAME_LEN * 5 );
            bytesBuf->buf = tBuf;
        }
    }
    /*
        printf("bytesBuf->len=%d  oldbufLen=%d  strlen=%d\n",bytesBuf->len,j,i);
        printf("bytesBuf->buf:%s\n",bytesBuf->buf);
     */
    return 0;
}


int
getMountedSubPhyPath( char * logMountPoint, char * phyMountPoint,
                      char * logSubPath, char * phySubPathOut ) {
    char *tmpPtr;
    int len = strlen( logMountPoint );

    if ( strncmp( logSubPath, logMountPoint, len ) != 0 ) {
        rodsLog( LOG_ERROR,
                 "getMountedSubPhyPath: sub path %s not in mount point %s",
                 logSubPath, logMountPoint );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    /* skip over the mount point */
    tmpPtr = logSubPath + len;
    /* compose the physical path */
    snprintf( phySubPathOut, MAX_NAME_LEN, "%s%s", phyMountPoint,
              tmpPtr );
    return 0;

}

int
getSpecCollTypeStr( specColl_t * specColl, char * outStr ) {
    int i;

    if ( specColl->collClass == NO_SPEC_COLL ) {
        return SYS_UNMATCHED_SPEC_COLL_TYPE;
    }
    else if ( specColl->collClass == MOUNTED_COLL ) {
        rstrcpy( outStr, MOUNT_POINT_STR, NAME_LEN );
        return 0;
    }
    else if ( specColl->collClass == LINKED_COLL ) {
        rstrcpy( outStr, LINK_POINT_STR, NAME_LEN );
        return 0;
    }
    else {
        for ( i = 0; i < NumStructFileType; i++ ) {
            if ( specColl->type == StructFileTypeDef[i].type ) {
                rstrcpy( outStr, StructFileTypeDef[i].typeName, NAME_LEN );
                return 0;
            }
        }
        rodsLog( LOG_ERROR,
                 "getSpecCollTypeStr: unmatch specColl type %d", specColl->type );
        return SYS_UNMATCHED_SPEC_COLL_TYPE;
    }
}

int
resolveSpecCollType( char * type, char * collection, char * collInfo1,
                     char * collInfo2, specColl_t * specColl ) {
    int i;

    if ( specColl == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( *type == '\0' ) {
        specColl->collClass = NO_SPEC_COLL;
        return SYS_UNMATCHED_SPEC_COLL_TYPE;
    }

    rstrcpy( specColl->collection, collection,
             MAX_NAME_LEN );

    if ( strcmp( type, MOUNT_POINT_STR ) == 0 ) {
        specColl->collClass = MOUNTED_COLL;
        rstrcpy( specColl->phyPath, collInfo1, MAX_NAME_LEN );

        irods::hierarchy_parser parse;
        parse.set_string( collInfo2 );

        std::string first_resc;
        parse.first_resc( first_resc );

        rstrcpy( specColl->resource, first_resc.c_str(), NAME_LEN );
        rstrcpy( specColl->rescHier, collInfo2, NAME_LEN );

        return 0;
    }
    else if ( strcmp( type, LINK_POINT_STR ) == 0 ) {
        specColl->collClass = LINKED_COLL;
        rstrcpy( specColl->phyPath, collInfo1, MAX_NAME_LEN );

        return 0;
    }
    else {
        for ( i = 0; i < NumStructFileType; i++ ) {
            if ( strcmp( type, StructFileTypeDef[i].typeName ) == 0 ) {
                specColl->collClass = STRUCT_FILE_COLL;
                specColl->type = StructFileTypeDef[i].type;
                rstrcpy( specColl->objPath, collInfo1,
                         MAX_NAME_LEN );
                parseCachedStructFileStr( collInfo2, specColl );
                return 0;
            }
        }

        specColl->collClass = NO_SPEC_COLL;
        rodsLog( LOG_ERROR,
                 "resolveSpecCollType: unmatch specColl type %s", type );
        return SYS_UNMATCHED_SPEC_COLL_TYPE;
    }
}

int
parseCachedStructFileStr( char * collInfo2, specColl_t * specColl ) {
    char *tmpPtr1, *tmpPtr2;
    int len;

    if ( collInfo2 == NULL || specColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "parseCachedStructFileStr: NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strlen( collInfo2 ) == 0 ) {
        /* empty */
        specColl->cacheDir[0] = specColl->resource[0] = '\0';
        return 0;
    }

    tmpPtr1 = strstr( collInfo2, ";;;" );

    if ( tmpPtr1 == NULL ) {
        rodsLog( LOG_NOTICE,
                 "parseCachedStructFileStr: collInfo2 %s format error 1", collInfo2 );
        return SYS_COLLINFO_2_FORMAT_ERR;
    }

    len = ( int )( tmpPtr1 - collInfo2 );
    strncpy( specColl->cacheDir, collInfo2, len );

    tmpPtr1 += 3;

    tmpPtr2 = strstr( tmpPtr1, ";;;" );

    if ( tmpPtr2 == NULL ) {
        rodsLog( LOG_NOTICE,
                 "parseCachedStructFileStr: collInfo2 %s format error 2", collInfo2 );
        return SYS_COLLINFO_2_FORMAT_ERR;
    }

    len = ( int )( tmpPtr2 - tmpPtr1 );
    *tmpPtr2 = '\0';

    irods::hierarchy_parser parse;
    parse.set_string( tmpPtr1 );

    std::string first_resc;
    parse.first_resc( first_resc );

    snprintf( specColl->resource, sizeof( specColl->resource ),
              "%s", first_resc.c_str() );
    snprintf( specColl->rescHier, sizeof( specColl->rescHier ),
              "%s", tmpPtr1 );
    tmpPtr2 += 3;

    specColl->cacheDirty = atoi( tmpPtr2 );

    return 0;
}

int
makeCachedStructFileStr( char * collInfo2, specColl_t * specColl ) {
    if ( collInfo2 == NULL || specColl == NULL ) {
        rodsLog( LOG_ERROR,
                 "makeCachedStructFileStr: NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strlen( specColl->resource ) == 0 || strlen( specColl->cacheDir ) == 0 ) {
        return 0;
    }

    snprintf( collInfo2, MAX_NAME_LEN, "%s;;;%s;;;%d",
              //specColl->cacheDir, specColl->resource, specColl->cacheDirty);
              specColl->cacheDir, specColl->rescHier, specColl->cacheDirty );

    return 0;
}

int
getErrno( int irodError ) {
    int unixErrno = irodError % 1000;

    if ( unixErrno < 0 ) {
        unixErrno = -1 * unixErrno;
    }

    return unixErrno;
}

int
getIrodsErrno( int irodError ) {
    int irodsErrno = irodError / 1000 * 1000;
    return irodsErrno;
}

structFileOprType_t
getSpecCollOpr( keyValPair_t * condInput, specColl_t * specColl ) {
    if ( specColl == NULL ) {
        return NOT_SPEC_COLL_OPR;
    }

    if ( specColl->collClass != STRUCT_FILE_COLL ) {
        return NON_STRUCT_FILE_SPEC_COLL_OPR;
    }

    if ( getValByKey( condInput, STRUCT_FILE_OPR_KW ) == NULL ) {
        return NORMAL_OPR_ON_STRUCT_FILE_COLL;
    }
    else {
        return STRUCT_FILE_SPEC_COLL_OPR;
    }
}


/**
 * Output a list of keyword-value pairs as a property string.
 *
 * @param       list            a keyword-value pair list
 * @param       string          a property string
 * @return                      a status code, 0 on success
 */
int
keyValToString( keyValPair_t * list, char** string ) {
    if ( NULL == list || NULL == string ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "keyValToString :: null input parameter" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    int nBytes = 0;
    int nKeys = list->len;
    int i;

    //if ( list == NULL ) // JMC cppcheck - nullptr
    //return SYS_INTERNAL_NULL_INPUT_ERR;

    /* Scan the list to figure out how much space we need. */
    for ( i = 0; i < nKeys; i++ ) {
        int nk, nv;
        if ( list->keyWord[i] == NULL || list->keyWord[i][0] == '\0' ) {
            continue;    /* Null keyword means empty entry */
        }
        if ( list->value[i] == NULL ) {
            continue;    /* Null value is not legal */
        }
        nk = strlen( list->keyWord[i] );
        nv = strlen( list->value[i] );

        /* <keyword>value</keyword> */
        nBytes += 1 + nk + 1 +   nv   + 2 + nk + 1 + 1;
    }
    nBytes++;

    /* Allocate the space. */
    *string = ( char* )malloc( nBytes );
    memset( *string, 0, nBytes );

    /* Write the string. */
    for ( i = 0; i < nKeys; i++ ) {
        if ( list->keyWord[i] == NULL || list->keyWord[i][0] == '\0' ) {
            continue;    /* Null keyword means empty entry */
        }
        if ( list->value[i] == NULL ) {
            continue;    /* Null value is not legal */
        }

        strcat( *string, "<" );
        strcat( *string, list->keyWord[i] );
        strcat( *string, ">" );
        strcat( *string, list->value[i] );
        strcat( *string, "</" );
        strcat( *string, list->keyWord[i] );
        strcat( *string, ">\n" );
    }

    return 0;
}




/**
 * Parse a property string into a list of keyword-value pairs.
 * The property string is assumed to be well-formed.
 *
 * @param       string          a property string
 * @param       list            a keyword-value pair list
 * @return                      a status code, 0 on success
 */
int
keyValFromString( char * string, keyValPair_t** list ) {
    int index = 0;
    int len = strlen( string );

    /* Create and clear the list. */
    *list = ( keyValPair_t* )malloc( sizeof( keyValPair_t ) );
    memset( *list, 0, sizeof( keyValPair_t ) );

    /* Parse the string.  Extract out the tag and value */
    /* without doing any memory allocations (to save time). */
    while ( TRUE ) {
        int startTag = -1;
        int endTag = -1;
        char* tag = NULL;

        int startCloseTag = -1;
        int endCloseTag = -1;
        char* closeTag = NULL;

        int startValue = -1;
        int endValue = -1;
        char* value = NULL;

        /* Skip over everything until the start */
        /* of the next tag. */
        while ( index < len && string[index] != '<' ) {
            ++index;
        }
        if ( index == len ) {
            break;    /* Done */
        }


        /* Get the tag. */
        index++;                /* Skip '<' */
        startTag = index;
        while ( index < len && string[index] != '>' ) {
            ++index;
        }
        if ( index == len ) {
            break;    /* Done */
        }
        endTag = index;
        index++;                /* Skip '>' */
        startValue = index;


        /* Skip to the end of the value. */
        while ( index < len && string[index] != '<' ) {
            ++index;
        }
        if ( index == len ) {
            break;    /* Done */
        }
        endValue = index;


        /* Get the close tag.Y */
        index += 2;             /* Skip '</' */
        startCloseTag = index;
        while ( index < len && string[index] != '>' ) {
            ++index;
        }
        if ( index == len ) {
            break;    /* Done */
        }
        endCloseTag = index;


        /* Compare the opening and closing tags */
        /* and make sure they match. */
        if ( ( endTag - startTag ) != ( endCloseTag - startCloseTag ) )
            /* Find a better error code! */
        {
            return UNMATCHED_KEY_OR_INDEX;
        }
        tag = string + startTag;
        closeTag = string + startCloseTag;
        value = string + startValue;
        if ( strncmp( tag, closeTag, ( endTag - startTag ) ) != 0 )
            /* Find a better error code! */
        {
            return UNMATCHED_KEY_OR_INDEX;
        }

        /* Temporarily add a NULL at the end of the tag */
        /* and the value.  This gives us two NULL-terminated */
        /* strings to pass in to the addKeyVal. */
        string[endTag] = '\0';          /* was '>' */
        string[endValue] = '\0';        /* was '<' */

        /* Add the key-value pair. */
        addKeyVal( *list, tag, value );

        /* Remove the NULLs added above. */
        string[endTag] = '>';
        string[endValue] = '<';
    }

    return 0;
}

int
clearSendXmsgInfo( sendXmsgInfo_t * sendXmsgInfo ) {
    if ( sendXmsgInfo == NULL ) {
        return 0;
    }

    if ( sendXmsgInfo->msg != NULL ) {
        free( sendXmsgInfo->msg );
    }

    if ( sendXmsgInfo->deliPort != NULL ) {
        free( sendXmsgInfo->deliPort );
    }

    if ( sendXmsgInfo->miscInfo != NULL ) {
        free( sendXmsgInfo->miscInfo );
    }

    if ( sendXmsgInfo->deliAddress != NULL &&
            *sendXmsgInfo->deliAddress != NULL ) {
        int i;

        for ( i = 0; i < sendXmsgInfo->numDeli; i++ ) {
            free( sendXmsgInfo->deliAddress[i] );
        }
        free( sendXmsgInfo->deliAddress );
    }
    memset( sendXmsgInfo, 0, sizeof( sendXmsgInfo_t ) );

    return 0;
}

void
freeStringIfNotNull( char * str ) {
    if ( str != NULL ) {
        free( str );
    }
}

void
clearModAVUMetadataInp( void* voidInp ) {
    modAVUMetadataInp_t * modAVUMetadataInp = ( modAVUMetadataInp_t* )voidInp;
    freeStringIfNotNull( modAVUMetadataInp->arg0 );
    freeStringIfNotNull( modAVUMetadataInp->arg1 );
    freeStringIfNotNull( modAVUMetadataInp->arg2 );
    freeStringIfNotNull( modAVUMetadataInp->arg3 );
    freeStringIfNotNull( modAVUMetadataInp->arg4 );
    freeStringIfNotNull( modAVUMetadataInp->arg5 );
    freeStringIfNotNull( modAVUMetadataInp->arg6 );
    freeStringIfNotNull( modAVUMetadataInp->arg7 );
    freeStringIfNotNull( modAVUMetadataInp->arg8 );
    freeStringIfNotNull( modAVUMetadataInp->arg9 );
    memset( modAVUMetadataInp, 0, sizeof( modAVUMetadataInp_t ) );
    return;
}

/* freeRodsObjStat - free a rodsObjStat_t. Note that this should only
 * be used by the client because specColl also is freed which is cached
 * on the server
 */
int
freeRodsObjStat( rodsObjStat_t * rodsObjStat ) {
    if ( rodsObjStat == NULL ) {
        return 0;
    }

    if ( rodsObjStat->specColl != NULL ) {
        free( rodsObjStat->specColl );
    }

    free( rodsObjStat );

    return 0;
}

int
parseHostAddrStr( char * hostAddr, rodsHostAddr_t * addr ) {
    char port[SHORT_STR_LEN];
    char buffer[LONG_NAME_LEN];

    if ( hostAddr == NULL || addr == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( splitPathByKey( hostAddr, buffer, LONG_NAME_LEN, port, SHORT_STR_LEN, ':' ) < 0 ) {
        rstrcpy( addr->hostAddr, hostAddr, LONG_NAME_LEN );
        addr->portNum = 0;
    }
    else {
        rstrcpy( addr->hostAddr, buffer, LONG_NAME_LEN );
        addr->portNum = atoi( port );
    }
    return 0;
}

/*
   Print some release information.
   Used by the i-commands when printting the help text.
 */
void
printReleaseInfo( char * cmdName ) {
    char tmp[40];
    strncpy( tmp, RODS_REL_VERSION, 40 );   /* to skip over the 'rods' part
                                                             of the string */
    tmp[39] = '\0';
    printf( "\niRODS Version %s                %s                      %s\n",
            ( char* )&tmp[4], RODS_RELEASE_DATE, cmdName );
    return;
}

unsigned int
seedRandom() {
    unsigned int seed;
    const int random_fd = open( "/dev/urandom", O_RDONLY );
    if ( random_fd == -1 ) {
        rodsLog( LOG_ERROR, "seedRandom: failed to open /dev/urandom" );
        return FILE_OPEN_ERR;
    }
    char buf[sizeof( seed )];
    const ssize_t count = read( random_fd, &buf, sizeof( buf ) );
    close( random_fd );
    if ( count != sizeof( seed ) ) {
        rodsLog( LOG_ERROR, "seedRandom: failed to read enough bytes from /dev/urandom" );
        return FILE_READ_ERR;
    }
    memcpy( &seed, buf, sizeof( seed ) );

#ifdef windows_platform
    srand( seed );
#else
    srandom( seed );
#endif

    return 0;
}

int
initBulkDataObjRegInp( genQueryOut_t * bulkDataObjRegInp ) {
    if ( bulkDataObjRegInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    memset( bulkDataObjRegInp, 0, sizeof( genQueryOut_t ) );

    bulkDataObjRegInp->attriCnt = 10;

    bulkDataObjRegInp->sqlResult[0].attriInx = COL_DATA_NAME;
    bulkDataObjRegInp->sqlResult[0].len = MAX_NAME_LEN;
    bulkDataObjRegInp->sqlResult[0].value =
        ( char * )malloc( MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[0].value,
           MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[1].attriInx = COL_DATA_TYPE_NAME;
    bulkDataObjRegInp->sqlResult[1].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[1].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[1].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[2].attriInx = COL_DATA_SIZE;
    bulkDataObjRegInp->sqlResult[2].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[2].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[2].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[3].attriInx = COL_D_RESC_NAME;
    bulkDataObjRegInp->sqlResult[3].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[3].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[3].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[4].attriInx = COL_D_DATA_PATH;
    bulkDataObjRegInp->sqlResult[4].len = MAX_NAME_LEN;
    bulkDataObjRegInp->sqlResult[4].value =
        ( char * )malloc( MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[4].value,
           MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[5].attriInx = COL_DATA_MODE;
    bulkDataObjRegInp->sqlResult[5].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[5].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[5].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[6].attriInx = OPR_TYPE_INX;
    bulkDataObjRegInp->sqlResult[6].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[6].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[6].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[7].attriInx = COL_DATA_REPL_NUM;
    bulkDataObjRegInp->sqlResult[7].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[7].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[7].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[8].attriInx = COL_D_DATA_CHECKSUM;
    bulkDataObjRegInp->sqlResult[8].len = NAME_LEN;
    bulkDataObjRegInp->sqlResult[8].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[8].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bulkDataObjRegInp->sqlResult[9].attriInx = COL_D_RESC_HIER;
    bulkDataObjRegInp->sqlResult[9].len = MAX_NAME_LEN;
    bulkDataObjRegInp->sqlResult[9].value =
        ( char * )malloc( MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( bulkDataObjRegInp->sqlResult[9].value,
           MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );


    bulkDataObjRegInp->continueInx = -1;

    return 0;
}

int
initBulkDataObjRegOut( genQueryOut_t **bulkDataObjRegOut ) {
    genQueryOut_t *myBulkDataObjRegOut;

    if ( bulkDataObjRegOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    myBulkDataObjRegOut = *bulkDataObjRegOut = ( genQueryOut_t* )malloc( sizeof( genQueryOut_t ) );
    if ( myBulkDataObjRegOut == NULL ) {
        return SYS_MALLOC_ERR;
    }

    memset( myBulkDataObjRegOut, 0, sizeof( genQueryOut_t ) );

    myBulkDataObjRegOut->attriCnt = 1;

    myBulkDataObjRegOut->sqlResult[0].attriInx = COL_D_DATA_ID;
    myBulkDataObjRegOut->sqlResult[0].len = NAME_LEN;
    myBulkDataObjRegOut->sqlResult[0].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( myBulkDataObjRegOut->sqlResult[0].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );

    myBulkDataObjRegOut->continueInx = -1;
    return 0;
}

int
fillBulkDataObjRegInp( const char * rescName, const char* rescHier, char * objPath,
                       char * filePath, char * dataType, rodsLong_t dataSize, int dataMode,
                       int modFlag, int replNum, char * chksum, genQueryOut_t * bulkDataObjRegInp ) {

    int rowCnt;

    if ( bulkDataObjRegInp == NULL || rescName == NULL || objPath == NULL ||
            filePath == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    rowCnt = bulkDataObjRegInp->rowCnt;

    if ( rowCnt >= MAX_NUM_BULK_OPR_FILES ) {
        return SYS_BULK_REG_COUNT_EXCEEDED;
    }

    rstrcpy( &bulkDataObjRegInp->sqlResult[0].value[MAX_NAME_LEN * rowCnt],
             objPath, MAX_NAME_LEN );
    rstrcpy( &bulkDataObjRegInp->sqlResult[1].value[NAME_LEN * rowCnt],
             dataType, NAME_LEN );
    snprintf( &bulkDataObjRegInp->sqlResult[2].value[NAME_LEN * rowCnt],
              NAME_LEN, "%lld", dataSize );
    rstrcpy( &bulkDataObjRegInp->sqlResult[3].value[NAME_LEN * rowCnt],
             rescName, NAME_LEN );
    rstrcpy( &bulkDataObjRegInp->sqlResult[4].value[MAX_NAME_LEN * rowCnt],
             filePath, MAX_NAME_LEN );
    snprintf( &bulkDataObjRegInp->sqlResult[5].value[NAME_LEN * rowCnt],
              NAME_LEN, "%d", dataMode );
    if ( modFlag == 1 ) {
        rstrcpy( &bulkDataObjRegInp->sqlResult[6].value[NAME_LEN * rowCnt],
                 MODIFY_OPR, NAME_LEN );
    }
    else {
        rstrcpy( &bulkDataObjRegInp->sqlResult[6].value[NAME_LEN * rowCnt],
                 REGISTER_OPR, NAME_LEN );
    }
    snprintf( &bulkDataObjRegInp->sqlResult[7].value[NAME_LEN * rowCnt],
              NAME_LEN, "%d", replNum );
    if ( chksum != NULL && strlen( chksum ) > 0 ) {
        rstrcpy( &bulkDataObjRegInp->sqlResult[8].value[NAME_LEN * rowCnt],
                 chksum, NAME_LEN );
    }
    else {
        bulkDataObjRegInp->sqlResult[8].value[NAME_LEN * rowCnt] = '\0';
    }
    snprintf( &bulkDataObjRegInp->sqlResult[9].value[MAX_NAME_LEN * rowCnt],
              MAX_NAME_LEN, "%s", rescHier );
    bulkDataObjRegInp->rowCnt++;

    return 0;
}

int
initAttriArrayOfBulkOprInp( bulkOprInp_t * bulkOprInp ) {
    genQueryOut_t *attriArray;
    int i;

    if ( bulkOprInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    attriArray = &bulkOprInp->attriArray;

    attriArray->attriCnt = 3;

    attriArray->sqlResult[0].attriInx = COL_DATA_NAME;
    attriArray->sqlResult[0].len = MAX_NAME_LEN;
    attriArray->sqlResult[0].value =
        ( char * )malloc( MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( attriArray->sqlResult[0].value,
           MAX_NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    attriArray->sqlResult[1].attriInx = COL_DATA_MODE;
    attriArray->sqlResult[1].len = NAME_LEN;
    attriArray->sqlResult[1].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( attriArray->sqlResult[1].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    attriArray->sqlResult[2].attriInx = OFFSET_INX;
    attriArray->sqlResult[2].len = NAME_LEN;
    attriArray->sqlResult[2].value =
        ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
    bzero( attriArray->sqlResult[2].value,
           NAME_LEN * MAX_NUM_BULK_OPR_FILES );

    if ( getValByKey( &bulkOprInp->condInput, REG_CHKSUM_KW ) != NULL ||
            getValByKey( &bulkOprInp->condInput, VERIFY_CHKSUM_KW ) != NULL ) {
        i = attriArray->attriCnt;
        attriArray->sqlResult[i].attriInx = COL_D_DATA_CHECKSUM;
        attriArray->sqlResult[i].len = NAME_LEN;
        attriArray->sqlResult[i].value =
            ( char * )malloc( NAME_LEN * MAX_NUM_BULK_OPR_FILES );
        bzero( attriArray->sqlResult[i].value,
               NAME_LEN * MAX_NUM_BULK_OPR_FILES );
        attriArray->attriCnt++;
    }
    attriArray->continueInx = -1;
    return 0;
}

int
fillAttriArrayOfBulkOprInp( char * objPath, int dataMode, char * inpChksum,
                            int offset, bulkOprInp_t * bulkOprInp ) {
    genQueryOut_t *attriArray;
    int rowCnt;
    sqlResult_t *chksum = NULL;

    if ( bulkOprInp == NULL || objPath == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    attriArray = &bulkOprInp->attriArray;

    rowCnt = attriArray->rowCnt;

    if ( rowCnt >= MAX_NUM_BULK_OPR_FILES ) {
        return SYS_BULK_REG_COUNT_EXCEEDED;
    }

    chksum = getSqlResultByInx( attriArray, COL_D_DATA_CHECKSUM );
    if ( inpChksum != NULL && strlen( inpChksum ) > 0 ) {
        if ( chksum == NULL ) {
            rodsLog( LOG_ERROR,
                     "initAttriArrayOfBulkOprInp: getSqlResultByInx for COL_D_DATA_CHECKSUM failed" );
            return UNMATCHED_KEY_OR_INDEX;
        }
        else {
            rstrcpy( &chksum->value[NAME_LEN * rowCnt], inpChksum, NAME_LEN );
        }
    }
    else {
        if ( chksum != NULL ) {
            chksum->value[NAME_LEN * rowCnt] = '\0';
        }
    }
    rstrcpy( &attriArray->sqlResult[0].value[MAX_NAME_LEN * rowCnt],
             objPath, MAX_NAME_LEN );
    snprintf( &attriArray->sqlResult[1].value[NAME_LEN * rowCnt],
              NAME_LEN, "%d", dataMode );
    snprintf( &attriArray->sqlResult[2].value[NAME_LEN * rowCnt],
              NAME_LEN, "%d", offset );

    attriArray->rowCnt++;

    return 0;
}

int
getAttriInAttriArray( const char * inpObjPath, genQueryOut_t * attriArray,
                      int * outDataMode, char **outChksum ) {
    int i;
    int startInx;
    sqlResult_t *objPath, *dataMode, *chksum;
    char *tmpObjPath, *tmpDataMode, *tmpChksum;

    if ( inpObjPath == NULL || attriArray == NULL || outDataMode == NULL ||
            outChksum == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( ( objPath =
                getSqlResultByInx( attriArray, COL_DATA_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getAttriInAttriArray: getSqlResultByInx for COL_DATA_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( dataMode =
                getSqlResultByInx( attriArray, COL_DATA_MODE ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "getAttriInAttriArray: getSqlResultByInx for COL_DATA_MODE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    chksum = getSqlResultByInx( attriArray, COL_D_DATA_CHECKSUM );

    startInx = attriArray->continueInx;
    if ( startInx >= attriArray->rowCnt || startInx < 0 ) {
        startInx = 0;
    }

    for ( i = startInx; i < attriArray->rowCnt; i++ ) {
        tmpObjPath = &objPath->value[objPath->len * i];
        if ( strcmp( inpObjPath, tmpObjPath ) == 0 ) {
            attriArray->continueInx = i + 1;
            tmpDataMode = &dataMode->value[dataMode->len * i];
            *outDataMode = atoi( tmpDataMode );
            if ( chksum != NULL ) {
                tmpChksum = &chksum->value[chksum->len * i];
                if ( strlen( tmpChksum ) > 0 ) {
                    *outChksum = tmpChksum;
                }
                else {
                    *outChksum = NULL;
                }
            }
            else {
                *outChksum = NULL;
            }
            return 0;
        }
    }

    for ( i = 0; i < startInx; i++ ) {
        tmpObjPath = &objPath->value[objPath->len * i];
        if ( strcmp( inpObjPath, tmpObjPath ) == 0 ) {
            attriArray->continueInx = i + 1;
            tmpDataMode = &dataMode->value[dataMode->len * i];
            *outDataMode = atoi( tmpDataMode );
            if ( chksum != NULL ) {
                tmpChksum = &chksum->value[chksum->len * i];
                if ( strlen( tmpChksum ) > 0 ) {
                    *outChksum = tmpChksum;
                }
                else {
                    *outChksum = NULL;
                }
            }
            else {
                *outChksum = NULL;
            }
            return 0;
        }
    }
    /* no match when got here */
    *outChksum = NULL;
    return UNMATCHED_KEY_OR_INDEX;
}

int
readToByteBuf( int fd, bytesBuf_t * bytesBuf ) {
    int toRead, buflen, nbytes;
    char *bufptr;

    if ( bytesBuf->len <= 0 ) {
        /* use default */
        buflen = INIT_SZ_FOR_EXECMD_BUF;
    }
    else {
        /* sanity check */
        buflen = bytesBuf->len;
        if ( buflen > MAX_SZ_FOR_EXECMD_BUF ) {
            return SYS_REQUESTED_BUF_TOO_LARGE;
        }
    }
    bytesBuf->len = 0;
    bytesBuf->buf = bufptr = ( char * )malloc( buflen );
    toRead = buflen;

    while ( 1 ) {
        nbytes = myRead( fd, bufptr, toRead, NULL, NULL );
        if ( nbytes == toRead ) { /* more */
            char *tmpPtr;

            bytesBuf->len += nbytes;
            if ( buflen >= MAX_SZ_FOR_EXECMD_BUF ) {
                return EXEC_CMD_OUTPUT_TOO_LARGE;
            }
            else {
                buflen = 4 * buflen;
                if ( buflen > MAX_SZ_FOR_EXECMD_BUF ) {
                    buflen = MAX_SZ_FOR_EXECMD_BUF;
                }
                toRead = buflen - bytesBuf->len;
                tmpPtr = ( char* )bytesBuf->buf;
                bytesBuf->buf = malloc( buflen );
                memcpy( bytesBuf->buf, tmpPtr, bytesBuf->len );
                free( tmpPtr );
                bufptr = ( char * ) bytesBuf->buf + bytesBuf->len;
            }
        }
        else {
            if ( nbytes > 0 ) {
                bytesBuf->len += nbytes;
                bufptr += nbytes;
            }
            if ( bytesBuf->len <= 0 ) {
                free( bytesBuf->buf );
                bytesBuf->buf = NULL;
            }
            break;
        }
    }
    if ( nbytes < 0 ) {
        return nbytes;
    }
    else {
        return 0;
    }
}

int
writeFromByteBuf( int fd, bytesBuf_t * bytesBuf ) {
    int toWrite, nbytes;
    char *bufptr;

    bufptr = ( char * )bytesBuf->buf;
    toWrite = bytesBuf->len;
    while ( ( nbytes = myWrite( fd, bufptr, toWrite, NULL ) ) >= 0 ) {
        toWrite -= nbytes;
        bufptr += nbytes;
        if ( toWrite <= 0 ) {
            break;
        }
    }
    close( fd );

    if ( toWrite != 0 ) {
        return SYS_COPY_LEN_ERR - errno;
    }
    else {
        return 0;
    }
}

int
setForceFlagForRestart( bulkOprInp_t * bulkOprInp, bulkOprInfo_t * bulkOprInfo ) {
    if ( bulkOprInp == NULL || bulkOprInfo == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( getValByKey( &bulkOprInp->condInput, FORCE_FLAG_KW ) != NULL ) {
        /* already has FORCE_FLAG_KW */
        return 0;
    }

    addKeyVal( &bulkOprInp->condInput, FORCE_FLAG_KW, "" );
    /* remember to remove it */
    bulkOprInfo->forceFlagAdded = 1;

    return 0;
}

int
getPhyBunPath( const char * collection, const char * objPath, const char * phyBunDir,
               char * outPhyBunPath ) {
    int collLen = strlen( collection );
    const char *subPath = objPath + collLen;

    if ( *subPath != '/' ) {
        rodsLogError( LOG_ERROR, USER_INPUT_PATH_ERR,
                      "getPhyBunPath: inconsistent collection %s and objPath %s",
                      collection, objPath );
        return USER_INPUT_PATH_ERR;
    }
    snprintf( outPhyBunPath, MAX_NAME_LEN, "%s%s", phyBunDir, subPath );
    return 0;
}

int mySetenvStr( const char * envname, const char * envval ) {
    int status;

#if defined(linux_platform)||defined(osx_platform)
    if ( envname == NULL || envval == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    status = setenv( envname, envval, 1 );
#else
    char *myBuf;
    int len;

    if ( envname == NULL || envval == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    len = strlen( envname ) + strlen( envval ) + 16;
    myBuf = ( char * )malloc( len );
    snprintf( myBuf, len, "%s=%s", envname, envval );
    status = putenv( myBuf );
    //      free( myBuf ); // JMC cppcheck - leak ==> backport 'fix' from comm trunk for solaris
#endif
    return status;
}

int
mySetenvInt( char * envname, int envval ) {
    int status;

#if defined(linux_platform)||defined(osx_platform)
    char myIntStr[NAME_LEN];
    if ( envname == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    snprintf( myIntStr, NAME_LEN, "%d", envval );
    status = setenv( envname, myIntStr, 1 );
#else
    char *myBuf;
    int len;
    if ( envname == NULL ) {
        return USER__NULL_INPUT_ERR;
    }
    len = strlen( envname ) + 20;
    myBuf = ( char * )malloc( len );
    snprintf( myBuf, len, "%s=%d", envname, envval );
    status = putenv( myBuf );
    //      free( myBuf ); // JMC cppcheck - leak ==> backport 'fix' from comm trunk for solaris
#endif
    return status;
}

int
getRandomArray( int **randomArray, int size ) {
    if ( size < 0 ) {
        *randomArray = NULL;
        return -1;
    }

    *randomArray = ( int * ) malloc( size * sizeof( int ) );
    for ( int i = 0; i < size; i++ ) {
        ( *randomArray )[i] = i + 1;
    }
    std::random_shuffle( *randomArray, *randomArray + size );

    return 0;
}

int
isPathSymlink( rodsArguments_t * rodsArgs, const char * myPath ) {
    path p( myPath );
    if ( rodsArgs != NULL && rodsArgs->link != True ) {
        return 0;
    }
    if ( exists( p ) && is_symlink( p ) ) {
        return 1;
    }
    else {
        return 0;
    }
}

void
clearAuthResponseInp( void * inauthResponseInp ) {
    authResponseInp_t *authResponseInp;

    authResponseInp = ( authResponseInp_t * ) inauthResponseInp;

    if ( authResponseInp == NULL ) {
        return;
    }
    free( authResponseInp->username );
    free( authResponseInp->response );
    memset( authResponseInp, 0, sizeof( authResponseInp_t ) );

    return;
}

char *trimPrefix( char * str ) {
    int i = 0;
    while ( str[i] != ' ' ) {
        i++;
    }
    while ( str[i] == ' ' ) {
        i++;
    }
    memmove( str, str + i, strlen( str ) + 1 - i );
    return str;
}

char *trimSpaces( char * str ) {
    char *p = str;
    char *psrc = str;

    while ( *psrc != '\0' && isspace( *psrc ) ) {
        psrc++;
    }

    while ( *psrc != '\0' ) {
        *( p++ ) = *( psrc++ );
    }

    p--;
    while ( isspace( *p ) && p - str >= 0 ) {
        p--;
    }

    p++;
    *p = '\0';

    return str;

}

int startsWith( const char * str, const char * prefix ) {
    return str == strstr( str, prefix );
}

int convertListToMultiString( char * strInput, int input ) {
    if ( strcmp( strInput, "null" ) == 0 ) {
        return 0;
    }
    char *src = strdup( strInput );

    char *p = strInput;
    char *psrc = src;

    /* replace % with %% */
    while ( *psrc != '\0' ) {
        if ( *psrc == '%' ) {
            *( p++ ) = '%';
            *( p++ ) = '%';
            psrc++;
        }
        else {
            *( p++ ) = *( psrc++ );
        }
    }
    *p = '\0';

    free( src );

    /* replace , with % and remove extra spaces */
    p = strInput;
    psrc = strInput;
    while ( *psrc != '\0' ) {
        /* variable name */
        while ( !isspace( *psrc ) && *psrc != '=' && *psrc != ',' && *psrc != '\0' ) {
            *( p++ ) = *( psrc++ );
        }

        /* skip spaces */
        while ( isspace( *psrc ) ) {
            psrc++;
        }
        if ( input ) {
            if ( *psrc == '=' ) {
                /* assignment */
                *( p++ ) = *( psrc++ );

                int inString = 0;
                char delim = '\0';
                while ( *psrc != '\0' ) {
                    if ( inString ) {
                        if ( *psrc == delim ) {
                            inString = 0;
                        }
                        else if ( *psrc == '\\' ) {
                            *( p++ ) = *( psrc++ );
                            if ( *psrc == '\0' ) {
                                return -1;
                            }
                        }
                        *( p++ ) = *( psrc++ );
                    }
                    else {
                        if ( *psrc == ',' ) {
                            *( p++ ) = '%';
                            psrc++;
                            break;
                        }
                        else {
                            if ( *psrc == '\'' || *psrc == '\"' ) {
                                inString = 1;
                                delim = *psrc;
                            }
                            *( p++ ) = *( psrc++ );
                        }
                    }
                }
            }
            else {
                return -1;
            }
        }
        else {
            if ( *psrc == '\0' ) {
                break;
            }
            else if ( *psrc == ',' ) {
                *( p++ ) = '%';
                psrc++;
            }
            else {
                return -1;
            }
        }
        /* skip spaces */
        while ( isspace( *psrc ) ) {
            psrc++;
        }
    }
    *p = '\0';
    return 0;
}

int
splitMultiStr( char * strInput, strArray_t * strArray ) {
    char *startPtr, *endPtr;
    int endReached = 0;

    if ( strInput == NULL || strArray == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    startPtr = endPtr = strInput;

    while ( 1 ) {
        // two %% will be taken as an input % instead of as a delimiter
        while ( *endPtr != '%' && *endPtr != '\0' ) {
            endPtr ++;
        }
        if ( *endPtr == '%' ) {
            if ( *( endPtr + 1 ) == '%' ) {
                endPtr ++;
                endPtr ++;
                continue;
            }
            *endPtr = '\0';
        }
        else {
            endReached = 1;
        }

        char *str = strdup( startPtr );
        char *p = str;
        char *psrc = str;
        while ( *psrc != '\0' ) {
            while ( *psrc != '%' && *psrc != '\0' ) {
                *( p++ ) = *( psrc++ );
            }
            if ( *psrc == '%' ) {
                *( p++ ) = *( psrc++ );
                psrc++;
            }
        }
        *p = '\0';

        addStrArray( strArray, str );

        free( str );

        if ( endReached == 1 ) {
            break;
        }

        endPtr++;
        startPtr = endPtr;
    }

    return strArray->len;
}

namespace boost {
    void assertion_failed( char const * expr,
                           char const * function, char const * file, long line ) {
        std::cerr <<
                  "***** Internal Program Error - assertion (" << expr << ") failed in "
                  << function << ":\n"
                  << file << '(' << line << "): errno = " << errno << std::endl;
    }


} // namespace boost

int
getPathStMode( const char* p ) {
    struct stat statbuf;

    if ( stat( p, &statbuf ) == 0 &&
            ( statbuf.st_mode & S_IFREG ) ) {
        return statbuf.st_mode;
    }
    else {
        return -1;
    }
}


int
hasSymlinkInDir( const char * mydir ) {
    int status;
    char subfilePath[MAX_NAME_LEN];
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;

    if ( mydir == NULL ) {
        return 0;
    }
    dirPtr = opendir( mydir );
    if ( dirPtr == NULL ) {
        return 0;
    }

    while ( ( myDirent = readdir( dirPtr ) ) != NULL ) {
        if ( strcmp( myDirent->d_name, "." ) == 0 ||
                strcmp( myDirent->d_name, ".." ) == 0 ) {
            continue;
        }
        snprintf( subfilePath, MAX_NAME_LEN, "%s/%s",
                  mydir, myDirent->d_name );
        status = lstat( subfilePath, &statbuf );
        if ( status != 0 ) {
            rodsLog( LOG_ERROR,
                     "hasSymlinkIndir: stat error for %s, errno = %d",
                     subfilePath, errno );
            continue;
        }
        if ( ( statbuf.st_mode & S_IFLNK ) == S_IFLNK ) {
            rodsLog( LOG_ERROR,
                     "hasSymlinkIndir: %s is a symlink",
                     subfilePath );
            closedir( dirPtr );
            return 1;
        }
        if ( ( statbuf.st_mode & S_IFDIR ) != 0 ) {
            if ( hasSymlinkInDir( subfilePath ) ) {
                closedir( dirPtr );
                return 1;
            }
        }
    }
    closedir( dirPtr );
    return 0;
}

int
hasSymlinkInPartialPath( const char * myPath, int pos ) {
    const char *curPtr = myPath + pos;
    struct stat statbuf;
    int status;

    status = lstat( myPath, &statbuf );
    if ( status != 0 ) {
        rodsLog( LOG_ERROR,
                 "hasSymlinkInPartialPath: stat error for %s, errno = %d",
                 myPath, errno );
        return 0;
    }
    if ( ( statbuf.st_mode & S_IFLNK ) == S_IFLNK ) {
        rodsLog( LOG_ERROR,
                 "hasSymlinkInPartialPath: %s is a symlink", myPath );
        return 1;
    }

    while ( ( curPtr = strchr( curPtr, '/' ) ) != NULL ) {
        std::string sub_path( myPath, curPtr - myPath );
        status = lstat( sub_path.c_str(), &statbuf );
        if ( status != 0 ) {
            rodsLog( LOG_ERROR,
                     "hasSymlinkInPartialPath: stat error for %s, errno = %d",
                     sub_path.c_str(), errno );
            return 0;
        }
        if ( ( statbuf.st_mode & S_IFLNK ) == S_IFLNK ) {
            rodsLog( LOG_ERROR,
                     "hasSymlinkInPartialPath: %s is a symlink", sub_path.c_str() );
            return 1;
        }
        curPtr++;
    }
    return 0;
}

int
hasSymlinkInPath( const char * myPath ) {
    static char lastCheckedPath[MAX_NAME_LEN];
    int status, i;
    int lastSlashPos = 0;

    for ( i = 0; i < MAX_NAME_LEN; i++ ) {
        if ( lastCheckedPath[i] != myPath[i] ) {
            break;
        }
        if ( lastCheckedPath[i] == '/' ) {
            lastSlashPos = i;
        }
    }
    status = hasSymlinkInPartialPath( myPath, lastSlashPos + 1 );
    if ( status == 0 ) {
        rstrcpy( lastCheckedPath, myPath, MAX_NAME_LEN );
    }
    return status;
}
