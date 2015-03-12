/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* stringOpr - a number of string operations designed for secure string
 * copying.
 */

#include "stringOpr.hpp"
#include "rodsErrorTable.hpp"
#include "rodsLog.hpp"
#include <string>
#include "boost/regex.hpp"

char *rmemmove( void *dest, const void *src, int strLen, int maxLen ) {

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

char *rmemcpy( void *dest, const void *src, int strLen, int maxLen ) {

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
    // snprintf with logging on fail

    if ( dest == NULL || src == NULL ) {
        return NULL;
    }
    int status = snprintf( dest, maxLen, "%s", src );

    if ( status >= 0 && status < maxLen ) {
        return dest;
    }
    else if ( status >= 0 ) {
        rodsLog( LOG_ERROR,
                 "rstrcpy not enough space in dest, slen:%d, maxLen:%d",
                 status, maxLen );
        return NULL;
    }
    else {
        rodsLog( LOG_ERROR, "rstrcpy encountered an encoding error." );
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
        return NULL;
    }

    return strncat( dest, src, slen );
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
        return NULL;
    }

    return strncat( dest, src, slen );
}

int
rSplitStr( const char *inStr, char* outStr1, size_t maxOutLen1,
           char* outStr2, size_t maxOutLen2, char key ) {
    std::string base_string( inStr );
    size_t index_of_first_key = base_string.find( key );
    if ( std::string::npos == index_of_first_key ) {
        index_of_first_key = base_string.size();
    }
    strncpy( outStr1, base_string.substr( 0, index_of_first_key ).c_str(), maxOutLen1 );
    if ( maxOutLen1 > 0 ) {
        outStr1[ maxOutLen1 - 1 ] = '\0';
    }
    if ( index_of_first_key >= maxOutLen1 ) {
        return USER_STRLEN_TOOLONG;
    }

    /* copy the second str */
    size_t copy_start = base_string.size() == index_of_first_key ? base_string.size() : index_of_first_key + 1;
    if ( rstrcpy( outStr2, base_string.substr( copy_start ).c_str(), maxOutLen2 ) == NULL ) {
        return USER_STRLEN_TOOLONG;
    }
    return 0;
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

    return len;
}

int
isAllDigit( const char * myStr ) {
    char c;

    while ( ( c = *myStr ) != '\0' ) {
        if ( isdigit( c ) == 0 ) {
            return 0;
        }
        myStr++;
    }
    return 1;
}

int
splitPathByKey( const char * srcPath, char * dir, size_t maxDirLen,
                char * file, size_t maxFileLen, char key ) {
    std::string srcPathString( srcPath );

    if ( maxDirLen == 0 || maxFileLen == 0 ) {
        rodsLog( LOG_ERROR, "splitPathByKey called with buffers of size 0" );
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( srcPathString.size() == 0 ) {
        *dir = '\0';
        *file = '\0';
        return 0;
    }

    size_t index_of_last_key = srcPathString.rfind( key );
    if ( std::string::npos == index_of_last_key ) {
        *dir = '\0';
        rstrcpy( file, srcPathString.c_str(), maxFileLen );
        return SYS_INVALID_FILE_PATH;
    }

    // If dir is the root directory, we want to return the single-character
    // string consisting of the key, NOT the empty string.
    std::string dirPathString = srcPathString.substr( 0, std::max< size_t >( index_of_last_key, 1 ) );
    std::string filePathString = srcPathString.substr( index_of_last_key + 1 ) ;

    rstrcpy( dir, dirPathString.c_str(), maxDirLen );
    rstrcpy( file, filePathString.c_str(), maxFileLen );

    if ( dirPathString.size() >= maxDirLen || filePathString.size() >= maxFileLen ) {
        rodsLog( LOG_ERROR, "splitPathByKey called with buffers of insufficient size" );
        return USER_STRLEN_TOOLONG;
    }

    return 0;

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
    return 0;
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
    return 0;
}

int
checkStringForSystem( const char * inString ) {
    if ( inString == NULL ) {
        return 0;
    }
    if ( boost::regex_match( inString, boost::regex( "[a-zA-Z0-9,./ ]*" ) ) ) {
        return 0;
    }
    return USER_INPUT_STRING_ERR;
}

/*
 * Check if inString is a valid email address.
 * This function only do a simple check that inString contains only a predefined set of characters.
 * It does not check the structure.
 * And this set of characters is a subset of that allowed in the RFCs.
 */
int
checkStringForEmailAddress( const char * inString ) {
    if ( inString == NULL ) {
        return 0;
    }
    if ( boost::regex_match( inString, boost::regex( "[-a-zA-Z0-9,./+*_@]*" ) ) ) {
        return 0;
    }
    return USER_INPUT_STRING_ERR;
}
