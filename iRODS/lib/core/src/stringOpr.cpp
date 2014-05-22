/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* stringOpr - a number of string operations designed for secure string
 * copying.
 */

#include "stringOpr.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"

char *rmemmove( void *dest, void *src, int strLen, int maxLen ) {

    if ( dest == NULL || src == NULL ) {
        return NULL;
    }

    if ( strLen > maxLen ) {
        return NULL;
    }

    if ( memmove( dest, src, strLen ) != NULL ) {
        return ( char* )dest;
    }
    else {
        return NULL;
    }
}

char *rmemcpy( void *dest, void *src, int strLen, int maxLen ) {

    if ( dest == NULL || src == NULL ) {
        return NULL;
    }

    if ( strLen > maxLen ) {
        return NULL;
    }

    if ( memcpy( dest, src, strLen ) != NULL ) {
        return ( char* )dest;
    }
    else {
        return NULL;
    }
}

char *rstrcpy( char *dest, const char *src, int maxLen ) {
    /*
    The purpose of this function is to make sure there is enough space in
    the destination char array so that memory (the stack or whatever)
    will not be corrupted.  This is called from throughout the irods code
    as a safer way to copy strings.  Generally, the caller has an
    destination array that is believed to be plenty large enough, but
    this is used to make sure memory is not corrupted.  A memory
    corruption will often cause very odd, un-safe, and difficult to
    resolve errors.  If there is not enough space, the caller's function
    will probably fail, but by protecting memory it should fail in a more
    controlled and safer manner.
     */
    int len;

    if ( dest == NULL || src == NULL ) {
        return NULL;
    }

    len = strlen( src );

    if ( len < 0 || len >= maxLen ) {
        rodsLog( LOG_ERROR,
                 "rstrcpy not enough space in dest, slen:%d, maxLen:%d",
                 len, maxLen );
        return NULL;
    }

    if ( strncpy( dest, src, len ) != NULL ) {
        dest[len] = '\0';
        return dest;
    }
    else {
        rodsLog( LOG_ERROR,
                 "rstrcpy not enough space in dest, slen:%d, maxLen:%d",
                 len, maxLen );
        return NULL;
    }
}

char *rstrcat( char *dest, const char *src, int maxLen ) {
    /*  rods strcat: like strncat but make sure the dest doesn't overflow.
        maxLen is actually max length that can be stored in dest, not
        just how much of src needs to be copied.  Hence the
        semantics is different from strncat.
    */

    int dlen, slen;

    if ( dest == NULL || src == NULL ) {
        return NULL;
    }

    dlen = strlen( dest );
    slen = strlen( src );

    if ( slen + dlen >= maxLen ) {
        rodsLog( LOG_ERROR,
                 "rstrcat not enough space in dest, slen:%d, dlen:%d, maxLen:%d",
                 slen, dlen, maxLen );
        return( NULL );
    }

    return( strncat( dest, src, slen ) );
}

/*  rods strncat: like strncat but make sure the dest doesn't overflow.
    maxLen is the max length that can be stored in dest,
    srcLen is the length to copy.
*/
char *rstrncat( char *dest, const char *src, int srcLen, int maxLen ) {

    int dlen, slen;

    if ( dest == NULL || src == NULL ) {
        return NULL;
    }

    dlen = strlen( dest );
    slen = srcLen;

    if ( slen + dlen >= maxLen ) {
        rodsLog( LOG_ERROR,
                 "rstrncat not enough space in dest, slen:%d, dlen:%d, maxLen:%d",
                 slen, dlen, maxLen );
        return( NULL );
    }

    return( strncat( dest, src, slen ) );
}

int
rSplitStr( char *inStr, char* outStr1, int maxOutLen1,
           char* outStr2, int maxOutLen2, char key ) {
    int len;
    int c;
    char *inPtr, *outPtr;

    inPtr = inStr;
    outPtr = outStr1;
    len = 0;

    while ( ( c = *inPtr ) != '\0' ) {
        inPtr ++;
        if ( c == key ) {
            break;
        }
        else {
            *outPtr = c;
        }
        if ( len >= maxOutLen1 ) {
            *outStr1 = '\0';
            return ( USER_STRLEN_TOOLONG );
        }
        outPtr ++;
        len ++;
    }

    *outPtr = '\0';

    /* copy the second str */

    if ( rstrcpy( outStr2, inPtr, maxOutLen2 ) == NULL ) {
        return ( USER_STRLEN_TOOLONG );
    }
    else {
        return ( 0 );
    }
}
/* copyStrFromBuf - copy a string from buf to outStr, skipping white space
 * and comment. also advance buf pointer
 * returns the len of string copied
 */

