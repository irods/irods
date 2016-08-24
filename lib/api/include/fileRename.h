#ifndef FILE_RENAME_HPP
#define FILE_RENAME_HPP

#include "rodsDef.h"
#include "rcConnect.h"

typedef struct {
    rodsHostAddr_t addr;
    char oldFileName[MAX_NAME_LEN];
    char newFileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
} fileRenameInp_t;
#define fileRenameInp_PI "struct RHostAddr_PI; str oldFileName[MAX_NAME_LEN]; str newFileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN];"

typedef struct {
    char file_name[ MAX_NAME_LEN ];
} fileRenameOut_t;
#define fileRenameOut_PI "str file_name[MAX_NAME_LEN];"

#ifdef __cplusplus
extern "C"
#endif
int rcFileRename( rcComm_t *conn, fileRenameInp_t *fileRenameInp, fileRenameOut_t** );

#endif
