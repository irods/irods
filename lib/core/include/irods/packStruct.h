#ifndef PACK_STRUCT_H__
#define PACK_STRUCT_H__

#include "irods/rodsDef.h"

#define MAX_PI_LEN                      1024            // max pack instruct length
#define SEMI_COL_FLAG                   0x2             // got semi colon at end
#define PACKED_OUT_ALLOC_SZ             (16 * 1024)     // initial alloc size for packedOutput
#define SUB_STRUCT_ALLOC_SZ             1024            // initial alloc size for unpacking sub struct
#define MAX_PACKED_OUT_ALLOC_SZ         (1024 * 1024)
#define NULL_PTR_PACK_STR               "%@#ANULLSTR$%"

// definition for the flag in packXmlTag()
#define START_TAG_FL                    0
#define END_TAG_FL                      1
#define LF_FL                           2 // line feed

// indicate the end of packing table
#define PACK_TABLE_END_PI               "PACK_TABLE_END_PI"

#define XML_TAG                         "iRODSStruct"

typedef struct PackingInstruction {
    const char *name;
    const char *packInstruct;
    void (*clearInStruct)(void*);
} packInstruct_t;

typedef struct PackingConstant {
    char *name;
    int value;
} packConstant_t;

// packType
typedef enum PackingTypeIndex {
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

// for the packOpr input in resolvePackedItem()
typedef enum PackingOperation {
    PACK_OPR,
    UNPACK_OPR
} packOpr_t;

typedef struct PackingType {
    char *name;                 // the Name of the type
    packTypeInx_t  number;      // the type number
    int size;                   // size in bytes of this type
} packType_t;

#define MAX_PACK_DIM    20

// definition for pointerType
#define NON_POINTER     0
#define A_POINTER       1
#define NO_FREE_POINTER 2
#define NO_PACK_POINTER 3

// definition for packFlag
#define FREE_POINTER    0x1     // free the pointer after packing

typedef struct packItem {
    packTypeInx_t typeInx;
    char *name;                     // The packing instruction
    int pointerType;                // See definition
    const void *pointer;            // The value of a pointer
    int intValue;                   // For int type only
    char strValue[NAME_LEN];        // For str type only
    int dim;                        // The dimension if it is an array
    int dimSize[MAX_PACK_DIM];      // The size of each dimension
    int hintDim;                    // The Hint dimension
    int hintDimSize[MAX_PACK_DIM];  // The size of each Hint dimension
    const struct packItem *parent;
    struct packItem *prev;
    struct packItem *next;
} packItem_t;

typedef struct BytesBufferArray {
    int numBuf;
    bytesBuf_t* bBufArray;  // pointer to an array of bytesBuf_t
} bytesBufArray_t;

typedef struct PackedOutput {
    bytesBuf_t bBuf;                    // Buffer (length => occupied memory)
    int bufSize;                        // Buffer capacity
    bytesBufArray_t nopackBufArray;     // bBuf for non packed buffer
} packedOutput_t;

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((deprecated("Use pack_struct")))
int packStruct(const void *inStruct,
               bytesBuf_t **packedResult,
               const char *packInstName,
               const packInstruct_t *myPackTable,
               int packFlag,
               irodsProt_t irodsProt);

__attribute__((deprecated("Use unpack_struct")))
int unpackStruct(const void *inPackStr,
                 void **outStruct,
                 const char *packInstName,
                 const packInstruct_t *myPackTable,
                 irodsProt_t irodsProt);

int pack_struct(const void *inStruct,
                bytesBuf_t **packedResult,
                const char *packInstName,
                const packInstruct_t *myPackTable,
                int packFlag,
                irodsProt_t irodsProt,
                const char* peer_version);

int unpack_struct(const void *inPackStr,
                  void **outStruct,
                  const char *packInstName,
                  const packInstruct_t *myPackTable,
                  irodsProt_t irodsProt,
                  const char* peer_version);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PACK_STRUCT_H__

