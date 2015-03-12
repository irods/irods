/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* packStruct.c - routine for pack and unfolding C struct
 */


#include "packStruct.hpp"
#include "rodsLog.hpp"
#include "rcGlobalExtern.hpp"
#include "base64.hpp"
#include "rcMisc.hpp"

#include "irods_pack_table.hpp"
#include <iostream>
#include <string>

int
packStruct( void *inStruct, bytesBuf_t **packedResult, const char *packInstName,
            const packInstructArray_t *myPackTable, int packFlag, irodsProt_t irodsProt ) {
    int status;
    packItem_t rootPackedItem;
    packedOutput_t packedOutput;
    void *inPtr;

    if ( inStruct == NULL || packedResult == NULL || packInstName == NULL ) {
        rodsLog( LOG_ERROR,
                 "packStruct: Input error. One of the input is NULL" );
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    /* Initialize the packedOutput */

    initPackedOutput( &packedOutput, MAX_PACKED_OUT_ALLOC_SZ );

    inPtr = inStruct;
    memset( &rootPackedItem, 0, sizeof( rootPackedItem ) );
    rootPackedItem.name = strdup( packInstName );
    status = packChildStruct( &inPtr, &packedOutput, &rootPackedItem,
                              myPackTable, 1, packFlag, irodsProt, NULL );

    if ( status < 0 ) {
        free( rootPackedItem.name );
        return status;
    }

    if ( irodsProt == XML_PROT ) {
        char *outPtr;
        /* add a NULL termination */
        extendPackedOutput( &packedOutput, 1, ( void ** )( static_cast< void * >( &outPtr ) ) );
        *outPtr = '\0';
        if ( getRodsLogLevel() >= LOG_DEBUG2 ) {
            printf( "packed XML: \n%s\n", ( char * ) packedOutput.bBuf->buf );
        }
    }
    *packedResult = packedOutput.bBuf;
    free( rootPackedItem.name );
    return 0;
}

int
unpackStruct( void *inPackedStr, void **outStruct, const char *packInstName,
              const packInstructArray_t *myPackTable, irodsProt_t irodsProt ) {
    int status;
    packItem_t rootPackedItem;
    packedOutput_t unpackedOutput;
    void *inPtr;

    if ( inPackedStr == NULL || outStruct == NULL || packInstName == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackStruct: Input error. One of the input is NULL" );
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    /* Initialize the unpackedOutput */

    initPackedOutput( &unpackedOutput, PACKED_OUT_ALLOC_SZ );

    inPtr = inPackedStr;
    memset( &rootPackedItem, 0, sizeof( rootPackedItem ) );
    rootPackedItem.name = strdup( packInstName );
    status = unpackChildStruct( &inPtr, &unpackedOutput, &rootPackedItem,
                                myPackTable, 1, irodsProt, NULL );

    if ( status < 0 ) {
        free( rootPackedItem.name );
        return status;
    }

    *outStruct = unpackedOutput.bBuf->buf;
    free( unpackedOutput.bBuf );
    free( rootPackedItem.name );

    return 0;
}

int
parsePackInstruct( const char *packInstruct, packItem_t **packItemHead ) {
    char buf[MAX_PI_LEN];
    packItem_t *myPackItem = NULL;
    packItem_t *prevPackItem = NULL;
    const char *inptr = packInstruct;
    int gotTypeCast = 0;
    int gotItemName = 0;
    int outLen = 0;

    while ( ( outLen = copyStrFromPiBuf( &inptr, buf, 0 ) ) > 0 ) {
        if ( myPackItem == NULL ) {
            myPackItem = ( packItem_t* )malloc( sizeof( packItem_t ) );
            memset( myPackItem, 0, sizeof( packItem_t ) );
        }

        if ( strcmp( buf,  ";" ) == 0 ) { /* delimiter */
            if ( gotTypeCast == 0 && gotItemName == 0 ) {
                /* just an extra ';' */
                continue;
            }
            else if ( gotTypeCast > 0 && gotItemName == 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: No varName for %s", packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            /* queue it */
            outLen = 0;
            gotTypeCast = 0;
            gotItemName = 0;
            if ( prevPackItem != NULL ) {
                prevPackItem->next = myPackItem;
                myPackItem->prev = prevPackItem;
            }
            else {
                *packItemHead = myPackItem;
            }
            prevPackItem = myPackItem;
            myPackItem = NULL;
            continue;
        }
        else if ( strcmp( buf,  "%" ) == 0 ) {  /* int dependent */
            /* int dependent type */
            if ( gotTypeCast > 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: % position error for %s", packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            myPackItem->typeInx = ( packTypeInx_t )packTypeLookup( buf );
            if ( myPackItem->typeInx < 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: packTypeLookup failed for %s", buf );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            gotTypeCast = 1;
            outLen = copyStrFromPiBuf( &inptr, buf, 1 );
            if ( outLen <= 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: ? No variable following ? for %s",
                         packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            myPackItem->name = strdup( buf );
            gotItemName = 1;
            continue;
        }
        else if ( strcmp( buf,  "?" ) == 0 ) {  /* dependent */
            /* dependent type */
            if ( gotTypeCast > 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: ? position error for %s", packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            myPackItem->typeInx = ( packTypeInx_t )packTypeLookup( buf );
            if ( myPackItem->typeInx < 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: packTypeLookup failed for %s", buf );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            gotTypeCast = 1;
            outLen = copyStrFromPiBuf( &inptr, buf, 0 );
            if ( outLen <= 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: ? No variable following ? for %s",
                         packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            rstrcpy( myPackItem->strValue, buf, NAME_LEN );
            continue;
        }
        else if ( strcmp( buf,  "*" ) == 0 ) {  /* pointer */
            myPackItem->pointerType = A_POINTER;
            if ( gotTypeCast == 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: * position error for %s", packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            continue;
        }
        else if ( strcmp( buf, "#" ) == 0 ) {  /* no pack pointer */
            myPackItem->pointerType = NO_PACK_POINTER;
            if ( gotTypeCast == 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: * position error for %s", packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            continue;
        }
        else if ( strcmp( buf,  "$" ) == 0 ) {  /* pointer but don't free */
            myPackItem->pointerType = NO_FREE_POINTER;
            if ( gotTypeCast == 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: * position error for %s", packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            continue;
        }
        else if ( gotTypeCast == 0 ) {  /* a typeCast */
            myPackItem->typeInx = ( packTypeInx_t )packTypeLookup( buf );
            if ( myPackItem->typeInx < 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: packTypeLookup failed for %s in %s",
                         buf, packInstruct );
                free( myPackItem );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            gotTypeCast = 1;
            continue;
        }
        else if ( gotTypeCast == 1 && gotItemName == 0 ) {    /* item name */
            myPackItem->name = strdup( buf );
            gotItemName = 1;
            continue;
        }
        else {
            rodsLog( LOG_ERROR,
                     "parsePackInstruct: too many string around %s in %s",
                     buf, packInstruct );
            free( myPackItem );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
    }
    if ( myPackItem != NULL ) {
        rodsLog( LOG_ERROR,
                 "parsePackInstruct: Pack Instruction %s not properly terminated",
                 packInstruct );
        free( myPackItem );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    return 0;
}

/* copy the next string from the inBuf to putBuf and advance the inBuf pointer.
 * special char '*', ';' and '?' will be returned as a string.
 */
int
copyStrFromPiBuf( const char **inBuf, char *outBuf, int dependentFlag ) {
    const char *inPtr = *inBuf;
    char *outPtr = outBuf;
    int outLen = 0;
    int c;

    while ( ( c = *inPtr ) != '\0' ) {
        if ( dependentFlag > 0 ) {
            /* read until ';' */
            if ( c == ';' ) {
                break;
            }
            else {
                if ( outLen > 0 || !isspace( c ) ) {
                    *outPtr = c;
                    outPtr++;
                    outLen++;
                }
                inPtr++;
            }
        }
        else if ( isspace( c ) ) {
            inPtr++;
            if ( outLen > 0 ) {
                /* we got something */
                break;
            }
        }
        else if ( c == '*' || c == ';' || c == '?' || c == '$' ) {
            if ( outLen > 0 ) {
                /* what we have in outBuf is type cast. don't advance
                 * inPtr because we'll pick up next time
                 */
                break;
            }
            else {
                *outPtr = c;
                outPtr++;
                inPtr++;
                outLen++;
                break;
            }
        }
        else {          /* just a normal char */
            *outPtr = c;
            outPtr++;
            inPtr++;
            outLen++;
        }
    }

    *outPtr = '\0';
    *inBuf = inPtr;

    return outLen;
}

int
packTypeLookup( char *typeName ) {
    /*TODO: i and return type should be reconsidered as of
      packTypeInx_t */
    int i;

    for ( i = 0; i < NumOfPackTypes; i++ ) {
        if ( strcmp( typeName, packTypeTable[i].name ) == 0 ) {
            return i;
        }
    }
    return -1;
}

void *
alignAddrToBoundary( void *ptr, int boundary ) {
#if defined(_LP64) || defined(__LP64__)
    rodsLong_t b, m;
    b = ( rodsLong_t ) ptr;

    m = b % boundary;

    if ( m == 0 ) {
        return ptr;
    }

    /* rodsLong_t is signed */
    if ( m < 0 ) {
        m = boundary + m;
    }

    return ( void* )( ( char * ) ptr + boundary - m );

#else
    uint b, m;
    b = ( uint ) ptr;

    m = b % boundary;

    if ( m == 0 ) {
        return ptr;
    }

    return ( void* )( ( char * ) ptr + boundary - m );

#endif
}

/* alignInt - align the inout pointer ptr to an int, char or void
 * boundary in a struct */

void *
alignInt( void *ptr ) {
    return alignAddrToBoundary( ptr, 4 );
}

/* alignInt16 - align the inout pointer ptr to an int16
 * boundary in a struct */

void *
alignInt16( void *ptr ) {
    return alignAddrToBoundary( ptr, 2 );
}

/* alignDoubleAddr - align the inout pointer ptr to a longlong ,
 * boundary in a struct */

void *
alignDouble( void *ptr ) {
#if defined(linux_platform) || defined(windows_platform)   /* no need align at 64 bit boundary for linux */
    /* By Bing on 5-29-08: Mike and I found that
       Windows 32-bit OS is aligned with 4. */

#if defined(_LP64) || defined(__LP64__)
    return alignAddrToBoundary( ptr, 8 );
#else
    return alignAddrToBoundary( ptr, 4 );
#endif // LP64

#else   /* non-linux_platform || non-windows*/
    return alignAddrToBoundary( ptr, 8 );
#endif  /* linux_platform | windows */
}

/* align pointer address in a struct */

void *ialignAddr( void *ptr ) {
#if defined(_LP64) || defined(__LP64__)
    return alignDouble( ptr );
#else
    return alignInt( ptr );
#endif
}

int
initPackedOutput( packedOutput_t *packedOutput, int len ) {
    memset( packedOutput, 0, sizeof( packedOutput_t ) );

    packedOutput->bBuf = ( bytesBuf_t * )malloc( sizeof( bytesBuf_t ) );
    packedOutput->bBuf->buf = malloc( len );
    packedOutput->bBuf->len = 0;
    packedOutput->bufSize = len;

    return 0;
}

int
initPackedOutputWithBuf( packedOutput_t *packedOutput, void *buf, int len ) {
    memset( packedOutput, 0, sizeof( packedOutput_t ) );

    packedOutput->bBuf = ( bytesBuf_t * )malloc( sizeof( bytesBuf_t ) );
    packedOutput->bBuf->buf = buf;
    packedOutput->bBuf->len = 0;
    packedOutput->bufSize = len;

    return 0;
}

int
resolvePackedItem( packItem_t *myPackedItem, void **inPtr, packOpr_t packOpr ) {
    int status;

    status = iparseDependent( myPackedItem );
    if ( status < 0 ) {
        return status;
    }

    status = resolveDepInArray( myPackedItem );

    if ( status < 0 ) {
        return status;
    }

    /* set up the pointer */

    if ( myPackedItem->pointerType > 0 ) {
        if ( packOpr == PACK_OPR ) {
            /* align the address */
            *inPtr = ialignAddr( *inPtr );
            if ( *inPtr != NULL ) {
                myPackedItem->pointer = ( **( void *** )inPtr );
                /* advance the pointer */
                *inPtr = ( void * )( ( char * ) * inPtr + sizeof( void * ) );
            }
            else {
                myPackedItem->pointer = NULL;
            }
        }
    }
    return 0;
}

int
iparseDependent( packItem_t *myPackedItem ) {
    int status;

    if ( myPackedItem->typeInx == PACK_DEPENDENT_TYPE ) {
        status =  resolveStrInItem( myPackedItem );
    }
    else if ( myPackedItem->typeInx == PACK_INT_DEPENDENT_TYPE ) {
        status =  resolveIntDepItem( myPackedItem );
    }
    else {
        status = 0;
    }
    return status;
}

int
resolveIntDepItem( packItem_t *myPackedItem ) {
    const char *tmpPtr = 0;
    char *bufPtr = 0;
    char buf[MAX_NAME_LEN], myPI[MAX_NAME_LEN], *pfPtr = NULL;
    int endReached = 0, c = 0;
    int outLen = 0;
    int keyVal = 0, status = 0;
    packItem_t *newPackedItem = 0, *tmpPackedItem = 0, *lastPackedItem = 0;

    tmpPtr = myPackedItem->name;
    bufPtr = buf;
    outLen = 0;
    endReached = 0;

    /* get the key */

    while ( ( c = *tmpPtr ) != '\0' ) {
        if ( c == ' ' || c == '\t' || c == '\n' || c == ':' ) {
            /* white space */
            if ( outLen == 0 ) {
                tmpPtr ++;
                continue;
            }
            else if ( endReached == 0 ) {
                *bufPtr = '\0';
                endReached = 1;
            }
            if ( c == ':' ) {
                tmpPtr ++;
                break;
            }
        }
        else if ( c == ';' ) {
            rodsLog( LOG_ERROR,
                     "resolveIntDepItem: end reached while parsing for key: %s",
                     myPackedItem->name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
        else {      /* just normal char */
            *bufPtr = *tmpPtr;
            outLen ++;
            bufPtr ++;
        }
        tmpPtr++;
    }

    keyVal = resolveIntInItem( buf, myPackedItem );
    if ( keyVal == SYS_PACK_INSTRUCT_FORMAT_ERR ) {
        rodsLog( LOG_ERROR,
                 "resolveIntDepItem: resolveIntInItem error for %s", buf );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    bufPtr = buf;

    while ( ( c = *tmpPtr ) != '\0' ) {
        if ( c == ',' || c == '=' ) {
            *bufPtr = '\0';
            if ( strstr( buf, "default" ) != 0 || atoi( buf ) == keyVal ) {
                /* get the PI */
                if ( c == ',' ) {
                    /* skip to = */
                    while ( 1 ) {
                        c = *tmpPtr;
                        if ( c == '=' ) {
                            break;
                        }
                        else if ( c == ';' || c == ':' || c == '\0' ) {
                            rodsLog( LOG_ERROR,
                                     "resolveIntDepItem: end reached with no PI%s",
                                     myPackedItem->name );
                            return SYS_PACK_INSTRUCT_FORMAT_ERR;
                        }
                        tmpPtr++;
                    }
                }
                tmpPtr++;
                myPI[0] = '\0';
                pfPtr = myPI;
                while ( 1 ) {
                    c = *tmpPtr;
                    if ( c == '\0' || c == ':' || c == ';' ) {
                        *pfPtr = ';';
                        pfPtr++;
                        *pfPtr = '\0';
                        break;
                    }
                    else {
                        *pfPtr = *tmpPtr;
                        tmpPtr ++;
                        pfPtr ++;
                    }
                }
                break;
            }
            bufPtr = buf;       /* reset buffer */
            if ( c == '=' ) {
                tmpPtr++;
                /* skip to next setting */
                while ( 1 ) {
                    c = *tmpPtr;
                    if ( c == ':' ) {
                        break;
                    }
                    if ( c == '\0' || c == ';' ) {
                        rodsLog( LOG_ERROR,
                                 "resolveIntDepItem: end reached with no PI%s",
                                 myPackedItem->name );
                        return SYS_PACK_INSTRUCT_FORMAT_ERR;
                    }
                    tmpPtr++;
                }
            }
        }
        else {          /* normal char */
            *bufPtr = *tmpPtr;
            bufPtr ++;
        }
        tmpPtr++;
    }

    newPackedItem = NULL;
    status = parsePackInstruct( myPI, &newPackedItem );

    if ( status < 0 ) {
        freePackedItem( newPackedItem );
        rodsLog( LOG_ERROR,
                 "resolveIntDepItem: parsePackFormat error for =%s", myPI );
        return status;
    }

    /* reset the link and switch myPackedItem<->newType */
    free( myPackedItem->name );
    tmpPackedItem = newPackedItem;
    while ( tmpPackedItem != NULL ) {
        lastPackedItem = tmpPackedItem;
        tmpPackedItem = tmpPackedItem->next;
    }

    if ( newPackedItem != lastPackedItem ) {
        newPackedItem->next->prev = myPackedItem;
        if ( myPackedItem->next != NULL ) {
            myPackedItem->next->prev = lastPackedItem;
        }
    }
    newPackedItem->prev = myPackedItem->prev;
    lastPackedItem->next = myPackedItem->next;

    *myPackedItem = *newPackedItem;

    free( newPackedItem );

    return 0;
}

int
resolveIntInItem( const char *name, packItem_t *myPackedItem ) {
    packItem_t *tmpPackedItem;
    int i;

    if ( isAllDigit( name ) ) {
        return atoi( name );
    }

    /* check the current item chain first */

    tmpPackedItem = myPackedItem->prev;

    while ( tmpPackedItem != NULL ) {
        if ( strcmp( name, tmpPackedItem->name ) == 0 &&
                packTypeTable[tmpPackedItem->typeInx].number == PACK_INT_TYPE ) {
            return tmpPackedItem->intValue;
        }
        if ( tmpPackedItem->prev == NULL && tmpPackedItem->parent != NULL ) {
            tmpPackedItem = tmpPackedItem->parent;
        }
        else {
            tmpPackedItem = tmpPackedItem->prev;
        }
    }

    /* Try the Rods Global table */

    i = 0;
    while ( strcmp( PackConstantTable[i].name, PACK_TABLE_END_PI ) != 0 ) {
        /* not the end */
        if ( strcmp( PackConstantTable[i].name, name ) == 0 ) {
            return PackConstantTable[i].value;
        }
        i++;
    }

    return SYS_PACK_INSTRUCT_FORMAT_ERR;
}

int
resolveStrInItem( packItem_t *myPackedItem ) {
    packItem_t *tmpPackedItem;
    char *name;

    name = myPackedItem->strValue;
    /* check the current item chain first */

    tmpPackedItem = myPackedItem->prev;

    while ( tmpPackedItem != NULL ) {
        if ( strcmp( name, tmpPackedItem->name ) == 0 &&
                packTypeTable[tmpPackedItem->typeInx].number == PACK_PI_STR_TYPE ) {
            break;
        }
        if ( tmpPackedItem->prev == NULL && tmpPackedItem->parent != NULL ) {
            tmpPackedItem = tmpPackedItem->parent;
        }
        else {
            tmpPackedItem = tmpPackedItem->prev;
        }
    }

    if ( tmpPackedItem == NULL || strlen( tmpPackedItem->strValue ) == 0 ) {
        rodsLog( LOG_ERROR,
                 "resolveStrInItem: Cannot resolve %s in %s",
                 name, myPackedItem->name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    myPackedItem->typeInx = PACK_STRUCT_TYPE;
    free( myPackedItem->name );
    myPackedItem->name = strdup( tmpPackedItem->strValue );

    return 0;
}

const void *
matchPackInstruct( char *name, const packInstructArray_t *myPackTable ) {
    int i;

    if ( myPackTable != NULL ) {
        i = 0;
        while ( strcmp( myPackTable[i].name, PACK_TABLE_END_PI ) != 0 ) {
            /* not the end */
            if ( strcmp( myPackTable[i].name, name ) == 0 ) {
                return myPackTable[i].packInstruct;
            }
            i++;
        }
    }

    /* Try the Rods Global table */

    i = 0;
    while ( strcmp( RodsPackTable[i].name, PACK_TABLE_END_PI ) != 0 ) {
        /* not the end */
        if ( strcmp( RodsPackTable[i].name, name ) == 0 ) {
            return RodsPackTable[i].packInstruct;
        }
        i++;
    }

    /* Try the API table */
#if 1
    irods::pack_entry_table& pk_tbl =  irods::get_pack_table();
    irods::pack_entry_table::iterator itr = pk_tbl.find( name );
    if ( itr != pk_tbl.end() ) {
        //return ( void* )itr->second.c_str();
        return ( void* )itr->second.packInstruct.c_str();
    }
#else
    i = 0;
    while ( strcmp( ApiPackTable[i].name, PACK_TABLE_END_PI ) != 0 ) {
        /* not the end */
        if ( strcmp( ApiPackTable[i].name, name ) == 0 ) {
            return ApiPackTable[i].packInstruct;
        }
        i++;
    }
#endif
    rodsLog( LOG_ERROR,
             "matchPackInstruct: Cannot resolve %s",
             name );

    return NULL;
}

int
resolveDepInArray( packItem_t *myPackedItem ) {
    myPackedItem->dim = myPackedItem->hintDim = 0;
    char openSymbol = '\0';         // either '(', '[', or '\0' depending on whether we are
    // in a parenthetical or bracketed expression, or not

    std::string buffer;
    for ( size_t nameIndex = 0; '\0' != myPackedItem->name[ nameIndex ]; nameIndex++ ) {
        char c = myPackedItem->name[ nameIndex ];
        if ( '[' == c || '(' == c ) {
            if ( openSymbol ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: got %c inside %c for %s",
                         c, openSymbol, myPackedItem->name );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            else if ( ( '[' == c && myPackedItem->dim >= MAX_PACK_DIM ) ||
                      ( '(' == c && myPackedItem->hintDim >= MAX_PACK_DIM ) ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: dimension of %s larger than %d",
                         myPackedItem->name, MAX_PACK_DIM );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            openSymbol = c;
            myPackedItem->name[ nameIndex ] = '\0';  // isolate the name. may be used later
            buffer.clear();
        }
        else if ( ']' == c || ')' == c ) {
            if ( ( ']' == c && '[' != openSymbol ) ||
                    ( ')' == c && '(' != openSymbol ) ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: Got %c without %c for %s",
                         c, ( ']' == c ) ? '[' : '(', myPackedItem->name );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            else if ( buffer.empty() ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: Empty %c%c in %s",
                         ( ']' == c ) ? '[' : '(', c, myPackedItem->name );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            openSymbol = '\0';

            int& dimSize = ( ']' == c ) ?
                           myPackedItem->dimSize[myPackedItem->dim++] :
                           myPackedItem->hintDimSize[myPackedItem->hintDim++];
            if ( ( dimSize = resolveIntInItem( buffer.c_str(), myPackedItem ) ) < 0 ) {
                rodsLog( LOG_ERROR, "resolveDepInArray:resolveIntInItem error for %s, intName=%s",
                         myPackedItem->name, buffer.c_str() );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
        }
        else if ( openSymbol ) {
            buffer += c;
        }
    }
    return 0;
}

int
packNonpointerItem( packItem_t *myPackedItem, void **inPtr,
                    packedOutput_t *packedOutput, const packInstructArray_t *myPackTable,
                    int packFlag, irodsProt_t irodsProt ) {
    int numElement;
    int elementSz;
    int typeInx;
    int myTypeNum;
    int i, status;

    typeInx = myPackedItem->typeInx;
    numElement = getNumElement( myPackedItem );
    elementSz = packTypeTable[typeInx].size;
    myTypeNum = packTypeTable[typeInx].number;

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        /* no need to align inPtr */

        status = packChar( inPtr, packedOutput, numElement * elementSz,
                           myPackedItem, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        break;
    case PACK_STR_TYPE:
    case PACK_PI_STR_TYPE: {
        /* no need to align inPtr */
        /* the size of the last dim is the max length of the string. Don't
         * want to pack the entire length, just to end of the string including
         * the NULL.
         */
        int maxStrLen, numStr, myDim;

        myDim = myPackedItem->dim;
        if ( myDim <= 0 ) {
            /* NULL pterminated */
            maxStrLen = -1;
            numStr = 1;
        }
        else {
            maxStrLen = myPackedItem->dimSize[myDim - 1];
            numStr = numElement / maxStrLen;
        }

        /* save the str */
        if ( numStr == 1 && myTypeNum == PACK_PI_STR_TYPE ) {
            snprintf( myPackedItem->strValue, sizeof( myPackedItem->strValue ), "%s", ( char* )*inPtr );
        }

        for ( i = 0; i < numStr; i++ ) {
            status = packString( inPtr, packedOutput, maxStrLen, myPackedItem,
                                 irodsProt );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "packNonpointerItem:strlen %d of %s > dim size %d,content:%s",
                         strlen( ( char* )*inPtr ), myPackedItem->name, maxStrLen, *inPtr );
                return status;
            }
        }
        break;
    }
    case PACK_INT_TYPE:
        /* align inPtr to 4 bytes boundary. Will not align outPtr */

        *inPtr = alignInt( *inPtr );

        status = packInt( inPtr, packedOutput, numElement, myPackedItem,
                          irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem->intValue = status;
        break;

    case PACK_INT16_TYPE:
        /* align inPtr to 2 bytes boundary. Will not align outPtr */

        *inPtr = alignInt16( *inPtr );

        status = packInt16( inPtr, packedOutput, numElement, myPackedItem,
                            irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem->intValue = status;
        break;

    case PACK_DOUBLE_TYPE:
        /* align inPtr to 8 bytes boundary. Will not align outPtr */
#if defined(osx_platform) || (defined(solaris_platform) && defined(i86_hardware))
        /* osx does not align */
        *inPtr = alignInt( *inPtr );
#else
        *inPtr = alignDouble( *inPtr );
#endif

        status = packDouble( inPtr, packedOutput, numElement, myPackedItem,
                             irodsProt );
        if ( status < 0 ) {
            return status;
        }
        break;

    case PACK_STRUCT_TYPE:
        /* no need to align boundary for struct */

        status = packChildStruct( inPtr, packedOutput, myPackedItem,
                                  myPackTable, numElement, packFlag, irodsProt, NULL );

        if ( status < 0 ) {
            return status;
        }

        break;

    default:
        rodsLog( LOG_ERROR,
                 "packNonpointerItem: Unknow type %d - %s ",
                 myTypeNum, myPackedItem->name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    return 0;
}

int
packPointerItem( packItem_t *myPackedItem, packedOutput_t *packedOutput,
                 const packInstructArray_t *myPackTable, int packFlag, irodsProt_t irodsProt ) {
    int numElement;     /* number of elements pointed to by one pointer */
    int numPointer;     /* number of pointers in the array of pointer */
    int elementSz;
    int typeInx;
    int myTypeNum;
    int i, j, status;
    void **pointerArray;
    void* singletonArray[1];
    void *pointer;      /* working pointer */
    void *origPtr;      /* for the purpose of freeing the original ptr */
    int myDim;

    /* if it is a NULL pointer, just pack a NULL_PTR_PACK_STR string */
    if ( myPackedItem->pointer == NULL ) {
        if ( irodsProt == NATIVE_PROT ) {
            packNullString( packedOutput );
        }
        return 0;
    }

    numElement = getNumHintElement( myPackedItem );
    myDim = myPackedItem->dim;
    typeInx = myPackedItem->typeInx;
    numPointer = getNumElement( myPackedItem );
    elementSz = packTypeTable[typeInx].size;
    myTypeNum = packTypeTable[typeInx].number;

    /* pointer is already aligned */
    if ( myDim == 0 ) {
        pointerArray = singletonArray;
        pointerArray[0] = myPackedItem->pointer;
        if ( myTypeNum == PACK_PI_STR_TYPE ) {
            /* save the str */
            snprintf( myPackedItem->strValue, sizeof( myPackedItem->strValue ),
                      "%s", ( char* )myPackedItem->pointer );
        }
    }
    else {
        pointerArray = ( void ** ) myPackedItem->pointer;
    }

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            if ( myPackedItem->pointerType == NO_PACK_POINTER ) {
                status = packNopackPointer( &pointer, packedOutput,
                                            numElement * elementSz, myPackedItem, irodsProt );
            }
            else {
                status = packChar( &pointer, packedOutput,
                                   numElement * elementSz, myPackedItem, irodsProt );
            }
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem->pointerType == A_POINTER ) {
                free( origPtr );
            }
            if ( status < 0 ) {
                return status;
            }
        }
        if ( ( packFlag & FREE_POINTER )  &&
                myPackedItem->pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( pointerArray );
        }
        break;
    case PACK_STR_TYPE:
    case PACK_PI_STR_TYPE: {
        /* the size of the last dim is the max length of the string. Don't
         * want to pack the entire length, just to end of the string including
         * the NULL.
         */
        int maxStrLen, numStr, myHintDim;
        myHintDim = myPackedItem->hintDim;
        if ( myHintDim <= 0 ) {   /* just a pointer to a null terminated str */
            maxStrLen = -1;
            numStr = 1;
        }
        else {
            /* the size of the last hint dimension */
            maxStrLen = myPackedItem->hintDimSize[myHintDim - 1];
            if ( numElement <= 0 || maxStrLen <= 0 ) {
                return 0;
            }
            numStr = numElement / maxStrLen;
        }

        for ( j = 0; j < numPointer; j++ ) {
            origPtr = pointer = pointerArray[j];

            for ( i = 0; i < numStr; i++ ) {
                status = packString( &pointer, packedOutput, maxStrLen,
                                     myPackedItem, irodsProt );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                             "packPointerItem: strlen of %s > dim size, content: %s ",
                             myPackedItem->name, pointer );
                    return status;
                }
            }
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem->pointerType == A_POINTER ) {
                free( origPtr );
            }
        }
        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem->pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( pointerArray );
        }
        break;
    }
    case PACK_INT_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            status = packInt( &pointer, packedOutput, numElement,
                              myPackedItem, irodsProt );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem->pointerType == A_POINTER ) {
                free( origPtr );
            }
            if ( status < 0 ) {
                return status;
            }
        }

        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem->pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( pointerArray );
        }
        break;

    case PACK_INT16_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            status = packInt16( &pointer, packedOutput, numElement,
                                myPackedItem, irodsProt );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem->pointerType == A_POINTER ) {
                free( origPtr );
            }
            if ( status < 0 ) {
                return status;
            }
        }

        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem->pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( pointerArray );
        }
        break;

    case PACK_DOUBLE_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            status = packDouble( &pointer, packedOutput, numElement,
                                 myPackedItem, irodsProt );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem->pointerType == A_POINTER ) {
                free( origPtr );
            }
            if ( status < 0 ) {
                return status;
            }
        }

        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem->pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( pointerArray );
        }
        break;

    case PACK_STRUCT_TYPE:
        /* no need to align boundary for struct */

        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];
            status = packChildStruct( &pointer, packedOutput, myPackedItem,
                                      myPackTable, numElement, packFlag, irodsProt, NULL );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem->pointerType == A_POINTER ) {
                free( origPtr );
            }
            if ( status < 0 ) {
                return status;
            }
        }
        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem->pointerType == A_POINTER
                && numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( pointerArray );
        }
        break;

    default:
        rodsLog( LOG_ERROR,
                 "packNonpointerItem: Unknow type %d - %s ",
                 myTypeNum, myPackedItem->name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    return 0;
}

int
getNumElement( packItem_t *myPackedItem ) {
    int i;
    int numElement = 1;

    for ( i = 0; i < myPackedItem->dim; i++ ) {
        numElement *= myPackedItem->dimSize[i];
    }
    return numElement;
}

int
getNumHintElement( packItem_t *myPackedItem ) {
    int i;
    int numElement = 1;

    for ( i = 0; i < myPackedItem->hintDim; i++ ) {
        numElement *= myPackedItem->hintDimSize[i];
    }
    return numElement;
}

int
extendPackedOutput( packedOutput_t *packedOutput, int extLen, void **outPtr ) {
    int newOutLen;
    int newBufSize;
    void *oldBuf;

    newOutLen = packedOutput->bBuf->len + extLen;
    if ( newOutLen <= packedOutput->bufSize ) {
        *outPtr = ( void * )( ( char * ) packedOutput->bBuf->buf +
                              packedOutput->bBuf->len );
        return 0;
    }

    /* double the size */
    newBufSize = packedOutput->bufSize + packedOutput->bufSize;
    if ( newBufSize <= newOutLen ||
            packedOutput->bufSize > MAX_PACKED_OUT_ALLOC_SZ ) {
        newBufSize = newOutLen + PACKED_OUT_ALLOC_SZ;
    }

    oldBuf = packedOutput->bBuf->buf;

    packedOutput->bBuf->buf = malloc( newBufSize );
    packedOutput->bufSize = newBufSize;

    if ( packedOutput->bBuf->buf == NULL ) {
        rodsLog( LOG_ERROR,
                 "extendPackedOutput: error malloc of size %d", newBufSize );
        *outPtr = NULL;
        return SYS_MALLOC_ERR;
    }
    if ( packedOutput->bBuf->len > 0 ) {
        memcpy( packedOutput->bBuf->buf, oldBuf, packedOutput->bBuf->len );
    }
    *outPtr = ( void * )( ( char * ) packedOutput->bBuf->buf +
                          packedOutput->bBuf->len );

    free( oldBuf );

    /* set the remaing to 0 */

    memset( *outPtr, 0, newBufSize - packedOutput->bBuf->len );

    return 0;
}

int
packItem( packItem_t *myPackedItem, void **inPtr, packedOutput_t *packedOutput,
          const packInstructArray_t *myPackTable, int packFlag, irodsProt_t irodsProt ) {
    int status;

    status = resolvePackedItem( myPackedItem, inPtr, PACK_OPR );
    if ( status < 0 ) {
        return status;
    }
    if ( myPackedItem->pointerType > 0 ) {
        /* a pointer type */
        status = packPointerItem( myPackedItem, packedOutput,
                                  myPackTable, packFlag, irodsProt );
    }
    else {
        status = packNonpointerItem( myPackedItem, inPtr, packedOutput,
                                     myPackTable, packFlag, irodsProt );
    }

    return status;
}

int
packChar( void **inPtr, packedOutput_t *packedOutput, int len,
          packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    void *outPtr;

    if ( len <= 0 ) {
        return 0;
    }

    if ( irodsProt == XML_PROT ) {
        packXmlTag( myPackedItem, packedOutput, START_TAG_FL );
    }

    if ( irodsProt == XML_PROT &&
            packTypeTable[myPackedItem->typeInx].number == PACK_BIN_TYPE ) {
        /* a bin pack type. encode it */
        unsigned long outlen;
        int status;

        outlen = 2 * len + 10;
        extendPackedOutput( packedOutput, outlen, &outPtr );
        if ( *inPtr == NULL ) { /* A NULL pointer */
            /* Don't think we'll have this condition. just fill it with 0 */
            memset( outPtr, 0, len );
            packedOutput->bBuf->len += len;
        }
        else {
            status = base64_encode( ( const unsigned char * ) * inPtr, len,
                                    ( unsigned char * ) outPtr, &outlen );
            if ( status < 0 ) {
                return status;
            }
            *inPtr = ( void * )( ( char * ) * inPtr + len );
            packedOutput->bBuf->len += outlen;
        }
    }
    else {
        extendPackedOutput( packedOutput, len, &outPtr );
        if ( *inPtr == NULL ) { /* A NULL pointer */
            /* Don't think we'll have this condition. just fill it with 0 */
            memset( outPtr, 0, len );
        }
        else {
            memcpy( outPtr, *inPtr, len );
            *inPtr = ( void * )( ( char * ) * inPtr + len );
        }
        packedOutput->bBuf->len += len;
    }

    if ( irodsProt == XML_PROT ) {
        packXmlTag( myPackedItem, packedOutput, END_TAG_FL );
    }

    return 0;
}

int
packString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen,
            packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = packXmlString( inPtr, packedOutput, maxStrLen, myPackedItem );
    }
    else {
        status = packNatString( inPtr, packedOutput, maxStrLen );
    }

    return status;
}

int
packNatString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen ) {
    int myStrlen;
    char *outPtr;

    if ( *inPtr == NULL ) {
        myStrlen = 0;
    }
    else {
        myStrlen = strlen( ( char* ) * inPtr );
    }
    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    extendPackedOutput( packedOutput, myStrlen + 1, ( void ** )( static_cast< void * >( &outPtr ) ) );
    if ( myStrlen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( ( char* )outPtr, ( char* )*inPtr, myStrlen + 1 );
    }

    if ( maxStrLen > 0 ) {
        *inPtr = ( void * )( ( char * ) * inPtr + maxStrLen );
    }
    else {
        *inPtr = ( void * )( ( char * ) * inPtr + myStrlen + 1 );
    }

    packedOutput->bBuf->len += ( myStrlen + 1 );

    return 0;
}

int
packXmlString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen,
               packItem_t *myPackedItem ) {
    int myStrlen;
    char *outPtr;
    char *xmlStr = NULL;
    int xmlLen;
    char *myInPtr;

    if ( *inPtr == NULL ) {
        myStrlen = 0;
        xmlLen = 0;
    }
    else {
        myStrlen = strlen( ( char* ) * inPtr );
        myInPtr = ( char * ) * inPtr;
        xmlLen = strToXmlStr( ( char * ) myInPtr, &xmlStr );
    }
    if ( NULL == xmlStr ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "packXmlString :: null xmlStr" );
        return -1;
    }

    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        if ( xmlStr != myInPtr ) {
            free( xmlStr );
        }
        return USER_PACKSTRUCT_INPUT_ERR;
    }
    packXmlTag( myPackedItem, packedOutput, START_TAG_FL );

    extendPackedOutput( packedOutput, xmlLen + 1, ( void ** )( static_cast< void * >( &outPtr ) ) );
    if ( xmlLen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( ( char* )outPtr, xmlStr, xmlLen + 1 );
    }

    if ( maxStrLen > 0 ) {
        *inPtr = ( void * )( ( char * ) * inPtr + maxStrLen );
    }
    else {
        *inPtr = ( void * )( ( char * ) * inPtr + xmlLen + 1 );
    }

    packedOutput->bBuf->len += ( xmlLen );
    packXmlTag( myPackedItem, packedOutput, END_TAG_FL );
    if ( xmlStr != myInPtr ) {
        free( xmlStr );
    }
    return 0;
}

