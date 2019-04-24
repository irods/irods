/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* packStruct.c - routine for pack and unfolding C struct
 */


#include "packStruct.h"
#include "alignPointer.hpp"
#include "rodsLog.h"
#include "rcGlobalExtern.h"
#include "base64.h"
#include "rcMisc.h"

#include "irods_pack_table.hpp"
#include <iostream>
#include <sstream>
#include <string>

int
packStruct( const void *inStruct, bytesBuf_t **packedResult, const char *packInstName,
            const packInstruct_t *myPackTable, int packFlag, irodsProt_t irodsProt ) {
    if ( inStruct == NULL || packedResult == NULL || packInstName == NULL ) {
        rodsLog( LOG_ERROR,
                 "packStruct: Input error. One of the input is NULL" );
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    /* Initialize the packedOutput */
    packedOutput_t packedOutput = initPackedOutput(MAX_PACKED_OUT_ALLOC_SZ);

    packItem_t rootPackedItem{};
    rootPackedItem.name = strdup( packInstName );
    int status = packChildStruct( inStruct, packedOutput, rootPackedItem,
                              myPackTable, 1, packFlag, irodsProt, NULL );
    free( rootPackedItem.name );

    if ( status < 0 ) {
        free( packedOutput.bBuf.buf );
        return status;
    }

    if ( irodsProt == XML_PROT ) {
        void *outPtr;
        status = extendPackedOutput( packedOutput, 1, outPtr ); 
        if ( SYS_MALLOC_ERR == status ) {
            return status;
        }

        /* add a NULL termination */
        *static_cast<char*>(outPtr) = '\0';
        if ( getRodsLogLevel() >= LOG_DEBUG9 ) {
            printf( "packed XML: \n%s\n", ( char * ) packedOutput.bBuf.buf );
        }
    }

    *packedResult = (bytesBuf_t*)malloc(sizeof(**packedResult));
    **packedResult = packedOutput.bBuf;
    return 0;
}

int
unpackStruct( const void *inPackedStr, void **outStruct, const char *packInstName,
              const packInstruct_t *myPackTable, irodsProt_t irodsProt ) {
    if ( inPackedStr == NULL || packInstName == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackStruct: Input error. One of the input is NULL" );
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    /* Initialize the unpackedOutput */
    packedOutput_t unpackedOutput = initPackedOutput(PACKED_OUT_ALLOC_SZ);

    packItem_t rootPackedItem{};
    rootPackedItem.name = strdup( packInstName );
    int status = unpackChildStruct( inPackedStr, unpackedOutput, rootPackedItem,
                                myPackTable, 1, irodsProt, NULL );
    free( rootPackedItem.name );

    if ( status < 0 ) {
        free( unpackedOutput.bBuf.buf );
        return status;
    }

    *outStruct = unpackedOutput.bBuf.buf;

    return 0;
}

int
parsePackInstruct( const char *packInstruct, packItem_t &packItemHead ) {
    char buf[MAX_PI_LEN];
    packItem_t *myPackItem = &packItemHead;
    packItem_t *prevPackItem = NULL;
    const char *inptr = packInstruct;
    int gotTypeCast = 0;
    int gotItemName = 0;

    while ( copyStrFromPiBuf( inptr, buf, 0 ) > 0 ) {
        if ( !myPackItem ) {
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
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            /* queue it */
            gotTypeCast = 0;
            gotItemName = 0;
            if ( prevPackItem != NULL ) {
                prevPackItem->next = myPackItem;
                myPackItem->prev = prevPackItem;
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
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            myPackItem->typeInx = ( packTypeInx_t )packTypeLookup( buf );
            if ( myPackItem->typeInx < 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: packTypeLookup failed for %s", buf );
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            gotTypeCast = 1;
            int outLen = copyStrFromPiBuf( inptr, buf, 1 );
            if ( outLen <= 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: ? No variable following ? for %s",
                         packInstruct );
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
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
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            myPackItem->typeInx = ( packTypeInx_t )packTypeLookup( buf );
            if ( myPackItem->typeInx < 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: packTypeLookup failed for %s", buf );
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            gotTypeCast = 1;
            int outLen = copyStrFromPiBuf( inptr, buf, 0 );
            if ( outLen <= 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: ? No variable following ? for %s",
                         packInstruct );
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
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
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            continue;
        }
        else if ( strcmp( buf, "#" ) == 0 ) {  /* no pack pointer */
            myPackItem->pointerType = NO_PACK_POINTER;
            if ( gotTypeCast == 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: # position error for %s", packInstruct );
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            continue;
        }
        else if ( strcmp( buf,  "$" ) == 0 ) {  /* pointer but don't free */
            myPackItem->pointerType = NO_FREE_POINTER;
            if ( gotTypeCast == 0 || gotItemName > 0 ) {
                rodsLog( LOG_ERROR,
                         "parsePackInstruct: $ position error for %s", packInstruct );
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
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
                if ( myPackItem != &packItemHead ) {
                    free( myPackItem );
                }
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
            if ( myPackItem != &packItemHead ) {
                free( myPackItem );
            }
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
    }
    if ( myPackItem != NULL ) {
        rodsLog( LOG_ERROR,
                 "parsePackInstruct: Pack Instruction %s not properly terminated",
                 packInstruct );
        if ( myPackItem != &packItemHead ) {
            free( myPackItem );
        }
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    return 0;
}

/* copy the next string from the inBuf to putBuf and advance the inBuf pointer.
 * special char '*', ';' and '?' will be returned as a string.
 */
int
copyStrFromPiBuf( const char *&inBuf, char *outBuf, int dependentFlag ) {
    const char *inPtr = inBuf;
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
    inBuf = inPtr;

    return outLen;
}

int
packTypeLookup( const char *typeName ) {
    /*TODO: i and return type should be reconsidered as of
      packTypeInx_t */
    for ( int i = 0; i < NumOfPackTypes; i++ ) {
        if ( strcmp( typeName, packTypeTable[i].name ) == 0 ) {
            return i;
        }
    }
    return -1;
}

packedOutput_t
initPackedOutput( const int len ) {
    return {
        .bBuf={
            .buf=malloc(len),
            .len=0
        },
        .bufSize=len,
        .nopackBufArray={}
    };
}

packedOutput_t
initPackedOutputWithBuf( void *buf, const int len ) {
    return {
        .bBuf={
            .buf=buf,
            .len=0
        },
        .bufSize=len,
        .nopackBufArray={}
    };
}

int
resolvePackedItem( packItem_t &myPackedItem, const void *&inPtr, packOpr_t packOpr ) {
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

    const void *ptr = inPtr;
    if ( myPackedItem.pointerType > 0 ) {
        if ( packOpr == PACK_OPR ) {
            /* align the address */
            ptr = ialignAddr( ptr );
            if ( ptr != NULL ) {
                myPackedItem.pointer = *static_cast<const void *const *>(ptr);
                /* advance the pointer */
                ptr = static_cast<const char *>(ptr) + sizeof(void *);
            }
            else {
                myPackedItem.pointer = NULL;
            }
        }
    }

    if ( strlen( myPackedItem.name ) == 0 ) {
	if ( myPackedItem.pointerType == 0 || myPackedItem.pointer != NULL ) {
	    rodsLog( LOG_ERROR,
		     "resolvePackedItem: Cannot resolve %s",
		     myPackedItem.strValue );
	    return SYS_PACK_INSTRUCT_FORMAT_ERR;
	}

	/* NULL pointer of unknown type: pack it as a string pointer */
	free( myPackedItem.name );
	myPackedItem.name = strdup( "STR_PTR_PI" );
    }
    inPtr = ptr;

    return 0;
}

int
iparseDependent( packItem_t &myPackedItem ) {
    int status;

    if ( myPackedItem.typeInx == PACK_DEPENDENT_TYPE ) {
        status =  resolveStrInItem( myPackedItem );
    }
    else if ( myPackedItem.typeInx == PACK_INT_DEPENDENT_TYPE ) {
        status =  resolveIntDepItem( myPackedItem );
    }
    else {
        status = 0;
    }
    return status;
}

int
resolveIntDepItem( packItem_t &myPackedItem ) {
    char buf[MAX_NAME_LEN], myPI[MAX_NAME_LEN], *pfPtr = NULL;
    int endReached = 0, c = 0;
    int outLen = 0;
    int keyVal = 0, status = 0;

    const char* tmpPtr = myPackedItem.name;
    char* bufPtr = buf;

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
                     myPackedItem.name );
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
                                     myPackedItem.name );
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
                                 myPackedItem.name );
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

    packItem_t newPackedItem{};
    status = parsePackInstruct( myPI, newPackedItem );

    if ( status < 0 ) {
        freePackedItem( newPackedItem );
        rodsLog( LOG_ERROR,
                 "resolveIntDepItem: parsePackFormat error for =%s", myPI );
        return status;
    }

    /* reset the link and switch myPackedItem<->newPackedItem */
    free( myPackedItem.name );

    packItem_t *lastPackedItem = &newPackedItem;
    while (lastPackedItem->next) {
        lastPackedItem = lastPackedItem->next;
    }

    if ( newPackedItem.next ) {
        newPackedItem.next->prev = &myPackedItem;
    }
    newPackedItem.prev = myPackedItem.prev;
    lastPackedItem->next = myPackedItem.next;
    if ( myPackedItem.next ) {
        myPackedItem.next->prev = lastPackedItem;
    }

    myPackedItem = newPackedItem;

    return 0;
}

int
resolveIntInItem( const char *name, const packItem_t &myPackedItem ) {
    int i;

    if ( isAllDigit( name ) ) {
        return atoi( name );
    }

    /* check the current item chain first */

    const packItem_t *tmpPackedItem = myPackedItem.prev;

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
resolveStrInItem( packItem_t &myPackedItem ) {
    const char *name = myPackedItem.strValue;
    /* check the current item chain first */

    const packItem_t* tmpPackedItem = myPackedItem.prev;

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

    if ( tmpPackedItem == NULL ) {
        rodsLog( LOG_ERROR,
                 "resolveStrInItem: Cannot resolve %s in %s",
                 name, myPackedItem.name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    myPackedItem.typeInx = PACK_STRUCT_TYPE;
    free( myPackedItem.name );
    myPackedItem.name = strdup( tmpPackedItem->strValue );

    return 0;
}

const char *
matchPackInstruct( const char *name, const packInstruct_t *myPackTable ) {

    if ( myPackTable != NULL ) {
        for (int i = 0; strcmp( myPackTable[i].name, PACK_TABLE_END_PI ) != 0; ++i) {
            if ( strcmp( myPackTable[i].name, name ) == 0 ) {
                return myPackTable[i].packInstruct;
            }
        }
    }

    /* Try the Rods Global table */

    for(int i = 0; strcmp( RodsPackTable[i].name, PACK_TABLE_END_PI ) != 0; ++i ) {
        if ( strcmp( RodsPackTable[i].name, name ) == 0 ) {
            return RodsPackTable[i].packInstruct;
        }
    }

    /* Try the API table */
    irods::pack_entry_table& pk_tbl =  irods::get_pack_table();
    irods::pack_entry_table::iterator itr = pk_tbl.find( name );
    if ( itr != pk_tbl.end() ) {
        return itr->second.packInstruct.c_str();
    }

    rodsLog( LOG_ERROR,
             "matchPackInstruct: Cannot resolve %s",
             name );

    return NULL;
}

int
resolveDepInArray( packItem_t &myPackedItem ) {
    myPackedItem.dim = myPackedItem.hintDim = 0;
    char openSymbol = '\0';         // either '(', '[', or '\0' depending on whether we are
    // in a parenthetical or bracketed expression, or not

    std::string buffer;
    for ( size_t nameIndex = 0; '\0' != myPackedItem.name[ nameIndex ]; nameIndex++ ) {
        char c = myPackedItem.name[ nameIndex ];
        if ( '[' == c || '(' == c ) {
            if ( openSymbol ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: got %c inside %c for %s",
                         c, openSymbol, myPackedItem.name );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            else if ( ( '[' == c && myPackedItem.dim >= MAX_PACK_DIM ) ||
                      ( '(' == c && myPackedItem.hintDim >= MAX_PACK_DIM ) ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: dimension of %s larger than %d",
                         myPackedItem.name, MAX_PACK_DIM );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            openSymbol = c;
            myPackedItem.name[ nameIndex ] = '\0';  // isolate the name. may be used later
            buffer.clear();
        }
        else if ( ']' == c || ')' == c ) {
            if ( ( ']' == c && '[' != openSymbol ) ||
                    ( ')' == c && '(' != openSymbol ) ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: Got %c without %c for %s",
                         c, ( ']' == c ) ? '[' : '(', myPackedItem.name );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            else if ( buffer.empty() ) {
                rodsLog( LOG_ERROR, "resolveDepInArray: Empty %c%c in %s",
                         ( ']' == c ) ? '[' : '(', c, myPackedItem.name );
                return SYS_PACK_INSTRUCT_FORMAT_ERR;
            }
            openSymbol = '\0';

            int& dimSize = ( ']' == c ) ?
                           myPackedItem.dimSize[myPackedItem.dim++] :
                           myPackedItem.hintDimSize[myPackedItem.hintDim++];
            if ( ( dimSize = resolveIntInItem( buffer.c_str(), myPackedItem ) ) < 0 ) {
                rodsLog( LOG_ERROR, "resolveDepInArray:resolveIntInItem error for %s, intName=%s",
                         myPackedItem.name, buffer.c_str() );
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
packNonpointerItem( packItem_t &myPackedItem, const void *&inPtr,
                    packedOutput_t &packedOutput, const packInstruct_t *myPackTable,
                    int packFlag, irodsProt_t irodsProt ) {
    int i, status;

    int typeInx = myPackedItem.typeInx;
    int numElement = getNumElement( myPackedItem );
    int elementSz = packTypeTable[typeInx].size;
    int myTypeNum = packTypeTable[typeInx].number;

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        /* no need to align inPtr */

        status = packChar( inPtr, packedOutput, numElement * elementSz,
                           myPackedItem.name, myPackedItem.typeInx, irodsProt );
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

        myDim = myPackedItem.dim;
        if ( myDim <= 0 ) {
            /* NULL pterminated */
            maxStrLen = -1;
            numStr = 1;
        }
        else {
            maxStrLen = myPackedItem.dimSize[myDim - 1];
            numStr = numElement / maxStrLen;
        }

        /* save the str */
        if ( numStr == 1 && myTypeNum == PACK_PI_STR_TYPE ) {
            snprintf( myPackedItem.strValue, sizeof( myPackedItem.strValue ), "%s", static_cast<const char*>(inPtr) );
        }

        for ( i = 0; i < numStr; i++ ) {
            status = packString( inPtr, packedOutput, maxStrLen, myPackedItem.name,
                                 irodsProt );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "packNonpointerItem:strlen %d of %s > dim size %d,content:%s",
                         strlen( ( const char* )inPtr ), myPackedItem.name, maxStrLen, inPtr );
                return status;
            }
        }
        break;
    }
    case PACK_INT_TYPE:
        /* align inPtr to 4 bytes boundary. Will not align outPtr */

        inPtr = alignInt( inPtr );

        status = packInt( inPtr, packedOutput, numElement, myPackedItem.name,
                          irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem.intValue = status;
        break;

    case PACK_INT16_TYPE:
        /* align inPtr to 2 bytes boundary. Will not align outPtr */

        inPtr = alignInt16( inPtr );

        status = packInt16( inPtr, packedOutput, numElement, myPackedItem.name,
                            irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem.intValue = status;
        break;

    case PACK_DOUBLE_TYPE:
        /* align inPtr to 8 bytes boundary. Will not align outPtr */
#if defined(osx_platform)
        /* osx does not align */
        inPtr = alignInt( inPtr );
#else
        inPtr = alignDouble( inPtr );
#endif

        status = packDouble( inPtr, packedOutput, numElement, myPackedItem.name,
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
                 myTypeNum, myPackedItem.name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    return 0;
}

int
packPointerItem( packItem_t &myPackedItem, packedOutput_t &packedOutput,
                 const packInstruct_t *myPackTable, int packFlag, irodsProt_t irodsProt ) {
    int i, j, status;
    const void* singletonArray[1];
    const void *pointer;      /* working pointer */
    const void *origPtr;      /* for the purpose of freeing the original ptr */

    /* if it is a NULL pointer, just pack a NULL_PTR_PACK_STR string */
    if ( myPackedItem.pointer == NULL ) {
        if ( irodsProt == NATIVE_PROT ) {
            packNullString( packedOutput );
        }
        return 0;
    }

    int numElement = getNumHintElement( myPackedItem ); /* number of elements pointed to by one pointer */
    int myDim = myPackedItem.dim;
    int typeInx = myPackedItem.typeInx;
    int numPointer = getNumElement( myPackedItem );     /* number of pointers in the array of pointer */
    int elementSz = packTypeTable[typeInx].size;
    int myTypeNum = packTypeTable[typeInx].number;

    /* pointer is already aligned */
    const void *const *  pointerArray = (myDim == 0) ? singletonArray : static_cast<const void *const*>(myPackedItem.pointer);
    if ( myDim == 0 ) {
        singletonArray[0] = myPackedItem.pointer;
        if ( myTypeNum == PACK_PI_STR_TYPE ) {
            /* save the str */
            snprintf( myPackedItem.strValue, sizeof( myPackedItem.strValue ),
                      "%s", static_cast<const char*>(myPackedItem.pointer) );
        }
    }

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            if ( myPackedItem.pointerType == NO_PACK_POINTER ) {
                status = packNopackPointer( const_cast<void *>(pointer), packedOutput,
                                            numElement * elementSz, myPackedItem.name, irodsProt );
            }
            else {
                status = packChar( pointer, packedOutput,
                                   numElement * elementSz, myPackedItem.name, myPackedItem.typeInx, irodsProt );
            }
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem.pointerType == A_POINTER ) {
                free( const_cast<void*>(origPtr) );
            }
            if ( status < 0 ) {
                return status;
            }
        }
        if ( ( packFlag & FREE_POINTER )  &&
                myPackedItem.pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( const_cast<void**>(pointerArray) );
        }
        break;
    case PACK_STR_TYPE:
    case PACK_PI_STR_TYPE: {
        /* the size of the last dim is the max length of the string. Don't
         * want to pack the entire length, just to end of the string including
         * the NULL.
         */
        int maxStrLen, numStr, myHintDim;
        myHintDim = myPackedItem.hintDim;
        if ( myHintDim <= 0 ) {   /* just a pointer to a null terminated str */
            maxStrLen = -1;
            numStr = 1;
        }
        else {
            /* the size of the last hint dimension */
            maxStrLen = myPackedItem.hintDimSize[myHintDim - 1];
            if ( numElement <= 0 || maxStrLen <= 0 ) {
                return 0;
            }
            numStr = numElement / maxStrLen;
        }

        for ( j = 0; j < numPointer; j++ ) {
            origPtr = pointer = pointerArray[j];

            for ( i = 0; i < numStr; i++ ) {
                status = packString( pointer, packedOutput, maxStrLen,
                                     myPackedItem.name, irodsProt );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR,
                             "packPointerItem: strlen of %s > dim size, content: %s ",
                             myPackedItem.name, pointer );
                    return status;
                }
            }
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem.pointerType == A_POINTER ) {
                free( const_cast<void*>(origPtr) );
            }
        }
        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem.pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( const_cast<void**>(pointerArray) );
        }
        break;
    }
    case PACK_INT_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            status = packInt( pointer, packedOutput, numElement,
                              myPackedItem.name, irodsProt );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem.pointerType == A_POINTER ) {
                free( const_cast<void*>(origPtr) );
            }
            if ( status < 0 ) {
                return status;
            }
        }

        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem.pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( const_cast<void**>(pointerArray) );
        }
        break;

    case PACK_INT16_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            status = packInt16( pointer, packedOutput, numElement,
                                myPackedItem.name, irodsProt );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem.pointerType == A_POINTER ) {
                free( const_cast<void*>(origPtr) );
            }
            if ( status < 0 ) {
                return status;
            }
        }

        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem.pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( const_cast<void**>(pointerArray) );
        }
        break;

    case PACK_DOUBLE_TYPE:
        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];

            status = packDouble( pointer, packedOutput, numElement,
                                 myPackedItem.name, irodsProt );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem.pointerType == A_POINTER ) {
                free( const_cast<void*>(origPtr) );
            }
            if ( status < 0 ) {
                return status;
            }
        }

        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem.pointerType == A_POINTER &&
                numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( const_cast<void**>(pointerArray) );
        }
        break;

    case PACK_STRUCT_TYPE:
        /* no need to align boundary for struct */

        for ( i = 0; i < numPointer; i++ ) {
            origPtr = pointer = pointerArray[i];
            status = packChildStruct( pointer, packedOutput, myPackedItem,
                                      myPackTable, numElement, packFlag, irodsProt, NULL );
            if ( ( packFlag & FREE_POINTER ) &&
                    myPackedItem.pointerType == A_POINTER ) {
                free( const_cast<void*>(origPtr) );
            }
            if ( status < 0 ) {
                return status;
            }
        }
        if ( ( packFlag & FREE_POINTER ) &&
                myPackedItem.pointerType == A_POINTER
                && numPointer > 0 && myDim > 0 ) {
            /* Array of pointers */
            free( const_cast<void**>(pointerArray) );
        }
        break;

    default:
        rodsLog( LOG_ERROR,
                 "packNonpointerItem: Unknown type %d - %s ",
                 myTypeNum, myPackedItem.name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    return 0;
}

