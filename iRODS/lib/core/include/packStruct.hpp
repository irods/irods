/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* packStruct.h - header file for packStruct.c
 */

#ifndef PACK_STRUCT_HPP
#define PACK_STRUCT_HPP

// =-=-=-=-=-=-=-
// boost includes
#include "rodsDef.h"

#define MAX_PI_LEN	1024	/* max pack instruct length */
#define SEMI_COL_FLAG	0x2	/* got semi colon at end */
#define PACKED_OUT_ALLOC_SZ (16*1024) /* initial alloc size for packedOutput */
#define SUB_STRUCT_ALLOC_SZ 1024 /* initial alloc size for unpacking sub
struct */
#define MAX_PACKED_OUT_ALLOC_SZ (1024*1024)
#define NULL_PTR_PACK_STR "%@#ANULLSTR$%"

/* definition for the flag in packXmlTag() */
#define START_TAG_FL	0
#define END_TAG_FL	1
#define LF_FL		2	/* line feed */

#ifdef __cplusplus
extern "C" {
#endif
typedef const char* packInstruct_t;

/* indicate the end of packing table */

#define PACK_TABLE_END_PI  "PACK_TABLE_END_PI"

#define XML_TAG		"iRODSStruct"
/* XXXXX should change (packInstruct_t *) to packInstruct_t. Then we don't
have to use globals */

typedef struct {
    const char *name;
    const char *packInstruct;
    void( *clearInStruct )( void* );
} packInstructArray_t;

typedef struct {
    char *name;
    int value;
} packConstantArray_t;

/* packType */
typedef enum {
    PACK_CHAR_TYPE,
    PACK_BIN_TYPE,
    PACK_STR_TYPE,
    PACK_PI_STR_TYPE,
    PACK_INT_TYPE,
    PACK_DOUBLE_TYPE,
    PACK_STRUCT_TYPE,
    PACK_DEPENDENT_TYPE,
    PACK_INT_DEPENDENT_TYPE,
    PACK_INT16_TYPE
} packTypeInx_t;

/* for the packOpr input in resolvePackedItem() */
typedef enum {
    PACK_OPR,
    UNPACK_OPR
} packOpr_t;

typedef struct {
    char *name;       	/* the Name of the type */
    packTypeInx_t  number;     /* the type number */
    int size;           /* size in bytes of this type */
} packType_t;

#define MAX_PACK_DIM	20

/* definition for pointerType */
#define NON_POINTER	0
#define A_POINTER	1
#define NO_FREE_POINTER 2
#define NO_PACK_POINTER 3

/* definition for packFlag */
#define FREE_POINTER	0x1	/* free the pointer after packing */

typedef struct packItem {
    packTypeInx_t typeInx;
    char *name;
    int pointerType;	/* see definition */
    void *pointer;	/* the value of a pointer */
    int intValue;	/* for int type only */
    char strValue[NAME_LEN];	/* for str type only */
    int dim;		/* the dimension if it is an array */
    int dimSize[MAX_PACK_DIM];	/* the size of each dimension */
    int hintDim;		/* the Hint dimension */
    int hintDimSize[MAX_PACK_DIM];	/* the size of each Hint dimension */
    struct packItem *parent;
    struct packItem *prev;
    struct packItem *next;
} packItem_t;

typedef struct {
    int numBuf;
    bytesBuf_t *bBufArray;	/* pointer to an array of bytesBuf_t */
} bytesBufArray_t;

typedef struct {
    bytesBuf_t *bBuf;
    int bufSize;
    bytesBufArray_t nopackBufArray;	/* bBuf for non packed buffer */
} packedOutput_t;

int
packStruct( void *inStruct, bytesBuf_t **packedResult, const char *packInstName,
            const packInstructArray_t *myPackTable, int packFlag, irodsProt_t irodsProt );

int
unpackStruct( void *inPackStr, void **outStruct, const char *packInstName,
              const packInstructArray_t *myPackTable, irodsProt_t irodsProt );
int
parsePackInstruct( const char *packInstruct, packItem_t **packItemHead );
int
copyStrFromPiBuf( const char **inBuf, char *outBuf, int dependentFlag );
int
packTypeLookup( char *typeName );

void *alignAddrToBoundary( void *ptr, int boundary );
void *alignInt( void *ptr );
void *alignInt16( void *ptr );
void *alignDouble( void *ptr );
void *ialignAddr( void *ptr );
int
initPackedOutput( packedOutput_t *packedOutput, int len );
int
initPackedOutputWithBuf( packedOutput_t *packedOutput, void *buf, int len );
int
resolvePackedItem( packItem_t *myPackedItem, void **inPtr,
                   packOpr_t packOpr );
int
resolveIntDepItem( packItem_t *myPackedItem );
int
resolveIntInItem( const char *name, packItem_t *myPackedItem );
const void *
matchPackInstruct( char *name, const packInstructArray_t *myPackTable );
int
resolveDepInArray( packItem_t *myPackedItem );
int
getNumElement( packItem_t *myPackedItem );
int
getNumHintElement( packItem_t *myPackedItem );
int
extendPackedOutput( packedOutput_t *packedOutput, int extLen, void **outPtr );
int
packItem( packItem_t *myPackedItem, void **inPtr,
          packedOutput_t *packedOutput, const packInstructArray_t *myPackTable,
          int packFlag, irodsProt_t irodsProt );
int
packPointerItem( packItem_t *myPackedItem, packedOutput_t *packedOutput,
                 const packInstructArray_t *myPackTable, int packFlag, irodsProt_t irodsProt );
int
packNonpointerItem( packItem_t *myPackedItem, void **inPtr,
                    packedOutput_t *packedOutput, const packInstructArray_t *myPackTable,
                    int packFlag, irodsProt_t irodsProt );
int
packChar( void **inPtr, packedOutput_t *packedOutput, int len,
          packItem_t *myPackedItem, irodsProt_t irodsProt );
int
packString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen,
            packItem_t *myPackedItem, irodsProt_t irodsProt );