int
strToXmlStr( char *inStr, char **outXmlStr ) {
    char *tmpPtr = 0;
    char *inPtr = 0;
    char *outPtr = 0;
    int cpLen = 0;

    *outXmlStr = NULL;
    if ( inStr == NULL ) {
        return 0;
    }

    inPtr = tmpPtr = inStr;

    while ( 1 ) {
        if ( *tmpPtr == '&' ) {
            if ( *outXmlStr == NULL ) {
                *outXmlStr = outPtr = ( char* )malloc( 5 * strlen( inPtr ) + 6 );
            }
            cpLen = tmpPtr - inPtr;
            strncpy( outPtr, inPtr, cpLen );
            outPtr += cpLen;
            /* strcat (outPtr, "&amp;"); */
            rstrcpy( outPtr, "&amp;", 6 );
            outPtr += 5;
            inPtr += cpLen + 1;
        }
        else if ( *tmpPtr == '<' ) {
            if ( *outXmlStr == NULL ) {
                *outXmlStr = outPtr = ( char* )malloc( 5 * strlen( inPtr ) + 6 );
            }
            cpLen = tmpPtr - inPtr;
            strncpy( outPtr, inPtr, cpLen );
            outPtr += cpLen;
            /* strcat (outPtr, "&lt;"); */
            rstrcpy( outPtr, "&lt;", 5 );
            outPtr += 4;
            inPtr += cpLen + 1;
        }
        else if ( *tmpPtr == '>' ) {
            if ( *outXmlStr == NULL ) {
                *outXmlStr = outPtr = ( char* )malloc( 5 * strlen( inPtr ) + 6 );
            }
            cpLen = tmpPtr - inPtr;
            strncpy( outPtr, inPtr, cpLen );
            outPtr += cpLen;
            rstrcpy( outPtr, "&gt;", 5 );
            /* strcat (outPtr, "&gt;"); */
            outPtr += 4;
            inPtr += cpLen + 1;
        }
        else if ( *tmpPtr == '"' ) {
            if ( *outXmlStr == NULL ) {
                *outXmlStr = outPtr = ( char* )malloc( 5 * strlen( inPtr ) + 6 );
            }
            cpLen = tmpPtr - inPtr;
            strncpy( outPtr, inPtr, cpLen );
            outPtr += cpLen;
            rstrcpy( outPtr, "&quot;", 7 );
            outPtr += 6;
            inPtr += cpLen + 1;
        }
        else if ( *tmpPtr == '`' ) {
            if ( *outXmlStr == NULL ) {
                *outXmlStr = outPtr = ( char* )malloc( 5 * strlen( inPtr ) + 6 );
            }
            cpLen = tmpPtr - inPtr;
            strncpy( outPtr, inPtr, cpLen );
            outPtr += cpLen;
            /* strcat (outPtr, "&apos;"); */
            rstrcpy( outPtr, "&apos;", 7 );
            outPtr += 6;
            inPtr += cpLen + 1;
        }
        else if ( *tmpPtr == '\0' ) {
            break;
        }
        tmpPtr++;
    }

    if ( *outXmlStr != NULL ) {
        /* copy the remaining */
        strcpy( outPtr, inPtr );
    }
    else {
        /* no predeclared char. just use the inStr */
        *outXmlStr = inStr;
    }

    return strlen( *outXmlStr );
}