int
getNumElement( const packItem_t &myPackedItem ) {
    int numElement = 1;
    for ( int i = 0; i < myPackedItem.dim; i++ ) {
        numElement *= myPackedItem.dimSize[i];
    }
    return numElement;
}

int
getNumHintElement( const packItem_t &myPackedItem ) {
    int numElement = 1;
    for ( int i = 0; i < myPackedItem.hintDim; i++ ) {
        numElement *= myPackedItem.hintDimSize[i];
    }
    return numElement;
}

int
extendPackedOutput( packedOutput_t &packedOutput, int extLen, void *&outPtr ) {

    int newOutLen = packedOutput.bBuf.len + extLen;
    if ( newOutLen <= packedOutput.bufSize ) {
        outPtr = ( char * ) packedOutput.bBuf.buf + packedOutput.bBuf.len;
        return 0;
    }

    /* double the size */
    int newBufSize = packedOutput.bufSize + packedOutput.bufSize;
    if ( newBufSize <= newOutLen ||
            packedOutput.bufSize > MAX_PACKED_OUT_ALLOC_SZ ) {
        newBufSize = newOutLen + PACKED_OUT_ALLOC_SZ;
    }

    packedOutput.bBuf.buf = realloc( packedOutput.bBuf.buf, newBufSize );
    packedOutput.bufSize = newBufSize;

    if ( packedOutput.bBuf.buf == NULL ) {
        rodsLog( LOG_ERROR,
                 "extendPackedOutput: error malloc of size %d", newBufSize );
        outPtr = NULL;
        return SYS_MALLOC_ERR;
    }
    outPtr = static_cast<char*>(packedOutput.bBuf.buf) + packedOutput.bBuf.len;

    /* set the remaining to 0 */
    memset( outPtr, 0, newBufSize - packedOutput.bBuf.len );

    return 0;
}

