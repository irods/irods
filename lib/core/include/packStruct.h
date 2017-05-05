/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* packStruct.h - header file for packStruct.c
 */

#ifndef PACK_STRUCT_H__
#define PACK_STRUCT_H__

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

/* indicate the end of packing table */

#define PACK_TABLE_END_PI  "PACK_TABLE_END_PI"

#define XML_TAG		"iRODSStruct"

typedef struct {
    const char *name;
    const char *packInstruct;
    void( *clearInStruct )( void* );
} packInstruct_t;

typedef struct {
    char *name;
    int value;
} packConstant_t;

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
    const void *pointer;	/* the value of a pointer */
    int intValue;	/* for int type only */
    char strValue[NAME_LEN];	/* for str type only */
    int dim;		/* the dimension if it is an array */
    int dimSize[MAX_PACK_DIM];	/* the size of each dimension */
    int hintDim;		/* the Hint dimension */
    int hintDimSize[MAX_PACK_DIM];	/* the size of each Hint dimension */
    const struct packItem *parent;
    struct packItem *prev;
    struct packItem *next;
} packItem_t;

typedef struct {
    int numBuf;
    bytesBuf_t *bBufArray;	/* pointer to an array of bytesBuf_t */
} bytesBufArray_t;

typedef struct {
    bytesBuf_t bBuf;
    int bufSize;
    bytesBufArray_t nopackBufArray;	/* bBuf for non packed buffer */
} packedOutput_t;

#ifdef __cplusplus
extern "C" {
#endif

int
packStruct( const void *inStruct, bytesBuf_t **packedResult, const char *packInstName,
            const packInstruct_t *myPackTable, int packFlag, irodsProt_t irodsProt );

int
unpackStruct( const void *inPackStr, void **outStruct, const char *packInstName,
              const packInstruct_t *myPackTable, irodsProt_t irodsProt );
int
parsePackInstruct( const char *packInstruct, packItem_t &packItemHead );
int
copyStrFromPiBuf( const char *&inBuf, char *outBuf, int dependentFlag );
int
packTypeLookup( const char *typeName );

packedOutput_t
initPackedOutput( const int len );
packedOutput_t
initPackedOutputWithBuf( void *buf, const int len );
int
resolvePackedItem( packItem_t &myPackedItem, const void *&inPtr,
                   packOpr_t packOpr );
int
resolveIntDepItem( packItem_t &myPackedItem );
int
resolveIntInItem( const char *name, const packItem_t &myPackedItem );
const char *
matchPackInstruct( const char *name, const packInstruct_t *myPackTable );
int
resolveDepInArray( packItem_t &myPackedItem );
int
getNumElement( const packItem_t &myPackedItem );
int
getNumHintElement( const packItem_t &myPackedItem );
int
extendPackedOutput( packedOutput_t &packedOutput, int extLen, void *&outPtr );
int
packItem( packItem_t &myPackedItem, const void *&inPtr,
          packedOutput_t &packedOutput, const packInstruct_t *myPackTable,
          int packFlag, irodsProt_t irodsProt );
int
packPointerItem( packItem_t &myPackedItem, packedOutput_t &packedOutput,
                 const packInstruct_t *myPackTable, int packFlag, irodsProt_t irodsProt );
int
packNonpointerItem( packItem_t &myPackedItem, const void *&inPtr,
                    packedOutput_t &packedOutput, const packInstruct_t *myPackTable,
                    int packFlag, irodsProt_t irodsProt );
int
packChar( const void *&inPtr, packedOutput_t &packedOutput, int len,
          const char* name, const packTypeInx_t typeInx, irodsProt_t irodsProt );
int
packString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen,
            const char *name, irodsProt_t irodsProt );
int
packNatString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen );
int
packXmlString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen,
               const char *name );
int
strToXmlStr( const char *inStr, char *&outXmlStr );
int
xmlStrToStr( const char *inStr, int myLen, char*& outStr );
int
packInt( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
         const char *name, irodsProt_t irodsProt );
int
packInt16( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
           const char *name, irodsProt_t irodsProt );
int
packDouble( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
            const char *name, irodsProt_t irodsProt );
int
packChildStruct( const void *&inPtr, packedOutput_t &packedOutput,
                 const packItem_t &myPackedItem, const packInstruct_t *myPackTable, int numElement,
                 int packFlag, irodsProt_t irodsProt, const char *packInstruct );
int
freePackedItem( packItem_t &packItemHead );
int
unpackItem( packItem_t &myPackedItem, const void *&inPtr,
            packedOutput_t &unpackedOutput, const packInstruct_t *myPackTable,
            irodsProt_t irodsProt );