int
xmlStrToStr( char *inStr, int myLen ) {
    int savedChar;
    char *tmpPtr;
    char *inPtr;
    int len;

    if ( inStr == NULL || myLen == 0 ) {
        return 0;
    }

    /* put a null at the end of the str because it is not null terminated */
    savedChar = inStr[myLen];
    inStr[myLen] = '\0';

    /* do a quick scan for & */

    if ( strchr( inStr, '&' ) == NULL ) {
        inStr[myLen] = savedChar;
        return myLen;
    }

    inPtr = inStr;

    while ( 1 ) {
        if ( ( tmpPtr = strchr( inPtr, '&' ) ) == NULL ) {
            break;
        }

        if ( strncmp( tmpPtr, "&amp;", 5 ) == 0 ) {
            inPtr = tmpPtr;
            *inPtr = '&';
            inPtr ++;
            ovStrcpy( inPtr, tmpPtr + 5 );
        }
        else if ( strncmp( tmpPtr, "&lt;", 4 ) == 0 ) {
            inPtr = tmpPtr;
            *inPtr = '<';
            inPtr ++;
            ovStrcpy( inPtr, tmpPtr + 4 );
        }
        else if ( strncmp( tmpPtr, "&gt;", 4 ) == 0 ) {
            inPtr = tmpPtr;
            *inPtr = '>';
            inPtr ++;
            ovStrcpy( inPtr, tmpPtr + 4 );
        }
        else if ( strncmp( tmpPtr, "&quot;", 6 ) == 0 ) {
            inPtr = tmpPtr;
            *inPtr = '"';
            inPtr ++;
            ovStrcpy( inPtr, tmpPtr + 6 );
        }
        else if ( strncmp( tmpPtr, "&apos;", 6 ) == 0 ) {
            inPtr = tmpPtr;
            *inPtr = '`';
            inPtr ++;
            ovStrcpy( inPtr, tmpPtr + 6 );
        }
        else {
            break;
        }
    }

    len = strlen( inStr );
    /* put the char back */
    inStr[myLen] = savedChar;

    return len;
}