int
packItem( packItem_t &myPackedItem, const void *&inPtr, packedOutput_t &packedOutput,
          const packInstruct_t *myPackTable, int packFlag, irodsProt_t irodsProt ) {
    int status;

    status = resolvePackedItem( myPackedItem, inPtr, PACK_OPR );
    if ( status < 0 ) {
        return status;
    }
    if ( myPackedItem.pointerType > 0 ) {
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
packChar( const void *&inPtr, packedOutput_t &packedOutput, int len,
          const char* name, const packTypeInx_t typeInx, irodsProt_t irodsProt ) {

    if ( len <= 0 ) {
        return 0;
    }

    if ( irodsProt == XML_PROT ) {
        packXmlTag( name, packedOutput, START_TAG_FL );
    }

    if ( irodsProt == XML_PROT &&
            packTypeTable[typeInx].number == PACK_BIN_TYPE ) {
        /* a bin pack type. encode it */
        unsigned long outlen;
        int status;

        outlen = 2 * len + 10;
        void *outPtr;
        extendPackedOutput( packedOutput, outlen, outPtr );
        if ( inPtr == NULL ) { /* A NULL pointer */
            /* Don't think we'll have this condition. just fill it with 0 */
            memset( outPtr, 0, len );
            packedOutput.bBuf.len += len;
        }
        else {
            status = base64_encode( static_cast<const unsigned char *>(inPtr), len,
                                    static_cast<unsigned char *>(outPtr), &outlen );
            if ( status < 0 ) {
                return status;
            }
            inPtr = static_cast<const char*>(inPtr) + len;
            packedOutput.bBuf.len += outlen;
        }
    }
    else {
        void *outPtr;
        extendPackedOutput( packedOutput, len, outPtr );
        if ( inPtr == NULL ) { /* A NULL pointer */
            /* Don't think we'll have this condition. just fill it with 0 */
            memset( outPtr, 0, len );
        }
        else {
            memcpy( outPtr, inPtr, len );
            inPtr = static_cast<const char *>(inPtr) + len;
        }
        packedOutput.bBuf.len += len;
    }

    if ( irodsProt == XML_PROT ) {
        packXmlTag( name, packedOutput, END_TAG_FL );
    }

    return 0;
}

int
packString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen,
            const char* name, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = packXmlString( inPtr, packedOutput, maxStrLen, name );
    }
    else {
        status = packNatString( inPtr, packedOutput, maxStrLen );
    }

    return status;
}

int
packNatString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen ) {
    int myStrlen;

    if ( inPtr == NULL ) {
        myStrlen = 0;
    }
    else {
        myStrlen = strlen( ( const char* )inPtr );
    }
    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    void *outPtr;
    int status = extendPackedOutput( packedOutput, myStrlen + 1, outPtr ); 
    if ( SYS_MALLOC_ERR == status ) {
        return status;
    }

    if ( myStrlen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( static_cast<char*>(outPtr), static_cast<const char*>(inPtr), myStrlen + 1 );
    }

    if ( maxStrLen > 0 ) {
        inPtr = ( const char * )inPtr + maxStrLen;
    }
    else {
        inPtr = ( const char * )inPtr + myStrlen + 1;
    }

    packedOutput.bBuf.len += ( myStrlen + 1 );

    return 0;
}

int
packXmlString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen,
               const char* name ) {

    if ( !inPtr ) {
        rodsLog( LOG_ERROR, "packXmlString :: null inPtr" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    int myStrlen = strlen( static_cast<const char*>(inPtr) );
    char *xmlStr = nullptr;
    int xmlLen = strToXmlStr( static_cast<const char*>(inPtr), xmlStr );
    if ( NULL == xmlStr ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "packXmlString :: null xmlStr" );
        return -1;
    }

    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        free( xmlStr );
        return USER_PACKSTRUCT_INPUT_ERR;
    }
    packXmlTag( name, packedOutput, START_TAG_FL );

    void *outPtr;
    int status = extendPackedOutput( packedOutput, xmlLen + 1, outPtr ); 
    if ( SYS_MALLOC_ERR == status ) {
        free( xmlStr );
        return status;
    }

    if ( xmlLen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( static_cast<char*>(outPtr), xmlStr, xmlLen + 1 );
    }

    if ( maxStrLen > 0 ) {
        inPtr = ( const char * )inPtr + maxStrLen;
    }
    else {
        inPtr = ( const char * )inPtr + xmlLen + 1;
    }

    packedOutput.bBuf.len += ( xmlLen );
    packXmlTag( name, packedOutput, END_TAG_FL );
    free( xmlStr );
    return 0;
}