int
packNatString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen );
int
packXmlString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen,
               packItem_t *myPackedItem );
int
strToXmlStr( char *inStr, char **outXmlStr );
int
xmlStrToStr( char *inStr, int myLen );
int
packInt( void **inPtr, packedOutput_t *packedOutput, int numElement,
         packItem_t *myPackedItem, irodsProt_t irodsProt );
int
packInt16( void **inPtr, packedOutput_t *packedOutput, int numElement,
           packItem_t *myPackedItem, irodsProt_t irodsProt );
int
packDouble( void **inPtr, packedOutput_t *packedOutput, int numElement,
            packItem_t *myPackedItem, irodsProt_t irodsProt );
int
packChildStruct( void **inPtr, packedOutput_t *packedOutput,
                 packItem_t *myPackedItem, const packInstructArray_t *myPackTable, int numElement,
                 int packFlag, irodsProt_t irodsProt, char *packInstruct );
int
freePackedItem( packItem_t *packItemHead );
int
unpackItem( packItem_t *myPackedItem, void **inPtr,
            packedOutput_t *unpackedOutput, const packInstructArray_t *myPackTable,
            irodsProt_t irodsProt );
int
unpackNonpointerItem( packItem_t *myPackedItem, void **inPtr,
                      packedOutput_t *unpackedOutput, const packInstructArray_t *myPackTable,
                      irodsProt_t irodsProt );