int
packNullString( packedOutput_t *packedOutput ) {
    int myStrlen;
    void *outPtr;

    myStrlen = strlen( NULL_PTR_PACK_STR );
    extendPackedOutput( packedOutput, myStrlen + 1, &outPtr );
    strncpy( ( char* )outPtr, NULL_PTR_PACK_STR, myStrlen + 1 );
    packedOutput->bBuf->len += ( myStrlen + 1 );
    return 0;
}

int
packInt( void **inPtr, packedOutput_t *packedOutput, int numElement,
         packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int *tmpIntPtr, *origIntPtr, *inIntPtr;
    int i;
    void *outPtr;
    int intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    inIntPtr = ( int * ) * inPtr;

    if ( inIntPtr != NULL ) {
        /* save this and return later */
        intValue = *inIntPtr;
    }

    if ( irodsProt == XML_PROT ) {
        if ( inIntPtr == NULL ) {
            /* pack nothing */
            return 0;
        }
        for ( i = 0; i < numElement; i++ ) {
            packXmlTag( myPackedItem, packedOutput, START_TAG_FL );
            extendPackedOutput( packedOutput, 12, &outPtr );
            snprintf( ( char* )outPtr, 12, "%d", *inIntPtr );
            packedOutput->bBuf->len += ( strlen( ( char* )outPtr ) );
            packXmlTag( myPackedItem, packedOutput, END_TAG_FL );
            inIntPtr++;
        }
        *inPtr = inIntPtr;
    }
    else {
        origIntPtr = tmpIntPtr = ( int * ) malloc( sizeof( int ) * numElement );

        if ( inIntPtr == NULL ) {
            /* a NULL pointer, fill the array with 0 */
            memset( origIntPtr, 0, sizeof( int ) * numElement );
        }
        else {
            for ( i = 0; i < numElement; i++ ) {
                *tmpIntPtr = htonl( *inIntPtr );
                tmpIntPtr ++;
                inIntPtr ++;
            }
            *inPtr = inIntPtr;
        }

        extendPackedOutput( packedOutput, sizeof( int ) * numElement, &outPtr );
        memcpy( outPtr, ( void * ) origIntPtr, sizeof( int ) * numElement );
        free( origIntPtr );
        packedOutput->bBuf->len += ( sizeof( int ) * numElement );

    }
    if ( intValue < 0 ) {
        /* prevent error exiting */
        intValue = 0;
    }

    return intValue;
}

int
packInt16( void **inPtr, packedOutput_t *packedOutput, int numElement,
           packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    short *tmpIntPtr, *origIntPtr, *inIntPtr;
    int i;
    void *outPtr;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    inIntPtr = ( short * ) * inPtr;

    if ( inIntPtr != NULL ) {
        /* save this and return later */
        intValue = *inIntPtr;
    }

    if ( irodsProt == XML_PROT ) {
        if ( inIntPtr == NULL ) {
            /* pack nothing */
            return 0;
        }
        for ( i = 0; i < numElement; i++ ) {
            packXmlTag( myPackedItem, packedOutput, START_TAG_FL );
            extendPackedOutput( packedOutput, 12, &outPtr );
            snprintf( ( char* )outPtr, 12, "%hi", *inIntPtr );
            packedOutput->bBuf->len += ( strlen( ( char* )outPtr ) );
            packXmlTag( myPackedItem, packedOutput, END_TAG_FL );
            inIntPtr++;
        }
        *inPtr = inIntPtr;
    }
    else {
        origIntPtr = tmpIntPtr = ( short * ) malloc( sizeof( short ) * numElement );

        if ( inIntPtr == NULL ) {
            /* a NULL pointer, fill the array with 0 */
            memset( origIntPtr, 0, sizeof( short ) * numElement );
        }
        else {
            for ( i = 0; i < numElement; i++ ) {
                *tmpIntPtr = htons( *inIntPtr );
                tmpIntPtr ++;
                inIntPtr ++;
            }
            *inPtr = inIntPtr;
        }

        extendPackedOutput( packedOutput, sizeof( short ) * numElement, &outPtr );
        memcpy( outPtr, ( void * ) origIntPtr, sizeof( short ) * numElement );
        free( origIntPtr );
        packedOutput->bBuf->len += ( sizeof( short ) * numElement );

    }
    if ( intValue < 0 ) {
        /* prevent error exiting */
        intValue = 0;
    }

    return intValue;
}

int
packDouble( void **inPtr, packedOutput_t *packedOutput, int numElement,
            packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    rodsLong_t *tmpDoublePtr, *origDoublePtr, *inDoublePtr;
    int i;
    void *outPtr;

    if ( numElement == 0 ) {
        return 0;
    }

    inDoublePtr = ( rodsLong_t * ) * inPtr;

    if ( irodsProt == XML_PROT ) {
        if ( inDoublePtr == NULL ) {
            /* pack nothing */
            return 0;
        }
        for ( i = 0; i < numElement; i++ ) {
            packXmlTag( myPackedItem, packedOutput, START_TAG_FL );
            extendPackedOutput( packedOutput, 20, &outPtr );
            snprintf( ( char* )outPtr, 20, "%lld", *inDoublePtr );
            packedOutput->bBuf->len += ( strlen( ( char* )outPtr ) );
            packXmlTag( myPackedItem, packedOutput, END_TAG_FL );
            inDoublePtr++;
        }
        *inPtr = inDoublePtr;
    }
    else {
        origDoublePtr = tmpDoublePtr = ( rodsLong_t * ) malloc(
                                           sizeof( rodsLong_t ) * numElement );

        if ( inDoublePtr == NULL ) {
            /* a NULL pointer, fill the array with 0 */
            memset( origDoublePtr, 0, sizeof( rodsLong_t ) * numElement );
        }
        else {
            for ( i = 0; i < numElement; i++ ) {
                myHtonll( *inDoublePtr, tmpDoublePtr );
                tmpDoublePtr ++;
                inDoublePtr ++;
            }
            *inPtr = inDoublePtr;
        }
        extendPackedOutput( packedOutput, sizeof( rodsLong_t ) * numElement,
                            &outPtr );
        memcpy( outPtr, ( void * ) origDoublePtr,
                sizeof( rodsLong_t ) * numElement );
        free( origDoublePtr );
        packedOutput->bBuf->len += ( sizeof( rodsLong_t ) * numElement );
    }

    return 0;
}