int
strToXmlStr( const char *inStr, char *&outXmlStr ) {
    outXmlStr = NULL;
    if ( inStr == NULL ) {
        return 0;
    }

    std::size_t copy_from = 0;
    std::stringstream xml{};
    std::size_t i;
    for ( i = 0; inStr[i] != '\0'; ++i ) {
        switch(inStr[i]) {
            case '&':
                xml.write(inStr + copy_from, i - copy_from);
                copy_from = i + 1;
                xml << "&amp;";
                break;
            case '<':
                xml.write(inStr + copy_from, i - copy_from);
                copy_from = i + 1;
                xml << "&lt;";
                break;
            case '>':
                xml.write(inStr + copy_from, i - copy_from);
                copy_from = i + 1;
                xml << "&gt;";
                break;
            case '"':
                xml.write(inStr + copy_from, i - copy_from);
                copy_from = i + 1;
                xml << "&quot;";
                break;
            case '`':
                xml.write(inStr + copy_from, i - copy_from);
                copy_from = i + 1;
                xml << "&apos;";
                break;
        }
    }
    xml.write(inStr + copy_from, i - copy_from);
    outXmlStr = strdup(xml.str().c_str());

    return strlen( outXmlStr );

}

int
xmlStrToStr( const char *inStr, int len, char *&outStr ) {
    outStr = nullptr;
    if ( inStr == NULL || len <= 0 ) {
        return 0;
    }

    std::size_t copy_from = 0;
    std::stringstream s{};
    for (std::size_t i = 0; i < static_cast<std::size_t>(len); ++i ) {
        if (inStr[i] == '&') {
            s.write(inStr + copy_from, i - copy_from);
            if ( strncmp( inStr + i + 1, "amp;", 4 ) == 0 ) {
                s << '&';
                i += 4;
            } else if ( strncmp( inStr + i + 1, "lt;", 3 ) == 0 ) {
                s << '<';
                i += 3;
            } else if ( strncmp( inStr + i + 1, "gt;", 3 ) == 0 ) {
                s << '>';
                i += 3;
            } else if ( strncmp( inStr + i + 1, "quot;", 5 ) == 0 ) {
                s << '"';
                i += 5;
            } else if ( strncmp( inStr + i + 1, "apos;", 5 ) == 0 ) {
                s << '`';
                i += 5;
            } else {
                break;
            }
            copy_from = i + 1;
        }
    }
    s.write(inStr + copy_from, len - copy_from);
    outStr = strdup(s.str().c_str());

    return strlen( outStr );
}

int
packNullString( packedOutput_t &packedOutput ) {

    int myStrlen = strlen( NULL_PTR_PACK_STR );
    void *outPtr;
    int status = extendPackedOutput( packedOutput, myStrlen + 1, outPtr );
    if ( SYS_MALLOC_ERR == status ) {
        return status;
    }
    strncpy( static_cast<char*>(outPtr), NULL_PTR_PACK_STR, myStrlen + 1 );
    packedOutput.bBuf.len += ( myStrlen + 1 );
    return 0;
}

int
packInt( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
         const char* name, irodsProt_t irodsProt ) {

    if ( numElement == 0 ) {
        return 0;
    }

    int intValue = 0;

    if ( irodsProt == XML_PROT ) {
        if ( inPtr == NULL ) {
            /* pack nothing */
            return 0;
        }
        const int* inIntPtr = ( const int * )inPtr;
        intValue = *inIntPtr;
        for ( int i = 0; i < numElement; i++ ) {
            packXmlTag( name, packedOutput, START_TAG_FL );
            void *outPtr;
            extendPackedOutput( packedOutput, 12, outPtr );
            snprintf( static_cast<char*>(outPtr), 12, "%d", *inIntPtr );
            packedOutput.bBuf.len += ( strlen( static_cast<char*>(outPtr) ) );
            packXmlTag( name, packedOutput, END_TAG_FL );
            inIntPtr++;
        }
        inPtr = inIntPtr;
    }
    else {
        int* origIntPtr = ( int * ) malloc( sizeof( int ) * numElement );

        if ( inPtr == NULL ) {
            /* a NULL pointer, fill the array with 0 */
            memset( origIntPtr, 0, sizeof( int ) * numElement );
        }
        else {
            const int* inIntPtr = ( const int * )inPtr;
            intValue = *inIntPtr;
            int* tmpIntPtr = origIntPtr;
            for ( int i = 0; i < numElement; i++ ) {
                *tmpIntPtr = htonl( *inIntPtr );
                tmpIntPtr ++;
                inIntPtr ++;
            }
            inPtr = inIntPtr;
        }

        void *outPtr;

        extendPackedOutput( packedOutput, sizeof( int ) * numElement, outPtr );
        memcpy( outPtr, origIntPtr, sizeof( int ) * numElement );
        free( origIntPtr );
        packedOutput.bBuf.len += ( sizeof( int ) * numElement );

    }
    if ( intValue < 0 ) {
        /* prevent error exiting */
        intValue = 0;
    }

    return intValue;
}

int
packInt16( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
           const char* name, irodsProt_t irodsProt ) {
    short *tmpIntPtr, *origIntPtr;
    int i;
    void *outPtr;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    const short* inIntPtr = ( const short * )inPtr;

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
            packXmlTag( name, packedOutput, START_TAG_FL );
            extendPackedOutput( packedOutput, 12, outPtr );
            snprintf( static_cast<char*>(outPtr), 12, "%hi", *inIntPtr );
            packedOutput.bBuf.len += ( strlen( static_cast<char*>(outPtr) ) );
            packXmlTag( name, packedOutput, END_TAG_FL );
            inIntPtr++;
        }
        inPtr = inIntPtr;
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
            inPtr = inIntPtr;
        }

        extendPackedOutput( packedOutput, sizeof( short ) * numElement, outPtr );
        memcpy( outPtr, origIntPtr, sizeof( short ) * numElement );
        free( origIntPtr );
        packedOutput.bBuf.len += ( sizeof( short ) * numElement );

    }
    if ( intValue < 0 ) {
        /* prevent error exiting */
        intValue = 0;
    }

    return intValue;
}

int
packDouble( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
            const char* name, irodsProt_t irodsProt ) {
    rodsLong_t *tmpDoublePtr, *origDoublePtr;
    int i;
    void *outPtr;

    if ( numElement == 0 ) {
        return 0;
    }

    const rodsLong_t* inDoublePtr = ( const rodsLong_t * )inPtr;

    if ( irodsProt == XML_PROT ) {
        if ( inDoublePtr == NULL ) {
            /* pack nothing */
            return 0;
        }
        for ( i = 0; i < numElement; i++ ) {
            packXmlTag( name, packedOutput, START_TAG_FL );
            extendPackedOutput( packedOutput, 20, outPtr );
            snprintf( static_cast<char*>(outPtr), 20, "%lld", *inDoublePtr );
            packedOutput.bBuf.len += ( strlen( static_cast<char*>(outPtr) ) );
            packXmlTag( name, packedOutput, END_TAG_FL );
            inDoublePtr++;
        }
        inPtr = inDoublePtr;
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
            inPtr = inDoublePtr;
        }
        extendPackedOutput( packedOutput, sizeof( rodsLong_t ) * numElement,
                            outPtr );
        memcpy( outPtr, origDoublePtr, sizeof( rodsLong_t ) * numElement );
        free( origDoublePtr );
        packedOutput.bBuf.len += ( sizeof( rodsLong_t ) * numElement );
    }

    return 0;
}

int
packChildStruct( const void *&inPtr, packedOutput_t &packedOutput,
                 const packItem_t &myPackedItem, const packInstruct_t *myPackTable, int numElement,
                 int packFlag, irodsProt_t irodsProt, const char *packInstructInp ) {

    if ( numElement == 0 ) {
        return 0;
    }

    if ( packInstructInp == NULL ) {
        packInstructInp = matchPackInstruct( myPackedItem.name, myPackTable );
    }

    if ( packInstructInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "packChildStruct: matchPackInstruct failed for %s",
                 myPackedItem.name );
        return SYS_UNMATCH_PACK_INSTRUCTI_NAME;
    }

    for ( int i = 0; i < numElement; i++ ) {
        packItem_t packItemHead{};

        int status = parsePackInstruct(packInstructInp, packItemHead );
        if ( status < 0 ) {
            freePackedItem( packItemHead );
            return status;
        }
        /* link it */
        packItemHead.parent = &myPackedItem;

        if ( irodsProt == XML_PROT ) {
            packXmlTag( myPackedItem.name, packedOutput, START_TAG_FL | LF_FL );
        }

        /* now pack each child item */
        packItem_t* tmpItem = &packItemHead;
        while ( tmpItem != NULL ) {
#if defined(solaris_platform)
            if ( tmpItem->pointerType == 0 &&
                    packTypeTable[tmpItem->typeInx].number == PACK_DOUBLE_TYPE ) {
                doubleInStruct = 1;
            }
#endif
            int status = packItem( *tmpItem, inPtr, packedOutput,
                               myPackTable, packFlag, irodsProt );
            if ( status < 0 ) {
                return status;
            }
            tmpItem = tmpItem->next;
        }
        freePackedItem( packItemHead );
#if defined(solaris_platform)
        /* seems that solaris align to 64 bit boundary if there is any
         * double in struct */
        if ( doubleInStruct > 0 ) {
            inPtr = alignDouble( inPtr );
        }
#endif
        if ( irodsProt == XML_PROT ) {
            packXmlTag( myPackedItem.name, packedOutput, END_TAG_FL );
        }
    }
    return 0;
}

int
freePackedItem( packItem_t &packItemHead ) {
    free( packItemHead.name );
    packItem_t *tmpItem = packItemHead.next;
    while ( tmpItem ) {
        packItem_t* nextItem = tmpItem->next;
        free( tmpItem->name );
        free( tmpItem );
        tmpItem = nextItem;
    }

    return 0;
}