int
copyStrFromBuf( char **buf, char *outStr, int maxOutLen ) {
    char *bufPtr, *outPtr;
    int len;
    int gotSpace;

    bufPtr = *buf;
    gotSpace = 0;

    /* skip over any space */

    while ( 1 ) {
        if ( *bufPtr == '\0' || *bufPtr == '\n' ) {
            return 0;
        }
        /* '#' must be preceded by a space to be a valid comment.
         * the calling routine must check if the line starts with a # */

        if ( *bufPtr == '#' && gotSpace > 0 ) {
            *outStr = '\0';
            return 0;
        }

        if ( isspace( *bufPtr ) ) {
            bufPtr ++;
            gotSpace ++;
            continue;
        }
        else {
            break;
        }
    }

    len = 0;
    outPtr = outStr;
    while ( !isspace( *bufPtr ) && *bufPtr != '\0' ) {
        len++;
        if ( len >= maxOutLen ) {
            *outStr = '\0';
            return USER_STRLEN_TOOLONG;
        }
        *outPtr = *bufPtr;
        outPtr++;
        bufPtr++;
    }

    *outPtr = '\0';
    *buf = bufPtr;

    return ( len );
}

int
isAllDigit( char * myStr ) {
    int c;

    while ( ( c = *myStr ) != '\0' ) {
        if ( isdigit( c ) == 0 ) {
            return ( 0 );
        }
        myStr++;
    }
    return ( 1 );
}

int
splitPathByKey( const char * srcPath, char * dir, char * file, char key ) {
    int pathLen, dirLen, fileLen;
    const char *srcPtr;

    pathLen = strlen( srcPath );

    if ( pathLen >= MAX_NAME_LEN ) {
        *dir = *file = '\0';
        return ( USER_STRLEN_TOOLONG );
    }
    else if ( pathLen <= 0 ) {
        *dir = '\0';
        *file = '\0';
        return ( 0 );
    }

    srcPtr = srcPath + pathLen - 1;

    while ( srcPtr != srcPath ) {
        if ( *srcPtr == key ) {
            dirLen = srcPtr - srcPath;
            strncpy( dir, srcPath, dirLen );
            dir[dirLen] = '\0';
            srcPtr ++;
            fileLen = pathLen - dirLen - 1;
            if ( fileLen > 0 ) {
                strncpy( file, srcPtr, fileLen );
                file[fileLen] = '\0';
            }
            else {
                *file = '\0';
            }
            return ( 0 );
        }
        srcPtr --;
    }

    /* Handle the special cases "/foo" */
    if ( *srcPtr == key ) {
        dirLen = 1;
        strncpy( dir, srcPath, dirLen );
        dir[dirLen] = '\0';
        srcPtr++;
        fileLen = pathLen - dirLen;
        if ( fileLen > 0 ) {
            strncpy( file, srcPtr, fileLen );
            file[fileLen] = '\0';
        }
        else {
            *file = '\0';
        }
        return ( 0 );
    }

    /* no Match. just copy srcPath to file */
    *dir = '\0';
    rstrcpy( file, srcPath, MAX_NAME_LEN );
    return ( SYS_INVALID_FILE_PATH );
}
int
trimWS( char * s ) {
    char *t;

    t = s;
    while ( isspace( *t ) ) {
        t++;
    }
    if ( s != t ) {
        memmove( s, t, strlen( t ) + 1 );
    }
    t = s + strlen( s ) - 1;
    while ( isspace( *t ) ) {
        t--;
    }
    *( t + 1 ) = '\0';

    /*TODO Please return appropriate value*/
    return ( 0 );
}
int
trimQuotes( char * s ) {
    char *t;

    if ( *s == '\'' || *s == '"' ) {
        memmove( s, s + 1, strlen( s + 1 ) + 1 );
        t = s + strlen( s ) - 1;
        if ( *t == '\'' || *t == '"' ) {
            *t = '\0';
        }
    }
    /* made it so that end quotes are removed only if quoted initially */
    /*TODO Please return appropriate value*/
    return ( 0 );
}

int
checkStringForSystem( char * inString ) {
    // .zZZZz. :: TODO - Make this do something. Sanitize strings to be
    // passed as arguments to system calls, I think?
    return( 0 );
}

/*
 * Check if inString is a valid email address.
 * This function only do a simple check that inString contains only a predefined set of characters.
 * It does not check the structure.
 * And this set of characters is a subset of that allowed in the RFCs.
 */
int
checkStringForEmailAddress( char * inString ) {
    char c;
    if ( inString == NULL ) {
        return( 0 );
    }
    c = *inString;
    while ( c != '\0' ) {
        if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
                ( c >= '0' && c <= '9' ) ||  c == ',' || c == '.' ||
                c == '/' || c == '-' || c == '+' || c == '*' || c == '_' || c == '@' ) {
        }
        else {
            return ( USER_INPUT_STRING_ERR );
        }
        c = *inString++;
    }
    return( 0 );
}