int
unpackChar( void **inPtr, packedOutput_t *packedOutput, int len,
            packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackCharToOutPtr( void **inPtr, void **outPtr, int len,
                    packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackNatCharToOutPtr( void **inPtr, void **outPtr, int len );
int
unpackXmlCharToOutPtr( void **inPtr, void **outPtr, int len,
                       packItem_t *myPackedItem );
int
unpackString( void **inPtr, packedOutput_t *unpackedOutput, int maxStrLen,
              packItem_t *myPackedItem, irodsProt_t irodsProt, char **outStr );
int
unpackNatString( void **inPtr, packedOutput_t *packedOutput, int maxStrLen,
                 char **outStr );
int
unpackXmlString( void **inPtr, packedOutput_t *unpackedOutput, int maxStrLen,
                 packItem_t *myPackedItem, char **outStr );
int
unpackInt( void **inPtr, packedOutput_t *packedOutput, int numElement,
           packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackIntToOutPtr( void **inPtr, void **outPtr, int numElement,
                   packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackXmlIntToOutPtr( void **inPtr, void **outPtr, int numElement,
                      packItem_t *myPackedItem );
int
unpackNatIntToOutPtr( void **inPtr, void **outPtr, int numElement );
int
unpackInt16( void **inPtr, packedOutput_t *unpackedOutput, int numElement,
             packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackInt16ToOutPtr( void **inPtr, void **outPtr, int numElement,
                     packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackNatInt16ToOutPtr( void **inPtr, void **outPtr, int numElement );
int
unpackXmlInt16ToOutPtr( void **inPtr, void **outPtr, int numElement,
                        packItem_t *myPackedItem );
int
unpackXmlDoubleToOutPtr( void **inPtr, void **outPtr, int numElement,
                         packItem_t *myPackedItem );
int
unpackDouble( void **inPtr, packedOutput_t *unpackedOutput, int numElement,
              packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackDoubleToOutPtr( void **inPtr, void **outPtr, int numElement,
                      packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackNatDoubleToOutPtr( void **inPtr, void **outPtr, int numElement );
int
unpackChildStruct( void **inPtr, packedOutput_t *unpackedOutput,
                   packItem_t *myPackedItem, const packInstructArray_t *myPackTable, int numElement,
                   irodsProt_t irodsProt, char *packInstructInp );
int
unpackPointerItem( packItem_t *myPackedItem, void **inPtr,
                   packedOutput_t *unpackedOutput, const packInstructArray_t *myPackTable,
                   irodsProt_t irodsProt );
void *
addPointerToPackedOut( packedOutput_t *packedOutput, int len, void *pointer );
int
getStrLen( void *inPtr, int maxStrLen );
int
unpackStringToOutPtr( void **inPtr, void **outPtr, int maxStrLen,
                      packItem_t *myPackedItem, irodsProt_t irodsProt );
int
unpackNatStringToOutPtr( void **inPtr, void **outPtr, int maxStrLen );
int
unpackXmlStringToOutPtr( void **inPtr, void **outPtr, int maxStrLen,
                         packItem_t *myPackedItem );
int
iparseDependent( packItem_t *myPackedItem );
int
resolveStrInItem( packItem_t *myPackedItem );
int
packNullString( packedOutput_t *packedOutput );
int
unpackNullString( void **inPtr, packedOutput_t *unpackedOutput,
                  packItem_t *myPackedItem, irodsProt_t irodsProt );
int
getNumStrAndStrLen( packItem_t *myPackedItem, int *numStr, int *maxStrLen );
int
getAllocLenForStr( packItem_t *myPackedItem, void **inPtr, int numStr,
                   int maxStrLen );
int
packXmlTag( packItem_t *myPackedItem, packedOutput_t *packedOutput,
            int endFlag );
int
parseXmlValue( void **inPtr, packItem_t *myPackedItem, int *endTagLen );
int
parseXmlTag( void **inPtr, packItem_t *myPackedItem, int flag, int *skipLen );
int
alignPackedOutput64( packedOutput_t *packedOutput );
int
packNopackPointer( void **inPtr, packedOutput_t *packedOutput, int len,
                   packItem_t *myPackedItem, irodsProt_t irodsProt );
int
ovStrcpy( char *outStr, char *inStr );
#ifdef __cplusplus
}
#endif
#endif	/* PACK_STRUCT_H */