int
unpackItem( packItem_t &myPackedItem, const void *&inPtr,
            packedOutput_t &unpackedOutput, const packInstruct_t *myPackTable,
            irodsProt_t irodsProt ) {
    int status;

    status = resolvePackedItem( myPackedItem, inPtr, UNPACK_OPR );
    if ( status < 0 ) {
        return status;
    }
    if ( myPackedItem.pointerType > 0 ) {
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
unpackNonpointerItem( packItem_t &myPackedItem, const void *&inPtr,
                      packedOutput_t &unpackedOutput, const packInstruct_t *myPackTable,
                      irodsProt_t irodsProt ) {
    int i, status = 0;

    int typeInx = myPackedItem.typeInx;
    int numElement = getNumElement( myPackedItem );
    int elementSz = packTypeTable[typeInx].size;
    int myTypeNum = packTypeTable[typeInx].number;

    switch ( myTypeNum ) {
    case PACK_CHAR_TYPE:
    case PACK_BIN_TYPE:
        status = unpackChar( inPtr, unpackedOutput, numElement * elementSz,
                             myPackedItem.name, myPackedItem.typeInx, irodsProt );
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

        myDim = myPackedItem.dim;
        if ( myDim <= 0 ) {
            /* null terminated */
            maxStrLen = -1;
            numStr = 1;
        }
        else {
            maxStrLen = myPackedItem.dimSize[myDim - 1];
            numStr = numElement / maxStrLen;
        }

        for ( i = 0; i < numStr; i++ ) {
            char *outStr = NULL;
            status = unpackString( inPtr, unpackedOutput, maxStrLen,
                                   myPackedItem.name, irodsProt, outStr );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "unpackNonpointerItem: strlen of %s > dim size, content: %s ",
                         myPackedItem.name, inPtr );
                return status;
            }
            if ( myTypeNum == PACK_PI_STR_TYPE && i == 0 && outStr != NULL ) {
                strncpy( myPackedItem.strValue, outStr, NAME_LEN );
            }
        }
        break;
    }
    case PACK_INT_TYPE:
        status = unpackInt( inPtr, unpackedOutput, numElement,
                            myPackedItem.name, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem.intValue = status;
        break;
    case PACK_INT16_TYPE:
        status = unpackInt16( inPtr, unpackedOutput, numElement,
                              myPackedItem.name, irodsProt );
        if ( status < 0 ) {
            return status;
        }
        myPackedItem.intValue = status;
        break;
    case PACK_DOUBLE_TYPE:
        status = unpackDouble( inPtr, unpackedOutput, numElement,
                               myPackedItem.name, irodsProt );
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
                 myTypeNum, myPackedItem.name );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    return status;
}
/* unpackChar - This routine functionally is the same as packChar */

int
unpackChar( const void *&inPtr, packedOutput_t &unpackedOutput, int len,
            const char* name, const packTypeInx_t typeInx, irodsProt_t irodsProt ) {
    void *outPtr;

    if ( len <= 0 ) {
        return 0;
    }

    /* no need to align address */

    extendPackedOutput( unpackedOutput, len, outPtr );
    if ( inPtr == NULL ) {     /* A NULL pointer */
        /* just fill it with 0 */
        memset( outPtr, 0, len );
    }
    else {
        unpackCharToOutPtr( inPtr, outPtr, len, name, typeInx, irodsProt );
    }
    unpackedOutput.bBuf.len += len;

    return 0;
}

int
unpackCharToOutPtr( const void *&inPtr, void *&outPtr, int len,
                    const char* name, const packTypeInx_t typeInx, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlCharToOutPtr( inPtr, outPtr, len, name, typeInx );
    }
    else {
        status = unpackNatCharToOutPtr( inPtr, outPtr, len );
    }
    return status;
}

int
unpackNatCharToOutPtr( const void *&inPtr, void *&outPtr, int len ) {
    memcpy( outPtr, inPtr, len );
    inPtr = static_cast<const char*>(inPtr) + len;
    outPtr = static_cast<char*>(outPtr) + len;

    return 0;
}

int
unpackXmlCharToOutPtr( const void *&inPtr, void *&outPtr, int len,
                       const char* name, const packTypeInx_t typeInx ) {
    int endTagLen = 0;
    int inLen = parseXmlValue( inPtr, name, endTagLen );
    if ( inLen < 0 ) {
        return inLen;
    }

    if ( packTypeTable[typeInx].number == PACK_BIN_TYPE ) {
        /* bin type. need to decode */
        unsigned long outLen = len;
        if ( int status = base64_decode( static_cast<const unsigned char *>(inPtr), inLen,
                    static_cast<unsigned char *>(outPtr), &outLen ) ) {
            return status;
        }
        if ( static_cast<int>(outLen) != len ) {
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
        memcpy( outPtr, inPtr, inLen );
    }
    inPtr = static_cast<const char*>(inPtr) + inLen + endTagLen;
    outPtr = static_cast<char*>(outPtr) + len;

    return 0;
}

int
unpackString( const void *&inPtr, packedOutput_t &unpackedOutput, int maxStrLen,
              const char* name, irodsProt_t irodsProt, char *&outStr ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlString( inPtr, unpackedOutput, maxStrLen,
                                  name, outStr );
    }
    else {
        status = unpackNatString( inPtr, unpackedOutput, maxStrLen, outStr );
    }
    return status;
}

int
unpackNatString( const void *&inPtr, packedOutput_t &unpackedOutput, int maxStrLen,
                 char *&outStr ) {
    int myStrlen = inPtr ? strlen( static_cast<const char*>(inPtr) ) : 0;
    int extLen = maxStrLen;
    void *outPtr;
    if ( myStrlen + 1 >= maxStrLen ) {
        if ( maxStrLen >= 0 ) {
            return USER_PACKSTRUCT_INPUT_ERR;
        }
        else {
            extLen = myStrlen + 1;
        }
    }

    int status = extendPackedOutput( unpackedOutput, extLen, outPtr );
    if ( SYS_MALLOC_ERR == status ) {
        return status;
    }

    if ( myStrlen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( static_cast<char*>(outPtr), static_cast<const char*>(inPtr), myStrlen + 1 );
        outStr = static_cast<char*>(outPtr);
    }

    if ( maxStrLen > 0 ) {
        inPtr = static_cast<const char*>(inPtr) + ( myStrlen + 1 );
        unpackedOutput.bBuf.len += maxStrLen;
    }
    else {
        inPtr = static_cast<const char*>(inPtr) + ( myStrlen + 1 );
        unpackedOutput.bBuf.len += myStrlen + 1;
    }

    return 0;
}

int
unpackXmlString( const void *&inPtr, packedOutput_t &unpackedOutput, int maxStrLen,
                 const char* name, char *&outStr ) {
    int myStrlen;
    void *outPtr;

    int endTagLen;
    int origStrLen = parseXmlValue( inPtr, name, endTagLen );
    if ( origStrLen < 0 ) {
        return origStrLen;
    }

    int extLen = maxStrLen;
    char* strBuf;
    myStrlen = xmlStrToStr( ( const char * )inPtr, origStrLen, strBuf );

    if ( myStrlen >= maxStrLen ) {
        if ( maxStrLen >= 0 ) {
            free(strBuf);
            return USER_PACKSTRUCT_INPUT_ERR;
        }
        else {
            extLen = myStrlen;
        }
    }

    int status = extendPackedOutput( unpackedOutput, extLen, outPtr ); 
    if ( SYS_MALLOC_ERR == status ) {
        free( strBuf );
        return status;
    }

    if ( myStrlen > 0 ) {
        strncpy( static_cast<char*>(outPtr), strBuf, myStrlen );
        outStr = static_cast<char*>(outPtr);
        outPtr = static_cast<char*>(outPtr) + myStrlen;
    }
    *static_cast<char*>(outPtr) = '\0';
    free(strBuf);

    inPtr = static_cast<const char*>(inPtr) + ( origStrLen + 1 );
    if ( maxStrLen > 0 ) {
        unpackedOutput.bBuf.len += maxStrLen;
    }
    else {
        unpackedOutput.bBuf.len += myStrlen + 1;
    }

    return 0;
}