int
unpackNonpointerItem( packItem_t &myPackedItem, const void *&inPtr,
                      packedOutput_t &unpackedOutput, const packInstruct_t *myPackTable,
                      irodsProt_t irodsProt );
int
unpackChar( const void *&inPtr, packedOutput_t &packedOutput, int len,
            const char* name, const packTypeInx_t typeInx, irodsProt_t irodsProt );
int
unpackCharToOutPtr( const void *&inPtr, void *&outPtr, int len,
                    const char* name, const packTypeInx_t typeInx, irodsProt_t irodsProt );
int
unpackNatCharToOutPtr( const void *&inPtr, void *&outPtr, int len );
int
unpackXmlCharToOutPtr( const void *&inPtr, void *&outPtr, int len,
                       const char* name, const packTypeInx_t typeInx );
int
unpackString( const void *&inPtr, packedOutput_t &unpackedOutput, int maxStrLen,
              const char *name, irodsProt_t irodsProt, char *&outStr );
int
unpackNatString( const void *&inPtr, packedOutput_t &packedOutput, int maxStrLen,
                 char *&outStr );
int
unpackXmlString( const void *&inPtr, packedOutput_t &unpackedOutput, int maxStrLen,
                 const char *name, char *&outStr );
int
unpackInt( const void *&inPtr, packedOutput_t &packedOutput, int numElement,
           const char *name, irodsProt_t irodsProt );
int
unpackIntToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                   const char *name, irodsProt_t irodsProt );
int
unpackXmlIntToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                      const char *name );
int
unpackNatIntToOutPtr( const void *&inPtr, void *&outPtr, int numElement );
int
unpackInt16( const void *&inPtr, packedOutput_t &unpackedOutput, int numElement,
             const char *name, irodsProt_t irodsProt );
int
unpackInt16ToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                     const char *name, irodsProt_t irodsProt );
int
unpackNatInt16ToOutPtr( const void *&inPtr, void *&outPtr, int numElement );
int
unpackXmlInt16ToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                        const char *name );
int
unpackXmlDoubleToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                         const char *name );
int
unpackDouble( const void *&inPtr, packedOutput_t &unpackedOutput, int numElement,
              const char *name, irodsProt_t irodsProt );
int
unpackDoubleToOutPtr( const void *&inPtr, void *&outPtr, int numElement,
                      const char *name, irodsProt_t irodsProt );
int
unpackNatDoubleToOutPtr( const void *&inPtr, void *&outPtr, int numElement );
int
unpackChildStruct( const void *&inPtr, packedOutput_t &unpackedOutput,
                   const packItem_t &myPackedItem, const packInstruct_t *myPackTable, int numElement,
                   irodsProt_t irodsProt, const char *packInstructInp );
int
unpackPointerItem( packItem_t &myPackedItem, const void *&inPtr,
                   packedOutput_t &unpackedOutput, const packInstruct_t *myPackTable,
                   irodsProt_t irodsProt );
void *
addPointerToPackedOut( packedOutput_t &packedOutput, int len, void *pointer );
int
unpackStringToOutPtr( const void *&inPtr, void *&outPtr, int maxStrLen,
                      const char *name, irodsProt_t irodsProt );
int
unpackNatStringToOutPtr( const void *&inPtr, void *&outPtr, int maxStrLen );
int
unpackXmlStringToOutPtr( const void *&inPtr, void *&outPtr, int maxStrLen,
                         const char *name );
int
iparseDependent( packItem_t &myPackedItem );
int
resolveStrInItem( packItem_t &myPackedItem );
int
packNullString( packedOutput_t &packedOutput );
int
unpackNullString( const void *&inPtr, packedOutput_t &unpackedOutput,
                  const packItem_t &myPackedItem, irodsProt_t irodsProt );
int
getNumStrAndStrLen( const packItem_t &myPackedItem, int &numStr, int &maxStrLen );
int
getAllocLenForStr( const packItem_t &myPackedItem, const void *inPtr, int numStr,
                   int maxStrLen );
int
packXmlTag( const char *name, packedOutput_t &packedOutput,
            int endFlag );
int
parseXmlValue( const void *&inPtr, const char *name, int &endTagLen );
int
parseXmlTag( const void *inPtr, const char *name, int flag, int &skipLen );
int
alignPackedOutput64( packedOutput_t &packedOutput );
int
packNopackPointer( void *inPtr, packedOutput_t &packedOutput, int len,
                   const char *name, irodsProt_t irodsProt );
int
ovStrcpy( char *outStr, const char *inStr );
#ifdef __cplusplus
}
#endif
#endif	// PACK_STRUCT_H__