int
packChildStruct( void **inPtr, packedOutput_t *packedOutput,
                 packItem_t *myPackedItem, const packInstructArray_t *myPackTable, int numElement,
                 int packFlag, irodsProt_t irodsProt, char *packInstructInp ) {
    const void *packInstruct;
    int i = 0, status = 0;
    packItem_t *packItemHead, *tmpItem;

    if ( numElement == 0 ) {
        return 0;
    }

    if ( packInstructInp == NULL ) {
        packInstruct = matchPackInstruct( myPackedItem->name, myPackTable );
    }
    else {
        packInstruct = packInstructInp;
    }

    if ( packInstruct == NULL ) {
        rodsLog( LOG_ERROR,
                 "packChildStruct: matchPackInstruct failed for %s",
                 myPackedItem->name );
        return SYS_UNMATCH_PACK_INSTRUCTI_NAME;
    }

    for ( i = 0; i < numElement; i++ ) {
        packItemHead = NULL;

        status = parsePackInstruct( ( const char* )packInstruct, &packItemHead );
        if ( status < 0 ) {
            freePackedItem( packItemHead );
            return status;
        }
        /* link it */
        if ( packItemHead != NULL ) {
            packItemHead->parent = myPackedItem;
        }

        if ( irodsProt == XML_PROT ) {
            packXmlTag( myPackedItem, packedOutput, START_TAG_FL | LF_FL );
        }

        /* now pack each child item */
        tmpItem = packItemHead;
        while ( tmpItem != NULL ) {
#if defined(solaris_platform) && !defined(i86_hardware)
            if ( tmpItem->pointerType == 0 &&
                    packTypeTable[tmpItem->typeInx].number == PACK_DOUBLE_TYPE ) {
                doubleInStruct = 1;
            }
#endif
            status = packItem( tmpItem, inPtr, packedOutput,
                               myPackTable, packFlag, irodsProt );
            if ( status < 0 ) {
                return status;
            }
            tmpItem = tmpItem->next;
        }
        freePackedItem( packItemHead );
#if defined(solaris_platform) && !defined(i86_hardware)
        /* seems that solaris align to 64 bit boundary if there is any
         * double in struct */
        if ( doubleInStruct > 0 ) {
            *inPtr = ( void * ) alignDouble( *inPtr );
        }
#endif
        if ( irodsProt == XML_PROT ) {
            packXmlTag( myPackedItem, packedOutput, END_TAG_FL );
        }
    }
    return status;
}

int
freePackedItem( packItem_t *packItemHead ) {
    packItem_t *tmpItem, *nextItem;

    tmpItem = packItemHead;

    while ( tmpItem != NULL ) {
        nextItem = tmpItem->next;
        if ( tmpItem->name != NULL ) {
            free( tmpItem->name );
        }
        free( tmpItem );
        tmpItem = nextItem;
    }

    return 0;
}

int
unpackItem( packItem_t *myPackedItem, void **inPtr,
            packedOutput_t *unpackedOutput, const packInstructArray_t *myPackTable,
            irodsProt_t irodsProt ) {
    int status;

    status = resolvePackedItem( myPackedItem, inPtr, UNPACK_OPR );
    if ( status < 0 ) {
        return status;
    }
    if ( myPackedItem->pointerType > 0 ) {
        /* a pointer type */
        status = unpackPointerItem( myPackedItem, inPtr, unpackedOutput,
                                    myPackTable, irodsProt );
    }
    else {
        status = unpackNonpointerItem( myPackedItem, inPtr, unpackedOutput,
                                       myPackTable, irodsProt );
    }

    return status;
}

int
unpackNonpointerItem( packItem_t *myPackedItem, void **inPtr,
                      packedOutput_t *unpackedOutput, const packInstructArray_t *myPackTable,
                      irodsProt_t irodsProt ) {
    int numElement;
    int elementSz;
    int typeInx;
    int myTypeNum;
    int i, status = 0;

    typeInx = myPackedItem->typeInx;
    numElement = getNumElement( myPackedItem );
    elementSz = packTypeTable[typeInx].size;
    myTypeNum = packTypeTable[typeInx].number;

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        status = unpackChar( inPtr, unpackedOutput, numElement * elementSz,
                             myPackedItem, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        break;
    case PACK_STR_TYPE:
    case PACK_PI_STR_TYPE: {
        /* no need to align inPtr */
        /* the size of the last dim is the max length of the string. Don't
         * want to pack the entire length, just to end of the string including
         * the NULL.
         */
        int maxStrLen, numStr, myDim;

        myDim = myPackedItem->dim;
        if ( myDim <= 0 ) {
            /* null terminated */
            maxStrLen = -1;
            numStr = 1;
        }
        else {
            maxStrLen = myPackedItem->dimSize[myDim - 1];
            numStr = numElement / maxStrLen;
        }

        for ( i = 0; i < numStr; i++ ) {
            char *outStr = NULL;
            status = unpackString( inPtr, unpackedOutput, maxStrLen,
                                   myPackedItem, irodsProt, &outStr );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "unpackNonpointerItem: strlen of %s > dim size, content: %s ",
                         myPackedItem->name, *inPtr );
                return status;
            }
            if ( myTypeNum == PACK_PI_STR_TYPE && i == 0 && outStr != NULL ) {
                strncpy( myPackedItem->strValue, outStr, NAME_LEN );
            }
        }
        break;
    }
    case PACK_INT_TYPE:
        status = unpackInt( inPtr, unpackedOutput, numElement,
                            myPackedItem, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem->intValue = status;
        break;
    case PACK_INT16_TYPE:
        status = unpackInt16( inPtr, unpackedOutput, numElement,
                              myPackedItem, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem->intValue = status;
        break;
    case PACK_DOUBLE_TYPE:
        status = unpackDouble( inPtr, unpackedOutput, numElement,
                               myPackedItem, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        break;
    case PACK_STRUCT_TYPE:
        status = unpackChildStruct( inPtr, unpackedOutput, myPackedItem,
                                    myPackTable, numElement, irodsProt, NULL );

        if ( status < 0 ) {
            return status;
        }
        break;
    default:
        rodsLog( LOG_ERROR,
                 "unpackNonpointerItem: Unknow type %d - %s ",
                 myTypeNum, myPackedItem->name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    return status;
}
/* unpackChar - This routine functionally is the same as packChar */

int
unpackChar( void **inPtr, packedOutput_t *unpackedOutput, int len,
            packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    void *outPtr;

    if ( len <= 0 ) {
        return 0;
    }

    /* no need to align address */

    extendPackedOutput( unpackedOutput, len, &outPtr );
    if ( *inPtr == NULL ) {     /* A NULL pointer */
        /* just fill it with 0 */
        memset( outPtr, 0, len );
    }
    else {
        unpackCharToOutPtr( inPtr, &outPtr, len, myPackedItem, irodsProt );
    }
    unpackedOutput->bBuf->len += len;

    return 0;
}

int
unpackCharToOutPtr( void **inPtr, void **outPtr, int len,
                    packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlCharToOutPtr( inPtr, outPtr, len, myPackedItem );
    }
    else {
        status = unpackNatCharToOutPtr( inPtr, outPtr, len );
    }
    return status;
}

int
unpackNatCharToOutPtr( void **inPtr, void **outPtr, int len ) {
    memcpy( *outPtr, *inPtr, len );
    *inPtr = ( void * )( ( char * ) * inPtr + len );
    *outPtr = ( void * )( ( char * ) * outPtr + len );

    return 0;
}

int
unpackXmlCharToOutPtr( void **inPtr, void **outPtr, int len,
                       packItem_t *myPackedItem ) {
    int endTagLen;
    int inLen = parseXmlValue( inPtr, myPackedItem, &endTagLen );
    if ( inLen < 0 ) {
        return inLen;
    }

    if ( packTypeTable[myPackedItem->typeInx].number == PACK_BIN_TYPE ) {
        /* bin type. need to decode */
        unsigned long outLen = len;
        if ( int status = base64_decode( ( const unsigned char * ) * inPtr,
                                         inLen, ( unsigned char * ) * outPtr, &outLen ) ) {
            return status;
        }
        if ( ( int ) outLen != len ) {
            rodsLog( LOG_NOTICE,
                     "unpackXmlCharToOutPtr: required len %d != %d from base64_decode",
                     len, outLen );
        }
    }
    else {
        if ( inLen != len ) {
            rodsLog( LOG_NOTICE,
                     "unpackXmlCharToOutPtr: required len %d != %d from input",
                     len, inLen );
            if ( inLen > len ) {
                return USER_PACKSTRUCT_INPUT_ERR;
            }
        }
        memcpy( *outPtr, *inPtr, inLen );
    }
    *inPtr = ( void * )( ( char * ) * inPtr + inLen + endTagLen );
    *outPtr = ( void * )( ( char * ) * outPtr + len );

    return 0;
}

int
unpackString( void **inPtr, packedOutput_t *unpackedOutput, int maxStrLen,
              packItem_t *myPackedItem, irodsProt_t irodsProt, char **outStr ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlString( inPtr, unpackedOutput, maxStrLen,
                                  myPackedItem, outStr );
    }
    else {
        status = unpackNatString( inPtr, unpackedOutput, maxStrLen, outStr );
    }
    return status;
}

int
unpackNatString( void **inPtr, packedOutput_t *unpackedOutput, int maxStrLen,
                 char **outStr ) {
    int myStrlen;
    void *outPtr;

    if ( *inPtr == NULL ) {
        myStrlen = 0;
    }
    else {
        myStrlen = strlen( ( char* ) * inPtr );
    }
    if ( myStrlen + 1 >= maxStrLen ) {
        if ( maxStrLen >= 0 ) {
            return USER_PACKSTRUCT_INPUT_ERR;
        }
        else {
            extendPackedOutput( unpackedOutput, myStrlen + 1, &outPtr );
        }
    }
    else {
        extendPackedOutput( unpackedOutput, maxStrLen, &outPtr );
    }

    if ( myStrlen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( ( char* )outPtr, ( char* )*inPtr, myStrlen + 1 );
        *outStr = ( char * ) outPtr;
    }

    if ( maxStrLen > 0 ) {
        *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + 1 ) );
        unpackedOutput->bBuf->len += maxStrLen;
    }
    else {
        *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + 1 ) );
        unpackedOutput->bBuf->len += myStrlen + 1;
    }

    return 0;
}

int
unpackXmlString( void **inPtr, packedOutput_t *unpackedOutput, int maxStrLen,
                 packItem_t *myPackedItem, char **outStr ) {
    int myStrlen;
    char *outPtr;
    int endTagLen;      /* the length of end tag */
    int origStrLen;

    origStrLen = parseXmlValue( inPtr, myPackedItem, &endTagLen );
    if ( origStrLen < 0 ) {
        return origStrLen;
    }

    myStrlen = xmlStrToStr( ( char * ) * inPtr, origStrLen );

    if ( myStrlen >= maxStrLen ) {
        if ( maxStrLen >= 0 ) {
            return USER_PACKSTRUCT_INPUT_ERR;
        }
        else {
            extendPackedOutput( unpackedOutput, myStrlen, ( void ** )( static_cast< void * >( &outPtr ) ) );
        }
    }
    else {
        extendPackedOutput( unpackedOutput, maxStrLen, ( void ** )( static_cast< void * >( &outPtr ) ) );
    }

    if ( myStrlen > 0 ) {
        strncpy( ( char* )outPtr, ( char* )*inPtr, myStrlen );
        *outStr = ( char * ) outPtr;
        outPtr += myStrlen;
    }
    *outPtr = '\0';

    *inPtr = ( void * )( ( char * ) * inPtr + ( origStrLen + 1 ) );
    if ( maxStrLen > 0 ) {
        unpackedOutput->bBuf->len += maxStrLen;
    }
    else {
        unpackedOutput->bBuf->len += myStrlen + 1;
    }

    return 0;
}

int
unpackStringToOutPtr( void **inPtr, void **outPtr, int maxStrLen,
                      packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlStringToOutPtr( inPtr, outPtr, maxStrLen,
                                          myPackedItem );
    }
    else {
        status = unpackNatStringToOutPtr( inPtr, outPtr, maxStrLen );
    }
    return status;
}