int
unpackStringToOutPtr( const void *&inPtr, void *&outPtr, int maxStrLen,
                      const char* name, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlStringToOutPtr( inPtr, outPtr, maxStrLen,
                                          name );
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
unpackNullString( const void *&inPtr, packedOutput_t &unpackedOutput,
                  const packItem_t &myPackedItem, irodsProt_t irodsProt ) {

    if ( inPtr == NULL ) {
        /* add a null pointer */
        addPointerToPackedOut( unpackedOutput, 0, NULL );
        return 0;
    }

    const char *myPtr = ( const char* )inPtr;
    if ( irodsProt == XML_PROT ) {

        /* check if tag exists */
        int skipLen = 0;
        int tagLen = parseXmlTag( inPtr, myPackedItem.name, START_TAG_FL,
                              skipLen );
        if ( tagLen < 0 ) {
            addPointerToPackedOut( unpackedOutput, 0, NULL );
            return 0;
        }
        else {
            myPtr = myPtr + tagLen + skipLen;
        }
    }
    else {
        if ( strcmp( ( const char* )inPtr, NULL_PTR_PACK_STR ) == 0 ) {
            int myStrlen = strlen( NULL_PTR_PACK_STR );
            addPointerToPackedOut( unpackedOutput, 0, NULL );
            inPtr = ( const char * )inPtr + ( myStrlen + 1 );
            return 0;
        }
    }
    /* need to do more checking for null */
    int myDim = myPackedItem.dim;
    int numPointer = getNumElement( myPackedItem );
    int numElement = getNumHintElement( myPackedItem );

    if ( numElement <= 0 || ( myDim > 0 && numPointer <= 0 ) ) {
        /* add a null pointer */
        addPointerToPackedOut( unpackedOutput, 0, NULL );
        if ( irodsProt == XML_PROT ) {
            if ( strncmp( myPtr, "</", 2 ) == 0 ) {
                myPtr += 2;
                int nameLen = strlen( myPackedItem.name );
                if ( strncmp( myPtr, myPackedItem.name, nameLen ) == 0 ) {
                    myPtr += ( nameLen + 1 );
                    if ( *myPtr == '\n' ) {
                        myPtr++;
                    }
                    inPtr = myPtr;
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
unpackInt( const void *&inPtr, packedOutput_t &unpackedOutput, int numElement,
           const char* name, irodsProt_t irodsProt ) {
    if ( numElement == 0 ) {
        return 0;
    }

    void *outPtr;
    extendPackedOutput( unpackedOutput, sizeof( int ) * ( numElement + 1 ), outPtr );

    int intValue = unpackIntToOutPtr( inPtr, outPtr, numElement, name, irodsProt );

    /* adjust len */
    unpackedOutput.bBuf.len = static_cast<int>(static_cast<char*>(outPtr) - static_cast<char*>(unpackedOutput.bBuf.buf)) + (sizeof(int) * numElement);

    if ( intValue < 0 ) {
        /* prevent error exit */
        intValue = 0;
    }

    return intValue;
}

int
unpackIntToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                   const char* name, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlIntToOutPtr( inPtr, outPtr, numElement, name );
    }
    else {
        status = unpackNatIntToOutPtr( inPtr, outPtr, numElement );
    }
    return status;
}

int
unpackNatIntToOutPtr( const void *&inPtr, void *&outPtr, int numElement ) {
    int *tmpIntPtr, *origIntPtr;
    int i;
    int intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    const void* inIntPtr = inPtr;

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
            inIntPtr = ( const char * )inIntPtr + sizeof( int );
        }
        inPtr = inIntPtr;
    }

    /* align unpackedOutput to 4 bytes boundary. Will not align inPtr */

    outPtr = alignInt( outPtr );

    memcpy( outPtr, origIntPtr, sizeof( int ) * numElement );
    free( origIntPtr );

    return intValue;
}

int
unpackXmlIntToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                      const char* name ) {
    int *tmpIntPtr;
    int i;
    int myStrlen;
    int intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    /* align outPtr to 4 bytes boundary. Will not align inPtr */

    outPtr = tmpIntPtr = ( int* )alignInt( outPtr );

    if ( inPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( outPtr, 0, sizeof( int ) * numElement );
    }
    else {
        char tmpStr[NAME_LEN];

        for ( i = 0; i < numElement; i++ ) {
            int endTagLen = 0;
            myStrlen = parseXmlValue( inPtr, name, endTagLen );
            if ( myStrlen < 0 ) {
                return myStrlen;
            }
            else if ( myStrlen >= NAME_LEN ) {
                rodsLog( LOG_ERROR,
                         "unpackXmlIntToOutPtr: input %s with value %s too long",
                         name, inPtr );
                return USER_PACKSTRUCT_INPUT_ERR;
            }
            strncpy( tmpStr, ( const char* )inPtr, myStrlen );
            tmpStr[myStrlen] = '\0';

            *tmpIntPtr = atoi( tmpStr );
            if ( i == 0 ) {
                /* save this and return later */
                intValue = *tmpIntPtr;
            }
            tmpIntPtr ++;
            inPtr = ( const char * )inPtr + ( myStrlen + endTagLen );
        }
    }
    return intValue;
}

int
unpackInt16( const void *&inPtr, packedOutput_t &unpackedOutput, int numElement,
             const char* name, irodsProt_t irodsProt ) {
    void *outPtr;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    extendPackedOutput( unpackedOutput, sizeof( short ) * ( numElement + 1 ),
                        outPtr );

    intValue = unpackInt16ToOutPtr( inPtr, outPtr, numElement, name,
                                    irodsProt );

    /* adjust len */
    unpackedOutput.bBuf.len = static_cast<int>(static_cast<char*>(outPtr) - static_cast<char*>(unpackedOutput.bBuf.buf)) + (sizeof( short ) * numElement);

    if ( intValue < 0 ) {
        /* prevent error exit */
        intValue = 0;
    }

    return intValue;
}

int
unpackInt16ToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                     const char* name, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlInt16ToOutPtr( inPtr, outPtr, numElement, name );
    }
    else {
        status = unpackNatInt16ToOutPtr( inPtr, outPtr, numElement );
    }
    return status;
}

int
unpackNatInt16ToOutPtr( const void *&inPtr, void *&outPtr, int numElement ) {
    short *tmpIntPtr, *origIntPtr;
    int i;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    const void* inIntPtr = inPtr;

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
            inIntPtr = ( char * ) inIntPtr + sizeof( short );
        }
        inPtr = inIntPtr;
    }

    /* align unpackedOutput to 4 bytes boundary. Will not align inPtr */

    outPtr = alignInt16( outPtr );

    memcpy( outPtr, origIntPtr, sizeof( short ) * numElement );
    free( origIntPtr );

    return intValue;
}

int
unpackXmlInt16ToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                        const char* name ) {
    short *tmpIntPtr;
    int i;
    int myStrlen;
    short intValue = 0;

    if ( numElement == 0 ) {
        return 0;
    }

    /* align outPtr to 4 bytes boundary. Will not align inPtr */

    outPtr = tmpIntPtr = ( short* )alignInt16( outPtr );

    if ( inPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( outPtr, 0, sizeof( short ) * numElement );
    }
    else {
        char tmpStr[NAME_LEN];

        for ( i = 0; i < numElement; i++ ) {
            int endTagLen = 0;
            myStrlen = parseXmlValue( inPtr, name, endTagLen );
            if ( myStrlen < 0 ) {
                return myStrlen;
            }
            else if ( myStrlen >= NAME_LEN ) {
                rodsLog( LOG_ERROR,
                         "unpackXmlIntToOutPtr: input %s with value %s too long",
                         name, inPtr );
                return USER_PACKSTRUCT_INPUT_ERR;
            }
            strncpy( tmpStr, ( const char* )inPtr, myStrlen );
            tmpStr[myStrlen] = '\0';

            *tmpIntPtr = atoi( tmpStr );
            if ( i == 0 ) {
                /* save this and return later */
                intValue = *tmpIntPtr;
            }
            tmpIntPtr ++;
            inPtr = ( const char * )inPtr + ( myStrlen + endTagLen );
        }
    }
    return intValue;
}

int
unpackDouble( const void *&inPtr, packedOutput_t &unpackedOutput, int numElement,
              const char* name, irodsProt_t irodsProt ) {
    void *outPtr;

    if ( numElement == 0 ) {
        return 0;
    }

    extendPackedOutput( unpackedOutput, sizeof( rodsLong_t ) * ( numElement + 1 ),
                        outPtr );

    unpackDoubleToOutPtr( inPtr, outPtr, numElement, name, irodsProt );

    /* adjust len */
    unpackedOutput.bBuf.len = static_cast<int>(static_cast<char*>(outPtr) - static_cast<char*>(unpackedOutput.bBuf.buf)) + (sizeof(rodsLong_t) * numElement);

    return 0;
}

int
unpackDoubleToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                      const char* name, irodsProt_t irodsProt ) {
    int status;

    if ( irodsProt == XML_PROT ) {
        status = unpackXmlDoubleToOutPtr( inPtr, outPtr, numElement,
                                          name );
    }
    else {
        status = unpackNatDoubleToOutPtr( inPtr, outPtr, numElement );
    }
    return status;
}

int
unpackNatDoubleToOutPtr( const void *&inPtr, void *&outPtr, int numElement ) {
    rodsLong_t *tmpDoublePtr, *origDoublePtr;
    int i;

    if ( numElement == 0 ) {
        return 0;
    }

    const void* inDoublePtr = inPtr;

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
            inDoublePtr = ( const char * ) inDoublePtr + sizeof( rodsLong_t );
        }
        inPtr = inDoublePtr;
    }
    /* align inPtr to 8 bytes boundary. Will not align outPtr */

#if defined(osx_platform)
    /* osx does not align */
    outPtr = alignInt( outPtr );
#else
    outPtr = alignDouble( outPtr );
#endif

    memcpy( outPtr, origDoublePtr, sizeof( rodsLong_t ) * numElement );
    free( origDoublePtr );

    return 0;
}

int
unpackXmlDoubleToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                         const char* name ) {
    rodsLong_t *tmpDoublePtr;
    int i;
    int myStrlen;

    if ( numElement == 0 ) {
        return 0;
    }

    /* align inPtr to 8 bytes boundary. Will not align outPtr */

#if defined(osx_platform)
    /* osx does not align */
    outPtr = tmpDoublePtr = ( rodsLong_t* )alignInt( outPtr );
#else
    outPtr = tmpDoublePtr = ( rodsLong_t* )alignDouble( outPtr );
#endif

    if ( inPtr == NULL ) {
        /* a NULL pointer, fill the array with 0 */
        memset( outPtr, 0, sizeof( rodsLong_t ) * numElement );
    }
    else {
        for ( i = 0; i < numElement; i++ ) {
            char tmpStr[NAME_LEN];

            int endTagLen = 0;
            myStrlen = parseXmlValue( inPtr, name, endTagLen );
            if ( myStrlen < 0 ) {
                return myStrlen;
            }
            else if ( myStrlen >= NAME_LEN ) {
                rodsLog( LOG_ERROR,
                         "unpackXmlDoubleToOutPtr: input %s with value %s too long",
                         name, inPtr );
                return USER_PACKSTRUCT_INPUT_ERR;
            }
            strncpy( tmpStr, ( const char* )inPtr, myStrlen );
            tmpStr[myStrlen] = '\0';

            *tmpDoublePtr = strtoll( tmpStr, 0, 0 );
            tmpDoublePtr ++;
            inPtr = ( const char * )inPtr + ( myStrlen + endTagLen );
        }
    }

    return 0;
}

int
unpackChildStruct( const void *&inPtr, packedOutput_t &unpackedOutput,
                   const packItem_t &myPackedItem, const packInstruct_t *myPackTable, int numElement,
                   irodsProt_t irodsProt, const char *packInstructInp ) {
#if defined(solaris_platform)
    int doubleInStruct = 0;
#endif
#if defined(solaris_platform)
    void *outPtr1, *outPtr2;
#endif

    if ( numElement == 0 ) {
        return 0;
    }

    if ( packInstructInp == NULL ) {
        packInstructInp = matchPackInstruct( myPackedItem.name, myPackTable );
    }

    if ( packInstructInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackChildStruct: matchPackInstruct failed for %s",
                 myPackedItem.name );
        return SYS_UNMATCH_PACK_INSTRUCTI_NAME;
    }

    for ( int i = 0; i < numElement; i++ ) {
        packItem_t unpackItemHead{};

        int status = parsePackInstruct( packInstructInp, unpackItemHead );
        if ( status < 0 ) {
            freePackedItem( unpackItemHead );
            return status;
        }
        /* link it */
        unpackItemHead.parent = &myPackedItem;

        if ( irodsProt == XML_PROT ) {
            int skipLen = 0;
            int status = parseXmlTag( inPtr, myPackedItem.name, START_TAG_FL | LF_FL,
                                  skipLen );
            if ( status >= 0 ) {
                inPtr = ( const char * )inPtr + status + skipLen;
            }
            else {
                if ( myPackedItem.pointerType > 0 ) {
                    /* a null pointer */
                    addPointerToPackedOut( unpackedOutput, 0, NULL );
                    continue;
                }
                else {
                    return status;
                }
            }
        }

        /* now unpack each child item */

#if defined(solaris_platform)
        doubleInStruct = 0;
#endif
        packItem_t* tmpItem = &unpackItemHead;
        while ( tmpItem != NULL ) {
#if defined(solaris_platform)
            if ( tmpItem->pointerType == 0 &&
                    packTypeTable[tmpItem->typeInx].number == PACK_DOUBLE_TYPE ) {
                doubleInStruct = 1;
            }
#endif
            int status = unpackItem( *tmpItem, inPtr, unpackedOutput,
                                 myPackTable, irodsProt );
            if ( status < 0 ) {
                return status;
            }
            tmpItem = tmpItem->next;
        }
        freePackedItem( unpackItemHead );
#if defined(solaris_platform)
        /* seems that solaris align to 64 bit boundary if there is any
         * double in struct */
        if ( doubleInStruct > 0 ) {
            extendPackedOutput( unpackedOutput, sizeof( rodsLong_t ), &outPtr1 );
            outPtr2 = alignDouble( outPtr1 );
            unpackedOutput.bBuf.len += ( ( int ) outPtr2 - ( int ) outPtr1 );
        }
#endif
        if ( irodsProt == XML_PROT ) {
            int skipLen = 0;
            int status = parseXmlTag( inPtr, myPackedItem.name, END_TAG_FL | LF_FL,
                                  skipLen );
            if ( status >= 0 ) {
                inPtr = ( const char * )inPtr + status + skipLen;
            }
            else {
                return status;
            }
        }
    }
    return 0;
}