/* unpackNullString - check if *inPtr points to a NULL string.
 * If it is, put a NULL pointer in unpackedOutput and returns 0.
 * Otherwise, returns 1.
 */

int
unpackNullString( void **inPtr, packedOutput_t *unpackedOutput,
                  packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int myStrlen;
    int numElement, numPointer;
    int myDim;
    char *myPtr;
    int tagLen;

    if ( *inPtr == NULL ) {
        /* add a null pointer */
        addPointerToPackedOut( unpackedOutput, 0, NULL );
        return 0;
    }

    if ( irodsProt == XML_PROT ) {
        int skipLen;

        myPtr = ( char* ) * inPtr;
        /* check if tag exist */
        tagLen = parseXmlTag( ( void ** )( static_cast< void * >( &myPtr ) ), myPackedItem, START_TAG_FL,
                              &skipLen );
        if ( tagLen < 0 ) {
            addPointerToPackedOut( unpackedOutput, 0, NULL );
            return 0;
        }
        else {
            myPtr = myPtr + tagLen + skipLen;
        }
    }
    else {
        if ( strcmp( ( char* )*inPtr, NULL_PTR_PACK_STR ) == 0 ) {
            myStrlen = strlen( NULL_PTR_PACK_STR );
            addPointerToPackedOut( unpackedOutput, 0, NULL );
            *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + 1 ) );
            return 0;
        }
    }
    /* need to do more checking for null */
    myDim = myPackedItem->dim;
    numPointer = getNumElement( myPackedItem );
    numElement = getNumHintElement( myPackedItem );

    if ( numElement <= 0 || ( myDim > 0 && numPointer <= 0 ) ) {
        /* add a null pointer */
        addPointerToPackedOut( unpackedOutput, 0, NULL );
        if ( irodsProt == XML_PROT ) {
            int nameLen;
            if ( strncmp( myPtr, "</", 2 ) == 0 ) {
                myPtr += 2;
                nameLen = strlen( myPackedItem->name );
                if ( strncmp( myPtr, myPackedItem->name, nameLen ) == 0 ) {
                    myPtr += ( nameLen + 1 );
                    if ( *myPtr == '\n' ) {
                        myPtr++;
                    }
                    *inPtr = myPtr;
                }
            }
        }
        return 0;
    }
    else {
        return 1;
    }
}

int
unpackInt( void **inPtr, packedOutput_t *unpackedOutput, int numElement,
           packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    void *outPtr;
    int intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    extendPackedOutput( unpackedOutput, sizeof( int ) * ( numElement + 1 ),
                        &outPtr );

    intValue = unpackIntToOutPtr( inPtr, &outPtr, numElement, myPackedItem,
                                  irodsProt );

    /* adjust len */
    unpackedOutput->bBuf->len = ( int )( ( char * ) outPtr -
                                         ( char * ) unpackedOutput->bBuf->buf ) + ( sizeof( int ) * numElement );

    if ( intValue < 0 ) {
        /* prevent error exit */
        intValue = 0;
    }

    return intValue;
}

int
unpackIntToOutPtr( void **inPtr, void **outPtr, int numElement,
                   packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlIntToOutPtr( inPtr, outPtr, numElement,
                                       myPackedItem );
    }
    else {
        status = unpackNatIntToOutPtr( inPtr, outPtr, numElement );
    }
    return status;
}

int
unpackNatIntToOutPtr( void **inPtr, void **outPtr, int numElement ) {
    int *tmpIntPtr, *origIntPtr;
    void *inIntPtr;
    int i;
    int intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    inIntPtr = *inPtr;

    origIntPtr = tmpIntPtr = ( int * ) malloc( sizeof( int ) * numElement );

    if ( inIntPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( origIntPtr, 0, sizeof( int ) * numElement );
    }
    else {
        for ( i = 0; i < numElement; i++ ) {
            int tmpInt;

            memcpy( &tmpInt, inIntPtr, sizeof( int ) );
            *tmpIntPtr = htonl( tmpInt );
            if ( i == 0 ) {
                /* save this and return later */
                intValue = *tmpIntPtr;
            }
            tmpIntPtr ++;
            inIntPtr = ( void * )( ( char * ) inIntPtr + sizeof( int ) );
        }
        *inPtr = inIntPtr;
    }

    /* align unpackedOutput to 4 bytes boundary. Will not align inPtr */

    *outPtr = alignInt( *outPtr );

    memcpy( *outPtr, ( void * ) origIntPtr, sizeof( int ) * numElement );
    free( origIntPtr );

    return intValue;
}

int
unpackXmlIntToOutPtr( void **inPtr, void **outPtr, int numElement,
                      packItem_t *myPackedItem ) {
    int *tmpIntPtr;
    int i;
    int myStrlen;
    int endTagLen;      /* the length of end tag */
    int intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    /* align outPtr to 4 bytes boundary. Will not align inPtr */

    *outPtr = tmpIntPtr = ( int* )alignInt( *outPtr );

    if ( *inPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( *outPtr, 0, sizeof( int ) * numElement );
    }
    else {
        char tmpStr[NAME_LEN];

        for ( i = 0; i < numElement; i++ ) {
            myStrlen = parseXmlValue( inPtr, myPackedItem, &endTagLen );
            if ( myStrlen < 0 ) {
                return myStrlen;
            }
            else if ( myStrlen >= NAME_LEN ) {
                rodsLog( LOG_ERROR,
                         "unpackXmlIntToOutPtr: input %s with value %s too long",
                         myPackedItem->name, *inPtr );
                return USER_PACKSTRUCT_INPUT_ERR;
            }
            strncpy( tmpStr, ( char* )*inPtr, myStrlen );
            tmpStr[myStrlen] = '\0';

            *tmpIntPtr = atoi( tmpStr );
            if ( i == 0 ) {
                /* save this and return later */
                intValue = *tmpIntPtr;
            }
            tmpIntPtr ++;
            *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + endTagLen ) );
        }
    }
    return intValue;
}

int
unpackInt16( void **inPtr, packedOutput_t *unpackedOutput, int numElement,
             packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    void *outPtr;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    extendPackedOutput( unpackedOutput, sizeof( short ) * ( numElement + 1 ),
                        &outPtr );

    intValue = unpackInt16ToOutPtr( inPtr, &outPtr, numElement, myPackedItem,
                                    irodsProt );

    /* adjust len */
    unpackedOutput->bBuf->len = ( int )( ( char * ) outPtr -
                                         ( char * ) unpackedOutput->bBuf->buf ) + ( sizeof( short ) * numElement );

    if ( intValue < 0 ) {
        /* prevent error exit */
        intValue = 0;
    }

    return intValue;
}

int
unpackInt16ToOutPtr( void **inPtr, void **outPtr, int numElement,
                     packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlInt16ToOutPtr( inPtr, outPtr, numElement,
                                         myPackedItem );
    }
    else {
        status = unpackNatInt16ToOutPtr( inPtr, outPtr, numElement );
    }
    return status;
}

int
unpackNatInt16ToOutPtr( void **inPtr, void **outPtr, int numElement ) {
    short *tmpIntPtr, *origIntPtr;
    void *inIntPtr;
    int i;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    inIntPtr = *inPtr;

    origIntPtr = tmpIntPtr = ( short * ) malloc( sizeof( short ) * numElement );

    if ( inIntPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( origIntPtr, 0, sizeof( short ) * numElement );
    }
    else {
        for ( i = 0; i < numElement; i++ ) {
            short tmpInt;

            memcpy( &tmpInt, inIntPtr, sizeof( short ) );
            *tmpIntPtr = htons( tmpInt );
            if ( i == 0 ) {
                /* save this and return later */
                intValue = *tmpIntPtr;
            }
            tmpIntPtr ++;
            inIntPtr = ( void * )( ( char * ) inIntPtr + sizeof( short ) );
        }
        *inPtr = inIntPtr;
    }

    /* align unpackedOutput to 4 bytes boundary. Will not align inPtr */

    *outPtr = alignInt16( *outPtr );

    memcpy( *outPtr, ( void * ) origIntPtr, sizeof( short ) * numElement );
    free( origIntPtr );

    return intValue;
}

int
unpackXmlInt16ToOutPtr( void **inPtr, void **outPtr, int numElement,
                        packItem_t *myPackedItem ) {
    short *tmpIntPtr;
    int i;
    int myStrlen;
    int endTagLen;      /* the length of end tag */
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    /* align outPtr to 4 bytes boundary. Will not align inPtr */

    *outPtr = tmpIntPtr = ( short* )alignInt16( *outPtr );

    if ( *inPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( *outPtr, 0, sizeof( short ) * numElement );
    }
    else {
        char tmpStr[NAME_LEN];

        for ( i = 0; i < numElement; i++ ) {
            myStrlen = parseXmlValue( inPtr, myPackedItem, &endTagLen );
            if ( myStrlen < 0 ) {
                return myStrlen;
            }
            else if ( myStrlen >= NAME_LEN ) {
                rodsLog( LOG_ERROR,
                         "unpackXmlIntToOutPtr: input %s with value %s too long",
                         myPackedItem->name, *inPtr );
                return USER_PACKSTRUCT_INPUT_ERR;
            }
            strncpy( tmpStr, ( char* )*inPtr, myStrlen );
            tmpStr[myStrlen] = '\0';

            *tmpIntPtr = atoi( tmpStr );
            if ( i == 0 ) {
                /* save this and return later */
                intValue = *tmpIntPtr;
            }
            tmpIntPtr ++;
            *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + endTagLen ) );
        }
    }
    return intValue;
}

int
unpackDouble( void **inPtr, packedOutput_t *unpackedOutput, int numElement,
              packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    void *outPtr;

    if ( numElement == 0 ) {
        return 0;
    }

    extendPackedOutput( unpackedOutput, sizeof( rodsLong_t ) * ( numElement + 1 ),
                        &outPtr );

    unpackDoubleToOutPtr( inPtr, &outPtr, numElement, myPackedItem, irodsProt );

    /* adjust len */
    unpackedOutput->bBuf->len = ( int )( ( char * ) outPtr -
                                         ( char * ) unpackedOutput->bBuf->buf ) + ( sizeof( rodsLong_t ) * numElement );

    return 0;
}

int
unpackDoubleToOutPtr( void **inPtr, void **outPtr, int numElement,
                      packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlDoubleToOutPtr( inPtr, outPtr, numElement,
                                          myPackedItem );
    }
    else {
        status = unpackNatDoubleToOutPtr( inPtr, outPtr, numElement );
    }
    return status;
}

int
unpackNatDoubleToOutPtr( void **inPtr, void **outPtr, int numElement ) {
    rodsLong_t *tmpDoublePtr, *origDoublePtr;
    void *inDoublePtr;
    int i;

    if ( numElement == 0 ) {
        return 0;
    }

    inDoublePtr = *inPtr;

    origDoublePtr = tmpDoublePtr = ( rodsLong_t * ) malloc(
                                       sizeof( rodsLong_t ) * numElement );

    if ( inDoublePtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( origDoublePtr, 0, sizeof( rodsLong_t ) * numElement );
    }
    else {
        for ( i = 0; i < numElement; i++ ) {
            rodsLong_t tmpDouble;

            memcpy( &tmpDouble, inDoublePtr, sizeof( rodsLong_t ) );

            myNtohll( tmpDouble, tmpDoublePtr );
            tmpDoublePtr ++;
            inDoublePtr = ( void * )( ( char * ) inDoublePtr + sizeof( rodsLong_t ) );
        }
        *inPtr = inDoublePtr;
    }
    /* align inPtr to 8 bytes boundary. Will not align outPtr */

#if defined(osx_platform) || (defined(solaris_platform) && defined(i86_hardware))
    /* osx does not align */
    *outPtr = alignInt( *outPtr );
#else
    *outPtr = alignDouble( *outPtr );
#endif

    memcpy( *outPtr, ( void * ) origDoublePtr, sizeof( rodsLong_t ) * numElement );
    free( origDoublePtr );

    return 0;
}