int
unpackPointerItem( packItem_t &myPackedItem, const void *&inPtr,
                   packedOutput_t &unpackedOutput, const packInstruct_t *myPackTable,
                   irodsProt_t irodsProt ) {
    int i = 0, j = 0, status = 0;
    void **pointerArray = nullptr;
    void *outPtr = nullptr;

    if ( unpackNullString( inPtr, unpackedOutput, myPackedItem, irodsProt )
            <= 0 ) {
        /* a null pointer and has been handled */
        return 0;
    }

    int myDim = myPackedItem.dim;
    int typeInx = myPackedItem.typeInx;
    int numPointer = getNumElement( myPackedItem );
    int numElement = getNumHintElement( myPackedItem );
    int elementSz = packTypeTable[typeInx].size;
    int myTypeNum = packTypeTable[typeInx].number;

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
                pointerArray = static_cast<void**>(addPointerToPackedOut(unpackedOutput, allocLen * elementSz, NULL));
            }
            else {
                pointerArray = static_cast<void**>(addPointerToPackedOut(unpackedOutput, allocLen * sizeof(void*), NULL));
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
            if ( myPackedItem.pointerType == NO_PACK_POINTER ) {
            }
            else {
                outPtr = addPointerToPackedOut( unpackedOutput,
                                                numElement * elementSz, NULL );
                status = unpackCharToOutPtr( inPtr, outPtr,
                                             numElement * elementSz, myPackedItem.name, myPackedItem.typeInx, irodsProt );
            }

            if ( status < 0 ) {
                return status;
            }
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                if ( myPackedItem.pointerType != NO_PACK_POINTER ) {
                    outPtr = pointerArray[i] = malloc( numElement * elementSz );
                    status = unpackCharToOutPtr( inPtr, outPtr,
                                                 numElement * elementSz, myPackedItem.name, myPackedItem.typeInx, irodsProt );
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

        getNumStrAndStrLen( myPackedItem, numStr, maxStrLen );

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
            myOutStr = static_cast<char*>(outPtr);
            for ( i = 0; i < numStr; i++ ) {
                status = unpackStringToOutPtr(
                             inPtr, outPtr, maxStrLen, myPackedItem.name, irodsProt );
                if ( status < 0 ) {
                    return status;
                }
                if ( myTypeNum == PACK_PI_STR_TYPE && i == 0 &&
                        myOutStr != NULL ) {
                    strncpy( myPackedItem.strValue, myOutStr, NAME_LEN );
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
                                 inPtr, outPtr, maxStrLen, myPackedItem.name, irodsProt );
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
            status = unpackIntToOutPtr( inPtr, outPtr, numElement,
                                        myPackedItem.name, irodsProt );
            /* don't chk status. It could be a -ive int */
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                outPtr = pointerArray[i] = malloc( numElement * elementSz );
                status = unpackIntToOutPtr( inPtr, outPtr,
                                            numElement * elementSz, myPackedItem.name, irodsProt );
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
            status = unpackInt16ToOutPtr( inPtr, outPtr, numElement,
                                          myPackedItem.name, irodsProt );
            /* don't chk status. It could be a -ive int */
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                outPtr = pointerArray[i] = malloc( numElement * elementSz );
                status = unpackInt16ToOutPtr( inPtr, outPtr,
                                              numElement * elementSz, myPackedItem.name, irodsProt );
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
            status = unpackDoubleToOutPtr( inPtr, outPtr, numElement,
                                           myPackedItem.name, irodsProt );
            /* don't chk status. It could be a -ive int */
        }
        else {
            /* pointer to an array of pointers */
            for ( i = 0; i < numPointer; i++ ) {
                outPtr = pointerArray[i] = malloc( numElement * elementSz );
                status = unpackDoubleToOutPtr( inPtr, outPtr,
                                               numElement * elementSz, myPackedItem.name, irodsProt );
                if ( status < 0 ) {
                    return status;
                }
            }
        }

        break;

    case PACK_STRUCT_TYPE: {
        /* no need to align boundary for struct */

        if ( myDim == 0 ) {
            /* we really don't know the size of each struct. */
            /* outPtr = addPointerToPackedOut (unpackedOutput,
               numElement * SUB_STRUCT_ALLOC_SZ); */
            outPtr = malloc( numElement * SUB_STRUCT_ALLOC_SZ );
            packedOutput_t subPackedOutput = initPackedOutputWithBuf( outPtr, numElement * SUB_STRUCT_ALLOC_SZ );
            status = unpackChildStruct( inPtr, subPackedOutput, myPackedItem, myPackTable, numElement, irodsProt, NULL );
            addPointerToPackedOut( unpackedOutput, numElement * SUB_STRUCT_ALLOC_SZ, subPackedOutput.bBuf.buf );
            subPackedOutput.bBuf.buf = NULL;
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
                packedOutput_t subPackedOutput = initPackedOutputWithBuf( outPtr, numElement * SUB_STRUCT_ALLOC_SZ );
                status = unpackChildStruct( inPtr, subPackedOutput, myPackedItem, myPackTable, numElement, irodsProt, NULL );
                pointerArray[i] = subPackedOutput.bBuf.buf;
                subPackedOutput.bBuf.buf = NULL;
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
                 myTypeNum, myPackedItem.name );

        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    return 0;
}

void *
addPointerToPackedOut( packedOutput_t &packedOutput, int len, void *pointer ) {
    void *outPtr;
    extendPackedOutput( packedOutput, sizeof(void*), outPtr );
    outPtr = ialignAddr( outPtr );

    void** tmpPtr = static_cast<void**>(outPtr);

    if ( pointer != NULL ) {
        *tmpPtr = pointer;
    }
    else if ( len > 0 ) {
        *tmpPtr = malloc( len );
        memset(*tmpPtr, 0, len);
    }
    else {
        /* add a NULL pointer */
        *tmpPtr = NULL;
    }

    packedOutput.bBuf.len = static_cast<int>( static_cast<char*>(outPtr) - static_cast<char*>(packedOutput.bBuf.buf)) + sizeof(void*);

    return *tmpPtr;
}

int
unpackNatStringToOutPtr( const void *&inPtr, void *&outPtr, int maxStrLen ) {
    int myStrlen;

    if ( inPtr == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackStringToOutPtr: NULL inPtr" );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }
    myStrlen = strlen( ( const char* )inPtr );

    /* maxStrLen = -1 means null terminated */
    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    rstrcpy( static_cast<char*>(outPtr), static_cast<const char*>(inPtr), myStrlen + 1 );

    inPtr = static_cast<const char*>(inPtr) + ( myStrlen + 1 );

    if ( maxStrLen >= 0 ) {
        outPtr = static_cast<char*>(outPtr) + maxStrLen;
    }
    else {
        outPtr = static_cast<char*>(outPtr) + ( myStrlen + 1 );
    }

    return 0;
}

int
unpackXmlStringToOutPtr( const void *&inPtr, void *&outPtr, int maxStrLen,
                         const char* name ) {

    if ( inPtr == NULL ) {
        rodsLog( LOG_ERROR,
                 "unpackXmlStringToOutPtr: NULL inPtr" );
        return SYS_PACK_INSTRUCT_FORMAT_ERR;
    }

    int endTagLen = 0;
    int origStrLen = parseXmlValue( inPtr, name, endTagLen );
    if ( origStrLen < 0 ) {
        return origStrLen;
    }

    char *myStrPtr;
    int myStrlen = xmlStrToStr( ( const char * )inPtr, origStrLen, myStrPtr );

    /* maxStrLen = -1 means null terminated */
    if ( maxStrLen >= 0 && myStrlen >= maxStrLen ) {
        return USER_PACKSTRUCT_INPUT_ERR;
    }

    if ( myStrlen == 0 ) {
        memset( outPtr, 0, 1 );
    }
    else {
        strncpy( static_cast<char*>(outPtr), myStrPtr, myStrlen + 1 );
    }

    inPtr = static_cast<const char*>(inPtr) + ( origStrLen + endTagLen );

    if ( maxStrLen >= 0 ) {
        outPtr = static_cast<char*>(outPtr) + maxStrLen;
    }
    else {
        outPtr = static_cast<char*>(outPtr) + ( myStrlen + 1 );
    }

    return 0;
}

/*
 * a maxStrLen of -1 means it is NULL terminated
 */

int
getNumStrAndStrLen( const packItem_t &myPackedItem, int& numStr, int& maxStrLen ) {

    int myHintDim = myPackedItem.hintDim;
    if ( myHintDim <= 0 ) {  /* just a pointer to a null terminated str */
        maxStrLen = -1;
        numStr = 1;
    }
    else {
        maxStrLen = myPackedItem.hintDimSize[myHintDim - 1];
        if ( maxStrLen <= 0 ) {
            numStr = 0;
        }
        else {
            numStr = getNumHintElement( myPackedItem ) / maxStrLen;
        }
    }

    return 0;
}

/* getAllocLenForStr - Get the alloc length for unpacking str pointer
 *
 * A -1 maxStrLen means NULL terminated
 */

int
getAllocLenForStr( const packItem_t &myPackedItem, const void *inPtr, int numStr,
                   int maxStrLen ) {
    int myLen;

    if ( numStr <= 1 ) {
        myLen = maxStrLen > 0 ? maxStrLen : strlen( (const char* ) inPtr ) + 1;
    }
    else {
        if ( maxStrLen < 0 ) {
            rodsLog( LOG_ERROR,
                     "unpackPointerItem: maxStrLen < 0 with numStr > 1 for %s",
                     myPackedItem.name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
        myLen = numStr * maxStrLen;
    }
    return myLen;
}

int
packXmlTag( const char* name, packedOutput_t &packedOutput,
            int flag ) {
    void *outPtr;

    /* +5 to include <>, '/', \n  and NULL */
    int myStrlen = strlen( name ) + 5;
    int status = extendPackedOutput( packedOutput, myStrlen, outPtr );
    if ( SYS_MALLOC_ERR == status ) {
        return status;
    }

    if ( flag & END_TAG_FL ) {
        snprintf( static_cast<char*>(outPtr), myStrlen, "</%s>\n", name );
    }
    else {
        if ( flag & LF_FL ) {
            snprintf( static_cast<char*>(outPtr), myStrlen, "<%s>\n", name );
        }
        else {
            snprintf( static_cast<char*>(outPtr), myStrlen, "<%s>", name );
        }
    }
    packedOutput.bBuf.len += strlen( static_cast<char*>(outPtr) );

    return 0;
}

int
parseXmlValue( const void *&inPtr, const char* name, int &endTagLen ) {

    if ( inPtr == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    int strLen = 0;
    int status = parseXmlTag( inPtr, name, START_TAG_FL, strLen );
    if ( status >= 0 ) {
        /* set inPtr to the beginning of the string value */
        inPtr = ( const char * ) inPtr + status + strLen;
    }
    else {
        return status;
    }

    strLen = 0;
    status = parseXmlTag( inPtr, name, END_TAG_FL | LF_FL, strLen );
    if ( status >= 0 ) {
        endTagLen = status;
    }
    else {
        return status;
    }

    return strLen;
}

/* parseXmlTag - Parse the str given in *inPtr for the tag given in
 * name.
 * The flag can be
 *    START_TAG_FL - look for <name>
 *    END_TAG_FL - look for </name>
 *    The LF_FL (line feed) can also be added START_TAG_FL or END_TAG_FL
 *    to skip a '\n" char after the tag.
 * Return the length of the tag, also put the number of char skipped to
 * reach the beginning og the tag in *skipLen
 */
int
parseXmlTag( const void *inPtr, const char* name, int flag, int &skipLen ) {
    const char *tmpPtr;
    int nameLen;
    int myLen = 0;

    const char* inStrPtr = ( const char * )inPtr;

    nameLen = strlen( name );

    if ( flag & END_TAG_FL ) {
        /* end tag */
        char endTag[MAX_NAME_LEN];

        snprintf( endTag, MAX_NAME_LEN, "</%s>", name );
        if ( ( tmpPtr = strstr( inStrPtr, endTag ) ) == NULL ) {
            rodsLog( LOG_ERROR,
                     "parseXmlTag: XML end tag error for %s, expect </%s>",
                     inPtr, name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }

        skipLen = tmpPtr - inStrPtr;

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
        skipLen = tmpPtr - inStrPtr;
        inStrPtr = tmpPtr + 1;
        myLen++;

        if ( strncmp( inStrPtr, name, nameLen ) != 0 ) {
            /* this can be normal */
            rodsLog( LOG_DEBUG10,
                     "parseXmlValue: XML start tag error for %s, expect <%s>",
                     inPtr, name );
            return SYS_PACK_INSTRUCT_FORMAT_ERR;
        }
        inStrPtr += nameLen;
        myLen += nameLen;

        if ( *inStrPtr != '>' ) {
            rodsLog( LOG_DEBUG10,
                     "parseXmlValue: XML start tag error for %s, expect <%s>",
                     inPtr, name );

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
alignPackedOutput64( packedOutput_t &packedOutput ) {
    void *outPtr, *alignedOutPtr;

    if ( packedOutput.bBuf.buf == NULL || packedOutput.bBuf.len == 0 ) {
        return 0;
    }

    outPtr = static_cast<char*>(packedOutput.bBuf.buf) + packedOutput.bBuf.len;

    alignedOutPtr = alignDouble( outPtr );

    if ( alignedOutPtr == outPtr ) {
        return 0;
    }

    if ( packedOutput.bBuf.len + 8 > packedOutput.bufSize ) {
        extendPackedOutput( packedOutput, 8, outPtr );
    }
    packedOutput.bBuf.len = packedOutput.bBuf.len + 8 - static_cast<int>(static_cast<char*>(alignedOutPtr) - static_cast<char*>(outPtr));

    return 0;
}

/* packNopackPointer - copy the char pointer in *inPtr into
 * packedOutput->nopackBufArray without packing. Pack the buffer index
 * into packedOutput as an integer.
 */

int
packNopackPointer( void *inPtr, packedOutput_t &packedOutput, int len,
                   const char* name, irodsProt_t irodsProt ) {
    int newNumBuf;
    int curNumBuf;
    bytesBuf_t *newBBufArray;
    int i;
    int status;

    curNumBuf = packedOutput.nopackBufArray.numBuf;
    if ( ( curNumBuf % PTR_ARRAY_MALLOC_LEN ) == 0 ) {
        newNumBuf = curNumBuf + PTR_ARRAY_MALLOC_LEN;

        newBBufArray = ( bytesBuf_t * ) malloc( newNumBuf * sizeof( bytesBuf_t ) );
        memset( newBBufArray, 0, newNumBuf * sizeof( bytesBuf_t ) );
        for ( i = 0; i < curNumBuf; i++ ) {
            newBBufArray[i].len = packedOutput.nopackBufArray.bBufArray[i].len;
            newBBufArray[i].buf = packedOutput.nopackBufArray.bBufArray[i].buf;
        }
        if ( packedOutput.nopackBufArray.bBufArray != NULL ) {
            free( packedOutput.nopackBufArray.bBufArray );
        }
        packedOutput.nopackBufArray.bBufArray = newBBufArray;
    }
    packedOutput.nopackBufArray.bBufArray[curNumBuf].len = len;
    packedOutput.nopackBufArray.bBufArray[curNumBuf].buf = inPtr;
    packedOutput.nopackBufArray.numBuf++;

    const void* intPtr = &curNumBuf;
    status = packInt( intPtr, packedOutput, 1, name,
                      irodsProt );

    if ( status < 0 ) {
        return status;
    }

    return 0;
}