int
unpackXmlDoubleToOutPtr( void **inPtr, void **outPtr, int numElement,
                         packItem_t *myPackedItem ) {
    rodsLong_t *tmpDoublePtr;
    int i;
    int myStrlen;
    int endTagLen;      /* the length of end tag */

    if ( numElement == 0 ) {
        return 0;
    }

    /* align inPtr to 8 bytes boundary. Will not align outPtr */

#if defined(osx_platform) || (defined(solaris_platform) && defined(i86_hardware))
    /* osx does not align */
    *outPtr = tmpDoublePtr = ( rodsLong_t* )alignInt( *outPtr );
#else
    *outPtr = tmpDoublePtr = ( rodsLong_t* )alignDouble( *outPtr );
#endif

    if ( *inPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( *outPtr, 0, sizeof( rodsLong_t ) * numElement );
    }
    else {
        for ( i = 0; i < numElement; i++ ) {
            char tmpStr[NAME_LEN];

            myStrlen = parseXmlValue( inPtr, myPackedItem, &endTagLen );
            if ( myStrlen < 0 ) {
                return myStrlen;
            }
            else if ( myStrlen >= NAME_LEN ) {
                rodsLog( LOG_ERROR,
                         "unpackXmlDoubleToOutPtr: input %s with value %s too long",
                         myPackedItem->name, *inPtr );
                return USER_PACKSTRUCT_INPUT_ERR;
            }
            strncpy( tmpStr, ( char* )*inPtr, myStrlen );
            tmpStr[myStrlen] = '\0';

            *tmpDoublePtr = strtoll( tmpStr, 0, 0 );
            tmpDoublePtr ++;
            *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + endTagLen ) );
        }
    }

    return 0;
}

int
unpackChildStruct( void **inPtr, packedOutput_t *unpackedOutput,
                   packItem_t *myPackedItem, const packInstructArray_t *myPackTable, int numElement,
                   irodsProt_t irodsProt, char *packInstructInp ) {
    const void *packInstruct = NULL;
    int i = 0, status = 0;
    packItem_t *unpackItemHead, *tmpItem;
    int skipLen = 0;
#if defined(solaris_platform) && !defined(i86_hardware)
    int doubleInStruct = 0;
#endif
#if defined(solaris_platform)
    void *outPtr1, *outPtr2;
#endif

    if ( numElement == 0 ) {
        return 0;
    }

    if ( packInstructInp == NULL ) {
        packInstruct = matchPackInstruct( myPackedItem->name, myPackTable );
    }
    else {
        packInstruct = packInstructInp;
    }

    if ( packInstruct == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackChildStruct: matchPackInstruct failed for %s",
                 myPackedItem->name );
        return SYS_UNMATCH_PACK_INSTRUCTI_NAME;
    }

    for ( i = 0; i < numElement; i++ ) {
        unpackItemHead = NULL;

        status = parsePackInstruct( static_cast<const char*>( packInstruct ), &unpackItemHead );
        if ( status < 0 ) {
            freePackedItem( unpackItemHead );
            return status;
        }
        /* link it */
        if ( unpackItemHead != NULL ) {
            unpackItemHead->parent = myPackedItem;
        }

        if ( irodsProt == XML_PROT ) {
            status = parseXmlTag( inPtr, myPackedItem, START_TAG_FL | LF_FL,
                                  &skipLen );
            if ( status >= 0 ) {
                *inPtr = ( char * ) * inPtr + status + skipLen;
            }
            else {
                if ( myPackedItem->pointerType > 0 ) {
                    /* a null pointer */
                    addPointerToPackedOut( unpackedOutput, 0, NULL );
                    status = 0;
                    continue;
                }
                else {
                    return status;
                }
            }
        }

        /* now unpack each child item */

#if defined(solaris_platform) && !defined(i86_hardware)
        doubleInStruct = 0;
#endif
        tmpItem = unpackItemHead;
        while ( tmpItem != NULL ) {
#if defined(solaris_platform) && !defined(i86_hardware)
            if ( tmpItem->pointerType == 0 &&
                    packTypeTable[tmpItem->typeInx].number == PACK_DOUBLE_TYPE ) {
                doubleInStruct = 1;
            }
#endif
            status = unpackItem( tmpItem, inPtr, unpackedOutput,
                                 myPackTable, irodsProt );
            if ( status < 0 ) {
                return status;
            }
            tmpItem = tmpItem->next;
        }
        freePackedItem( unpackItemHead );
#if defined(solaris_platform) && !defined(i86_hardware)
        /* seems that solaris align to 64 bit boundary if there is any
         * double in struct */
        if ( doubleInStruct > 0 ) {
            extendPackedOutput( unpackedOutput, sizeof( rodsLong_t ), &outPtr1 );
            outPtr2 = alignDouble( outPtr1 );
            unpackedOutput->bBuf->len += ( ( int ) outPtr2 - ( int ) outPtr1 );
        }
#endif
        if ( irodsProt == XML_PROT ) {
            status = parseXmlTag( inPtr, myPackedItem, END_TAG_FL | LF_FL,
                                  &skipLen );
            if ( status >= 0 ) {
                *inPtr = ( char * ) * inPtr + status + skipLen;
            }
            else {
                return status;
            }
        }
    }
    return status;
}

int
unpackPointerItem( packItem_t *myPackedItem, void **inPtr,
                   packedOutput_t *unpackedOutput, const packInstructArray_t *myPackTable,
                   irodsProt_t irodsProt ) {
    int numElement = 0, numPointer = 0;
    int elementSz = 0;
    int typeInx = 0;
    int myTypeNum = 0;
    int i = 0, j = 0, status = 0;
    void **pointerArray = 0;
    void *outPtr = 0;
    int myDim = 0;

    if ( unpackNullString( inPtr, unpackedOutput, myPackedItem, irodsProt )
            <= 0 ) {
        /* a null pointer and has been handled */
        return 0;
    }

    myDim = myPackedItem->dim;
    typeInx = myPackedItem->typeInx;
    numPointer = getNumElement( myPackedItem );
    numElement = getNumHintElement( myPackedItem );
    elementSz = packTypeTable[typeInx].size;
    myTypeNum = packTypeTable[typeInx].number;

    /* alloc pointer to an array of pointers if myDim > 0 */
    if ( myDim > 0 ) {
        if ( numPointer > 0 ) {
            int allocLen, myModu;

            /* allocate at PTR_ARRAY_MALLOC_LEN boundary */
            if ( ( myModu = numPointer % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
                allocLen = numPointer;
            }
            else {
                allocLen = numPointer + PTR_ARRAY_MALLOC_LEN - myModu;
            }
            if ( myTypeNum == PACK_DOUBLE_TYPE || myTypeNum == PACK_INT_TYPE ||
                    myTypeNum == PACK_INT16_TYPE ) {
                /* pointer to an array of int or double */
                pointerArray = ( void ** ) addPointerToPackedOut( unpackedOutput,
                               allocLen * elementSz, NULL );
            }
            else {
                pointerArray = ( void ** ) addPointerToPackedOut( unpackedOutput,
                               allocLen * sizeof( void * ), NULL );
            }
        }
        else {
            return 0;
        }
    }
    else if ( myDim < 0 ) {
        return SYS_NEGATIVE_SIZE;
    }

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        if ( myDim == 0 ) {
            if ( myPackedItem->pointerType == NO_PACK_POINTER ) {
            }
            else {
                outPtr = addPointerToPackedOut( unpackedOutput,
                                                numElement * elementSz, NULL );
                status = unpackCharToOutPtr( inPtr, &outPtr,
                                             numElement * elementSz, myPackedItem, irodsProt );
            }

            if ( status < 0 ) {
                return status;
            }
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                if ( myPackedItem->pointerType != NO_PACK_POINTER ) {
                    outPtr = pointerArray[i] = malloc( numElement * elementSz );
                    status = unpackCharToOutPtr( inPtr, &outPtr,
                                                 numElement * elementSz, myPackedItem, irodsProt );
                }
                if ( status < 0 ) {
                    return status;
                }
            }
        }
        break;
    case PACK_STR_TYPE:
    case PACK_PI_STR_TYPE: {
        /* the size of the last dim is the max length of the string. Don't
         * want to unpack the entire length, just to end of the string
         * including the NULL.
         */
        int maxStrLen = 0, numStr = 0, myLen = 0;

        getNumStrAndStrLen( myPackedItem, &numStr, &maxStrLen );

        if ( maxStrLen == 0 ) {
            /* add a null pointer mw. 9/15/06 */
            /* this check is probably not needed since it has been handled
               by numElement == 0 */
            outPtr = addPointerToPackedOut( unpackedOutput, 0, NULL );
            return 0;
        }

        if ( myDim == 0 ) {
            char *myOutStr;

            myLen = getAllocLenForStr( myPackedItem, inPtr, numStr, maxStrLen );
            if ( myLen < 0 ) {
                return myLen;
            }

            outPtr = addPointerToPackedOut( unpackedOutput, myLen, NULL );
            myOutStr = ( char * )outPtr;
            for ( i = 0; i < numStr; i++ ) {
                status = unpackStringToOutPtr(
                             inPtr, &outPtr, maxStrLen, myPackedItem, irodsProt );
                if ( status < 0 ) {
                    return status;
                }
                if ( myTypeNum == PACK_PI_STR_TYPE && i == 0 &&
                        myOutStr != NULL ) {
                    strncpy( myPackedItem->strValue, myOutStr, NAME_LEN );
                }
            }
        }
        else {
            for ( j = 0; j < numPointer; j++ ) {
                myLen = getAllocLenForStr( myPackedItem, inPtr, numStr,
                                           maxStrLen );
                if ( myLen < 0 ) {
                    return myLen;
                }
                outPtr = pointerArray[j] = malloc( myLen );
                for ( i = 0; i < numStr; i++ ) {
                    status = unpackStringToOutPtr(
                                 inPtr, &outPtr, maxStrLen, myPackedItem, irodsProt );
                    if ( status < 0 ) {
                        return status;
                    }
                }
            }
        }
        break;
    }
    case PACK_INT_TYPE:
        if ( myDim == 0 ) {
            outPtr = addPointerToPackedOut( unpackedOutput,
                                            numElement * elementSz, NULL );
            status = unpackIntToOutPtr( inPtr, &outPtr, numElement,
                                        myPackedItem, irodsProt );
            /* don't chk status. It could be a -ive int */
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                outPtr = pointerArray[i] = malloc( numElement * elementSz );
                status = unpackIntToOutPtr( inPtr, &outPtr,
                                            numElement * elementSz, myPackedItem, irodsProt );
                if ( status < 0 ) {
                    return status;
                }
            }
        }
        break;
    case PACK_INT16_TYPE:
        if ( myDim == 0 ) {
            outPtr = addPointerToPackedOut( unpackedOutput,
                                            numElement * elementSz, NULL );
            status = unpackInt16ToOutPtr( inPtr, &outPtr, numElement,
                                          myPackedItem, irodsProt );
            /* don't chk status. It could be a -ive int */
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                outPtr = pointerArray[i] = malloc( numElement * elementSz );
                status = unpackInt16ToOutPtr( inPtr, &outPtr,
                                              numElement * elementSz, myPackedItem, irodsProt );
                if ( status < 0 ) {
                    return status;
                }
            }
        }
        break;
    case PACK_DOUBLE_TYPE:
        if ( myDim == 0 ) {
            outPtr = addPointerToPackedOut( unpackedOutput,
                                            numElement * elementSz, NULL );
            status = unpackDoubleToOutPtr( inPtr, &outPtr, numElement,
                                           myPackedItem, irodsProt );
            /* don't chk status. It could be a -ive int */
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                outPtr = pointerArray[i] = malloc( numElement * elementSz );
                status = unpackDoubleToOutPtr( inPtr, &outPtr,
                                               numElement * elementSz, myPackedItem, irodsProt );
                if ( status < 0 ) {
                    return status;
                }
            }
        }

        break;

    case PACK_STRUCT_TYPE: {
        /* no need to align boundary for struct */
        packedOutput_t subPackedOutput;

        if ( myDim == 0 ) {
            /* we really don't know the size of each struct. */
            /* outPtr = addPointerToPackedOut (unpackedOutput,
               numElement * SUB_STRUCT_ALLOC_SZ); */
            outPtr = malloc( numElement * SUB_STRUCT_ALLOC_SZ );
            initPackedOutputWithBuf( &subPackedOutput, outPtr,
                                     numElement * SUB_STRUCT_ALLOC_SZ );
            status = unpackChildStruct( inPtr, &subPackedOutput, myPackedItem,
                                        myPackTable, numElement, irodsProt, NULL );
            /* need to free this because it is malloc'ed */
            if ( subPackedOutput.bBuf != NULL ) {
                addPointerToPackedOut( unpackedOutput,
                                       numElement * SUB_STRUCT_ALLOC_SZ, subPackedOutput.bBuf->buf );
                free( subPackedOutput.bBuf );
                subPackedOutput.bBuf = NULL;
            }
            if ( status < 0 ) {
                return status;
            }
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                /* outPtr = pointerArray[i] = malloc ( */
                outPtr = malloc(
                             numElement * SUB_STRUCT_ALLOC_SZ );
                initPackedOutputWithBuf( &subPackedOutput, outPtr,
                                         numElement * SUB_STRUCT_ALLOC_SZ );
                status = unpackChildStruct( inPtr, &subPackedOutput,
                                            myPackedItem, myPackTable, numElement, irodsProt, NULL );
                /* need to free this because it is malloc'ed */
                if ( subPackedOutput.bBuf != NULL ) {
                    pointerArray[i] = subPackedOutput.bBuf->buf;
                    free( subPackedOutput.bBuf );
                    subPackedOutput.bBuf = NULL;
                }
                if ( status < 0 ) {
                    return status;
                }
            }
        }
        break;
    }
    default:
        rodsLog( LOG_ERROR,
                 "unpackPointerItem: Unknown type %d - %s ",
                 myTypeNum, myPackedItem->name );

        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    return 0;
}

void *
addPointerToPackedOut( packedOutput_t *packedOutput, int len, void *pointer ) {
    void *outPtr;
    void **tmpPtr;


    extendPackedOutput( packedOutput, sizeof( void * ), &outPtr );
    outPtr = ialignAddr( outPtr );
    tmpPtr = ( void ** ) outPtr;

    if ( pointer != NULL ) {
        *tmpPtr = pointer;
    }
    else if ( len > 0 ) {
        *tmpPtr = malloc( len );
    }
    else {
        /* add a NULL pointer */
        *tmpPtr = NULL;
    }

    packedOutput->bBuf->len = ( int )( ( char * ) outPtr -
                                       ( char * ) packedOutput->bBuf->buf ) + sizeof( void * );

    return *tmpPtr;
}

int
unpackNatStringToOutPtr( void **inPtr, void **outPtr, int maxStrLen ) {
    int myStrlen;

    if ( inPtr == NULL || *inPtr == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackStringToOutPtr: NULL inPtr" );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    myStrlen = strlen( ( char* ) * inPtr );

    /* maxStrLen = -1 means null terminated */
    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    rstrcpy( ( char* )*outPtr, ( char* )*inPtr, myStrlen + 1 );

    *inPtr = ( void * )( ( char * ) * inPtr + ( myStrlen + 1 ) );

    if ( maxStrLen >= 0 ) {
        *outPtr = ( void * )( ( char * ) * outPtr + maxStrLen );
    }
    else {
        *outPtr = ( void * )( ( char * ) * outPtr + ( myStrlen + 1 ) );
    }

    return 0;
}

int
unpackXmlStringToOutPtr( void **inPtr, void **outPtr, int maxStrLen,
                         packItem_t *myPackedItem ) {
    int myStrlen;
    int endTagLen;      /* the length of end tag */
    char *myStrPtr;
    int origStrLen;

    if ( inPtr == NULL || *inPtr == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackXmlStringToOutPtr: NULL inPtr" );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    origStrLen = parseXmlValue( inPtr, myPackedItem, &endTagLen );
    if ( origStrLen < 0 ) {
        return origStrLen;
    }

    myStrlen = xmlStrToStr( ( char * ) * inPtr, origStrLen );

    /* maxStrLen = -1 means null terminated */
    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    if ( myStrlen == 0 ) {
        memset( *outPtr, 0, 1 );
    }
    else {
        myStrPtr = ( char* ) * outPtr;
        strncpy( myStrPtr, ( char* )*inPtr, myStrlen );
        myStrPtr[myStrlen] = '\0';
    }

    *inPtr = ( void * )( ( char * ) * inPtr + ( origStrLen + endTagLen ) );

    if ( maxStrLen >= 0 ) {
        *outPtr = ( void * )( ( char * ) * outPtr + maxStrLen );
    }
    else {
        *outPtr = ( void * )( ( char * ) * outPtr + ( myStrlen + 1 ) );
    }

    return 0;
}

int
getStrLen( void *inPtr, int maxStrLen ) {
    if ( maxStrLen <= 0 ) {
        return strlen( ( char* )inPtr ) + 1;
    }
    else {
        return maxStrLen;
    }
}

/*
 * a maxStrLen of -1 means it is NULL terminated
 */

int
getNumStrAndStrLen( packItem_t *myPackedItem, int *numStr, int *maxStrLen ) {
    int myHintDim;

    myHintDim = myPackedItem->hintDim;
    if ( myHintDim <= 0 ) {  /* just a pointer to a null terminated str */
        *maxStrLen = -1;
        *numStr = 1;
    }
    else {
        *maxStrLen = myPackedItem->hintDimSize[myHintDim - 1];
        if ( *maxStrLen <= 0 ) {
            *numStr = 0;
        }
        else {
            int numElement;
            numElement = getNumHintElement( myPackedItem );
            *numStr = numElement / *maxStrLen;
        }
    }

    return 0;
}

/* getAllocLenForStr - Get the alloc length for unpacking str pointer
 *
 * A -1 maxStrLen means NULL terminated
 */

int
getAllocLenForStr( packItem_t *myPackedItem, void **inPtr, int numStr,
                   int maxStrLen ) {
    int myLen;

    if ( numStr <= 1 ) {
        myLen = getStrLen( *inPtr, maxStrLen );
    }
    else {
        if ( maxStrLen < 0 ) {
            rodsLog( LOG_ERROR,
                     "unpackPointerItem: maxStrLen < 0 with numStr > 1 for %s",
                     myPackedItem->name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
        myLen = numStr * maxStrLen;
    }
    return myLen;
}

int
packXmlTag( packItem_t *myPackedItem, packedOutput_t *packedOutput,
            int flag ) {
    int myStrlen;
    void *outPtr;

    myStrlen = strlen( myPackedItem->name );

    /* include <>, '/', \n  and NULL */
    extendPackedOutput( packedOutput, myStrlen + 5, &outPtr );
    if ( flag & END_TAG_FL ) {
        snprintf( ( char* )outPtr, myStrlen + 5, "</%s>\n", myPackedItem->name );
    }
    else {
        if ( flag & LF_FL ) {
            snprintf( ( char* )outPtr, myStrlen + 5, "<%s>\n", myPackedItem->name );
        }
        else {
            snprintf( ( char* )outPtr, myStrlen + 5, "<%s>", myPackedItem->name );
        }
    }
    packedOutput->bBuf->len += strlen( ( char* )outPtr );

    return 0;
}

int
parseXmlValue( void **inPtr, packItem_t *myPackedItem, int *endTagLen ) {
    int status;
    int strLen = 0;

    if ( inPtr == NULL || *inPtr == NULL || myPackedItem == NULL ) {
        return 0;
    }

    status = parseXmlTag( inPtr, myPackedItem, START_TAG_FL, &strLen );
    if ( status >= 0 ) {
        /* set *inPtr to the beginning of the string value */
        *inPtr = ( char * ) * inPtr + status + strLen;
    }
    else {
        return status;
    }

    status = parseXmlTag( inPtr, myPackedItem, END_TAG_FL | LF_FL, &strLen );
    if ( status >= 0 ) {
        *endTagLen = status;
    }
    else {
        return status;
    }

    return strLen;
}

/* parseXmlTag - Parse the str given in *inPtr for the tag given in
 * myPackedItem->name.
 * The flag can be
 *    START_TAG_FL - look for <name>
 *    END_TAG_FL - look for </name>
 *    The LF_FL (line feed) can also be added START_TAG_FL or END_TAG_FL
 *    to skip a '\n" char after the tag.
 * Return the length of the tag, also put the number of char skipped to
 * reach the beginning og the tag in *skipLen
 */
int
parseXmlTag( void **inPtr, packItem_t *myPackedItem, int flag, int *skipLen ) {
    char *inStrPtr, *tmpPtr;
    int nameLen;
    int myLen = 0;

    inStrPtr = ( char * ) * inPtr;

    nameLen = strlen( myPackedItem->name );

    if ( flag & END_TAG_FL ) {
        /* end tag */
        char endTag[MAX_NAME_LEN];

        snprintf( endTag, MAX_NAME_LEN, "</%s>", myPackedItem->name );
        if ( ( tmpPtr = strstr( inStrPtr, endTag ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "parseXmlTag: XML end tag error for %s, expect </%s>",
                     *inPtr, myPackedItem->name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }

        *skipLen = tmpPtr - inStrPtr;

        myLen = nameLen + 3;
        inStrPtr = tmpPtr + nameLen + 3;
        if ( *inStrPtr == '\n' ) {
            myLen++;
        }
    }
    else {
        /* start tag */
        if ( ( tmpPtr = strstr( inStrPtr, "<" ) ) == NULL ) {
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
        *skipLen = tmpPtr - inStrPtr;
        inStrPtr = tmpPtr + 1;
        myLen++;

        if ( strncmp( inStrPtr, myPackedItem->name, nameLen ) != 0 ) {
            /* this can be normal */
            rodsLog( LOG_DEBUG1,
                     "parseXmlValue: XML start tag error for %s, expect <%s>",
                     *inPtr, myPackedItem->name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
        inStrPtr += nameLen;
        myLen += nameLen;

        if ( *inStrPtr != '>' ) {
            rodsLog( LOG_DEBUG1,
                     "parseXmlValue: XML start tag error for %s, expect <%s>",
                     *inPtr, myPackedItem->name );

            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }

        myLen++;

        inStrPtr ++;
        if ( ( flag & LF_FL ) && *inStrPtr == '\n' ) {
            myLen++;
        }
    }

    return myLen;
}

int
alignPackedOutput64( packedOutput_t *packedOutput ) {
    void *outPtr, *alignedOutPtr;

    if ( packedOutput->bBuf == NULL || packedOutput->bBuf->buf == NULL ||
            packedOutput->bBuf->len == 0 ) {
        return 0;
    }

    outPtr = ( void * )( ( char * ) packedOutput->bBuf->buf +
                         packedOutput->bBuf->len );

    alignedOutPtr = alignDouble( outPtr );

    if ( alignedOutPtr == outPtr ) {
        return 0;
    }

    if ( packedOutput->bBuf->len + 8 > packedOutput->bufSize ) {
        extendPackedOutput( packedOutput, 8, &outPtr );
    }
    packedOutput->bBuf->len = packedOutput->bBuf->len + 8 -
                              ( int )( ( char * ) alignedOutPtr - ( char * ) outPtr );

    return 0;
}

/* packNopackPointer - copy the char pointer in *inPtr into
 * packedOutput->nopackBufArray without packing. Pack the buffer index
 * into packedOutput as an integer.
 */

int
packNopackPointer( void **inPtr, packedOutput_t *packedOutput, int len,
                   packItem_t *myPackedItem, irodsProt_t irodsProt ) {
    int newNumBuf;
    int curNumBuf, *intPtr;
    bytesBuf_t *newBBufArray;
    int i;
    int status;

    curNumBuf = packedOutput->nopackBufArray.numBuf;
    if ( ( curNumBuf % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newNumBuf = curNumBuf + PTR_ARRAY_MALLOC_LEN;

        newBBufArray = ( bytesBuf_t * ) malloc( newNumBuf * sizeof( bytesBuf_t ) );
        memset( newBBufArray, 0, newNumBuf * sizeof( bytesBuf_t ) );
        for ( i = 0; i < curNumBuf; i++ ) {
            newBBufArray[i].len = packedOutput->nopackBufArray.bBufArray[i].len;
            newBBufArray[i].buf = packedOutput->nopackBufArray.bBufArray[i].buf;
        }
        if ( packedOutput->nopackBufArray.bBufArray != NULL ) {
            free( packedOutput->nopackBufArray.bBufArray );
        }
        packedOutput->nopackBufArray.bBufArray = newBBufArray;
    }
    packedOutput->nopackBufArray.bBufArray[curNumBuf].len = len;
    packedOutput->nopackBufArray.bBufArray[curNumBuf].buf = *inPtr;
    packedOutput->nopackBufArray.numBuf++;

    intPtr = ( int * )malloc( sizeof( int ) );
    *intPtr = curNumBuf;
    status = packInt( ( void ** )( static_cast< void * >( &intPtr ) ), packedOutput, 1, myPackedItem,
                      irodsProt );

    free( intPtr );
    if ( status < 0 ) {
        return status;
    }

    return 0;
}

/* ovStrcpy - overwrite outStr with inStr. inStr can be
 * part of outStr.
 */

int
ovStrcpy( char *outStr, char *inStr ) {
    int i;
    int len = strlen( inStr );

    for ( i = 0; i < len + 1; i++ ) {
        *outStr = *inStr;
        inStr++;
        outStr++;
    }

    return 0;
}
